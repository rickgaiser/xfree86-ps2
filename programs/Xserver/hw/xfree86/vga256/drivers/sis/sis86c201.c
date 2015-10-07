/* $XConsortium: sis86c201.c /main/11 1996/10/27 13:24:11 kaleb $ */
/*
 * Copyright 1995 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Alan Hourihane, alanh@fairlite.demon.co.uk
 *
 * Modified 1996 by Xavier Ducoin <xavier@rd.lectra.fr>
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/sis/sis86c201.c,v 3.17.2.23 1999/12/21 07:43:42 hohndel Exp $ */

/* 
  Modified Feb - Jul 1998 for SiS 5597/8 by Mike Chapman <mike@paranoia.com>, 
  Juanjo Santamarta <santamarta@ctv.es>, 
  and Mitani Hiroshi <hmitani@drl.mei.co.jp> 
     
  Further modified in July 1998 to fix bugs in the driver for the SiS6326 by        
  David Thomas <davtom@dream.org.uk>. 
  
  Added SiS520/630 support February/March 1999 by
  Xavier Ducoin <x.ducoin@lectra.com> and
  Dirk Hohndel <hohndel@XFree86.Org>
*/


/*#define DEBUG    */
/*#define DEBUG1   */
/*#define IO_DEBUG */

#define USE_XAA
#include "X.h"
#include "input.h"
#include "screenint.h"
#include "dix.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
#include "vga.h"
#include "vgaPCI.h"

#ifdef XFreeXDGA
#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#ifdef XF86VGA16
#define MONOVGA
#endif

#if !defined(MONOVGA) && !defined(XF86VGA16)
#include "vga256.h"
#endif
#include "sis_driver.h"

#ifndef MONOVGA
extern vgaHWCursorRec vgaHWCursor;
#endif

/* Blitter related */
/*address in video ram of tile/stipple pattern*/
unsigned int sisBLTPatternAddress = -1; 
int sisBLTPatternOffscreenSize = 0 ;
unsigned char *sisBltDataWindow = NULL;
Bool sisAvoidImageBLT = FALSE;

int sisReg32MMIO[]={0x8280,0x8284,0x8288,0x828C,0x8290,0x8294,0x8298,0x829C,
		    0x82A0,0x82A4,0x82A8,0x82AC};
/* Engine Register for the 2nd Generation Graphics Engine */
int sis2Reg32MMIO[]={0x8200,0x8204,0x8208,0x820C,0x8210,0x8214,0x8218,0x821C,
		     0x8220,0x8224,0x8228,0x822C,0x8230,0x8234,0x8238,0x823C,
		     0x8240,0x8244,0x8248,0x824C,0x8250,0x8300};
#ifndef MONOVGA
extern GCOps cfb16TEOps1Rect, cfb16TEOps, cfb16NonTEOps1Rect, cfb16NonTEOps;
extern GCOps cfb24TEOps1Rect, cfb24TEOps, cfb24NonTEOps1Rect, cfb24NonTEOps;
#endif

Bool sisHWCursor = FALSE ;
Bool sisTurboQueue = FALSE ;

/* Clock related */
typedef struct {
    unsigned char msr;
    unsigned char xr2A;
    unsigned char xr2B;
    unsigned char xr2C;
    unsigned char xr2D;
    unsigned char xr13;			/* extended clock gen for 5597 */
    int Clock;
} sisClockReg, *sisClockPtr;

typedef struct {
	vgaHWRec std;          		/* std IBM VGA register 	*/
	unsigned char Port_3C4[0x40];	/* 0x37 top for 20x, 0x3D for 5597, 0x3F for 530*/
	unsigned char Port_3D4[0x1B];	/* Only CR19 and CR1A used by 630 */
	unsigned char ClockReg2;
	sisClockReg   sisClock;
} vgaSISRec, *vgaSISPtr;

/* alias for specific extended registers   */
#define ClockReg	Port_3C4[0x07] 
#define DualBanks	Port_3C4[0x0B]
#define BankReg		Port_3C4[0x06]
#define CRTCOff		Port_3C4[0x0A]
#define DispCRT		Port_3C4[0x27]
#define Unknown		Port_3C4[0x08]
#define LinearAddr0	Port_3C4[0x20]
#define LinearAddr1	Port_3C4[0x21]

static Bool SISClockSelect();
static char *SISIdent();
static Bool SISProbe();
static void SISEnterLeave();
static Bool SISInit();
static int  SISValidMode();
static void *SISSave();
static void SISRestore();
static void SISFbInit();
static void SISAdjust();
static void SISDisplayPowerManagementSet();
extern void SISSetRead();
extern void SISSetWrite();
extern void SISSetReadWrite();

extern int SISCursorHotX;
extern int SISCursorHotY;
extern int SISCursorWidth;
extern int SISCursorHeight;

extern Bool SISCursorInit();
extern void SISRestoreCursor();
extern void SISWarpCursor();
extern void SISQueryBestSize();

vgaVideoChipRec SIS = {
  SISProbe,
  SISIdent,
  SISEnterLeave,
  SISInit,
  SISValidMode,
  SISSave,
  SISRestore,
  SISAdjust,
  vgaHWSaveScreen,
  (void (*)())NoopDDA,
  SISFbInit,
  SISSetRead,
  SISSetWrite,
  SISSetReadWrite,
  0x10000,
  0x10000,
  16,
  0xffff,
  0x00000, 0x10000,
  0x00000, 0x10000,
  TRUE,                           
  VGA_DIVIDE_VERT,
  {0,},
  16,				
  FALSE,
  0,
  0,
  TRUE,
  TRUE,
  TRUE,
  NULL,
  1,            /* ClockMulFactor */
  1             /* ClockDivFactor */
};

#define new ((vgaSISPtr)vgaNewVideoState)
#define write_xr(num,val) {outb(0x3c4, num);outb(0x3c5, val);}
#define read_xr(num,var) {outb(0x3c4, num);var=inb(0x3c5);}

int SISchipset;
int SISfamily;
/* default is enhanced mode (use option to disable)*/
Bool sisUseLinear = TRUE;
Bool sisUseMMIO = TRUE;
Bool sisUseXAAcolorExp = TRUE;
static int SISDisplayableMemory;
volatile unsigned char *sisMMIOBase = NULL;
unsigned int PCIMMIOBase=0 ;
unsigned long turbo_queue_address ;

/*
 * SISIdent --
 */
static char *
SISIdent(n)
	int n;
{
	static char *chipsets[] = {"sis86c201", "sis86c202", "sis86c205", 
				   "sis86c215", "sis86c225",  	
				   "sis5597", "sis5598", "sis6326",
	                           "sis530", "sis620", "sis300" ,
			  	   "sis630", "sis540" };

	if (n + 1 > sizeof(chipsets) / sizeof(char *))
		return(NULL);
	else
		return(chipsets[n]);
}

/*
 * SISClockSelect --
 * 	select one of the possible clocks ...
 */
static Bool
SISClockSelect(no)
	int no;
{
	static unsigned char save1, save2;
	unsigned char temp;
#ifdef DEBUG
    ErrorF("SISClockSelect(%d)\n",no);
#endif

	/*
	 * CS0 and CS1 are in MiscOutReg
	 *
	 * CS2,CS3,CS4 are in 0x3c4 index 7
	 * But - only active when CS0/CS1 are set.
	 */
	switch(no)
	{
	case CLK_REG_SAVE:
		save1 = inb(0x3CC);
		read_xr(0x07,save2);
		break;
	case CLK_REG_RESTORE:
		outb(0x3C2, save1);
		outw(0x3c4, (save2 << 8) | 0x07);
		break;
	default:
		/*
		 * Do CS0 and CS1 and set them - makes index 7 valid
		 */
		temp = inb(0x3CC);
		temp &= ~0x0C ;
		if ( no >= 2 ) {
		    outb(0x3C2, temp | 0x0C);
		    outw(0x3c4, (no << 8) | 0x07);
		}
		else /* using vga clock */
		   outb(0x3C2, temp | ( (no<<2) & 0x0C) ); 
		    
	}
	return(TRUE);
}

static Bool
ClockProgramable()
{
#if defined(MONOVGA) || defined(XF86VGA16)
    return FALSE;
#else
    return (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, 
		       &vga256InfoRec.clockOptions)) ;
#endif
}

/* Returns memory clock in KHz */
static int
sisMClk()
{ int mclk;
  unsigned char xr28, xr29, xr13, xr10;
#ifdef DEBUG
  ErrorF("sisMClk()");
#endif

    if (SISchipset == SIS530) {
	/* The MCLK in SiS530/620 is set by BIOS in SR10 */
	read_xr(0x10, xr10);
	switch(xr10 & 0x0f) {
	case 1:
		mclk = 75000; /* 75 Mhz */
		break;
	case 2:
		mclk = 83000; /* 83 Mhz */
		break;
	case 3:
		mclk = 100000;/* 100 Mhz */
		break;
	case 0:
	default:
		mclk = 66000; /* 66 Mhz */
	}
	return(mclk);
    }

    /* Numerator */
    read_xr(0x28,xr28);
    mclk=14318*((xr28 & 0x7f)+1);
#ifdef DEBUG
  ErrorF("14318 * %d(M)",(xr28 & 0x7f)+1);
#endif

    /* Denumerator */
    read_xr(0x29,xr29);
    mclk=mclk/((xr29 & 0x1f)+1);

#ifdef DEBUG
  ErrorF(" / %d(N)",(xr29 & 0x1f)+1);
#endif
  /* Divider. Does not seem to work for mclk for older cards */
  if ( ((SISchipset==SIS6326) || (SISfamily==SIS300)) && ((xr28 & 0x80)!=0) ) {
         mclk = mclk*2;
#ifdef DEBUG
  ErrorF(" * 2(VLD) ");
#endif
    }

    if (SISfamily == SIS300)
	xr13 = xr29 & 0x80;
    else
	    /* Post-scaler. Values depends on SR13 bit 7  */
	read_xr(0x13,xr13);

    if ( (xr13 & 0x80)==0 ) {
      mclk = mclk / (((xr29 & 0x60) >> 5)+1);
#ifdef DEBUG
  ErrorF(" / %d(Psn)",((xr29 & 0x60) >> 5)+1);
#endif
    }
    else {
      /* Values 00 and 01 are reserved */
      if ((xr29 & 0x60) == 0x40) { mclk=mclk/6;
#ifdef DEBUG
  ErrorF(" / 6(Psn)");
#endif
      }
      if ((xr29 & 0x60) == 0x60) { mclk=mclk/8;
#ifdef DEBUG
  ErrorF(" / 8(Psn)");
#endif
      }
    }
#ifdef DEBUG
  ErrorF(" = %d \n",mclk);
  ErrorF("SR28=%X ; SR29=%X ; SR13=%X\n",xr28,xr29,xr13);
#endif

    return(mclk);
}

/***** Only for SiS 5597 / 6326 *****/
/* Returns estimated memory bandwidth in Kbits/sec (for dotclock defaults)        */
/* Currently, a very rough estimate (2 cycles / read ; 1 for fast_vram), with 70% to allow for */
static int
sisMemBandWidth()
{ int band;

   if (vga256InfoRec.MemClk)  
     band=vga256InfoRec.MemClk; 
   else 
     band=sisMClk();

   outb(0x3c4, 0x0C);

   if (((inb(0x3c5) >> 1) & 3) == 0) /* Only 1 bank Vram */
     band = (band * 16);
   else
     band = (band * 32);

   if (OFLG_ISSET(OPTION_FAST_VRAM, &vga256InfoRec.options)) {
     band=band*2;
   } else {
     outb(0x3c4, 0x34);
     if ((inb(0x3c5) & 0xC0) == 0xC0) band=band*2;
   };

   /* Check AGP 2x Transfer Rate */
   outb(0x3c4, 0x0D);
   if ((inb(0x3c5) & 0x30) == 0x30) band=band*2;

   return(band*7/10);
}

float magic300[4] = { 1.2, 1.368421, 2.263158, 1.2 };
float magic630[4] = { 2.03, 2.03, 3.47, 2.03 };
static
int sis300MemBandWidth()
{
	int	mclk;
	unsigned char temp;

	mclk = sisMClk();

	read_xr(0x14, temp);
	temp = temp >> 6;
	switch (temp)  {
	case 1:
		mclk *= 64;
		break;
	case 2:
		mclk *= 128;
		break;
	case 3:
	case 0:
		mclk *= 32;
		break;
	}

	switch (SISchipset)  {
	case SIS300:
		return (int)(mclk / magic300[temp]);
	    break;
	case SIS630:
	case SIS540:
		return (int)(mclk / magic630[temp]);
	    break;
	}
}

int sisCalcVCLK(
	int Clock,
	int *out_n,
	int *out_dn,
	int *out_div,
	int *out_sbit,
	int *out_scale)
{
	float f,x,y,t, error, min_error;
	int n, dn, best_n, best_dn;

	/*
	 * Rules
	 *
	 * VCLK = 14.318 * (Divider/Post Scalar) * (Numerator/DeNumerator)
	 * Factor = (Divider/Post Scalar)
	 * Divider is 1 or 2
	 * Post Scalar is 1, 2, 3, 4, 6 or 8
	 * Numberator ranged from 1 to 128
	 * DeNumerator ranged from 1 to 32
	 * a. VCO = VCLK/Factor, suggest range is 150 to 250 Mhz
	 * b. Post Scalar selected from 1, 2, 4 or 8 first.
	 * c. DeNumerator selected from 2.
	 *
	 * According to rule a and b, the VCO ranges that can be scaled by
	 * rule b are:
	 *	150    - 250    (Factor = 1)
	 *	 75    - 125    (Factor = 2)
	 *	 37.5  -  62.5  (Factor = 4)
	 *	 18.75 -  31.25 (Factor = 8)
	 *
	 * The following ranges use Post Scalar 3 or 6:
	 *	125    - 150    (Factor = 1.5)
	 *	 62.5  -  75    (Factor = 3)
	 *       31.25 -  37.5  (Factor = 6)
	 *
	 * Steps:
	 * 1. divide the Clock by 2 until the Clock is less or equal to 31.25.
	 * 2. if the divided Clock is range from 18.25 to 31.25, than
	 *    the Factor is 1, 2, 4 or 8.
	 * 3. if the divided Clock is range from 15.625 to 18.25, than
	 *    the Factor is 1.5, 3 or 6.
	 * 4. select the Numberator and DeNumberator with minimum deviation.
	 *
	 * ** this function can select VCLK ranged from 18.75 to 250 Mhz
	 */
	f = (float) Clock;
	f /= 1000.0;
	if ((f > 250.0) || (f < 18.75))
		return 0;

	min_error = f;
	y = 1.0;
	x = f;
	while (x > 31.25) {
		y *= 2.0;
		x /= 2.0;
	}
	if (x >= 18.25) {
		x *= 8.0;
		y = 8.0 / y;
	} else if (x >= 15.625) {
		x *= 12.0;
		y = 12.0 / y;
	}

	t = y;
	if (t == (float) 1.5) {
		*out_div = 2;
		t *= 2.0;
	} else {
		*out_div = 1;
	}
	if (t > (float) 4.0) {
		*out_sbit = 1;
		t /= 2.0;
	} else {
		*out_sbit = 0;
	}

	*out_scale = (int) t;

	for (dn=2;dn<=32;dn++) {
		for (n=1;n<=128;n++) {
			error = x;
			error -= ((float) 14.318 * (float) n / (float) dn);
			if (error < (float) 0)
				error = -error;
			if (error < min_error) {
				min_error = error;
				best_n = n;
				best_dn = dn;
			}
		}
	}
	*out_n = best_n;
	*out_dn = best_dn;
#ifdef DEBUG
ErrorF("sisCalcVCLK: Clock=%d, n=%d, dn=%d, div=%d, sbit=%d, scale=%d\n",
	Clock, best_n, best_dn, *out_div, *out_sbit, *out_scale);
#endif
	return 1;
}

static void
sisCalcClock(int Clock, int max_VLD, unsigned int *vclk)
{
    int M, N, P, PSN, VLD, PSNx;

    int bestM, bestN, bestP, bestPSN, bestVLD;
    double bestError, abest = 42.0, bestFout;
    double target;

    double Fvco, Fout;
    double error, aerror;



    /*
     *	fd = fref*(Numerator/Denumerator)*(Divider/PostScaler)
     *
     *	M 	= Numerator [1:128] 
     *  N 	= DeNumerator [1:32]
     *  VLD	= Divider (Vco Loop Divider) : divide by 1, 2
     *  P	= Post Scaler : divide by 1, 2, 3, 4
     *  PSN     = Pre Scaler (Reference Divisor Select) 
     * 
     * result in vclk[]
     */
#define Midx 	0
#define Nidx 	1
#define VLDidx 	2
#define Pidx 	3
#define PSNidx 	4
#define Fref 14318180
/* stability constraints for internal VCO -- MAX_VCO also determines 
 * the maximum Video pixel clock */
#define MIN_VCO Fref
#define MAX_VCO 135000000
#define MAX_VCO_5597 353000000
#define MAX_PSN 0 /* no pre scaler for this chip */
#define TOLERANCE 0.01	/* search smallest M and N in this tolerance */
  
  int M_min = 2;
  int M_max = 128;
  
/*  abest=10000.0; */
 
  target = Clock * 1000;
 
     if (SISchipset == SIS5597 || SISchipset == SIS6326){
 	int low_N = 2;
 	int high_N = 5;
 	int PSN = 1;
 
 	P = 1;
 	if (target < MAX_VCO_5597 / 2)
 	    P = 2;
 	if (target < MAX_VCO_5597 / 3)
 	    P = 3;
 	if (target < MAX_VCO_5597 / 4)
 	    P = 4;
 	if (target < MAX_VCO_5597 / 6)
 	    P = 6;
 	if (target < MAX_VCO_5597 / 8)
 	    P = 8;
 
 	Fvco = P * target;
 
#ifdef DEBUG
         ErrorF("P= %d Fvco. : %.2f MHz\n", P, Fvco / 1.0e6);
#endif 
 	for (N = low_N; N <= high_N; N++){
 	    double M_desired = Fvco / Fref * N;
 	    if (M_desired > M_max * max_VLD)
 		continue;
 
 	    if ( M_desired > M_max ) {
 		M = M_desired / 2 + 0.5;
 		VLD = 2;
 	    } else {
 		M = Fvco / Fref * N + 0.5;
 		VLD = 1;
 	    };
 
 	    Fout = (double)Fref * (M * VLD)/(N * P);
 
#ifdef DEBUG
             ErrorF("M= %d,VLD= %d, Fout. : %.2f MHz\n", M, VLD, Fout / 1.0e6);
#endif 
 	    error = (target - Fout) / target;
 	    aerror = (error < 0) ? -error : error;
/* 	    if (aerror < abest && abest > TOLERANCE) {*/
 	    if (aerror < abest) {
 	        abest = aerror;
 	        bestError = error;
 	        bestM = M;
 	        bestN = N;
 	        bestP = P;
 	        bestPSN = PSN;
 	        bestVLD = VLD;
 	        bestFout = Fout;
 	    }
 	}
     }
     else {
         for (PSNx = 0; PSNx <= MAX_PSN ; PSNx++) {
 	    int low_N, high_N;
 	    double FrefVLDPSN;
 
 	    PSN = !PSNx ? 1 : 4;
 
 	    low_N = 2;
 	    high_N = 32;
 
 	    for ( VLD = 1 ; VLD <= max_VLD ; VLD++ ) {
 
 	        FrefVLDPSN = (double)Fref * VLD / PSN;
 	        for (N = low_N; N <= high_N; N++) {
 		    double tmp = FrefVLDPSN / N;
 
 		    for (P = 1; P <= 4; P++) {	
 		        double Fvco_desired = target * ( P );
 		        double M_desired = Fvco_desired / tmp;
 
 		        /* Which way will M_desired be rounded?  
 		         *  Do all three just to be safe.  
 		         */
 		        int M_low = M_desired - 1;
 		        int M_hi = M_desired + 1;
 
 		        if (M_hi < M_min || M_low > M_max)
 			    continue;
 
 		        if (M_low < M_min)
 			    M_low = M_min;
 		        if (M_hi > M_max)
 			    M_hi = M_max;
 
 		        for (M = M_low; M <= M_hi; M++) {
 			    Fvco = tmp * M;
 			    if (Fvco <= MIN_VCO)
 			        continue;
 			    if (Fvco > MAX_VCO)
 			        break;
 
 			    Fout = Fvco / ( P );
 
 			    error = (target - Fout) / target;
 			    aerror = (error < 0) ? -error : error;
 			    if (aerror < abest) {
 			        abest = aerror;
 			        bestError = error;
 			        bestM = M;
 			        bestN = N;
 			        bestP = P;
 			        bestPSN = PSN;
 			        bestVLD = VLD;
 			        bestFout = Fout;
 			    }
#ifdef DEBUG1
 			ErrorF("Freq. selected: %.2f MHz, M=%d, N=%d, VLD=%d,"
 			       " P=%d, PSN=%d\n",
 			       (float)(Clock / 1000.), M, N, P, VLD, PSN);
 			ErrorF("Freq. set: %.2f MHz\n", Fout / 1.0e6);
#endif
 		        }
 		    }
 	        }
 	    }
         }
  }
  vclk[Midx]    = bestM;
  vclk[Nidx]    = bestN;
  vclk[VLDidx]  = bestVLD;
  vclk[Pidx]    = bestP;
  vclk[PSNidx]  = bestPSN;
  #ifdef DEBUG
     ErrorF("Freq. selected: %.2f MHz, M=%d, N=%d, VLD=%d, P=%d, PSN=%d\n",
 	(float)(Clock / 1000.), vclk[Midx], vclk[Nidx], vclk[VLDidx], 
 	   vclk[Pidx], vclk[PSNidx]);
     ErrorF("Freq. set: %.2f MHz\n", bestFout / 1.0e6);
     ErrorF("VCO Freq.: %.2f MHz\n", bestFout*bestP / 1.0e6);
  #endif
#ifdef DEBUG1
                       ErrorF("abest=%f\n",
                               abest);
#endif
  }

/* Setting MClock is dangerous !!. */
static void 
sisSetMClk(int Clock)
{
  unsigned int vclk[5];
  unsigned char temp, xr28, xr29;
#ifdef DEBUG
  ErrorF("sisSetMClk(%d)\n",Clock);
#endif

/* We can use SiSCalcClock for Mclck finding, but with max_VLD = 1 */
  sisCalcClock(Clock, 1, vclk);
  
  outw(0x3c4, 0x8605); 	/* Unlock extended registers */

  xr28 = (vclk[Midx] - 1) & 0x7f ;
  xr28 |= ((vclk[VLDidx] == 2 ) ? 1 : 0 ) << 7 ;
  xr29  = (vclk[Nidx] -1) & 0x1f ;	/* bits [4:0] contain denumerator -MC */
  if (vclk[Pidx] <= 4){
 	    xr29 |= (vclk[Pidx] -1 ) << 5 ; /* postscale 1,2,3,4 */
 	    read_xr(0x13, temp );
 	    temp &= 0x7F;
 	    write_xr(0x13, temp );
 	} else {
        xr29 |= ((vclk[Pidx] / 2) -1 ) << 5 ;  /* postscale 6,8 */
 
	    read_xr(0x13, temp );
 	    temp |= 0x80;
 	    write_xr(0x13, temp );
 	};
 
 	write_xr(0x28, xr28 );
 	write_xr(0x29, xr29 );
/* Reset */

#ifdef DEBUG
  ErrorF("MClk=%.3f MHz\n",sisMClk() / 1000.0);
#endif
}


static void
sisClockSave(Clock)
    sisClockPtr Clock;
{
#ifdef DEBUG
    ErrorF("sisClockSave(Clock)\n");
#endif
    Clock->msr = (inb(0x3CC) & 0xFE);	/* save the standard VGA clock 
					 * registers */
    if (SISfamily == SIS300) {
	read_xr(0x2B, Clock->xr2B);
	read_xr(0x2C, Clock->xr2C);
	read_xr(0x2D, Clock->xr2D);
    } else {
	read_xr(0x2A, Clock->xr2A);
	read_xr(0x2B, Clock->xr2B);
	read_xr(0x13, Clock->xr13);
    }

}

static void
sisClockRestore(Clock)
    sisClockPtr Clock;
{
#ifdef DEBUG
    ErrorF("sisClockRestore(Clock)\n");
#endif
    outb(0x3C2, Clock->msr);
    if (SISfamily == SIS300) {
	write_xr(0x2B, Clock->xr2B);
	write_xr(0x2C, Clock->xr2C);
	write_xr(0x2D, Clock->xr2D);
    } else {
	write_xr(0x2A, Clock->xr2A);
	write_xr(0x2B, Clock->xr2B);
	write_xr(0x13, Clock->xr13);
    }
}

static Bool
sisClockFind(no, Clock)
    int no;
    sisClockPtr Clock;
{
    int clock ;

    clock = vga256InfoRec.clock[no] ;

#ifdef DEBUG
    ErrorF("sisClockFind(%d %d)\n",no,clock);
#endif

    if (no > (vga256InfoRec.clocks - 1))
	return (FALSE);

    Clock->Clock = clock;

    return (TRUE);
}

static void 
sisClockLoad(Clock)
    sisClockPtr Clock;
 {
    unsigned int 	vclk[5];
    unsigned char 	temp, xr2a, xr2b, xr2c;
    int num, denum, div, sbit, scale;
  #ifdef DEBUG
      ErrorF("sisClockLoad(Clock)\n");
  #endif

    if (!Clock->Clock) {       /* Hack to load saved console clock */
	sisClockRestore(Clock) ;
    } else if (((SISchipset == SIS530) || (SISfamily == SIS300))
		&& (sisCalcVCLK(Clock->Clock, &num,
		&denum, &div, &sbit, &scale))){
		if (SISchipset == SIS530) {
			xr2a = (num - 1) & 0x7f;
			xr2a |= (div == 2)? 0x80 : 0;
			xr2b = (denum - 1) & 0x1f;
			xr2b |= (((scale - 1) & 3) << 5);

			/* When set VCLK, you should set SR13 first */
			if (sbit) {
				read_xr(0x13, temp);
				temp |= 0x40;
				write_xr(0x13, temp);
			} else {
				read_xr(0x13, temp);
				temp &= 0xBF;
				write_xr(0x13, temp);
			}

			write_xr(0x2A, xr2a);
			write_xr(0x2B, xr2b);
ErrorF("SetVCLK Finished\n");
		} else { /* SIS300 */
			xr2b = (num - 1) & 0x7f;
			if (div == 2)
				xr2b |= 0x80;
			xr2c = (denum - 1) & 0x1f;
			xr2c |= (((scale - 1) & 3) << 5);
			if (sbit)
				xr2c |= 0x80;
			write_xr(0x2B, xr2b);
			write_xr(0x2C, xr2c);
			write_xr(0x2D, 0x80);
		}
		/* if sisCalcVCLK cannot handle the request clock,
		   try sisCalcClock! */
    } else {
	sisCalcClock(Clock->Clock, 2, vclk);

	xr2a = (vclk[Midx] - 1) & 0x7f ;
	xr2a |= ((vclk[VLDidx] == 2 ) ? 1 : 0 ) << 7 ;
	xr2b  = (vclk[Nidx] -1) & 0x1f ;	/* bits [4:0] contain denumerator -MC */
 	if (vclk[Pidx] <= 4){
 #ifdef DEBUG
 	ErrorF("vclk[Pidx]=%2x (1..4)\n", vclk[Pidx]);
 #endif
 	    xr2b |= (vclk[Pidx] -1 ) << 5 ; /* postscale 1,2,3,4 */
 	    read_xr(0x13, temp );
 	    temp &= 0xBF;
 	    write_xr(0x13, temp );
 	} else {
 #ifdef DEBUG
 	ErrorF("vclk[Pidx]=%2x (6,8)\n", vclk[Pidx]);
 #endif
	    xr2b |= ((vclk[Pidx] / 2) -1 ) << 5 ;  /* postscale 6,8 */
 
 	    read_xr(0x13, temp );
 	    temp |= 0x40;
 	    write_xr(0x13, temp );
 	};
 	xr2b |= 0x80 ;   /* gain for high frequency */
 
 	write_xr(0x2A, xr2a );
 	write_xr(0x2B, xr2b );
 #ifdef DEBUG
 	ErrorF("xr2a=%2x xr2b=%2x\n",xr2a, xr2b);
 #endif
    }
    if (SISchipset == SIS5597 || SISchipset == SIS6326) {
 
 	/* EDO flag - unknown if useful */
 	read_xr(0x23, temp);
 	if (OFLG_ISSET(OPTION_EDO_VRAM, &vga256InfoRec.options))
 	    temp |= 0x40;
 	write_xr(0x23, temp);
#ifdef DEBUG
 	ErrorF("SR23=%2x  ", temp);
#endif 
    }
    if (SISchipset == SIS5597 || SISchipset == SIS6326 || SISchipset == SIS530) {
 	/* clock reg - this does something to filter noise */
 	/*write_xr(0x3B, 0x08 );*/
#ifdef DEBUG
 	read_xr(0x3B, temp );
 	ErrorF("SR3B=%2x  ",temp);
#endif 
 	/* host bus. Reserved on 6326 */
        if (SISchipset == SIS5597) {
     	    read_xr(0x34, temp);
 	    if (OFLG_ISSET(OPTION_HOST_BUS, &vga256InfoRec.options))
 	    	temp |= 0x18; 
 	    else temp &= ~0x18;
 	        write_xr(0x34, temp);
 	    read_xr(0x3D, temp);
 	    if (OFLG_ISSET(OPTION_HOST_BUS, &vga256InfoRec.options))
 	     	  temp &= 0x0F; 
 	      write_xr(0x3D, temp); 
	}
	    
 	/* One-Cycle VRAM */
 	read_xr(0x34, temp);
 	if (OFLG_ISSET(OPTION_FAST_VRAM, &vga256InfoRec.options))
 	    write_xr(0x34, temp | 0xC0); 
#ifdef DEBUG
 	ErrorF("SR34=%2x  ",temp);
#endif
 
 	/* pci burst */
 	read_xr(0x35, temp);
 	if (OFLG_ISSET(OPTION_PCI_BURST_ON, &vga256InfoRec.options)) 
 	    temp |= 0x10;
 	else if (OFLG_ISSET(OPTION_PCI_BURST_OFF, &vga256InfoRec.options))
 	    temp &= ~0x10;
#ifdef DEBUG
 	ErrorF("SR35=%2x  ",temp);
#endif
 	write_xr(0x35, temp);

 	/* pci burst,also */
	if (SISchipset != SIS530) {
		read_xr(0x26, temp);
		if (OFLG_ISSET(OPTION_PCI_BURST_ON, &vga256InfoRec.options)) 
			temp |= 0x20;
		else if (OFLG_ISSET(OPTION_PCI_BURST_OFF, &vga256InfoRec.options))
			temp &= ~0x20;
		write_xr(0x26, temp);
#ifdef DEBUG
		ErrorF("SR26=%2x  \n",temp);
#endif
	}
/* Merge FIFOs */	     
 	read_xr(0x07, temp);
 	    temp |= 0x80;
	write_xr(0x07, temp);
    };
}


/* 
 * SISProbe --
 * 	check whether a supported SiS board is installed
 */
static Bool
SISProbe()
{
  	int numClocks;
  	unsigned char temp;

	/*
         * Set up I/O ports to be used by this card
	 */
	xf86ClearIOPortList(vga256InfoRec.scrnIndex);
	xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);

	SISchipset = -1;

  	if (vga256InfoRec.chipset)
    	{
		/*
		 * If chipset from XF86Config doesn't match...
		 */
		if (!StrCaseCmp(vga256InfoRec.chipset, SISIdent(SIS86C201)))
			SISfamily = SISchipset = SIS86C201;
		else if (!StrCaseCmp(vga256InfoRec.chipset, SISIdent(SIS86C202)))
 			SISfamily = SISchipset = SIS86C202;
 		else if (!StrCaseCmp(vga256InfoRec.chipset, SISIdent(SIS86C205)))
 			SISfamily = SISchipset = SIS86C205;
 		else if (!StrCaseCmp(vga256InfoRec.chipset, SISIdent(SIS86C215)))
 			SISfamily = SISchipset = SIS86C205;
 		else if (!StrCaseCmp(vga256InfoRec.chipset, SISIdent(SIS86C225)))
 			SISfamily = SISchipset = SIS86C205;
 		else if (!StrCaseCmp(vga256InfoRec.chipset, SISIdent(SIS5597)) ||
 				!StrCaseCmp(vga256InfoRec.chipset, SISIdent(SIS5598)) )
 			SISfamily = SISchipset = SIS5597;
 		else if (!StrCaseCmp(vga256InfoRec.chipset, SISIdent(SIS6326)))
 			SISfamily = SISchipset = SIS6326;
 		else if (!StrCaseCmp(vga256InfoRec.chipset, SISIdent(SIS530))||
 			 !StrCaseCmp(vga256InfoRec.chipset, SISIdent(SIS620)) )
 			SISfamily = SISchipset = SIS530;
 		else if (!StrCaseCmp(vga256InfoRec.chipset, SISIdent(SIS300)))
 			{ SISchipset = SIS300;	SISfamily = SIS300; }
 		else if (!StrCaseCmp(vga256InfoRec.chipset, SISIdent(SIS630)))
 			{ SISchipset = SIS630;	SISfamily = SIS300; }
 		else if (!StrCaseCmp(vga256InfoRec.chipset, SISIdent(SIS540)))
 			{ SISchipset = SIS540;	SISfamily = SIS300; }
 		else
 			return(FALSE);
    	}
	else
	{
		if (vgaPCIInfo && vgaPCIInfo->Vendor == PCI_VENDOR_SIS)
		{
			switch(vgaPCIInfo->ChipType)
			{
			case PCI_CHIP_SG86C201: 	/* 86C201 */
				SISfamily = SISchipset = SIS86C201;
				break;
			case PCI_CHIP_SG86C202:		/* 86C202 */
				SISfamily = SISchipset = SIS86C202;
				break;
			case PCI_CHIP_SG86C205:		/* 86C205 */
				SISfamily = SISchipset = SIS86C205;
				break;
			case PCI_CHIP_SG86C215:		/* 86C215 */
				SISfamily = SISchipset = SIS86C205;
				break;
			case PCI_CHIP_SG86C225:		/* 86C225 */
				SISfamily = SISchipset = SIS86C205;
				break;
                        case PCI_CHIP_SIS5597:          /* 5597/5598 */
                                SISfamily = SISchipset = SIS5597;
                                break;
			case PCI_CHIP_SIS6326:		/* 6326 */
				SISfamily = SISchipset = SIS6326;
				break;
			case PCI_CHIP_SIS530:		/* 530/620 */
				SISfamily = SISchipset = SIS530;
				break;
			case PCI_CHIP_SIS300:		/* 300 */
				SISchipset = SIS300;
				SISfamily = SIS300;
				break;
			case PCI_CHIP_SIS630:
				SISchipset = SIS630;
				SISfamily = SIS300;
				break;
			case PCI_CHIP_SIS540:
				SISchipset = SIS540;
				SISfamily = SIS300;
				break;
			}
		}
		if (SISchipset == -1)
			return (FALSE);
		vga256InfoRec.chipset = SISIdent(SISchipset);
	}
	ErrorF("Using XFree86 SiS driver version %d.%d.%d\n",
			SIS_MAJOR_VERSION, SIS_MINOR_VERSION, SIS_PATCH_LEVEL);

	SISEnterLeave(ENTER);
	
 	/* 
	 * How much Video Ram have we got?
	 */
        if ((vga256InfoRec.videoRam > 4096) && (SISchipset == SIS6326))
	    vga256InfoRec.videoRam = 4096; /* Higher values don't work properly */
    	if (!vga256InfoRec.videoRam)
    	{
		unsigned char temp;
		unsigned char bsiz;
            if (SISchipset == SIS6326 || SISchipset == SIS530)  
		{
	    
		read_xr(0x0C,temp);
		temp >>= 1;
		switch (temp & 0x0B) 
		{
		case 0x00: 
    			vga256InfoRec.videoRam = 1024;
			break;
		case 0x01:
			vga256InfoRec.videoRam = 2048;
			break;
		case 0x02: 
			vga256InfoRec.videoRam = 4096;
			break;
		case 0x03: 
			if(SISchipset == SIS6326)
			    vga256InfoRec.videoRam = 1024;
			else
			    vga256InfoRec.videoRam = 0;
			break;
		case 0x08: 
    			vga256InfoRec.videoRam = 0;
			break;
		case 0x09:
			vga256InfoRec.videoRam = 2048;
			break;
		case 0x0A: 
			vga256InfoRec.videoRam = 4096;
			break;
		case 0x0B: 
			if(SISchipset == SIS530) {
				vga256InfoRec.videoRam = 8192;
			} else {
			/* Detected 8mb board, but we use 4MB for now */
			ErrorF("%s %s: Found 8MB board, treating like 4MB.\n",
			XCONFIG_PROBED, vga256InfoRec.name);
#if 0
			vga256InfoRec.videoRam = 8192;
#else
			vga256InfoRec.videoRam = 4096;
#endif
			}
			break;
		}
	    }		
	    else if (SISfamily == SIS300)  {
			unsigned char mem;
			read_xr(0x14,mem);
			mem &= 0x3F;
			vga256InfoRec.videoRam = 1024;
			while (mem) {
				if (mem & 1) {
					vga256InfoRec.videoRam <<= 1;
					mem >>= 1;
					continue;
				} else
					break;
			}
	    }
	    else
	    {
		if ( SISchipset == SIS5597)  
		{

		/* Because 5597 shares main memory, 
		   I test for BIOS CONFIGURED memory.*/

       		        read_xr(0x0C,bsiz);
		        bsiz = (bsiz >> 1) & 3;

       		        read_xr(0x2F,temp);
			temp &= 7;
			temp++;
			if (bsiz > 0) temp = temp << 1;

			vga256InfoRec.videoRam = 256 * temp;

		}
		else /* 86c20x */
		{
       		        read_xr(0x0F,temp);

			switch (temp & 0x03) 
			{
			case 0: 
				vga256InfoRec.videoRam = 1024;
				break;
			case 1:
				vga256InfoRec.videoRam = 2048;
				break;
			case 2: 
				vga256InfoRec.videoRam = 4096;
				break;
			}
		}
	    }
	    if ((SISfamily != SIS300) && (SISchipset != SIS530)
		&& (vga256InfoRec.videoRam > 4096))
	    {
	    	ErrorF("%s %s: Restricting video memory use to 4 MB\n",
		       XCONFIG_PROBED, vga256InfoRec.name);
		vga256InfoRec.videoRam = 4096;
	    }
     	}

#if defined(MONOVGA) || defined(XF86VGA16)
	if (!vga256InfoRec.clocks) {
            numClocks = 4;
	    vgaGetClocks(numClocks, SISClockSelect);
	}
#else

        /***************** Calculate here MClk *************************/
	/* Allow the user to override it. Dangerous!!. The BIOS should *
	 * set a proper Mclk, adequate for memory type                 */

 	if (SISchipset == SIS5597 || SISchipset == SIS6326) 
          if ( vga256InfoRec.MemClk ) 
             ErrorF("%s %s: User max MEM-clock of %d kHz overrides %d kHz BIOS setup\n",
               XCONFIG_GIVEN, vga256InfoRec.name, vga256InfoRec.MemClk,sisMClk());
	  
	/*
	 * If clocks are not specified in XF86Config file, probe for them
	 */

	if ( (OFLG_ISSET(OPTION_HW_CLKS, &vga256InfoRec.options)) ||
	     (SISchipset == SIS86C201) )  {
	    /* if sis86c201 force to use the hw clock 
	     * if programmable clock works with the sis86c201
	     * let us know
	     */
	    if (!vga256InfoRec.clocks) {
		numClocks = 32;
		vgaGetClocks(numClocks, SISClockSelect);
	    }
	}
	else {
	    OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions);
	    ErrorF("%s %s: using programmable clocks.\n",
		   XCONFIG_PROBED, vga256InfoRec.name);
	    if(!vga256InfoRec.clockprog)
		vga256InfoRec.clocks = 0;
	}


	/***************** maximal dot clock *************************/
        /*
         * Check for user provided. If not, establish to 135Mhz 
	 * except for 5597 and 6326, in which we use approx memory 
	 * bandwidth and 530/620, where we use the default 180MHz
	 */

        if ((SISchipset == SIS86C205) ||  (SISchipset == SIS86C202))
           vga256InfoRec.maxClock = 135000;
        else 
	  if (SISchipset == SIS6326) {
	    vga256InfoRec.maxClock = (sisMemBandWidth() / vgaBitsPerPixel);
	    if (vga256InfoRec.maxClock > 175500)
               vga256InfoRec.maxClock = 175500;
	  } else if (SISfamily == SIS300) {
		vga256InfoRec.maxClock = sis300MemBandWidth() / vgaBitsPerPixel;
	  } else if (SISchipset == SIS530) {
            vga256InfoRec.maxClock = 230000;
	    /* check the mem type installed */
	    read_xr(0x0e, temp);
	    ErrorF("%s %s: memory type installed %s\n",XCONFIG_PROBED, vga256InfoRec.name,
		   (temp & 0x80) ? "SGRAM" : "SDRAM");
	  } else 
	      if (SISchipset == SIS5597) {
	        vga256InfoRec.maxClock = (sisMemBandWidth() / vgaBitsPerPixel);
	        if (vga256InfoRec.maxClock > 135000)
                vga256InfoRec.maxClock = 135000;
              };
    
                                                                 
        if ( vga256InfoRec.dacSpeeds[0] ) {
          ErrorF("%s %s: user max dot-clock of %d kHz overrides %d kHz limit\n",     
             XCONFIG_GIVEN, vga256InfoRec.name, vga256InfoRec.dacSpeeds[0],             
	     vga256InfoRec.maxClock);	   
	  vga256InfoRec.maxClock = vga256InfoRec.dacSpeeds[0];
	};
	 
#endif	/* XF86VGA16*/

	vga256InfoRec.bankedMono = TRUE;
#ifndef MONOVGA
	/* We support Direct Video Access */
	vga256InfoRec.directMode = XF86DGADirectPresent;

	OFLG_SET(OPTION_HW_CURSOR, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_SW_CURSOR, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_HW_CLKS, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_LINEAR, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_MMIO, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_NOLINEAR_MODE, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_NO_BITBLT, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_NO_IMAGEBLT, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_NOACCEL, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_FAST_VRAM, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_PCI_BURST_ON, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_EXT_ENG_QUEUE, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_NO_WAIT, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_HOST_BUS, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_FIFO_AGGRESSIVE, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_FIFO_MODERATE, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_FIFO_CONSERV, &SIS.ChipOptionFlags);
	OFLG_SET(OPTION_EDO_VRAM, &SIS.ChipOptionFlags);
#else
	/* Set to 130MHz at 16 colours */
	vga256InfoRec.maxClock = 130000;
#endif
#ifdef DPMSExtension
	vga256InfoRec.DPMSSet = SISDisplayPowerManagementSet;
#endif

    	return(TRUE);
}

static unsigned long
sisPCIMemBase()
{

    if (vgaPCIInfo && vgaPCIInfo->Vendor == PCI_VENDOR_SIS) {
	if (vgaPCIInfo->ThisCard && vgaPCIInfo->ThisCard->_base0) {
	    return vgaPCIInfo->ThisCard->_base0 & 0xFFF80000;
	} else {
	    ErrorF("%s %s: %s: Can't find valid PCI "
		"Base Address\n", XCONFIG_PROBED,
		vga256InfoRec.name, vga256InfoRec.chipset);
	    return 0;
	}
    } else {
	ErrorF("%s %s: %s: Can't find PCI device in "
	    "configuration space\n", XCONFIG_PROBED,
	    vga256InfoRec.name, vga256InfoRec.chipset);
	return 0;
    }
}

static unsigned long
sisPCIMMIOBase()
{
    if (vgaPCIInfo && vgaPCIInfo->ThisCard && vgaPCIInfo->ThisCard->_base1)
	return vgaPCIInfo->ThisCard->_base1 & 0xFFFFFFF0;
    else
	return 0;
}

/*
 * SISScrnInit --
 *
 * Sets some accelerated functions
 */		
static int
SISScrnInit(pScreen, LinearBase, virtualX, virtualY, res1, res2, width)
ScreenPtr pScreen;
char *LinearBase;
int virtualX, virtualY, res1, res2, width;
{
#ifdef DEBUG
    ErrorF("SISScrnInit\n");
#endif
#if !defined(MONOVGA) && !defined(XF86VGA16)
    pScreen->CopyWindow = siscfbCopyWindow;
    pScreen->PaintWindowBackground = sisPaintWindow;
    pScreen->PaintWindowBorder = sisPaintWindow;
#endif	
    return(TRUE);
}

/*
 * SISFbInit --
 *	enable speedups for the chips that support it
 */
static void
SISFbInit()
{
#ifndef MONOVGA
	unsigned long j;
	unsigned long i;
	pointer sisVideoMem;
	long *poker;
	int offscreen_available;
	int offscreen_used = 0 ;
#ifdef DEBUG
	ErrorF("SISFbInit()\n");
#endif

	if (OFLG_ISSET(OPTION_LINEAR, &vga256InfoRec.options))
	{
	    ErrorF("%s %s: Enabling Linear Addressing\n",
		   XCONFIG_GIVEN, vga256InfoRec.name);
		sisUseLinear = TRUE;
	}
	if (OFLG_ISSET(OPTION_NOLINEAR_MODE, &vga256InfoRec.options))
	{
	    ErrorF("%s %s: Disabling Linear Addressing\n",
		   XCONFIG_GIVEN, vga256InfoRec.name);
		sisUseLinear = FALSE;
	}

	if ( sisUseLinear ) {
	    if (vga256InfoRec.MemBase != 0) {
		SIS.ChipLinearBase = vga256InfoRec.MemBase;
		ErrorF("%s %s: base address is set at 0x%X.\n",
		       XCONFIG_GIVEN, vga256InfoRec.name, SIS.ChipLinearBase);
	    }	
	    else {
		SIS.ChipLinearBase = sisPCIMemBase();
		if (SIS.ChipLinearBase == 0) {
		    unsigned long addr,addr2 ;

		    read_xr(0x21,addr);
		    addr &= 0x1f ;
		    addr <<= 27 ;
		    read_xr(0x20,addr2);
		    addr2 <<= 19 ;
		    addr |= addr2 ;
		    if ( addr == 0 )  {
			ErrorF("%s %s: Disabling Linear Addressing\n",
			       XCONFIG_PROBED, vga256InfoRec.name);
			ErrorF("%s %s:   Try to set MemBase in XF86Config\n",
			       XCONFIG_PROBED, vga256InfoRec.name);
			sisUseLinear = FALSE;
		    }
		    else {
			SIS.ChipLinearBase = addr ;
			ErrorF("%s %s: Trying Linear Addressing at 0x0%x\n",
			       XCONFIG_PROBED, vga256InfoRec.name,
			       SIS.ChipLinearBase);
		    }
		}
	    }
	}

	if ( sisUseLinear && xf86LinearVidMem() )
	{
	    SIS.ChipLinearSize = vga256InfoRec.videoRam * 1024;
	    ErrorF("%s %s: Using Linear Frame Buffer at 0x0%x, Size %dMB\n"
		   ,XCONFIG_PROBED, vga256InfoRec.name,
		   SIS.ChipLinearBase, SIS.ChipLinearSize/1048576);
	}	

	if (sisUseLinear) 
	    SIS.ChipUseLinearAddressing = TRUE;
	else 
	    SIS.ChipUseLinearAddressing = FALSE;

	if (sisUseMMIO && OFLG_ISSET(OPTION_NO_BITBLT,&vga256InfoRec.options)){
	    sisUseMMIO = FALSE ;
	    ErrorF("%s %s: SIS: Bit Block Transfert disabled\n",
		   OFLG_ISSET(OPTION_NO_BITBLT, &vga256InfoRec.options) ?
		   XCONFIG_GIVEN : XCONFIG_PROBED, vga256InfoRec.name); 
	}

	if ( sisUseMMIO || (SISfamily == SIS300)){
	    PCIMMIOBase = sisPCIMMIOBase() ;
	    if (SISfamily != SIS300)
		sisUseMMIO = TRUE ;
	    if ( PCIMMIOBase == 0 ) {
		/* use default base */
		if ( sisUseLinear) 
		    /* sisMMIOBase = vgaBase , but not yet mapped here */
		    PCIMMIOBase = 0xA0000 ;
		else {
		    PCIMMIOBase = 0xB0000 ;
		    sisMMIOBase = xf86MapVidMem(vga256InfoRec.scrnIndex, 
					    MMIO_REGION,
					    (pointer)(PCIMMIOBase), 0x10000L);
		}
	    } else {
		sisMMIOBase = xf86MapVidMem(vga256InfoRec.scrnIndex, 
					    MMIO_REGION,
					    (pointer)(PCIMMIOBase), 0x10000L);
	    }
	    ErrorF("%s %s: SIS: Memory mapped I/O selected at 0x0%x\n",
		   OFLG_ISSET(OPTION_MMIO, &vga256InfoRec.options) ?
		   XCONFIG_GIVEN : XCONFIG_PROBED, 
		   vga256InfoRec.name,PCIMMIOBase);
	}


	SISDisplayableMemory = vga256InfoRec.displayWidth
				* vga256InfoRec.virtualY
				* (vgaBitsPerPixel / 8);

	offscreen_available = vga256InfoRec.videoRam * 1024 - 
					SISDisplayableMemory ;
	

	if (OFLG_ISSET(OPTION_HW_CURSOR, &vga256InfoRec.options) ||
	    !OFLG_ISSET(OPTION_SW_CURSOR, &vga256InfoRec.options) )
	{
	    OFLG_SET(OPTION_HW_CURSOR, &vga256InfoRec.options) ;
	    /* SiS needs upper 16K for hardware cursor */
           if (offscreen_available < 16384)
		ErrorF("%s %s: Not enough off-screen video"
		       " memory for hardware cursor,"
		       " using software cursor.\n",
		       XCONFIG_PROBED, vga256InfoRec.name);
	    else {
		SISCursorWidth = 64;
		SISCursorHeight = 64;
		vgaHWCursor.Initialized = TRUE;
		vgaHWCursor.Init = SISCursorInit;
		vgaHWCursor.Restore = SISRestoreCursor;
		vgaHWCursor.Warp = SISWarpCursor;
		vgaHWCursor.QueryBestSize = SISQueryBestSize;
		sisHWCursor = TRUE;
		ErrorF("%s %s: Using hardware cursor\n",
		       XCONFIG_GIVEN, vga256InfoRec.name);
		/* new offscreen_available */
		offscreen_available -= 16384 ;
                offscreen_used += 16384 ;
	    }
	}

	/* Enable Turbo-queue */
        if (OFLG_ISSET(OPTION_EXT_ENG_QUEUE, &vga256InfoRec.options))
 	    if ( ((SISchipset == SIS5597) || (SISchipset == SIS6326) || 
		  (SISchipset == SIS530)) && 
		 (offscreen_available >= (sisHWCursor ? 49152 : 32768)) )  {
		ErrorF("%s %s: SIS: Enabling Turbo-queue \n",XCONFIG_GIVEN, vga256InfoRec.name); 
		offscreen_available -= 32768 ;
		offscreen_used += 32768 ;

		if (sisHWCursor) { /* alignment loss */
     		    offscreen_available -= 16384 ;
		    offscreen_used += 16384 ;
		    }
		    
		sisTurboQueue = TRUE;

		turbo_queue_address = vga256InfoRec.videoRam - (offscreen_used / 1024);
	    }
	else {
 	    sisTurboQueue = FALSE;
	}

	/* Enable Turbo-queue for SiS 300 */

	if ((SISfamily == SIS300) &&
		(offscreen_available >= (sisHWCursor ? 114688: 65536))){
		/* Turbo Queue size is 64K */
		offscreen_available -= 65536;
		offscreen_used += 65536;

		if (sisHWCursor) { /* alignment loss */
     		    offscreen_available -= 49152;
		    offscreen_used += 49152;
		    }
		    
		sisTurboQueue = TRUE;
		turbo_queue_address = vga256InfoRec.videoRam
					- (offscreen_used / 1024);
		ErrorF("%s %s: SIS: Enabling Turbo-queue\n",
			XCONFIG_GIVEN, vga256InfoRec.name); 
	}
    
	if (OFLG_ISSET(OPTION_NO_IMAGEBLT, &vga256InfoRec.options)) {
	  ErrorF("%s %s: SIS: Not using mono expand system-to-video BitBLT.\n",
		 XCONFIG_GIVEN, vga256InfoRec.name);
	    sisAvoidImageBLT = TRUE;
	}

	if (sisUseMMIO) {
	   if (!OFLG_ISSET(OPTION_NOACCEL, &vga256InfoRec.options)) {
	       if ( !sisUseLinear || 
		   OFLG_ISSET(OPTION_XAA_NO_COL_EXP, &vga256InfoRec.options))
		   sisUseXAAcolorExp = FALSE;
	       if ((SISchipset != SIS530) && (SISfamily != SIS300))
		 SISAccelInit();
	       else
		 SIS2AccelInit();
	   }
	   else if ( sisUseLinear ) {
	       ErrorF("%s %s: SIS: using old accelerated functions.\n",
		 XCONFIG_PROBED, vga256InfoRec.name);
	    switch (vgaBitsPerPixel) {
	      case 8:

		vga256LowlevFuncs.doBitbltCopy = siscfbDoBitbltCopy;
		vga256LowlevFuncs.vgaBitblt = sisMMIOBitBlt;
		
		vga256LowlevFuncs.fillRectSolidCopy = sisMMIOFillRectSolid;

		vga256LowlevFuncs.fillBoxSolid = siscfbFillBoxSolid;
		vga256TEOps1Rect.FillSpans = sisMMIOFillSolidSpansGeneral;
		vga256TEOps.FillSpans = sisMMIOFillSolidSpansGeneral;
		vga256LowlevFuncs.fillSolidSpans =sisMMIOFillSolidSpansGeneral;

		/* Setup the address of the tile/stipple in vram. 
		 * be aligned on a 64 bytes value. Size of the space
		 * is 1024 */
		/* in the future it might be better to keep all the offscreen
		   memory for cache pixmap/bitmap
		 */
		if (offscreen_available < 1024) {
			ErrorF("%s %s: Not enough off-screen video"
			       " memory for expand color."
			       " using builin pattern reg for 512 pixels\n",
			       XCONFIG_PROBED, vga256InfoRec.name);
		    sisBLTPatternOffscreenSize = 
			((SISchipset == SIS86C205) || (SISchipset == SIS5597)||(SISchipset == SIS6326)||(SISchipset == SIS530)) ? 512 : 256  ;
		    }			
		else {
		    int CursorSize = sisHWCursor?16384:0 ;
		    sisBLTPatternAddress = vga256InfoRec.videoRam * 1024 
			- offscreen_used - 1024;
		    sisBLTPatternOffscreenSize = 1024 ;
		}
		/* Hook special op. fills (and tiles): */
		vga256TEOps1Rect.PolyFillRect = siscfbPolyFillRect;
		vga256NonTEOps1Rect.PolyFillRect = siscfbPolyFillRect;
		vga256TEOps.PolyFillRect = siscfbPolyFillRect;
		vga256NonTEOps.PolyFillRect = siscfbPolyFillRect;

		if (!OFLG_ISSET(OPTION_NO_IMAGEBLT,
				&vga256InfoRec.options)) {
		    vga256TEOps1Rect.PolyGlyphBlt = sisMMIOPolyGlyphBlt;
		    vga256TEOps.PolyGlyphBlt = sisMMIOPolyGlyphBlt;
		    vga256LowlevFuncs.teGlyphBlt8 = sisMMIOImageGlyphBlt;
		    vga256TEOps1Rect.ImageGlyphBlt = sisMMIOImageGlyphBlt;
		    vga256TEOps.ImageGlyphBlt = sisMMIOImageGlyphBlt;
		}

		break;
	      case 16:
		/* There are no corresponding structures to vga256LowlevFuncs
		 * for 16/24bpp. Hence we have to hook to the cfb functions
		 * in a similar way to the cirrus driver. For now I've just
		 * implemented the most basic of blits */

		cfb16TEOps1Rect.CopyArea = siscfb16CopyArea;
		cfb16TEOps.CopyArea = siscfb16CopyArea;
		cfb16NonTEOps1Rect.CopyArea = siscfb16CopyArea;
		cfb16NonTEOps.CopyArea = siscfb16CopyArea;

		cfb16TEOps1Rect.FillSpans = sisMMIOFillSolidSpansGeneral;
		cfb16TEOps.FillSpans = sisMMIOFillSolidSpansGeneral;
		cfb16NonTEOps1Rect.FillSpans = sisMMIOFillSolidSpansGeneral;
		cfb16NonTEOps.FillSpans = sisMMIOFillSolidSpansGeneral;
		
		/* Setup the address of the tile/stipple in vram. 
		 * be aligned on a 64 bytes value. Size of the space
		 * is 1024 */
		/* in the future it might be better to keep all the offscreen
		   memory for cache pixmap/bitmap
		 */
		if (offscreen_available < 1024) {
			ErrorF("%s %s: Not enough off-screen video"
			       " memory for expand color."
			       " using builin pattern reg for 512 pixels\n",
			       XCONFIG_PROBED, vga256InfoRec.name);
			sisBLTPatternOffscreenSize = 
			    ((SISchipset == SIS86C205) || (SISchipset == SIS5597) || (SISchipset == SIS6326)||(SISchipset == SIS530)) ? 512 : 256  ;
		    }
		else {
		    int CursorSize = sisHWCursor?16384:0 ;
		    sisBLTPatternAddress = vga256InfoRec.videoRam * 1024 
			- offscreen_used - 1024;
		    sisBLTPatternOffscreenSize = 1024 ;
		}
		cfb16TEOps1Rect.PolyFillRect = siscfbPolyFillRect;
		cfb16TEOps.PolyFillRect = siscfbPolyFillRect;
		cfb16NonTEOps1Rect.PolyFillRect = siscfbPolyFillRect;
		cfb16NonTEOps.PolyFillRect = siscfbPolyFillRect;

		if (!OFLG_ISSET(OPTION_NO_IMAGEBLT,
				&vga256InfoRec.options)) {
		    cfb16TEOps1Rect.PolyGlyphBlt = sisMMIOPolyGlyphBlt;
		    cfb16TEOps.PolyGlyphBlt = sisMMIOPolyGlyphBlt;
		    cfb16TEOps1Rect.ImageGlyphBlt = sisMMIOImageGlyphBlt;
		    cfb16TEOps.ImageGlyphBlt = sisMMIOImageGlyphBlt;
		}

		break ;
	      case 24:
		cfb24TEOps1Rect.CopyArea = siscfb24CopyArea;
		cfb24TEOps.CopyArea = siscfb24CopyArea;
		cfb24NonTEOps1Rect.CopyArea = siscfb24CopyArea;
		cfb24NonTEOps.CopyArea = siscfb24CopyArea;
		
		cfb24TEOps1Rect.FillSpans = sisMMIOFillSolidSpansGeneral;
		cfb24TEOps.FillSpans = sisMMIOFillSolidSpansGeneral;
		cfb24NonTEOps1Rect.FillSpans = sisMMIOFillSolidSpansGeneral;
		cfb24NonTEOps.FillSpans = sisMMIOFillSolidSpansGeneral;
		
		/* Setup the address of the tile/stipple in vram. 
		 * be aligned on a 64 bytes value. Size of the space
		 * is 1024 */
		/* in the future it might be better to keep all the offscreen
		   memory for cache pixmap/bitmap
		 */
		if (offscreen_available < 1024) {
			ErrorF("%s %s: Not enough off-screen video"
			       " memory for expand color."
			       " using builin pattern reg for 512 pixels\n",
			       XCONFIG_PROBED, vga256InfoRec.name);
			sisBLTPatternOffscreenSize = 
  			    ((SISchipset == SIS86C205) || (SISchipset == SIS5597) || (SISchipset == SIS6326)||(SISchipset == SIS530)) ? 512 : 256  ;
		    }
		else {
		    int CursorSize = sisHWCursor?16384:0 ;
		    sisBLTPatternAddress = vga256InfoRec.videoRam * 1024 
			- offscreen_used - 1024;
		    sisBLTPatternOffscreenSize = 1024 ;
		}
		cfb24TEOps1Rect.PolyFillRect = siscfbPolyFillRect;
		cfb24TEOps.PolyFillRect = siscfbPolyFillRect;
		cfb24NonTEOps1Rect.PolyFillRect = siscfbPolyFillRect;
		cfb24NonTEOps.PolyFillRect = siscfbPolyFillRect;

		/* the enhanced color expansion is not supported
		 * by the engine in 16M-color graphic mode.
		 */
		sisAvoidImageBLT = TRUE;

		break;

	    }
	    vgaSetScreenInitHook(SISScrnInit);
	   }

       }		

#endif /* MONOVGA */
}

/*
 * SISEnterLeave --
 * 	enable/disable io-mapping
 */
static void
SISEnterLeave(enter)
	Bool enter;
{
  	unsigned char temp;
#ifdef DEBUG
    ErrorF("SISEnterLeave(");
    if (enter)
	ErrorF("Enter)\n");
    else
	ErrorF("Leave)\n");
#endif

#ifndef MONOVGA
#ifdef XFreeXDGA
	if (vga256InfoRec.directMode & XF86DGADirectGraphics && !enter)
	if (OFLG_ISSET(OPTION_HW_CURSOR, &vga256InfoRec.options))
		SISHideCursor();
#endif
#endif

  	if (enter)
    	{
      		xf86EnableIOPorts(vga256InfoRec.scrnIndex);
		vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
      		outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
      		outb(vgaIOBase + 5, temp & 0x7F);

		outw(0x3c4, 0x8605); 	/* Unlock extended registers */
    	}
  	else
    	{
#ifndef MONOVGA
		if (SISfamily == SIS300) { /* hide cursor */
			unsigned char tmp;
			int mapped = 1;
			outb(0x3c4, 5);
			if (inb(0x3c5) == 0x21)
				outb(0x3c5, 0x86);

			outb(0x3c4, 0x20);
			if (!(inb(0x3c5)&1)) {
				mapped = 0;
				tmp = inb(0x3c4)|1;
				outb(0x3c5, tmp);
			}

			SISHideCursor();

			if (!mapped) {
				outb(0x3c4, 0x20);
				tmp = inb(0x3c5)&0xFE;
				outb(0x3c5, tmp);
			}
		}
#endif

		outw(0x3c4, 0x0005);	/* Lock extended registers */

      		xf86DisableIOPorts(vga256InfoRec.scrnIndex);

    	}
}

/*
 * SISRestore --
 *      restore a video mode
 */
static void
SISRestore(restore)
     	vgaSISPtr restore;
{
    int i, last;

#ifdef DEBUG
    ErrorF("SISRestore\n");
#endif
    vgaProtect(TRUE);
#ifdef DEBUG
    ErrorF("Restoring extended registers\n");
#endif
/* SR5 is Password/identification. If restored to A1 then 
   the rest of the registers can't be written */

    restore->Port_3C4[5]=0x86;
   
    switch(SISchipset) {
    case SIS5597: last = SIS5597_3C5_LAST;
	break;
    case SIS6326: last = SIS6326_3C5_LAST;
	break;
    case SIS530:  last = SIS530_3C5_LAST;
	break;
    case SIS630:
    case SIS540:
		outw(0x3d4, restore->Port_3D4[0x19] << 8 | 0x19);
		outw(0x3d4, restore->Port_3D4[0x1A] << 8 | 0x1a);
    case SIS300:  last = SIS300_3C5_LAST;
	break;
    default:      last = DEFAULT_3C5_LAST;
	break;
    }
    if ((SISchipset == SIS630) && (restore->Port_3C4[0x1E] & 0x40))
	outw(0x3c4, restore->Port_3C4[0x20] << 8 | 0x20);

    for (i = 5 ; i <= last; i++) {
	outb(0x3c4, i);
#ifdef IO_DEBUG
	    ErrorF("XR%X Contents - %X ", i, inb(0x3c5));
#endif
	if (inb(0x3c5) != restore->Port_3C4[i])
	    outb(0x3c5,restore->Port_3C4[i]);
#ifdef IO_DEBUG
	    ErrorF("Restore to - %X Read after - %X\n",restore->Port_3C4[i], inb(0x3c5));
#endif
    }

    /* set the clock */
    if ( ClockProgramable() ) {
	if (restore->std.NoClock >= 0)
	    sisClockLoad(&restore->sisClock);
    }
    else
	outw(0x3c4, ((restore->ClockReg) << 8) | 0x07);

    /*
     * Now restore generic VGA Registers
     */
    vgaHWRestore((vgaHWPtr)restore);

    outb(0x3C2, restore->ClockReg2);

    vgaProtect(FALSE);
}

/*
 * SISSave --
 *      save the current video mode
 */
static void *
SISSave(save)
     	vgaSISPtr save;
{
	int i, last;
#ifdef DEBUG
    ErrorF("SISSave\n");
#endif
  	save = (vgaSISPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgaSISRec));

	switch(SISchipset) {
	case SIS5597: last = SIS5597_3C5_LAST;
		break;
	case SIS6326: last = SIS6326_3C5_LAST;
		break;
	case SIS530:  last = SIS530_3C5_LAST;
		break;
	case SIS630:
	case SIS540:
		outb(0x3d4, 0x19);
		save->Port_3D4[0x19] = inb(0x3d5);
		outb(0x3d4, 0x1a);
		save->Port_3D4[0x1A] = inb(0x3d5);
	case SIS300:  last = SIS300_3C5_LAST;
		break;
	default:      last = DEFAULT_3C5_LAST;
		break;
	}

	for (i = 6 ; i <= last; i++) {
		outb(0x3c4, i);
		save->Port_3C4[i] = inb(0x3c5) ;
#ifdef IO_DEBUG
	ErrorF("XS%X - %X\n", i, inb(0x3c5));
#endif
	}

	save->ClockReg2 = inb(0x3CC);

	/* save clock */
	if ( ClockProgramable() )
	    sisClockSave(&save->sisClock);

  	return ((void *) save);
}
struct QConfig  {
	int	GT;
	int	QC;
};

struct	QConfig qconfig[20] = {
	{1, 0x0}, {1, 0x2}, {1, 0x4}, {1, 0x6}, {1, 0x8},
	{1, 0x3}, {1, 0x5}, {1, 0x7}, {1, 0x9}, {1, 0xb},
	{0, 0x0}, {0, 0x2}, {0, 0x4}, {0, 0x6}, {0, 0x8},
	{0, 0x3}, {0, 0x5}, {0, 0x7}, {0, 0x9}, {0, 0xb}};

int cycleA[20][2] = {
	{88,88}, {80,80}, {78,78}, {72,72}, {70,70},
	{79,72}, {77,70}, {71,64}, {69,62}, {49,44},
	{73,78}, {65,70}, {63,68}, {57,62}, {55,60},
	{64,62}, {62,60}, {56,54}, {54,52}, {34,34}};

static BOOL
Find630CRT_CPUthreshold(
	int	vclk, /* in MHz */
	int	mclk, /* in MHz */
	unsigned char sr14,
	unsigned char *sr16, /* in/out */
	unsigned char *sr8,  /* out */
	unsigned char *sr9,  /* out */
	unsigned char *srF)  /* out */
{
	unsigned int	lowa, low, bpp, cyclea;
	unsigned int	i, j;
	pciTagRec NBridge;
	CARD32	temp;

	switch(vgaBitsPerPixel){
	case 16: bpp = 2;
		break;
	case 24: /* 24 bpp not support */
	case 32:
		bpp = 4;
		break;
	case 8:
	default:
		bpp = 1;
	}

	i = 0;
	j = (sr14 >> 6) - 1;

	while (1)  {
#ifdef	DEBUG
		ErrorF("Config %d GT = %d, QC = %x, CycleA = %d\n",
			i, qconfig[i].GT, qconfig[i].QC, cycleA[i][j]);
#endif
		cyclea = cycleA[i][j];
		lowa = cyclea * vclk * bpp;
		lowa = (lowa + (mclk-1)) / mclk;
		lowa = (lowa + 15) / 16;
		low = lowa + 1;
		if (low <= 0x13)
			break;
		else
			if (i < 19)
				i++;
			else  {
				low = 0x13;
#ifdef	DEBUG
	ErrorF("This mode may has threshold problem and had better removed\n");
#endif
				break;
			}
	}
#ifdef	DEBUG
	ErrorF("Using Config %d with CycleA = %d\n", i, cyclea);
#endif
	*sr8 = ((low & 0xF) << 4) | 0xF;
	*srF = (low & 0x10) << 1;
	*sr9 = lowa + 4;
	if (*sr9 > 15)
		*sr9 = 15;

	/* write PCI configuration space */
	NBridge = pcibusTag(0, 0, 0);
	temp = pcibusRead(NBridge, 0x50);
	temp &= 0xF0FFFFFF;
	temp |= qconfig[i].QC << 24;
	pcibusWrite(NBridge, 0x50, temp);

	temp = pcibusRead(NBridge, 0xA0);
	temp &= 0xF0FFFFFF;
	temp |= qconfig[i].GT << 24;
	pcibusWrite(NBridge, 0xA0, temp);

#ifdef	DEBUG
	temp = pcibusRead(NBridge, 0x50);
	ErrorF("QueueConfig: 0x80000050 = %x\n", temp);
	temp = pcibusRead(NBridge, 0xA0);
	ErrorF("QueueConfig: 0x800000A0 = %x\n", temp);
#endif

	return TRUE;
}

struct funcargc {
	char	base;
	char	inc;
};

struct funcargc funca[12] = {
	{61, 3}, {52, 5}, {68, 7}, {100, 11},
	{43, 3}, {42, 5}, {54, 7}, {78, 11},
	{34, 3}, {37, 5}, {47, 7}, {67, 11}};

struct funcargc funcb[12] = {
	{81, 4}, {72, 6}, {88, 8}, {120, 12},
	{55, 4}, {54, 6}, {66, 8}, {90, 12},
	{42, 4}, {45, 6}, {55, 8}, {75, 12}};

char timing[8] = {1, 2, 2, 3, 0, 1, 1, 2};

static void
Find300CRT_CPUthreshold(
	int	vclk, /* in MHz */
	int	mclk, /* in MHz */
	unsigned char sr14,
	unsigned char *sr16, /* in/out */
	unsigned char sr18,
	unsigned char *sr8,  /* out */
	unsigned char *sr9,  /* out */
	unsigned char *srF)  /* out */
{
	unsigned char i,j,bpp;
	int lowa, lowb, low;
	struct funcargc *p;

	switch(vgaBitsPerPixel){
	case 16: bpp = 2;
		break;
	case 24: /* 24 bpp not support */
	case 32:
		bpp = 4;
		break;
	case 8:
	default:
		bpp = 1;
	}

	do {
		i = ((sr18 >> 4) & 0x06) | ((sr18 >> 1) & 1);
		j = ((sr14 >> 4) & 0x0c) | ((*sr16 >> 6) & 3);
		p = &funca[j];

		lowa = (p->base + p->inc*timing[i])*vclk*bpp;
		lowa = (lowa + (mclk-1)) / mclk;
		lowa = (lowa + 15)/16;

		p = &funcb[j];
		lowb = (p->base + p->inc*timing[i])*vclk*bpp;
		lowb = (lowb + (mclk-1)) / mclk;
		lowb = (lowb + 15)/16;

		if (lowb < 4)
			lowb = 0;
		else
			lowb -= 4;

		low = (lowa > lowb)? lowa: lowb;

		low++;

		if (low <= 0x13) {
			break;
		} else {
			i = (*sr16 >> 6) & 3;
			if (!i) {
				low = 0x13;
				break;
			} else {
				i--;
				*sr16 = (*sr16 & 0x3c) | (i << 6);
			}
		}
	} while (1);

	*sr8 = ((low & 0xF) << 4) | 0xF;
	*srF = (low & 0x10) << 1;
	*sr9 = low + 3;
	if (*sr9 > 15)
		*sr9 = 15;
}

/*
 *	In graphic mode: data ---------> to selector switch
 *
 * Now, let's look at the selector switch
 *
 *                                         FIFO
 *                             MCLK    ________________   VCLK
 * cpu/engine  <---o        o-------->|________________|---------> CRT
 *                 ^       ^             ^         ^
 *                  \     /              |         |
 *                   \   /               |         |
 *                    \ /                |         |
 *             selector switch   Threshold low Threshold high
 *
 * CRT consumes the data in the fifo. When data in FIFO reaches the 
 * level of  threshold low, the selector will switch to right so that the 
 * FIFO can be filled by data. When the data in FIFO  reaches the threshold 
 * high level, the selector will switch back to left.
 * The threshold low must  be set to a minimum level or the snow 
 * phenomenon (flicker noise) will be found on the screen.
 *
 * The threshold low should increase if  bpp increased, cause it means the 
 * required data for CRT is increased when bpp increased. When the threshold 
 * low  increased, the distance between threshold high and thereshold low 
 * should not be too near, else it will have the selector switch frequently, 
 * a bad performance result.
 */
static int
FindCRT_CPUthreshold(dotClock,bpp,thresholdLow,thresholdHigh)
int dotClock ;
int bpp;
int *thresholdLow;
int *thresholdHigh;
{
    unsigned char temp;
    unsigned char temp2;
    int mclk;
    int safetymargin, gap;
    float z, z1, z2; /* Yeou */
    int factor;
    

    /* here is an hack to set the CRT/CPU threshold register.
       the value in the three arrays come from Dump Registers in W95
     */
    struct ThresholdREC {
	int freq;
	int thresholdHigh;
	int thresholdLow;
    } ;

#ifdef OldThresholds
/*
    static struct ThresholdREC threshold8[]={{25250,0x6,0x3},{31500,0x6,0x3},
					     {40000,0x7,0x4},{44900,0x7,0x4},
					     {56250,0x7,0x4},
					     {65500,0x8,0x5},{78750,0x9,0x6},
					     {95000,0xa,0x7},{110000,0xd,0xb},
					     {135000,0xc,0xa}};
*/
    /* adjusted for X11 */
    /* this is the best values I've found.
     * there is no glitch but with intensive generation snow still appears 
     * on screen  especially well seen on the tiled root window.
     */
    static struct ThresholdREC threshold8[]={{25250,0x6,0x3},{31500,0x6,0x3},
					     {40000,0x7,0x4},{44900,0x7,0x4},
					     {56250,0x9,0x5},
					     {65000,0xe,0x5},
                                             {78750,0xd,0x6},{85000,0xd,0x6},
					     {95000,0xb,0x7},{110000,0xc,0xa},
					     {135000,0xc,0xa}};
/*
    static struct ThresholdREC threshold16[]={{25250,0x8,0x5},{31500,0x8,0x5},
					      {40000,0xa,0x7},{44900,0xa,0x7},
					      {56250,0xb,0x8},
					      {65500,0xc,0xa},{78750,0xe,0xc},
					      {95000,0xf,0xd}};
*/
    static struct ThresholdREC threshold16[]={{25250,0x8,0x5},{31500,0x8,0x5},
					      {40000,0xa,0x7},{44900,0xa,0x7},
					      {56250,0xf,0x7},
					      {65000,0xf,0x7},{78750,0xf,0x8},
                                              {95000,0xf,0xd},{110000,0xf,0xd}};

    static struct ThresholdREC threshold24[]={{25250,0xa,0x7},{31500,0xa,0x7},
                                              {40000,0xc,0x9},{56250,0xe,0xc}};

    int nfreq ;
    int i;
    struct ThresholdREC *thresholdTab;

    switch ( bpp ) {
      case 8:
	thresholdTab = threshold8 ;
	nfreq = sizeof(threshold8)/sizeof(struct ThresholdREC);
	break;
      case 16:
	thresholdTab = threshold16 ;
	nfreq = sizeof(threshold16)/sizeof(struct ThresholdREC);
	break;
      case 24:
	thresholdTab = threshold24 ;
	nfreq = sizeof(threshold24)/sizeof(struct ThresholdREC);
	break;
      default:
	thresholdTab = threshold8 ;
	nfreq = sizeof(threshold8)/sizeof(struct ThresholdREC);
    }	    

    for ( i = 0 ; i < nfreq ; i++ ) 
	if ( thresholdTab[i].freq >= dotClock ) break ;
    if ( i == 0 ) {
	 *thresholdLow = thresholdTab[0].thresholdLow ;
	 *thresholdHigh = thresholdTab[0].thresholdHigh ;
	 return ;
     }
    if ( i == nfreq ) { /* nothing found */
	*thresholdLow = thresholdTab[nfreq -1].thresholdLow ;
	*thresholdHigh = thresholdTab[nfreq -1].thresholdHigh ;
    }
    else {
    	*thresholdLow = thresholdTab[i-1].thresholdLow + 
	    ((thresholdTab[i].thresholdLow - thresholdTab[i-1].thresholdLow) *
	     (dotClock - thresholdTab[i-1].freq)) / 
		 ( thresholdTab[i].freq - thresholdTab[i-1].freq) ;
	*thresholdHigh = thresholdTab[i-1].thresholdHigh + 
	    ((thresholdTab[i].thresholdHigh - thresholdTab[i-1].thresholdHigh)*
	     (dotClock - thresholdTab[i-1].freq)) / 
		 ( thresholdTab[i].freq - thresholdTab[i-1].freq) ; 

    }

#ifdef DEBUG
    ErrorF("Old FindCRT_CPUthreshold(%d, %d) = 0x%x 0x%x\n", dotClock, bpp,
	   *thresholdLow,*thresholdHigh);
#endif
#else /*OldThresholds*/
    /* Juanjo Santamarta */
    
    mclk=sisMClk();
    
    if (SISchipset == SIS530) /* Yeou for 530 thresholod */
    {
        /* z = f *((dotClock * bpp)/(buswidth*mclk);
           thresholdLow  = (z+1)/2 + 4;
           thresholdHigh = 0x1F;

           where f = 0x60 when UMA (SR0D D[0] = 1)
                     0x30      LFB (SR0D D[0] = 0)
        */
        read_xr (0x0d, temp);
        if (temp & 0x01) factor = 0x60;
        else factor = 0x30;

        z1 = (float)(dotClock * bpp);
        z2 = (float)(64.0 * mclk);
        z = ((float) factor * (z1 / z2));
        *thresholdLow = ((int)z + 1) / 2 + 4 ;
	if (*thresholdLow > 0x1F)
		*thresholdLow = 0x1F;
        *thresholdHigh = 0x1F;
    }
    else {   /* For sis other product */
     /* Adjust thresholds. Safetymargin is to be adjusted by fifo_XXX 
	options. Try to mantain a fifo margin of gap. At high Vclk*bpp
	this isn't possible, so limit the thresholds. 
       
	The values I guess are :

	FIFO_CONSERVATIVE : safetymargin = 5 ;
	FIFO_MODERATE     : safetymargin = 3 ;
	Default           : safetymargin = 1 ;  (good enough in many cases) 
	FIFO_AGGRESSIVE   : safetymargin = 0 ;
	 
	gap=4 seems to be the best value in either case...
     */
    
	if (OFLG_ISSET(OPTION_FIFO_CONSERV, &vga256InfoRec.options)) 
		safetymargin=5; 
	else if (OFLG_ISSET(OPTION_FIFO_MODERATE, &vga256InfoRec.options)) 
		safetymargin=3; 
	else if (OFLG_ISSET(OPTION_FIFO_AGGRESSIVE, &vga256InfoRec.options)) 
		safetymargin=0; 
	else 
		safetymargin=1; 

	gap = 4;
	*thresholdLow = ((bpp*dotClock) / mclk)+safetymargin;
	*thresholdHigh = ((bpp*dotClock) / mclk)+gap+safetymargin;

	/* 24 bpp seems to need lower FIFO limits. 
	   At 16bpp is possible to put a thresholdHigh of 0 (0x10) with
	   good results on my system (good performance,
	   and virtually no noise) */
	if ( *thresholdLow > (bpp < 24 ? 0xe:0x0d) ) {
		*thresholdLow = (bpp < 24 ? 0xe:0x0d);
	}

	if ( *thresholdHigh > (bpp < 24 ? 0x10:0x0f) ) {
		*thresholdHigh = (bpp < 24 ? 0x10:0x0f);
	}
    }

#ifdef DEBUG
    ErrorF("FindCRT_CPUthreshold(%d, %d) = 0x%x 0x%x\n", dotClock, bpp,
	   *thresholdLow,*thresholdHigh);
    ErrorF("%s %s: %s: MClk = %d Hz\n", XCONFIG_PROBED,
	    vga256InfoRec.name, vga256InfoRec.chipset, sisMClk()*1000); 
#endif
#endif
}


SIS300Init(mode)
	DisplayModePtr mode;
{
	static unsigned char orig_sr16 = 0;
	static int orig_sr16_save = 0;
	unsigned char temp,sr8,sr9,srF,sr14,sr16,sr18;
	int offset;
	int i;
	unsigned int	CRT_CPUthresholdLow ;
	unsigned int	CRT_CPUthresholdHigh ;
	unsigned char	CRT_ENGthreshold ;
        unsigned int vclk[5];

#if !defined(MONOVGA) && !defined(XF86VGA16)
	offset = vga256InfoRec.displayWidth >>
#ifdef MONOVGA
		(mode->Flags & V_INTERLACE ? 3 : 4);
#else
		(mode->Flags & V_INTERLACE ? 2 : 3);
#endif

	/* some generic settings */
	new->std.Attribute[0x10] = 0x01;   /* mode */
	new->std.Attribute[0x11] = 0x00;   /* overscan (border) color */
	new->std.Attribute[0x12] = 0x0F;   /* enable all color planes */
	new->std.Attribute[0x13] = 0x00;   /* horiz pixel panning 0 */

	if ( (vgaBitsPerPixel == 16) || (vgaBitsPerPixel == 24) ||
	     (vgaBitsPerPixel == 32))
	    new->std.Graphics[0x05] = 0x00;    /* normal read/write mode */

	if (vgaBitsPerPixel == 16) {
	    offset <<= 1;	       /* double the width of the buffer */
	} else if (vgaBitsPerPixel == 24) {
	    offset += offset << 1;
	} else if (vgaBitsPerPixel == 32) {
	    offset <<= 2;
	} 

	if (! orig_sr16_save) {
		orig_sr16 = new->Port_3C4[0x16];
		orig_sr16_save = 1;
	}

	new->Port_3C4[0x06] = 0x02;

	if (mode->Flags & V_INTERLACE)
		new->Port_3C4[0x06] |= 0x20;

	if (vgaBitsPerPixel == 32)
		new->Port_3C4[0x06] |= 0x10;
	else if (vgaBitsPerPixel == 24)
		new->Port_3C4[0x06] |= 0x0C;
	else if (vgaBitsPerPixel == 16)
		new->Port_3C4[0x06] |= 0x08;

	new->Port_3C4[0x07] &= 0xFC;
	new->Port_3C4[0x07] |= 0x10;
	if (vga256InfoRec.clock[mode->Clock] < 100000)
		new->Port_3C4[0x07] |= 3;
	else if (vga256InfoRec.clock[mode->Clock] < 200000)
		new->Port_3C4[0x07] |= 2;
	else if (vga256InfoRec.clock[mode->Clock] < 250000)
		new->Port_3C4[0x07] |= 1;
	
	sr14 = new->Port_3C4[0x14];
	sr16 = orig_sr16;
	sr18 = new->Port_3C4[0x18];

	if (SISchipset == SIS300)
		Find300CRT_CPUthreshold(
			vga256InfoRec.clock[mode->Clock]/1000, 
			sisMClk()/1000,
			sr14, &sr16, sr18, &sr8, &sr9, &srF);
	else
		if (Find630CRT_CPUthreshold(
				vga256InfoRec.clock[mode->Clock]/1000,
				sisMClk()/1000,
				sr14, &sr16, &sr8, &sr9, &srF)==FALSE)
			return FALSE;

	new->Port_3C4[0x08] = sr8;
	new->Port_3C4[0x09] &= 0xF0;
	new->Port_3C4[0x09] |= (sr9 & 0xF);
	new->Port_3C4[0x0F] &= 0xDF;
	new->Port_3C4[0x0F] |= (srF & 0x20);
	new->Port_3C4[0x16] = sr16;

	new->Port_3C4[0x0A] = (
		(((mode->CrtcVSyncEnd  ) & 0x10 ) <<  1) | /* D5 */
		(((mode->CrtcVSyncEnd+1) & 0x100) >>  4) | /* D4 */
		(((mode->CrtcVSyncStart) & 0x400) >>  7) | /* D3 */
		(((mode->CrtcVSyncStart) & 0x400) >>  8) | /* D2 */
		(((mode->CrtcVDisplay-1) & 0x400) >>  9) | /* D1 */
		(((mode->CrtcVTotal-2)   & 0x400) >> 10)); /* D0 */

	new->Port_3C4[0x0B] = (
		(((mode->CrtcHSyncStart >> 3) & 0x300) >> 2)       | /* D76 */
		((((mode->CrtcHSyncStart >> 3) - 1) & 0x300) >> 4) | /* D54 */
		((((mode->CrtcHDisplay >> 3) - 1) & 0x300) >> 6)   | /* D32 */
		((((mode->CrtcHTotal >> 3) - 5) & 0x300) >> 8));     /* D10 */
			
	new->Port_3C4[0x0C] = (
		(((mode->CrtcHSyncEnd >> 3) & 0x20) >> 3) | /* D2 */
		(((mode->CrtcHSyncEnd >> 3) & 0xC0) >> 6)); /* D10 */

	if ((mode->Flags & V_INTERLACE) &&
	    (SISchipset==SIS630 || SISchipset==SIS540))  {
		temp = ((new->Port_3C4[0x0B] & 0xC0) << 2) | new->std.CRTC[4];
		temp -= (((((new->Port_3C4[0x0B] & 0x03) << 8) | new->std.CRTC[0]) + 5)/2);
		new->Port_3D4[0x1A] &= 0xFC;
		new->Port_3D4[0x1A] |= (temp & 0x300) >> 8;
		new->Port_3D4[0x19] = temp & 0xFF;
	}
	else  {
		new->Port_3D4[0x1A] &= 0xFC;
		new->Port_3D4[0x19] = 0;
	}

	new->std.CRTC[0x13] = offset & 0xFF;
	new->Port_3C4[0x0E] &= 0xF0;
	new->Port_3C4[0x0E] |= (offset >> 8) & 0x0F;

	new->Port_3C4[0x0F] |= 0x08; /* disable line compare */
	new->Port_3C4[0x10] =
		((mode->HDisplay * (vgaBitsPerPixel >> 3) + 63) >> 6) + 1;
	new->Port_3C4[0x06] |= 0x01;

	if ( ClockProgramable() ){
	    /* init clock */
	    if (!sisClockFind(mode->Clock, &new->sisClock)) {
		ErrorF("Can't find desired clock\n");
		return (FALSE);
	    }
	}

	if (new->std.NoClock >= 0) {
	    new->ClockReg2 = inb(0x3CC) | 0x0C; /* set internal/external clk */
	}

	new->Port_3C4[0x20] |= 0x81; /* linear address mode, MMIO */
	new->Port_3C4[0x21] &= 0xBF;
	new->Port_3C4[0x21] |= 0x80; /* PCI 32-bit access */
	/* Enable Turbo-queue */
        if (sisTurboQueue) {
		new->Port_3C4[0x26] = (turbo_queue_address >>  6) & 0xFF;
		new->Port_3C4[0x27] = (turbo_queue_address >> 14) & 0x03;
		new->Port_3C4[0x27] |= 0x80;
	} else {
            /* Disable turbo-queue */
		new->Port_3C4[0x27] &= 0x7F;
	}
	
	new->Port_3C4[0x1E] = 0x40;

	if (vga256InfoRec.clock[mode->Clock] > 150000) {
		new->Port_3C4[0x07] |= 0x80;
		new->Port_3C4[0x32] |= 0x08;
	} else {
		new->Port_3C4[0x07] &= 0x7F;
		new->Port_3C4[0x32] &= 0xF7;
	}

#endif
        return(TRUE);
}

/*
 * SISInit --
 *      Handle the initialization, etc. of a screen.
 */
static Bool
SISInit(mode)
    	DisplayModePtr mode;
{
	unsigned char temp;
	int offset;
	int i, last;
	unsigned int	CRT_CPUthresholdLow ;
	unsigned int	CRT_CPUthresholdHigh ;
	unsigned char	CRT_ENGthreshold ;
        unsigned int vclk[5];
#ifdef DEBUG
    ErrorF("SISInit\n");
#endif
	/*
	 * Initialize generic VGA registers.
	 */
	vgaHWInit(mode, sizeof(vgaSISRec));

	switch(SISchipset) {
	case SIS5597: last = SIS5597_3C5_LAST;
		break;
	case SIS6326: last = SIS6326_3C5_LAST;
		break;
	case SIS530: last = SIS530_3C5_LAST;
		break;
	case SIS630:
	case SIS540:
	case SIS300: last = SIS300_3C5_LAST;
		break;
	default: last = DEFAULT_3C5_LAST;
		break;
	}

	/* get SIS Specific Registers */
	for (i = 5 ; i <= last; i++) {
	    outb(0x3c4, i);
	    new->Port_3C4[i] = inb(0x3c5);
	}
	
	if (SISfamily == SIS300)
		return(SIS300Init(mode));

#if !defined(MONOVGA) && !defined(XF86VGA16)
	offset = vga256InfoRec.displayWidth >>
#ifdef MONOVGA
		(mode->Flags & V_INTERLACE ? 3 : 4);
#else
		(mode->Flags & V_INTERLACE ? 2 : 3);

	new->std.Attribute[16] = 0x01;
	new->std.CRTC[20] = 0x40;
	new->std.CRTC[23] = 0xA3;
#endif

	/* some generic settings */
	new->std.Attribute[0x10] = 0x01;   /* mode */
	new->std.Attribute[0x11] = 0x00;   /* overscan (border) color */
	new->std.Attribute[0x12] = 0x0F;   /* enable all color planes */
	new->std.Attribute[0x13] = 0x00;   /* horiz pixel panning 0 */

	if ( (vgaBitsPerPixel == 16) || (vgaBitsPerPixel == 24) ||
	     (vgaBitsPerPixel == 32) )
	    new->std.Graphics[0x05] = 0x00;    /* normal read/write mode */

	if (vgaBitsPerPixel == 16) {
	    offset <<= 1;	       /* double the width of the buffer */
	} else if (vgaBitsPerPixel == 24) {
	    offset += offset << 1;
	} else if (vgaBitsPerPixel == 32) {
	    offset <<= 2;
	} 

	new->BankReg = 0x02;
	if (SISchipset == SIS530)
		new->DualBanks = 0x01;
	else
		new->DualBanks = 0x00; 

	if ( sisUseLinear ) {
	    new->BankReg |= 0x80;  	/* enable linear mode addressing */
	    new->LinearAddr0 = (SIS.ChipLinearBase & 0x07f80000) >> 19 ; 
	    if ((SISchipset == SIS530) && (vga256InfoRec.videoRam == 8192))
		new->LinearAddr1 = ((SIS.ChipLinearBase & 0xf8000000) >> 27) |
				(0x80) ; /* Enable Linear with max 8 mb*/
	    else
		new->LinearAddr1 = ((SIS.ChipLinearBase & 0xf8000000) >> 27) |
		                (0x60) ; /* Enable Linear with max 4 mb*/
	}	
	else
	    new->DualBanks |= 0x08;

	if (SISchipset == SIS530) {
		if (mode->CrtcVDisplay > 1024)
			/* disable line compare */
			new->Port_3C4[0x38] |= 0x04;
		else
			new->Port_3C4[0x38] &= 0xFB;

		if ((vgaBitsPerPixel == 24) || (vgaBitsPerPixel == 32) ||
			(mode->CrtcHDisplay >= 1280))
			/* Enable high speed DCLK */
			 new->Port_3C4[0x3E] |= 1;
		else
			new->Port_3C4[0x3E] &= 0xFE;
	}

	if (vgaBitsPerPixel == 16) 
	    if (xf86weight.green == 5)
		new->BankReg |= 0x04;	/* 16bpp = 5-5-5             */
	    else
		new->BankReg |= 0x08;	/* 16bpp = 5-6-5             */

	if (vgaBitsPerPixel == 24) {
	    new->BankReg |= 0x10;
	    new->DualBanks |= 0x80;
	}

	if ((SISchipset == SIS530) && (vgaBitsPerPixel == 32)) {
	    new->Port_3C4[0x09] |= 0x80;
	    new->DualBanks |= 0x80;
	}

	new->std.CRTC[0x13] = offset & 0xFF;
	new->CRTCOff = ((offset & 0xF00) >> 4) | 
	    (((mode->CrtcVTotal-2) & 0x400) >> 10 ) |
		(((mode->CrtcVDisplay-1) & 0x400) >> 9 ) |
		    (((mode->CrtcVDisplay-1) & 0x400) >> 8 ) |
			(((mode->CrtcVSyncStart) & 0x400) >> 7 ) ;

	if (SISchipset == SIS530) {
		/* Extended Horizontal Overflow Register */
		new->Port_3C4[0x12] &= 0xE0;
		new->Port_3C4[0x12] |= (
			(((mode->CrtcHTotal >> 3) - 5) & 0x100) >> 8 |
			(((mode->CrtcHDisplay >> 3) - 1) & 0x100) >> 7 |
			(((mode->CrtcHSyncStart >> 3) - 1)& 0x100) >> 6 |
			((mode->CrtcHSyncStart >> 3) & 0x100) >> 5 |
			((mode->CrtcHSyncEnd >> 3) & 0x40)  >> 2);
	}
	
	if (mode->Flags & V_INTERLACE)
		new->BankReg |= 0x20;


	if ( ClockProgramable() ){
	    /* init clock */
	    if (!sisClockFind(new->std.NoClock, &new->sisClock)) {
		ErrorF("Can't find desired clock\n");
		return (FALSE);
	    }
	}

	if (new->std.NoClock >= 0) {
/* 3C4[7] isn't clock register in 5597, 205 or 6236.*/
	  if (SISchipset == SIS86C201 || SISchipset == SIS86C202)  
 	    new->ClockReg = new->std.NoClock;   /* not used in programmable  */
	    new->ClockReg2 = inb(0x3CC) | 0x0C; /* set internal/external clk */
	}

	/*  
	 *  this is function of the bandwidth
	 *  (pixelsize, displaysize, dotclock)
         *  worst case is not optimal
	 */
	CRT_ENGthreshold = 0x1F; /* ?????? old == 0x1F	*/
	FindCRT_CPUthreshold(vga256InfoRec.clock[new->std.NoClock], 
			     vgaBitsPerPixel,
			     &CRT_CPUthresholdLow, &CRT_CPUthresholdHigh);
	new->Port_3C4[0x08] = (CRT_ENGthreshold & 0x0F) | 
	    (CRT_CPUthresholdLow & 0x0F)<<4 ;
        new->Port_3C4[0x09] &= 0xF0;  /* Yeou */
        new->Port_3C4[0x09] |= (CRT_CPUthresholdHigh & 0x0F); /* Yeou */
	/* new->Port_3C4[0x09] = (CRT_CPUthresholdHigh & 0x0F) ; */

	if (SISchipset == SIS530) {
		/* CRT/CPU/Engine Threshold Bit[4] */
		new->Port_3C4[0x3F] &= 0xE3;
		new->Port_3C4[0x3F] |= ((CRT_CPUthresholdHigh & 0x10) |
					((CRT_ENGthreshold & 0x10) >> 1) |
					((CRT_CPUthresholdLow & 0x10) >> 2));
	}

	new->Port_3C4[0x27] |= 0x30 ; /* invalid logical screen width */

	if ( sisUseMMIO ) {
	    new->Port_3C4[0x27] |= 0x40 ; /* enable Graphic Engine Prog */
	    if ( !sisMMIOBase ) 
		sisMMIOBase = (unsigned char *)vgaBase ;	    
	    switch ( PCIMMIOBase ) {
	      case 0xA0000:
		new->Port_3C4[0x0B] |= 0x20 ; /* enable MMIO at 0xAxxxx */
		break;
	      case 0xB0000:
		new->Port_3C4[0x0B] |= 0x40 ; /* enable MMIO  at 0xBxxxx*/
		break;
	      default:
		new->Port_3C4[0x0B] |= 0x60 ; /* enable MMIO at PCI reg */
	    }
	    new->Port_3C4[0x0C] |= 0x80 ; /* 64-bit mode */ 
	    /*
	     * Setup the address to write monochrome source data to, for
	     * system to the screen colour expansion.
	     */
	    sisBltDataWindow = vgaLinearBase ;
	}
 
        if (SISchipset == SIS5597 || SISchipset == SIS6326 || SISchipset == SIS530) {
               new->Port_3C4[0x0C] |= 0x20; /* readahead cache */
               new->Port_3C4[0x07] |= 0x80; /* combine FIFOs */
               }

	 /* MClk Override                   */

 	if (SISchipset == SIS5597 || SISchipset == SIS6326) 
          if ( vga256InfoRec.MemClk ) {
            /* We can use SiSCalcClock for Mclck finding, but with max_VLD = 1 */
            sisCalcClock(vga256InfoRec.MemClk, 1, vclk);
  
            new->Port_3C4[0x28] = (vclk[Midx] - 1) & 0x7f ;
            new->Port_3C4[0x28] |= ((vclk[VLDidx] == 2 ) ? 1 : 0 ) << 7 ;
            new->Port_3C4[0x29]  = (vclk[Nidx] -1) & 0x1f ;	/* bits [4:0] contain denumerator -MC */
            if (vclk[Pidx] <= 4){
 	        new->Port_3C4[0x29] |= (vclk[Pidx] -1 ) << 5 ; /* postscale 1,2,3,4 */
 		new->Port_3C4[0x13] &= 0x7F;
    	    } else {
                new->Port_3C4[0x29] |= ((vclk[Pidx] / 2) -1 ) << 5 ;  /* postscale 6,8 */
 	        new->Port_3C4[0x13] |= 0x80;
	    }
	  }
	
	/* Enable Turbo-queue */
        if (sisTurboQueue) {
            new->Port_3C4[0x27] |= 0xC0; /* Enable engine programming and turbo-queue */
 	    if  (SISchipset == SIS6326)   {
#if 1
	        new->Port_3C4[0x3C] &= 0xFC ; /* All queue for 2D engine */
#else
	        new->Port_3C4[0x3C] &= 0xFD ; /* Equal queue for 2D / 3D */
#endif
	    }
	    temp = turbo_queue_address / 32;
	    if (SISchipset == SIS530)
		new->Port_3C4[0x2C] = temp & 0xFF;
	    else
		new->Port_3C4[0x2C] = temp & 0x7F;
#ifdef DEBUG
	    if (SISchipset == SIS530)
		ErrorF("Turbo-queue buffer at top - %d K; sr2C = 0x0%x \n", vga256InfoRec.videoRam - (temp*32), temp & 0xFF);
	    else
		ErrorF("Turbo-queue buffer at top - %d K; sr2C = 0x0%x \n", vga256InfoRec.videoRam - (temp*32), temp & 0x7F);
#endif
	} else {
            new->Port_3C4[0x27] &= ~0x80; /* Disable turbo-queue */
	}
	
#else /* VGA16 */
	if (new->std.NoClock >= 0) {
/* Juanjo Santamarta. 3C4[7] isn't clock register in 5597, 205 or 6236.  */
	  if (SISchipset == SIS86C201 || SISchipset == SIS86C202)  
 	    new->ClockReg = new->std.NoClock;   
	    temp = inb(0x3CC) & ~0x0C ; 
	    new->ClockReg2 = temp | ( (new->ClockReg<<2) & 0x0C) ;
	}


#endif
        return(TRUE);
}

/*
 * SISAdjust --
 *      adjust the current video frame to display the mousecursor
 */

static void 
SISAdjust(x, y)
	int x, y;
{
	unsigned char temp;
	int base;

#ifdef MONOVGA
	base = (y * vga256InfoRec.displayWidth + x + 3) >> 3;
#else
	base = y * vga256InfoRec.displayWidth + x ;
	/* calculate base bpp dep. */
	switch (vgaBitsPerPixel) {
	  case 16:
	    base >>= 1;
	    break;
	  case 24:
	    base = ((base * 3)) >> 2;
	    base -= base % 6;
	    break;
	  case 32:
	    break;
	  default:       /* 8bpp */
	    base >>= 2;
	    break;
    }
#endif

  	outw(vgaIOBase + 4, (base & 0x00FF00) | 0x0C);
	outw(vgaIOBase + 4, ((base & 0x00FF) << 8) | 0x0D);

	if (SISfamily == SIS300) {
		write_xr(0x0d, (base >> 16) & 0xFF);
	} else {
		read_xr(0x27,temp);
		temp &= 0xF0;
		temp |= (base & 0x0F0000) >> 16;
		write_xr(0x27,temp);
#ifdef DEBUG
	ErrorF("3C5/27h set to hex %2X, base %d\n",  temp, base);
#endif

		if (SISchipset == SIS530) {
		/* Extended screen starting address Bit[20] */
			read_xr(0x26,temp);
			temp &= 0xFE;
			temp |= (base >> 20) & 1;
			write_xr(0x26,temp);
		}

#ifdef XFreeXDGA
		if (vga256InfoRec.directMode & XF86DGADirectGraphics) {
		/* Wait until vertical retrace is in progress. */
			while (inb(vgaIOBase + 0xA) & 0x08);
			while (!(inb(vgaIOBase + 0xA) & 0x08));
		}
#endif
	}
}

/*
 * SISValidMode --
 *
 */
static int
SISValidMode(mode, verbose,flag)
DisplayModePtr mode;
Bool verbose;
int flag;
{
	return MODE_OK;
}

/*
 * MGADisplayPowerManagementSet --
 *
 * Sets VESA Display Power Management Signaling (DPMS) Mode.
 */
#ifdef DPMSExtension
static void SISDisplayPowerManagementSet(PowerManagementMode)
int PowerManagementMode;
{
	unsigned char extDDC_PCR;
	unsigned char crtc17;
	unsigned char seq1;

#ifdef DEBUG
	ErrorF("SISDisplayPowerManagementSet(%d)\n",PowerManagementMode);
#endif
	if (!xf86VTSema) return;
	outb(vgaIOBase + 4, 0x17);
	crtc17 = inb(vgaIOBase + 5);
	outb(0x3c4, 0x11);
	extDDC_PCR = inb(0x3c5) & ~0xC0;
	switch (PowerManagementMode)
	{
	case DPMSModeOn:
	    /* HSync: On, VSync: On */
	    seq1 = 0x00 ;
	    crtc17 |= 0x80;
	    break;
	case DPMSModeStandby:
	    /* HSync: Off, VSync: On */
	    seq1 = 0x20 ;
	    extDDC_PCR |= 0x40;
	    break;
	case DPMSModeSuspend:
	    /* HSync: On, VSync: Off */
	    seq1 = 0x20 ;
	    extDDC_PCR |= 0x80;
	    break;
	case DPMSModeOff:
	    /* HSync: Off, VSync: Off */
	    seq1 = 0x20 ;
	    extDDC_PCR |= 0xC0;
	    /* DPMSModeOff is not supported with ModeStandby | ModeSuspend  */
            /* need same as the generic VGA function */
	    crtc17 &= ~0x80;
	    break;
	}
	outw(0x3c4, 0x0100);	/* Synchronous Reset */
	outb(0x3c4, 0x01);	/* Select SEQ1 */
	seq1 |= inb(0x3c5) & ~0x20;
	outb(0x3c5, seq1);
	usleep(10000);
	outb(vgaIOBase + 4, 0x17);
	outb(vgaIOBase + 5, crtc17);
	write_xr(0x11,extDDC_PCR);
	outw(0x3c4, 0x0300);	/* End Reset */
}
#endif










