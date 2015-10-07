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

#include <linux/ps2/gs.h>
#include "mi.h"
#include "mipointer.h"
#include "gsx.h"

#ifdef MITSHM
#define _XSHM_SERVER_
#include "extensions/shmstr.h"
#endif /* MITSHM */

extern Bool gsxDCInitialize (
    ScreenPtr               pScreen,
    miPointerScreenFuncPtr  screenFuncs
);

extern int disableBackingStore;
extern miBSFuncRec gsxBSFuncs;

int gsxWindowIndex; // reference from gsxGetWindowPriv(w) in gsx.h

extern Bool xf86Exiting, xf86Resetting;
extern WindowPtr *WindowTable;
static ScreenPtr savepScreen = NULL;
static PixmapPtr ppix = NULL;

#ifdef MITSHM
extern void ShmRegisterFuncs();
extern void ShmSetPixmapFormat();

extern void gsxShmPutImage();
extern PixmapPtr gsxShmCreatePixmap();
static ShmFuncs gsxShmFuncs = {gsxShmCreatePixmap, gsxShmPutImage};
#endif /* MITSHM */

static Bool gsxCursorOffScreen (ppScreen, x, y)
    ScreenPtr   *ppScreen;
    int         *x, *y;
{
    return FALSE;
}

static void gsxCrossScreen (pScreen, entering)
    ScreenPtr   pScreen;
    Bool        entering;
{
}

#ifdef XINPUT
extern void xf86eqEnqueue ( struct _xEvent * );
#endif

static miPointerScreenFuncRec gsxPointerCursorFuncs =
{
    gsxCursorOffScreen,
    gsxCrossScreen,
    miPointerWarpCursor,
#ifdef XINPUT
    xf86eqEnqueue,
#endif
};


Bool gsxSetupScreen( ScreenPtr pScreen )
{
    if (!mfbAllocatePrivates(pScreen, &gsxWindowIndex, &gsxGCIndex))
        return FALSE;
    if (!cfbAllocatePrivates(pScreen, &gsxWindowIndex, &gsxGCIndex))
        return FALSE;
    if (!cfb16AllocatePrivates(pScreen, &gsxWindowIndex, &gsxGCIndex))
        return FALSE;
    if (!cfb32AllocatePrivates(pScreen, &gsxWindowIndex, &gsxGCIndex))
        return FALSE;

    if (!AllocateWindowPrivate(pScreen, gsxWindowIndex,
         sizeof(gsxWindowPrivate)) ||
        !AllocateGCPrivate(pScreen, gsxGCIndex,
         sizeof(gsxGCPrivate)) )
                return FALSE;

    // setting pScreen methods in cfbSetupScreen
    pScreen->defColormap = FakeClientID(0);
    /* let CreateDefColormap do whatever it wants for pixels */
    pScreen->blackPixel = pScreen->whitePixel = (Pixel) 0;

    pScreen->CloseScreen = gsxCloseScreen;
    pScreen->QueryBestSize = gsxQueryBestSize;
    pScreen->SaveScreen = gsxSaveScreen;
    pScreen->GetImage = gsxGetImage;
    pScreen->GetSpans = gsxGetSpans;
    pScreen->CreateWindow = gsxCreateWindow;
    pScreen->DestroyWindow = gsxDestroyWindow;
    pScreen->PositionWindow = gsxPositionWindow;
    pScreen->ChangeWindowAttributes = gsxChangeWindowAttributes;
    pScreen->RealizeWindow = gsxMapWindow;
    pScreen->UnrealizeWindow = gsxUnmapWindow;
    pScreen->PaintWindowBackground = gsxPaintWindow;
    pScreen->PaintWindowBorder = gsxPaintWindow;
    pScreen->CopyWindow = gsxCopyWindow;
    pScreen->CreatePixmap = cfbCreatePixmap;
    pScreen->DestroyPixmap = cfbDestroyPixmap;
    pScreen->RealizeFont = mfbRealizeFont;
    pScreen->UnrealizeFont = mfbUnrealizeFont;
    pScreen->CreateGC = gsxCreateGC;
    pScreen->CreateColormap = cfbInitializeColormap;
    pScreen->DestroyColormap = (void (*)())NoopDDA;
    pScreen->InstallColormap = cfbInstallColormap;
    pScreen->UninstallColormap = cfbUninstallColormap;
    pScreen->ListInstalledColormaps = cfbListInstalledColormaps;
    pScreen->StoreColors = (void (*)())NoopDDA;
    pScreen->ResolveColor = cfbResolveColor;
    pScreen->BitmapToRegion = mfbPixmapToRegion;

    mfbRegisterCopyPlaneProc(pScreen, miCopyPlane);
//    mfbRegisterCopyPlaneProc(pScreen, cfbCopyPlane);

    return TRUE;
}

Bool gsxFinishScreenInit( ScreenPtr pScreen, int xsize, int ysize,
                          int dpix, int dpiy)
{
    VisualPtr pVisuals;
    int       nVisuals; 
    DepthPtr  pDepths;
    int       nDepths;
    VisualPtr pVisual;
    VisualID  defaultVisualID;
    int       rootDepth = 0;
    int       rtn;
    int       composite_res;
    int       depth;

    depth = gsxGetScreenPriv(pScreen)->depth;
    // Initialize Visual
    rtn = gsxCreateVisuals(pScreen,&pVisuals,&nVisuals,&pDepths,&nDepths);
    if (!rtn)	return FALSE;
    pVisual = gsxGetVisual(nVisuals,pVisuals,TrueColor,depth);
    if (pVisual) {
        defaultVisualID = pVisual->vid;
        rootDepth = pVisual->nplanes;
    } else {
      return FALSE;
    }

    gsxGetScreenPriv(pScreen)->pVisual = pVisual;
    // Set Black & White Pixel
    pScreen->blackPixel = gsxGetScreenPriv(pScreen)->blackPixel;
    pScreen->whitePixel = gsxGetScreenPriv(pScreen)->whitePixel;    

    // Initialize mi
    // We call miScreenInit with pixelWidth=0,
    // so pScreen->CloseScreen is not changed to miCloseScreen in it.
    if (!miScreenInit(pScreen,0/* pointer to screen bitmap */,
                      xsize,ysize,dpix,dpiy,
                      0/* pixel width of frame buffer */,
                      rootDepth,nDepths,pDepths, 
                      defaultVisualID,nVisuals,pVisuals,
		      disableBackingStore ? NULL: &gsxBSFuncs))
        return FALSE;
#ifdef MITSHM
    ShmRegisterFuncs(pScreen, &gsxShmFuncs);
    ShmSetPixmapFormat(pScreen, ZPixmap);
#endif

    switch (gsxGetScreenPriv(pScreen)->mode) {
      case PS2_GS_VESA:
      	composite_res = gsxGetScreenPriv(pScreen)->res | 
		gsxGetScreenPriv(pScreen)->framerate;
	break;
      case PS2_GS_PAL:
      case PS2_GS_NTSC:
        composite_res =  gsxGetScreenPriv(pScreen)->interlace ;
	break;
      default: /* PS2_GS_DTV */
      	composite_res = gsxGetScreenPriv(pScreen)->res;
	break;
    }

    gsosSetScreen(
        gsxGetScreenPriv(pScreen)->mode,
        composite_res,   
        xsize, ysize,
        gsxGetScreenPriv(pScreen)->fbp,
        gsxGetScreenPriv(pScreen)->psm,
	gsxGetScreenPriv(pScreen)->cur_fbmask,
	gsxGetScreenPriv(pScreen)->odd_even_mix,
	gsxGetScreenPriv(pScreen)->margin_x,
	gsxGetScreenPriv(pScreen)->margin_y);

    if (gsosAcquire() < 0)
	return FALSE;
    gsxInitGSregs(pScreen);

#if GSX_CACHED_TILE
    gsxInitCT(pScreen);
#endif

    // Initialize Display Cursor
#if GSX_ACCL_SPRITE_DC
    if (!gsxDCInitialize(pScreen, &gsxPointerCursorFuncs)) 
        return FALSE;
#else
    if (!miDCInitialize(pScreen, &gsxPointerCursorFuncs))
        return FALSE;
#endif

    // Create Default Colormap
    return gsxCreateDefColormap(pScreen);

}


Bool gsxScreenInit( ScreenPtr pScreen, int dpixy)
{
  // Allocate Window & GC private
  // setup the pScreen methods
  if (TRUE == gsxSetupScreen(pScreen)) {
    int w,h;
    w = gsxGetScreenPriv(pScreen)->width;
    h = gsxGetScreenPriv(pScreen)->height;
    if (TRUE == gsxFinishScreenInit(pScreen, w, h, dpixy, dpixy)) {
      savepScreen = pScreen;
      return TRUE;
    }
    // fail in gsxFinishScreenInit
    // Reset gsxSetupScreen
  }
  // fail in gsxSetupScreen
  return FALSE;
}


Bool
gsxCloseScreen (index, pScreen)
    int         index;
    ScreenPtr   pScreen;
{
    gsxScreenPrivate *pScreenPriv = gsxGetScreenPriv(pScreen);

    /* save screen */
    (void)(*pScreen->SaveScreen)(pScreen, SCREEN_SAVER_OFF);

    /* clean visualIDs & visuals & depths */
    /* VisualIDs & Visuals & Depths are in the pVisualIDs area */
    xfree (pScreenPriv->pVisualIDs);

#if GSX_CACHED_TILE
    gsxCloseCT(pScreen);
#endif

    /* clean screen private */
    xfree (pScreenPriv);

    gsosClose();
    return TRUE;
}


/*
 *      Assign a new serial number to the window.
 *      Used to force GC validation on VT switch.
 */

/*ARGSUSED*/
static int
gsxNewSerialNumber(pWin, data)
    WindowPtr pWin;
    pointer data;
{
    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    return WT_WALKCHILDREN;
}


/*
 * gsxEnterLeaveVT -- 
 *      grab/ungrab the current VT completely.
 */

void
gsxEnterLeaveVT(enter, screen_idx)
    Bool enter;
    int screen_idx;
{
  ScreenPtr pScreen = savepScreen;

#ifdef DEBUG
  ErrorF("gsxEnterLeaveVT: ");
  if (enter)
  	ErrorF("Enter\n");
  else
  	ErrorF("Leave\n");
#endif

  /* Force every GC writing to the screen to be validated.  */
  if (pScreen && !xf86Exiting && !xf86Resetting)
      WalkTree(pScreen, gsxNewSerialNumber, 0);

  if (enter)
    {
      while (gsosAcquire() < 0) {
	  gsosRelease();
	  sleep(1);
      }
      gsxInitGSregs(pScreen);

      /*
       * copy the dummy buffer to the real screen.
       */
      if (!xf86Resetting)
      {
	gsxScreenPtr pScreenPriv; 
	pScreenPriv = gsxGetScreenPriv(pScreen);

	gsosWriteImage(0, 0, pScreen->width, pScreen->height,
		      pScreenPriv->fbp, pScreenPriv->fbw, pScreenPriv->psm,
		      ppix->devPrivate.ptr);
      }
      if (ppix) {
	(pScreen->DestroyPixmap)(ppix);
        ppix = NULL;
      }

#if GSX_CACHED_TILE
      gsxReloadTileAndStippleCT(pScreen);
#if GSX_ACCL_SPRITE_DC
      gsxReloadCursorCT(pScreen);
#endif /* GSX_ACCL_SPRITE_DC */
#if GSX_ACCL_DASH
      gsxReloadDashCT(pScreen);
#endif /* GSX_ACCL_DASH */
#endif /* GSX_CACHED_TILE */

#if GSX_ACCL_SPRITE_DC
	  gsxSpriteRestoreCursor(pScreen);
#endif /* GSX_ACCL_SPRITE_DC */
    }
  else
    {
      /*
       * Create a dummy pixmap to write to while VT is switched out.
       * Copy the screen to that pixmap
       */
      if (!xf86Exiting)
      {
        ppix = (pScreen->CreatePixmap)(pScreen, pScreen->width, 
				       pScreen->height, pScreen->rootDepth);
        if (ppix)
        {
	  gsxScreenPtr pScreenPriv; 
	  pScreenPriv = gsxGetScreenPriv(pScreen);

#if GSX_ACCL_SPRITE_DC
	  gsxSpriteRemoveCursor(pScreen);
#endif /* GSX_ACCL_SPRITE_DC */

	  gsosReadImage(0, 0, pScreen->width, pScreen->height,
			pScreenPriv->fbp, pScreenPriv->fbw, pScreenPriv->psm,
			ppix->devPrivate.ptr);
	  pScreen->devPrivates[cfb32ScreenPrivateIndex].ptr = ppix;
        }

        gsosRelease();
      }
    }

#ifdef DEBUG
  ErrorF("gsxEnterLeaveVT: Done\n");
#endif
}
