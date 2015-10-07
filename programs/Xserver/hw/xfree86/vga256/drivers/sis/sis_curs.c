/* $XConsortium: sis_curs.c /main/3 1996/10/25 15:39:12 kaleb $ */
/*
 * Copyright 1994  The XFree86 Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * DAVID WEXELBLAT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Adapted for SiS by Alan Hourihane <alanh@fairlite.demon.co.uk>.
 *
 * Hardware cursor handling. Adapted from cirrus/cir_cursor.c and
 * accel/s3/s3Cursor.c, and ark/ark_cursor.c
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/sis/sis_curs.c,v 3.4.2.9 1999/12/21 07:43:44 hohndel Exp $ */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "input.h"
#include "cursorstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "windowstr.h"
#include "xf86.h"
#include "mipointer.h"
#include "xf86Priv.h"
#include "xf86_Option.h"
#include "xf86_OSlib.h"
#include "vga.h"

#include "sis_driver.h"
#include "sis_Blitter.h"

#include "vga256.h"
#include "xf86xaa.h"

extern int sisHWCursorType;

extern Bool vgaUseLinearAddressing;
extern Bool sisUseLinear;

static int sisFlipCursor = 0;
static Bool SISRealizeCursor();
static Bool SISUnrealizeCursor();
static void SISSetCursor();
static void SISMoveCursor();
static void SISRecolorCursor();

static miPointerSpriteFuncRec SISPointerSpriteFuncs =
{
   SISRealizeCursor,
   SISUnrealizeCursor,
   SISSetCursor,
   SISMoveCursor,
};

/* vga256 interface defines Init, Restore, Warp, QueryBestSize. */


extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern xf86InfoRec xf86Info;

static int SISCursorGeneration = -1;
static int SISCursorControlMode;
static int SISCursorAddress;

/*#define DEBUG*/

/*
 * This is the set variables that defines the cursor state within the
 * driver.
 */

int SISCursorHotX;
int SISCursorHotY;
int SISCursorWidth;	/* Must be set before calling SISCursorInit. */
int SISCursorHeight;
static CursorPtr SISCursorpCurs;

/*
 * This is a high-level init function, called once; it passes a local
 * miPointerSpriteFuncRec with additional functions that we need to provide.
 * It is called by the SVGA server.
 */

Bool SISCursorInit(pm, pScr)
	char *pm;
	ScreenPtr pScr;
{
        unsigned char temp, temp2;
	if (SISCursorGeneration != serverGeneration) {
		if (!(miPointerInitialize(pScr, &SISPointerSpriteFuncs,
		&xf86PointerScreenFuncs, FALSE)))
			return FALSE;

		SISCursorHotX = 0;
		SISCursorHotY = 0;
		pScr->RecolorCursor = SISRecolorCursor;
		SISCursorGeneration = serverGeneration;
	}

	/*
	 * Define the SIS cursor mode. Always 64x64 size !
	 */

	/* Pop the cursor in the last 16KB aligned segment */
	SISCursorAddress = vga256InfoRec.videoRam * 1024 - 16384;

	/* Program the cursor address in the chipset, because if the user 
	   specify a less amount of memory in XF86Config than in the BIOS,
	   the calculated address don't match with the BIOS config */
	if ( (SISchipset == SIS5597) || 
	     (SISchipset == SIS6326) || 
	     (SISchipset == SIS530))
        {
		/* bits [22:18] of the address */
	        temp = SISCursorAddress >> 18;
		/* copy bits [21:18] into the top bits of SR38 */
                temp2 = (rdinx(0x3C4,0x38) & 0x0F) | (temp << 4);
#ifdef DEBUG
                ErrorF ("Programming Cursor address: %d SR38=0x%X\n",
			SISCursorAddress, temp2);
#endif	
                outb(0x3C5, temp2);
		/* if set, store bit [22] to SR3E */
		if (temp & 0x10)
		{
			temp2 = rdinx(0x3C4,0x3E);
			outb(0x3C5, temp2 | 0x04);
		}
		/* disable the hardware cursor side pattern */
                temp2 = rdinx(0x3C4,0x1E);
                outb(0x3C5, (temp2 & 0xF7) );
	} else if (SISfamily == SIS300) {
		unsigned addr, *p;
		addr = (SISCursorAddress >> 10) & 0xFFFF;

		/* hardware cursor registers in MMIO space */
		p = (unsigned *) (sisMMIOBase + SIS300_CURSOR_CONTROL);

		*p &= 0x00ff0000;
		*p |= addr; /* use pattern 0 */
	}

	return TRUE;
}

/*
 * This enables displaying of the cursor by the SIS graphics chip.
 */

static void SISShowCursor() {
	unsigned char temp;
	unsigned *p;

	if (SISfamily == SIS300) {
		p = (unsigned *) (sisMMIOBase + SIS300_CURSOR_CONTROL);
		*p |= SIS300_CURSOR_ENABLE;
	} else {
		temp = rdinx(0x3C4, 0x06);
		outb(0x3C5, temp | 0x40);
	}
}

/*
 * This disables displaying of the cursor by the SIS graphics chip.
 */

void SISHideCursor() {
	unsigned char temp;
	unsigned *p;

	if (SISfamily == SIS300) {
		p = (unsigned *) (sisMMIOBase + SIS300_CURSOR_CONTROL);
		*p &= SIS300_CURSOR_DISABLE;
	} else {
		temp = rdinx(0x3C4, 0x06);
		outb(0x3C5, temp & 0xBF);
	}
}

/*
 * This function is called when a new cursor image is requested by
 * the server. The main thing to do is convert the bitwise image
 * provided by the server into a format that the graphics card
 * can conveniently handle, and store that in system memory.
 * Adapted from accel/s3/s3Cursor.c.
 */

/*
 * SiS300 has changed hardware cursor bitmap format.
 * The new format is still a 64x64 bitmap, 64 lines, each line is
 * 16 bytes. But each line is 8 bytes AND mask first and
 * followed by 8 bytes XOR mask.
 * If cursor image is smaller than 64x64, then fill the AND mask with 0xFF
 * and the XOR mask with 0x00.
 *
 *    8 bytes AND mask 8 bytes XOR mask
 *    |--------------| |--------------|
 *  ^ ................ ................
 *  |
 * 64
 *  l
 *  i
 *  n
 *  e
 *  s
 *  |
 *  v ................ ................
 *
 */

static Bool SISRealizeCursor(pScr, pCurs)
	ScreenPtr pScr;
	CursorPtr pCurs;
{
   register int i, j;
   unsigned char *pServMsk;
   unsigned char *pServSrc;
   int   index = pScr->myNum;
   pointer *pPriv = &pCurs->bits->devPriv[index];
   unsigned char *ram, *p;
   int wsrc, h;
   CursorBitsPtr bits = pCurs->bits;

   if (pCurs->bits->refcnt > 1)
      return TRUE;

   if (SISfamily == SIS300)
	ram = (unsigned char *)xalloc(1024);
   else
	ram = (unsigned char *)xalloc(16384);

   if (!ram)
      return FALSE;

   if (SISfamily == SIS300) {
	p = ram;
	for (i=0;i<SISCursorHeight;i++) {
		for (j=0;j<8;j++)
			*p++ = 0xFF;
		for (j=0;j<8;j++)
			*p++ = 0x00;
	}
   } else
	memset (ram, 0xAA, 16384);

   *pPriv = (pointer) ram;

   pServSrc = (unsigned char *)bits->source;
   pServMsk = (unsigned char *)bits->mask;

   h = bits->height;
   if (h > SISCursorHeight)
	h = SISCursorHeight;

   wsrc = PixmapBytePad(bits->width, 1) + 1;
   if (wsrc > SISCursorWidth)
	wsrc = SISCursorWidth;

   for (i = 0; i < SISCursorHeight; i++,ram+=16) {
	for (j = 0; j < SISCursorWidth / 16; j++) {
	unsigned char m, s;

     if (SISfamily == SIS300) {
	if (i < h && j < wsrc) {
                m = *pServMsk++;
                s = *pServSrc++;
                m = ~m;
                ram[j] = (((m & 0x01) << 7) | ((m & 0x02) << 5) |
                          ((m & 0x04) << 3) | ((m & 0x08) << 1) |
                          ((m & 0x10) >> 1) | ((m & 0x20) >> 3) |
                          ((m & 0x40) >> 5) | ((m & 0x80) >> 7));
                ram[j+8] = (((s & 0x01) << 7) | ((s & 0x02) << 5) |
                            ((s & 0x04) << 3) | ((s & 0x08) << 1) |
                            ((s & 0x10) >> 1) | ((s & 0x20) >> 3) |
                            ((s & 0x40) >> 5) | ((s & 0x80) >> 7));
	}
     } else {
	if (i < h && j < wsrc) {
		m = *pServMsk++;
		s = *pServSrc++;

		m = ~m;

		ram[j*2] = ((m&0x01) << 7) | ((s&0x01) << 6) |
			   ((m&0x02) << 4) | ((s&0x02) << 3) |
			   ((m&0x04) << 1) |  (s&0x04)       |
			   ((m&0x08) >> 2) | ((s&0x08) >> 3) ;
		ram[(j*2)+1] = ((m&0x10) << 3) | ((s&0x10) << 2) |
			        (m&0x20)       | ((s&0x20) >> 1) |
			       ((m&0x40) >> 3) | ((s&0x40) >> 4) |
			       ((m&0x80) >> 6) | ((s&0x80) >> 7) ;
	} else {
		ram[j*2] = 0xAA;
		ram[(j*2)+1] = 0xAA;
	}
	}
      }
    }
   return TRUE;
}

/*
 * This is called when a cursor is no longer used. The intermediate
 * cursor image storage that we created needs to be deallocated.
 */

static Bool SISUnrealizeCursor(pScr, pCurs)
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

/*
 * This function uploads a cursor image to the video memory of the
 * graphics card. The source image has already been converted by the
 * Realize function to a format that can be quickly transferred to
 * the card.
 * This is a local function that is not called from outside of this
 * module.
 */

extern void SISSetWrite();

static void SISLoadCursorToCard(pScr, pCurs, x, y)
	ScreenPtr pScr;
	CursorPtr pCurs;
{
	unsigned char *cursor_image;
	int index = pScr->myNum;

	if (!xf86VTSema)
		return;
#if 0
	/* Check if blitter is active. Mustn't touch the video ram
           till it is finished */
	if ( sisUseMMIO )
	  xf86AccelInfoRec.Sync();
#endif

	cursor_image = pCurs->bits->devPriv[index];

	if (SISfamily == SIS300) {
		memcpy((unsigned char *)vgaLinearBase + SISCursorAddress,
			cursor_image, 1024);
	} else {
		if (vgaUseLinearAddressing)
			memcpy((unsigned char *)vgaLinearBase +
				SISCursorAddress, cursor_image, 16384);
		else {
			vgaSaveBank();
			SISSetWrite(SISCursorAddress >> 16);
			memcpy((unsigned char *)vgaBase + (SISCursorAddress
				& 0xFFFF), cursor_image, 16384);
			vgaRestoreBank();
		}
	}
}

/*
 * This function should make the graphics chip display new cursor that
 * has already been "realized". We need to upload it to video memory,
 * make the graphics chip display it.
 * This is a local function that is not called from outside of this
 * module (although it largely corresponds to what the SetCursor
 * function in the Pointer record needs to do).
 */

static void SISLoadCursor(pScr, pCurs, x, y)
	ScreenPtr pScr;
	CursorPtr pCurs;
	int x, y;
{
	if (!xf86VTSema)
		return;

	if (!pCurs)
		return;

	/* Remember the cursor currently loaded into this cursor slot. */
	SISCursorpCurs = pCurs;

	SISHideCursor();

	/* Program the cursor image address in video memory. */
#if 0
 	wrinx(vgaIOBase + 4, 0x44, 
		((SISCursorAddress/1024) & 0x00FF));
	wrinx(vgaIOBase + 4, 0x45, 
		((SISCursorAddress/1024) & 0xFF00) >> 8);
#endif

	SISRecolorCursor(pScr, pCurs, 1);

	SISLoadCursorToCard(pScr, pCurs, x, y);

	/* Position cursor */
	SISMoveCursor(pScr, x, y);

#if 0
	if ( sisUseMMIO )
	  xf86AccelInfoRec.Sync();
#endif

	/* Turn it on. */
	SISShowCursor();
}

/*
 * This function should display a new cursor at a new position.
 */

static void SISSetCursor(pScr, pCurs, x, y, generateEvent)
	ScreenPtr pScr;
	CursorPtr pCurs;
	int x, y;
	Bool generateEvent;
{
	if (!pCurs)
		return;

	SISCursorHotX = pCurs->bits->xhot;
	SISCursorHotY = pCurs->bits->yhot;

	SISLoadCursor(pScr, pCurs, x, y);
}

/*
 * This function should redisplay a cursor that has been
 * displayed earlier. It is called by the SVGA server.
 */

void SISRestoreCursor(pScr)
	ScreenPtr pScr;
{
	int x, y;

	miPointerPosition(&x, &y);

	SISLoadCursor(pScr, SISCursorpCurs, x, y);
}

/*
 * This function is called when the current cursor is moved. It makes
 * the graphic chip display the cursor at the new position.
 */

static void SISMoveCursor(pScr, x, y)
	ScreenPtr pScr;
	int x, y;
{
	int xorigin, yorigin;
	unsigned *p;

	if (!xf86VTSema)
		return;

	x -= vga256InfoRec.frameX0 + SISCursorHotX;
	y -= vga256InfoRec.frameY0 + SISCursorHotY;

	/*
	 * If the cursor is partly out of screen at the left or top,
	 * we need set the origin.
	 */
	xorigin = 0;
	yorigin = 0;
	if (x < 0) {
		xorigin = -x;
		x = 0;
	}
	if (y < 0) {
		yorigin = -y;
		y = 0;
	}

	if (XF86SCRNINFO(pScr)->modes->Flags & V_DBLSCAN)
		y *= 2;
	if (XF86SCRNINFO(pScr)->modes->Flags & V_INTERLACE)
		y /= 2;

	if (SISfamily == SIS300) {
		p = (unsigned *) (sisMMIOBase + SIS300_CURSOR_H_LOC);

		/* Horizontal */
		*p++ = ((xorigin & 0x3F) << 16) | (x & 0xFFF);

		/* Vertical */
		*p = ((yorigin & 0x3F) << 16) | (y & 0x7FF);
	} else {
	/* Program the cursor origin (offset into the cursor bitmap). */
		wrinx(0x3C4, 0x1C, xorigin);
		wrinx(0x3C4, 0x1F, yorigin);

	/* Program the new cursor position. */
		wrinx(0x3C4, 0x1A, x);		/* Low byte. */
		wrinx(0x3C4, 0x1B, x >> 8);	/* High byte. */
		wrinx(0x3C4, 0x1D, y);		/* Low byte. */
		wrinx(0x3C4, 0x1E, y >> 8);	/* High byte. */
	}
}

/*
 * This is a local function that programs the colors of the cursor
 * on the graphics chip.
 * Adapted from accel/s3/s3Cursor.c.
 */

static void
SISRecolorCursor(pScr, pCurs, displayed)
     ScreenPtr pScr;
     CursorPtr pCurs;
     Bool displayed;
{
   unsigned short bred, bgreen, bblue;
   unsigned short fred, fgreen, fblue;
   int shift;
   int i;
   VisualPtr pVisual;
   unsigned char AddColReg;
   unsigned *p;

   if (!xf86VTSema)
       return;

   if (!displayed)
       return;

   /* Find the PseudoColour or TrueColor visual for the colour mapping
    * function
    */

   for (i = 0, pVisual = pScr->visuals; i < pScr->numVisuals; i++, pVisual++)
     {
       if ((pVisual->class == PseudoColor) || (pVisual->class == TrueColor))
	 break;
     }

   if (i == pScr->numVisuals)
     {
       ErrorF ("SIS: Failed to find a visual for mapping hardware cursor colours\n");
       return;
     }

   shift = 10 ; 	/* vga is a 6 bit dac, cursor color too */

   bred   = pCurs->backRed;
   bgreen = pCurs->backGreen;
   bblue  = pCurs->backBlue;

   pScr->ResolveColor (&bred, &bgreen, &bblue, pVisual);

   fred   = pCurs->foreRed;
   fgreen = pCurs->foreGreen;
   fblue  = pCurs->foreBlue;

   pScr->ResolveColor (&fred, &fgreen, &fblue, pVisual);

   if (SISfamily == SIS300) {
	p = (unsigned *) (sisMMIOBase + SIS300_CURSOR_COLOR0);

	/* cursor color 0 */
	*p++ = ((bred   & 0xFF00) << 8)|
	       ((bgreen & 0xFF00))     |
	       ((bblue  & 0xFF00) >> 8);

	/* cursor color 1 */
	*p = ((fred   & 0xFF00) << 8)|
	     ((fgreen & 0xFF00))     |
	     ((fblue  & 0xFF00) >> 8);
   } else {
	wrinx(0x3C4, 0x14, bred>>shift);
	wrinx(0x3C4, 0x15, bgreen>>shift);
	wrinx(0x3C4, 0x16, bblue>>shift);

	wrinx(0x3C4, 0x17, fred>>shift);
	wrinx(0x3C4, 0x18, fgreen>>shift);
	wrinx(0x3C4, 0x19, fblue>>shift);
   }
}

/*
 * This doesn't do very much. It just calls the mi routine. It is called
 * by the SVGA server.
 */

void SISWarpCursor(pScr, x, y)
	ScreenPtr pScr;
	int x, y;
{
	miPointerWarpCursor(pScr, x, y);
	xf86Info.currentScreen = pScr;
}

/*
 * This function is called by the SVGA server. It returns the
 * size of the hardware cursor that we support when asked for.
 * It is called by the SVGA server.
 */

void SISQueryBestSize(class, pwidth, pheight, pScreen)
	int class;
	unsigned short *pwidth;
	unsigned short *pheight;
	ScreenPtr pScreen;
{
 	if (*pwidth > 0) {
 		if (class == CursorShape) {
			*pwidth = SISCursorWidth;
			*pheight = SISCursorHeight;
		}
		else
		    (void) mfbQueryBestSize(class, pwidth, pheight, pScreen);
	}
}



