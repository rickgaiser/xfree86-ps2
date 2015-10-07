/*
 * gsxsprite.c
 *
 * gsx software sprite routines
 * based on mi/misprite.c
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


/* $XConsortium: misprite.c,v 5.47 94/04/17 20:27:53 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/mi/misprite.c,v 3.0.4.1 1999/11/18 15:37:35 hohndel Exp $ */

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

# include   "X.h"
# include   "Xproto.h"
# include   "misc.h"
# include   "pixmapstr.h"
# include   "input.h"
# include   "mi.h"
# include   "cursorstr.h"
# include   "font.h"
# include   "scrnintstr.h"
# include   "colormapst.h"
# include   "windowstr.h"
# include   "gcstruct.h"
# include   "mipointer.h"
# include   "mispritest.h"
# include   "dixfontstr.h"
# include   "fontstruct.h"
# include   "gsx.h"
# include   "common/xf86.h"

#if GSX_ACCL_SPRITE_DC

#ifdef MITSHM
extern ShmFuncsPtr shmFuncs[];
static void gsxSpriteShmPutImage();
#endif


/*
 * screen wrappers
 */

static int  gsxSpriteScreenIndex;
static unsigned long gsxSpriteGeneration = 0;

static Bool	    gsxSpriteCloseScreen();
static void	    gsxSpriteGetImage();
static void	    gsxSpriteGetSpans();
static void	    gsxSpriteSourceValidate();
static Bool	    gsxSpriteCreateGC();
static void	    gsxSpriteBlockHandler();
static void	    gsxSpriteInstallColormap();
static void	    gsxSpriteStoreColors();

static void	    gsxSpritePaintWindowBackground();
static void	    gsxSpritePaintWindowBorder();
static void	    gsxSpriteCopyWindow();
static void	    gsxSpriteClearToBackground();

static void	    gsxSpriteSaveDoomedAreas();
static RegionPtr    gsxSpriteRestoreAreas();
static void	    gsxSpriteComputeSaved();

#define SCREEN_PROLOGUE(pScreen, field)\
  ((pScreen)->field = \
   ((gsxSpriteScreenPtr) (pScreen)->devPrivates[gsxSpriteScreenIndex].ptr)->field)

#define SCREEN_EPILOGUE(pScreen, field, wrapper)\
    ((pScreen)->field = wrapper)

/*
 * GC func wrappers
 */

static int  gsxSpriteGCIndex;

static void gsxSpriteValidateGC (),  gsxSpriteCopyGC ();
static void gsxSpriteDestroyGC(),    gsxSpriteChangeGC();
static void gsxSpriteChangeClip(),   gsxSpriteDestroyClip();
static void gsxSpriteCopyClip();

static GCFuncs	gsxSpriteGCFuncs = {
    gsxSpriteValidateGC,
    gsxSpriteChangeGC,
    gsxSpriteCopyGC,
    gsxSpriteDestroyGC,
    gsxSpriteChangeClip,
    gsxSpriteDestroyClip,
    gsxSpriteCopyClip,
};

#define GC_FUNC_PROLOGUE(pGC)					\
    miSpriteGCPtr   pGCPriv =					\
	(miSpriteGCPtr) (pGC)->devPrivates[gsxSpriteGCIndex].ptr;\
    (pGC)->funcs = pGCPriv->wrapFuncs;				\
    if (pGCPriv->wrapOps)					\
	(pGC)->ops = pGCPriv->wrapOps;

#define GC_FUNC_EPILOGUE(pGC)					\
    pGCPriv->wrapFuncs = (pGC)->funcs;				\
    (pGC)->funcs = &gsxSpriteGCFuncs;				\
    if (pGCPriv->wrapOps)					\
    {								\
	pGCPriv->wrapOps = (pGC)->ops;				\
	(pGC)->ops = &gsxSpriteGCOps;				\
    }

/*
 * GC op wrappers
 */

static void	    gsxSpriteFillSpans(),	gsxSpriteSetSpans();
static void	    gsxSpritePutImage();
static RegionPtr    gsxSpriteCopyArea(),	gsxSpriteCopyPlane();
static void	    gsxSpritePolyPoint(),	gsxSpritePolylines();
static void	    gsxSpritePolySegment(),	gsxSpritePolyRectangle();
static void	    gsxSpritePolyArc(),		gsxSpriteFillPolygon();
static void	    gsxSpritePolyFillRect(),	gsxSpritePolyFillArc();
static int	    gsxSpritePolyText8(),	gsxSpritePolyText16();
static void	    gsxSpriteImageText8(),	gsxSpriteImageText16();
static void	    gsxSpriteImageGlyphBlt(),	gsxSpritePolyGlyphBlt();
static void	    gsxSpritePushPixels();
#ifdef NEED_LINEHELPER
static void	    gsxSpriteLineHelper();
#endif

static GCOps gsxSpriteGCOps = {
    gsxSpriteFillSpans,	   gsxSpriteSetSpans,	    gsxSpritePutImage,	
    gsxSpriteCopyArea,	   gsxSpriteCopyPlane,	    gsxSpritePolyPoint,
    gsxSpritePolylines,	   gsxSpritePolySegment,    gsxSpritePolyRectangle,
    gsxSpritePolyArc,	   gsxSpriteFillPolygon,    gsxSpritePolyFillRect,
    gsxSpritePolyFillArc,  gsxSpritePolyText8,	    gsxSpritePolyText16,
    gsxSpriteImageText8,   gsxSpriteImageText16,    gsxSpriteImageGlyphBlt,
    gsxSpritePolyGlyphBlt, gsxSpritePushPixels
#ifdef NEED_LINEHELPER
    , gsxSpriteLineHelper
#endif
};

/*
 * testing only -- remove cursor for every draw.  Eventually,
 * each draw operation will perform a bounding box check against
 * the saved cursor area
 */

#define GC_SETUP_CHEAP(pDrawable)				    \
    gsxSpriteScreenPtr	pScreenPriv = (gsxSpriteScreenPtr)	    \
	(pDrawable)->pScreen->devPrivates[gsxSpriteScreenIndex].ptr; \

#define GC_SETUP(pDrawable, pGC)				    \
    GC_SETUP_CHEAP(pDrawable)					    \
    miSpriteGCPtr	pGCPrivate = (miSpriteGCPtr)		    \
	(pGC)->devPrivates[gsxSpriteGCIndex].ptr;		    \
    GCFuncs *oldFuncs = pGC->funcs;

#define GC_SETUP_AND_CHECK(pDrawable, pGC)			    \
    GC_SETUP(pDrawable, pGC);					    \
    if (GC_CHECK((WindowPtr)pDrawable))				    \
	gsxSpriteRemoveCursor (pDrawable->pScreen);
    
#define GC_CHECK(pWin)						    \
    (pScreenPriv->isUp &&					    \
        (pScreenPriv->pCacheWin == pWin ?			    \
	    pScreenPriv->isInCacheWin : (			    \
	    (pScreenPriv->pCacheWin = (pWin)),			    \
	    (pScreenPriv->isInCacheWin =			    \
		(pWin)->drawable.x < pScreenPriv->saved.x2 &&	    \
		pScreenPriv->saved.x1 < (pWin)->drawable.x +	    \
				    (int) (pWin)->drawable.width && \
		(pWin)->drawable.y < pScreenPriv->saved.y2 &&	    \
		pScreenPriv->saved.y1 < (pWin)->drawable.y +	    \
				    (int) (pWin)->drawable.height &&\
		RECT_IN_REGION((pWin)->drawable.pScreen, &(pWin)->borderClip, \
			&pScreenPriv->saved) != rgnOUT))))

#define GC_OP_PROLOGUE(pGC) { \
    (pGC)->funcs = pGCPrivate->wrapFuncs; \
    (pGC)->ops = pGCPrivate->wrapOps; \
    }

#define GC_OP_EPILOGUE(pGC) { \
    pGCPrivate->wrapOps = (pGC)->ops; \
    (pGC)->funcs = oldFuncs; \
    (pGC)->ops = &gsxSpriteGCOps; \
    }

/*
 * pointer-sprite method table
 */

static Bool gsxSpriteRealizeCursor (),	gsxSpriteUnrealizeCursor ();
static void gsxSpriteSetCursor (),	gsxSpriteMoveCursor ();

miPointerSpriteFuncRec gsxSpritePointerFuncs = {
    gsxSpriteRealizeCursor,
    gsxSpriteUnrealizeCursor,
    gsxSpriteSetCursor,
    gsxSpriteMoveCursor,
};

/*
 * other misc functions
 */

void gsxSpriteRemoveCursor (),	gsxSpriteRestoreCursor ();

/*
 * miSpriteInitialize -- called from device-dependent screen
 * initialization proc after all of the function pointers have
 * been stored in the screen structure.
 */

Bool
gsxSpriteInitialize (pScreen, cursorFuncs, screenFuncs)
    ScreenPtr		    pScreen;
    gsxSpriteCursorFuncPtr   cursorFuncs;
    miPointerScreenFuncPtr  screenFuncs;
{
    gsxSpriteScreenPtr	pPriv;
    VisualPtr		pVisual;
    
    if (gsxSpriteGeneration != serverGeneration)
    {
	gsxSpriteScreenIndex = AllocateScreenPrivateIndex ();
	if (gsxSpriteScreenIndex < 0)
	    return FALSE;
	gsxSpriteGeneration = serverGeneration;
	gsxSpriteGCIndex = AllocateGCPrivateIndex ();
    }
    if (!AllocateGCPrivate(pScreen, gsxSpriteGCIndex, sizeof(miSpriteGCRec)))
	return FALSE;
    pPriv = (gsxSpriteScreenPtr) xalloc (sizeof (gsxSpriteScreenRec));
    if (!pPriv)
	return FALSE;
    if (!miPointerInitialize (pScreen, &gsxSpritePointerFuncs, screenFuncs,TRUE))
    {
	xfree ((pointer) pPriv);
	return FALSE;
    }
    for (pVisual = pScreen->visuals;
	 pVisual->vid != pScreen->rootVisual;
	 pVisual++)
	;
    pPriv->pVisual = pVisual;
    pPriv->CloseScreen = pScreen->CloseScreen;
    pPriv->GetImage = pScreen->GetImage;
    pPriv->GetSpans = pScreen->GetSpans;
    pPriv->SourceValidate = pScreen->SourceValidate;
    pPriv->CreateGC = pScreen->CreateGC;
    pPriv->BlockHandler = pScreen->BlockHandler;
    pPriv->InstallColormap = pScreen->InstallColormap;
    pPriv->StoreColors = pScreen->StoreColors;

    pPriv->PaintWindowBackground = pScreen->PaintWindowBackground;
    pPriv->PaintWindowBorder = pScreen->PaintWindowBorder;
    pPriv->CopyWindow = pScreen->CopyWindow;
    pPriv->ClearToBackground = pScreen->ClearToBackground;

    pPriv->SaveDoomedAreas = pScreen->SaveDoomedAreas;
    pPriv->RestoreAreas = pScreen->RestoreAreas;

    pPriv->pCursor = NULL;
    pPriv->x = 0;
    pPriv->y = 0;
    pPriv->isUp = FALSE;
    pPriv->shouldBeUp = FALSE;
    pPriv->pCacheWin = NullWindow;
    pPriv->isInCacheWin = FALSE;
    pPriv->checkPixels = TRUE;
    pPriv->pInstalledMap = NULL;
    pPriv->pColormap = NULL;
    pPriv->funcs = cursorFuncs;
    pPriv->colors[SOURCE_COLOR].red = 0;
    pPriv->colors[SOURCE_COLOR].green = 0;
    pPriv->colors[SOURCE_COLOR].blue = 0;
    pPriv->colors[MASK_COLOR].red = 0;
    pPriv->colors[MASK_COLOR].green = 0;
    pPriv->colors[MASK_COLOR].blue = 0;
#ifdef MITSHM
    pPriv->orgShmFuncsPtr = shmFuncs[pScreen->myNum];
    pPriv->shmFuncs = *shmFuncs[pScreen->myNum];
    shmFuncs[pScreen->myNum] = &pPriv->shmFuncs;
    shmFuncs[pScreen->myNum]->PutImage = gsxSpriteShmPutImage;
#endif
    pScreen->devPrivates[gsxSpriteScreenIndex].ptr = (pointer) pPriv;
    pScreen->CloseScreen = gsxSpriteCloseScreen;
    pScreen->GetImage = gsxSpriteGetImage;
    pScreen->GetSpans = gsxSpriteGetSpans;
    pScreen->SourceValidate = gsxSpriteSourceValidate;
    pScreen->CreateGC = gsxSpriteCreateGC;
    pScreen->BlockHandler = gsxSpriteBlockHandler;
    pScreen->InstallColormap = gsxSpriteInstallColormap;
    pScreen->StoreColors = gsxSpriteStoreColors;

    pScreen->PaintWindowBackground = gsxSpritePaintWindowBackground;
    pScreen->PaintWindowBorder = gsxSpritePaintWindowBorder;
    pScreen->CopyWindow = gsxSpriteCopyWindow;
    pScreen->ClearToBackground = gsxSpriteClearToBackground;

    pScreen->SaveDoomedAreas = gsxSpriteSaveDoomedAreas;
    pScreen->RestoreAreas = gsxSpriteRestoreAreas;

    return TRUE;
}

/*
 * Screen wrappers
 */

/*
 * CloseScreen wrapper -- unwrap everything, free the private data
 * and call the wrapped function
 */

static Bool
gsxSpriteCloseScreen (i, pScreen)
    ScreenPtr	pScreen;
{
    gsxSpriteScreenPtr   pScreenPriv;

    pScreenPriv = (gsxSpriteScreenPtr) pScreen->devPrivates[gsxSpriteScreenIndex].ptr;

    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    pScreen->GetImage = pScreenPriv->GetImage;
    pScreen->GetSpans = pScreenPriv->GetSpans;
    pScreen->SourceValidate = pScreenPriv->SourceValidate;
    pScreen->CreateGC = pScreenPriv->CreateGC;
    pScreen->BlockHandler = pScreenPriv->BlockHandler;
    pScreen->InstallColormap = pScreenPriv->InstallColormap;
    pScreen->StoreColors = pScreenPriv->StoreColors;

    pScreen->PaintWindowBackground = pScreenPriv->PaintWindowBackground;
    pScreen->PaintWindowBorder = pScreenPriv->PaintWindowBorder;
    pScreen->CopyWindow = pScreenPriv->CopyWindow;
    pScreen->ClearToBackground = pScreenPriv->ClearToBackground;

    pScreen->SaveDoomedAreas = pScreenPriv->SaveDoomedAreas;
    pScreen->RestoreAreas = pScreenPriv->RestoreAreas;

    xfree ((pointer) pScreenPriv);

    return (*pScreen->CloseScreen) (i, pScreen);
}

static void
gsxSpriteGetImage (pDrawable, sx, sy, w, h, format, planemask, pdstLine)
    DrawablePtr	    pDrawable;
    int		    sx, sy, w, h;
    unsigned int    format;
    unsigned long   planemask;
    char	    *pdstLine;
{
    ScreenPtr	    pScreen = pDrawable->pScreen;
    gsxSpriteScreenPtr    pScreenPriv;
    
    SCREEN_PROLOGUE (pScreen, GetImage);

    pScreenPriv = (gsxSpriteScreenPtr) pScreen->devPrivates[gsxSpriteScreenIndex].ptr;

    if (pDrawable->type == DRAWABLE_WINDOW &&
        pScreenPriv->isUp &&
	ORG_OVERLAP(&pScreenPriv->saved,pDrawable->x,pDrawable->y, sx, sy, w, h))
    {
	gsxSpriteRemoveCursor (pScreen);
    }

    (*pScreen->GetImage) (pDrawable, sx, sy, w, h,
			  format, planemask, pdstLine);

    SCREEN_EPILOGUE (pScreen, GetImage, gsxSpriteGetImage);
}

static void
gsxSpriteGetSpans (pDrawable, wMax, ppt, pwidth, nspans, pdstStart)
    DrawablePtr	pDrawable;
    int		wMax;
    DDXPointPtr	ppt;
    int		*pwidth;
    int		nspans;
    char	*pdstStart;
{
    ScreenPtr		    pScreen = pDrawable->pScreen;
    gsxSpriteScreenPtr	    pScreenPriv;
    
    SCREEN_PROLOGUE (pScreen, GetSpans);

    pScreenPriv = (gsxSpriteScreenPtr) pScreen->devPrivates[gsxSpriteScreenIndex].ptr;

    if (pDrawable->type == DRAWABLE_WINDOW && pScreenPriv->isUp)
    {
	register DDXPointPtr    pts;
	register int    	*widths;
	register int    	nPts;
	register int    	xorg,
				yorg;

	xorg = pDrawable->x;
	yorg = pDrawable->y;

	for (pts = ppt, widths = pwidth, nPts = nspans;
	     nPts--;
	     pts++, widths++)
 	{
	    if (SPN_OVERLAP(&pScreenPriv->saved,pts->y+yorg,
			     pts->x+xorg,*widths))
	    {
		gsxSpriteRemoveCursor (pScreen);
		break;
	    }
	}
    }

    (*pScreen->GetSpans) (pDrawable, wMax, ppt, pwidth, nspans, pdstStart);

    SCREEN_EPILOGUE (pScreen, GetSpans, gsxSpriteGetSpans);
}

static void
gsxSpriteSourceValidate (pDrawable, x, y, width, height)
    DrawablePtr	pDrawable;
    int		x, y, width, height;
{
    ScreenPtr		    pScreen = pDrawable->pScreen;
    gsxSpriteScreenPtr	    pScreenPriv;
    
    SCREEN_PROLOGUE (pScreen, SourceValidate);

    pScreenPriv = (gsxSpriteScreenPtr) pScreen->devPrivates[gsxSpriteScreenIndex].ptr;

    if (pDrawable->type == DRAWABLE_WINDOW && pScreenPriv->isUp &&
	ORG_OVERLAP(&pScreenPriv->saved, pDrawable->x, pDrawable->y,
		    x, y, width, height))
    {
	gsxSpriteRemoveCursor (pScreen);
    }

    if (pScreen->SourceValidate)
	(*pScreen->SourceValidate) (pDrawable, x, y, width, height);

    SCREEN_EPILOGUE (pScreen, SourceValidate, gsxSpriteSourceValidate);
}

static Bool
gsxSpriteCreateGC (pGC)
    GCPtr   pGC;
{
    ScreenPtr	    pScreen = pGC->pScreen;
    Bool	    ret;
    miSpriteGCPtr   pPriv;

    SCREEN_PROLOGUE (pScreen, CreateGC);
    
    pPriv = (miSpriteGCPtr)pGC->devPrivates[gsxSpriteGCIndex].ptr;

    ret = (*pScreen->CreateGC) (pGC);

    pPriv->wrapOps = NULL;
    pPriv->wrapFuncs = pGC->funcs;
    pGC->funcs = &gsxSpriteGCFuncs;

    SCREEN_EPILOGUE (pScreen, CreateGC, gsxSpriteCreateGC);

    return ret;
}

static void
gsxSpriteBlockHandler (i, blockData, pTimeout, pReadmask)
    int	i;
    pointer	blockData;
    OSTimePtr	pTimeout;
    pointer	pReadmask;
{
    ScreenPtr		pScreen = screenInfo.screens[i];
    gsxSpriteScreenPtr	pPriv;

    pPriv = (gsxSpriteScreenPtr) pScreen->devPrivates[gsxSpriteScreenIndex].ptr;

    SCREEN_PROLOGUE(pScreen, BlockHandler);
    
    (*pScreen->BlockHandler) (i, blockData, pTimeout, pReadmask);

    SCREEN_EPILOGUE(pScreen, BlockHandler, gsxSpriteBlockHandler);

    if (!xf86VTSema)
	return;

    if (!pPriv->isUp && pPriv->shouldBeUp)
	gsxSpriteRestoreCursor (pScreen);
}

static void
gsxSpriteInstallColormap (pMap)
    ColormapPtr	pMap;
{
    ScreenPtr		pScreen = pMap->pScreen;
    gsxSpriteScreenPtr	pPriv;

    pPriv = (gsxSpriteScreenPtr) pScreen->devPrivates[gsxSpriteScreenIndex].ptr;

    SCREEN_PROLOGUE(pScreen, InstallColormap);
    
    (*pScreen->InstallColormap) (pMap);

    SCREEN_EPILOGUE(pScreen, InstallColormap, gsxSpriteInstallColormap);

    pPriv->pInstalledMap = pMap;
    if (pPriv->pColormap != pMap)
    {
    	pPriv->checkPixels = TRUE;
	if (pPriv->isUp)
	    gsxSpriteRemoveCursor (pScreen);
    }
}

static void
gsxSpriteStoreColors (pMap, ndef, pdef)
    ColormapPtr	pMap;
    int		ndef;
    xColorItem	*pdef;
{
    ScreenPtr		pScreen = pMap->pScreen;
    gsxSpriteScreenPtr	pPriv;
    int			i;
    int			updated;
    VisualPtr		pVisual;

    pPriv = (gsxSpriteScreenPtr) pScreen->devPrivates[gsxSpriteScreenIndex].ptr;

    SCREEN_PROLOGUE(pScreen, StoreColors);
    
    (*pScreen->StoreColors) (pMap, ndef, pdef);

    SCREEN_EPILOGUE(pScreen, StoreColors, gsxSpriteStoreColors);

    if (pPriv->pColormap == pMap)
    {
	updated = 0;
	pVisual = pMap->pVisual;
	if (pVisual->class == DirectColor)
	{
	    /* Direct color - match on any of the subfields */

#define MaskMatch(a,b,mask) (((a)&(pVisual->mask)) == ((b)&(pVisual->mask)))

#define UpdateDAC(plane,dac,mask) {\
    if (MaskMatch (pPriv->colors[plane].pixel,pdef[i].pixel,mask)) {\
	pPriv->colors[plane].dac = pdef[i].dac; \
	updated = 1; \
    } \
}

#define CheckDirect(plane) \
	    UpdateDAC(plane,red,redMask) \
	    UpdateDAC(plane,green,greenMask) \
	    UpdateDAC(plane,blue,blueMask)

	    for (i = 0; i < ndef; i++)
	    {
		CheckDirect (SOURCE_COLOR)
		CheckDirect (MASK_COLOR)
	    }
	}
	else
	{
	    /* PseudoColor/GrayScale - match on exact pixel */
	    for (i = 0; i < ndef; i++)
	    {
	    	if (pdef[i].pixel == pPriv->colors[SOURCE_COLOR].pixel)
	    	{
		    pPriv->colors[SOURCE_COLOR] = pdef[i];
		    if (++updated == 2)
		    	break;
	    	}
	    	if (pdef[i].pixel == pPriv->colors[MASK_COLOR].pixel)
	    	{
		    pPriv->colors[MASK_COLOR] = pdef[i];
		    if (++updated == 2)
		    	break;
	    	}
	    }
	}
    	if (updated)
    	{
	    pPriv->checkPixels = TRUE;
	    if (pPriv->isUp)
	    	gsxSpriteRemoveCursor (pScreen);
    	}
    }
}

static void
gsxSpriteFindColors (pScreen)
    ScreenPtr	pScreen;
{
    gsxSpriteScreenPtr	pScreenPriv = (gsxSpriteScreenPtr)
			    pScreen->devPrivates[gsxSpriteScreenIndex].ptr;
    CursorPtr		pCursor;
    xColorItem		*sourceColor, *maskColor;

    pCursor = pScreenPriv->pCursor;
    sourceColor = &pScreenPriv->colors[SOURCE_COLOR];
    maskColor = &pScreenPriv->colors[MASK_COLOR];
    if (pScreenPriv->pColormap != pScreenPriv->pInstalledMap ||
	!(pCursor->foreRed == sourceColor->red &&
	  pCursor->foreGreen == sourceColor->green &&
          pCursor->foreBlue == sourceColor->blue &&
	  pCursor->backRed == maskColor->red &&
	  pCursor->backGreen == maskColor->green &&
	  pCursor->backBlue == maskColor->blue))
    {
	pScreenPriv->pColormap = pScreenPriv->pInstalledMap;
	sourceColor->red = pCursor->foreRed;
	sourceColor->green = pCursor->foreGreen;
	sourceColor->blue = pCursor->foreBlue;
	FakeAllocColor (pScreenPriv->pColormap, sourceColor);
	maskColor->red = pCursor->backRed;
	maskColor->green = pCursor->backGreen;
	maskColor->blue = pCursor->backBlue;
	FakeAllocColor (pScreenPriv->pColormap, maskColor);
	/* "free" the pixels right away, don't let this confuse you */
	FakeFreeColor(pScreenPriv->pColormap, sourceColor->pixel);
	FakeFreeColor(pScreenPriv->pColormap, maskColor->pixel);
    }
    pScreenPriv->checkPixels = FALSE;
}

/*
 * BackingStore wrappers
 */

static void
gsxSpriteSaveDoomedAreas (pWin, pObscured, dx, dy)
    WindowPtr	pWin;
    RegionPtr	pObscured;
    int		dx, dy;
{
    ScreenPtr		pScreen;
    gsxSpriteScreenPtr   pScreenPriv;
    BoxRec		cursorBox;

    pScreen = pWin->drawable.pScreen;
    
    SCREEN_PROLOGUE (pScreen, SaveDoomedAreas);

    pScreenPriv = (gsxSpriteScreenPtr) pScreen->devPrivates[gsxSpriteScreenIndex].ptr;
    if (pScreenPriv->isUp)
    {
	cursorBox = pScreenPriv->saved;

	if (dx || dy)
 	{
	    cursorBox.x1 += dx;
	    cursorBox.y1 += dy;
	    cursorBox.x2 += dx;
	    cursorBox.y2 += dy;
	}
	if (RECT_IN_REGION( pScreen, pObscured, &cursorBox) != rgnOUT)
	    gsxSpriteRemoveCursor (pScreen);
    }

    (*pScreen->SaveDoomedAreas) (pWin, pObscured, dx, dy);

    SCREEN_EPILOGUE (pScreen, SaveDoomedAreas, gsxSpriteSaveDoomedAreas);
}

static RegionPtr
gsxSpriteRestoreAreas (pWin, prgnExposed)
    WindowPtr	pWin;
    RegionPtr	prgnExposed;
{
    ScreenPtr		pScreen;
    gsxSpriteScreenPtr   pScreenPriv;
    RegionPtr		result;

    pScreen = pWin->drawable.pScreen;
    
    SCREEN_PROLOGUE (pScreen, RestoreAreas);

    pScreenPriv = (gsxSpriteScreenPtr) pScreen->devPrivates[gsxSpriteScreenIndex].ptr;
    if (pScreenPriv->isUp)
    {
	if (RECT_IN_REGION( pScreen, prgnExposed, &pScreenPriv->saved) != rgnOUT)
	    gsxSpriteRemoveCursor (pScreen);
    }

    result = (*pScreen->RestoreAreas) (pWin, prgnExposed);

    SCREEN_EPILOGUE (pScreen, RestoreAreas, gsxSpriteRestoreAreas);

    return result;
}

/*
 * Window wrappers
 */

static void
gsxSpritePaintWindowBackground (pWin, pRegion, what)
    WindowPtr	pWin;
    RegionPtr	pRegion;
    int		what;
{
    ScreenPtr	    pScreen;
    gsxSpriteScreenPtr    pScreenPriv;

    pScreen = pWin->drawable.pScreen;

    SCREEN_PROLOGUE (pScreen, PaintWindowBackground);

    pScreenPriv = (gsxSpriteScreenPtr) pScreen->devPrivates[gsxSpriteScreenIndex].ptr;
    if (pScreenPriv->isUp)
    {
	/*
	 * If the cursor is on the same screen as the window, check the
	 * region to paint for the cursor and remove it as necessary
	 */
	if (RECT_IN_REGION( pScreen, pRegion, &pScreenPriv->saved) != rgnOUT)
	    gsxSpriteRemoveCursor (pScreen);
    }

    (*pScreen->PaintWindowBackground) (pWin, pRegion, what);

    SCREEN_EPILOGUE (pScreen, PaintWindowBackground, gsxSpritePaintWindowBackground);
}

static void
gsxSpritePaintWindowBorder (pWin, pRegion, what)
    WindowPtr	pWin;
    RegionPtr	pRegion;
    int		what;
{
    ScreenPtr	    pScreen;
    gsxSpriteScreenPtr    pScreenPriv;

    pScreen = pWin->drawable.pScreen;

    SCREEN_PROLOGUE (pScreen, PaintWindowBorder);

    pScreenPriv = (gsxSpriteScreenPtr) pScreen->devPrivates[gsxSpriteScreenIndex].ptr;
    if (pScreenPriv->isUp)
    {
	/*
	 * If the cursor is on the same screen as the window, check the
	 * region to paint for the cursor and remove it as necessary
	 */
	if (RECT_IN_REGION( pScreen, pRegion, &pScreenPriv->saved) != rgnOUT)
	    gsxSpriteRemoveCursor (pScreen);
    }

    (*pScreen->PaintWindowBorder) (pWin, pRegion, what);

    SCREEN_EPILOGUE (pScreen, PaintWindowBorder, gsxSpritePaintWindowBorder);
}

static void
gsxSpriteCopyWindow (pWin, ptOldOrg, pRegion)
    WindowPtr	pWin;
    DDXPointRec	ptOldOrg;
    RegionPtr	pRegion;
{
    ScreenPtr	    pScreen;
    gsxSpriteScreenPtr    pScreenPriv;
    BoxRec	    cursorBox;
    int		    dx, dy;

    pScreen = pWin->drawable.pScreen;

    SCREEN_PROLOGUE (pScreen, CopyWindow);

    pScreenPriv = (gsxSpriteScreenPtr) pScreen->devPrivates[gsxSpriteScreenIndex].ptr;
    if (pScreenPriv->isUp)
    {
	/*
	 * check both the source and the destination areas.  The given
	 * region is source relative, so offset the cursor box by
	 * the delta position
	 */
	cursorBox = pScreenPriv->saved;
	dx = pWin->drawable.x - ptOldOrg.x;
	dy = pWin->drawable.y - ptOldOrg.y;
	cursorBox.x1 -= dx;
	cursorBox.x2 -= dx;
	cursorBox.y1 -= dy;
	cursorBox.y2 -= dy;
	if (RECT_IN_REGION( pScreen, pRegion, &pScreenPriv->saved) != rgnOUT ||
	    RECT_IN_REGION( pScreen, pRegion, &cursorBox) != rgnOUT)
	    gsxSpriteRemoveCursor (pScreen);
    }

    (*pScreen->CopyWindow) (pWin, ptOldOrg, pRegion);

    SCREEN_EPILOGUE (pScreen, CopyWindow, gsxSpriteCopyWindow);
}

static void
gsxSpriteClearToBackground (pWin, x, y, w, h, generateExposures)
    WindowPtr pWin;
    short x,y;
    unsigned short w,h;
    Bool generateExposures;
{
    ScreenPtr		pScreen;
    gsxSpriteScreenPtr	pScreenPriv;
    int			realw, realh;

    pScreen = pWin->drawable.pScreen;

    SCREEN_PROLOGUE (pScreen, ClearToBackground);

    pScreenPriv = (gsxSpriteScreenPtr) pScreen->devPrivates[gsxSpriteScreenIndex].ptr;
    if (GC_CHECK(pWin))
    {
	if (!(realw = w))
	    realw = (int) pWin->drawable.width - x;
	if (!(realh = h))
	    realh = (int) pWin->drawable.height - y;
	if (ORG_OVERLAP(&pScreenPriv->saved, pWin->drawable.x, pWin->drawable.y,
			x, y, realw, realh))
	{
	    gsxSpriteRemoveCursor (pScreen);
	}
    }

    (*pScreen->ClearToBackground) (pWin, x, y, w, h, generateExposures);

    SCREEN_EPILOGUE (pScreen, ClearToBackground, gsxSpriteClearToBackground);
}

/*
 * GC Func wrappers
 */

static void
gsxSpriteValidateGC (pGC, changes, pDrawable)
    GCPtr	pGC;
    Mask	changes;
    DrawablePtr	pDrawable;
{
    GC_FUNC_PROLOGUE (pGC);

    (*pGC->funcs->ValidateGC) (pGC, changes, pDrawable);
    
    pGCPriv->wrapOps = NULL;
    if (pDrawable->type == DRAWABLE_WINDOW && ((WindowPtr) pDrawable)->viewable)
    {
	WindowPtr   pWin;
	RegionPtr   pRegion;

	pWin = (WindowPtr) pDrawable;
	pRegion = &pWin->clipList;
	if (pGC->subWindowMode == IncludeInferiors)
	    pRegion = &pWin->borderClip;
	if (REGION_NOTEMPTY(pDrawable->pScreen, pRegion))
	    pGCPriv->wrapOps = pGC->ops;
    }

    GC_FUNC_EPILOGUE (pGC);
}

static void
gsxSpriteChangeGC (pGC, mask)
    GCPtr	    pGC;
    unsigned long   mask;
{
    GC_FUNC_PROLOGUE (pGC);

    (*pGC->funcs->ChangeGC) (pGC, mask);
    
    GC_FUNC_EPILOGUE (pGC);
}

static void
gsxSpriteCopyGC (pGCSrc, mask, pGCDst)
    GCPtr	    pGCSrc, pGCDst;
    unsigned long   mask;
{
    GC_FUNC_PROLOGUE (pGCDst);

    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    
    GC_FUNC_EPILOGUE (pGCDst);
}

static void
gsxSpriteDestroyGC (pGC)
    GCPtr   pGC;
{
    GC_FUNC_PROLOGUE (pGC);

    (*pGC->funcs->DestroyGC) (pGC);
    
    GC_FUNC_EPILOGUE (pGC);
}

static void
gsxSpriteChangeClip (pGC, type, pvalue, nrects)
    GCPtr   pGC;
    int		type;
    pointer	pvalue;
    int		nrects;
{
    GC_FUNC_PROLOGUE (pGC);

    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);

    GC_FUNC_EPILOGUE (pGC);
}

static void
gsxSpriteCopyClip(pgcDst, pgcSrc)
    GCPtr pgcDst, pgcSrc;
{
    GC_FUNC_PROLOGUE (pgcDst);

    (* pgcDst->funcs->CopyClip)(pgcDst, pgcSrc);

    GC_FUNC_EPILOGUE (pgcDst);
}

static void
gsxSpriteDestroyClip(pGC)
    GCPtr	pGC;
{
    GC_FUNC_PROLOGUE (pGC);

    (* pGC->funcs->DestroyClip)(pGC);

    GC_FUNC_EPILOGUE (pGC);
}

/*
 * GC Op wrappers
 */

static void
gsxSpriteFillSpans(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
    GC_SETUP(pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	register DDXPointPtr    pts;
	register int    	*widths;
	register int    	nPts;

	for (pts = pptInit, widths = pwidthInit, nPts = nInit;
	     nPts--;
	     pts++, widths++)
 	{
	     if (SPN_OVERLAP(&pScreenPriv->saved,pts->y,pts->x,*widths))
	     {
		 gsxSpriteRemoveCursor (pDrawable->pScreen);
		 break;
	     }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->FillSpans) (pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);

    GC_OP_EPILOGUE (pGC);
}

static void
gsxSpriteSetSpans(pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted)
    DrawablePtr		pDrawable;
    GCPtr		pGC;
    char		*psrc;
    register DDXPointPtr ppt;
    int			*pwidth;
    int			nspans;
    int			fSorted;
{
    GC_SETUP(pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	register DDXPointPtr    pts;
	register int    	*widths;
	register int    	nPts;

	for (pts = ppt, widths = pwidth, nPts = nspans;
	     nPts--;
	     pts++, widths++)
 	{
	     if (SPN_OVERLAP(&pScreenPriv->saved,pts->y,pts->x,*widths))
	     {
		 gsxSpriteRemoveCursor(pDrawable->pScreen);
		 break;
	     }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->SetSpans) (pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted);

    GC_OP_EPILOGUE (pGC);
}

static void
gsxSpritePutImage(pDrawable, pGC, depth, x, y, w, h, leftPad, format, pBits)
    DrawablePtr	  pDrawable;
    GCPtr   	  pGC;
    int		  depth;
    int	    	  x;
    int	    	  y;
    int	    	  w;
    int	    	  h;
    int	    	  format;
    char    	  *pBits;
{
    GC_SETUP(pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	if (ORG_OVERLAP(&pScreenPriv->saved,pDrawable->x,pDrawable->y,
			x,y,w,h))
 	{
	    gsxSpriteRemoveCursor (pDrawable->pScreen);
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PutImage) (pDrawable, pGC, depth, x, y, w, h, leftPad, format, pBits);

    GC_OP_EPILOGUE (pGC);
}

static RegionPtr
gsxSpriteCopyArea (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty)
    DrawablePtr	  pSrc;
    DrawablePtr	  pDst;
    GCPtr   	  pGC;
    int	    	  srcx;
    int	    	  srcy;
    int	    	  w;
    int	    	  h;
    int	    	  dstx;
    int	    	  dsty;
{
    RegionPtr rgn;

    GC_SETUP(pDst, pGC);

    /* check destination/source overlap. */
    if (GC_CHECK((WindowPtr) pDst) &&
	 (ORG_OVERLAP(&pScreenPriv->saved,pDst->x,pDst->y,dstx,dsty,w,h) ||
	  ((pDst == pSrc) &&
	   ORG_OVERLAP(&pScreenPriv->saved,pSrc->x,pSrc->y,srcx,srcy,w,h))))
    {
	gsxSpriteRemoveCursor (pDst->pScreen);
    }
 
    GC_OP_PROLOGUE (pGC);

    rgn = (*pGC->ops->CopyArea) (pSrc, pDst, pGC, srcx, srcy, w, h,
				 dstx, dsty);

    GC_OP_EPILOGUE (pGC);

    return rgn;
}

static RegionPtr
gsxSpriteCopyPlane (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty, plane)
    DrawablePtr	  pSrc;
    DrawablePtr	  pDst;
    register GCPtr pGC;
    int     	  srcx,
		  srcy;
    int     	  w,
		  h;
    int     	  dstx,
		  dsty;
    unsigned long  plane;
{
    RegionPtr rgn;

    GC_SETUP(pDst, pGC);

    /*
     * check destination/source for overlap.
     */
    if (GC_CHECK((WindowPtr) pDst) &&
	(ORG_OVERLAP(&pScreenPriv->saved,pDst->x,pDst->y,dstx,dsty,w,h) ||
	 ((pDst == pSrc) &&
	  ORG_OVERLAP(&pScreenPriv->saved,pSrc->x,pSrc->y,srcx,srcy,w,h))))
    {
	gsxSpriteRemoveCursor (pDst->pScreen);
    }

    GC_OP_PROLOGUE (pGC);

    rgn = (*pGC->ops->CopyPlane) (pSrc, pDst, pGC, srcx, srcy, w, h,
				  dstx, dsty, plane);

    GC_OP_EPILOGUE (pGC);

    return rgn;
}

static void
gsxSpritePolyPoint (pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		mode;		/* Origin or Previous */
    int		npt;
    xPoint 	*pptInit;
{
    xPoint	t;
    int		n;
    BoxRec	cursor;
    register xPoint *pts;

    GC_SETUP (pDrawable, pGC);

    if (npt && GC_CHECK((WindowPtr) pDrawable))
    {
	cursor.x1 = pScreenPriv->saved.x1 - pDrawable->x;
	cursor.y1 = pScreenPriv->saved.y1 - pDrawable->y;
	cursor.x2 = pScreenPriv->saved.x2 - pDrawable->x;
	cursor.y2 = pScreenPriv->saved.y2 - pDrawable->y;

	if (mode == CoordModePrevious)
	{
	    t.x = 0;
	    t.y = 0;
	    for (pts = pptInit, n = npt; n--; pts++)
	    {
		t.x += pts->x;
		t.y += pts->y;
		if (cursor.x1 <= t.x && t.x <= cursor.x2 &&
		    cursor.y1 <= t.y && t.y <= cursor.y2)
		{
		    gsxSpriteRemoveCursor (pDrawable->pScreen);
		    break;
		}
	    }
	}
	else
	{
	    for (pts = pptInit, n = npt; n--; pts++)
	    {
		if (cursor.x1 <= pts->x && pts->x <= cursor.x2 &&
		    cursor.y1 <= pts->y && pts->y <= cursor.y2)
		{
		    gsxSpriteRemoveCursor (pDrawable->pScreen);
		    break;
		}
	    }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PolyPoint) (pDrawable, pGC, mode, npt, pptInit);

    GC_OP_EPILOGUE (pGC);
}

static void
gsxSpritePolylines (pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr	  pDrawable;
    GCPtr   	  pGC;
    int	    	  mode;
    int	    	  npt;
    DDXPointPtr	  pptInit;
{
    BoxPtr  cursor;
    register DDXPointPtr pts;
    int	    n;
    int	    x, y, x1, y1, x2, y2;
    int	    lw;
    int	    extra;

    GC_SETUP (pDrawable, pGC);

    if (npt && GC_CHECK((WindowPtr) pDrawable))
    {
	cursor = &pScreenPriv->saved;
	lw = pGC->lineWidth;
	x = pptInit->x + pDrawable->x;
	y = pptInit->y + pDrawable->y;

	if (npt == 1)
	{
	    extra = lw >> 1;
	    if (LINE_OVERLAP(cursor, x, y, x, y, extra))
		gsxSpriteRemoveCursor (pDrawable->pScreen);
	}
	else
	{
	    extra = lw >> 1;
	    /*
	     * mitered joins can project quite a way from
	     * the line end; the 11 degree miter limit limits
	     * this extension to 10.43 * lw / 2, rounded up
	     * and converted to int yields 6 * lw
	     */
	    if (pGC->joinStyle == JoinMiter)
		extra = 6 * lw;
	    else if (pGC->capStyle == CapProjecting)
		extra = lw;
	    for (pts = pptInit + 1, n = npt - 1; n--; pts++)
	    {
		x1 = x;
		y1 = y;
		if (mode == CoordModeOrigin)
		{
		    x2 = pDrawable->x + pts->x;
		    y2 = pDrawable->y + pts->y;
		}
		else
		{
		    x2 = x + pts->x;
		    y2 = y + pts->y;
		}
		x = x2;
		y = y2;
		LINE_SORT(x1, y1, x2, y2);
		if (LINE_OVERLAP(cursor, x1, y1, x2, y2, extra))
		{
		    gsxSpriteRemoveCursor (pDrawable->pScreen);
		    break;
		}
	    }
	}
    }
    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->Polylines) (pDrawable, pGC, mode, npt, pptInit);

    GC_OP_EPILOGUE (pGC);
}

static void
gsxSpritePolySegment(pDrawable, pGC, nseg, pSegs)
    DrawablePtr pDrawable;
    GCPtr 	pGC;
    int		nseg;
    xSegment	*pSegs;
{
    int	    n;
    register xSegment *segs;
    BoxPtr  cursor;
    int	    x1, y1, x2, y2;
    int	    extra;

    GC_SETUP(pDrawable, pGC);

    if (nseg && GC_CHECK((WindowPtr) pDrawable))
    {
	cursor = &pScreenPriv->saved;
	extra = pGC->lineWidth >> 1;
	if (pGC->capStyle == CapProjecting)
	    extra = pGC->lineWidth;
	for (segs = pSegs, n = nseg; n--; segs++)
	{
	    x1 = segs->x1 + pDrawable->x;
	    y1 = segs->y1 + pDrawable->y;
	    x2 = segs->x2 + pDrawable->x;
	    y2 = segs->y2 + pDrawable->y;
	    LINE_SORT(x1, y1, x2, y2);
	    if (LINE_OVERLAP(cursor, x1, y1, x2, y2, extra))
	    {
		gsxSpriteRemoveCursor (pDrawable->pScreen);
		break;
	    }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PolySegment) (pDrawable, pGC, nseg, pSegs);

    GC_OP_EPILOGUE (pGC);
}

static void
gsxSpritePolyRectangle(pDrawable, pGC, nrects, pRects)
    DrawablePtr	pDrawable;
    GCPtr	pGC;
    int		nrects;
    xRectangle	*pRects;
{
    register xRectangle *rects;
    BoxPtr  cursor;
    int	    lw;
    int	    n;
    int     x1, y1, x2, y2;
    
    GC_SETUP (pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	lw = pGC->lineWidth >> 1;
	cursor = &pScreenPriv->saved;
	for (rects = pRects, n = nrects; n--; rects++)
	{
	    x1 = rects->x + pDrawable->x;
	    y1 = rects->y + pDrawable->y;
	    x2 = x1 + (int)rects->width;
	    y2 = y1 + (int)rects->height;
	    if (LINE_OVERLAP(cursor, x1, y1, x2, y1, lw) ||
		LINE_OVERLAP(cursor, x2, y1, x2, y2, lw) ||
		LINE_OVERLAP(cursor, x1, y2, x2, y2, lw) ||
		LINE_OVERLAP(cursor, x1, y1, x1, y2, lw))
	    {
		gsxSpriteRemoveCursor (pDrawable->pScreen);
		break;
	    }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PolyRectangle) (pDrawable, pGC, nrects, pRects);

    GC_OP_EPILOGUE (pGC);
}

static void
gsxSpritePolyArc(pDrawable, pGC, narcs, parcs)
    DrawablePtr	pDrawable;
    register GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    BoxPtr  cursor;
    int	    lw;
    int	    n;
    register xArc *arcs;
    
    GC_SETUP (pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	lw = pGC->lineWidth >> 1;
	cursor = &pScreenPriv->saved;
	for (arcs = parcs, n = narcs; n--; arcs++)
	{
	    if (ORG_OVERLAP (cursor, pDrawable->x, pDrawable->y,
			     arcs->x - lw, arcs->y - lw,
			     (int) arcs->width + pGC->lineWidth,
 			     (int) arcs->height + pGC->lineWidth))
	    {
		gsxSpriteRemoveCursor (pDrawable->pScreen);
		break;
	    }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PolyArc) (pDrawable, pGC, narcs, parcs);

    GC_OP_EPILOGUE (pGC);
}

static void
gsxSpriteFillPolygon(pDrawable, pGC, shape, mode, count, pPts)
    register DrawablePtr pDrawable;
    register GCPtr	pGC;
    int			shape, mode;
    int			count;
    DDXPointPtr		pPts;
{
    int x, y, minx, miny, maxx, maxy;
    register DDXPointPtr pts;
    int n;

    GC_SETUP (pDrawable, pGC);

    if (count && GC_CHECK((WindowPtr) pDrawable))
    {
	x = pDrawable->x;
	y = pDrawable->y;
	pts = pPts;
	minx = maxx = pts->x;
	miny = maxy = pts->y;
	pts++;
	n = count - 1;

	if (mode == CoordModeOrigin)
	{
	    for (; n--; pts++)
	    {
		if (pts->x < minx)
		    minx = pts->x;
		else if (pts->x > maxx)
		    maxx = pts->x;
		if (pts->y < miny)
		    miny = pts->y;
		else if (pts->y > maxy)
		    maxy = pts->y;
	    }
	    minx += x;
	    miny += y;
	    maxx += x;
	    maxy += y;
	}
	else
	{
	    x += minx;
	    y += miny;
	    minx = maxx = x;
	    miny = maxy = y;
	    for (; n--; pts++)
	    {
		x += pts->x;
		y += pts->y;
		if (x < minx)
		    minx = x;
		else if (x > maxx)
		    maxx = x;
		if (y < miny)
		    miny = y;
		else if (y > maxy)
		    maxy = y;
	    }
	}
	if (BOX_OVERLAP(&pScreenPriv->saved,minx,miny,maxx,maxy))
	    gsxSpriteRemoveCursor (pDrawable->pScreen);
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->FillPolygon) (pDrawable, pGC, shape, mode, count, pPts);

    GC_OP_EPILOGUE (pGC);
}

static void
gsxSpritePolyFillRect(pDrawable, pGC, nrectFill, prectInit)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nrectFill; 	/* number of rectangles to fill */
    xRectangle	*prectInit;  	/* Pointer to first rectangle to fill */
{
    GC_SETUP(pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	register int	    nRect;
	register xRectangle *pRect;
	register int	    xorg, yorg;

	xorg = pDrawable->x;
	yorg = pDrawable->y;

	for (nRect = nrectFill, pRect = prectInit; nRect--; pRect++) {
	    if (ORGRECT_OVERLAP(&pScreenPriv->saved,xorg,yorg,pRect)){
		gsxSpriteRemoveCursor(pDrawable->pScreen);
		break;
	    }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PolyFillRect) (pDrawable, pGC, nrectFill, prectInit);

    GC_OP_EPILOGUE (pGC);
}

static void
gsxSpritePolyFillArc(pDrawable, pGC, narcs, parcs)
    DrawablePtr	pDrawable;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    GC_SETUP(pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	register int	n;
	BoxPtr		cursor;
	register xArc *arcs;

	cursor = &pScreenPriv->saved;

	for (arcs = parcs, n = narcs; n--; arcs++)
	{
	    if (ORG_OVERLAP(cursor, pDrawable->x, pDrawable->y,
			    arcs->x, arcs->y,
 			    (int) arcs->width, (int) arcs->height))
	    {
		gsxSpriteRemoveCursor (pDrawable->pScreen);
		break;
	    }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PolyFillArc) (pDrawable, pGC, narcs, parcs);

    GC_OP_EPILOGUE (pGC);
}

/*
 * general Poly/Image text function.  Extract glyph information,
 * compute bounding box and remove cursor if it is overlapped.
 */

static Bool
miSpriteTextOverlap (pDraw, font, x, y, n, charinfo, imageblt, w, cursorBox)
    DrawablePtr   pDraw;
    FontPtr	  font;
    int		  x, y;
    unsigned int  n;
    CharInfoPtr   *charinfo;
    Bool	  imageblt;
    unsigned int  w;
    BoxPtr	  cursorBox;
{
    ExtentInfoRec extents;

    x += pDraw->x;
    y += pDraw->y;

    if (FONTMINBOUNDS(font,characterWidth) >= 0)
    {
	/* compute an approximate (but covering) bounding box */
	if (!imageblt || (charinfo[0]->metrics.leftSideBearing < 0))
	    extents.overallLeft = charinfo[0]->metrics.leftSideBearing;
	else
	    extents.overallLeft = 0;
	if (w)
	    extents.overallRight = w - charinfo[n-1]->metrics.characterWidth;
	else
	    extents.overallRight = FONTMAXBOUNDS(font,characterWidth)
				    * (n - 1);
	if (imageblt && (charinfo[n-1]->metrics.characterWidth >
			 charinfo[n-1]->metrics.rightSideBearing))
	    extents.overallRight += charinfo[n-1]->metrics.characterWidth;
	else
	    extents.overallRight += charinfo[n-1]->metrics.rightSideBearing;
	if (imageblt && FONTASCENT(font) > FONTMAXBOUNDS(font,ascent))
	    extents.overallAscent = FONTASCENT(font);
	else
	    extents.overallAscent = FONTMAXBOUNDS(font, ascent);
	if (imageblt && FONTDESCENT(font) > FONTMAXBOUNDS(font,descent))
	    extents.overallDescent = FONTDESCENT(font);
	else
	    extents.overallDescent = FONTMAXBOUNDS(font,descent);
	if (!BOX_OVERLAP(cursorBox,
			 x + extents.overallLeft,
			 y - extents.overallAscent,
			 x + extents.overallRight,
			 y + extents.overallDescent))
	    return FALSE;
	else if (imageblt && w)
	    return TRUE;
	/* if it does overlap, fall through and compute exactly, because
	 * taking down the cursor is expensive enough to make this worth it
	 */
    }
    QueryGlyphExtents(font, charinfo, n, &extents);
    if (imageblt)
    {
	if (extents.overallWidth > extents.overallRight)
	    extents.overallRight = extents.overallWidth;
	if (extents.overallWidth < extents.overallLeft)
	    extents.overallLeft = extents.overallWidth;
	if (extents.overallLeft > 0)
	    extents.overallLeft = 0;
	if (extents.fontAscent > extents.overallAscent)
	    extents.overallAscent = extents.fontAscent;
	if (extents.fontDescent > extents.overallDescent)
	    extents.overallDescent = extents.fontDescent;
    }
    return (BOX_OVERLAP(cursorBox,
			x + extents.overallLeft,
			y - extents.overallAscent,
			x + extents.overallRight,
			y + extents.overallDescent));
}

/*
 * values for textType:
 */
#define TT_POLY8   0
#define TT_IMAGE8  1
#define TT_POLY16  2
#define TT_IMAGE16 3

static int 
miSpriteText (pDraw, pGC, x, y, count, chars, fontEncoding, textType, cursorBox)
    DrawablePtr	    pDraw;
    GCPtr	    pGC;
    int		    x,
		    y;
    unsigned long    count;
    char	    *chars;
    FontEncoding    fontEncoding;
    Bool	    textType;
    BoxPtr	    cursorBox;
{
    CharInfoPtr *charinfo;
    register CharInfoPtr *info;
    unsigned long i;
    unsigned int  n;
    int		  w;
    void   	  (*drawFunc)();

    Bool imageblt;

    imageblt = (textType == TT_IMAGE8) || (textType == TT_IMAGE16);

    charinfo = (CharInfoPtr *) ALLOCATE_LOCAL(count * sizeof(CharInfoPtr));
    if (!charinfo)
	return x;

    GetGlyphs(pGC->font, count, (unsigned char *)chars,
	      fontEncoding, &i, charinfo);
    n = (unsigned int)i;
    w = 0;
    if (!imageblt)
	for (info = charinfo; i--; info++)
	    w += (*info)->metrics.characterWidth;

    if (n != 0) {
	if (miSpriteTextOverlap(pDraw, pGC->font, x, y, n, charinfo, imageblt, w, cursorBox))
	    gsxSpriteRemoveCursor(pDraw->pScreen);

#ifdef AVOID_GLYPHBLT
	/*
	 * On displays like Apollos, which do not optimize the GlyphBlt functions because they
	 * convert fonts to their internal form in RealizeFont and optimize text directly, we
	 * want to invoke the text functions here, not the GlyphBlt functions.
	 */
	switch (textType)
	{
        default:
	case TT_POLY8:
	    drawFunc = (void (*)())pGC->ops->PolyText8;
	    break;
	case TT_IMAGE8:
	    drawFunc = pGC->ops->ImageText8;
	    break;
	case TT_POLY16:
	    drawFunc = (void (*)())pGC->ops->PolyText16;
	    break;
	case TT_IMAGE16:
	    drawFunc = pGC->ops->ImageText16;
	    break;
	}
	(*drawFunc) (pDraw, pGC, x, y, (int) count, chars);
#else /* don't AVOID_GLYPHBLT */
	/*
	 * On the other hand, if the device does use GlyphBlt ultimately to do text, we
	 * don't want to slow it down by invoking the text functions and having them call
	 * GetGlyphs all over again, so we go directly to the GlyphBlt functions here.
	 */
	drawFunc = imageblt ? pGC->ops->ImageGlyphBlt : pGC->ops->PolyGlyphBlt;
	(*drawFunc) (pDraw, pGC, x, y, n, charinfo, FONTGLYPHS(pGC->font));
#endif /* AVOID_GLYPHBLT */
    }
    DEALLOCATE_LOCAL(charinfo);
    return x + w;
}

static int
gsxSpritePolyText8(pDrawable, pGC, x, y, count, chars)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		x, y;
    int 	count;
    char	*chars;
{
    int	ret;

    GC_SETUP (pDrawable, pGC);

    GC_OP_PROLOGUE (pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
	ret = miSpriteText (pDrawable, pGC, x, y, (unsigned long)count, chars,
			    Linear8Bit, TT_POLY8, &pScreenPriv->saved);
    else
	ret = (*pGC->ops->PolyText8) (pDrawable, pGC, x, y, count, chars);

    GC_OP_EPILOGUE (pGC);
    return ret;
}

static int
gsxSpritePolyText16(pDrawable, pGC, x, y, count, chars)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    int	ret;

    GC_SETUP(pDrawable, pGC);

    GC_OP_PROLOGUE (pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
	ret = miSpriteText (pDrawable, pGC, x, y, (unsigned long)count,
			    (char *)chars,
			    FONTLASTROW(pGC->font) == 0 ?
			    Linear16Bit : TwoD16Bit, TT_POLY16, &pScreenPriv->saved);
    else
	ret = (*pGC->ops->PolyText16) (pDrawable, pGC, x, y, count, chars);

    GC_OP_EPILOGUE (pGC);
    return ret;
}

static void
gsxSpriteImageText8(pDrawable, pGC, x, y, count, chars)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		x, y;
    int		count;
    char	*chars;
{
    GC_SETUP(pDrawable, pGC);

    GC_OP_PROLOGUE (pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
	(void) miSpriteText (pDrawable, pGC, x, y, (unsigned long)count,
			     chars, Linear8Bit, TT_IMAGE8, &pScreenPriv->saved);
    else
	(*pGC->ops->ImageText8) (pDrawable, pGC, x, y, count, chars);

    GC_OP_EPILOGUE (pGC);
}

static void
gsxSpriteImageText16(pDrawable, pGC, x, y, count, chars)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    GC_SETUP(pDrawable, pGC);

    GC_OP_PROLOGUE (pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
	(void) miSpriteText (pDrawable, pGC, x, y, (unsigned long)count,
			     (char *)chars,
			    FONTLASTROW(pGC->font) == 0 ?
			    Linear16Bit : TwoD16Bit, TT_IMAGE16, &pScreenPriv->saved);
    else
	(*pGC->ops->ImageText16) (pDrawable, pGC, x, y, count, chars);

    GC_OP_EPILOGUE (pGC);
}

static void
gsxSpriteImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GCPtr 	pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    pointer 	pglyphBase;	/* start of array of glyphs */
{
    GC_SETUP(pDrawable, pGC);

    GC_OP_PROLOGUE (pGC);

    if (GC_CHECK((WindowPtr) pDrawable) &&
	miSpriteTextOverlap (pDrawable, pGC->font, x, y, nglyph, ppci, TRUE, 0, &pScreenPriv->saved))
    {
	gsxSpriteRemoveCursor(pDrawable->pScreen);
    }
    (*pGC->ops->ImageGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);

    GC_OP_EPILOGUE (pGC);
}

static void
gsxSpritePolyGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    pointer	pglyphBase;	/* start of array of glyphs */
{
    GC_SETUP (pDrawable, pGC);

    GC_OP_PROLOGUE (pGC);

    if (GC_CHECK((WindowPtr) pDrawable) &&
	miSpriteTextOverlap (pDrawable, pGC->font, x, y, nglyph, ppci, FALSE, 0, &pScreenPriv->saved))
    {
	gsxSpriteRemoveCursor(pDrawable->pScreen);
    }
    (*pGC->ops->PolyGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);

    GC_OP_EPILOGUE (pGC);
}

static void
gsxSpritePushPixels(pGC, pBitMap, pDrawable, w, h, x, y)
    GCPtr	pGC;
    PixmapPtr	pBitMap;
    DrawablePtr pDrawable;
    int		w, h, x, y;
{
    GC_SETUP(pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable) &&
	ORG_OVERLAP(&pScreenPriv->saved,pDrawable->x,pDrawable->y,x,y,w,h))
    {
	gsxSpriteRemoveCursor (pDrawable->pScreen);
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PushPixels) (pGC, pBitMap, pDrawable, w, h, x, y);

    GC_OP_EPILOGUE (pGC);
}

#ifdef NEED_LINEHELPER
/*
 * I don't expect this routine will ever be called, as the GC
 * will have been unwrapped for the line drawing
 */

static void
gsxSpriteLineHelper()
{
    FatalError("gsxSpriteLineHelper called\n");
}
#endif

/*
 * miPointer interface routines
 */

static Bool
gsxSpriteRealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    gsxSpriteScreenPtr	pScreenPriv;

    pScreenPriv = (gsxSpriteScreenPtr)
    			pScreen->devPrivates[gsxSpriteScreenIndex].ptr;
    if (pCursor == pScreenPriv->pCursor)
	pScreenPriv->checkPixels = TRUE;
    return (*pScreenPriv->funcs->RealizeCursor) (pScreen, pCursor);
}

static Bool
gsxSpriteUnrealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    gsxSpriteScreenPtr	pScreenPriv;

    pScreenPriv = (gsxSpriteScreenPtr) 
    			pScreen->devPrivates[gsxSpriteScreenIndex].ptr;
    if (pCursor == pScreenPriv->pCursor) 
    {
    	if (pScreenPriv->isUp)
          gsxSpriteRemoveCursor (pScreen);
        pScreenPriv->pCursor = 0;
    }
    return (*pScreenPriv->funcs->UnrealizeCursor) (pScreen, pCursor);
}

#ifdef MITSHM
static void gsxSpriteShmPutImage(dst, pGC, depth, format, 
			w, h, sx, sy, sw, sh, dx, dy, data)
    DrawablePtr dst;
    GCPtr	pGC;
    int		depth, w, h, sx, sy, sw, sh, dx, dy;
    unsigned int format;
    char 	*data;
{
    GC_SETUP_CHEAP(dst)

    if ((dst->type!=DRAWABLE_PIXMAP) && GC_CHECK((WindowPtr) dst))
    {
	if (ORG_OVERLAP(&pScreenPriv->saved,dst->x,dst->y,
			dx,dy,sw,sh))
 	{
	    gsxSpriteRemoveCursor (dst->pScreen);
	}
    }

    (*pScreenPriv->orgShmFuncsPtr->PutImage) (dst, pGC, depth, format, 
    			w, h, sx, sy, sw, sh, dx, dy, data);
}
#endif /* MITSHM */

static void
gsxSpriteSetCursor (pScreen, pCursor, x, y)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    gsxSpriteScreenPtr	pScreenPriv;

    pScreenPriv = (gsxSpriteScreenPtr) 
    			pScreen->devPrivates[gsxSpriteScreenIndex].ptr;
    if (!pCursor)
    {
    	pScreenPriv->shouldBeUp = FALSE;
    	if (pScreenPriv->isUp)
	    gsxSpriteRemoveCursor (pScreen);
	pScreenPriv->pCursor = 0;
	return;
    }
    pScreenPriv->shouldBeUp = TRUE;
    if (pScreenPriv->x == x &&
	pScreenPriv->y == y &&
	pScreenPriv->pCursor == pCursor &&
	!pScreenPriv->checkPixels)
    {
	return;
    }
    pScreenPriv->x = x;
    pScreenPriv->y = y;
    pScreenPriv->pCacheWin = NullWindow;
    if (pScreenPriv->checkPixels || pScreenPriv->pCursor != pCursor)
    {
	if (pScreenPriv->isUp)
            gsxSpriteRemoveCursor (pScreen);

        if (pScreenPriv->pCursor != pCursor ) {
	    int	cur_w, cur_h;
	    int hotx, hoty;

	    pScreenPriv->pCursor = pCursor;
            gsxSpriteComputeSaved (pScreen, &cur_w, &cur_h, &hotx, &hoty);
	}
	gsxSpriteFindColors (pScreen);
    }
    if (pScreenPriv->isUp) {
	int     width, height;
	int	sx, sy;
	/*
	 * check to see if the old saved region
	 * encloses the new sprite, in which case we use
	 * the flicker-free MoveCursor primitive.
	 */
        (*pScreenPriv->funcs->GetRealizedSize)(pScreen,  pCursor,
					&width, &height, &sx, &sy);
	sx = pScreenPriv->x - sx;
	sy = pScreenPriv->y - sy;
	if (sx + width >= pScreenPriv->saved.x1 &&
	    sx < pScreenPriv->saved.x2 &&
	    sy + height >= pScreenPriv->saved.y1 &&
	    sy < pScreenPriv->saved.y2 &&
	    width + (2 * GSX_SPRITE_PAD) ==
		pScreenPriv->saved.x2 - pScreenPriv->saved.x1 &&
	    height + (2 * GSX_SPRITE_PAD) ==
		pScreenPriv->saved.y2 - pScreenPriv->saved.y1
	    )
	{
	    pScreenPriv->isUp = FALSE;
	    if (!(sx >= pScreenPriv->saved.x1 &&
	      	  sx + width < pScreenPriv->saved.x2 &&
	      	  sy >= pScreenPriv->saved.y1 &&
	      	  sy + height < pScreenPriv->saved.y2))
	    {
		int oldx1, oldy1, dx, dy;

		oldx1 = pScreenPriv->saved.x1;
		oldy1 = pScreenPriv->saved.y1;
		dx = oldx1 - (sx - GSX_SPRITE_PAD);
		dy = oldy1 - (sy - GSX_SPRITE_PAD);
		pScreenPriv->saved.x1 -= dx;
		pScreenPriv->saved.y1 -= dy;
		pScreenPriv->saved.x2 -= dx;
		pScreenPriv->saved.y2 -= dy;
		(void) (*pScreenPriv->funcs->ChangeSave) (pScreen,
				pScreenPriv->saved.x1,
 				pScreenPriv->saved.y1,
				pScreenPriv->saved.x2 - pScreenPriv->saved.x1,
				pScreenPriv->saved.y2 - pScreenPriv->saved.y1,
				dx, dy);
	    }
	    (void) (*pScreenPriv->funcs->MoveCursor) (pScreen, pCursor,
				  pScreenPriv->saved.x1,
 				  pScreenPriv->saved.y1,
				  pScreenPriv->saved.x2 - pScreenPriv->saved.x1,
				  pScreenPriv->saved.y2 - pScreenPriv->saved.y1,
				  sx - pScreenPriv->saved.x1,
				  sy - pScreenPriv->saved.y1,
				  pScreenPriv->colors[SOURCE_COLOR].pixel,
				  pScreenPriv->colors[MASK_COLOR].pixel);
	    pScreenPriv->isUp = TRUE;
	}
	else
	{
	    gsxSpriteRemoveCursor (pScreen);
	}
    }
    if (!pScreenPriv->isUp && pScreenPriv->pCursor)
	gsxSpriteRestoreCursor (pScreen);
}

static void
gsxSpriteMoveCursor (pScreen, x, y)
    ScreenPtr	pScreen;
    int		x, y;
{
    gsxSpriteScreenPtr	pScreenPriv;

    pScreenPriv = (gsxSpriteScreenPtr) 
    			pScreen->devPrivates[gsxSpriteScreenIndex].ptr;
    gsxSpriteSetCursor (pScreen, pScreenPriv->pCursor, x, y);
}

/*
 * undraw/draw cursor
 */

void
gsxSpriteRemoveCursor (pScreen)
    ScreenPtr	pScreen;
{
    gsxSpriteScreenPtr   pScreenPriv;

    pScreenPriv = (gsxSpriteScreenPtr) 
    			pScreen->devPrivates[gsxSpriteScreenIndex].ptr;

    if (!pScreenPriv->isUp) {
        pScreenPriv->pCacheWin = NullWindow;
        return;
    }
    pScreenPriv->isUp = FALSE;
    pScreenPriv->pCacheWin = NullWindow;
    if (!(*pScreenPriv->funcs->RestoreUnderCursor) (pScreen,
				 pScreenPriv->saved.x1,
				 pScreenPriv->saved.y1,
				 pScreenPriv->saved.x2 - pScreenPriv->saved.x1,
				 pScreenPriv->saved.y2 - pScreenPriv->saved.y1))
    {
	pScreenPriv->isUp = TRUE;
    }
}

/*
 * Called from the block handler, restores the cursor
 * before waiting for something to do.
 */

void
gsxSpriteRestoreCursor (pScreen)
    ScreenPtr	pScreen;
{
    gsxSpriteScreenPtr  pScreenPriv;
    int			x, y;
    CursorPtr		pCursor;
    int                 hotx, hoty;
    int                 w,h;

    pScreenPriv = (gsxSpriteScreenPtr) 
    			pScreen->devPrivates[gsxSpriteScreenIndex].ptr;
    if (pScreenPriv->isUp) {
        gsxSpriteRemoveCursor (pScreen);
    }
    pCursor = pScreenPriv->pCursor;
    if (!pCursor)
    	return;

    gsxSpriteComputeSaved (pScreen, &w, &h, &hotx, &hoty);
    x = pScreenPriv->x - hotx;
    y = pScreenPriv->y - hoty;
    if ((*pScreenPriv->funcs->SaveUnderCursor) (pScreen,
			      pScreenPriv->saved.x1,
			      pScreenPriv->saved.y1,
			      pScreenPriv->saved.x2 - pScreenPriv->saved.x1,
			      pScreenPriv->saved.y2 - pScreenPriv->saved.y1))
    {
	if (pScreenPriv->checkPixels)
	    gsxSpriteFindColors (pScreen);
	if ((*pScreenPriv->funcs->PutUpCursor) (pScreen, pCursor, x, y,
				  pScreenPriv->colors[SOURCE_COLOR].pixel,
				  pScreenPriv->colors[MASK_COLOR].pixel))
	    pScreenPriv->isUp = TRUE;
    }
}

/*
 * compute the desired area of the screen to save
 */

static void
gsxSpriteComputeSaved (pScreen, pw, ph, photx, photy)
    ScreenPtr	pScreen;
    int *pw, *ph;
    int *photx, *photy;
{
    gsxSpriteScreenPtr   pScreenPriv;
    int		    x, y, w, h;
    int		    wpad, hpad;
    CursorPtr	    pCursor;

    pScreenPriv = (gsxSpriteScreenPtr) 
    			pScreen->devPrivates[gsxSpriteScreenIndex].ptr;
    pCursor = pScreenPriv->pCursor;

    (*pScreenPriv->funcs->GetRealizedSize)(pScreen, pCursor, 
    						&w, &h, photx, photy);
    x = pScreenPriv->x - *photx;
    y = pScreenPriv->y - *photy;
    *pw = w;
    *ph = h;
    wpad = GSX_SPRITE_PAD;
    hpad = GSX_SPRITE_PAD;
    pScreenPriv->saved.x1 = x - wpad;
    pScreenPriv->saved.y1 = y - hpad;
    pScreenPriv->saved.x2 = pScreenPriv->saved.x1 + w + wpad * 2;
    pScreenPriv->saved.y2 = pScreenPriv->saved.y1 + h + hpad * 2;

}

#endif /* GSX_ACCL_SPRITE_DC */
