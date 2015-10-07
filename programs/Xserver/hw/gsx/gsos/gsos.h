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

#ifndef GSOS_H
#define GSOS_H

/*#ifndef __R5900
#error "not supported on this machine"
#endif*/

#ifndef __R5900
#define __R5900
#endif

extern void ErrorF( char *, ... );

#include "gsostypes.h"
#include "gsosregs.h"
#include <sys/ioctl.h>
#include <linux/ps2/dev.h>

/* Flags for optimizing */
#define GSOS_LAZY_EXEC
#define GSOS_SPR_IMAGEBUFFER

/* System const */
#define GSOS_SCRATCHPAD_SIZE   (16*1024)

/* GIF params */
#define GSOS_GIFTAG_SIZE       (128/8)
#define GSOS_GIF_EOP_CONTINUE  0
#define GSOS_GIF_EOP_TERMINATE 1
#define GSOS_GIF_PRE_IGNORE    0
#define GSOS_GIF_PRE_ENABLE    1
#define GSOS_GIF_PRI_IIP       (1<<3)
#define GSOS_GIF_PRI_TME       (1<<4)
#define GSOS_GIF_PRI_FGE       (1<<5)
#define GSOS_GIF_PRI_ABE       (1<<6)
#define GSOS_GIF_PRI_AA1       (1<<7)
#define GSOS_GIF_PRI_FST       (1<<8)
#define GSOS_GIF_PRI_CTXT      (1<<9)
#define GSOS_GIF_PRI_FIX       (1<<10)
#define GSOS_GIF_FLG_PACKED    0
#define GSOS_GIF_FLG_REGLIST   1
#define GSOS_GIF_FLG_IMAGE     2

/* GIF REG specifier */
#define GSOS_GIF_REG_PRIM      PS2_GIFTAG_REGS_PRIM
#define GSOS_GIF_REG_RGBAQ     PS2_GIFTAG_REGS_RGBAQ
#define GSOS_GIF_REG_ST        PS2_GIFTAG_REGS_ST
#define GSOS_GIF_REG_UV        PS2_GIFTAG_REGS_UV
#define GSOS_GIF_REG_XYZF2     PS2_GIFTAG_REGS_XYZF2
#define GSOS_GIF_REG_XYZ2      PS2_GIFTAG_REGS_XYZ2
#define GSOS_GIF_REG_TEX0_1    PS2_GIFTAG_REGS_TEX0_1
#define GSOS_GIF_REG_TEX0_2    PS2_GIFTAG_REGS_TEX0_2
#define GSOS_GIF_REG_CLAMP_1   PS2_GIFTAG_REGS_CLAMP_1
#define GSOS_GIF_REG_CLAMP_2   PS2_GIFTAG_REGS_CLAMP_2
#define GSOS_GIF_REG_FOG       PS2_GIFTAG_REGS_FOG
#define GSOS_GIF_REG_XYZF3     PS2_GIFTAG_REGS_XYZF3
#define GSOS_GIF_REG_XYZ3      PS2_GIFTAG_REGS_XYZ
#define GSOS_GIF_REG_AD        PS2_GIFTAG_REGS_AD
#define GSOS_GIF_REG_NOF       PS2_GIFTAG_REGS_NOP

/* PRIM register */
#define GSOS_PRIM_POINT    PS2_GS_PRIM_PRIM_POINT
#define GSOS_PRIM_LINE     PS2_GS_PRIM_PRIM_LINE
#define GSOS_PRIM_LSTRIP   PS2_GS_PRIM_PRIM_LINESTRIP
#define GSOS_PRIM_TRIANGLE PS2_GS_PRIM_PRIM_TRIANGLE
#define GSOS_PRIM_TSTRIP   PS2_GS_PRIM_PRIM_TRISTRIP
#define GSOS_PRIM_TFAN     PS2_GS_PRIM_PRIM_TRIFAN
#define GSOS_PRIM_SPRITE   PS2_GS_PRIM_PRIM_SPRITE


/* GIF Tag (EE User's Man. P139) */
#define EOP_SHIFT	PS2_GIFTAG_EOP_THOFFSET
#define EOP_MASK	(1 << EOP_SHIFT)

#define PRE_SHIFT	PS2_GIFTAG_PRE_THOFFSET
#define PRE_MASK	(1 << PRE_SHIFT)
#define PRE_SHIFT_STR	"46"

#define PRIM_SHIFT	PS2_GIFTAG_PRIM_THOFFSET
#define PRIM_MASK	(0x3FF << PRIM_SHIFT)
#define PRIM_SHIFT_STR	"47"

#define FLG_SHIFT	PS2_GIFTAG_FLG_THOFFSET
#define FLG_MASK	(3 << FLG_SHIFT)
#define FLG_SHIFT_STR	"58"

#define NREG_SHIFT	PS2_GIFTAG_NREG_THOFFSET
#define NREG_MASK	(3 <<NREG_SHIFT)
#define NREG_SHIFT_STR	"60"


/* externs */
int gsosOpen(void);
int gsosClose(void);
int gsosAcquire(void);
int gsosRelease(void);
void gsosSetScreen(int mode, int res, int w, int h, 
	int fbp, int psm, unsigned int fbmask, int odd_even_mix, int dx, int dy);
int gsosReadImage(int x, int y, int w, int h, 
	unsigned int bp, int bw, int psm, GSOSuchar *pPix );
int gsosWriteImage(int x, int y, int w, int h, 
	unsigned int bp, int bw, int psm, GSOSuchar *pPix );
void gsosFrameInit(int fbw, int psm, int fbp, unsigned int fbmask);
void gsosDoKick(void);

extern int gsos_maxpacket_size;
extern GSOSbit64 *ps2dbp[2]; /* double buffered DMA base pointer */
extern GSOSbit64 *ps2cur;    /* current pointer of DMA buffer */
extern int ps2dbf;           /* double buffer flag */
extern int ps2count;         /* counter for DMA buffer */
extern int ps2gsfd ;         /* file pointer for driver */
extern GSOSbit64 *ps2lastgiftag;  /* last giftag ptr */

#ifdef GSOS_SPR_IMAGEBUFFER
extern GSOSbit64 *ps2sprp;	/* ScratchPad */
extern GSOSbit64 *ps2dbp_spr[2];/* double buffered IMAGE base pointer */
extern int ps2dbf_spr;		/* double IMAGE buffer flag */
extern int gsos_maximage_size;
#endif

extern int ps2dma_limit;

/*
 * static inline
 * void gsosSetEOPTerminate(void)
 */
#ifdef __MIPSEL__
#define gsosSetEOPTerminate() \
do	\
{	\
    if (ps2lastgiftag) {		\
        (* (unsigned int *) ps2lastgiftag) |= (1 << EOP_SHIFT); \
        ps2lastgiftag = 0; \
    }	\
} while (0)
#else
#define gsosSetEOPTerminate() \
do	\
{	\
  if (ps2lastgiftag) {		\
    __asm__ __volatile__(	\
        ".set push\n"	\
        "       .set    arch=r5900\n"	\
        "       lq	$8, (%0)\n"	\
	"	or	$8, $8, %1\n"	\
        "       sq      $8, (%0)\n"	\
        "       .set    pop"	\
        : /*no output*/\
	: "r"(ps2lastgiftag), "i"(1 <<EOP_SHIFT)	\
        : "$8" ); 	\
    ps2lastgiftag = 0;\
  }	\
} while (0)
#endif

extern const int __gsos_datatype_size[];

#define gsosCalcPrimSize(nreg, flag) \
  (((nreg)==0 ? 16 : (nreg)) * __gsos_datatype_size[flag & 3])


#define gsosCalcPacketSize(nloop, nreg, flag) \
  ((nloop) * gsosCalcPrimSize(nreg, flag))


static
inline
int gsosRemainBufSize()
{
  int n;
  n = (int)((char *)ps2cur - (char *)ps2dbp[ps2dbf]);
  if(gsos_maxpacket_size < n) {
    ErrorF("gsosRemainBufSize:n=%d,ps2cur=%x,ps2buf=%x\n", 
    		n, ps2cur, ps2dbp[ps2dbf]);
    return 0;
  }
  return gsos_maxpacket_size - n;
}

static
inline
int gsosCalcPacketCount(int nreg, int flag)
{
  int packetSize;
  int bufferSize;
  packetSize = gsosCalcPrimSize(nreg, flag);
  bufferSize = gsosRemainBufSize() - GSOS_GIFTAG_SIZE;
  if(bufferSize <= 0) {
    return 0;	/* can't allocate GIFTag */
  }
  return (bufferSize > packetSize) ? (bufferSize / packetSize) : 0;
}

#define gsosCalcMaxPacketCount(nreg, flag) \
  ((gsos_maxpacket_size - GSOS_GIFTAG_SIZE)/gsosCalcPrimSize(nreg, flag))


/*
 * static inline 
 * void gsosStoreQword(GSOSbit128 *addr, GSOSbit128 val)
 */
#define gsosStoreQword(addr, val) \
do	\
{	\
    GSOSQword src;	\
	\
    src.ti = val;	\
    __asm__ __volatile__(	\
        ".set push\n"		\
        "       .set    arch=r5900\n"	\
        "       pextlw  $8, %1, %0\n"	\
        "       pextlw  $9, %3, %2\n"	\
        "       pcpyld  $8, $9, $8\n"	\
        "       sq      $8, (%4)\n"	\
        "       .set    pop"	\
        : /* no output */	\
        : "r"(src.si.lo0), "r"(src.si.lo1),	\
           "r"(src.si.hi0), "r"(src.si.hi1),	\
           "r" (addr),		\
	: "$8", "$9"); \
} \
while(0)


/*
 * static inline
 * void gsosSetPacket2(
 *   register GSOSbit64 d1, 
 *   register GSOSbit64 d2)
 */

#define gsosSetPacket2(d1, d2) \
do	\
{	\
    GSOSDword cnv1, cnv2;	\
    	\
    cnv1.di=d1;	\
    cnv2.di=d2;	\
    ps2cur += 2;	\
    ps2count += 2;	\
    __asm__ __volatile__(	\
        ".set push\n"	\
        "       .set    arch=r5900\n"	\
        "       pextlw  $8, %1, %0\n"	\
        "       pextlw  $9, %3, %2\n"	\
        "       pcpyld  $8, $9, $8\n"	\
        "       sq      $8, %5(%4)\n"	\
        "       .set    pop"	\
        : /* no output */	\
        : "r"(cnv1.si.lo), "r"(cnv1.si.hi),	\
           "r"(cnv2.si.lo), "r"(cnv2.si.hi),	\
           "r" (ps2cur),		\
           "i" (-sizeof(*ps2cur)*2)	\
	: "$8", "$9");	\
}	\
while (0)


#define gsosSetPacketAddrData(addr, data) gsosSetPacket2(data, addr)
#define gsosSetGifTag(tag, regs) gsosSetPacket2(tag, regs)

/*
 * static inline
 * void gsosSetPacket4(
 *   unsigned int d1, unsigned int d2, 
 *   unsigned int d3, unsigned int d4)
 */
#define gsosSetPacket4( d1, d2, d3, d4) \
do	\
{	\
    	\
    ps2cur += 2;	\
    ps2count += 2;	\
    __asm__ __volatile__(	\
        ".set push\n" \
        "       .set    arch=r5900\n"	\
        "       pextlw  $8, %1, %0\n"	\
        "       pextlw  $9, %3, %2\n"	\
        "       pcpyld  $8, $9, $8\n"	\
        "       sq      $8, %5(%4)\n"	\
        "       .set    pop"	\
        :  /* no output */ \
        :  "r"((unsigned int)(d1)), "r"((unsigned int)(d2)),	\
           "r"((unsigned int)(d3)), "r"((unsigned int)(d4)),	\
	   "r"(ps2cur),			\
           "i" (-sizeof(*ps2cur)*2)	\
	: "$8", "$9");	\
}	\
while (0)



/*
 * static inline
 * void gsosSetPacketAddrData4(
 *     unsigned int addr, 
 *     register unsigned short d1, 
 *     register unsigned short d2, 
 *     register unsigned short d3, 
 *     register unsigned short d4)
 */

#define  gsosSetPacketAddrData4(addr, d1, d2, d3, d4) \
do 	\
{	\
    	\
    ps2cur += 2;	\
    ps2count += 2;	\
    __asm__ __volatile__(	\
        ".set push\n"	\
        "       .set    arch=r5900\n"	\
        "	pextlh	$8, %1, %0\n"	\
        "	pextlh	$9, %3, %2\n"	\
        "	pextlw	$8, $9, $8\n"	\
        "       pextlw  $9, $0, %4\n"	\
        "       pcpyld  $8, $9, $8\n"	\
        "       sq      $8, %6(%5)\n"	\
        "       .set    pop"	\
        : /* no output */	\
        : "r"((unsigned short)(d1)), "r"((unsigned short)(d2)),	\
          "r"((unsigned short)(d3)), "r"((unsigned short)(d4)),	\
          "r"((unsigned int)(addr)),	\
          "r" (ps2cur),			\
          "i" (-sizeof(*ps2cur)*2)	\
	: "$8", "$9");	\
	\
}	\
while (0)

/*
 * static inline
 * void gsosSetXyPackedXYZ2_SUBPIX_SHIFT_OFF(
 *   unsigned int x, unsigned int y,
 *   unsigned const int shift,
 *   unsigned const int off)
 */
#define gsosSetXyPackedXYZ2_SUBPIX_SHIFT_OFF(x, y, shift, off)	\
do	\
{	\
    __asm__ __volatile__(	\
        ".set push\n" \
        "       .set    arch=r5900\n"	\
        "       pextlw  $8, %1, %0\n"	\
        "	dsll    $8, %4\n"	\
        "       pcpyld  $8, $0, $8\n"	\
        "       sq      $8, %3(%2)\n"	\
        "       .set    pop"	\
        : /* no output */	\
        :  "r"((unsigned short)(x)), "r"((unsigned short)(y)),	\
           "r" (ps2cur),		\
           "i" (sizeof(*ps2cur)*off*2),	\
           "i" (shift)	\
	: "$8");	\
}	\
while (0)

/*
 * static inline
 * void gosForwardPacket(unsigned const int n)
 */
#define gosForwardPacket(n) \
do	\
{	\
    ps2cur += 2*n;	\
    ps2count += 2*n;	\
}	\
while (0) 

/*
 * static inline
 * void gsosSetXyPackedXYZ2(
 *   unsigned short x, unsigned short y)
 */
#define gsosSetXyPackedXYZ2(x, y)	\
do	\
{	\
    ps2cur += 2;	\
    ps2count += 2;	\
    __asm__ __volatile__(	\
        ".set push\n" \
        "       .set    arch=r5900\n"	\
        "       pextlw  $8, %1, %0\n"	\
        "       pcpyld  $8, $0, $8\n"	\
        "       sq      $8, %3(%2)\n"	\
        "       .set    pop"	\
        : /* no output */	\
        :  "r"((unsigned short)(x)), "r"((unsigned short)(y)),	\
           "r" (ps2cur),		\
           "i" (-sizeof(*ps2cur)*2)	\
	: "$8");	\
}	\
while (0)



#ifdef GSOS_LAZY_EXEC
#define gsosExec()	do { ; } while(0)
#else
#define gsosExec()	gsosKick()
#endif

/*
 * static inline void gsosKick()
 */

#define gsosKick()	\
do	\
{	\
    struct ps2_packet pkt ;	\
	\
    if( ps2count ){	\
        gsosSetEOPTerminate();	\
        pkt.ptr = ps2dbp[ps2dbf] ;	\
        pkt.len = (int)((char *)ps2cur - (char *)ps2dbp[ps2dbf]) ;	\
        ioctl( ps2gsfd, PS2IOC_SENDA, &pkt ) ;	\
	if( !ps2dma_limit ) \
	    ioctl( ps2gsfd, PS2IOC_SENDQCT, 2 ) ;	\
        ps2dbf = 1 - ps2dbf ;	\
        ps2cur = ps2dbp[ps2dbf] ;	\
        ps2count = 0 ;	\
    }	\
}	\
while (0)

static 
inline
int gsosMakeGiftag(
	unsigned int nloop,		/* repeat num */
	unsigned int pre,		/* prim field enable */
	unsigned int prim,		/* prim date set to PRIM register */
	unsigned int flg,		/* 0:PACKED 1:REGLIST 2:IMAGE */
	unsigned int nreg,		/* number of regs(MAX=16) */
	GSOSbit64 regs)	/* register description */
{
  int ps;
  int rs;

  GSOSDword cnv;

  /* Now gsosSetEOPTerminate() sets eop, so ignore eop param here */

  ps = gsosCalcPacketSize(nloop, nreg, flg) + GSOS_GIFTAG_SIZE;
  rs = gsosRemainBufSize();
  if(rs < ps) {	
    gsosKick();
    rs = gsos_maxpacket_size;
  }
  if(rs < ps) {
    /* Packet too large */
    ErrorF("Packet too large\n");
    return 1;
  }
  ps2lastgiftag = ps2cur;	/* save giftag ptr */
  ps2cur += 2;
  ps2count += 2;

  cnv.di =  regs;
  __asm__ __volatile__(	
        ".set push\n" 
        "	.set arch=r5900\n" 
	"	dsll	$8, %1, " PRE_SHIFT_STR "	# pre\n"
	"			#\n"
	"	or	$9, $8, %0	# nloop\n"
	"			#\n"
	"	dsll	$8, %2, " PRIM_SHIFT_STR "	# prim\n"
	"	or	$9, $8	#\n"
	"			#\n"
	"	dsll	$8, %3, " FLG_SHIFT_STR "	# flg\n"
	"	or	$9, $8	#\n"
	"			#\n"
	"	dsll	$8, %4, " NREG_SHIFT_STR "	# nreg\n"
	"	or	$8, $9	#\n"
	"			#\n"
        "       pextlw  $9, %6, %5\n"
        "	pcpyld  $8, $9, $8\n"
	"			#\n"
        "	sq      $8, %8(%7)\n"
	"			#\n"
        "	.set pop" 

        : /* no output */
        :  "r"((unsigned int)(nloop)), 

	   "r"((unsigned int)(pre)), 
	   "r"((unsigned int)(prim)),
           "r"((unsigned int)(flg)),
	   "r"((unsigned int)(nreg)),

	   "r"((unsigned int)(cnv.si.lo)), 
	   "r"((unsigned int)(cnv.si.hi)),
           "r" (ps2cur),
           "i" (-sizeof(*ps2cur)*2)
	: "$8", "$9");

  return 0 ;
}


#define gsosBytePerPixel(psm) (((psm) == PS2_GS_PSMCT16) ? 2 : 4)

#endif /* GSOS_H */
