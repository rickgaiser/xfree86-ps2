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

#include "gsos.h"

#include <stdio.h>

#include <assert.h>
#define AssertGsosCommandBufferExecuted() assert(ps2count==0)
    // assert command buffer in gsos is flushed

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <linux/ps2/dev.h>

#define GSOS_PS2MEM_SIZE (32 * 2 * 1024)

/* for gsosCalcPrimSize() */
const int __gsos_datatype_size[] = {
	16, /*PACKED*/
	 8, /*REGLIST*/
	16, /*IMAGE*/
	16  /*inhibit*/};

 
GSOSbit64 *ps2lastgiftag = NULL ;    /* last giftag ptr */

int ps2gsfd = -1 ;		/* file pointer for driver */
static int ps2memfd = -1 ;	/* file pointer for driver */
static GSOSbit64 *ps2buf = NULL ; /* DMA buffer */
GSOSbit64 *ps2dbp[2] ;		/* double buffered DMA base pointer */
int ps2dbf = 0 ;		/* double DMA buffer flag */
int gsos_maxpacket_size;

#ifdef GSOS_SPR_IMAGEBUFFER
static int ps2sprfd;		/* ScratchPad fd */
GSOSbit64 *ps2sprp;		/* ScratchPad */
GSOSbit64 *ps2dbp_spr[2] ;	/* double buffered IMAGE base pointer */
int ps2dbf_spr = 0 ;		/* double IMAGE buffer flag */
int gsos_maximage_size;
#endif

GSOSbit64 *ps2cur = NULL ;	/* current pointer of DMA buffer */
int ps2count = 0 ;		/* counter for DMA buffer */

int ps2dma_limit = 0;

static struct ps2_screeninfo consoleinfo;
static struct ps2_screeninfo graphicsinfo;
static unsigned int g_fbmask;
static int g_odd_even_mix, g_dx, g_dy;

static void gsosDoSetScreen(void);
static void gsosClearFrameBuffer(int w, int h, int mw, int mh);

int gsosOpen(void)
{
    return 0;
}

int gsosClose(void)
{
    return gsosRelease();
}

int gsosAcquire(void)
{
    if (ps2gsfd >= 0)
	return 0;

    gsos_maxpacket_size = GSOS_PS2MEM_SIZE / 2;

    /* open driver */
    ps2gsfd = open( PS2_DEV_GS, O_RDWR ) ;
    if( ps2gsfd < 0 ){
	fprintf (stderr,"gsosOpen: can't open driver(" PS2_DEV_GS ")\n" ) ;
        return -1 ;
    }

    /* open driver */
    ps2memfd = open( PS2_DEV_MEM, O_RDWR ) ;
    if( ps2memfd < 0 ){
	fprintf (stderr,"gsosOpen: can't open driver(" PS2_DEV_MEM ")\n" ) ;
        return -1 ;
    }

    /* memory allocation for DMA buffer */
    if( ps2buf == NULL ){
        ps2buf = (GSOSbit64 *)mmap( NULL, GSOS_PS2MEM_SIZE,
            PROT_READ | PROT_WRITE, MAP_SHARED, ps2memfd, 0 ) ;
        if( ps2buf == NULL ){
	    fprintf (stderr,"gsosOpen: memory allocateion error\n");
            close(ps2gsfd) ;
            close(ps2memfd) ;
            return -1 ;
        }
    }
#ifdef PS2IOC_SENDLIMIT
    if( ioctl( ps2gsfd, PS2IOC_SENDLIMIT, 1 ) < 0 )
	ps2dma_limit = 0;
    else
	ps2dma_limit = 1;	/* PS2IOC_SENDLIMIT is available */
#endif
#ifdef GSOS_SPR_IMAGEBUFFER

    gsos_maximage_size = GSOS_SCRATCHPAD_SIZE / 2;

    /* open ScratchPad driver */
    ps2sprfd = open( PS2_DEV_SPR, O_RDWR ) ;
    if( ps2sprfd < 0 ){
	fprintf (stderr,"gsosOpen: can't open driver(" PS2_DEV_SPR ")\n" ) ;
        return -1 ;
    }

    /* memory allocation for ScratchPad Memory */
    if( ps2sprp == NULL ){
        ps2sprp = (GSOSbit64 *)mmap( NULL, 16384,
            PROT_READ | PROT_WRITE, MAP_SHARED, ps2sprfd, 0 ) ;
        if( ps2sprp == NULL ){
	    fprintf (stderr,"gsosOpen: ScratchPad memory allocateion error\n");
	    if(ps2buf != NULL) {
	      munmap((void *)ps2buf, GSOS_PS2MEM_SIZE);
	    }
	    close(ps2sprfd);
            close(ps2gsfd) ;
            close(ps2memfd) ;
            return -1 ;
        }
    }
    ps2dbp_spr[0] = ps2sprp;
    ps2dbp_spr[1] = (GSOSbit64 *)((unsigned long)ps2sprp + gsos_maximage_size);
    ps2dbf_spr = 0;

#endif
    ps2dbp[0] = ps2buf;
    ps2dbp[1] = (GSOSbit64 *)((unsigned long)ps2buf + gsos_maxpacket_size);
    ps2dbf = 0;
    ps2count = 0 ;
    ps2cur = ps2dbp[ps2dbf];
    ps2lastgiftag = NULL;

    ioctl( ps2gsfd, PS2IOC_GSCREENINFO, &consoleinfo ) ;
    gsosDoSetScreen();

    return 0;
}

int gsosRelease(void)
{
    if (ps2gsfd < 0)
	return 0;

    // wait until idle
    ioctl( ps2gsfd, PS2IOC_SENDQCT, 1);

    munmap( (void *)ps2buf, GSOS_PS2MEM_SIZE ) ;
    ps2buf = NULL;
#ifdef GSOS_SPR_IMAGEBUFFER
    munmap( (void *)ps2sprp, GSOS_SCRATCHPAD_SIZE ) ;
    ps2sprp = NULL;
#endif
    if( ps2gsfd >= 0 ){
        ioctl( ps2gsfd, PS2IOC_SSCREENINFO, &consoleinfo ) ;
        close( ps2gsfd ) ;
        ps2gsfd = -1 ;
    }
    if( ps2memfd >= 0 ){
        close( ps2memfd ) ;
        ps2memfd = -1 ;
    }
#ifdef GSOS_SPR_IMAGEBUFFER
    if( ps2sprfd >= 0 ){
        close( ps2sprfd ) ;
        ps2sprfd = -1 ;
    }
#endif
    return 0 ;
}

void gsosSetScreen( int mode, int res, int w, int h, int fbp, int psm,
			unsigned int fbmask, int odd_even_mix, int dx, int dy)
{
    graphicsinfo.mode = mode;
    graphicsinfo.res = res;
    graphicsinfo.w = w;
    graphicsinfo.h = h;
    graphicsinfo.fbp = fbp;
    graphicsinfo.psm = psm;

    g_fbmask = fbmask;
    g_odd_even_mix = odd_even_mix;
    g_dx = dx;
    g_dy = dy;
}

static void gsosDoSetScreen(void)
{
    int interlace;
    struct ps2_dispfb ch1;
    struct ps2_display d1;
    struct ps2_pmode pmode;

    ioctl(ps2gsfd, PS2IOC_SSCREENINFO, &graphicsinfo);

    interlace = (((graphicsinfo.mode == PS2_GS_NTSC || graphicsinfo.mode == PS2_GS_PAL) &&
		  (graphicsinfo.res & PS2_GS_INTERLACE) )||
		 (graphicsinfo.mode == PS2_GS_DTV && graphicsinfo.res == PS2_GS_1080I));

    if (interlace && g_odd_even_mix != 0xff && g_odd_even_mix != 0) {
	ch1.ch=0;
	ioctl(ps2gsfd, PS2IOC_GDISPFB, &ch1);
	ch1.ch = 1;
	ioctl(ps2gsfd, PS2IOC_SDISPFB, &ch1);

	d1.ch = 0;
	ioctl(ps2gsfd, PS2IOC_GDISPLAY, &d1);
	d1.ch = 1;
	d1.dy += 1;
	ioctl(ps2gsfd, PS2IOC_SDISPLAY, &d1);

	ioctl(ps2gsfd, PS2IOC_GPMODE, &pmode);
	pmode.sw = 3;
	pmode.alp = g_odd_even_mix;
	pmode.amod = 0;
	pmode.mmod = 1; // use ALP as odd_even_mix
	pmode.slbg = 0; // use OUT2
	ioctl(ps2gsfd, PS2IOC_SPMODE, &pmode);
    }

    gsosFrameInit((graphicsinfo.w + g_dx + 63) / 64,
		  graphicsinfo.psm, graphicsinfo.fbp, g_fbmask);
    gsosClearFrameBuffer(graphicsinfo.w, graphicsinfo.h, g_dx, g_dy);
}


int gsosReadImage( 
  int x, int y, int w, int h,
  unsigned int bp,
  int bw, int psm,
  GSOSuchar *pPix )
{
    struct ps2_image img;
    size_t size;
    int nlines;
    int maxlines;
    int txfline;
    int linebytewidth;

    if (ps2gsfd < 0)
	return -1;

    gsosKick();

#ifdef GSOS_SPR_IMAGEBUFFER

    
    // wait until idle
    ioctl( ps2gsfd, PS2IOC_SENDQCT, 1);
    ps2dbf_spr = 0;


    linebytewidth =  w * gsosBytePerPixel(psm);
    maxlines = GSOS_SCRATCHPAD_SIZE / linebytewidth;
    img.ptr = ps2sprp;
    img.fbp = bp ;
    img.fbw = bw ;
    img.psm = psm ;
    img.x = x ;
    img.w = w ;
    nlines = h;
    while(nlines > 0) {
      txfline = (maxlines < nlines) ? maxlines : nlines;
      img.y = y ;
      img.h = txfline ;
      if (ioctl(ps2gsfd, PS2IOC_STOREIMAGE, &img) < 0)
	return -1;
   
      size = txfline * linebytewidth;
      memcpy((void *)pPix, (void *)ps2sprp, size) ;
      y += txfline;
      nlines -= txfline;
      pPix += size;
    }
#else*/
    linebytewidth =  w * gsosBytePerPixel(psm);
    maxlines = gsos_maxpacket_size / linebytewidth;
    img.ptr = ps2dbp[ps2dbf] ;
    img.fbp = bp ;
    img.fbw = bw ;
    img.psm = psm ;
    img.x = x ;
    img.w = w ;
    nlines = h;
    while(nlines > 0) {
      txfline = (maxlines < nlines) ? maxlines : nlines;
      img.y = y ;
      img.h = txfline ;
      if (ioctl(ps2gsfd, PS2IOC_STOREIMAGE, &img) < 0)
	return -1;
   
      size = txfline * linebytewidth;
      memcpy((void *)pPix, (void *)ps2dbp[ps2dbf], size) ;
      y += txfline;
      nlines -= txfline;
      pPix += size;
    }
#endif
    return 0 ;
}

int gsosWriteImage( 
  int x, int y, int w, int h,
  unsigned int bp,
  int bw, int psm,
  GSOSuchar *pPix )
{
    size_t size;
    int nlines;
    int maxlines;
    int txfline;
    int linebytewidth;
    int wqc;
    struct ps2_image img;
    struct ps2_image *imgp = &img;;

    if (ps2gsfd < 0)
	return -1;

    gsosKick();

#ifdef GSOS_SPR_IMAGEBUFFER
    imgp->ptr = ps2dbp_spr[ps2dbf_spr];
#else
    imgp->ptr = ps2dbp[ps2dbf];
#endif
    imgp->fbp = bp;
    imgp->fbw = bw;
    imgp->psm = psm;
    imgp->x = x;
    imgp->w = w;
    nlines = h;
    linebytewidth =  w * gsosBytePerPixel(psm);
#ifdef GSOS_SPR_IMAGEBUFFER
    maxlines = gsos_maximage_size / linebytewidth;
#else
    maxlines = gsos_maxpacket_size / linebytewidth;
#endif

    while(nlines > 0) {
      txfline = (maxlines < nlines) ? maxlines : nlines;
      size = txfline * linebytewidth;
      memcpy((void *)imgp->ptr, (void *)pPix, size);

      imgp->y = y ;
      imgp->h = txfline ;

      y += txfline;
      nlines -= txfline;
      pPix += size;

#ifdef GSOS_LAZY_EXEC
      wqc = 2;
#else
      wqc = (nlines > 0) ? 2 : 1;
#endif

      if(ioctl(ps2gsfd, PS2IOC_LOADIMAGEA, &img) < 0)
	return -1;
#ifdef GSOS_LAZY_EXEC
      if(!ps2dma_limit)
#else
      if(!ps2dma_limit || wqc != 2)
#endif
        ioctl( ps2gsfd, PS2IOC_SENDQCT, wqc);

#ifdef GSOS_SPR_IMAGEBUFFER
      ps2dbf_spr = 1 - ps2dbf_spr;
      imgp->ptr = ps2dbp_spr[ps2dbf_spr];
#else
      ps2dbf = 1 - ps2dbf;
      ps2cur = ps2dbp[ps2dbf]; 
      imgp->ptr = ps2dbp[ps2dbf];
#endif

    }
    return 0 ;
}

static void gsosClearFrameBuffer(int w, int h, int mw, int mh)
{

    unsigned int r,g,b,a;

    gsosMakeGiftag(2, 
    		GSOS_GIF_PRE_IGNORE, 0, 
		GSOS_GIF_FLG_PACKED, 1, 
		GSOS_GIF_REG_AD);

    /* set SCISSOR rect */
    gsosSetPacketAddrData4(GSOS_SCISSOR_1,
                              (GSOSbit64)0, (GSOSbit64)(w+mw),
                              (GSOSbit64)0, (GSOSbit64)(h+mh));
    gsosSetPacketAddrData(GSOS_PRIM, GSOS_PRIM_SPRITE);

    // Clear (0,0) - (w+mw, h+mh)
    r=g=0; b=0; a=0;
    gsosMakeGiftag(2, 
                     GSOS_GIF_PRE_IGNORE, 0,
                     GSOS_GIF_FLG_PACKED, 2,
                     (GSOS_GIF_REG_XYZ2  << 4) |
                     (GSOS_GIF_REG_RGBAQ << 0));
    gsosSetPacket4( r, g, b, a ) ;
    gsosSetPacket4(GSOS_SUBPIX_OFST(0),
                     GSOS_SUBPIX_OFST(0),0,0);

    gsosSetPacket4( r, g, b, a ) ;
    gsosSetPacket4(GSOS_SUBPIX_OFST(w+mw),
                     GSOS_SUBPIX_OFST(h+mh),0,0);

    // Fill (0,0) - (w, h) with light blue
    r=g=80; b=130; a=0;
    gsosMakeGiftag(2, 
                     GSOS_GIF_PRE_IGNORE, 0,
                     GSOS_GIF_FLG_PACKED, 2,
                     (GSOS_GIF_REG_XYZ2  << 4) |
                     (GSOS_GIF_REG_RGBAQ << 0));
    gsosSetPacket4( r, g, b, a ) ;
    gsosSetPacket4(GSOS_SUBPIX_OFST(0),
                     GSOS_SUBPIX_OFST(0),0,0);

    gsosSetPacket4( r, g, b, a ) ;
    gsosSetPacket4(GSOS_SUBPIX_OFST(w),
                     GSOS_SUBPIX_OFST(h),0,0);

    gsosKick();
}

void gsosFrameInit(int fbw, int psm, int fbp, unsigned int fbmask)
{
    int          result;
    GSOSbit64    frame;

    if (ps2gsfd < 0)
	return;

    result = gsosMakeGiftag(2, 
		GSOS_GIF_PRE_IGNORE, 0, 
		GSOS_GIF_FLG_PACKED, 1,
		GSOS_GIF_REG_AD );
    /* Frame */
    frame = GsosFrameData(fbp, fbw, psm, fbmask);
    gsosSetPacketAddrData(GSOS_FRAME_1, frame);
    /* XYOFFSET */
    gsosSetPacketAddrData(GSOS_XYOFFSET_1,
			  GsosXyoffsetData(GSOS_XYOFFSET<<4,GSOS_XYOFFSET<<4));
    gsosExec() ;
}

void gsosDoKick()
{
    gsosKick();
}

