/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/s3_savage/s3sav_cursor.c,v 1.1.2.2 1999/12/01 12:49:33 hohndel Exp $ */

/*
 *
 * Copyright 1991 MIPS Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of MIPS not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  MIPS makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * MIPS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL MIPS
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Modified by Amancio Hasty and Jon Tombs
 *
 */

/*
 * Hardware cursor support for S3 ViRGE SVGA driver. Taken with
 * very few changes from the accel/s3_virge cursor file.
 * S. Marineau, 19/04/97.
 */


#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "input.h"
#include "cursorstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "windowstr.h"

#include "compiler.h"
#include "xf86.h"
#include "xf86_OSlib.h"
#include "mipointer.h"
#include "inputstr.h"
#include "xf86Priv.h"
#include "xf86_Option.h"
#include "vga256.h"
#include "vga.h"
#include "xf86xaa.h"
#include "regs3sav.h"
#include "s3sav_driver.h"

static Bool S3SAVRealizeCursor();
static Bool S3SAVUnrealizeCursor();
static void S3SAVSetCursor();
static void S3SAVMoveCursor();
static void S3SAVRecolorCursor();

static miPointerSpriteFuncRec s3vPointerSpriteFuncs =
{
   S3SAVRealizeCursor,
   S3SAVUnrealizeCursor,
   S3SAVSetCursor,
   S3SAVMoveCursor,
};

extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern xf86InfoRec xf86Info;

/* For byte swapping, use the XAA array */
extern unsigned char byte_reversed[256];
extern S3VPRIV s3vPriv;
extern pointer s3savMmioMem;
extern int vgaCRIndex, vgaCRReg;

static int s3vCursGeneration = -1;
static int s3vHotX;
static int s3vHotY;
static int s3vCursorVRAMMemSegment;
static CursorPtr s3vCursorpCurs;

#define MAX_CURS 64

Bool
S3SAVCursorInit(pm, pScr)
     char *pm;
     ScreenPtr pScr;
{

   if (s3vCursGeneration != serverGeneration) {
      if (!(miPointerInitialize(pScr, &s3vPointerSpriteFuncs,
	   &xf86PointerScreenFuncs, FALSE)))
               return FALSE;

      s3vHotX = 0;
      s3vHotY = 0;
      pScr->RecolorCursor = S3SAVRecolorCursor;
      s3vCursGeneration = serverGeneration;
   }

   /* 
    * We place the cursor in high memory, just before the command overflow
    * buffer.  Savage4 requires 4k alignment for the cursor image.
    */

   if( s3vPriv.chip < S3_SAVAGE2000) {
      s3vCursorVRAMMemSegment = (S3_IN32(0x48C14) & 0x3FFF) * 2 - 4;
   } else {
      s3vCursorVRAMMemSegment = ((S3_IN32(0x48C18) >> 4) & 0x3FFF) * 2 - 4;
   }

   if( !s3vCursorVRAMMemSegment )
      s3vCursorVRAMMemSegment = vga256InfoRec.videoRam - 1;

   return TRUE;
}

/* This allows the cursor to be displayed */

void
S3SAVShowCursor()
{
   unsigned char tmp;

   /* turn cursor on */
   outb(vgaCRIndex, 0x45);
   tmp = inb(vgaCRReg);
   outb(vgaCRReg, tmp | 0x01);
}

void
S3SAVHideCursor()
{
   unsigned char tmp;

   /* turn cursor off */
   outb(vgaCRIndex, 0x45);
   tmp = inb(vgaCRReg);
   outb(vgaCRReg, tmp & 0xFE);
}

static Bool
S3SAVRealizeCursor(pScr, pCurs)
     ScreenPtr pScr;
     CursorPtr pCurs;

{
   register int i, j;
   unsigned short *pServMsk;
   unsigned short *pServSrc;
   int   index = pScr->myNum;
   pointer *pPriv = &pCurs->bits->devPriv[index];
   int   wsrc, h;
   unsigned short *ram;
   CursorBitsPtr bits = pCurs->bits;

   if (pCurs->bits->refcnt > 1)
      return TRUE;

   ram = (unsigned short *)xalloc(1024);
   *pPriv = (pointer) ram;

   if (!ram)
      return FALSE;

   pServSrc = (unsigned short *)bits->source;
   pServMsk = (unsigned short *)bits->mask;

   h = bits->height;
   if (h > MAX_CURS)
      h = MAX_CURS;

   wsrc = PixmapBytePad(bits->width, 1);	/* words per line */

   for (i = 0; i < MAX_CURS; i++) {
      for (j = 0; j < MAX_CURS / 16; j++) {
	 unsigned short mask, source;

	 if (i < h && j < wsrc / 2) {
	    mask = *pServMsk++;
	    source = *pServSrc++;

	    ((char *)&mask)[0] = byte_reversed[((unsigned char *)&mask)[0]];
	    ((char *)&mask)[1] = byte_reversed[((unsigned char *)&mask)[1]];

	    ((char *)&source)[0] = byte_reversed[((unsigned char *)&source)[0]];
	    ((char *)&source)[1] = byte_reversed[((unsigned char *)&source)[1]];

	    if (j < MAX_CURS / 8) { /* j < MAX_CURS / 16 implies this */
	       *ram++ = ~mask;
	       *ram++ = source & mask;
	    }
	 } else {
	    *ram++ = 0xffff;
	    *ram++ = 0x0;
	 }
      }
      if (j < wsrc / 2) {
	 pServMsk += (wsrc/2 - j);
	 pServSrc += (wsrc/2 - j);
      }
   }
   return TRUE;
}

static Bool
S3SAVUnrealizeCursor(pScr, pCurs)
     ScreenPtr pScr;
     CursorPtr pCurs;
{
   pointer priv;


   if (pCurs->bits->refcnt <= 1 &&
     (priv = pCurs->bits->devPriv[pScr->myNum])) {
         xfree(priv);
         pCurs->bits->devPriv[pScr->myNum] = 0x0;
   }
   return TRUE;
}

static void
S3SAVLoadCursor(pScr, pCurs, x, y)
     ScreenPtr pScr;
     CursorPtr pCurs;
     int x, y;
{
   int   index = pScr->myNum;
   unsigned short *ram;
   unsigned char tmp;
   char * videobuffer = (char *) xf86AccelInfoRec.FramebufferBase;

   if (!xf86VTSema)
      return;

   if (!pCurs)
      return;
 
   /* Remember which cursor is loaded */
   s3vCursorpCurs = pCurs;

   /* Wait for vertical retrace */
   /* VerticalRetraceWait(); */

   /* turn cursor off */
   S3VHideCursor();

   /* move cursor off-screen */
   outb(vgaCRIndex, 0x46);
   outb(vgaCRReg, 0xff);
   outb(vgaCRIndex, 0x47);
   outb(vgaCRReg, 0x7f);
   outb(vgaCRIndex, 0x49);
   outb(vgaCRReg, 0xff);
   outb(vgaCRIndex, 0x4e);
   outb(vgaCRReg, 0x3f);
   outb(vgaCRIndex, 0x4f);
   outb(vgaCRReg, 0x3f);
   outb(vgaCRIndex, 0x48);
   outb(vgaCRReg, 0x7f);

   /* Load storage location.  */
   outb(vgaCRIndex, 0x4d);
   outb(vgaCRReg, (0xff & s3vCursorVRAMMemSegment));
   outb(vgaCRIndex, 0x4c);
   outb(vgaCRReg, (0xff00 & s3vCursorVRAMMemSegment) >> 8);

   ram = (unsigned short *)pCurs->bits->devPriv[index];
   MemToBus(videobuffer + s3vCursorVRAMMemSegment * 1024, (char *) ram, 1024);

   if( s3vPriv.chip == S3_SAVAGE4 ) {
      /*
       * Bug in Savage4 Rev B requires us to do an MMIO read after
       * loading the cursor.
       */
      volatile unsigned int i = ALT_STATUS_WORD0;
   }

   /* Wait for vertical retrace */
   VerticalRetraceWait();

   /* position cursor */
   S3SAVMoveCursor(0, x, y);
   S3SAVRecolorCursor(pScr, pCurs, TRUE);

   /* turn cursor on */
   S3SAVShowCursor();
}

static void
S3SAVSetCursor(pScr, pCurs, x, y, generateEvent)
     ScreenPtr pScr;
     CursorPtr pCurs;
     int   x, y;
     Bool  generateEvent;

{
   int index = pScr->myNum;

   if (!pCurs)
      return;

   s3vHotX = pCurs->bits->xhot;
   s3vHotY = pCurs->bits->yhot;
   S3SAVLoadCursor(pScr, pCurs, x, y);

}

void
S3SAVRestoreCursor(pScr)
     ScreenPtr pScr;
{
   int index = pScr->myNum;
   int x, y;

   miPointerPosition(&x, &y);
   S3SAVLoadCursor(pScr, s3vCursorpCurs, x, y);
}

void
S3SAVRepositionCursor(pScr)
     ScreenPtr pScr;
{
   int x, y;


   miPointerPosition(&x, &y);
   /* Wait for vertical retrace */
   VerticalRetraceWait();
   S3SAVMoveCursor(pScr, x, y);
}

static void
S3SAVMoveCursor(pScr, x, y)
     ScreenPtr pScr;
     int   x, y;
{
   unsigned char xoff, yoff;

   if (!xf86VTSema)
      return;

   x -= vga256InfoRec.frameX0;
   y -= vga256InfoRec.frameY0;

   x -= s3vHotX;
   y -= s3vHotY;

   /* adjust for frame buffer base address granularity */
   if (vgaBitsPerPixel == 8)
     x += ((vga256InfoRec.frameX0) & 3);
   else if (vgaBitsPerPixel == 16)
     x += ((vga256InfoRec.frameX0) & 1);
   else if (vgaBitsPerPixel == 24)
     x += ((vga256InfoRec.frameX0+2) & 3) - 2;

   /*
    * Make these even when used.  There is a bug/feature on at least
    * some chipsets that causes a "shadow" of the cursor in interlaced
    * mode.  Making this even seems to have no visible effect, so just
    * do it for the generic case.
    */
   if (x < 0) {
     xoff = ((-x) & 0xFE);
     x = 0;
   } else {
     xoff = 0;
   }

   if (y < 0) {
      yoff = ((-y) & 0xFE);
      y = 0;
   } else {
      yoff = 0;
   }

   /* WaitIdle(); */
   /* This is the recomended order to move the cursor */
   outb(vgaCRIndex, 0x46);
   outb(vgaCRReg, (x & 0xff00)>>8);

   outb(vgaCRIndex, 0x47);
   outb(vgaCRReg, (x & 0xff));

   outb(vgaCRIndex, 0x49);
   outb(vgaCRReg, (y & 0xff));

   outb(vgaCRIndex, 0x4e);
   outb(vgaCRReg, xoff);

   outb(vgaCRIndex, 0x4f);
   outb(vgaCRReg, yoff);

   outb(vgaCRIndex, 0x48);
   outb(vgaCRReg, (y & 0xff00)>>8);
}


static void
S3SAVRecolorCursor(pScr, pCurs, displayed)
     ScreenPtr pScr;
     CursorPtr pCurs;
     Bool displayed;
{
   ColormapPtr pmap;
   unsigned short packedcolfg, packedcolbg;
   xColorItem sourceColor, maskColor;
   Bool bNeedExtra = FALSE;

   /* Clock doubled modes need an extra cursor stack write. */
   outb(0x3c4, 0x18);
   if( inb(0x3c5) & 0x80 ) 
   {
      outb(0x3c4, 0x15);
      if( inb(0x3c5) & 0x50 )
	 bNeedExtra = TRUE;
   }

   if (!xf86VTSema)
      return;

   if (!displayed)
      return;

   /*
    * The /MX family is apparently unique among the Savages, in that
    * the cursor color is always straight RGB.  The rest of the Savages
    * use palettized values at 8-bit when not clock doubled.
    */

   if( s3vPriv.chip == S3_SAVAGE_MX )
      bNeedExtra = TRUE;

   switch (vgaBitsPerPixel) {
   case 16:
      if (vga256InfoRec.weight.green == 5) {
	 packedcolfg = ((pCurs->foreRed   & 0xf800) >>  1)
	    | ((pCurs->foreGreen & 0xf800) >>  6)
	    | ((pCurs->foreBlue  & 0xf800) >> 11);
	 packedcolbg = ((pCurs->backRed   & 0xf800) >>  1)
	    | ((pCurs->backGreen & 0xf800) >>  6)
	    | ((pCurs->backBlue  & 0xf800) >> 11);
      } else {
	 packedcolfg = ((pCurs->foreRed   & 0xf800) >>  0)
	    | ((pCurs->foreGreen & 0xfc00) >>  5)
	    | ((pCurs->foreBlue  & 0xf800) >> 11);
	 packedcolbg = ((pCurs->backRed   & 0xf800) >>  0)
	    | ((pCurs->backGreen & 0xfc00) >>  5)
	    | ((pCurs->backBlue  & 0xf800) >> 11);
      }
      outb(vgaCRIndex, 0x45);
      inb(vgaCRReg);		/* reset stack pointer */
      outb(vgaCRIndex, 0x4A);
      outb(vgaCRReg, packedcolfg);
      outb(vgaCRReg, packedcolfg>>8);
      if( bNeedExtra )
      {
	  outb(vgaCRReg, packedcolfg);
	  outb(vgaCRReg, packedcolfg>>8);
      }
      outb(vgaCRIndex, 0x45);
      inb(vgaCRReg);		/* reset stack pointer */
      outb(vgaCRIndex, 0x4B);
      outb(vgaCRReg, packedcolbg);
      outb(vgaCRReg, packedcolbg>>8);
      if( bNeedExtra )
      {
	  outb(vgaCRReg, packedcolbg);
	  outb(vgaCRReg, packedcolbg>>8);
      }
      break;
   case 8:
      if( !bNeedExtra )
      {
	 vgaGetInstalledColormaps(pScr, &pmap);
	 sourceColor.red = pCurs->foreRed;
	 sourceColor.green = pCurs->foreGreen;
	 sourceColor.blue = pCurs->foreBlue;
	 FakeAllocColor(pmap, &sourceColor);
	 maskColor.red = pCurs->backRed;
	 maskColor.green = pCurs->backGreen;
	 maskColor.blue = pCurs->backBlue;
	 FakeAllocColor(pmap, &maskColor);
	 FakeFreeColor(pmap, sourceColor.pixel);
	 FakeFreeColor(pmap, maskColor.pixel);

	 outb(vgaCRIndex, 0x45);
	 inb(vgaCRReg);	/* reset stack pointer */
	 outb(vgaCRIndex, 0x4A);
	 outb(vgaCRReg, sourceColor.pixel);
	 outb(vgaCRReg, sourceColor.pixel);
	 outb(vgaCRIndex, 0x45);
	 inb(vgaCRReg);	/* reset stack pointer */
	 outb(vgaCRIndex, 0x4B);
	 outb(vgaCRReg, maskColor.pixel);
	 outb(vgaCRReg, maskColor.pixel);
	 break;
      }
      /* else */
      /* FALLTHROUGH */
   case 24:
   case 32:
      outb(vgaCRIndex, 0x45);
      inb(vgaCRReg);		/* reset stack pointer */
      outb(vgaCRIndex, 0x4A);
      outb(vgaCRReg, pCurs->foreBlue >>8);
      outb(vgaCRReg, pCurs->foreGreen>>8);
      outb(vgaCRReg, pCurs->foreRed  >>8);

      outb(vgaCRIndex, 0x45);
      inb(vgaCRReg);		/* reset stack pointer */
      outb(vgaCRIndex, 0x4B);
      outb(vgaCRReg, pCurs->backBlue >>8);
	 outb(vgaCRReg, pCurs->backGreen>>8);
      outb(vgaCRReg, pCurs->backRed  >>8);
      break;
   }
}

void
S3SAVWarpCursor(pScr, x, y)
     ScreenPtr pScr;
     int   x, y;
{
   miPointerWarpCursor(pScr, x, y);
   xf86Info.currentScreen = pScr;
}

void
S3SAVQueryBestSize(class, pwidth, pheight, pScreen)
     int class;
     unsigned short *pwidth;
     unsigned short *pheight;
     ScreenPtr pScreen;
{
   if (*pwidth > 0) {
      switch (class) {
         case CursorShape:
	    if (*pwidth > 64)
	       *pwidth = 64;
	    if (*pheight > 64)
	       *pheight = 64;
	    break;
         default:
	    mfbQueryBestSize(class, pwidth, pheight, pScreen);
	    break;
      }
   }
}
