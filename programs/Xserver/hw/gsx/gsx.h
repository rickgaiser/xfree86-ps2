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

#ifndef GSX_H
#define GSX_H

#include "scrnintstr.h"
#include "cfb.h"
#ifdef MITSHM
#define _XSHM_SERVER_
#include "extensions/shmstr.h"
#endif

#ifndef __R5900
#define __R5900
#endif

extern Bool cfb16CreateGC(GCPtr pGC);
extern Bool cfb32CreateGC(GCPtr pGC);
extern Bool cfb16AllocatePrivates(
	ScreenPtr /*pScreen*/,
	int * /*window_index*/,
	int * /*gc_index*/
);

extern Bool cfb32AllocatePrivates(
	ScreenPtr /*pScreen*/,
	int * /*window_index*/,
	int * /*gc_index*/
);

extern void cfb16GetImage(
    DrawablePtr /*pDrawable*/,
    int /*sx*/,
    int /*sy*/,
    int /*w*/,
    int /*h*/,
    unsigned int /*format*/,
    unsigned long /*planeMask*/,
    char * /*pdstLine*/
);

extern void cfb32GetImage(
    DrawablePtr /*pDrawable*/,
    int /*sx*/,
    int /*sy*/,
    int /*w*/,
    int /*h*/,
    unsigned int /*format*/,
    unsigned long /*planeMask*/,
    char * /*pdstLine*/
);



/*
** optimize switchs
*/
#define GSX_CACHED_TILE	TRUE	/* Use cached tile */
#define GSX_ACCL_DASH	TRUE	/* Put dash pattern into texture */
#define GSX_ACCL_SPRITE_DC TRUE	/* Put currsor int texture */

#if !GSX_CACHED_TILE
#undef GSX_ACCL_DASH
#define GSX_ACCL_DASH	FALSE
#undef GSX_ACCL_SPRITE_DC
#define GSX_ACCL_SPRITE_DC FALSE
#endif

/*
** constant
*/
#define GSX_ENABLE        1
#define GSX_DISABLE       0
#define GSX_FBA_VALUE0    0
#define GSX_FBA_VALUE1    1
#define GSX_COLMASK_ALL   0
#define GSX_COLMASK_ASHUT 1
#define GSX_COLMASK_AONLY 2

#define GSX_FRAME_BASE    (0x0 / 64)
#define GSX_ZBUF_BASE     0x0	/* unused */
#define GSX_TEX0_BASE     0x0	/* unused */

#define GSX_TEXTURE_CORD_MAX_X 1024	/* 2^10 */
#define GSX_TEXTURE_CORD_MAX_Y 1024	/* 2^10 */
#define GSX_TEXTURE_FORWARD_W (GSX_TEXTURE_CORD_MAX_X/2)
#define GSX_TEXTURE_FORWARD_H (GSX_TEXTURE_CORD_MAX_Y/2)

/*
 * GSX LOCAL
 */
#include "X.h"
#include "Xproto.h"
#include "gcstruct.h"
#include "dixfontstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "gsos.h"

typedef int GSXint ;
typedef unsigned int GSXuint ;
typedef short GSXshort ;
typedef unsigned short GSXushort ;
typedef char GSXchar ;
typedef unsigned char GSXuchar ;
typedef long GSXlong ;
typedef unsigned long GSXulong ;
typedef float GSXfloat ;
typedef double GSXdouble ;
typedef int GSXWindowID ;


typedef struct gsxTileTextureRec  gsxTileTexture;
struct gsxTileTextureRec {
    unsigned short	tbp;
    unsigned char	tbw;
    unsigned char	psm;
    unsigned char	log2tw;	
    unsigned char	log2th;
    unsigned char	fake_w;	
    unsigned char	fake_h;
};

#define TILE_CACHE_WIDTH	64
#define TILE_CACHE_HEIGHT	64

typedef struct gsxCachedTileRec gsxCachedTile;
typedef struct gsxCachedTileRec * gsxCachedTilePtr;

struct gsxCachedTileRec {
    gsxCachedTilePtr	prev;
    gsxCachedTilePtr	next;
    int			refcnt;
    char		width;
    char		height;
    char		depth;
    char		fillStyle;
    unsigned	int	fg_pix, bg_pix;
    gsxTileTexture	texinfo;
    unsigned int 	*data;
};


typedef struct {
    /* copy of each register */
    GSOSbit64   xyoffset_1 ;
    GSOSbit64   frame_1 ; 
    GSOSbit64   zbuf_1 ;
    GSOSbit64   prmodecont ;
    GSOSbit64   tex0_1 ; 
    GSOSbit64   tex1_1 ;
    GSOSbit64   tex2_1 ;
    GSOSbit64   clamp_1 ;
    GSOSbit64   miptbp1_1 ;
    GSOSbit64   miptbp2_1 ;
    GSOSbit64   scissor_1 ;
    GSOSbit64   alpha_1 ;
    GSOSbit64   test_1 ;
    GSOSbit64   fba_1 ;
    GSOSbit64   prmode ;
    GSOSbit64   texclut ;
    GSOSbit64   texa ;
    GSOSbit64   dimx ;
    GSOSbit64   dthe ;
    GSOSbit64   colclamp ;
    GSOSbit64   pabe ;
} gsxGSRegs, *gsxGSRegsPtr;

typedef struct {
    unsigned short  width, height;  /* screen size */
    Pixel blackPixel;
    Pixel whitePixel;
    VisualID *pVisualIDs;

    // descriptions about fbmask layout
    char fbmask_alpha_shift;
    char fbmask_red_shift;
    char fbmask_green_shift;
    char fbmask_blue_shift;
    unsigned int fbmask_alpha_mask;	// 1: alpha portion
    unsigned int fbmask_red_mask;	// 1: red portion
    unsigned int fbmask_green_mask;	// 1: green portion
    unsigned int fbmask_blue_mask;	// 1: blue portion
        
    // descriptions abourt frame buffer
    unsigned char psm;
    unsigned int fbw;
    unsigned int fbp;
    unsigned int fb_height;
    unsigned int gsbuffer_size;
    GSXuint rgb_pix_mask;
    VisualPtr	pVisual;
    short framerate;			/* PS2_GS_75Hz or PS2_GS_60Hz */
    char depth;				/* depth: 16 or 24 */
    int mode;
    int res;
    int interlace;
    short odd_even_mix;
    short margin_x,margin_y;

    // misc.
    unsigned int cur_fbmask;		/* in GS, 1:write disable */
    GSXuint gsxFg, gsxBg;		// CopyPlane fg/bg

#if GSX_CACHED_TILE
    pointer cachedTiles_data;
    gsxCachedTilePtr cachedTiles;
    gsxCachedTilePtr freedTiles;
    gsxCachedTilePtr usedTiles;
    short	reservedCTs;
#endif
#if GSX_ACCL_DASH
    gsxCachedTilePtr dashCT;
    char	dashLen;    
    char	lastNumInDashList;
    unsigned char	lastDashStyle;
    char	lastDash[TILE_CACHE_WIDTH];
#endif
#if GSX_ACCL_SPRITE_DC
    gsxCachedTilePtr saveDCArea;
    gsxCachedTilePtr cursorCT;
#endif
} gsxScreenPrivate, *gsxScreenPtr;

#define gsxGetScreenPriv(s) \
    ((gsxScreenPtr)(s)->devPrivates[gsxScreenIndex].ptr)


/* private field of window */
typedef struct {
    unsigned	char fastBorder; /* non-zero if border is 32 bits wide */
    unsigned	char fastBackground;
    unsigned	short unused;	/* pad for alignment with Sun compiler */
    DDXPointRec	oldRotate;
    PixmapPtr	pRotatedBackground;
    PixmapPtr	pRotatedBorder;
#if GSX_CACHED_TILE
    gsxCachedTilePtr	pCachedBackground;
    gsxCachedTilePtr	pCachedBorder;
#endif

} gsxWindowPrivate, *gsxWindowPtr;

#define gsxGetWindowPriv(w) \
    ((gsxWindowPtr)(w)->devPrivates[gsxWindowIndex].ptr)

typedef struct {
    unsigned char	rop;            /* special case rop values */
    /* next two values unused in cfb, included for compatibility with mfb */
    unsigned char	ropOpStip;      /* rop for opaque stipple */
    /* this value is ropFillArea in mfb, usurped for cfb */
    unsigned char	oneRect;    /*  drawable has one clip rect */
    unsigned		fExpose:1;  /* callexposure handling ? */
    unsigned		freeCompClip:1;
    PixmapPtr		pRotatedPixmap;
    RegionPtr		pCompositeClip; /* FREE_CC or REPLACE_CC */
    unsigned long   xor, and;   /* reduced rop values */
    /* KEEP above fields, reserved for cfbXXX()  */

    /* gsx privates */
#if GSX_CACHED_TILE
    gsxCachedTilePtr	pCachedTile;
#endif
    GCFuncs     *funcs;
    GCOps       *mops;      /* ops for pixmap */
    GCOps       *wops;      /* ops for window */
    GCOps       ops;        /* ops holder */
    Mask        changes;    /* changes while DRAWABLE_PIXMAP */
    unsigned short  patW, patH; 	/* size of tile or stipple */
    unsigned int	fbmask;		/* in GS, 1:write disable */

} gsxGCPrivate, *gsxGCPtr;

#define gsxGetGCPriv(gc) \
    ((gsxGCPtr)(gc)->devPrivates[gsxGCIndex].ptr)

#define GSXCHANGEGC( g, m, v )  \
    { \
        XID val = (v) ; \
        DoChangeGC( (g), (m), &(val), 0 ) ; \
    }

#define GSXVALIDATEGC( d, g )  ValidateGC( (d), (g) )


/*
 * gsx private index
 */
extern int gsxScreenIndex;
extern int gsxGCIndex;
extern int gsxWindowIndex;


/*
 * gsx screen functions
 */
extern Bool gsxCloseScreen();
extern void gsxQueryBestSize();
extern Bool gsxSaveScreen();
extern void gsxGetImage();
extern void gsxGetSpans();
extern Bool gsxCreateWindow();
extern Bool gsxDestroyWindow();
extern Bool gsxPositionWindow();
extern Bool gsxChangeWindowAttributes();
extern Bool gsxMapWindow();
extern Bool gsxUnmapWindow();
extern void gsxPaintWindow();
extern void gsxCopyWindow();
extern Bool gsxCreateGC();

/*
 * GC functions
 */
extern void gsxValidateGC();
extern void gsxChangeGC();
extern void gsxCopyGC();
extern void gsxDestroyGC();
extern void gsxChangeClip();
extern void gsxDestroyClip();
extern void gsxCopyClip();

/*
 * GC ops
 */
extern RegionPtr gsxCopyAreaLop() ;
extern RegionPtr gsxCopyAreaGxcpy() ;
extern RegionPtr gsxCopyAreaGxcpyPm() ;
extern RegionPtr gsxCopyPlaneLop() ;
extern RegionPtr gsxCopyPlaneGxcpy() ;
extern RegionPtr gsxCopyPlaneGxcpyPm() ;
extern void gsxDoBitBlt() ;
extern void gsxPolyPointSoft() ;
extern void gsxPolyPointGxcpy() ;
extern void gsxPolyLinesSoft() ;
extern void gsxPolyLinesGxcpy() ;
extern void gsxPolySegmentSoft() ;
extern void gsxPolySegmentGxcpy() ;
extern void gsxPolyRectangleSoft() ;
extern void gsxPolyArcSoft() ;
extern void gsxFillPolygonSoft() ;
extern void gsxPolyFillRectSoft() ;
extern void gsxPolyFillRectGxcpy() ;
extern void gsxPolyFillArcSoft() ;
extern void gsxImageText8Cached() ;
extern GSXint gsxPolyText8Cached() ;
extern void gsxFillSpansSoft() ;
extern void gsxFillSpansGxcpy() ;
extern void gsxFillSpansCached() ;
extern void gsxSetSpansSoft() ;
extern void gsxSetSpansGxcpy() ;
extern void gsxImageGlyphBltSoft();
extern void gsxImageGlyphBltGxcpy();
extern void gsxPolyGlyphBltSoft();
extern void gsxPushPixelsSoft();
extern void gsxPutImage();
extern void gsxPolyRectangleGxcpy() ;

/*
 * gsx functions
 */
extern Bool gsxScreenInit();
extern Bool gsxSetupScreen();
extern Bool gsxFinishScreenInit();
extern int gsxCreateVisuals();
extern Bool gsxCreateDefColormap();
extern VisualPtr gsxGetVisual();

extern void gsxCalcDrawRegion( DDXPointPtr pPoint, int n,  xRectangle *pRect );
extern void gsxCalcDrawRegionSeg( xSegment *pSegments, int n, xRectangle *pRect );
extern void gsxCalcDrawRegionSpan( DDXPointPtr pPoints, int *pWidths, int n, xRectangle *pRect );
extern void gsxCalcDrawRegionRect( xRectangle *pSrcRect, int n, xRectangle *pRect );
extern void gsxCalcDrawRegionArc( xArc *pArc, int n, xRectangle *pRect );
extern void gsxSetFBA( GCPtr pGC, unsigned int mask );
extern void gsxSetTexfunc( gsxGCPtr pPriv, int tfunc );
extern void gsxGetRgbaWithVisual(VisualPtr pVisual,
	unsigned int rgb, int *r, int *g, int *b, int *a );
extern void gsxCacheInvalid( void );
extern void gsxPixmapToFill( GCPtr pGC );
extern unsigned int gsxGetColormask( GCPtr pGC, unsigned int kind );

#define gsxGetRgba( pGC, rgb, r, g, b, a )	\
    gsxGetRgbaWithVisual(				\
	gsxGetScreenPriv((pGC)->pScreen)->pVisual,	\
	rgb,						\
	r,						\
	g,						\
	b,						\
	a						\
    )

#define gsxSetColormask(pScreenPriv, fbmask) 		\
do {							\
    if ( pScreenPriv->cur_fbmask != fbmask) {		\
        pScreenPriv->cur_fbmask = fbmask;		\
        gsosFrameInit((pScreenPriv)->fbw,		\
			(pScreenPriv)->psm,		\
                        (pScreenPriv)->fbp,		\
                        (pScreenPriv)->cur_fbmask);	\
    }							\
}							\
while (0)



extern void gsxInitGSregs(ScreenPtr pScreen);

/* Return expornent of the closest power of two greater than or equal to 
   given value */
extern const char gsxLog2GETbl[];
static inline int gsxGetLog2GE(int width)
{
  if (width<0||width>64) return -1;
  if (width>32) return 6;
  return gsxLog2GETbl[width];
}

#define gsxCanIgnorePlMask(planemask, rgb_pix_mask) \
    ! (~((unsigned int)planemask) & ((unsigned int)(rgb_pix_mask)))

#define gsxIsPlanemask(pGC) \
    ! (gsxCanIgnorePlMask((pGC)->planemask, gsxGetScreenPriv((pGC)->pScreen)->rgb_pix_mask))

#define gsxGetBytePerPixel(pScreenPriv) \
	gsosBytePerPixel((pScreenPriv)->psm)

#define gsxFrameInit(pScreenPriv) \
    gsosFrameInit((pScreenPriv)->fbw, 		\
			(pScreenPriv)->psm, 	\
			(pScreenPriv)->fbp,	\
			(pScreenPriv)->cur_fbmask	\
			)


/*
 * ReadImage & WriteImage wrapper
 */
extern int gsxReadImage(int x, int y, int w, int h, unsigned int bp, int bw, int psm,PixmapPtr pPix);
extern int gsxWriteImage(int x, int y, int w, int h, unsigned int bp, int bw, int psm, PixmapPtr pPix);

#define gsxGetLineMargin(pGC)	\
	((((pGC)->lineWidth/2)<3 ? 3 : (pGC)->lineWidth/2)+1)

/*
 * Debug
 */
extern void gsxPixmapDump(PixmapPtr pPix, int image);
extern void gsxImageDump(void *p, int w, int h, int bsz, int image);

#if GSX_CACHED_TILE
/*
 * texture cached tile functions
 */
extern void gsxInitCT(ScreenPtr pScreen);
extern void gsxCloseCT(ScreenPtr pScreen);
extern gsxCachedTilePtr gsxGetCT(ScreenPtr pScreen, 
					GCPtr pGC, PixmapPtr pTile);
extern void gsxDestroyCT(ScreenPtr pScreen, gsxCachedTilePtr pCT);
void gsxPolyFillRectCached(
    DrawablePtr dst,
    GCPtr   pGC,
    int     n,
    xRectangle  *pRect);

#endif

#if GSX_ACCL_DASH
/*
 * accelerated dashed line functions
 */
Bool gsxCheckDash(GCPtr pGC, int do_register);
void gsxPolySegmentDashCT(
    DrawablePtr dst,
    GCPtr   pGC,
    int     n,
    xSegment    *pSegments);

void gsxPolyLinesDashCT(
    DrawablePtr dst,
    GCPtr   pGC,
    int     mode,
    int     n,
    DDXPointPtr pPoint);

void gsxDoPolyLinesDashCT(
    DrawablePtr dst,
    GCPtr   pGC,
    int     mode,
    int     n,
    DDXPointPtr pPoint);

void gsxPolyRectangleDashCT(
    DrawablePtr dst,
    GCPtr   pGC,
    int     n,
    xRectangle  *pRect);

#endif /* GSX_ACCL_DASH */


#ifdef GSOS_SPR_IMAGEBUFFER
#define IS_SINGLE_OP_COVERED(x,bytepp)	(((x)*(bytepp)) < gsos_maximage_size)
#else
#define IS_SINGLE_OP_COVERED(x,bytepp)	(((x)*(bytepp)) < gsos_maxpacket_size)
#endif


#define GSX_SPRITE_PAD  8

#if  GSX_ACCL_SPRITE_DC
/* for gsxSprite and gsxDisplayCursor */
typedef struct {
    Bool	(*RealizeCursor)(
		ScreenPtr /*pScreen*/,
		CursorPtr /*pCursor*/
    );
    Bool	(*UnrealizeCursor)(
		ScreenPtr /*pScreen*/,
		CursorPtr /*pCursor*/
    );
    Bool	(*PutUpCursor)(
		ScreenPtr /*pScreen*/,
		CursorPtr /*pCursor*/,
		int /*x*/,
		int /*y*/,
		unsigned long /*source*/,
		unsigned long /*mask*/
    );
    Bool	(*SaveUnderCursor)(
		ScreenPtr /*pScreen*/,
		int /*x*/,
		int /*y*/,
		int /*w*/,
		int /*h*/
    );
    Bool	(*RestoreUnderCursor)(
		ScreenPtr /*pScreen*/,
		int /*x*/,
		int /*y*/,
		int /*w*/,
		int /*h*/
    );
    Bool	(*MoveCursor)(
		ScreenPtr /*pScreen*/,
		CursorPtr /*pCursor*/,
		int /*x*/,
		int /*y*/,
		int /*w*/,
		int /*h*/,
		int /*dx*/,
		int /*dy*/,
		unsigned long /*source*/,
		unsigned long /*mask*/
    );
    Bool	(*ChangeSave)(
		ScreenPtr /*pScreen*/,
		int /*x*/,
		int /*y*/,
		int /*w*/,
		int /*h*/,
		int /*dx*/,
		int /*dy*/
    );
    Bool	(*GetRealizedSize)(
		ScreenPtr /*pScreen*/,
		CursorPtr /*pCursor*/,
		int */*w*/,
		int */*h*/,
		int */*hotx*/,
		int */*hoty*/
    );
} gsxSpriteCursorFuncRec, *gsxSpriteCursorFuncPtr;

typedef struct {
    CloseScreenProcPtr			CloseScreen;
    GetImageProcPtr			GetImage;
    GetSpansProcPtr			GetSpans;
    SourceValidateProcPtr		SourceValidate;
    CreateGCProcPtr			CreateGC;
    ScreenBlockHandlerProcPtr		BlockHandler;
    InstallColormapProcPtr		InstallColormap;
    StoreColorsProcPtr			StoreColors;
    PaintWindowBackgroundProcPtr	PaintWindowBackground;
    PaintWindowBorderProcPtr		PaintWindowBorder;
    CopyWindowProcPtr			CopyWindow;
    ClearToBackgroundProcPtr		ClearToBackground;
    SaveDoomedAreasProcPtr		SaveDoomedAreas;
    RestoreAreasProcPtr			RestoreAreas;

    CursorPtr	    pCursor;
    int		    x;
    int		    y;
    BoxRec	    saved;
    Bool	    isUp;
    Bool	    shouldBeUp;
    WindowPtr	    pCacheWin;
    Bool	    isInCacheWin;
    Bool	    checkPixels;
    xColorItem	    colors[2];
    ColormapPtr	    pInstalledMap;
    ColormapPtr	    pColormap;
    VisualPtr	    pVisual;
    gsxSpriteCursorFuncPtr    funcs;
#ifdef MITSHM
    ShmFuncs	    shmFuncs;
    ShmFuncsPtr     orgShmFuncsPtr;
#endif
} gsxSpriteScreenRec, *gsxSpriteScreenPtr;

extern void
gsxBuildCursorCT(
        ScreenPtr pScreen,
        unsigned char * pSourceBits,
        unsigned char * pMaskBits,
        short    sourceBitsWidth,
        short    sourceBitsHeight,
        int       width,
        int       height,
        unsigned  long fgPix,
        unsigned  long bgPix
);


extern void gsxPutCursorBitsCT(
	ScreenPtr pScreen,
	INT16	screenX,
	INT16	screenY,
	INT16	width,
	INT16	height
);

extern void gsxMoveDCAreaCT(
	ScreenPtr pScreen,
	INT16	srcX,
	INT16	srcY,
	INT16	width,
	INT16	height,
	INT16	dstX,
	INT16	dstY
);

extern void gsxRestoreDCAreaCT(
	ScreenPtr pScreen,
	INT16	srcX,
	INT16	srcY,
	INT16	width,
	INT16	height,
	INT16	screenX,
	INT16	screenY
);

extern void gsxSaveDCAreaCT(
	ScreenPtr pScreen,
	INT16	screenX,
	INT16	screenY,
	INT16	width,
	INT16	height,
	INT16	dstX,
	INT16	dstY
);

#endif /* GSX_ACCL_SPRITE_DC */

#endif /* GSX_H */
