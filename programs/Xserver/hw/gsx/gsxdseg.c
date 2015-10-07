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


#include "gsx.h"

#if GSX_ACCL_DASH

#define DEBUG

static inline
void doPolySegmentDashCT(dst,pGC,n,pSegments)
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
    int i;
    int nCount;
    int pc;
    int packets;
    int result;
    GSOSbit64 tex;
    GSOSbit64 test_1;
    gsxScreenPtr	pScreenPriv;
    gsxCachedTilePtr	pCT;

    if (!n) return;

    /*    endp = (pGC->capStyle==CapNotLast)? GSX_DISABLE:GSX_ENABLE; */

    pScreenPriv = gsxGetScreenPriv(dst->pScreen);
    pCT = pScreenPriv->dashCT;

    gsxSetColormask(pScreenPriv, pPriv->fbmask);
    gsosMakeGiftag(4, 
    		GSOS_GIF_PRE_IGNORE, 0, 
		GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);

    /* set texture */
    gsosSetPacketAddrData(GSOS_CACHEINVLD, 0) ;
    tex = GsosTex0Data( pCT->texinfo.tbp /*tbp0*/,
			pCT->texinfo.tbw /*tbw*/,
			pCT->texinfo.psm /*psm*/,
			pCT->texinfo.log2tw /*tw*/,
			pCT->texinfo.log2th /*th*/,
			1 /*tcc: RGBA */,
			1 /*tfx: DECAL */,
			0 /*cbp*/,
			0 /*cpsm*/,
			0 /*csm*/,
			0 /*csa*/,
			0 /*cld*/);
    gsosSetPacketAddrData(GSOS_TEX0_1, tex) ;
    /* enable pmode on prim */
    gsosSetPacketAddrData(GSOS_PRMODECONT, 1) ;

    /* set alpha test */
    test_1 = GsosTestData( 1,	/*ATE:	ON*/
    			4, 	/*ATST:	EQUAL*/
			0, 	/*AREF: 0*/
			0,	/*AFAIL: KEEP*/
			0,	/*DATE: OFF*/
			0, 	/*DATM: 0*/
			0,	/*ZTE: OFF*/
			0 	/*ZTST:NEVER*/
			) ;
    gsosSetPacketAddrData(GSOS_TEST_1, test_1 );


    pbox  = REGION_RECTS(pPriv->pCompositeClip);
    while (pbox < plast) {
	int tp;

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
	tp = pGC->dashOffset;
	nCount = n;
	while(nCount > 0) {

	  if (nCount == 1) {
	    packets = 1;
	  } else {
	    pc = gsosCalcPacketCount(4, GSOS_GIF_FLG_PACKED)/4;
	    if(pc == 0) {	/* can't allocate buffer */
	      gsosExec();		/* flush DMA */
	      pc = gsosCalcMaxPacketCount(4, GSOS_GIF_FLG_PACKED)/4;
	    }
	    packets = (nCount < pc) ? nCount : pc;
	  }

          result = gsosMakeGiftag(packets * 2,
                      GSOS_GIF_PRE_ENABLE, 
	              (GSOS_PRIM_LINE|GSOS_GIF_PRI_TME|GSOS_GIF_PRI_FST),
                      GSOS_GIF_FLG_PACKED, 2,
	              GSOS_GIF_REG_XYZ2<<4|GSOS_GIF_REG_UV);

	  if(result) break;
	  for(i = 0; i < packets; i++, p++) {
	    int w; 
	    int h;
	    int tx1;
	    int tx2;

	    w = abs(p->x2 - p->x1);
	    h = abs(p->y2 - p->y1);
	    tx1 = tp;
	    tx2 = tx1+max(w,h);

            /* UV */
            gsosSetXyPackedXYZ2(GSOS_SUBTEX_OFST(tx1),
			GSOS_SUBTEX_OFST(0));
            /* XY */
            gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(p->x1 + xorg), 
			   GSOS_SUBPIX_OFST(p->y1 + yorg));

            /* UV */
            gsosSetXyPackedXYZ2(GSOS_SUBTEX_OFST(tx2),
			GSOS_SUBTEX_OFST(0));
            /* XY */
            gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(p->x2 + xorg), 
			   GSOS_SUBPIX_OFST(p->y2 + yorg));

	  }
	  nCount -= packets;
	}
        pbox++;
    }
    gsosMakeGiftag(3,
    	GSOS_GIF_PRE_IGNORE, 0, 
	GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);
    /* disble pmode on prim */
    gsosSetPacketAddrData(GSOS_PRMODECONT, 0) ;
    /* clear pmode */
    gsosSetPacketAddrData(GSOS_PRMODE, 0) ;
    /* clear alpha test */
    test_1 = GsosTestData( 1, 1, 0, 0, 0, 0, 0, 0 ) ;
    gsosSetPacketAddrData(GSOS_TEST_1, test_1 );

    gsosExec() ;

}

void gsxPolySegmentDashCT(dst, pGC, n, pSegments)
    DrawablePtr dst;
    GCPtr   pGC;
    int     n;
    xSegment    *pSegments;
{
    xRectangle	aRect;
    gsxScreenPtr pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    int		dashLen = pScreenPriv->dashLen;

    gsxCalcDrawRegionSeg( pSegments, n, &aRect ) ;
    if ((aRect.width + dashLen)>(GSX_TEXTURE_CORD_MAX_X-1) ||
        (aRect.height + dashLen)>(GSX_TEXTURE_CORD_MAX_Y-1)) {
	    gsxPolySegmentSoft(dst, pGC, n, pSegments);
    	    return;
    }

    gsxCheckDash(pGC, TRUE);
    doPolySegmentDashCT(dst, pGC, n, pSegments) ;
}

#endif /* GSX_ACCL_DASH */
