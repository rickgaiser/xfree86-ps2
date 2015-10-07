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
#include "mi.h"

static inline void gsxDoBitbltWW();
static inline void gsxDoBitbltMW();
static inline void gsxDoBitbltWM();
static inline void gsxGetBitmap();
static inline void gsxPutBitmap();

/*
 * BitBlt Window to Window
 */
static inline void
gsxDoBitbltWW(pSrc,pDst,alu,nbox,pbox,pptSrc,planemask,bitPlane)
    DrawablePtr     pSrc, pDst;
    int         alu;
    int         nbox;
    BoxPtr      pbox;
    DDXPointPtr     pptSrc;
    unsigned long   planemask, bitPlane;
{
  int w, h;
  unsigned int bp;
  int sbw, dbw, psm;
  int depth;
  gsxScreenPtr pScreenPriv;

  pScreenPriv = gsxGetScreenPriv(pSrc->pScreen);

  bp = pScreenPriv->fbp;
  sbw = dbw = pScreenPriv->fbw;
  psm = pScreenPriv->psm;
  depth = pDst->depth;

  if( bitPlane == 0 ){    /* ordinary copy area */

    while (nbox > 0) {
      int pc;
      int i;
      int packets;
      int result;

      if (nbox == 1)  {
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
		     GSOS_GIF_PRE_IGNORE, 0, GSOS_GIF_FLG_PACKED, 1,
		     GSOS_GIF_REG_AD);

      if (result) break;

      for (i=0; i<packets; i++) {
        unsigned int sr0, sr1;

        w = pbox->x2 - pbox->x1 ;
        h = pbox->y2 - pbox->y1 ;

	sr0 = sbw | (psm << (24-16));
	sr1 = dbw | (psm << (24-16));
        gsosSetPacketAddrData4(GSOS_BITBLTBUF, bp, sr0, bp, sr1);

        sr0 = pbox->y1;
        if ( pptSrc->y  > pbox->y1 || 
	   (pptSrc->y  == pbox->y1 && pptSrc->x > pbox->x1 ) )
           ;
        else
           sr0 |= (3 << (59-48));
        gsosSetPacketAddrData4(GSOS_TRXPOS, 
		pptSrc->x,
		pptSrc->y,
		pbox->x1,
		sr0 );
	
        gsosSetPacketAddrData4(GSOS_TRXREG, w, 0, h, 0);

        gsosSetPacketAddrData(GSOS_TRXDIR, 2);

        pbox++;
        pptSrc++;
      }
      nbox -= packets;
    }
    gsosExec();

  } else {              /* copy plane */

    if(depth > 1) {
      /* Dst is Pixmap */
      GSXulong  *ulds, *uldd;
      GSXushort *usds, *usdd;
      GSXuchar  *sbuf=0, *dbuf=0;
      int x, y;
      GSXulong fg,bg;
      int bufsize;
      int bytepp;

      fg = pScreenPriv->gsxFg & planemask;
      bg = pScreenPriv->gsxBg & planemask;
      bytepp = gsxGetBytePerPixel(pScreenPriv);

      while (--nbox >= 0) {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	bufsize = w * bytepp * h;
	/* dst */
	dbuf = (GSXuchar *) xrealloc(dbuf, bufsize);
	uldd = (GSXulong *)dbuf;
	usdd = (GSXushort *)uldd;
	/* src */
	sbuf = (GSXuchar *) xrealloc(sbuf, bufsize);
	ulds = (GSXulong*)sbuf;
	usds = (GSXushort *)ulds;


	gsosReadImage(pptSrc->x, pptSrc->y,w,h,bp,dbw,psm,sbuf);
	if(depth <= 16) {	/* RGBA16 */
	  for(y = 0; y < h; y++) {
	    for(x = 0; x < w; x++) {
	      if(usds[x] & bitPlane) {
		usdd[x] = (usdd[x] & ~planemask) | fg;
	      } else {
		usdd[x] = (usdd[x] & ~planemask) | bg;
	      }
	    }
	    usds += w;
	    usdd += w;
	  }
	} else {		/* RGBA32 */
	  for(y = 0 ; y < h; y++) {
	    for(x = 0; x < w; x++) {
	      if(ulds[x] & bitPlane) {
		uldd[x] = (uldd[x] & ~planemask) | fg;
	      } else {
		uldd[x] = (uldd[x] & ~planemask) | bg;
	      }
	    }
	    ulds += w;
	    uldd += w;
	  }
	}
	gsosWriteImage(pbox->x1, pbox->y1, w, h, bp, dbw, psm, dbuf);
	pbox++;
	pptSrc++;
      }
      if (sbuf) xfree(sbuf);
      if (dbuf) xfree(dbuf);
    }
  }
}

/*
 * BitBlt Pixmap to Window
 */
static inline void
gsxDoBitbltMW(pSrc,pDst,alu,nbox,pbox,pptSrc,planemask,bitPlane)
    DrawablePtr     pSrc, pDst;
    int         alu;
    int         nbox;
    BoxPtr      pbox;
    DDXPointPtr     pptSrc;
    unsigned long   planemask, bitPlane;
{
  GSXuchar *data, *d ;
  int y, w, h ;
  unsigned char depth ;
  unsigned int bp ;
  int bw, psm ;
  int bytepp;
  gsxScreenPtr pScreenPriv;

  pScreenPriv = gsxGetScreenPriv(pDst->pScreen);
  depth = pDst->depth ;
  bp = pScreenPriv->fbp;
  bw = pScreenPriv->fbw;
  psm = pScreenPriv->psm;
  bytepp = gsxGetBytePerPixel(pScreenPriv);

  data = (GSXuchar*)((PixmapPtr)pSrc)->devPrivate.ptr;

  if( bitPlane == 0 ){    /* ordinary copy area */

#ifdef GSOS_SPR_IMAGEBUFFER

    int DstdevKind;
    int SrcdevKind;
    int wqc;
    struct ps2_image img;
    struct ps2_image *imgp = &img;

    gsosKick();

    SrcdevKind = ((PixmapPtr)pSrc)->devKind;

    imgp->ptr = ps2dbp_spr[ps2dbf_spr];
    imgp->fbp = bp;
    imgp->fbw = bw;
    imgp->psm = psm;

    while (--nbox >= 0) {
      GSXuchar *dd;
      int size;
      int nlines;
      int maxlines;
      int txfline;
      int linebytewidth;
      int i;

      w = pbox->x2 - pbox->x1 ;
      h = pbox->y2 - pbox->y1 ;

      d = data + pptSrc->x * bytepp + ((PixmapPtr)pSrc)->devKind * pptSrc->y;
      y = pbox->y1;
      linebytewidth = w * gsosBytePerPixel(psm);
      maxlines = gsos_maximage_size / linebytewidth;
      DstdevKind = w * bytepp;

      imgp->x = pbox->x1;
      imgp->w = w;

      nlines = h;
      while(nlines > 0) {
	txfline = (maxlines < nlines) ? maxlines : nlines;
	size = txfline * linebytewidth;
	imgp->y = y ;
	imgp->h = txfline ;
	y += txfline;
	nlines -= txfline;

	dd = imgp->ptr;
	if(SrcdevKind == DstdevKind) {
	  memcpy((void *)dd, (void *)d, size);
	  d += size;
	} else {
	  for(i = 0; i < txfline; i++) {
	    memcpy((void *)dd, (void *)d, DstdevKind);
	    d  += SrcdevKind;
	    dd += DstdevKind ;
	  }
	}
#ifdef GSOS_LAZY_EXEC
        wqc = 2;
#else
        wqc = (nlines > 0) ? 2 : 1;
#endif

	if(ioctl(ps2gsfd, PS2IOC_LOADIMAGEA, &img) < 0) {
	  ErrorF("gsxDoBitbltMW: PS2IOC_LOADIMAGEA Error\n");
	  return;
	}
#ifdef GSOS_LAZY_EXEC
	if(!ps2dma_limit)
#else
	if(!ps2dma_limit || wqc != 2)
#endif
	  ioctl(ps2gsfd, PS2IOC_SENDQCT, wqc);

	ps2dbf_spr = 1 - ps2dbf_spr;
	imgp->ptr = ps2dbp_spr[ps2dbf_spr];

      }

      pbox++;
      pptSrc++;
    }

#else /* GSOS_SPR_IMAGEBUFFER */

    GSXuchar *buf=0;

    while (--nbox >= 0) {
      w = pbox->x2 - pbox->x1 ;
      h = pbox->y2 - pbox->y1 ;

      d = data + pptSrc->x * bytepp + ((PixmapPtr)pSrc)->devKind * pptSrc->y;
      {
	GSXuchar *dd;
	int      DstdevKind;

	DstdevKind = w * bytepp;
	buf = xrealloc(buf, DstdevKind * h);
	dd = buf;
	for( y = 0 ; y < h ; y++ ){
	  memcpy(dd, d, DstdevKind);
	  d += ((PixmapPtr)pSrc)->devKind ;
	  dd += DstdevKind ;
	}
	gsosWriteImage(pbox->x1, pbox->y1, w, h, bp, bw, psm, buf);
      }
      pbox++;
      pptSrc++;
    }
    if (buf) xfree(buf);

#endif /* GSOS_SPR_IMAGEBUFFER */

  } else {              /* copy plane */

    if(pSrc->depth > 1) {
      GSXulong  *buf = 0;
      GSXulong  *ulds, *uldd;
      GSXushort *usds, *usdd;
      int x, y ;
      GSXulong fg,bg;
      int srclinesize;
      unsigned long   iplanemask;

      iplanemask = ~planemask;
      fg = pScreenPriv->gsxFg & planemask;
      bg = pScreenPriv->gsxBg & planemask;
      srclinesize = ((PixmapPtr)pSrc)->devKind / bytepp;

      while( --nbox >= 0 ){
	w = pbox->x2 - pbox->x1 ;
	h = pbox->y2 - pbox->y1 ;

	/* src */
	ulds = (GSXulong *)(data + pptSrc->x * bytepp +
			    ((PixmapPtr)pSrc)->devKind * pptSrc->y);
	usds = (GSXushort *)ulds;

	/* dst */
	buf  = (GSXulong *)xrealloc(buf, w * bytepp * h);
	uldd = buf;
	usdd = (GSXushort *)uldd;

	if(depth <= 16) {	/* RGBA16 */
	  for(y = 0; y < h; y++) {
	    for(x = 0; x < w; x++) {
	      if(usds[x] & bitPlane) {
		usdd[x] = (usdd[x] & iplanemask) | fg;
	      } else {
		usdd[x] = (usdd[x] & iplanemask) | bg;
	      }
	    }
	    usds += srclinesize;
	    usdd += w;
	  }
	} else {		/* RGBA32 */
	  for(y = 0; y < h; y++) {
	    for(x = 0; x < w; x++) {
	      if(ulds[x] & bitPlane) {
		uldd[x] = (uldd[x] & iplanemask) | fg;
	      } else {
		uldd[x] = (uldd[x] & iplanemask) | bg;
	      }
	    }
	    ulds += srclinesize;
	    uldd += w;
	  }
	}
	gsosWriteImage(pbox->x1, pbox->y1, w, h, bp, bw, psm, (GSXuchar *)buf);
	pbox++;
	pptSrc++;
      }
      if(buf) xfree(buf);

    } else {
      /* from Bitmap */
      while( --nbox >= 0 ){
	w = pbox->x2 - pbox->x1 ;
	h = pbox->y2 - pbox->y1 ;
	gsxPutBitmap(pSrc, pDst,
		     pptSrc->x, pptSrc->y,
		     w, h,
		     pbox->x1, pbox->y1,
		     pScreenPriv->gsxFg, pScreenPriv->gsxBg);
	pbox++;
	pptSrc++;
      }
    }
  }

}

/*
 * BitBlt Window to Pixmap
 */
static inline void
gsxDoBitbltWM(pSrc,pDst,alu,nbox,pbox,pptSrc,planemask,bitPlane)
    DrawablePtr     pSrc, pDst;
    int             alu;
    int             nbox;
    BoxPtr          pbox;
    DDXPointPtr     pptSrc;
    unsigned long   planemask, bitPlane;
{
  GSXuchar *data, *d;
  int y, w, h;
  unsigned char src_depth;
  unsigned char dst_depth;
  unsigned int bp;
  int bw, psm;
  int bytepp;
  int dstDevKind;
  gsxScreenPtr pScreenPriv;

  src_depth = pSrc->depth;
  dst_depth = pDst->depth;

  pScreenPriv = gsxGetScreenPriv(pDst->pScreen);
  bp = pScreenPriv->fbp;
  bw = pScreenPriv->fbw;
  psm = pScreenPriv->psm;
  bytepp = gsxGetBytePerPixel(pScreenPriv);

  dstDevKind = ((PixmapPtr)pDst)->devKind;

  data = (GSXuchar*)((PixmapPtr)pDst)->devPrivate.ptr;

  if(bitPlane == 0) {    /* ordinary copy area */
    GSXuchar *buf=0;

    while(--nbox >= 0) {
      w = pbox->x2 - pbox->x1;
      h = pbox->y2 - pbox->y1;
      d = data + pbox->x1 * bytepp + dstDevKind * pbox->y1;
      {
	int devKind;
	GSXuchar *dd;

	devKind = w * bytepp;
	buf = (GSXuchar *)xrealloc(buf, devKind * h);
	gsosReadImage(pptSrc->x, pptSrc->y, w, h, bp, bw, psm, buf);

	dd = buf;
	for(y = 0 ; y < h ; y++) {
	  memcpy(d, dd, devKind);
	  dd += devKind;
	  d += dstDevKind;
	}
      }
      pbox++;
      pptSrc++;
    }
    if (buf) xfree(buf);

  } else {              /* copy plane */

    if(dst_depth > 1) {
      /* Dst is Pixmap */
      GSXulong  *buf = 0;
      GSXulong  *ulds, *uldd;
      GSXushort *usds, *usdd;
      int x, y;
      GSXulong fg,bg;
      int dstlinesize;

      fg = pScreenPriv->gsxFg & planemask;
      bg = pScreenPriv->gsxBg & planemask;
      dstlinesize = dstDevKind / bytepp;

      while (--nbox >= 0) {
	w = pbox->x2 - pbox->x1 ;
	h = pbox->y2 - pbox->y1 ;

	/* dst */
	uldd = (GSXulong *)(data + pbox->x1 * bytepp + dstDevKind * pbox->y1);
	usdd = (GSXushort *)uldd;

	/* src */
	buf  = (GSXulong*)xrealloc(buf, w * bytepp * h);
	ulds = buf;
	usds = (GSXushort *)ulds;
	gsosReadImage(pptSrc->x, pptSrc->y,w,h,bp,bw,psm,(GSXuchar*)buf);

	if(dst_depth <= 16) {	/* RGBA16 */
	  for(y = 0; y < h; y++) {
	    for(x = 0; x < w; x++) {
	      if(usds[x] & bitPlane) {
		usdd[x] = (usdd[x] & ~planemask) | fg;
	      } else {
		usdd[x] = (usdd[x] & ~planemask) | bg;
	      }
	    }
	    usds += w;
	    usdd += dstlinesize;
	  }
	} else {		/* RGBA32 */
	  for(y = 0 ; y < h; y++) {
	    for(x = 0; x < w; x++) {
	      if(ulds[x] & bitPlane) {
		uldd[x] = (uldd[x] & ~planemask) | fg;
	      } else {
		uldd[x] = (uldd[x] & ~planemask) | bg;
	      }
	    }
	    ulds += w;
	    uldd += dstlinesize;
	  }
	}
	pbox++;
	pptSrc++;
      }
      if(buf) xfree(buf);

    } else {

      /* Dst is Bitmap */
      while( --nbox >= 0 ){
	w = pbox->x2 - pbox->x1 ;
	h = pbox->y2 - pbox->y1 ;
	gsxGetBitmap
	  (pSrc,pDst,pptSrc->x,pptSrc->y,w,h,pbox->x1,pbox->y1,bitPlane);
	pbox++;
	pptSrc++;
      }

    }
  }
}

/*
 * BitBlt
 */
void gsxDoBitBlt(pSrc,pDst,alu,prgnDst,pptSrc,planemask,bitPlane)
     DrawablePtr     pSrc, pDst;
     int             alu;
     RegionPtr       prgnDst;
     DDXPointPtr     pptSrc;
     unsigned long   planemask;
{
  register BoxPtr pbox = REGION_RECTS(prgnDst);
  register int    nbox = REGION_NUM_RECTS(prgnDst);
  BoxPtr      pboxTmp, pboxNext, pboxBase;
  /* temporaries for shuffling rectangles */
  BoxPtr      pboxNew1 = NULL, pboxNew2 = NULL;
  DDXPointPtr     pptTmp, pptNew1 = NULL, pptNew2 = NULL;

  if (nbox == 0 || planemask == 0)
    return;

  if (pSrc->type == pDst->type) {         /* WIN -> WIN */
    if (pptSrc->y < pbox->y1) {
      /* walk source botttom to top */
      /* keep ordering in each band, reverse order of bands */
      pboxNew1 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
      pptNew1 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
      if(!pboxNew1 || !pptNew1) {
        if (pboxNew1) DEALLOCATE_LOCAL(pboxNew1);
        if (pptNew1) DEALLOCATE_LOCAL(pptNew1);
        return;
      }
      pboxBase = pboxNext = pbox+nbox-1;
      while (pboxBase >= pbox) {
        while ((pboxNext >= pbox) && 
	       (pboxBase->y1 == pboxNext->y1))
	  pboxNext--;
        pboxTmp = pboxNext+1;
        pptTmp = pptSrc + (pboxTmp - pbox);
        while (pboxTmp <= pboxBase)
	  {
	    *pboxNew1++ = *pboxTmp++;
	    *pptNew1++ = *pptTmp++;
	  }
        pboxBase = pboxNext;
      }
      pboxNew1 -= nbox;
      pbox = pboxNew1;
      pptNew1 -= nbox;
      pptSrc = pptNew1;
    }

    if (pptSrc->x < pbox->x1) {
      /* walk source right to left */
      /* reverse order of rects in each band */
      pboxNew2 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
      pptNew2 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
      if(!pboxNew2 || !pptNew2) {
        if (pptNew2) DEALLOCATE_LOCAL(pptNew2);
        if (pboxNew2) DEALLOCATE_LOCAL(pboxNew2);
        if (pboxNew1) {
	  DEALLOCATE_LOCAL(pptNew1);
	  DEALLOCATE_LOCAL(pboxNew1);
        }
        return;
      }
      pboxBase = pboxNext = pbox;
      while (pboxBase < pbox+nbox) {
        while ((pboxNext < pbox+nbox) &&
	       (pboxNext->y1 == pboxBase->y1))
	  pboxNext++;
        pboxTmp = pboxNext;
        pptTmp = pptSrc + (pboxTmp - pbox);
        while (pboxTmp != pboxBase)
	  {
	    *pboxNew2++ = *--pboxTmp;
	    *pptNew2++ = *--pptTmp;
	  }
        pboxBase = pboxNext;
      }
      pboxNew2 -= nbox;
      pbox = pboxNew2;
      pptNew2 -= nbox;
      pptSrc = pptNew2;
    }
  }         /* WIN -> WIN */

  if (nbox) {
    if (pSrc->type == pDst->type) {         /* WIN -> WIN */
      gsxDoBitbltWW(pSrc,pDst,alu,nbox,pbox,pptSrc,planemask,bitPlane);
    } else if (pDst->type != DRAWABLE_PIXMAP) { /* MEM -> WIN */
      gsxDoBitbltMW(pSrc,pDst,alu,nbox,pbox,pptSrc,planemask,bitPlane);
    } else {                    /* WIN -> MEM */
      gsxDoBitbltWM(pSrc,pDst,alu,nbox,pbox,pptSrc,planemask,bitPlane);
    }
  }

  /* free up stuff */
  if (pboxNew2) {
    DEALLOCATE_LOCAL(pptNew2);
    DEALLOCATE_LOCAL(pboxNew2);
  }
  if (pboxNew1) {
    DEALLOCATE_LOCAL(pptNew1);
    DEALLOCATE_LOCAL(pboxNew1);
  }
}

/*
 * Get Bitmap From Window
 */
static inline
void gsxGetBitmap(pSrc,pDst,srcx,srcy,w,h,dstx,dsty,bitPlane)
    DrawablePtr pSrc, pDst ;
    int srcx, srcy, w, h, dstx, dsty ;
    GSXulong bitPlane ;
{
    GSXuchar *ds, *dd ;
    GSXuchar *buf;
    int bytepp, component, id ;
    int x, y ;
    GSXulong pmask ;
    GSXuchar mask ;
    int srcline;
    int dstline;
    gsxScreenPtr pScreenPriv;
    int bp, bw, psm;

    pScreenPriv = gsxGetScreenPriv(pDst->pScreen);
    bp = pScreenPriv->fbp;
    bw = pScreenPriv->fbw;
    psm = pScreenPriv->psm;
    bytepp = gsxGetBytePerPixel(pScreenPriv);

    srcline = w * bytepp;
    buf = ALLOCATE_LOCAL(srcline * h);

    gsosReadImage(srcx, srcy, w, h, bp, bw, psm, buf);

    if( bitPlane & 0x000000ffL ){
        component = 0 ;
        pmask = bitPlane & 0xff ;
    }else if( bitPlane & 0x0000ff00L ){
        component = 1 ;
        pmask = (bitPlane >> 8) & 0xff ;
    }else if( bitPlane & 0x00ff0000L ){
        component = 2 ;
        pmask = (bitPlane >> 16) & 0xff ;
    }else if( bitPlane & 0xff000000L ){
        component = 3 ;
        pmask = (bitPlane >> 24) & 0xff ;
    } else {
	ErrorF("gsxGetBitmap: unknown bitPlane:%x\n", bitPlane);
        DEALLOCATE_LOCAL(buf);
        return;
    }
    ds = buf;
    dd = (GSXuchar *)((PixmapPtr)pDst)->devPrivate.ptr 
       + ((PixmapPtr)pDst)->devKind * dsty ;
    dstline = ((PixmapPtr)pDst)->devKind;
    for( y=0 ; y < h ; y++ ){
        for( x=0 ; x < w ; x++ ){
            id = (dstx + x) / 8 ;
            mask = (GSXuchar)1 << ((dstx +x) % 8) ;
            if( ds[(srcx+x)*bytepp+component] & pmask ){
                dd[id] = ( dd[id] & ~mask ) | ( 0xff & mask ) ;       
            }else{
                dd[id] = ( dd[id] & ~mask ) | ( 0 & mask ) ;       
            }
        }
        ds += srcline;
        dd += dstline;
    }
    DEALLOCATE_LOCAL(buf);
}

/*
 * Put Bitmap to Window
 */
static inline
void gsxPutBitmap(pSrc,pDst,srcx,srcy,w,h,dstx,dsty,fg,bg)
    DrawablePtr pSrc,pDst ;
    int srcx, srcy, w, h, dstx, dsty ;
    GSXuint fg, bg ;
{
  GSXuchar *ds, *dd;
  int bytepp;
  int x, y;
  GSXuchar mask;
  int srcline;
  int dstline;
  unsigned int bp;
  int bw, psm;
  int maxlines, nlines, txfline, size;
  int yy;
  GSXulong  *ddl;
  GSXushort *dds;
  gsxScreenPtr pScreenPriv;
  int wqc;
  struct ps2_image img;
  struct ps2_image *imgp = &img;

  gsosKick();

  pScreenPriv = gsxGetScreenPriv(pDst->pScreen);
  bp  = pScreenPriv->fbp;
  bw  = pScreenPriv->fbw;
  psm = pScreenPriv->psm;
  bytepp = gsxGetBytePerPixel(pScreenPriv);
  dstline = w * bytepp;
  srcline = ((PixmapPtr)pSrc)->devKind;
  ds = (GSXuchar *)((PixmapPtr)pSrc)->devPrivate.ptr
    + srcline * srcy ;
#ifdef GSOS_SPR_IMAGEBUFFER
  maxlines = gsos_maximage_size / dstline;
  imgp->ptr = ps2dbp_spr[ps2dbf_spr];
#else
  maxlines = gsos_maxpacket_size / dstline;
  imgp->ptr = ps2dbp[ps2dbf];
#endif
  imgp->fbp = bp;
  imgp->fbw = bw;
  imgp->psm = psm;
  imgp->x = dstx;
  imgp->w = w;

  fg &= pScreenPriv->rgb_pix_mask;
  bg &= pScreenPriv->rgb_pix_mask;

  nlines = h;
  y = dsty;
  while(nlines > 0) {
    txfline = (maxlines < nlines) ? maxlines : nlines;
    size = txfline * dstline;
    imgp->y = y ;
    imgp->h = txfline ;
    y += txfline;
    nlines -= txfline;
      
    dd = imgp->ptr;
    if(psm == PS2_GS_PSMCT16) {			/* RGBA16 */
      for(yy = 0; yy < txfline; yy++) {
	ddl = (GSXulong*)dd;
	dds = (GSXushort*)dd;
	for(x = 0 ; x < w; x++) {
	  mask = (GSXuchar)1 << ((srcx +x) % 8) ;
	  if( ds[(srcx+x)/8] & mask ){
	    dds[x] = fg;
	  }else{
	    dds[x] = bg;
	  }
	}
	ds += srcline;
	dd += dstline;
      }
    } else {			/* RGBA32 */
      for(yy = 0; yy < txfline; yy++) {
	ddl = (GSXulong*)dd;
	dds = (GSXushort*)dd;
	for(x = 0; x < w; x++) {
	  mask = (GSXuchar)1 << ((srcx +x) % 8) ;
	  if( ds[(srcx+x)/8] & mask ){
	    ddl[x] = fg;
	  }else{
	    ddl[x] = bg;
	  }
	}
	ds += srcline;
	dd += dstline;
      }
    }
#ifdef GSOS_LAZY_EXEC
    wqc = 2;
#else
    wqc = (nlines > 0) ? 2 : 1;
#endif

    if(ioctl(ps2gsfd, PS2IOC_LOADIMAGEA, &img) < 0) {
      ErrorF("gsxDoBitbltMW: PS2IOC_LOADIMAGEA Error\n");
      return;
    }
#ifdef GSOS_LAZY_EXEC
    if(!ps2dma_limit)
#else
    if(!ps2dma_limit || wqc != 2)
#endif
      ioctl(ps2gsfd, PS2IOC_SENDQCT, wqc);

#ifdef GSOS_SPR_IMAGEBUFFER
    ps2dbf_spr = 1 - ps2dbf_spr;
    imgp->ptr = ps2dbp_spr[ps2dbf_spr];
#else
    ps2dbf = 1 - ps2dbf;
    ps2cur = ps2dbp[ps2dbf];
    imgp->ptr = ps2dbp[ps2dbf];
#endif

  }
}

/*
 * Copy Area with Logical Operation
 */
RegionPtr gsxCopyAreaLop( src,dst,pGC,srcx,srcy,w,h,dstx,dsty )
    DrawablePtr src, dst;
    GCPtr   pGC;
    int     srcx, srcy, w, h, dstx, dsty ;
{
    xRectangle aRect;
    PixmapPtr  srcLop;
    PixmapPtr  dstLop;
    int        srcLopf;
    RegionPtr  dstRegion;
    int        sx, sy;
    GCPtr      pGC1;
    gsxScreenPtr pScreenPriv;
    int bp,bw,psm;

    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    bp=pScreenPriv->fbp;
    bw=pScreenPriv->fbw;
    psm=pScreenPriv->psm;


    if( src->type == DRAWABLE_PIXMAP && dst->type == DRAWABLE_PIXMAP ){
        return (*gsxGetGCPriv(pGC)->mops->CopyArea)
           (src, dst, pGC, srcx, srcy, w, h, dstx, dsty ) ;
    }

    /* get region for LOP */
    aRect.width  = w ;
    aRect.height = h ;

    /* Source */
    aRect.x = srcx + src->x ;
    aRect.y = srcy + src->y ;

    if(src->type == DRAWABLE_WINDOW) {
      srcLop = (*src->pScreen->CreatePixmap)
        (dst->pScreen, aRect.width, aRect.height, dst->depth ) ;
      if( !srcLop ){
        return NULL ;
      }
      gsxReadImage(aRect.x, aRect.y, aRect.width, aRect.height, 
		   bp,bw,psm,
		   srcLop);
      sx = 0;
      sy = 0;
      srcLopf = TRUE;
    } else {
      srcLop  = (PixmapPtr)src;
      sx = srcx;
      sy = srcy;
      srcLopf = FALSE;
    }

    /* Destination */
    /* get region for LOP */
    aRect.x = dstx + dst->x ;
    aRect.y = dsty + dst->y ;

    dstLop = (*dst->pScreen->CreatePixmap)
      (dst->pScreen, aRect.width, aRect.height, dst->depth ) ;
    if( !dstLop ){
      if(srcLopf) {
	(*src->pScreen->DestroyPixmap)(srcLop) ;
      }
      return NULL ;
    }
    if(dst->type == DRAWABLE_WINDOW) {
      gsxReadImage(aRect.x, aRect.y, aRect.width, aRect.height, 
		   bp,bw,psm,
		   dstLop);
    } else {
      pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
      CopyGC(pGC, pGC1, ~GCClipMask | ~GCFunction);
      GSXVALIDATEGC((DrawablePtr)dstLop, pGC1);
      dstRegion = (*gsxGetGCPriv(pGC)->mops->CopyArea)((DrawablePtr)dst, (DrawablePtr)dstLop, pGC1, dstx, dsty, w, h, 0, 0);
      FreeScratchGC(pGC1);
    }

    /* Raster Operation */
    pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
    CopyGC(pGC, pGC1, ~GCClipMask);
    GSXVALIDATEGC((DrawablePtr)dstLop, pGC1);
    dstRegion = (*gsxGetGCPriv(pGC1)->mops->CopyArea)((DrawablePtr)srcLop, (DrawablePtr)dstLop, pGC1, sx, sy, w, h, 0, 0);
    GSXVALIDATEGC(dst, pGC);
    FreeScratchGC(pGC1);

    if(dst->type == DRAWABLE_WINDOW) {
      dstRegion = (RegionPtr)cfbBitBlt((DrawablePtr)dstLop, dst, pGC, 0, 0, w, h, dstx, dsty, gsxDoBitBlt, 0L);
    }
    if(srcLopf) {
      (*src->pScreen->DestroyPixmap)(srcLop) ;
    }
    (*dst->pScreen->DestroyPixmap)(dstLop) ;

    return dstRegion ;
}

/*
 * Copy Area GXcopy
 */
RegionPtr gsxCopyAreaGxcpy( src,dst,pGC,srcx,srcy,w,h,dstx,dsty )
    DrawablePtr src, dst;
    GCPtr   pGC;
    int     srcx, srcy, w, h, dstx, dsty ;
{
    RegionPtr rgn ;

    if( src->type == DRAWABLE_PIXMAP && dst->type == DRAWABLE_PIXMAP ){
        return (*gsxGetGCPriv(pGC)->mops->CopyArea)
           (src, dst, pGC, srcx, srcy, w, h, dstx, dsty ) ;
    }
    /* DoBitblt */
    rgn = (RegionPtr)cfbBitBlt( src, dst, pGC, 
        srcx, srcy, w, h, dstx, dsty, gsxDoBitBlt, 0L ) ;

    return rgn ;
}

/*
 * Copy Plane with Logical Operation
 */
RegionPtr gsxCopyPlaneLop(src,dst,pGC,srcx,srcy,w,h,dstx,dsty,plane)
    DrawablePtr src, dst;
    GCPtr   pGC;
    int     srcx, srcy;
    int     w, h;
    int     dstx, dsty;
    long    plane;
{
    xRectangle aRect;
    PixmapPtr  srcLop;
    PixmapPtr  dstLop;
    int        srcLopf;
    RegionPtr  dstRegion;
    int        sx, sy;
    GCPtr      pGC1;
    gsxScreenPtr pScreenPriv;
    int		bp,bw,psm;

    if( src->type == DRAWABLE_PIXMAP && dst->type == DRAWABLE_PIXMAP ){
        return (*gsxGetGCPriv(pGC)->mops->CopyPlane)
           (src, dst, pGC, srcx, srcy, w, h, dstx, dsty, plane ) ;
    }

    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    bp  = pScreenPriv->fbp;
    bw  = pScreenPriv->fbw;
    psm = pScreenPriv->psm;

    /* get region for LOP */
    aRect.width  = w ;
    aRect.height = h ;

    /* Source */
    aRect.x = srcx + src->x ;
    aRect.y = srcy + src->y ;

    if(src->type == DRAWABLE_WINDOW) {
      srcLop = (*src->pScreen->CreatePixmap)
        (dst->pScreen, aRect.width, aRect.height, dst->depth ) ;
      if( !srcLop ){
        return NULL ;
      }
      gsxReadImage(aRect.x, aRect.y, aRect.width, aRect.height, 
		   bp, bw, psm, srcLop);
      sx = 0;
      sy = 0;
      srcLopf = TRUE;
    } else {
      srcLop  = (PixmapPtr)src;
      sx = srcx;
      sy = srcy;
      srcLopf = FALSE;
    }

    /* Destination */
    /* get region for LOP */
    aRect.x = dstx + dst->x ;
    aRect.y = dsty + dst->y ;

    dstLop = (*dst->pScreen->CreatePixmap)
      (dst->pScreen, aRect.width, aRect.height, dst->depth ) ;
    if( !dstLop ){
      if(srcLopf) {
	(*src->pScreen->DestroyPixmap)(srcLop) ;
      }
      return NULL ;
    }
    if(dst->type == DRAWABLE_WINDOW) {
      gsxReadImage(aRect.x, aRect.y, aRect.width, aRect.height, 
		   bp, bw, psm, dstLop);
    } else {
      pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
      CopyGC(pGC, pGC1, ~GCClipMask | ~GCFunction);
      GSXVALIDATEGC((DrawablePtr)dstLop, pGC1);
      dstRegion = (*gsxGetGCPriv(pGC)->mops->CopyArea)((DrawablePtr)dst, (DrawablePtr)dstLop, pGC1, dstx, dsty, w, h, 0, 0);
      FreeScratchGC(pGC1);
    }

    /* Raster Operation */
    pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
    CopyGC(pGC, pGC1, ~GCClipMask);
    GSXVALIDATEGC((DrawablePtr)dstLop, pGC1);
    dstRegion = (*gsxGetGCPriv(pGC1)->mops->CopyPlane)((DrawablePtr)srcLop, (DrawablePtr)dstLop, pGC1, sx, sy, w, h, 0, 0, plane);
    GSXVALIDATEGC(dst, pGC);
    FreeScratchGC(pGC1);

    if(dst->type == DRAWABLE_WINDOW) {
      dstRegion = (RegionPtr)cfbBitBlt((DrawablePtr)dstLop, dst, pGC, 0, 0, w, h, dstx, dsty, gsxDoBitBlt, 0L);
    }
    if(srcLopf) {
      (*src->pScreen->DestroyPixmap)(srcLop) ;
    }
    (*dst->pScreen->DestroyPixmap)(dstLop) ;

    return dstRegion ;
}

/*
 * Copy Plane for GXcopy
 */
RegionPtr gsxCopyPlaneGxcpy(src,dst,pGC,srcx,srcy,w,h,dstx,dsty,plane)
    DrawablePtr src, dst;
    GCPtr   pGC;
    int     srcx, srcy;
    int     w, h;
    int     dstx, dsty;
    long    plane;
{
    RegionPtr   ret;
    gsxScreenPtr pScreenPriv;

    if (dst->type==DRAWABLE_PIXMAP && src->type==DRAWABLE_PIXMAP) {
        return (*gsxGetGCPriv(pGC)->mops->CopyPlane)
                (src,dst,pGC,srcx,srcy,w,h,dstx,dsty,plane);
    }
    /* use CopyPlane Fg/Bg colors */
    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    pScreenPriv->gsxFg = pGC->fgPixel & pScreenPriv->rgb_pix_mask;
    pScreenPriv->gsxBg = pGC->bgPixel & pScreenPriv->rgb_pix_mask;
    ret = (RegionPtr)cfbBitBlt
       (src,dst,pGC,srcx,srcy,w,h,dstx,dsty,gsxDoBitBlt,plane);
    return ret;
}

/*
 * SaveAreas for Backing Store
 */
void
gsxSaveAreas(pPixmap, prgnSave, xorg, yorg, pWin)
    PixmapPtr    pPixmap;        /* Backing pixmap */
    RegionPtr    prgnSave;       /* Region to save (pixmap-relative) */
    int          xorg;           /* X origin of region */
    int          yorg;           /* Y origin of region */
    WindowPtr    pWin;
{
  BoxPtr       pBox;
  int          i;
  GSXuchar     *data, *d;
  int          y, w, h;
  unsigned int bp;
  int          bw, psm;
  int          bytepp;
  int          dstDevKind;
  GSXuchar     *buf=0;
  gsxScreenPtr pScreenPriv;

  pScreenPriv = gsxGetScreenPriv((pWin)->drawable.pScreen);
  bp = pScreenPriv->fbp;
  bw = pScreenPriv->fbw;
  psm = pScreenPriv->psm;

  bytepp = gsxGetBytePerPixel(pScreenPriv);
  dstDevKind = pPixmap->devKind;
  data = (GSXuchar*)pPixmap->devPrivate.ptr;

  i = REGION_NUM_RECTS(prgnSave);
  pBox = REGION_RECTS(prgnSave);
  while(--i >= 0) {
    w = pBox->x2 - pBox->x1 ;
    h = pBox->y2 - pBox->y1;
    d = data + pBox->x1 * bytepp + dstDevKind * pBox->y1;
    {
      int      devKind;
      GSXuchar *dd;

      devKind = w * bytepp;
      buf = (GSXuchar *) xrealloc(buf, devKind * h);
      gsosReadImage(pBox->x1 + xorg, pBox->y1 + yorg, w, h, bp, bw, psm, buf);
      dd = buf;
      for(y = 0 ; y < h ; y++) {
	memcpy(d, dd, devKind);
	dd += devKind;
	d += dstDevKind;
      }
    }
    pBox++;
  }
  if (buf) xfree(buf);
}

/*
 * RestoreAreas for Backing Store
 */
void
gsxRestoreAreas(pPixmap, prgnRestore, xorg, yorg, pWin)
    PixmapPtr    pPixmap;        /* Backing pixmap */
    RegionPtr    prgnRestore;    /* Region to restore (screen-relative)*/
    int          xorg;           /* X origin of window */
    int          yorg;           /* Y origin of window */
    WindowPtr    pWin;
{
  BoxPtr        pBox;
  int           i;
  GSXuchar      *data, *d;
  int           y, w, h;
  unsigned int  bp;
  int           bw, psm;
  int           bytepp;
  int           devKind;
  GSXuchar      *buf=0;
  gsxScreenPtr pScreenPriv;

  pScreenPriv = gsxGetScreenPriv((pWin)->drawable.pScreen);
  bp = pScreenPriv->fbp;
  bw = pScreenPriv->fbw;
  psm = pScreenPriv->psm;

  bytepp = gsxGetBytePerPixel(pScreenPriv);
  devKind = ((PixmapPtr)pPixmap)->devKind;

  data = (GSXuchar*)((PixmapPtr)pPixmap)->devPrivate.ptr;
  i = REGION_NUM_RECTS(prgnRestore);
  pBox = REGION_RECTS(prgnRestore);
  while(--i >= 0) {
    GSXuchar *dd;
    int      DstdevKind;

    w = pBox->x2 - pBox->x1 ;
    h = pBox->y2 - pBox->y1 ;
    d = data + (pBox->x1 - xorg) * bytepp + devKind * (pBox->y1 - yorg);
    DstdevKind = w * bytepp;
    buf = (GSXuchar *)xrealloc(buf, DstdevKind * h);
    dd = buf;
    for( y = 0 ; y < h ; y++ ){
      memcpy(dd, d, DstdevKind);
      d += devKind;
      dd += DstdevKind;
    }
    gsosWriteImage(pBox->x1, pBox->y1, w, h, bp, bw, psm, buf);
    pBox++;
  }
  if (buf) xfree(buf);
}
