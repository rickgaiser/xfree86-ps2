/*
 * gsxdispcur.c
 *
 * gsx cursor display routines
 * baeed on mi/midspcur.c
 */

/*

Copyright 2001 Sony Corporation.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the Sony Corporation shall not
be used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the Sony Corporation.

*/

/* $XConsortium: midispcur.c,v 5.14 94/04/17 20:27:28 dpw Exp $ */

/*

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
*/



#undef USE_NO_TILE_CT	// For debug only. Define to kill texture accel.

#define NEED_EVENTS
# include   "X.h"
# include   "misc.h"
# include   "input.h"
# include   "cursorstr.h"
# include   "windowstr.h"
# include   "regionstr.h"
# include   "dixstruct.h"
# include   "scrnintstr.h"
# include   "servermd.h"
#ifdef USE_NO_TILE_CT
#else
# include   "mipointer.h"
# include   "misprite.h"
#endif
# include   "gcstruct.h"
# include   "gsx.h"

#ifdef USE_NO_TILE_CT
#include "../../mi/midispcur.c"
#endif

#if GSX_ACCL_SPRITE_DC

extern WindowPtr    *WindowTable;

extern
Bool
gsxSpriteInitialize (
    ScreenPtr		    pScreen,
    gsxSpriteCursorFuncPtr   cursorFuncs,
    miPointerScreenFuncPtr  screenFuncs
);

/* per-screen private data */

static int	gsxDCScreenIndex;
static unsigned long gsxDCGeneration = 0;

static Bool	gsxDCCloseScreen();

typedef struct {
    CloseScreenProcPtr CloseScreen;
    int		realW, realH;	 // Realized cursor size
    pointer	lastCursorPriv;
    unsigned long  lastSourcePix, lastMaskPix;
} gsxDCScreenRec, *gsxDCScreenPtr;

/*
 * sprite/cursor method table
 */

static Bool	gsxDCRealizeCursor(),	    gsxDCUnrealizeCursor();
static Bool	gsxDCPutUpCursor(),	    gsxDCSaveUnderCursor();
static Bool	gsxDCRestoreUnderCursor(),  gsxDCMoveCursor();
static Bool	gsxDCChangeSave(),          gsxDCGetRealizedSize();

static
gsxSpriteCursorFuncRec
gsxDCFuncs = {
#ifdef USE_NO_TILE_CT
    miDCRealizeCursor,
    miDCUnrealizeCursor,
    miDCPutUpCursor,
    miDCSaveUnderCursor,
    miDCRestoreUnderCursor,
    miDCMoveCursor,
    miDCChangeSave,
#else
    gsxDCRealizeCursor,
    gsxDCUnrealizeCursor,
    gsxDCPutUpCursor,
    gsxDCSaveUnderCursor,
    gsxDCRestoreUnderCursor,
    gsxDCMoveCursor,
    gsxDCChangeSave,
#endif
    gsxDCGetRealizedSize
};

#ifdef USE_NO_TILE_CT
Bool
gsxDCInitialize (pScreen, screenFuncs)
    ScreenPtr               pScreen;
    miPointerScreenFuncPtr  screenFuncs;
{
    miDCScreenPtr   pScreenPriv;

    if (miDCGeneration != serverGeneration)
    {
        miDCScreenIndex = AllocateScreenPrivateIndex ();
        if (miDCScreenIndex < 0)
            return FALSE;
        miDCGeneration = serverGeneration;
    }
    pScreenPriv = (miDCScreenPtr) xalloc (sizeof (miDCScreenRec));
    if (!pScreenPriv)
        return FALSE;

    /*
     * initialize the entire private structure to zeros
     */

    pScreenPriv->pSourceGC =
        pScreenPriv->pMaskGC =
        pScreenPriv->pSaveGC =
        pScreenPriv->pRestoreGC =
        pScreenPriv->pMoveGC =
        pScreenPriv->pPixSourceGC =
        pScreenPriv->pPixMaskGC = NULL;
    
    pScreenPriv->pSave = pScreenPriv->pTemp = NULL;

    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = miDCCloseScreen;
    
    pScreen->devPrivates[miDCScreenIndex].ptr = (pointer) pScreenPriv;

    if (!gsxSpriteInitialize (pScreen, &gsxDCFuncs, screenFuncs))
    {
        xfree ((pointer) pScreenPriv);
        return FALSE;
    }
    return TRUE;
}

#else /* USE_NO_TILE_CT */

Bool
gsxDCInitialize (pScreen, screenFuncs)
    ScreenPtr		    pScreen;
    miPointerScreenFuncPtr  screenFuncs;
{
    gsxDCScreenPtr   pScreenPriv;

    if (gsxDCGeneration != serverGeneration)
    {
	gsxDCScreenIndex = AllocateScreenPrivateIndex ();
	if (gsxDCScreenIndex < 0)
	    return FALSE;
	gsxDCGeneration = serverGeneration;
    }
    pScreenPriv = (gsxDCScreenPtr) xalloc (sizeof (gsxDCScreenRec));
    if (!pScreenPriv)
	return FALSE;

    /*
     * initialize the entire private structure to zeros
     */


    pScreenPriv->lastCursorPriv = NULL;
    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = gsxDCCloseScreen;
    pScreen->devPrivates[gsxDCScreenIndex].ptr = (pointer) pScreenPriv;

    if (!gsxSpriteInitialize (pScreen, &gsxDCFuncs, screenFuncs))
    {
	xfree ((pointer) pScreenPriv);
	return FALSE;
    }
    return TRUE;
}

#endif /* USE_NO_TILE_CT */

static Bool
gsxDCCloseScreen (index, pScreen)
    ScreenPtr	pScreen;
{
    gsxDCScreenPtr   pScreenPriv;

    pScreenPriv = (gsxDCScreenPtr) pScreen->devPrivates[gsxDCScreenIndex].ptr;
    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    xfree ((pointer) pScreenPriv);
    return (*pScreen->CloseScreen) (index, pScreen);
}

static Bool
gsxDCRealizeCursor (
    ScreenPtr	pScreen,
    CursorPtr	pCursor
)
{
    if (pCursor->bits->refcnt <= 1) 
	pCursor->bits->devPriv[pScreen->myNum] = (pointer)NULL;
    return TRUE;
}

static inline pointer
gsxDCRealize (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    pointer   pPriv;

    pPriv = (pointer) pCursor;
    pCursor->bits->devPriv[pScreen->myNum] = (pointer) pPriv;

    return pPriv;
}

static Bool
gsxDCUnrealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    pointer   pPriv;
    gsxDCScreenPtr   pScreenPriv;

    pPriv = (pointer) pCursor->bits->devPriv[pScreen->myNum];
    if (pPriv && (pCursor->bits->refcnt <= 1))
    {
        pScreenPriv = (gsxDCScreenPtr)
		pScreen->devPrivates[gsxDCScreenIndex].ptr;
	if (pScreenPriv->lastCursorPriv == pPriv)
	    pScreenPriv->lastCursorPriv = NULL;

	pCursor->bits->devPriv[pScreen->myNum] = (pointer)NULL;
    }
    return TRUE;
}


static Bool
gsxDCPutUpCursor (pScreen, pCursor, x, y, source, mask)
    ScreenPtr	    pScreen;
    CursorPtr	    pCursor;
    int		    x, y;
    unsigned long   source, mask;
{
    gsxDCScreenPtr   pScreenPriv;
    pointer          pPriv;
    WindowPtr	     pWin;

    pPriv = (pointer) pCursor->bits->devPriv[pScreen->myNum];
    if (!pPriv)
    {
	pPriv = gsxDCRealize(pScreen, pCursor);
	if (!pPriv)
	    return FALSE;
    }
    pScreenPriv = (gsxDCScreenPtr) pScreen->devPrivates[gsxDCScreenIndex].ptr;

    if (pScreenPriv->lastCursorPriv != pPriv || 
    		pScreenPriv->lastSourcePix != source ||
		pScreenPriv->lastMaskPix != mask ) {

        // limit size and hostsopt of cursor and record these.
	gsxDCGetRealizedSize(pScreen, pCursor, 
			&pScreenPriv->realW, &pScreenPriv->realH,
			0, 0);

        // build texture image  and load it to GS
	gsxBuildCursorCT(pScreen,
			pCursor->bits->source, pCursor->bits->mask,
			pCursor->bits->width, pCursor->bits->height,
			pScreenPriv->realW, pScreenPriv->realH,
			source, mask);
		
        pScreenPriv->lastCursorPriv = pPriv;

        pScreenPriv->lastSourcePix = source;
        pScreenPriv->lastMaskPix = mask;
    }

    pWin = WindowTable[pScreen->myNum];

    // draw cursor
    gsxPutCursorBitsCT(pScreen, 
	pWin->drawable.x + x, pWin->drawable.y + y,
	pScreenPriv->realW, pScreenPriv->realH);
    return TRUE;

}

static Bool
gsxDCSaveUnderCursor (pScreen, x, y, w, h)
    ScreenPtr	pScreen;
    int		x, y, w, h;
{
    WindowPtr	    pWin;

    pWin = WindowTable[pScreen->myNum];

    gsxSaveDCAreaCT(pScreen, pWin->drawable.x + x, pWin->drawable.y + y, 
    			w,h, 0,0);
    return TRUE;

}

static Bool
gsxDCRestoreUnderCursor (pScreen, x, y, w, h)
    ScreenPtr	pScreen;
    int		x, y, w, h;
{
    WindowPtr	    pWin;

    pWin = WindowTable[pScreen->myNum];

    gsxRestoreDCAreaCT(pScreen, 0, 0, w, h, 
			pWin->drawable.x + x, pWin->drawable.y + y);
    return TRUE;

}

static Bool
gsxDCChangeSave (pScreen, x, y, w, h, dx, dy)
    ScreenPtr	    pScreen;
    int		    x, y, w, h, dx, dy;
{
    WindowPtr	    pWin;
    int		    sourcex, sourcey, destx, desty, copyw, copyh;


    pWin = WindowTable[pScreen->myNum];

    /*
     * copy the old bits to the screen.
     */
    if (dy > 0)
    {
	gsxRestoreDCAreaCT(pScreen, 
		0, h-dy,
		w, dy,
		pWin->drawable.x+x+dx, pWin->drawable.y+y+h);
    }
    else if (dy < 0)
    {
	gsxRestoreDCAreaCT(pScreen, 
		0, 0,
    		w, -dy,
		pWin->drawable.x+x+dx, pWin->drawable.y+y+dy);
    }
    if (dy >= 0)
    {
	desty = y + dy;
	sourcey = 0;
	copyh = h - dy;
    }
    else
    {
	desty = y;
	sourcey = - dy;
	copyh = h + dy;
    }
    if (dx > 0)
    {
	gsxRestoreDCAreaCT(pScreen, 
		w-dx, sourcey,
		dx, copyh,
		pWin->drawable.x+x+w, pWin->drawable.y+desty);
    }
    else if (dx < 0)
    {
	gsxRestoreDCAreaCT(pScreen, 
		0, sourcey,
		-dx, copyh,
		pWin->drawable.x+x+dx, pWin->drawable.y+desty);
    }

    /*
     * move the bits that are still valid within the saveArea
     */
    if (dx >= 0)
    {
	sourcex = 0;
	destx = dx;
	copyw = w - dx;
    }
    else
    {
	destx = 0;
	sourcex = - dx;
	copyw = w + dx;
    }
    if (dy >= 0)
    {
	sourcey = 0;
	desty = dy;
	copyh = h - dy;
    }
    else
    {
	desty = 0;
	sourcey = -dy;
	copyh = h + dy;
    }

    gsxMoveDCAreaCT(pScreen, sourcex, sourcey, copyw, copyh, destx, desty);

    /*
     * copy the new bits from the screen into the remaining areas of the
     * pixmap
     */
    if (dy > 0)
    {
        gsxSaveDCAreaCT(pScreen,
		pWin->drawable.x + x, pWin->drawable.y + y,
		w, dy,
		0,  0);
    }
    else if (dy < 0)
    {
        gsxSaveDCAreaCT(pScreen,
		pWin->drawable.x + x, pWin->drawable.y + y + h + dy,
		w, -dy,
		0, h+dy);
    }
    if (dy >= 0)
    {
	desty = dy;
	sourcey = y + dy;
	copyh = h - dy;
    }
    else
    {
	desty = 0;
	sourcey = y;
	copyh = h + dy;
    }
    if (dx > 0)
    {
        gsxSaveDCAreaCT(pScreen,
		pWin->drawable.x + x, pWin->drawable.y + sourcey,
		dx, copyh,
		0, desty
		);
    }
    else if (dx < 0)
    {
        gsxSaveDCAreaCT(pScreen,
		pWin->drawable.x + x + w + dx, pWin->drawable.y + sourcey,
		-dx, copyh,
		w + dx, desty
		);
    }
    return TRUE;
}

static Bool
gsxDCMoveCursor (pScreen, pCursor, x, y, w, h, dx, dy, source, mask)
    ScreenPtr	    pScreen;
    CursorPtr	    pCursor;
    int		    x, y, w, h, dx, dy;
    unsigned long   source, mask;
{
    gsxDCScreenPtr   pScreenPriv;
    pointer          pPriv;
    WindowPtr	     pWin;

    pPriv = (pointer) pCursor->bits->devPriv[pScreen->myNum];
    if (!pPriv)
    {
	pPriv = gsxDCRealize(pScreen, pCursor);
	if (!pPriv)
	    return FALSE;
    }
    pScreenPriv = (gsxDCScreenPtr) pScreen->devPrivates[gsxDCScreenIndex].ptr;

    if (pScreenPriv->lastCursorPriv != pPriv ||
    		pScreenPriv->lastSourcePix != source ||
		pScreenPriv->lastMaskPix != mask ) {

        // limit size and hostsopt of cursor and record these.
	gsxDCGetRealizedSize(pScreen, pCursor, 
			&pScreenPriv->realW, &pScreenPriv->realH,
			0, 0);

        // build texture image  and load it to GS
	gsxBuildCursorCT(pScreen,
			pCursor->bits->source, pCursor->bits->mask,
			pCursor->bits->width, pCursor->bits->height,
			pScreenPriv->realW, pScreenPriv->realH,
			source, mask);
		
        pScreenPriv->lastCursorPriv = pPriv;

        pScreenPriv->lastSourcePix = source;
        pScreenPriv->lastMaskPix = mask;
    }

    pWin = WindowTable[pScreen->myNum];

    // Restore save area to FB 
    gsxRestoreDCAreaCT(pScreen, 0, 0, w, h, 
			pWin->drawable.x + x, pWin->drawable.y + y);

    // draw cursor
    gsxPutCursorBitsCT(pScreen, 
	pWin->drawable.x + x + dx, pWin->drawable.y + y +dy,
	pScreenPriv->realW, pScreenPriv->realH);


    return TRUE;

}


static
Bool  gsxDCGetRealizedSize(
    ScreenPtr pScreen, 
    CursorPtr pCursor,
    int *w, int *h,
    int *hotx, int *hoty)
{

    int tw,th;
    tw = min(pCursor->bits->width, TILE_CACHE_WIDTH-2*GSX_SPRITE_PAD);
    if (w) *w = tw;
    th = min(pCursor->bits->height, TILE_CACHE_HEIGHT-2*GSX_SPRITE_PAD);
    if (h) *h = th;
    if (hotx) {
        *hotx = min(pCursor->bits->xhot, tw);
	*hotx = (*hotx<0) ? 0: *hotx;
    }
    if (hoty) {
        *hoty = min(pCursor->bits->yhot, th);
	*hoty = (*hoty<0) ? 0: *hoty;
    }
    return TRUE;
}

#endif /* GSX_ACCL_SPRITE_DC */
