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

#include "pixmapstr.h"
#include "gsx.h"

const char gsxLog2GETbl[33]= { -1, 
  			   0, 1, 2, 2, 3,
			   3, 3, 3, 4, 4,
			   4, 4, 4, 4, 4,
			   4, 5, 5, 5, 5,
			   5, 5, 5, 5, 5,
			   5, 5, 5, 5, 5,
			   5, 5};

void gsxCalcDrawRegion( pPoint, n, pRect )
    DDXPointPtr pPoint;
    int n;
    xRectangle *pRect;
{
    int x, y;
    int minx, miny, maxx, maxy;
    int i;

    if( !n ){
        pRect->x = 0;
        pRect->y = 0;
        pRect->width = 0;
        pRect->height = 0;
    }
    x = minx = maxx = pPoint[0].x ;
    y = miny = maxy = pPoint[0].y ;
    for( i=1; i < n ; i++ ){
        x = pPoint[i].x ;
        y = pPoint[i].y ;
        if( x < minx ){
            minx = x ;
        }
        if( y < miny ){
            miny = y ;
        }
        if( x > maxx ){
            maxx = x ;
        }
        if( y > maxy ){
            maxy = y ;
        }
    }
    pRect->x = minx ;
    pRect->y = miny ;
    pRect->width = maxx - minx + 1;
    pRect->height = maxy - miny + 1;
}

void gsxCalcDrawRegionSeg( pSegments, n, pRect )
    xSegment *pSegments;
    int n;
    xRectangle *pRect;
{
    int minx, miny, maxx, maxy;
    int i;

    if( !n ){
        pRect->x = 0;
        pRect->y = 0;
        pRect->width = 0;
        pRect->height = 0;
    }
    minx = maxx = pSegments[0].x1 ;
    miny = maxy = pSegments[0].y1 ;
    for( i=0; i < n ; i++ ){
        if( pSegments[i].x1 < minx ){
            minx = pSegments[i].x1 ;
        }
        if( pSegments[i].y1 < miny ){
            miny = pSegments[i].y1 ;
        }
        if( pSegments[i].x1 > maxx ){
            maxx = pSegments[i].x1 ;
        }
        if( pSegments[i].y1 > maxy ){
            maxy = pSegments[i].y1 ;
        }
        if( pSegments[i].x2 < minx ){
            minx = pSegments[i].x2 ;
        }
        if( pSegments[i].y2 < miny ){
            miny = pSegments[i].y2 ;
        }
        if( pSegments[i].x2 > maxx ){
            maxx = pSegments[i].x2 ;
        }
        if( pSegments[i].y2 > maxy ){
            maxy = pSegments[i].y2 ;
        }
    }
    pRect->x = minx ;
    pRect->y = miny ;
    pRect->width = maxx - minx + 1;
    pRect->height = maxy - miny + 1;
}

void gsxCalcDrawRegionSpan( pPoint, pWidths, n, pRect )
    DDXPointPtr pPoint;
    int *pWidths;
    int n;
    xRectangle *pRect;
{
    int minx, miny, maxx, maxy;
    int i;

    if( !n ){
        pRect->x = 0;
        pRect->y = 0;
        pRect->width = 0;
        pRect->height = 0;
    }
    minx = pPoint[0].x ;
    maxx = pPoint[0].x + pWidths[0] ;
    miny = maxy = pPoint[0].y ;
    for( i=1; i < n ; i++ ){
        if( pPoint[i].x < minx ){
            minx = pPoint[i].x ;
        }
        if( pPoint[i].y < miny ){
            miny = pPoint[i].y ;
        }
        if( pPoint[i].x + pWidths[i] > maxx ){
            maxx = pPoint[i].x + pWidths[i] ;
        }
        if( pPoint[i].y > maxy ){
            maxy = pPoint[i].y ;
        }
    }
    pRect->x = minx ;
    pRect->y = miny ;
    pRect->width = maxx - minx +1 ;
    pRect->height = maxy - miny +1 ;
}

void gsxCalcDrawRegionRect( pSrcRect, n, pRect )
    xRectangle *pSrcRect;
    int n;
    xRectangle *pRect;
{
  int minx, miny, maxx, maxy;
  int i;
  int tx, ty;

  if(n == 0){
    pRect->x = 0;
    pRect->y = 0;
    pRect->width = 0;
    pRect->height = 0;
  }
  minx = pSrcRect[0].x;
  maxx = pSrcRect[0].x + pSrcRect[0].width;
  miny = pSrcRect[0].y;
  maxy = pSrcRect[0].y + pSrcRect[0].height;
  for(i = 1; i < n; i++) {
    if( pSrcRect[i].x < minx ) {
      minx = pSrcRect[i].x ;
    }
    if( pSrcRect[i].y < miny ) {
      miny = pSrcRect[i].y ;
    }
    tx = pSrcRect[i].x + pSrcRect[i].width;
    if(tx > maxx) {
      maxx = tx;
    }
    ty = pSrcRect[i].y + pSrcRect[i].height;
    if(ty > maxy) {
      maxy = ty;
    }
  }
  pRect->x = minx ;
  pRect->y = miny ;
  tx = maxx - minx;
  if(tx <= 0) {
    tx++;
  }
  ty = maxy - miny;
  if(ty <= 0) {
    ty++;
  }
  pRect->width = tx;
  pRect->height = ty;
}
    
void gsxCalcDrawRegionArc( pArc, n, pRect )
    xArc *pArc;
    int n;
    xRectangle *pRect;
{
    int minx, miny, maxx, maxy;
    int i;

    if( !n ){
        pRect->x = 0;
        pRect->y = 0;
        pRect->width = 0;
        pRect->height = 0;
    }
    minx = pArc[0].x ;
    maxx = pArc[0].x + pArc[0].width ;
    miny = pArc[0].y ;
    maxy = pArc[0].y + pArc[0].height ;
    for( i=1; i < n ; i++ ){
        if( pArc[i].x < minx ){
            minx = pArc[i].x ;
        }
        if( pArc[i].y < miny ){
            miny = pArc[i].y ;
        }
        if( pArc[i].x + pArc[i].width > maxx ){
            maxx = pArc[i].x + pArc[i].width ;
        }
        if( pArc[i].y + pArc[i].height > maxy ){
            maxy = pArc[i].y + pArc[i].height ;
        }
    }
    pRect->x = minx ;
    pRect->y = miny ;
    pRect->width = maxx - minx ;
    pRect->height = maxy - miny ;
}
    
unsigned int
gsxGetColormask( pGC, kind )
    GCPtr pGC;
    unsigned int kind;
{
    gsxScreenPtr pScreenPriv;
    unsigned int alpha_enables;
    unsigned int rgb_enables;
    unsigned int enables;
    unsigned int plmask;
    unsigned int r,g,b,a;

    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);

    // convert to fbmask's bit layout
    plmask =  pGC->planemask;
    r = (pScreenPriv->pVisual->redMask & plmask) 
    		>> pScreenPriv->pVisual->offsetRed;
    g = (pScreenPriv->pVisual->greenMask & plmask) 
    		>> pScreenPriv->pVisual->offsetGreen;
    b = (pScreenPriv->pVisual->blueMask & plmask) 
    		>> pScreenPriv->pVisual->offsetBlue;
    a = plmask & ~ pScreenPriv->rgb_pix_mask;

    rgb_enables = ((r << pScreenPriv->fbmask_red_shift) &
    		pScreenPriv->fbmask_red_mask) |
	      ((g << pScreenPriv->fbmask_green_shift)  &
    		pScreenPriv->fbmask_green_mask) |
	      ((b << pScreenPriv->fbmask_blue_shift) &
    		pScreenPriv->fbmask_blue_mask);
    alpha_enables = ((a << pScreenPriv->fbmask_alpha_shift) &
    		pScreenPriv->fbmask_alpha_mask);

    switch( kind ){
    case GSX_COLMASK_ASHUT:
	enables =  rgb_enables & ~pScreenPriv->fbmask_alpha_mask;
        break ;
    case GSX_COLMASK_AONLY:
	enables =  pScreenPriv->fbmask_alpha_mask;
        break ;
    case GSX_COLMASK_ALL:
    default:
        enables = rgb_enables | alpha_enables;
    }
    return ~enables;
}

void gsxGetRgbaWithVisual( pVisual, rgb, r, g, b, a)
    VisualPtr pVisual;
    unsigned int rgb;
    int *r;
    int *g;
    int *b;
    int *a;
{
    int off;
    off = 8 - pVisual->bitsPerRGBValue; /* XXX: */
    if (off<=0) off=0;
    *r = (pVisual->redMask & rgb) >> pVisual->offsetRed << off;
    *g = (pVisual->greenMask & rgb) >> pVisual->offsetGreen << off;
    *b = (pVisual->blueMask & rgb) >> pVisual->offsetBlue << off;
    *a = 0x80;  /* XXX */
}

void gsxSetGSRegs(gsxGSRegsPtr pRegs)
{
    gsosMakeGiftag( 21,
    	GSOS_GIF_PRE_IGNORE, 0, 
	GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD ) ;
    gsosSetPacketAddrData( GSOS_XYOFFSET_1, pRegs->xyoffset_1 ) ;
    gsosSetPacketAddrData( GSOS_FRAME_1, pRegs->frame_1 ) ;
    gsosSetPacketAddrData( GSOS_ZBUF_1, pRegs->zbuf_1 ) ;
    gsosSetPacketAddrData( GSOS_PRMODECONT, pRegs->prmodecont ) ;
    gsosSetPacketAddrData( GSOS_TEX0_1, pRegs->tex0_1 ) ;
    gsosSetPacketAddrData( GSOS_CLAMP_1, pRegs->clamp_1 ) ;
    gsosSetPacketAddrData( GSOS_TEX1_1, pRegs->tex1_1 ) ;
    gsosSetPacketAddrData( GSOS_TEX2_1, pRegs->tex2_1 ) ;
    gsosSetPacketAddrData( GSOS_MIPTBP1_1, pRegs->miptbp1_1 ) ;
    gsosSetPacketAddrData( GSOS_MIPTBP2_1, pRegs->miptbp2_1 ) ;
    gsosSetPacketAddrData( GSOS_SCISSOR_1, pRegs->scissor_1 ) ;
    gsosSetPacketAddrData( GSOS_ALPHA_1, pRegs->alpha_1 ) ;
    gsosSetPacketAddrData( GSOS_TEST_1, pRegs->test_1 ) ;
    gsosSetPacketAddrData( GSOS_FBA_1, pRegs->fba_1 ) ;
    gsosSetPacketAddrData( GSOS_PRMODE, pRegs->prmode ) ;
    gsosSetPacketAddrData( GSOS_TEXCLUT, pRegs->texclut ) ;
    gsosSetPacketAddrData( GSOS_TEXA, pRegs->texa ) ;
    gsosSetPacketAddrData( GSOS_DIMX, pRegs->dimx ) ;
    gsosSetPacketAddrData( GSOS_DTHE, pRegs->dthe ) ;
    gsosSetPacketAddrData( GSOS_COLCLAMP, pRegs->colclamp ) ;
    gsosSetPacketAddrData( GSOS_PABE, pRegs->pabe ) ;

    gsosExec() ;
}

void
gsxInitGSregs(ScreenPtr pScreen)
{
    gsxScreenPtr pScreenPriv;
    gsxGSRegs  gsregs;
    gsxGSRegsPtr pRegs = &gsregs;
    int psm ;
    unsigned int fbmask;
    GSOSulong texbase;


    pScreenPriv = gsxGetScreenPriv(pScreen);
    fbmask = pScreenPriv->cur_fbmask;
    psm = pScreenPriv->psm;
    texbase = GSX_TEX0_BASE;	/* unused */

    pRegs->xyoffset_1 = GsosXyoffsetData( GSOS_XYOFFSET<<4, GSOS_XYOFFSET<<4);
    pRegs->frame_1 = 
        GsosFrameData( pScreenPriv->fbp, pScreenPriv->fbw, psm, fbmask ) ;
    pRegs->zbuf_1 = GsosZbufData( GSX_ZBUF_BASE, 0, 0 ) ;
    pRegs->prmodecont = GsosPrmodecontData( 0 ) ;
    pRegs->tex0_1 = 
        GsosTex0Data(
        texbase, 1, psm,
        6, 6,        /* 64x64 */
        1,           /* RGBA */
        0,           /* MODULATE */
        0, 0, 0, 0, 0 ) ;
    pRegs->tex1_1 = GsosTex1Data( 0, 0, 0, 0, 0, 0, 0 ) ;
    pRegs->tex2_1 = GsosTex2Data( 0, 0, 0, 0, 0, 0 ) ;
    pRegs->clamp_1 = GsosClampData( 0, 0, 0, 0, 0, 0 ) ;
    pRegs->miptbp1_1 = GsosMiptbp1Data( 0, 0, 0, 0, 0, 0 ) ;
    pRegs->miptbp2_1 = GsosMiptbp2Data( 0, 0, 0, 0, 0, 0 ) ;
    pRegs->scissor_1 =
        GsosScissorData( 0, pScreen->width-1, 0, pScreen->height-1 ) ;
    pRegs->alpha_1 = GsosAlphaData( 0, 0, 0, 0, 0 ) ;
    /* alpha test enable */
    pRegs->test_1 = GsosTestData( 1, 1, 0, 0, 0, 0, 0, 0 ) ;
    pRegs->fba_1 = GsosFbaData( 0 ) ;
    /* use UV */
    pRegs->prmode = GsosPrmodeData( 0, 0, 0, 0, 0, 1, 0, 0 ) ;
    pRegs->texclut = GsosTexclutData( 0, 0, 0 ) ;
    pRegs->texa = GsosTexaData( 0, 0, 0x80 ) ;
    pRegs->dimx = GsosDimxData( 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 ) ;
    pRegs->dthe = GsosDtheData( 0 ) ;
    pRegs->colclamp = GsosColclampData( 0 ) ;
    pRegs->pabe = GsosPabeData( 0 ) ;

    gsxSetGSRegs(pRegs);
}

void gsxCacheInvalid()
{
    gsosMakeGiftag( 1,
    	GSOS_GIF_PRE_IGNORE, 0, 
	GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD );
    gsosSetPacketAddrData( GSOS_CACHEINVLD, 0 ) ;
    gsosExec() ;
}


/*ARGSUSED*/
void
gsxQueryBestSize(class, pwidth, pheight, pScreen)
    int class;
    unsigned short *pwidth;
    unsigned short *pheight;
    ScreenPtr pScreen;
{
    switch(class)
    {
      case CursorShape:
          if (*pwidth > TILE_CACHE_WIDTH-GSX_SPRITE_PAD*2)
	      *pwidth=TILE_CACHE_WIDTH-GSX_SPRITE_PAD*2;
          if (*pheight > TILE_CACHE_HEIGHT-GSX_SPRITE_PAD*2)
              *pheight=TILE_CACHE_HEIGHT-GSX_SPRITE_PAD*2;
          break;
      case StippleShape:
      case TileShape:
          {
	    int i;
	    if (TILE_CACHE_WIDTH < *pwidth) {
		*pwidth = TILE_CACHE_WIDTH;
	    } else {
		i = gsxGetLog2GE(*pwidth);
		if (i>=0) {
		   *pwidth = 1<< i;
		}
	    }
	    if (TILE_CACHE_HEIGHT < *pheight) {
		*pheight = TILE_CACHE_HEIGHT;
	    } else {
		i = gsxGetLog2GE(*pheight);
		if (i>=0) {
		    *pheight = 1<< i;
		}
	    }
	  }
          break;
    }
}


Bool
gsxSaveScreen (pScreen, on)
    ScreenPtr   pScreen;
    int         on;
{

    if (on != SCREEN_SAVER_FORCER)
    {
        if (on == SCREEN_SAVER_ON) {
            /* save screen */
        } else {
            /* enable screen */
        }
    }
    return( TRUE );
}

int gsxReadImage( 
  int x, int y, int w, int h,
  unsigned int bp,
  int bw, int psm,
  PixmapPtr pPix)
{
  if(psm == PS2_GS_PSMCT16) {	/* RGBA16 */
    int d;
    d = pPix->devKind - w * 2;
    if(d > 0) {
      w += d / 2;
    }
  }
  return gsosReadImage(x, y, w, h, bp, bw, psm, pPix->devPrivate.ptr);
}

#if 0
int gsxWriteImage( 
  int x, int y, int w, int h,
  unsigned int bp,
  int bw, int psm,
  PixmapPtr pPix )
{
  int      line;
  GSXuchar *sp, *dp, *buf;
  int      DstdevKind;
  int      SrcdevKind;
  int      bufsize;

  DstdevKind = w * 2;
  SrcdevKind = pPix->devKind;
  if(psm == PS2_GS_PSMCT32 || SrcdevKind == DstdevKind) { /* RGBA32 or ...*/
    return gsosWriteImage(x, y, w, h, bp, bw, psm, pPix->devPrivate.ptr);
  }
  bufsize = DstdevKind * h;
  buf = ALLOCATE_LOCAL(bufsize);
  dp = buf;
  sp = (GSXuchar *)pPix->devPrivate.ptr;
  for(line = 0; line < h; line++) {
    memcpy(dp, sp, DstdevKind);
    dp += DstdevKind;
    sp += SrcdevKind;
  }
  gsosWriteImage(x, y, w, h, bp, bw, psm, buf);
  DEALLOCATE_LOCAL(buf);
  return 0;
}
#endif

/*
 * Dump image for DEBUG
 */
#if 0
void gsxImageDump(void *p, int w, int h, int bsz, int image)
{
  GSXuchar  *dc;
  GSXushort *ds;
  GSXulong  *dl;
  int x, y;

  dc = (GSXuchar *)p;
  ds = (GSXushort *)p;
  dl = (GSXulong *)p;
  /*  ErrorF("%08x:", dc); */
  for(y = 0; y < h; y++) {
    for(x = 0; x < w; x++) {
      switch(bsz) {
      case 1:
	if(image) {
	  if(*dc++ == 0) ErrorF("."); else ErrorF("X");
	} else {
	  ErrorF("%02x ", *dc++);
	}
	break;
      case 2:
	if(image) {
	  if(*ds++ == 0) ErrorF("."); else ErrorF("X");
	} else {
	  ErrorF("%04x ", *ds++);
	}
	break;
      case 4:
	if(image) {
	  if(*dl++ == 0) ErrorF("."); else ErrorF("X");
	} else {
	  ErrorF("%08x ", *dl++);
	}
      }
    }
    ErrorF("\n");
  }
}

void gsxPixmapDump(PixmapPtr pPix, int image)
{
  GSXuchar  *line_top;
  int x, y;
  int w, h;
  int bpp;

  line_top = (GSXuchar *)pPix->devPrivate.ptr;
  w = ((DrawablePtr )pPix)->width;
  h = ((DrawablePtr )pPix)->height;
  bpp = BitsPerPixel(((DrawablePtr )pPix)->depth);
  ErrorF("gsxPixmapDump: w=%d, h=%d, bpp=%d\n", w, h, bpp);
  for(y = 0; y < h; y++) {
    GSXuchar  *dc;
    GSXushort *ds;
    GSXulong  *dl;
    dc = (GSXuchar  *)line_top;
    ds = (GSXushort *)line_top;
    dl = (GSXulong  *)line_top;
    for(x = 0; x < w; x++) {
      switch(bpp) {
      case 1:
	if(image) {
	  if(*dc++ == 0) ErrorF("."); else ErrorF("X");
	} else {
	  if(*dc == 0) ErrorF(".."); else ErrorF("%02x ", *dc);
	  dc++;
	}
	break;
      case 16:
	if(image) {
	  if(*ds++ == 0) ErrorF("."); else ErrorF("X");
	} else {
	  if(*ds == 0) ErrorF("...."); else ErrorF("%04x ", *ds);
	  ds++;
	}
	break;
      case 24:
      case 32:
	if(image) {
	  if(*dl++ == 0) ErrorF("."); else ErrorF("X");
	} else {
	  if(*dl == 0) ErrorF("...."); else ErrorF("%08x ", *dl);
	  dl++;
	}
	break;
      default:
	ErrorF("Unknown depth=%d\n", bpp);
	return;
      }
    }
    line_top += pPix->devKind;
    ErrorF("\n");
  }
}
#endif
