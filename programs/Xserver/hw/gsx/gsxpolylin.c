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
 * Calculate total area of individual line
 */
static inline 
int CalcDrawPrimitiveRegion(
    DDXPointPtr pPoint,
    int n,
    int margin)
{
    int region = 0;
    int x0,y0,x1,y1;
    int width, height;

    x0 = pPoint[0].x;
    y0 = pPoint[0].y;
    while(--n > 0) {
        x0 = pPoint[0].x;
        y0 = pPoint[0].y;
        x1 = pPoint[1].x;
        y1 = pPoint[1].y;
        if(x1 > x0) {
            width  = x1 - x0;
        } else {
           width  = x0 - x1;
        }
        if(y1 > y0) {
           height = y1 - y0;
        } else {
           height = y0 - y1;
        }
        width  += margin * 2;
        height += margin * 2;
        region += width * height;
        pPoint++;
     }
     return region;
}

static inline 
void gsxPolyLinesSoftWhole(
    DrawablePtr dst,
    GCPtr   pGC, 
    int     n,
    DDXPointPtr pPoint, 
    xRectangle aRect)
{
    gsxGCPtr pPriv = gsxGetGCPriv(pGC) ;
    gsxScreenPtr pScreenPriv;
    /* xRectangle aRect ; */
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
        dstLop) ;

    /* ValidateGC( destination = PIXMAP ) */
    pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
    CopyGC(pGC, pGC1, ~GCClipMask);

    /* adjust pattern origin */
    if (pGC->fillStyle != FillSolid) {
        ChangeGCVal gcval[2];
        gcval[0].val = pGC1->patOrg.x + 
			(pPriv->patW - drawRegion_x % pPriv->patW);
        gcval[1].val = pGC1->patOrg.y + 
			(pPriv->patH - drawRegion_y % pPriv->patH);
        DoChangeGC (pGC1, GCTileStipXOrigin|GCTileStipYOrigin, 
      			(XID *)gcval, TRUE);
    }
    GSXVALIDATEGC( (DrawablePtr)dstLop, pGC1 ) ;

    /* draw lines */
    for( nn = 0 ; nn < n ; nn++ ) {
        pPoint[nn].x -= drawRegion_x;
        pPoint[nn].y -= drawRegion_y;
    }

    (*pGC1->ops->Polylines)( (DrawablePtr)dstLop, pGC1, 
    					CoordModeOrigin, n, pPoint ) ;

    FreeScratchGC(pGC1);

    /* ValidateGC( destination = WINDOW ) */
    GSXVALIDATEGC( dst, pGC ) ;

    /* post-proccess for LOP */
    cfbBitBlt( (DrawablePtr)dstLop, dst, pGC, 0, 0, aRect.width, aRect.height,
        drawRegion_x, drawRegion_y, gsxDoBitBlt, 0 ) ;

    (*dst->pScreen->DestroyPixmap)(dstLop) ;
}

void gsxPolyLinesSoft(
    DrawablePtr dst,
    GCPtr   pGC,
    int     mode,
    int     n,
    DDXPointPtr pPoint)
{
    gsxGCPtr pPriv = gsxGetGCPriv(pGC) ;
    gsxScreenPtr pScreenPriv;
    xRectangle aRect ;
    PixmapPtr dstLop ;
    GCPtr pGC1;
    int margin;
    xPoint point[2];
    int width,height;
    int scrn_x, scrn_y;
    int dr;
    int x0, y0;

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
    margin = gsxGetLineMargin(pGC);
    gsxCalcDrawRegion(pPoint, n, &aRect);
    dr = aRect.width * aRect.height; /*  area of every line */
    if (IS_SINGLE_OP_COVERED(dr,gsxGetBytePerPixel(pScreenPriv)) || 
    	      dr < CalcDrawPrimitiveRegion(pPoint, n, margin)) {
        gsxPolyLinesSoftWhole(dst, pGC, n, pPoint, aRect);
        return;
    }

    pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
    CopyGC(pGC, pGC1, ~GCClipMask);
    while(--n > 0) {
        if(pPoint[1].x > pPoint[0].x) {
            x0 = pPoint[0].x;
            width  = pPoint[1].x - pPoint[0].x;
        } else {
            x0 = pPoint[1].x;
            width  = pPoint[0].x - pPoint[1].x;
        }
        if(pPoint[1].y > pPoint[0].y) {
            y0 = pPoint[0].y;
            height = pPoint[1].y - pPoint[0].y;
        } else {
            y0 = pPoint[1].y;
            height = pPoint[0].y - pPoint[1].y;
        }
        width  += margin * 2;
        height += margin * 2;
        scrn_x = x0 - margin + dst->x;
        scrn_y = y0 - margin + dst->y;

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
	    gcval[0].val = pGC1->patOrg.x + 
	    		(pPriv->patW - (scrn_x - dst->x) % pPriv->patW);
	    gcval[1].val = pGC1->patOrg.y + 
	    		(pPriv->patH - (scrn_y - dst->y) % pPriv->patH);
            DoChangeGC (pGC1, GCTileStipXOrigin|GCTileStipYOrigin, 
      			(XID *)gcval, TRUE);
        }
        /* ValidateGC( destination = PIXMAP ) */
        GSXVALIDATEGC( (DrawablePtr)dstLop, pGC1 ) ;

        /* draw lines */
        point[0].x = pPoint[0].x - x0 + margin;
        point[0].y = pPoint[0].y - y0 + margin;
        point[1].x = pPoint[1].x - x0 + margin;
        point[1].y = pPoint[1].y - y0 + margin;
        (*pGC1->ops->Polylines)((DrawablePtr)dstLop, pGC1, CoordModeOrigin, 2, point);


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




#ifdef __R5900
asm (".p2align 6\n");
#endif

void gsxPolyLinesGxcpy(
    DrawablePtr dst,
    GCPtr   pGC,
    int     mode,
    int     n,
    DDXPointPtr pPoint)
{
    gsxGCPtr    pPriv = gsxGetGCPriv(pGC);
    BoxPtr      pbox  = REGION_RECTS(pPriv->pCompositeClip);
    BoxPtr      plast = pbox + REGION_NUM_RECTS(pPriv->pCompositeClip);
    int xorg = dst->x, yorg = dst->y;
    DDXPointPtr p = pPoint, pLast = pPoint + n;
    int x = p->x, y = p->y ;
    int rgb, r, g, b, a ;
    int nCount;
    int pc;
    int packets;
    int result;
    int morepacket;
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

#define ADDTIONAL_OFF 1024
#define OFFSET_SHIFT 4

    rgb = pGC->fgPixel & gsxGetScreenPriv(dst->pScreen)->rgb_pix_mask;
    gsxGetRgba( pGC, rgb, &r, &g, &b, &a ) ;

    /* set RGB */
    gsosMakeGiftag(2,
    	GSOS_GIF_PRE_IGNORE, 0, 
	GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);

    rgba = r | g << 8 |  b << 16 |  a << 24 ;
    gsosSetPacketAddrData(GSOS_RGBAQ, rgba) ;

    /* set offset */
    gsosSetPacketAddrData(GSOS_XYOFFSET_1,
	(GSOS_SUBPIX_OFST(ADDTIONAL_OFF-xorg) & 0xffff) |
	((unsigned long long) 
		(GSOS_SUBPIX_OFST(ADDTIONAL_OFF-yorg) & 0xffff) << 32));

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

	p = pPoint;
	nCount = n;
	morepacket = 0;
	while(nCount > 0) {

	    if (nCount == 1) {
	      packets = 1;
	    } else {
	      pc = gsosCalcPacketCount(1, GSOS_GIF_FLG_PACKED) - morepacket;
	      if(pc <= 0) {	/* can't allocate buffer */
	        gsosExec();		/* flush DMA */
	        pc = gsosCalcMaxPacketCount(1, GSOS_GIF_FLG_PACKED) 
				- morepacket;
	      }
	      packets = (nCount < pc) ? nCount : pc;
	    }
	    nCount -= packets;
	    result = gsosMakeGiftag(packets + morepacket,
				  GSOS_GIF_PRE_ENABLE, GSOS_PRIM_LSTRIP,
				  GSOS_GIF_FLG_PACKED, 1,
				  GSOS_GIF_REG_XYZ2   );
	    if(result) break;
	    if(morepacket == 1) {
	      gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST((p-1)->x + ADDTIONAL_OFF),
			   GSOS_SUBPIX_OFST((p-1)->y + ADDTIONAL_OFF));
	    }
	    morepacket = 1;

#if defined( __MIPSEL__) && defined(__R5900)
    __asm__ __volatile__(	
        ".set push\n" 
        "       .set    arch=r5900\n"	

	"	sra	$11, %3, 2\n"	// packets
	"	beqz	$11, 2f\n"
	"	dli	$8, %4\n"	// offset
	"	dsll	$8, 32\n"
	"	or	$8, %4\n"	// offset
	"	.p2align 4\n"
	"1:\n"
	"	lw	$9, (%1)\n"	// p
		"	lw	$10, 4(%1)\n"	// p
	"	pextlh	$9, $0, $9\n"
		"	pextlh	$10, $0, $10\n"
	"	paddh	$9, $9, $8\n"
		"	paddh	$10, $10, $8\n"
        "	dsll    $9, %5\n"	// shift
		"	dsll    $10, %5\n"	// shift
        "       pcpyld  $9, $0, $9\n"	
		"       pcpyld  $10, $0, $10\n"	
        "       sq      $9, (%0)\n"	// ps2cur
		"       sq      $10, 16(%0)\n"	// ps2cur

	"	lw	$9, 8(%1)\n"	// p
		"	lw	$10, 12(%1)\n"	// p
	"	pextlh	$9, $0, $9\n"
		"	pextlh	$10, $0, $10\n"
	"	paddh	$9, $9, $8\n"
		"	paddh	$10, $10, $8\n"
        "	dsll    $9, %5\n"	// shift
		"	dsll    $10, %5\n"	// shift
        "       pcpyld  $9, $0, $9\n"	
		"       pcpyld  $10, $0, $10\n"	
        "       sq      $9, 32(%0)\n"	// ps2cur
		"       sq      $10, 48(%0)\n"	// ps2cur

	"	subu	$11, 1\n"
	"	addu	%0, 64\n"	// ps2cur
	"	addu	%1, 16\n"	// p
	"	addu	%2, 8\n"	// ps2count
	"	.p2align 3\n"
	"	bnez	$11, 1b\n"
	"2:\n"
        "       .set    pop"	
        :  "+r"(ps2cur), 
		"+r"(p), 
		"+r"(ps2count)
        :  "r"(packets), 
		"i"(GSOS_XYOFFSET+ADDTIONAL_OFF),
		"i"(OFFSET_SHIFT)
	: "$8","$9","$10","$11");
#else
	    for(i = 0; i < (packets>>2); i ++) {
#ifdef __R5900
	      __asm__ __volatile__ (".p2align 4\n");
#endif
	      gsosSetXyPackedXYZ2_SUBPIX_SHIFT_OFF(
	      		p->x+GSOS_XYOFFSET+ADDTIONAL_OFF, 
			p->y+GSOS_XYOFFSET+ADDTIONAL_OFF, OFFSET_SHIFT, 0);
	      gsosSetXyPackedXYZ2_SUBPIX_SHIFT_OFF(
	      		(p+1)->x+GSOS_XYOFFSET+ADDTIONAL_OFF, 
			(p+1)->y+GSOS_XYOFFSET+ADDTIONAL_OFF, OFFSET_SHIFT, 1);
	      gsosSetXyPackedXYZ2_SUBPIX_SHIFT_OFF(
	      		(p+2)->x+GSOS_XYOFFSET+ADDTIONAL_OFF, 
			(p+2)->y+GSOS_XYOFFSET+ADDTIONAL_OFF, OFFSET_SHIFT, 2);
	      gsosSetXyPackedXYZ2_SUBPIX_SHIFT_OFF(
	      		(p+3)->x+GSOS_XYOFFSET+ADDTIONAL_OFF, 
			(p+3)->y+GSOS_XYOFFSET+ADDTIONAL_OFF, OFFSET_SHIFT, 3);
	      gosForwardPacket(4);
	      p+=4;
	    }
#endif

	    if (packets&3)
	      switch(packets&3) {
                case 3:
	          gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(p->x + ADDTIONAL_OFF),
			   GSOS_SUBPIX_OFST(p->y + ADDTIONAL_OFF));
	          p++;
		  /* fall through */
                case 2:
	          gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(p->x + ADDTIONAL_OFF),
			   GSOS_SUBPIX_OFST(p->y + ADDTIONAL_OFF));
	          p++;
		  /* fall through */
                case 1:
	          gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(p->x + ADDTIONAL_OFF),
			   GSOS_SUBPIX_OFST(p->y + ADDTIONAL_OFF));
	          p++;
		  /* fall through */
	      }
        }
        pbox++;
    }

    // recover offset
    gsosMakeGiftag(1,
    	GSOS_GIF_PRE_IGNORE, 0, 
	GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);
    gsosSetPacketAddrData(GSOS_XYOFFSET_1,
                          GsosXyoffsetData(GSOS_XYOFFSET<<4,GSOS_XYOFFSET<<4));
    gsosExec() ;
}
