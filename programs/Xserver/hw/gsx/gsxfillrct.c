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

void gsxPolyFillRectSoft(dst,pGC,n,pRect)
    DrawablePtr dst;
    GCPtr   pGC;
    int     n;
    xRectangle  *pRect;
{
    gsxGCPtr pPriv = gsxGetGCPriv(pGC) ;
    gsxScreenPtr pScreenPriv;
    xRectangle aRect ;
    PixmapPtr dstLop ;
    int nn ;
    GCPtr pGC1;
    int lineWidth;
    int drawRegion_x;
    int drawRegion_y;

    if(!n) return ;

    /* get region for LOP */
    gsxCalcDrawRegionRect( pRect, n, &aRect ) ;
    drawRegion_x = aRect.x;
    drawRegion_y = aRect.y;
    lineWidth = pGC->lineWidth;
    lineWidth = (((lineWidth < 2) ? 2 : lineWidth) + 1);
    aRect.x += dst->x;
    aRect.y += dst->y;
    aRect.width  += lineWidth;
    aRect.height += lineWidth;

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
      gcval[0].val = pGC1->patOrg.x + (pPriv->patW - drawRegion_x % pPriv->patW);
      gcval[1].val = pGC1->patOrg.y + (pPriv->patH - drawRegion_y % pPriv->patH);
      DoChangeGC (pGC1, GCTileStipXOrigin|GCTileStipYOrigin, 
      			(XID *)gcval, TRUE);
    }
    GSXVALIDATEGC( (DrawablePtr)dstLop, pGC1 ) ;

    /* draw Rectangles */
    for( nn = 0 ; nn < n ; nn++ ) {
        pRect[nn].x -= drawRegion_x;
        pRect[nn].y -= drawRegion_y;
    }
    (*pGC1->ops->PolyFillRect)( (DrawablePtr)dstLop, pGC1, n, pRect ) ;

    FreeScratchGC(pGC1);

    /* ValidateGC( destination = WINDOW ) */
    GSXVALIDATEGC( dst, pGC ) ;

    /* post-proccess for LOP */
    cfbBitBlt( (DrawablePtr)dstLop, dst, pGC, 0, 0, aRect.width, aRect.height,
        drawRegion_x, drawRegion_y, gsxDoBitBlt, 0 ) ;

    (*dst->pScreen->DestroyPixmap)(dstLop) ;
}

#if GSX_CACHED_TILE

void gsxPolyFillRectCached(
    DrawablePtr dst,
    GCPtr   pGC,
    int     n,
    xRectangle  *pRect)
{
    gsxGCPtr	pPriv = gsxGetGCPriv(pGC);
    xRectangle	*p;
    BoxPtr      pbox  = REGION_RECTS(pPriv->pCompositeClip);
    BoxPtr      plast = pbox + REGION_NUM_RECTS(pPriv->pCompositeClip);
    gsxCachedTilePtr	pCT;
    WindowPtr		pWin;
    gsxWindowPtr	pPrivWin;
    GSOSbit64	tex, test_1;
    int		tex_orgx, tex_orgy;
    int		tex_w, tex_h;
    int xorg = dst->x, yorg = dst->y;
    gsxScreenPtr pScreenPriv;

    pWin = (WindowPtr)dst;
    pPrivWin = gsxGetWindowPriv(pWin);
    pCT = pPriv->pCachedTile;


    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    gsxSetColormask(pScreenPriv, pPriv->fbmask);
    gsosMakeGiftag(5,
    	GSOS_GIF_PRE_IGNORE, 0, 
	GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);

    /* set SCISSOR rect */
    gsosSetPacketAddrData4(GSOS_SCISSOR_1,
	      (GSOSbit64)0, (GSOSbit64)pWin->drawable.pScreen->width-1,
	      (GSOSbit64)0, (GSOSbit64)pWin->drawable.pScreen->height-1);
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



    /*** drawing ***/
    tex_orgx = pWin->drawable.x + pGC->patOrg.x;
    tex_orgy = pWin->drawable.y + pGC->patOrg.y;
    tex_w = pCT->texinfo.fake_w ? pCT->texinfo.fake_w : GSX_TEXTURE_FORWARD_W ;
    tex_h = pCT->texinfo.fake_h ? pCT->texinfo.fake_h : GSX_TEXTURE_FORWARD_H ;

#ifdef DEBUG
    ErrorF("tbp:0x%x tbw:0x%x psm:0x%x tw:%d th:%d [ref:%d]\n", 
	pCT->texinfo.tbp, pCT->texinfo.tbw, pCT->texinfo.psm,
	pCT->texinfo.log2tw, pCT->texinfo.log2th, pCT->refcnt);
    if (pCT->texinfo.fake_w) 
        ErrorF("pCT->texinfo.fake_w:%d\n", pCT->texinfo.fake_w);
    if (pCT->texinfo.fake_h) 
        ErrorF("pCT->texinfo.fake_h:%d\n", pCT->texinfo.fake_h);
#endif 

    while (pbox < plast) {
        int x1,y1;
        int x2,y2;
        int x,y;
        int tx,ty;
        int dtx,dty;
	int w=0;
	int h=0;
	int nCount;

	if ((pbox->x1>pbox->x2) || (pbox->y1>pbox->y2)) {
	    ErrorF("Unexpected x1:y1 x2:y2\n", 
		pbox->x1, pbox->y1, pbox->x2, pbox->y2);
	}

        gsosMakeGiftag(1, 
		GSOS_GIF_PRE_IGNORE, 0, 
		GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);
        /* set SCISSOR rect */
        gsosSetPacketAddrData4( GSOS_SCISSOR_1,
            (GSOSbit64)pbox->x1, (GSOSbit64)pbox->x2-1,
            (GSOSbit64)pbox->y1, (GSOSbit64)pbox->y2-1 );

	p = pRect;
	for (nCount=n,p=pRect; nCount >0; nCount--, p++) {

	    x1 = p->x + xorg; y1 = p->y + yorg;
	    x2 = x1 + p->width; y2 = y1 + p->height;

	    dtx=(x1 - tex_orgx) % tex_w;
	    dty=(y1 - tex_orgy) % tex_h;

            for (tx=dtx, x=x1; x<x2; tx=0,x+=w)  {
	        w = min(x2-x, tex_w-tx);

	        for (ty=dty, y=y1; y<y2; ty=0,y+=h)  {
	            h = min(y2-y, tex_h-ty);

                    gsosMakeGiftag(2,
                      GSOS_GIF_PRE_ENABLE, 
	  	        (GSOS_PRIM_SPRITE|GSOS_GIF_PRI_TME|GSOS_GIF_PRI_FST),
                      GSOS_GIF_FLG_PACKED, 2,
	              GSOS_GIF_REG_XYZ2<<4|GSOS_GIF_REG_UV);
        	    /* UV */
                    gsosSetXyPackedXYZ2(GSOS_SUBTEX_OFST(tx),
			GSOS_SUBTEX_OFST(ty));
	            /* XY */
                    gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(x),
		       GSOS_SUBPIX_OFST(y));
	            /* UV */
                    gsosSetXyPackedXYZ2(GSOS_SUBTEX_OFST(tx+w),
			GSOS_SUBTEX_OFST(ty+h));
        	    /* XY */
                    gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(x+w),
		       GSOS_SUBPIX_OFST(y+h));
                }
            }
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

#endif /* GSX_CACHED_TILE */

static inline void doPolyFillRect(dst,pGC,n,pRect)
    DrawablePtr dst;
    GCPtr   pGC;
    int     n;
    xRectangle  *pRect;
{
    gsxGCPtr        pPriv = gsxGetGCPriv(pGC);
    BoxPtr      pbox  = REGION_RECTS(pPriv->pCompositeClip);
    BoxPtr      plast = pbox + REGION_NUM_RECTS(pPriv->pCompositeClip);
    int xorg = dst->x, yorg = dst->y;
    xRectangle *p = pRect;
    int rgb, r, g, b, a ;
    int i;
    int nCount;
    int pc;
    int packets;
    int result;
    GSOSbit64	rgba;
    gsxScreenPtr pScreenPriv;

    if (!n) return;

    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    gsxSetColormask(pScreenPriv, pPriv->fbmask);
    rgb = pGC->fgPixel & gsxGetScreenPriv(dst->pScreen)->rgb_pix_mask ;
    gsxGetRgba( pGC, rgb, &r, &g, &b, &a ) ;

    /* set RGB */
    gsosMakeGiftag(1, 
		GSOS_GIF_PRE_IGNORE, 0, 
		GSOS_GIF_FLG_PACKED, 1, 
		GSOS_GIF_REG_AD);

    rgba = r | g << 8 |  b << 16 |  a << 24 ;
    gsosSetPacketAddrData(GSOS_RGBAQ, rgba) ;

    while (pbox < plast) {
        gsosMakeGiftag(1, 
		GSOS_GIF_PRE_IGNORE, 0, 
		GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);
        /* set SCISSOR rect */
        gsosSetPacketAddrData4( GSOS_SCISSOR_1,
            (GSOSbit64)pbox->x1, (GSOSbit64)pbox->x2-1,
            (GSOSbit64)pbox->y1, (GSOSbit64)pbox->y2-1 );

	p = pRect;
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
	  		GSOS_GIF_PRE_ENABLE, GSOS_PRIM_SPRITE,
			GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_XYZ2  );
	  if(result) break;
	  for(i = 0; i < packets; i++, p++) {
	    gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(p->x + xorg),
			   GSOS_SUBPIX_OFST(p->y + yorg));

            gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(p->x + p->width  + xorg), 
			GSOS_SUBPIX_OFST(p->y + p->height + yorg));
	  }
	  nCount -= packets;
	}
        pbox++;
    }
    gsosExec() ;

}

void gsxPolyFillRectGxcpy(dst,pGC,n,pRect)
    DrawablePtr dst;
    GCPtr   pGC;
    int     n;
    xRectangle  *pRect;
{

    doPolyFillRect( dst, pGC, n, pRect ) ;
}

