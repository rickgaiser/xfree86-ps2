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

#include "gsx.h"

/*
 * Calculate total area of individual point
 */
static inline 
int CalcDrawPrimitiveRegion(
    int n,
    int margin)
{
    int region = 0;
    int width, height;

    while(--n > 0) {
        width  = margin * 2;
        height = margin * 2;
        region += width * height;
     }
     return region;
}

static inline
void gsxPolyPointSoftWhole(
    DrawablePtr dst,
    GCPtr   pGC,
    int     n,
    DDXPointPtr pPoint,
    xRectangle aRect)
{
    gsxScreenPtr pScreenPriv;
    PixmapPtr dstLop ;
    int nn ;
    GCPtr pGC1;
    int drawRegion_x;
    int drawRegion_y;

    if (!n) return;

    /* get region for LOP */
    drawRegion_x = aRect.x;
    drawRegion_y = aRect.y;
    aRect.x += dst->x ;
    aRect.y += dst->y ;
    
    dstLop = (*dst->pScreen->CreatePixmap)
        (dst->pScreen, aRect.width, aRect.height, dst->depth ) ;
    if( !dstLop ){
        return ;
    }
    /* copy destination */
    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    gsxReadImage(aRect.x, aRect.y, aRect.width, aRect.height,
	pScreenPriv->fbp,
	pScreenPriv->fbw,
	pScreenPriv->psm,
        dstLop);

    /* ValidateGC( destination = PIXMAP ) */
    pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
    CopyGC(pGC, pGC1, ~GCClipMask);
    GSXVALIDATEGC( (DrawablePtr)dstLop, pGC1 ) ;

    /* draw points */
    for( nn=0 ; nn < n ; nn++ ){
        pPoint[nn].x -= drawRegion_x;
        pPoint[nn].y -= drawRegion_y;
    }
    (*pGC1->ops->PolyPoint)( (DrawablePtr)dstLop, pGC1, CoordModeOrigin, n, pPoint ) ;

    FreeScratchGC(pGC1);

    /* ValidateGC( destination = WINDOW ) */
    GSXVALIDATEGC( dst, pGC ) ;

    /* post-proccess for LOP */
    cfbBitBlt( (DrawablePtr)dstLop, dst, pGC, 0, 0, aRect.width, aRect.height,
        drawRegion_x, drawRegion_y, gsxDoBitBlt, 0 ) ;

    (*dst->pScreen->DestroyPixmap)(dstLop) ;
}

void gsxPolyPointSoft(
    DrawablePtr dst,
    GCPtr   pGC,
    int     mode,
    int     n,
    DDXPointPtr pPoint)
{
    gsxScreenPtr pScreenPriv;
    xRectangle aRect ;
    PixmapPtr dstLop ;
    GCPtr pGC1;
    int margin;
    xPoint point;
    int width,height;
    int scrn_x, scrn_y;
    int dr;

    if(!n) return ;

    if (mode == CoordModePrevious) {
        DDXPointPtr p = pPoint;
	DDXPointPtr pLast = pPoint + n;
	int	x, y;
        x = p->x; y = p->y;
        while (++p < pLast) {
            x = p->x += x;
            y = p->y += y;
        }
    }
    /* select faster method */
    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    margin = 4;
    gsxCalcDrawRegion(pPoint, n, &aRect);
    dr = aRect.width * aRect.height; /*  area of every point */
    if (IS_SINGLE_OP_COVERED(dr,gsxGetBytePerPixel(pScreenPriv)) || 
    	      dr < CalcDrawPrimitiveRegion(n, margin)) {
        gsxPolyPointSoftWhole(dst, pGC, n, pPoint, aRect);
        return;
    }

    pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
    CopyGC(pGC, pGC1, ~GCClipMask);
    width  = margin * 2;
    height = margin * 2;
    point.x = margin;
    point.y = margin;

    while(--n > 0) {
        scrn_x = pPoint->x - margin + dst->x;
        scrn_y = pPoint->y - margin + dst->y;

        dstLop = (*dst->pScreen->CreatePixmap)
                     (dst->pScreen, width, height, dst->depth );

        if( !dstLop ){
            return ;
        }

        /* ValidateGC( destination = PIXMAP ) */
        GSXVALIDATEGC( (DrawablePtr)dstLop, pGC1 ) ;

        /* copy destination */
        gsxReadImage(scrn_x, scrn_y, width, height,
	    pScreenPriv->fbp,
	    pScreenPriv->fbw,
	    pScreenPriv->psm,
            dstLop);

        /* draw lines */
        (*pGC1->ops->PolyPoint)((DrawablePtr)dstLop, pGC1, CoordModeOrigin, 1, &point);


        /* ValidateGC( destination = WINDOW ) */
        GSXVALIDATEGC( dst, pGC ) ;

        /* post-proccess for LOP */
        cfbBitBlt((DrawablePtr)dstLop, dst, pGC,
		0, 0, width, height,
		scrn_x - dst->x, scrn_y - dst->y,
		gsxDoBitBlt, 0);

        (*dst->pScreen->DestroyPixmap)(dstLop) ;
        pPoint++;
     }
     FreeScratchGC(pGC1);
}



static inline
void doPolyPoint(dst, pGC, mode, n, pPoint)
    DrawablePtr dst; 
    GCPtr pGC; 
    int mode; 
    int n; 
    DDXPointPtr pPoint ;
{
    gsxGCPtr    pPriv = gsxGetGCPriv(pGC);
    BoxPtr      pbox = REGION_RECTS(pPriv->pCompositeClip);
    int         numRects = REGION_NUM_RECTS(pPriv->pCompositeClip);
    BoxPtr      plast = pbox + numRects;
    int xorg = dst->x, yorg = dst->y;
    DDXPointPtr p = pPoint, pLast = pPoint + n;
    int x = p->x, y = p->y;
    int rgb, r, g, b, a ;
    int i;
    int nCount;
    int pc;
    int packets;
    int result;
    GSOSbit64 rgba;
    gsxScreenPtr pScreenPriv;
    
    if (!n) return;

    if (mode == CoordModePrevious) {
        while (++p < pLast) {
            x = p->x += x;
            y = p->y += y;
        }
    }

    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    gsxSetColormask(pScreenPriv, pPriv->fbmask);

    rgb = pGC->fgPixel & gsxGetScreenPriv(dst->pScreen)->rgb_pix_mask;
    gsxGetRgba( pGC, rgb, &r, &g, &b, &a ) ;

    /* set RGB */
    gsosMakeGiftag(1,
    	GSOS_GIF_PRE_IGNORE, 0, 
	GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);

    rgba = r | g << 8 |  b << 16 |  a << 24 ;
    gsosSetPacketAddrData(GSOS_RGBAQ, rgba) ;


    while (pbox < plast) {
        gsosMakeGiftag(1,
		GSOS_GIF_PRE_IGNORE, 0, 
		GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD );
        /* set SCISSOR rect */
        gsosSetPacketAddrData4( GSOS_SCISSOR_1, 
            (GSOSbit64)pbox->x1, (GSOSbit64)pbox->x2-1,
            (GSOSbit64)pbox->y1, (GSOSbit64)pbox->y2-1 ) ;


	p = pPoint;
	nCount = n;
	while(nCount > 0) {
	  if (nCount == 1) {
	    packets = 1;
	  } else {
	    pc = gsosCalcPacketCount(1, GSOS_GIF_FLG_PACKED);
	    if(pc == 0) {	/* can't allocate buffer */
	      gsosExec();		/* flush DMA */
	      pc = gsosCalcMaxPacketCount(1, GSOS_GIF_FLG_PACKED);
	    }
	    packets = (nCount < pc) ? nCount : pc;
	  }
	  
	  result = gsosMakeGiftag(packets,
	  		GSOS_GIF_PRE_ENABLE, GSOS_PRIM_POINT,
			GSOS_GIF_FLG_PACKED, 1,
			GSOS_GIF_REG_XYZ2);
	  if(result) break;
	  for(i = 0; i < packets; i++, p++) {
            gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(p->x + xorg), 
			   GSOS_SUBPIX_OFST(p->y + yorg));
	  }
	  nCount -= packets;
	}
        pbox++;
    }
    gsosExec() ;
}

void gsxPolyPointGxcpy(dst,pGC,mode,n,pPoint)
    DrawablePtr dst;
    GCPtr   pGC;
    int     mode;
    int     n;
    DDXPointPtr pPoint;
{
    doPolyPoint( dst, pGC, mode, n, pPoint ) ;
}

