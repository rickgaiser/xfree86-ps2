/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86_Option.h,v 3.65.2.16 1998/09/27 12:58:54 hohndel Exp $ */
/*
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: xf86_Option.h /main/26 1996/10/27 18:07:36 kaleb $ */

#ifndef _XF86_OPTION_H
#define _XF86_OPTION_H

/*
 * Structures and macros for handling option flags.
 *
 * MAX_OFLAGS should be a multiple of FLAGBITS
 */
#define MAX_OFLAGS	256
#define FLAGBITS	(8 * sizeof(CARD32))
typedef struct {
	CARD32 flag_bits[MAX_OFLAGS/FLAGBITS];
} OFlagSet;

#define OFLG_SET(f,p)	((p)->flag_bits[(f)/FLAGBITS] |= (1 << ((f)%FLAGBITS)))
#define OFLG_CLR(f,p)	((p)->flag_bits[(f)/FLAGBITS] &= ~(1 << ((f)%FLAGBITS)))
#define OFLG_ISSET(f,p)	((p)->flag_bits[(f)/FLAGBITS] & (1 << ((f)%FLAGBITS)))
#define OFLG_ZERO(p)	memset((char *)(p), 0, sizeof(*(p)))

/*
 * Option flags.  Define these in numeric order.
 */
/* SVGA clock-related options */
#define OPTION_LEGEND		 0  /* Legend board with 32 clocks */
#define OPTION_SWAP_HIBIT	 1  /* WD90Cxx-swap high-order clock sel bit */
#define OPTION_8CLKS		 2  /* Probe 8 clocks instead of 4 (PVGA1) */
#define OPTION_16CLKS		 3  /* probe for 16 clocks instead of 8 */
#define OPTION_PROBE_CLKS	 4  /* Force clock probe for cards where a
				       set of preset clocks is used */
#define OPTION_HIBIT_HIGH	 5  /* Initial state of high order clock bit */
#define OPTION_HIBIT_LOW	 6
#define OPTION_CLKDIV2		 7  /* allow using clocks divided by 2 
				       in addition to bare clocks */
#define OPTION_HW_CLKS		 8  /* (ct) Hardware clocks */
#define OPTION_FORCE_VCLK1	 9  /* (ct) Use VClk1 as programmable clock */

/* Laptop display options */
#define OPTION_INTERN_DISP	15  /* Laptops - enable internal display (WD)*/
#define OPTION_EXTERN_DISP	16  /* Laptops - enable external display (WD)*/
#define OPTION_CLGD6225_LCD	17  /* Option to avoid setting the DAC to */
				   /* white on a clgd6225 with the LCD */
				   /* enabled */
#define OPTION_STN              18  /* DSTN option (CT)*/
#define OPTION_EXT_FRAM_BUF     19 /* external frame accelerator (CT) */
#define OPTION_LCD_STRETCH      20 /* disable LCD stretching */
#define OPTION_LCD_CENTER	21 /* enable LCD centering */
#define OPTION_PANEL_SIZE	22 /* (CT) Fix wrong panel size set in registers */

/* Memory options */
#define OPTION_FAST_DRAM	30 /* fast DRAM (for ET4000, S3, AGX) */
#define OPTION_MED_DRAM		31 /* medium speed DRAM (for S3, AGX) */
#define OPTION_SLOW_DRAM	32 /* slow DRAM (for Cirrus, S3, AGX) */
#define OPTION_NO_MEM_ACCESS	33 /* Unable to access video ram directly */
#define OPTION_NOLINEAR_MODE	34 /* chipset has broken linear access mode */
#define OPTION_INTEL_GX		35 /* Linear fb on an Intel GX/Pro (Mach32) */
#define OPTION_NO_2MB_BANKSEL	36 /* For cirrus cards with 512kx8 memory */
#define OPTION_FIFO_CONSERV	37 /* (cirrus) (agx) */
#define OPTION_FIFO_AGGRESSIVE	38 /* (cirrus) (agx) */
#define OPTION_MMIO		39 /* Use MMIO for Cirrus 543x */
#define OPTION_LINEAR		40 /* Use linear fb for Cirrus */
#define OPTION_FIFO_MODERATE  	41 /* (agx) */
#define OPTION_SLOW_VRAM	42 /* (s3) */
#define OPTION_SLOW_DRAM_REFRESH 43 /* (s3) */
#define OPTION_FAST_VRAM	44 /* (s3) */
#define OPTION_PCI_BURST_ON	45 /* W32/SVGA */
#define OPTION_PCI_BURST_OFF	46 /* W32/SVGA */
#define OPTION_W32_INTERLEAVE_ON  47 /* W32/SVGA */
#define OPTION_W32_INTERLEAVE_OFF 48 /* W32/SVGA */
#define OPTION_SLOW_EDODRAM	49 /* slow EDO-DRAM (for S3) */
#define OPTION_EARLY_RAS_PRECHARGE	50 /* shift RAS prechange signal (for S3) */
#define OPTION_LATE_RAS_PRECHARGE	51 /* shift RAS prechange signal (for S3) */

/* Accel/cursor features */
#define OPTION_NOACCEL		60 /* Disable accel support in SVGA server */
#define OPTION_HW_CURSOR	61 /* Turn on HW cursor */
#define OPTION_SW_CURSOR	62 /* Turn off HW cursor (Mach32) */
#define OPTION_NO_BITBLT	63 /* Disable hardware bitblt (cirrus) */
#define OPTION_FAVOUR_BITBLT	64 /* Favour use of BitBLT (cirrus) */
#define OPTION_NO_IMAGEBLT	65 /* Avoid system-to-video BitBLT (cirrus) */
#define OPTION_NO_FONT_CACHE	66 /* Don't enable the font cache */
#define OPTION_NO_PIXMAP_CACHE	67 /* Don't enable the pixmap cache */
#define OPTION_TRIO32_FC_BUG	68 /* Workaround Trio32 font cache bug */
#define OPTION_S3_968_DASH_BUG	69 /* Workaround S3 968 dashed line bug */

/* RAMDAC options */
#define OPTION_BT485_CURS	80 /* Override Bt485 RAMDAC probe */
#define OPTION_TI3020_CURS	81 /* Use 3020 RAMDAC cursor (default) */
#define OPTION_NO_TI3020_CURS	82 /* Override 3020 RAMDAC cursor use */
#define OPTION_DAC_8_BIT	83 /* 8-bit DAC operation */
#define OPTION_SYNC_ON_GREEN	84 /* Set Sync-On-Green in RAMDAC */
#define OPTION_BT482_CURS       85 /* Use Bt482 RAMDAC cursor */
#define OPTION_S3_964_BT485_VCLK	86 /* probe/invert VCLK for 964 + Bt485 */
#define OPTION_TI3026_CURS	87 /* Use 3026 RAMDAC cursor (default) */
#define OPTION_IBMRGB_CURS	88 /* Use IBM RGB52x RAMDAC cursor (default) */
#define OPTION_DAC_6_BIT	89 /* 6-bit DAC operation */

/* Vendor specific options */
#define OPTION_SPEA_MERCURY	100 /* pixmux for SPEA Mercury (S3) */
#define OPTION_NUMBER_NINE	101 /* pixmux for #9 with Bt485 (S3) */
#define OPTION_STB_PEGASUS	102 /* pixmux for STB Pegasus (S3) */
#define OPTION_ELSA_W1000PRO	103 /* pixmux for ELSA Winner 1000PRO (S3) */
#define OPTION_ELSA_W2000PRO	104 /* pixmux for ELSA Winner 2000PRO (S3) */
#define OPTION_DIAMOND		105 /* Diamond boards (S3) */
#define OPTION_GENOA		106 /* Genoa boards (S3) */
#define OPTION_STB		107 /* STB boards (S3) */
#define OPTION_HERCULES		108 /* Hercules boards (S3) */
#define OPTION_MIRO_MAGIC_S4	109 /* miroMagic S4 with (S3) 928 and BT485 */
#define OPTION_ELSA_W2000PRO_X8	110 /* clock/phase_detect for ELSA Winner 2000PRO/X-8 (S3) */
#define OPTION_MIRO_80SV	111 /* clock/phase_detect for MIRO 80SV (S3) */
#define OPTION_AST_MACH32       112 /* AST's soldered-in SVGA motherboard (Mach32) */

/* Misc options */
#define OPTION_CSYNC		120 /* Composite sync */
#define OPTION_SECONDARY	121 /* Use secondary address (HGC1280) */
#define OPTION_PCI_HACK		122 /* (S3) */
#define OPTION_POWER_SAVER	123 /* Enable DPMS Power Saving */
#define OPTION_OVERRIDE_BIOS	124 /* Override BIOS for Mach64 */
#define OPTION_NO_BLOCK_WRITE	125 /* No block write mode for Mach64 */
#define OPTION_BLOCK_WRITE	126 /* Block write mode for Mach64 */
#define OPTION_NO_BIOS_CLOCKS	127 /* Override BIOS clocks for Mach64 */
#define OPTION_S3_INVERT_VCLK	128 /* invert VLCK (CR67:0) (S3) */
#define OPTION_NO_PROGRAM_CLOCKS 129 /* Turn off clock programming */
#define OPTION_NO_PCI_PROBE	130 /* Disable PCI probe (VGA) */
#define OPTION_TRIO64VP_BUG1	131 /* Trio64V+ bug hack #1 */
#define OPTION_TRIO64VP_BUG2	132 /* Trio64V+ bug hack #2 */
#define OPTION_TRIO64VP_BUG3	133 /* Trio64V+ bug hack #3 */
#define OPTION_USE_MODELINE	134 /* use modeline for LCD instead of preset (ct)*/
#define OPTION_SUSPEND_HACK	135 /* (CT) Use different suspend/resume scheme */
#define OPTION_18_BIT_BUS	136 /* (CT) Use 18bit TFT bus for 24bpp mode */
#define OPTION_PCI_RETRY	137 /* Use PCI-retry instead of busy-waiting */
#define OPTION_NO_PCI_DISC	138 /* Disable PCI disconnect (S3) */
#define OPTION_NO_SPLIT_XFER	139 /* Disable split VRAM transfers to avoid pixel wrapping (S3) */
#define OPTION_MGA_24BPP_FIX	140 /* change PLL for higher clocks at 24bpp */
#define OPTION_MGA_SDRAM	141 /* MGA equipped with SDRAM */

/* Debugging options */
#define OPTION_SHOWCACHE	150 /* Allow cache to be seen (S3) */
#define OPTION_FB_DEBUG		151 /* Linear fb debug for S3 */

/* Some AGX Tuning/Debugging options -- several are for development testing */
#define OPTION_8_BIT_BUS        160 /* Force 8-bit CPU interface - MR1:0 */
#define OPTION_WAIT_STATE       161 /* Force 1 bus wait state - MR1:1<-1 */
#define OPTION_NO_WAIT_STATE    162 /* Force no bus wait state - MR:1<-0 */
#define OPTION_CRTC_DELAY       163 /* Select XGA Mode Delay - MR1:3 */
#define OPTION_VRAM_128         164 /* VRAM shift every 128 cycles - MR2:0 */
#define OPTION_VRAM_256         165 /* VRAM shift every 256 cycles - MR2:0 */
#define OPTION_REFRESH_20       166 /* # clocks between scr refreshs - MR3:5 */
#define OPTION_REFRESH_25       167 /* # clocks between scr refreshs - MR3:5 */
#define OPTION_VLB_A            168 /* VESA VLB transaction type A   - MR7:2 */
#define OPTION_VLB_B            169 /* VESA VLB transaction type B   - MR7:2 */
#define OPTION_SPRITE_REFRESH   170 /* Sprite refresh every hsync    - MR8:4 */
#define OPTION_SCREEN_REFRESH   171 /* Screen refresh during blank   - MR8:5 */
#define OPTION_VRAM_DELAY_LATCH	172 /* Delay Latch                   - MR7:3 */
#define OPTION_VRAM_DELAY_RAS	173 /* Delay RAS signal              - MR7:4 */
#define OPTION_VRAM_EXTEND_RAS  174 /* Extend the RAS signal         - MR8:6 */
#define OPTION_ENGINE_DELAY     175 /* Wait state for some VLB's     - MR5:3 */

/* Some options for oti087, debugging and fine tunning */
#define OPTION_CLOCK_50         180
#define OPTION_CLOCK_66         181
#define OPTION_NO_WAIT          182
#define OPTION_FIRST_WWAIT      183
#define OPTION_WRITE_WAIT       184
#define OPTION_ONE_WAIT         185
#define OPTION_READ_WAIT        186
#define OPTION_ALL_WAIT         187
#define OPTION_ENABLE_BITBLT    188


/* #ifdef PC98 */
#define OPTION_PCSKB		 190 /* SELECT EPSON PCSKB for S3 Server */
#define OPTION_PCSKB4		 191 /* SELECT EPSON PCSKB for S3 Server */
#define OPTION_PCHKB		 192 /* SELECT EPSON PCHKB for S3 Server */
#define OPTION_NECWAB		 193 /* SELECT NEC WAB-A/B for S3 Server */
#define OPTION_PW805I		 194 /* SELECT Canopus PW805i for S3 Server */
#define OPTION_PWLB		 195 /* SELECT Canopus PW_LB for S3 Server */
#define OPTION_PW968		 196 /* SELECT Canopus PW968 for S3 Server */
#define OPTION_GA98NB1           190 /* SELECT IO DATA GA-98NB1 for SVGA */
#define OPTION_GA98NB2           201 /* SELECT IO DATA GA-98NB2 for SVGA */
#define OPTION_GA98NB4           202 /* SELECT IO DATA GA-98NB4 for SVGA */
#define OPTION_WAP               203 /* SELECT MELCO WAP-2000/4000 for SVGA */
#define OPTION_NEC_CIRRUS        204 /* SELECT NEC Internal Server for SVGA */
#define OPTION_EPSON_MEM_WIN	 205 /* ENABLE mem-window 0xF00000 for EPSON */
#define OPTION_PW_MUX            206 /* ENABLE MUX on PW928II */
#define OPTION_NOINIT		 207 /* Not Initialize SDAC & VGA Registers */
#define OPTION_PC98TGUI		 208 /* SELECT NEC TGUI9660 */
/* #endif */

#define OPTION_TGUI_PCI_READ_ON  211 /* Trident TGUI PCI burst read */
#define OPTION_TGUI_PCI_WRITE_ON 212 /* Trident TGUI PCI burst write */
#define OPTION_TGUI_TVOUT	 213 /* Trident TV output force */
#define OPTION_CYBER_SHADOW	 214 /* Trident Cyber Shadow registers */
#define OPTION_TGUI_MCLK_66	 215 /* Trident MCLK at 66MHz */

/* more Memory options */
#define OPTION_FPM_VRAM		220 /* (s3v) */
#define OPTION_EDO_VRAM		221 /* (s3v) */
#define OPTION_NO_MMIO		222 /* Disable MMIO (Cirrus 543x/4x) */

/* XAA options */
#define OPTION_XAA_BENCHMARK	230 /* Perform start-up benchmarks */
#define OPTION_XAA_NO_COL_EXP	231 /* Disable color expansion. */
  
/* options for Glint server */
#define OPTION_FIREGL3000       232 /* Assume a Fire GL 3000 card */
#define OPTION_OVERCLOCK_MEM	233 /* run the memory faster than it should */

/* options for sis server */
#define OPTION_HOST_BUS		240
#define OPTION_EXT_ENG_QUEUE	241

/* NeoMagic options */
#define OPTION_PROG_LCD_MODE_REGS       245 /* Enable lcd mode programming */
#define OPTION_NO_PROG_LCD_MODE_REGS    246 /* Disable lcd mode programming */
#define OPTION_PROG_LCD_MODE_STRETCH    247 /* Enable lcd mode programming when */
					    /* stretch mode is enabled */
#define OPTION_NO_PROG_LCD_MODE_STRETCH 248 /* Disable lcd mode programming when */
					    /* stretch mode is enabled */
#define OPTION_OVERRIDE_VALIDATE_MODE   249 /* Skip mode validation for LCD panels */

/*
 *  MAX flag value is 256.  If larger is needed, remember to update
 *  MAX_OFLAGS at the top of this file.
 */


#define CLOCK_OPTION_PROGRAMABLE 0 /* has a programable clock */
#define CLOCK_OPTION_ICD2061A	 1 /* use ICD 2061A programable clocks      */
#define CLOCK_OPTION_SC11412     3 /* use SC11412 programmable clocks */
#define CLOCK_OPTION_S3GENDAC    4 /* use S3 Gendac programmable clocks */
#define CLOCK_OPTION_TI3025      5 /* use TI3025 programmable clocks */
#define CLOCK_OPTION_ICS2595     6 /* use ICS2595 programmable clocks */
#define CLOCK_OPTION_CIRRUS      7 /* use Cirrus programmable clocks */
#define CLOCK_OPTION_CH8391      8 /* use Chrontel 8391 programmable clocks */
#define CLOCK_OPTION_ICS5342     9 /* use ICS 5342 programmable clocks */
#define CLOCK_OPTION_S3TRIO     10 /* use S3 Trio32/64 programmable clocks */
#define CLOCK_OPTION_TI3026     11 /* use TI3026 programmable clocks */
#define CLOCK_OPTION_IBMRGB     12 /* use IBM RGB52x programmable clocks */
#define CLOCK_OPTION_STG1703    13 /* use STG1703 programmable clocks */
#define CLOCK_OPTION_ICS5341    14 /* use ICS 5341 (ET4000W32p) */
#define CLOCK_OPTION_TRIDENT    15 /* use programmable clock on TGUI */
#define CLOCK_OPTION_ATT409     16 /* use ATT20C409 programmable clock */
#define CLOCK_OPTION_CH8398     17 /* use Chrontel 8398 programmable clock */
#define CLOCK_OPTION_GLORIA8    18 /* use ELSA Gloria-8 TVP3030/ICS9161 clock */
#define CLOCK_OPTION_ET6000     19 /* use ET6000 built-in programmable clock */
#define CLOCK_OPTION_ICS1562    20 /* used for TGA server */
#define CLOCK_OPTION_S3AURORA   21 /* use S3 Aurora64V+ programmable clocks */
#define CLOCK_OPTION_S3TRIO64V2 22 /* use S3 Trio64V2 or ViRGE/DX/GX 170MHz clocks */
#define CLOCK_OPTION_ICS5301    23 /* use ICS 5301 (ET4000W32i) */

/*
 * Table to map option strings to tokens.
 */
typedef struct {
  char *name;
  int  token;
} OptFlagRec, *OptFlagPtr;

#ifdef INIT_OPTIONS
OptFlagRec xf86_OptionTab[] = {
  { "",			-1 },
};

OptFlagRec xf86_ClockOptionTab [] = {
  { "",			-1 },
};

#else
extern OptFlagRec xf86_OptionTab[];
extern OptFlagRec xf86_ClockOptionTab[];
#endif

#endif /* _XF86_OPTION_H */

