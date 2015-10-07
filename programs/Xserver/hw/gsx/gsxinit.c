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
#include "common/xf86.h"
#include "common/xf86Procs.h"
#include "common/xf86_OSproc.h"
#include "common/xf86_OSlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <linux/ps2/gs.h>

int gsxScreenIndex; // reference from gsxGetScreenPtr(s) in gsx.h
unsigned long gsxGeneration = 0;

extern int monitorResolution;


#include <X11/XWDFile.h>

#define VFB_DEFAULT_INDEX 2
#define VFB_DEFAULT_DEPTH  24

#define VFB_DEFAULT_WHITEPIXEL 0
#define VFB_DEFAULT_BLACKPIXEL 1
#define VFB_DEFAULT_LINEBIAS 0

#define DEFAULT_MIX ((int)(0.35f * 255.0f))

#define TV_MARGIN_X(x) ((x/20) > 31 ? 31 : (x/20))
#define TV_MARGIN_Y(y) (y/20)

#define DTV_MARGIN_X(x) ((x/20) > 31 ? 31 : (x/20))
#define DTV_MARGIN_Y(y) (y/20)


typedef struct
{
    int width;
    int height;
    char depth;
    Pixel blackPixel;
    Pixel whitePixel;
    short framerate;
    int res;
    int mode;
    short interlace;
    short odd_even_mix;
    short margin_x,margin_y;

} gsxScreenInfo, *gsxScreenInfoPtr;

static short gsxNumScreens;
static gsxScreenInfo gsxScreens[MAXSCREENS];
static Bool gsxPixmapDepths[33];

struct vesa_restab {
    short w;
    short h;
    short res;
};

static struct vesa_restab restab[]={
    { 640, 480, PS2_GS_640x480},
    { 800, 600, PS2_GS_800x600},
    {1024, 768, PS2_GS_1024x768},
    {1280, 1024,PS2_GS_1280x1024}
};

void SetGSXFrameRate(int scrn, int framerate)
{
  if(scrn < 0 || scrn >= gsxNumScreens) {
    return;
  }
  if(framerate != 60) {
    framerate = 75;
  }
  gsxScreens[scrn].framerate = 
    	(framerate == 75) ? PS2_GS_75Hz : PS2_GS_60Hz;
}

void SetGSXInterlaceMix(int scrn, int odd_even_mix)
{
  if(scrn < 0 || scrn >= gsxNumScreens) {
    return;
  }
  odd_even_mix  = (int) (odd_even_mix * 1.0f /100.0f * 255.0f);
}

void SetGSXVesaScreen(int scrn,
		      int width, int height, int depth, int res)
{
  if(scrn < 0 || scrn >= gsxNumScreens) {
    return;
  }
  gsxScreens[scrn].mode   = PS2_GS_VESA;
  gsxScreens[scrn].width  = width;
  gsxScreens[scrn].height = height;
  gsxScreens[scrn].depth  = depth;
  gsxScreens[scrn].res = res;

}

void SetGSXTVScreen(int scrn, int mode, int width, int height, int depth, int interlace)
{
  int mx, my;
  int res;

  if(scrn < 0 || scrn >= gsxNumScreens) {
    return;
  }
  if(mode == PS2_GS_DTV) {
    mx = DTV_MARGIN_X(width);
    my = DTV_MARGIN_Y(height);
    switch(height) {
    case 720:
      res = PS2_GS_720P;
      break;
    case 1080:
      res = PS2_GS_1080I;
      break;
    case 480:
    default:
      res = PS2_GS_480P;
      break;
    }
    gsxScreens[scrn].res = res;
  } else {
    mx = TV_MARGIN_X(width);
    my = TV_MARGIN_Y(height);
  }
  width  -= 2*mx;
  height -= 2*my;
  gsxScreens[scrn].mode   = mode;
  gsxScreens[scrn].width  = width;
  gsxScreens[scrn].height = height;
  gsxScreens[scrn].depth  = depth;
  gsxScreens[scrn].margin_x     = mx;
  gsxScreens[scrn].margin_y     = my;
  if(interlace >= 0) {
    gsxScreens[scrn].interlace = interlace;
  }
}

void SetGSXInterlaceMode(int scrn, int interlace)
{
  if(scrn < 0 || scrn >= gsxNumScreens) {
    return;
  }
  gsxScreens[scrn].interlace = interlace;
}

static void
gsxInitializePixmapDepths()
{
    int i;
    gsxPixmapDepths[1] = TRUE; /* always need bitmaps */
    for (i = 2; i <= 32; i++)
	gsxPixmapDepths[i] = FALSE;
}

static void
gsxInitializeDefaultScreens()
{
    int i;

    for (i = 0; i < MAXSCREENS; i++)
    {
	gsxScreens[i].mode = PS2_GS_VESA;
	gsxScreens[i].width  = restab[VFB_DEFAULT_INDEX].w;
	gsxScreens[i].height = restab[VFB_DEFAULT_INDEX].h;
	gsxScreens[i].res =  restab[VFB_DEFAULT_INDEX].res;

	gsxScreens[i].depth  = VFB_DEFAULT_DEPTH;
	gsxScreens[i].blackPixel = VFB_DEFAULT_BLACKPIXEL;
	gsxScreens[i].whitePixel = VFB_DEFAULT_WHITEPIXEL;
	gsxScreens[i].framerate =  PS2_GS_60Hz;
	gsxScreens[i].odd_even_mix = DEFAULT_MIX;
	gsxScreens[i].margin_x =0;
	gsxScreens[i].margin_y =0;
    }
    gsxNumScreens = 1;
}

static inline int 
formatBitsPerPixel(depth)
    int depth;
{
    if (depth == 1) return 1;
    else if (depth <= 8) return 8;
    else if (depth <= 16) return 16;
    else return 32;
}

void
ddxGiveUp()
{
    xf86CloseConsole() ;
    gsosClose() ;
}

void
AbortDDX()
{
    ddxGiveUp();
}

void
OsVendorInit()
{
}

void
OsVendorFatalError()
{
}

void
ddxUseMsg()
{
  ErrorF("\n");
  ErrorF("\n");
  ErrorF("Device Dependent Usage\n");
  ErrorF("-screen n [VESA,]WxHxD[,fr]  set VESA screen's config\n");
  ErrorF("    WxHxD (width, height and depth):\n");
  ErrorF("        640x480x16   640x480x24     800x600x16   800x600x24\n");
  ErrorF("       1024x768x16  1024x768x24   1280x1024x16\n");
  ErrorF("    fr (frame rate in Hz): 60 or 75\n");
  ErrorF("-screen n NTSC[,xD][,inter[,M]|,nointer]\n");
  ErrorF("    set NTSC screen's config\n");
  ErrorF("        D (depth): 16 or 24 (default 24)\n");
  ErrorF("        M (odd-even line mixing raito): 0..100 (default 35)n");
  ErrorF("-screen n PAL[,xD][,inter[,M]|,nointer]\n");
  ErrorF("    set PAL screen's config\n");
  ErrorF("        D (depth): 16 or 24 (default 24)\n");
  ErrorF("        M (odd-even line mixing raito): 0..100 (default 35)\n");
  ErrorF("-screen n DTV[,xD][,480p|,720p|1080i[,M]]\n\
	set DTV screen's config.\n");
  ErrorF("-interlace-mix [0-100]   set odd-even line mixing raito (default:35).\n");
  if (getuid() == 0) {
    ErrorF("-xgsconfig file        specify a configuration file\n");
  }
  ErrorF("-blackpixel n          pixel value for black\n");
  ErrorF("-whitepixel n          pixel value for white\n");
  ErrorF("-framerate n           set frame-rate in Hz (n: 60, 75)\n");
}


int checkResolution(int width, int height)
{

    int i;
    for(i = 0; i < sizeof(restab) / sizeof(struct vesa_restab); i++) {
        if(restab[i].w == width && restab[i].h == height) {
          return restab[i].res;
        }
    }
    return -1;
}

static int 
get_dtv_params (char *p, int *width, int *height, 
		int *depth, int *res, int *odd_even_mix)
{
	/* [xD][,480p|,720p|,1080i[,M]] */
	int cnt;
	int r;
	char *q, c;
	int m;

	cnt = 0;
	r = 480;
	c = 'p';
	*depth = 24;
	if (!p) goto out;

	*depth = -1;

	p ++;
	if (*p=='x') {
	    p++;
	    *depth = strtol(p, &q, 10);
	    if (p != q ) {
	      if (*depth != 24 && *depth != 16)
	        return -1;
	      p = q;
	      if (!*p) goto out;
	      if (*p != ',') return -1;
	    }
	    p++;
	}
        cnt = sscanf(p, "%d%c,%d", &r, &c, &m);
	if (cnt < 2) {
	      return -1;
	}  
    out:
	c = tolower(c);
	if (c!='p' && c!='i') return -1;
	switch (r) {
	  case 480:
	    if (c!='p') return -1;
	    if (cnt != 2 ) return -1;
	    *odd_even_mix = 0;
	    *res = PS2_GS_480P;
	    *width = 720; *height = 480; 
	    if (*depth == -1) *depth=24;
	    break;
	  case 720:
	    if (c!='p') return -1;
	    if (cnt != 2 ) return -1;
	    *odd_even_mix = 0;
	    *res = PS2_GS_720P;
	    *width = 1024; *height = 720; 
	    if (*depth == -1) *depth=24;
	    break;
	  case 1080:
	    if (c!='i') return -1;
	    if (cnt == 3 ){
	        if (m <0 || m>100) return -1;
	        *odd_even_mix = m;
	    }
	    *res = PS2_GS_1080I;
 	    *width = 1920; *height = 1080; 
	    if (*depth == -1) *depth=16;
	    if (*depth != 16) return -1;
	    break;
	}
	return 0;
}

static int 
get_tv_params (int mode, char *p, int *width, int *height, 
		int *depth, int *interlace, int *odd_even_mix)
{
	/* [xD,][nointer|inter[,M]] */
	char *q;
	int m;
	int cnt;

	*interlace = PS2_GS_INTERLACE;
	*depth = 24;
	if (!p) goto out;

	p ++;
	if (*p=='x') {
	    p++;
	    *depth = strtol(p, &q, 10);
	    if (p != q) {
	      if (*depth != 24 && *depth != 16)
	        return -1;
	      p = q;
	      if (!*p) goto out;
	      if (*p != ',') return -1;
	    }
	    p++;
	}

	switch (tolower(*p)) {
	  case 'n':
	    *interlace = PS2_GS_NOINTERLACE;
            *odd_even_mix = 0;
	    break;
	  case 'i':
            cnt = sscanf(p, "%*[A-z],%d", &m);
	    if (cnt == 1) {
		if (m<0 || m>100) return -1;
	        *odd_even_mix = m;
	    }
	    break;
	  default:  
	    return -1;
	    break;
	}
out:
	*width = 640;
	*height = (mode == PS2_GS_NTSC) ? 448 : 512;

	if (*interlace == PS2_GS_NOINTERLACE) {
	  *height /= 2;
	}
	return 0;
}

static 
int get_vesa_params( char *p, 
	int *width, int *height, int *depth, int *framerate, int *res)
{
	int cnt;
	int fr;
	/* WxHxD[,fr] */

	fr = 60;
	cnt = sscanf(p, "%dx%dx%d,%d", 
		width, height, depth, &fr);
	if (cnt==4) {
	  if (fr!=60 && fr!=75) return -1;
	  *framerate = fr==75 ? PS2_GS_75Hz : PS2_GS_60Hz;
	}  else if (cnt!=3 ) {
	  return -1;
	}

	*res = checkResolution(*width, *height);
	if(*res<0) {
	  return -1;
	}
	switch(*depth) {
	case 16:
	  break;
	case 24:
	  if(*width != 1280)
	    break;
	  /* fall throught */
	default:
	  return -1;
	}
	return 0;
}

static short lastScreen = -1;

int
ddxProcessArgument (argc, argv, i)
    int argc;
    char *argv[];
    int i;
{
    static Bool firstTime = TRUE;

    if (firstTime) {
	gsxInitializeDefaultScreens();
	gsxInitializePixmapDepths();
        firstTime = FALSE;
    }

    if(getuid() == 0 && strcmp(argv[i], "-xgsconfig") == 0) {
      if(!argv[i+1]) {
	return 0;
      }
      if(strlen(argv[i+1]) >= PATH_MAX) {
	FatalError("XGSConfig path name too long\n");
      }
      strcpy(xf86ConfigFile, argv[i+1]);
      ErrorF("xf86ConfigFile=%s\n", xf86ConfigFile);
      return 2;
    }


    /* -screen n */
    if (strcmp (argv[i], "-screen") == 0) {
	int screenNum;
	int width, height, depth;
	int interlace=0;
	int framerate = -1;
	int odd_even_mix=-1;
	int res=0;
	int mode;
	char *p;
	int margin_x,margin_y;

	margin_x=margin_y=0;

	if (i + 2 >= argc) {
	  UseMsg();
	  exit(1);
	}

	/* screen number */
	if(!isdigit(*argv[i+1])) {
	    ErrorF("Invalid screen number %s\n", argv[i+1]);
	    UseMsg();
	    exit(1);
	}
	screenNum = atoi(argv[i+1]);
	if (screenNum < 0 || screenNum >= MAXSCREENS) {
	    ErrorF("Invalid screen number %d\n", screenNum);
	    UseMsg();
	    exit(1);
	}

	p = argv[i+2];

	mode = ~PS2_GS_NTSC;
	switch (tolower(*p)) {
	  
	  case 'n':
	    /* NTSC */
	    mode = PS2_GS_NTSC;
	    /* fallthrough */
	  case 'p':
	    /* PAL */
	    if (mode != PS2_GS_NTSC)
	      mode = PS2_GS_PAL;
	    p = strchr(p,',');
	    if (get_tv_params (mode, p, &width, &height, 
				&depth, &interlace, &odd_even_mix)<0)
	      goto errout;

	    margin_x = TV_MARGIN_X(width);
	    margin_y = TV_MARGIN_Y(height);
	    width -= 2*margin_x;
	    height -= 2*margin_y;
	    break;

	  case 'd':
	    /* DTV */
	    mode = PS2_GS_DTV;
	    p = strchr(p,',');
	    if (get_dtv_params (p, &width, &height, 
				&depth, &res, &odd_even_mix)<0)
	      goto errout;

	    margin_x = DTV_MARGIN_X(width);
	    margin_y = DTV_MARGIN_Y(height);
	    width -= 2*margin_x;
	    height -= 2*margin_y;
	    break;

	  default:
	    if ( !isdigit(*p) && (tolower(*p) != 'v') ) {
	errout:
	      ErrorF("Invalid screen configuration %s\n", argv[i+2]);
	      UseMsg();
	      exit(1);
	    }
	    /* VESA */
	    if (tolower(*p) == 'v') {
	      p = strchr(p,',');
	      if (!p) goto errout;
	      p++;
	    }
	    mode = PS2_GS_VESA;
	    if (get_vesa_params (p, &width, &height, 
					&depth, &framerate, &res)<0)
	      goto errout;
	}

	if (odd_even_mix != -1) {
	    odd_even_mix  = (int) (odd_even_mix * 1.0f /100.0f * 255.0f);
	    gsxScreens[screenNum].odd_even_mix  = odd_even_mix;
	}
        if (framerate != -1 ) {
	    gsxScreens[screenNum].framerate = framerate;
        }

	gsxScreens[screenNum].width  = width;
	gsxScreens[screenNum].height = height;
	gsxScreens[screenNum].depth  = depth;
	gsxScreens[screenNum].res  = res;
	gsxScreens[screenNum].mode  = mode;
	gsxScreens[screenNum].interlace  = interlace;
	gsxScreens[screenNum].margin_x  = margin_x;
	gsxScreens[screenNum].margin_y  = margin_y;
	xf86bpp = depth;

	if (screenNum >= gsxNumScreens)
	    gsxNumScreens = screenNum + 1;
	lastScreen = screenNum;

	return 3;
    }

    /* -interlace-mix n [0-100] */
    if (strcmp (argv[i], "-interlace-mix") == 0) {
      unsigned int mix;
      if (++i >= argc) UseMsg();
      mix = atoi(argv[i]);
      if (mix<0 || mix>100) {
	    UseMsg();
      }
      mix  = (int) (mix * 1.0f /100.0f * 255.0f);
      if (-1 == lastScreen) {
	    int i;
	    for (i = 0; i < MAXSCREENS; i++) {
		gsxScreens[i].odd_even_mix = mix;
	    }
      } else {
	    gsxScreens[lastScreen].odd_even_mix = mix;
      }
      return 2;
    }

    /* -blackpixel n */
    if (strcmp (argv[i], "-blackpixel") == 0) {
	Pixel pix;
	if (++i >= argc) UseMsg();
	pix = atoi(argv[i]);
	if (-1 == lastScreen) {
	    int i;
	    for (i = 0; i < MAXSCREENS; i++) {
		gsxScreens[i].blackPixel = pix;
	    }
	} else {
	    gsxScreens[lastScreen].blackPixel = pix;
	}
	return 2;
    }

    /* -whitepixel n */
    if (strcmp (argv[i], "-whitepixel") == 0) {
	Pixel pix;
	if (++i >= argc) UseMsg();
	pix = atoi(argv[i]);
	if (-1 == lastScreen) {
	    int i;
	    for (i = 0; i < MAXSCREENS; i++) {
		gsxScreens[i].whitePixel = pix;
	    }
	} else {
	    gsxScreens[lastScreen].whitePixel = pix;
	}
	return 2;
    }
    /* -framerate n */
    if (strcmp (argv[i], "-framerate") == 0) {
	unsigned int fr;
	if (++i >= argc) UseMsg();
	fr = atoi(argv[i]);
	if ( fr != 60 && fr != 75 ) {
	    UseMsg();
	} else if (-1 == lastScreen) {
	    int i;
	    for (i = 0; i < MAXSCREENS; i++) {
		gsxScreens[i].framerate = (fr == 75) ?
						PS2_GS_75Hz: PS2_GS_60Hz;
	    }
	} else {
	    gsxScreens[lastScreen].framerate = (fr == 75) ?
	    					PS2_GS_75Hz : PS2_GS_60Hz;
	}
	return 2;
    }

    return 0;
}


static Bool gsxProbe()
{
    return TRUE ;
}

static Bool gsxServerReset()
{
    gsxScreenIndex= AllocateScreenPrivateIndex();
    if (gsxScreenIndex < 0)
        return FALSE;
    return TRUE;
}


static Bool gsxScreenPrivateInit(int scrIndex, ScreenPtr pScreen )
{
    gsxScreenPtr pScreenPriv = gsxGetScreenPriv(pScreen);

    // initialize ScreenPrivate

    // Get width & height from device
    pScreenPriv->width  = gsxScreens[scrIndex].width;
    pScreenPriv->height = gsxScreens[scrIndex].height;

    // Set Black & White pixel
    pScreenPriv->blackPixel = gsxScreens[scrIndex].blackPixel;
    pScreenPriv->whitePixel = gsxScreens[scrIndex].whitePixel;

    pScreenPriv->framerate = gsxScreens[scrIndex].framerate;
    pScreenPriv->depth = gsxScreens[scrIndex].depth;
    pScreenPriv->mode = gsxScreens[scrIndex].mode;
    pScreenPriv->res = gsxScreens[scrIndex].res;
    pScreenPriv->interlace = gsxScreens[scrIndex].interlace;
    pScreenPriv->odd_even_mix = gsxScreens[scrIndex].odd_even_mix;
    pScreenPriv->margin_x = gsxScreens[scrIndex].margin_x;
    pScreenPriv->margin_y = gsxScreens[scrIndex].margin_y;

    pScreenPriv->fbw = (pScreenPriv->margin_x+pScreenPriv->width+63)/64;
    pScreenPriv->fb_height = pScreenPriv->margin_y+pScreenPriv->height;
    pScreenPriv->fbp = GSX_FRAME_BASE;	// 2048 GSWORD unit

    switch (pScreenPriv->depth) {
    case 16:
        pScreenPriv->psm = PS2_GS_PSMCT16;
	pScreenPriv->rgb_pix_mask = 0x7fff;

        pScreenPriv->fbmask_alpha_shift = 31;
	pScreenPriv->fbmask_red_shift = 3;
	pScreenPriv->fbmask_green_shift = 11;
	pScreenPriv->fbmask_blue_shift = 19;

        pScreenPriv->fbmask_red_mask = 
		0x1f << pScreenPriv->fbmask_red_shift;
        pScreenPriv->fbmask_green_mask =
		0x1f << pScreenPriv->fbmask_green_shift;
        pScreenPriv->fbmask_blue_mask =
		0x1f << pScreenPriv->fbmask_blue_shift;
        pScreenPriv->fbmask_alpha_mask =
		0x1 << pScreenPriv->fbmask_alpha_shift;
        break;
    case 24:
        pScreenPriv->psm = PS2_GS_PSMCT32;
	pScreenPriv->rgb_pix_mask = 0x00ffffff;

	pScreenPriv->fbmask_red_shift = 0;
	pScreenPriv->fbmask_green_shift = 8;
	pScreenPriv->fbmask_blue_shift = 16;
        pScreenPriv->fbmask_alpha_shift = 24;

        pScreenPriv->fbmask_alpha_mask =
		0xff << pScreenPriv->fbmask_alpha_shift;
        pScreenPriv->fbmask_red_mask = 
		0xff << pScreenPriv->fbmask_red_shift;
        pScreenPriv->fbmask_green_mask =
		0xff << pScreenPriv->fbmask_green_shift;
        pScreenPriv->fbmask_blue_mask =
		0xff << pScreenPriv->fbmask_blue_shift;
        break;
    default:
        ErrorF("initPrivateRegisters: illegal depth:%d\n", pScreenPriv->depth);
        return FALSE;
    }
    // disbale alpha planes
    pScreenPriv->cur_fbmask = pScreenPriv->fbmask_alpha_mask;

   {
        extern int	ps2gsfd;
        struct ps2_gsinfo gsinfo;

#if 0
        ioctl(ps2gsfd, PS2IOC_GSINFO, &gsinfo);
#else
	gsinfo.size = 4 * 1024 * 1024;		/***** TODO *****/
#endif
#ifdef DEBUG
#define GSWORD_SIZE 4
	ErrorF("CT start::0x%x(byte): buffer size:0x%x(byte)\n", 
		bp*64*GSWORD_SIZE, gsinfo.size);
#endif
        pScreenPriv->gsbuffer_size = gsinfo.size;
    }


    return TRUE;
}


static Bool gsxInit(int index, ScreenPtr pScreen, int argc, char **argv)
{
    gsxScreenPtr pScreenPriv;

    // open GS
    if (0 != gsosOpen()) {
        ErrorF("gsxInit: gsosOpen failed\n");
        return FALSE;
    }
#if 0
    gsosAcquire();
#endif

    // allocation of gsxScreen private
    pScreenPriv = (gsxScreenPtr)xalloc(sizeof(gsxScreenPrivate));
    if (0 != pScreenPriv) {
        pScreen->devPrivates[gsxScreenIndex].ptr = (pointer)pScreenPriv;

        // Initialize ScreenPrivate
        if (TRUE == gsxScreenPrivateInit(index,pScreen)) {

            if (TRUE == gsxScreenInit(pScreen,monitorResolution)) {
                return TRUE;
       	    }
            // fail in gsxScreenInit
        }
        // fail to initialize screen
        xfree(pScreenPriv);
    }

    // close GS (fail to alloc or initialize screen)
    gsosClose();

    return FALSE;
}

void  gsxGSBlockHandler(pTimeout, pBlockData, pReadmask)
    pointer pTimeout;
    pointer pBlockData;
    pointer pReadmask; 
{
    if (ps2count > 0 && xf86VTSema)
        gsosDoKick();
}


void InitOutput(pScreenInfo, argc, argv)
    ScreenInfo 	  *pScreenInfo;
    int     	  argc;
    char    	  **argv;
{
    int i;
    int NumFormats = 0;

    if (serverGeneration == 1) {
        gsxProbe();
    }
    xf86Config(TRUE) ;
    xf86OpenConsole() ;

#ifdef XKB
    xf86InitXkb();
#endif

    if (!monitorResolution)
	monitorResolution = 100;

    /* must have a pixmap depth to match every screen depth */
    for (i = 0; i < gsxNumScreens; i++)
    {
	gsxPixmapDepths[(int)gsxScreens[i].depth] = TRUE;
    }

    for (i = 1; i <= 32; i++)
    {
	if (gsxPixmapDepths[i])
	{
	    if (NumFormats >= MAXFORMATS)
		FatalError ("MAXFORMATS is too small for this server\n");
	    pScreenInfo->formats[NumFormats].depth = i;
	    pScreenInfo->formats[NumFormats].bitsPerPixel = 
	    					formatBitsPerPixel(i);
	    pScreenInfo->formats[NumFormats].scanlinePad = BITMAP_SCANLINE_PAD;
	    NumFormats++;
	}
    }

    pScreenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    pScreenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    pScreenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;
    pScreenInfo->numPixmapFormats = NumFormats;

    // Reset server
    if (gsxGeneration != serverGeneration) {
        gsxGeneration = serverGeneration;
        gsxServerReset();
    }

    for (i = 0; i < gsxNumScreens; i++) {
        if (-1 == AddScreen(gsxInit, argc, argv)) {
            FatalError("Couldn't add screen %d", i);
        }
    }

#ifdef GSOS_LAZY_EXEC
    RemoveBlockAndWakeupHandlers((BlockHandlerProcPtr) gsxGSBlockHandler, 
				(WakeupHandlerProcPtr)NoopDDA,
                                NULL);
    RegisterBlockAndWakeupHandlers((BlockHandlerProcPtr) gsxGSBlockHandler, 
				(WakeupHandlerProcPtr)NoopDDA,
                                NULL);
#endif

}

