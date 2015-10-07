/*
 * $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Init.c,v 3.66.2.4 1998/10/11 12:35:37 hohndel Exp $
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $XConsortium: xf86Init.c /main/37 1996/10/23 18:43:39 kaleb $ */

#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#else
extern int atoi();
#endif

#define NEED_EVENTS
#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "input.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "site.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86Version.h"
#include "mipointer.h"

#ifdef XINPUT
#include "XI.h"
#include "XIproto.h"
#include "xf86_Config.h"
#include "xf86Xinput.h"
#else
#include "inputstr.h"
#endif

#include "opaque.h"

#ifdef XTESTEXT1
#include "atKeynames.h"
extern int xtest_command_key;
#endif /* XTESTEXT1 */

/* xf86Exiting is set while the screen is shutting down (even on a reset) */
Bool xf86Exiting = FALSE;
Bool xf86Resetting = FALSE;
Bool xf86ProbeFailed = TRUE;
Bool xf86FlipPixels = FALSE;
#ifdef XF86VIDMODE
Bool xf86VidModeEnabled = TRUE;
Bool xf86VidModeAllowNonLocal = FALSE;
#endif
#ifdef XF86MISC
Bool xf86MiscModInDevEnabled = TRUE;
Bool xf86MiscModInDevAllowNonLocal = FALSE;
#endif
Bool xf86AllowMouseOpenFail = FALSE;
PciProbeType xf86PCIFlags = PCIProbe1;
Bool xf86ScreensOpen = FALSE;
int xf86Verbose = 1;
Bool xf86fpFlag = FALSE;
Bool xf86coFlag = FALSE;
Bool xf86sFlag = FALSE;
Bool xf86ProbeOnly = FALSE;
char xf86ConfigFile[PATH_MAX] = "";
int  xf86bpp = -1;
xrgb xf86weight = { 0, 0, 0 } ;	/* RGB weighting at 16 bpp */
double xf86rGamma=1.0, xf86gGamma=1.0, xf86bGamma=1.0;
unsigned char xf86rGammaMap[256], xf86gGammaMap[256], xf86bGammaMap[256];
char *xf86ServerName = NULL;
Bool xf86BestRefresh = FALSE;


xf86InfoRec xf86Info;

/*
 * InitInput --
 *      Initialize all supported input devices...what else is there
 *      besides pointer and keyboard? Two DeviceRec's are allocated and
 *      registered as the system pointer and keyboard devices.
 */

void
InitInput(argc, argv)
     int     	  argc;
     char    	  **argv;
{
  xf86Info.vtRequestsPending = FALSE;
  xf86Info.inputPending = FALSE;
#ifdef XTESTEXT1
  xtest_command_key = KEY_Begin + MIN_KEYCODE;
#endif /* XTESTEXT1 */

  xf86Info.pKeyboard = AddInputDevice(xf86Info.kbdProc, TRUE); 
  xf86Info.pMouse =  AddInputDevice(xf86Info.mouseDev->mseProc, TRUE);
  RegisterKeyboardDevice(xf86Info.pKeyboard); 
  RegisterPointerDevice(xf86Info.pMouse); 

#ifdef XINPUT
  (xf86Info.pMouse)->public.devicePrivate = xf86Info.mouseLocal;
  ((LocalDevicePtr) xf86Info.mouseLocal)->dev = xf86Info.pMouse;
#else
  (xf86Info.pMouse)->public.devicePrivate = (pointer) xf86Info.mouseDev;
#endif

#ifdef XINPUT
  InitExtInput();
#endif
  
  miRegisterPointerDevice(screenInfo.screens[0], xf86Info.pMouse);
#ifdef XINPUT
  xf86XinputFinalizeInit(xf86Info.pMouse);
  xf86eqInit ((DevicePtr)xf86Info.pKeyboard, (DevicePtr)xf86Info.pMouse);
#else
  mieqInit (xf86Info.pKeyboard, xf86Info.pMouse);
#endif
}


#ifdef DPMSExtension
/*
 * DPMSSet --
 *	Device dependent DPMS mode setting hook.  This is called whenever
 *	the DPMS mode is to be changed.
 */
void
DPMSSet(CARD16 level)
{
}

/*
 * DPMSSupported --
 *	Return TRUE if any screen supports DPMS.
 */
Bool
DPMSSupported(void)
{
    return FALSE;
}
#endif /* DPMSExtension */

