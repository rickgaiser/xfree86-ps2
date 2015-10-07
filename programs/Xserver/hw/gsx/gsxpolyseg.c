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

static inline
int CalcDrawPrimitiveRegionSeg(
    xSegment *pSegments,
    int n,
    int margin)
{
  int region = 0;
  int x1,y1,x2,y2;
  int width, height;
  int i;

  for(i = 0; i < n; i++) {
    x1 = pSegments[i].x1;
    y1 = pSegments[i].y1;
    x2 = pSegments[i].x2;
    y2 = pSegments[i].y2;

    width = abs(x2 - x1);
    height = abs(y2 - y1);
    width  += margin * 2;
    height += margin * 2;
    region += width * height;
  }
  return region;
}

static inline
void gsxPolySegmentSoftWhole(
    DrawablePtr dst,
    GCPtr       pGC,
    int         n,
    xSegment    *pSegments,
    xRectangle  aRect)
{
    gsxGCPtr pPriv = gsxGetGCPriv(pGC) ;
    gsxScreenPtr pScreenPriv;
    PixmapPtr dstLop ;
    int nn ;
    GCPtr pGC1;
    int margin;
    int drawRegion_x;
    int drawRegion_y;

    if(!n) return ;

    /* get region for LOP */
    margin = gsxGetLineMargin(pGC);
    aRect.x += dst->x - margin;
    aRect.y += dst->y - margin;
    aRect.width  += margin * 2;
    aRect.height += margin * 2;
    drawRegion_x = aRect.x - dst->x;
    drawRegion_y = aRect.y - dst->y;

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

    /* adjust pattern origin */
    if (pGC->fillStyle != FillSolid) {
      ChangeGCVal gcval[2];
      gcval[0].val = pGC->patOrg.x + (pPriv->patW - drawRegion_x % pPriv->patW);
      gcval[1].val = pGC->patOrg.y + (pPriv->patH - drawRegion_y % pPriv->patH);
      DoChangeGC (pGC1, GCTileStipXOrigin|GCTileStipYOrigin, 
      			(XID *)gcval, TRUE);
    }
    GSXVALIDATEGC( (DrawablePtr)dstLop, pGC1 ) ;

    /* draw */
    for( nn = 0 ; nn < n ; nn++ ) {
        pSegments[nn].x1 -= drawRegion_x;
        pSegments[nn].x2 -= drawRegion_x;
        pSegments[nn].y1 -= drawRegion_y;
        pSegments[nn].y2 -= drawRegion_y;
    }
    (*pGC1->ops->PolySegment)( (DrawablePtr)dstLop, pGC1, n, pSegments ) ;

    FreeScratchGC(pGC1);

    /* ValidateGC( destination = WINDOW ) */
    GSXVALIDATEGC( dst, pGC ) ;

    /* post-proccess for LOP */
    cfbBitBlt( (DrawablePtr)dstLop, dst, pGC, 0, 0, aRect.width, aRect.height,
        drawRegion_x, drawRegion_y, gsxDoBitBlt, 0 ) ;

    (*dst->pScreen->DestroyPixmap)(dstLop) ;
}

void gsxPolySegmentSoft(dst,pGC,n,pSegments)
    DrawablePtr dst;
    GCPtr       pGC;
    int         n;
    xSegment    *pSegments;
{
    gsxGCPtr   pPriv = gsxGetGCPriv(pGC) ;
    gsxScreenPtr pScreenPriv;
    xRectangle aRect ;
    PixmapPtr  dstLop ;
    GCPtr      pGC1;
    int        margin;
    int        width,height;
    int        scrn_x, scrn_y;
    int        winorg_x, winorg_y;
    int        dr;
    int        x1, y1;
    xSegment   seg;

    if(!n) return ;

    /* select faster method */
    margin = gsxGetLineMargin(pGC);
    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    gsxCalcDrawRegionSeg( pSegments, n, &aRect ) ;
    dr = aRect.width * aRect.height; /*  area of every line */
    if (IS_SINGLE_OP_COVERED(dr,gsxGetBytePerPixel(pScreenPriv)) || 
    	    dr < CalcDrawPrimitiveRegionSeg(pSegments, n, margin)) {
        gsxPolySegmentSoftWhole(dst, pGC, n, pSegments, aRect);
        return;
    }

    pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
    CopyGC(pGC, pGC1, ~GCClipMask);

    while (n-- > 0) {
      x1 = min(pSegments->x1, pSegments->x2);
      width = abs(pSegments->x2 - pSegments->x1);
      y1 = min(pSegments->y1, pSegments->y2);
      height = abs(pSegments->y2 - pSegments->y1);

      width  += margin * 2;
      height += margin * 2;
      scrn_x = x1 - margin + dst->x;
      scrn_y = y1 - margin + dst->y;
      winorg_x = scrn_x - dst->x;
      winorg_y = scrn_y - dst->y;

      dstLop = (*dst->pScreen->CreatePixmap)
        (dst->pScreen, width, height, dst->depth ) ;
      if( !dstLop ){
        return ;
      }
      /* copy destination */
      gsxReadImage(scrn_x, scrn_y, width, height,
		   pScreenPriv->fbp,
		   pScreenPriv->fbw,
		   pScreenPriv->psm,
		   dstLop);

      /* adjust pattern origin */
      if (pGC->fillStyle != FillSolid) {
          ChangeGCVal gcval[2];
          gcval[0].val = pGC->patOrg.x + (pPriv->patW - winorg_x % pPriv->patW);
          gcval[1].val = pGC->patOrg.y + (pPriv->patH - winorg_y % pPriv->patH);
          DoChangeGC (pGC1, GCTileStipXOrigin|GCTileStipYOrigin, 
      			(XID *)gcval, TRUE);
      }
      /* ValidateGC( destination = PIXMAP ) */
      GSXVALIDATEGC( (DrawablePtr)dstLop, pGC1 ) ;

      /* draw */
      seg.x1 = pSegments->x1 - x1 + margin;
      seg.y1 = pSegments->y1 - y1 + margin;
      seg.x2 = pSegments->x2 - x1 + margin;
      seg.y2 = pSegments->y2 - y1 + margin;
      pSegments++;
      (*pGC1->ops->PolySegment)((DrawablePtr)dstLop, pGC1, 1, &seg);


      /* ValidateGC( destination = WINDOW ) */
      GSXVALIDATEGC( dst, pGC ) ;

      /* post-proccess for LOP */
      cfbBitBlt( (DrawablePtr)dstLop, dst, pGC, 0, 0, width, height,
		 winorg_x, winorg_y, gsxDoBitBlt, 0 ) ;

      (*dst->pScreen->DestroyPixmap)(dstLop) ;
    }

    FreeScratchGC(pGC1);
}

static inline
void doPolySegment(dst,pGC,n,pSegments)
    DrawablePtr dst;
    GCPtr   pGC;
    int     n;
    xSegment    *pSegments;
{
    gsxGCPtr    pPriv = gsxGetGCPriv(pGC);
    BoxPtr      pbox  = REGION_RECTS(pPriv->pCompositeClip);
    BoxPtr      plast = pbox + REGION_NUM_RECTS(pPriv->pCompositeClip);
    int xorg = dst->x, yorg = dst->y;
    xSegment *p = pSegments;
    int rgb, r, g, b, a ;
    int i;
    int nCount;
    int pc;
    int packets;
    int result;
    GSOSbit64 rgba;
    gsxScreenPtr pScreenPriv;

    if (!n) return;

    /*    endp = (pGC->capStyle==CapNotLast)? GSX_DISABLE:GSX_ENABLE; */

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

    pbox  = REGION_RECTS(pPriv->pCompositeClip);
    while (pbox < plast) {
        gsosMakeGiftag(1,
		GSOS_GIF_PRE_IGNORE, 0, 
		GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);
        /* set SCISSOR rect */
        gsosSetPacketAddrData4( GSOS_SCISSOR_1,
            (GSOSbit64)pbox->x1, (GSOSbit64)pbox->x2-1,
            (GSOSbit64)pbox->y1, (GSOSbit64)pbox->y2-1 );
        /*
           draw lines
           actually we should consider capStyle == CapNotLast
        */

        p = pSegments ;
	nCount = n;
	while(nCount > 0) {
	  if (nCount == 1) {
	    packets = 1;
	  } else {
	    pc = gsosCalcPacketCount(2, GSOS_GIF_FLG_PACKED)/2;
	    if(pc == 0) {	/* can't allocate buffer */
	      gsosExec();		/* flush DMA */
	      pc = gsosCalcMaxPacketCount(2, GSOS_GIF_FLG_PACKED)/2;
	    }
	    packets = (nCount < pc) ? nCount : pc;
	  }

	  result = gsosMakeGiftag(packets * 2,
				  GSOS_GIF_PRE_ENABLE, GSOS_PRIM_LINE,
				  GSOS_GIF_FLG_PACKED, 1,
				  GSOS_GIF_REG_XYZ2);
	  if(result) break;
	  for(i = 0; i < packets; i++, p++) {
            gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(p->x1 + xorg), 
			   GSOS_SUBPIX_OFST(p->y1 + yorg));
	    
            gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(p->x2 + xorg), 
			   GSOS_SUBPIX_OFST(p->y2 + yorg));
	  }
	  nCount -= packets;
	}
        pbox++;
    }
    gsosExec() ;

}

void gsxPolySegmentGxcpy(dst,pGC,n,pSegments)
    DrawablePtr dst;
    GCPtr   pGC;
    int     n;
    xSegment    *pSegments;
{
    doPolySegment( dst, pGC, n, pSegments ) ;
}
