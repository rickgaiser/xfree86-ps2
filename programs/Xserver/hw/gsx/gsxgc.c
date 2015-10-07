/*

Copyright (C) 2000  Sony Computer Entertainment Inc.

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

Except as contained in this notice, the name of the 
Sony Computer Entertainment Inc. shall not be used in advertising or 
otherwise to promote the sale, use or other dealings in this Software without
prior written authorization from the Sony Computer Entertainment Inc.

*/

#include "font.h"
#include "servermd.h"
#include "pixmap.h"
#include "mi.h"
#include "gsx.h"
#include "common/xf86.h"

#define GC_FUNC_PROLOGUE(pGC) \
    gsxGCPtr pGCPriv = (gsxGCPtr)(pGC)->devPrivates[gsxGCIndex].ptr ; \
    GCOpsPtr oldOps = (pGC)->ops ; \
    (pGC)->funcs = pGCPriv->funcs ; \
    if( pGCPriv->mops ) (pGC)->ops = pGCPriv->mops ;

#define GC_FUNC_EPILOGUE(pGC) \
    pGCPriv->funcs = (pGC)->funcs ; \
    (pGC)->funcs = (GCFuncs *)&gsxFuncs ; \
    if( pGCPriv->mops ){ \
        pGCPriv->mops = (pGC)->ops ; \
        (pGC)->ops = oldOps ; \
    }

int gsxGCIndex = 0 ;
int gsxScreenIndex = 0 ;


static
const
GCFuncs gsxFuncs = {
    gsxValidateGC,
    gsxChangeGC,
    gsxCopyGC,
    gsxDestroyGC,
    gsxChangeClip,
    gsxDestroyClip,
    gsxCopyClip
};

static
int mopsPolyText8(dst, pGC, x, y, count, chars)
    DrawablePtr     dst;
    GCPtr       pGC;
    int         x, y;
    int         count;
    char        *chars;
{
    return 
    (*gsxGetGCPriv(pGC)->mops->PolyText8)(dst, pGC, x, y, count, chars);
}

static
void mopsImageText8(dst, pGC, x, y, count, chars)
    DrawablePtr     dst;
    GCPtr       pGC;
    int         x, y;
    int         count;
    char        *chars;
{
    (*gsxGetGCPriv(pGC)->mops->ImageText8)(dst, pGC, x, y, count, chars);
}


static void mopsFillSpans(dst,pGC,nSpans,pPoints,pWidths,sorted)
    DrawablePtr	dst;
    GCPtr	pGC;
    int		nSpans;
    xPoint	*pPoints;
    int		*pWidths;
    int		sorted;
{
    (*gsxGetGCPriv(pGC)->mops->FillSpans)(dst,pGC,nSpans,pPoints,pWidths,sorted);
}

static void mopsSetSpans(dst,pGC,pSrc,pPoints,pWidths,nSpans,sorted)
    DrawablePtr	dst;
    GCPtr	pGC;
    unsigned char *pSrc;
    xPoint	*pPoints;
    int		*pWidths;
    int		nSpans;
    int		sorted;
{
    (*gsxGetGCPriv(pGC)->mops->SetSpans)(dst,pGC,pSrc,pPoints,pWidths,nSpans,sorted);
}

static void mopsPutImage(dst,pGC,depth,x,y,w,h,leftPad,format,pBinImage)
    DrawablePtr	dst;
    GCPtr	pGC;
    int		depth;
    int		x, y, w, h;
    int		leftPad;
    unsigned int format;
    char	*pBinImage;
{
    (*gsxGetGCPriv(pGC)->mops->PutImage)(dst,pGC,depth,x,y,w,h,leftPad,format,pBinImage);
}

static void mopsPolyPoint(dst,pGC,mode,n,pPoint)
    DrawablePtr	dst;
    GCPtr	pGC;
    int		mode;
    int		n;
    xPoint	*pPoint;
{
    (*gsxGetGCPriv(pGC)->mops->PolyPoint)(dst,pGC,mode,n,pPoint);
}

static void mopsPolylines(dst,pGC,mode,n,pPoint)
    DrawablePtr	dst;
    GCPtr	pGC;
    int		mode;
    int		n;
    xPoint	*pPoint;
{
    (*gsxGetGCPriv(pGC)->mops->Polylines)(dst,pGC,mode,n,pPoint);
}

static void mopsPolySegment(dst,pGC,n,pSegments)
    DrawablePtr	dst;
    GCPtr	pGC;
    int		n;
    xSegment	*pSegments;
{
    (*gsxGetGCPriv(pGC)->mops->PolySegment)(dst,pGC,n,pSegments);
}

static void mopsPolyRectangle(dst,pGC,n,pRect)
    DrawablePtr	dst;
    GCPtr	pGC;
    int		n;
    xRectangle	*pRect;
{
    (*gsxGetGCPriv(pGC)->mops->PolyRectangle)(dst,pGC,n,pRect);
}

static void mopsPolyArc(dst,pGC,n,pArc)
    DrawablePtr	dst;
    GCPtr	pGC;
    int		n;
    xArc	*pArc;
{
    (*gsxGetGCPriv(pGC)->mops->PolyArc)(dst,pGC,n,pArc);
}

static void mopsFillPolygon(dst,pGC,shape,mode,count,pPoint)
    DrawablePtr	dst;
    GCPtr	pGC;
    int		shape;
    int		mode;
    int		count;
    xPoint	*pPoint;
{
    (*gsxGetGCPriv(pGC)->mops->FillPolygon)(dst,pGC,shape,mode,count,pPoint);
}

static void mopsPolyFillRect(dst,pGC,n,pRect)
    DrawablePtr dst;
    GCPtr       pGC;
    int         n;
    xRectangle  *pRect;
{
    (*gsxGetGCPriv(pGC)->mops->PolyFillRect)(dst,pGC,n,pRect);
}

static void mopsPolyFillArc(dst,pGC,n,pArc)
    DrawablePtr	dst;
    GCPtr	pGC;
    int		n;
    xArc	*pArc;
{
    (*gsxGetGCPriv(pGC)->mops->PolyFillArc)(dst,pGC,n,pArc);
}

static void mopsImageGlyphBlt(dst,pGC,x,y,nglyph,ppci,pglyphBase)
    DrawablePtr	dst;
    GCPtr	pGC;
    int		x;
    int		y;
    unsigned int nglyph;
    CharInfoPtr	*ppci;
    char	*pglyphBase;
{
    (*gsxGetGCPriv(pGC)->mops->ImageGlyphBlt)(dst,pGC,x,y,nglyph,ppci,pglyphBase);
}

static void mopsPolyGlyphBlt(dst,pGC,x,y,nglyph,ppci,pglyphBase)
    DrawablePtr	dst;
    GCPtr	pGC;
    int		x;
    int		y;
    unsigned int nglyph;
    CharInfoPtr	*ppci;
    char	*pglyphBase;
{
    (*gsxGetGCPriv(pGC)->mops->PolyGlyphBlt)(dst,pGC,x,y,nglyph,ppci,pglyphBase);
}

static void mopsPushPixels(pGC,pBitMap,dst,dx,dy,xOrg,yOrg)
    GCPtr	pGC;
    PixmapPtr	pBitMap;
    DrawablePtr	dst;
    int		dx;
    int		dy;
    int		xOrg;
    int		yOrg;
{
    (*gsxGetGCPriv(pGC)->mops->PushPixels)(pGC,pBitMap,dst,dx,dy,xOrg,yOrg);
}


static const
GCOps mopsOps = {
    mopsFillSpans,
    mopsSetSpans,
    mopsPutImage,
    gsxCopyAreaGxcpy,
    gsxCopyPlaneGxcpy,
    mopsPolyPoint,
    mopsPolylines,
    mopsPolySegment,
    mopsPolyRectangle,
    mopsPolyArc,
    mopsFillPolygon,
    mopsPolyFillRect,
    mopsPolyFillArc,
    mopsPolyText8,
    miPolyText16,
    mopsImageText8,
    miImageText16,
    mopsImageGlyphBlt,
    mopsPolyGlyphBlt,
    mopsPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};


static const
GCOps gsxLopOps = {
    gsxFillSpansSoft,
    gsxSetSpansSoft,
    gsxPutImage,
    gsxCopyAreaLop,
    gsxCopyPlaneLop,
    gsxPolyPointSoft,
    gsxPolyLinesSoft,
    gsxPolySegmentSoft,
    gsxPolyRectangleSoft,
    gsxPolyArcSoft,
    gsxFillPolygonSoft,
    gsxPolyFillRectSoft,
    gsxPolyFillArcSoft,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    gsxImageGlyphBltSoft,
    gsxPolyGlyphBltSoft,
    gsxPushPixelsSoft
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};


static const
GCOps gsxGxcpyOps = {
    gsxFillSpansGxcpy,
    gsxSetSpansGxcpy,
    gsxPutImage,
    gsxCopyAreaGxcpy,
    gsxCopyPlaneGxcpy,
    gsxPolyPointGxcpy,
    gsxPolyLinesSoft,
    gsxPolySegmentSoft,
    gsxPolyRectangleSoft,
    miPolyArc,
    miFillPolygon,
    gsxPolyFillRectGxcpy,
    miPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    gsxImageGlyphBltGxcpy,
    gsxPolyGlyphBltSoft,
    gsxPushPixelsSoft
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static Bool last_xf86VTSema = FALSE;

void
gsxValidateGC(pGC, changes, dst)
    register GCPtr	pGC;
    Mask		changes;
    DrawablePtr		dst;
{
    gsxGCPtr		pPriv = gsxGetGCPriv(pGC);
    Mask		mask, index;
    Bool		new_patt = FALSE, new_fill = FALSE, new_line = FALSE;
    Bool		new_lop = FALSE;
    Bool		new_mask = FALSE;
    Bool		new_fgbg = FALSE;
    Bool		fast_line = FALSE;
    Bool		fast_span = FALSE;
    Bool		fast_fill = FALSE;
    ScreenPtr		pScreen = pGC->pScreen;
    Bool		VTSwitch;

    VTSwitch = (last_xf86VTSema != xf86VTSema);
    if (!xf86VTSema) {
#ifdef DEBUG
	ErrorF("gsxValidateGC: screen has been virtualized\n");
#endif
	pGC->ops = pPriv->mops;
	(*pPriv->funcs->ValidateGC)(pGC, changes, dst);
	pPriv->mops = pGC->ops;
	return;
    }
    if (VTSwitch) {
	last_xf86VTSema = xf86VTSema;
#ifdef DEBUG
	ErrorF("gsxValidateGC: switched %d\n", xf86VTSema);
#endif
    }

    pGC->ops = pPriv->mops;
    (*pPriv->funcs->ValidateGC)(pGC, changes, dst);
    pPriv->mops = pGC->ops;
    
    if (dst->type == DRAWABLE_PIXMAP) {
        pGC->ops = (GCOps*)&mopsOps;
        pPriv->changes |= changes;
        return;
    }

    mask = changes | pPriv->changes;
    pPriv->changes = 0;

    while (mask) {
        index = lowbit (mask);
        mask &= ~index;

        switch (index) {
        case GCFunction:
            new_lop = TRUE ;
            break;

        case GCBackground:
        case GCForeground:
	    new_fgbg = TRUE;
            break;

        case GCPlaneMask:
            new_lop = TRUE ;
            new_mask = TRUE ;
            break;

        case GCLineWidth:
            new_line = TRUE;
            break;

        case GCJoinStyle:
        case GCCapStyle:
            break;

        case GCLineStyle:
            new_line = TRUE;
            break;

        case GCFillStyle:
            new_fill = TRUE;
            if (pGC->fillStyle != FillSolid)
                new_patt = TRUE;
            break;

        case GCFillRule:
            break;

        case GCTile:
            if (pGC->fillStyle==FillTiled)
                new_fill = new_patt = TRUE;
            break;

        case GCStipple:
            if (pGC->fillStyle==FillStippled
                || pGC->fillStyle==FillOpaqueStippled)
                new_fill = new_patt = TRUE;
            break;

        case GCTileStipXOrigin:
        case GCTileStipYOrigin:
            if (pGC->fillStyle != FillSolid)
                new_patt = TRUE;
            break;

        case GCFont:
        case GCSubwindowMode:
        case GCGraphicsExposures:
        case GCClipXOrigin:
        case GCClipYOrigin:
        case GCClipMask:
            break;

        case GCDashOffset:
        case GCDashList:
            new_line = TRUE;
            break;

        case GCArcMode:
        default:
            break;
        }
    }

    if (new_fgbg) {
      if (pGC->fillStyle != FillSolid) 
        new_fill =  TRUE;
      if (pGC->lineStyle != LineSolid) 
        new_line = TRUE;
    }

    if (new_patt) {
      if (pGC->fillStyle != FillTiled) {
	pPriv->patW = pGC->stipple->drawable.width;
	pPriv->patH = pGC->stipple->drawable.height;
      } else if (pGC->tileIsPixel) {
	pPriv->patW = 1;
	pPriv->patH = 1;
      } else {
	pPriv->patW = pGC->tile.pixmap->drawable.width;
	pPriv->patH = pGC->tile.pixmap->drawable.height;
      }
    }

    if (new_mask) {
      pPriv->fbmask = gsxGetColormask( pGC, GSX_COLMASK_ASHUT ) ;
    }
    if (new_line || new_fill || new_lop || new_patt) {

        if (pGC->ops != &pPriv->ops) {
            pPriv->ops = *pGC->ops;
            pGC->ops = &pPriv->ops;
        }

        *pGC->ops = gsxLopOps;
        if ((pGC->alu == GXcopy) && (pGC->fillStyle == FillSolid)) {
            *pGC->ops = gsxGxcpyOps;
            if (new_mask && gsxIsPlanemask(pGC)) {
	        pGC->ops->CopyArea = gsxCopyAreaLop;
                pGC->ops->CopyPlane = gsxCopyPlaneLop;
	    }
        }
#ifdef DEBUG
	if (new_line) {
		ErrorF("alu:%x fillStyle:%x lineStyle:%x\n",
			pGC->alu, 
			pGC->fillStyle,
			pGC->lineStyle);
	}
#endif
        if (pGC->alu == GXcopy){
	    if (!gsxIsPlanemask(pGC)) {
	        pGC->ops->CopyArea = gsxCopyAreaGxcpy;
                pGC->ops->CopyPlane = gsxCopyPlaneGxcpy;
	    }
            pGC->ops->PolyPoint = gsxPolyPointGxcpy;
            pGC->ops->ImageGlyphBlt = gsxImageGlyphBltGxcpy;
            if (pGC->fillStyle == FillSolid
                    	|| pGC->fillStyle == FillStippled ){
                pGC->ops->ImageText8 = gsxImageText8Cached;
                pGC->ops->PolyText8 = gsxPolyText8Cached;
            }
            if (pGC->lineWidth <= 1) {
	        if (pGC->fillStyle == FillSolid) { 
                    pGC->ops->PolyArc = miZeroPolyArc;
		    if(pGC->lineStyle == LineSolid) {
                        pGC->ops->Polylines = gsxPolyLinesGxcpy;
                        pGC->ops->PolySegment = gsxPolySegmentGxcpy;
                        pGC->ops->PolyRectangle = gsxPolyRectangleGxcpy;
#if GSX_ACCL_DASH
		    } else if (gsxCheckDash(pGC, FALSE)) {
                        pGC->ops->PolyRectangle = gsxPolyRectangleDashCT;
                        pGC->ops->Polylines = gsxPolyLinesDashCT;
                        pGC->ops->PolySegment = gsxPolySegmentDashCT;
#endif
		    }
	        }
            }
        }
#if GSX_CACHED_TILE
	if (new_fill){ 
            if (pPriv->pCachedTile) {
                gsxDestroyCT(pScreen, pPriv->pCachedTile);
                pPriv->pCachedTile = NULL;
            }
	    if (pGC->alu == GXcopy && pGC->fillStyle != FillSolid) {

		if (pGC->fillStyle == FillTiled) {
                    if (!pGC->tileIsPixel) {
                        pPriv->pCachedTile = 
			    gsxGetCT(pScreen, pGC, pGC->tile.pixmap);
                    }
                } else {
                    if (pGC->stipple) {
                        pPriv->pCachedTile = 
			    gsxGetCT(pScreen, pGC, pGC->stipple);
                    }
		}
                if (pPriv->pCachedTile) {
                    pGC->ops->PolyFillRect = gsxPolyFillRectCached;
                    pGC->ops->FillPolygon = miFillPolygon;
                    pGC->ops->FillSpans = gsxFillSpansCached;
	        } else {
                    pGC->ops->PolyFillRect = gsxPolyFillRectSoft;
                    pGC->ops->FillPolygon = gsxFillPolygonSoft;
                    pGC->ops->FillSpans = gsxFillSpansSoft;
	        }
            }
        }
#endif

	fast_span = pGC->ops->FillSpans == gsxFillSpansGxcpy 
#if GSX_ACCL_DASH
		|| pGC->ops->FillSpans == gsxFillSpansCached
#endif
		;
			
	if ((new_fill || new_line) && fast_span &&
		((pGC->lineStyle == LineSolid)  ||
		 (pGC->fillStyle == FillSolid))) {

	    fast_line = pGC->ops->Polylines == gsxPolyLinesGxcpy 
#if GSX_ACCL_DASH
		|| pGC->ops->Polylines == gsxPolyLinesDashCT
#endif
		;

	    fast_fill = pGC->ops->PolyFillRect == gsxPolyFillRectGxcpy
#if GSX_CACHED_TILE
		|| pGC->ops->PolyFillRect == gsxPolyFillRectCached
#endif
		;

	    if (!fast_line) {
                  pGC->ops->PolySegment = miPolySegment;
                  pGC->ops->PolyRectangle = miPolyRectangle;

                  pGC->ops->PolyArc = miPolyArc;
	          pGC->ops->Polylines = (pGC->lineStyle == LineSolid)  ?
			    miWideLine : miWideDash;

                  if (pGC->lineWidth <= 1) {
		      Bool fast_point;
		      Bool dospans_in_arc;

		      if (pGC->lineStyle == LineSolid) {
		          pGC->ops->Polylines = miZeroLine;
			  pGC->ops->PolySegment = miPolySegment;
		      }  else {
		          // miZeroDashLine() is slower than gsxPolyLinesSoft().
		          pGC->ops->Polylines = gsxPolyLinesSoft;
		          // miPolySegment depends on PolyLines
			  pGC->ops->PolySegment = gsxPolySegmentSoft;
		      }

		      dospans_in_arc = (pGC->lineStyle != LineSolid) 
		      			|| (pGC->fillStyle != FillSolid);
		      fast_point =  pGC->ops->PolyPoint == gsxPolyPointGxcpy;

		      if (dospans_in_arc || fast_point)
                         pGC->ops->PolyArc = miZeroPolyArc;
	         }
	    }
	    if (!fast_fill) {
	        pGC->ops->FillPolygon = miFillPolygon;
	        pGC->ops->PolyFillRect = miPolyFillRect;
	    }
	}
        pPriv->wops = pGC->ops;

    }else{    /* if GC is not changed */

        pGC->ops = pPriv->wops;
    }
}

Bool
gsxCreateGC(pGC)
    register GCPtr	pGC;
{
    gsxGCPtr	pPriv = gsxGetGCPriv(pGC);

    switch (BitsPerPixel(pGC->depth)) {
    case 1:
        if (! mfbCreateGC(pGC)) return FALSE;
    case 8:
        if (! cfbCreateGC(pGC)) return FALSE;
        break;
    case 16:
        if (! cfb16CreateGC(pGC)) return FALSE;
        break;
    case 32:
        if (! cfb32CreateGC(pGC)) return FALSE;
        break;
    default:
        ErrorF("gsxCreteGC: illegal bits per pixel\n");
        return FALSE;
    }

    pPriv->changes = 0;
#if GSX_CACHED_TILE
    pPriv->pCachedTile = NULL;
#endif

    pPriv->funcs = pGC->funcs;
    pPriv->mops = pGC->ops;
    pPriv->wops = (GCOps*) &gsxGxcpyOps;

    pGC->funcs = (GCFuncs *)&gsxFuncs;
    pGC->ops = pPriv->wops;

    pPriv->fbmask = gsxGetColormask( pGC, GSX_COLMASK_ASHUT ) ;

    return TRUE;
}

void gsxChangeGC(pGC,mask)
    GCPtr	pGC;
    BITS32	mask;
{
    (*gsxGetGCPriv(pGC)->funcs->ChangeGC)(pGC,mask);
}

void gsxCopyGC(pGC,changes,pGCdst)
    GCPtr	pGC;
    Mask	changes;
    GCPtr	pGCdst;
{
    (*gsxGetGCPriv(pGC)->funcs->CopyGC)(pGC,changes,pGCdst);
}

void gsxDestroyGC(pGC)
    GCPtr	pGC;
{
#if GSX_CACHED_TILE
    gsxGCPtr pPriv;
#endif
    GC_FUNC_PROLOGUE( pGC ) ;
    (*pGC->funcs->DestroyGC)(pGC) ;
#if GSX_CACHED_TILE
    pPriv = gsxGetGCPriv(pGC);
    if (pPriv->pCachedTile) {
        gsxDestroyCT(pGC->pScreen, pPriv->pCachedTile);
	pPriv->pCachedTile = NULL;
    }
#endif
    GC_FUNC_EPILOGUE( pGC ) ;
}

void gsxChangeClip(pGC,type,pvalues,nrects)
    GCPtr	pGC;
    unsigned int type;
    pointer	pvalues;
    int		nrects;
{
    (*gsxGetGCPriv(pGC)->funcs->ChangeClip)(pGC,type,pvalues,nrects);
}

void gsxDestroyClip(pGC)
    GCPtr	pGC;
{
    (*gsxGetGCPriv(pGC)->funcs->DestroyClip)(pGC);
}

void gsxCopyClip(pGC,pGCdst)
    GCPtr	pGC;
    GCPtr	pGCdst;
{
    (*gsxGetGCPriv(pGC)->funcs->CopyClip)(pGC,pGCdst);
}

