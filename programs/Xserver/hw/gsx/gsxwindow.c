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

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "mistruct.h"
#include "regionstr.h"
#include "cfbmskbits.h"
#include "pixmap.h"

#include "gsx.h"
#include "common/xf86.h"

extern WindowPtr *WindowTable;

Bool gsxCreateWindow(pWin)
    WindowPtr pWin;
{
    gsxWindowPtr pPrivWin;

    pPrivWin = gsxGetWindowPriv(pWin);
    pPrivWin->pRotatedBorder = NullPixmap;
    pPrivWin->pRotatedBackground = NullPixmap;
    pPrivWin->fastBackground = FALSE;
    pPrivWin->fastBorder = FALSE;
#if GSX_CACHED_TILE
    pPrivWin->pCachedBackground = NULL;
    pPrivWin->pCachedBorder = NULL;
#endif

    return TRUE;
}

Bool gsxDestroyWindow(pWin)
    WindowPtr pWin;
{
    gsxWindowPtr pPrivWin;

    pPrivWin = gsxGetWindowPriv(pWin);

    if (pPrivWin->pRotatedBorder)
	cfbDestroyPixmap(pPrivWin->pRotatedBorder);
    if (pPrivWin->pRotatedBackground)
	cfbDestroyPixmap(pPrivWin->pRotatedBackground);

#if GSX_CACHED_TILE
    if (pPrivWin->pCachedBackground) {
        gsxDestroyCT(pWin->drawable.pScreen, pPrivWin->pCachedBackground);
        pPrivWin->pCachedBackground = NULL;
    }
    if (pPrivWin->pCachedBorder) {
        gsxDestroyCT(pWin->drawable.pScreen, pPrivWin->pCachedBorder);
        pPrivWin->pCachedBorder = NULL;
    }
#endif

    return(TRUE);
}

/*ARGSUSED*/
Bool
gsxMapWindow(pWindow)
    WindowPtr pWindow;
{
    return(TRUE);
}

/* (x, y) is the upper left corner of the window on the screen 
   do we really need to pass this?  (is it a;ready in pWin->absCorner?)
   we only do the rotation for pixmaps that are 32 bits wide
   (padded or otherwise.)
   cfbChangeWindowAttributes() has already put a copy of the pixmap
   in pPrivWin->pRotated*
*/
/*ARGSUSED*/
Bool gsxPositionWindow(pWin, x, y)
    WindowPtr pWin;
    int x, y;
{
    return (TRUE);
}

/*ARGSUSED*/
Bool gsxUnmapWindow(pWindow)
    WindowPtr pWindow;
{
    return (TRUE);
}

Bool
gsxChangeWindowAttributes(pWin, mask)
    WindowPtr pWin;
    unsigned long mask;
{
#if GSX_CACHED_TILE
    unsigned long index;
    gsxWindowPtr pPrivWin;
    int width;
    WindowPtr pBgWin;

    pPrivWin = gsxGetWindowPriv(pWin);
    while (mask) {
	index = lowbit(mask);
	mask &= ~index;
	switch (index) {
	case CWBackPixmap:
	    if (pPrivWin->pCachedBackground) {
		gsxDestroyCT(pWin->drawable.pScreen, 
				pPrivWin->pCachedBackground);
		pPrivWin->pCachedBackground = NULL;
	    }

	    if (pWin->backgroundState == None) {
		pPrivWin->fastBackground = FALSE;
	    } else if (pWin->backgroundState == ParentRelative) {
		pPrivWin->fastBackground = FALSE;
	    } else if (width = pWin->background.pixmap->drawable.width,
		    (width <= TILE_CACHE_WIDTH) && !(width & (width - 1))) {

		cfbCopyRotatePixmap (pWin->background.pixmap,
				     &pPrivWin->pRotatedBackground,
				     0, 0);
		if (pPrivWin->pRotatedBackground) {
		    pPrivWin->fastBackground = TRUE;
		    pPrivWin->pCachedBackground = 
		    	gsxGetCT(pWin->drawable.pScreen, NULL,
					pPrivWin->pRotatedBackground);
		} else {
		    pPrivWin->fastBackground = FALSE;
		    pPrivWin->pCachedBackground = 
		    	gsxGetCT(pWin->drawable.pScreen, NULL, 
					pWin->background.pixmap);
		}
	    } else {
		pPrivWin->fastBackground = FALSE;
		pPrivWin->pCachedBackground = 
			gsxGetCT(pWin->drawable.pScreen, NULL,
					pWin->background.pixmap);
	    }
	    break;

	case CWBackPixel:
	    if (pPrivWin->pCachedBackground) {
		gsxDestroyCT(pWin->drawable.pScreen, 
				pPrivWin->pCachedBackground);
		pPrivWin->pCachedBackground = NULL;
	    }
	    pPrivWin->fastBackground = FALSE;
	    break;

	case CWBorderPixmap:
	    if (pPrivWin->pCachedBorder) {
		gsxDestroyCT(pWin->drawable.pScreen, 
				pPrivWin->pCachedBorder);
		pPrivWin->pCachedBorder = NULL;
	    }
	    if (((width = pWin->border.pixmap->drawable.width) <= TILE_CACHE_WIDTH) &&
		!(width & (width - 1))) {
		for (pBgWin = pWin;
		     pBgWin->backgroundState == ParentRelative;
		     pBgWin = pBgWin->parent);
		cfbCopyRotatePixmap(pWin->border.pixmap,
				      &pPrivWin->pRotatedBorder,
				      0, 0);
		if (pPrivWin->pRotatedBorder) {
		    pPrivWin->fastBorder = TRUE;
		    pPrivWin->pCachedBorder = 
		    	gsxGetCT(pWin->drawable.pScreen, NULL,
					pPrivWin->pRotatedBorder);
		} else {
		    pPrivWin->fastBorder = FALSE;
		    pPrivWin->pCachedBorder = 
		    	gsxGetCT(pWin->drawable.pScreen, NULL,
					pWin->border.pixmap);
		}
	    } else {
		pPrivWin->fastBorder = FALSE;
		pPrivWin->pCachedBorder = 
			gsxGetCT(pWin->drawable.pScreen, NULL,
					pWin->border.pixmap);
	    }
	    break;
	case CWBorderPixel:
	    if (pPrivWin->pCachedBorder) {
		gsxDestroyCT(pWin->drawable.pScreen, 
				pPrivWin->pCachedBorder);
		pPrivWin->pCachedBorder = NULL;
	    }
	    pPrivWin->fastBorder = FALSE;
	}
    }
#endif /* GSX_CACHED_TILE */
    return (TRUE);
}

void
gsxPaintWindowSolid(pWin, pRegion, what, pixel)
    WindowPtr   pWin;
    RegionPtr   pRegion;
    int         what;
    Pixel       pixel;
{
    int          nbox = REGION_NUM_RECTS(pRegion);
    BoxPtr       pbox = REGION_RECTS(pRegion);
    int          r,g,b,a;
    unsigned int rgb = pixel;

    int		packets;
    int		pc;
    int		result;
    int		i;
    GSOSbit64 rgba;
    gsxScreenPtr        pScreenPriv;

    pScreenPriv = gsxGetScreenPriv(pWin->drawable.pScreen);

    /*** color of fill ***/
    gsxGetRgbaWithVisual( pScreenPriv->pVisual, rgb,&r,&g,&b,&a);

    /* set SCISSOR rect */
    gsosMakeGiftag(2,
    	GSOS_GIF_PRE_IGNORE, 0, 
	GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);
    gsosSetPacketAddrData4(GSOS_SCISSOR_1,
	      (GSOSbit64)0, (GSOSbit64)pWin->drawable.pScreen->width-1,
	      (GSOSbit64)0, (GSOSbit64)pWin->drawable.pScreen->height-1);

    /* set RGB */
    rgba = r | g << 8 |  b << 16 |  a << 24 ;
    gsosSetPacketAddrData(GSOS_RGBAQ, rgba) ;

    // clear fbmask
    if ((pScreenPriv)->cur_fbmask)
        gsosFrameInit((pScreenPriv)->fbw,
            (pScreenPriv)->psm,
            (pScreenPriv)->fbp,
            0 /* enable all planes */);

    /*** drawing ***/
    while(nbox>0) {

      if (nbox == 1)  {
        packets = 1;
      } else {
        pc = gsosCalcPacketCount(2, GSOS_GIF_FLG_PACKED)/2;
        if (pc==0) {
          gsosExec();
	  pc = gsosCalcMaxPacketCount(2, GSOS_GIF_FLG_PACKED)/2;
        }
        packets = (nbox < pc) ? nbox : pc;
      }
      result = gsosMakeGiftag(packets * 2,
          GSOS_GIF_PRE_ENABLE, GSOS_PRIM_SPRITE,
          GSOS_GIF_FLG_PACKED, 1,
	  GSOS_GIF_REG_XYZ2);

      for (i=0; i<packets; i++, pbox++) {

        gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(pbox->x1),
		       GSOS_SUBPIX_OFST(pbox->y1));

        gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(pbox->x2),
		       GSOS_SUBPIX_OFST(pbox->y2));
      }
      nbox -= packets;
    }

    // restore fbmask
    if ((pScreenPriv)->cur_fbmask)
        gsosFrameInit((pScreenPriv)->fbw,
            (pScreenPriv)->psm,
            (pScreenPriv)->fbp,
            (pScreenPriv)->cur_fbmask);

    gsosExec() ;
}

void
gsxPaintWindowTiled(pWin, pRegion, what, pTile)
    WindowPtr   pWin;
    RegionPtr   pRegion;
    int         what;
    PixmapPtr   pTile;
{
    int        n;
    BoxPtr     pbox;
    xRectangle aRect ;
    PixmapPtr dstLop ;
    int nn ;
    GCPtr pGC;
    GCPtr pGC1;
    ChangeGCVal v[2];
    DrawablePtr dst;
    xRectangle rct;
    int bp;
    int bw;
    int psm;
    gsxScreenPtr pScreenPriv;

    n = REGION_NUM_RECTS(pRegion);
    if(!n) return ;

    dst = &(pWin->drawable);
    pScreenPriv = gsxGetScreenPriv(dst->pScreen);
    pGC = GetScratchGC(dst->depth, dst->pScreen);

    bp = pScreenPriv->fbp;
    bw = pScreenPriv->fbw;
    psm = pScreenPriv->psm;

    pbox  = REGION_RECTS(pRegion);

    pGC1 = GetScratchGC(dst->depth, dst->pScreen);
    v[0].val = FillTiled;
    v[1].ptr = pTile;
    DoChangeGC(pGC1, GCFillStyle|GCTile, (XID*)v, TRUE);

    for(nn = 0; nn < n; nn++, pbox++) {
      aRect.x = pbox->x1;
      aRect.y = pbox->y1;
      aRect.width  = pbox->x2 - pbox->x1;
      aRect.height = pbox->y2 - pbox->y1;

      dstLop = (*dst->pScreen->CreatePixmap)
        (dst->pScreen, aRect.width, aRect.height, dst->depth ) ;
      if( !dstLop ){
        return ;
      }


      /* draw Rectangles */
      /* adjust screen to pixmap */
      rct.x = 0;
      rct.y = 0;
      rct.width  = aRect.width;
      rct.height = aRect.height;

      /* ValidateGC( destination = PIXMAP ) */
      v[0].val = -(aRect.x - dst->x);
      v[1].val = -(aRect.y - dst->y);
      DoChangeGC(pGC1, GCTileStipXOrigin|GCTileStipYOrigin, (XID*)v, TRUE);
      GSXVALIDATEGC((DrawablePtr)dstLop, pGC1);

      (*pGC1->ops->PolyFillRect)((DrawablePtr)dstLop, pGC1, 1, &rct);


      /* ValidateGC( destination = WINDOW ) */
      GSXVALIDATEGC(dst, pGC);

      {
	size_t size;
	int nlines;
	int maxlines;
	int txfline;
	int linebytewidth;
	int wqc;
	GSXuchar *pPix;
	int y;
	int i, devKind;
	struct ps2_image img;
	struct ps2_image *imgp = &img;;

	gsosKick();
	pPix = (GSXuchar *)dstLop->devPrivate.ptr;
	devKind = dstLop->devKind;

/*#ifdef GSOS_SPR_IMAGEBUFFER
	imgp->ptr = ps2dbp_spr[ps2dbf_spr];
#else*/
	imgp->ptr = ps2dbp[ps2dbf];
//#endif
	imgp->fbp = bp;
	imgp->fbw = bw;
	imgp->psm = psm;
	imgp->x = aRect.x;
	imgp->w = aRect.width;
	nlines = aRect.height;
	y = aRect.y;
	linebytewidth = gsosBytePerPixel(psm) * aRect.width;
/*#ifdef GSOS_SPR_IMAGEBUFFER
	maxlines = gsos_maximage_size / linebytewidth;
#else*/
	maxlines = gsos_maxpacket_size / linebytewidth;
//#endif

	while(nlines > 0) {
	  txfline = (maxlines < nlines) ? maxlines : nlines;
	  if(linebytewidth == devKind) {
	    size = txfline * linebytewidth;
	    memcpy((void *)imgp->ptr, (void *)pPix, size);
	    pPix += size;
	  } else {
	    GSXuchar *dd = (GSXuchar*)imgp->ptr;
	    for(i = 0; i < txfline; i++) {
	      memcpy((void *)dd, (void *)pPix, linebytewidth);
	      pPix += devKind;
	      dd += linebytewidth;
	    }
	  }

	  imgp->y = y ;
	  imgp->h = txfline ;

	  y += txfline;
	  nlines -= txfline;

#ifdef GSOS_LAZY_EXEC
	  wqc = 2;
#else
	  wqc = (nlines > 0) ? 2 : 1;
#endif

	  if(ioctl(ps2gsfd, PS2IOC_LOADIMAGEA, &img) < 0)
	    return;
#ifdef GSOS_LAZY_EXEC
	  if(!ps2dma_limit)
#else
	  if(!ps2dma_limit || wqc != 2)
#endif
	    ioctl( ps2gsfd, PS2IOC_SENDQCT, wqc);

/*#ifdef GSOS_SPR_IMAGEBUFFER
	  ps2dbf_spr = 1 - ps2dbf_spr;
	  imgp->ptr = ps2dbp_spr[ps2dbf_spr];
#else*/
	  ps2dbf = 1 - ps2dbf;
	  ps2cur = ps2dbp[ps2dbf]; 
	  imgp->ptr = ps2dbp[ps2dbf];
//#endif

	}
      }

      (*dst->pScreen->DestroyPixmap)(dstLop);
    }
    FreeScratchGC(pGC1);
    FreeScratchGC(pGC);
}


#if GSX_CACHED_TILE

void
gsxPaintWindowCachedTiled(pWin, pRegion, what)
    WindowPtr   pWin;
    RegionPtr   pRegion;
    int         what;
{
    int		nbox = REGION_NUM_RECTS(pRegion);
    BoxPtr	pbox = REGION_RECTS(pRegion);
    int		result;
    gsxCachedTilePtr	pCT;
    gsxWindowPtr pPrivWin;
    GSOSbit64	tex;
    int		tex_orgx, tex_orgy;
    int		tex_w, tex_h;
    gsxScreenPtr pScreenPriv;

    pScreenPriv = gsxGetScreenPriv(pWin->drawable.pScreen);

    // clear fbmask
    if ((pScreenPriv)->cur_fbmask)
        gsosFrameInit((pScreenPriv)->fbw,
            (pScreenPriv)->psm,
            (pScreenPriv)->fbp,
            0 /* enable all planes */);

    pPrivWin = gsxGetWindowPriv(pWin);
    if (what ==  PW_BACKGROUND)
      pCT = pPrivWin->pCachedBackground;
    else
      pCT = pPrivWin->pCachedBorder;

    gsosMakeGiftag(4,
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
			0 /*tcc*/,
			1 /*tfx*/,
			0 /*cbp*/,
			0 /*cpsm*/,
			0 /*csm*/,
			0 /*csa*/,
			0 /*cld*/);
    gsosSetPacketAddrData(GSOS_TEX0_1, tex) ;
    /* enable pmode on prim */
    gsosSetPacketAddrData(GSOS_PRMODECONT, 1) ;


    /*** drawing ***/
    tex_orgx = pWin->drawable.x;
    tex_orgy = pWin->drawable.y;
    tex_w = pCT->texinfo.fake_w ? pCT->texinfo.fake_w : GSX_TEXTURE_FORWARD_W ;
    tex_h = pCT->texinfo.fake_h ? pCT->texinfo.fake_h : GSX_TEXTURE_FORWARD_H ;

#ifdef DEBUG
    ErrorF("tbp:0x%x tbw:0x%x psm:0x%x tw:%d th:%d\n", 
	pCT->texinfo.tbp, pCT->texinfo.tbw, pCT->texinfo.psm,
	pCT->texinfo.log2tw, pCT->texinfo.log2th);

    if (pCT->texinfo.fake_w) 
        ErrorF("pCT->texinfo.fake_w:%d\n", pCT->texinfo.fake_w);
    if (pCT->texinfo.fake_h) 
        ErrorF("pCT->texinfo.fake_h:%d\n", pCT->texinfo.fake_h);
#endif 

    while(nbox>0) {
        int x2,y2;
        int x,y;
        int tx,ty;
        int dtx,dty;
	int w=0;
	int h=0;

	if ((pbox->x1>pbox->x2) || (pbox->y1>pbox->y2)) {
	    ErrorF("Unexpected x1:y1 x2:y2\n", 
		pbox->x1, pbox->y1, pbox->x2, pbox->y2);
	}

	x2 = pbox->x2;
	y2 = pbox->y2;
	dtx=(pbox->x1 - tex_orgx) % tex_w;
	dty=(pbox->y1 - tex_orgy) % tex_h;

        for (tx=dtx, x=pbox->x1; x<x2; tx=0,x+=w)  {
	    w = min(x2-x, tex_w-tx);

	    for (ty=dty, y=pbox->y1; y<y2; ty=0,y+=h)  {
	        h = min(y2-y, tex_h-ty);

                result = gsosMakeGiftag(2,
                  GSOS_GIF_PRE_ENABLE, 
	  	    (GSOS_PRIM_SPRITE|GSOS_GIF_PRI_TME|GSOS_GIF_PRI_FST),
                  GSOS_GIF_FLG_PACKED, 2,
	          GSOS_GIF_REG_XYZ2<<4|GSOS_GIF_REG_UV);

	        if (result<0) break;

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
	pbox++;
        nbox--;
    }
    gsosMakeGiftag(2,
    	GSOS_GIF_PRE_IGNORE, 0, 
	GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);
    /* disble pmode on prim */
    gsosSetPacketAddrData(GSOS_PRMODECONT, 0) ;
    /* clear pmode */
    gsosSetPacketAddrData(GSOS_PRMODE, 0) ;

    // restore fbmask
    if ((pScreenPriv)->cur_fbmask)
        gsosFrameInit((pScreenPriv)->fbw,
            (pScreenPriv)->psm,
            (pScreenPriv)->fbp,
            (pScreenPriv)->cur_fbmask);

    gsosExec() ;
}

#endif /* GSX_CACHED_TILE */

void
gsxPaintWindow(pWin, pRegion, what)
    WindowPtr   pWin;
    RegionPtr   pRegion;
    int         what;
{
    gsxWindowPtr pPrivWin;

    if (!xf86VTSema) {
#ifdef DEBUG
	ErrorF("gsxPaintWindow: screen has been virtualized\n");
#endif
	cfbPaintWindow(pWin, pRegion, what);
	return;
    }

    pPrivWin = gsxGetWindowPriv(pWin);
    switch(what) {
    case PW_BACKGROUND:
        while (pWin->backgroundState == ParentRelative)
            pWin = pWin->parent;

        switch (pWin->backgroundState) {
        case None:
            break;

        case BackgroundPixmap:
#if GSX_CACHED_TILE
	    if (pPrivWin->pCachedBackground) 
                gsxPaintWindowCachedTiled(pWin,pRegion,what);
	    else
#endif
                gsxPaintWindowTiled(pWin,pRegion,what,
                                pWin->background.pixmap);
            break;

        case BackgroundPixel:
            gsxPaintWindowSolid(pWin,pRegion,what,
                                pWin->background.pixel);
            break;
        }
	break;
    case PW_BORDER:
        if (pWin->borderIsPixel) {
           gsxPaintWindowSolid(pWin,pRegion,what,
                                pWin->border.pixel);
        } else {
#if GSX_CACHED_TILE
	    if (pPrivWin->pCachedBackground) 
                gsxPaintWindowCachedTiled(pWin,pRegion,what);
	    else
#endif
                gsxPaintWindowTiled(pWin,pRegion,what,
                                pWin->border.pixmap);
        }
	break;
    }
}


static inline void gsxCopyBuffer(pDraw, prgnDst, dx, dy)
    DrawablePtr pDraw;
    RegionPtr   prgnDst;
    int         dx, dy;
{
  register int        nbox = REGION_NUM_RECTS(prgnDst);
  register BoxPtr     pbox = REGION_RECTS(prgnDst);
  BoxPtr              pboxNew1 = NULL, pboxNew2 = NULL;
  BoxPtr              pboxTmp, pboxNext, pboxBase;
  int                 w,h;
  gsxScreenPtr        pScreenPriv;
  int                 bp;
  int                 bw;
  int                 psm;
  int                 result;
  int                 screen_width, screen_height;

  if (!nbox)
    return;

  pScreenPriv = gsxGetScreenPriv(pDraw->pScreen);
  bp = pScreenPriv->fbp;
  bw = pScreenPriv->fbw;
  psm =pScreenPriv->psm;

  if (dy < 0) {
    /* walk source botttom to top */
    /* keep ordering in each band, reverse order of bands */
    pboxNew1 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
    if(!pboxNew1)
      return;
    pboxBase = pboxNext = pbox+nbox-1;
    while (pboxBase >= pbox) {
      while ((pboxNext >= pbox) &&
	     (pboxBase->y1 == pboxNext->y1))
	pboxNext--;
      pboxTmp = pboxNext+1;
      while (pboxTmp <= pboxBase) *pboxNew1++ = *pboxTmp++;
      pboxBase = pboxNext;
    }
    pboxNew1 -= nbox;
    pbox = pboxNew1;
  }

  if (dx < 0) {
    /* walk source right to left */
    /* reverse order of rects in each band */
    pboxNew2 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
    if(!pboxNew2) {
      if (pboxNew1) DEALLOCATE_LOCAL(pboxNew1);
      return;
    }
    pboxBase = pboxNext = pbox;
    while (pboxBase < pbox+nbox) {
      while ((pboxNext < pbox+nbox) &&
	     (pboxNext->y1 == pboxBase->y1))
	pboxNext++;
      pboxTmp = pboxNext;
      while (pboxTmp != pboxBase) *pboxNew2++ = *--pboxTmp;
      pboxBase = pboxNext;
    }
    pboxNew2 -= nbox;
    pbox = pboxNew2;
  }

  screen_width  = pDraw->pScreen->width;
  screen_height = pDraw->pScreen->height;

  while (nbox > 0) {

    int pc;
    int i;
    int packets;


    if (nbox==1) {
      packets = 1;
    } else {
      pc = gsosCalcPacketCount(4, GSOS_GIF_FLG_PACKED)/4;
      if (pc==0) {
        gsosExec();
        pc = gsosCalcMaxPacketCount(4, GSOS_GIF_FLG_PACKED)/4;
      }
      packets = pc < nbox ? pc : nbox;
    }
    result = gsosMakeGiftag(4*packets,
		   GSOS_GIF_PRE_IGNORE, 0,
		   GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);

    if (result) break;

    for (i=0; i<packets; i++) {

      int srcx, srcy, dstx, dsty;
      unsigned int sr0;

      srcx = pbox->x1 + dx;
      srcy = pbox->y1 + dy;
      dstx = pbox->x1;
      dsty = pbox->y1;
      w    = pbox->x2 - dstx;
      h    = pbox->y2 - dsty;

      sr0 = bw | (psm << (24-16));
      gsosSetPacketAddrData4(GSOS_BITBLTBUF, bp, sr0, bp, sr0);

      sr0 = dsty;
      if(srcy > dsty || (srcy == dsty && srcx > dstx))
           ;
      else
          sr0 |= (3 << (59-48));
      gsosSetPacketAddrData4(GSOS_TRXPOS, 
          srcx,
          srcy,
          dstx,
          sr0 );
	
      gsosSetPacketAddrData4(GSOS_TRXREG, w, 0, h, 0);

      gsosSetPacketAddrData(GSOS_TRXDIR, 2);

      pbox++;
    }
    nbox -= packets;
  }
  gsosExec();

  /* free up stuff */
  if (pboxNew2) DEALLOCATE_LOCAL(pboxNew2);
  if (pboxNew1) DEALLOCATE_LOCAL(pboxNew1);
}


void gsxCopyWindow(pWin, ptOldOrg, prgnSrc)
    WindowPtr   pWin;
    DDXPointRec ptOldOrg;
    RegionPtr   prgnSrc;
{
    ScreenPtr   pScreen = pWin->drawable.pScreen;
    int         dx = ptOldOrg.x - pWin->drawable.x;
    int         dy = ptOldOrg.y - pWin->drawable.y;
    RegionRec   rgnDst;

    (*pScreen->RegionInit)(&rgnDst, NullBox, 0);
    (*pScreen->TranslateRegion)(prgnSrc, -dx, -dy);
    (*pScreen->Intersect)(&rgnDst, &pWin->borderClip, prgnSrc);
    gsxCopyBuffer(&pWin->drawable, &rgnDst, dx, dy);
    (*pScreen->RegionUninit)(&rgnDst);
}

