/*

Copyright 1994, 2001 Sony Corporation.

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

Except as contained in this notice, the name of the Sony Corporation shall not
be used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the Sony Corporation.

*/


#include "X.h"

#include "misc.h"
#include "fontstruct.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "gsx.h"

#if GSX_CACHED_TILE



#define GSWORD_SIZE	4 /* 32bit */


#ifdef DEBUG

void
dump_line(unsigned char *p, int w, int psm)
{
    int i;
    unsigned int val;
    int sz = gsosBytePerPixel(psm);

    for (i=0; i< w; i++) {
	if (sz==2) {
		ErrorF("%2.2x%2.2x:", p[0], p[1]);
	} else {
		ErrorF("%2.2x%2.2x", p[0], p[1]);
		ErrorF("%2.2x%2.2x:", p[2], p[3]);
	}
	p += sz;
    }
}

void
dump_tex( int bp, int bw, int psm, int w, int h)
{
    static unsigned int b[1024];
    int j;

    ErrorF("dump_tex: bp:0x%x tbw:0x%x psm:0x%x w:%d h:%d\n", 
    	bp, bw, psm, w, h);
    for(j=0;j<h;j++) {
        int sz;
        int i;
        unsigned int val;
        char *p;

	p = (char *) b;
        gsosReadImage(0, j, w, 1, bp, bw, psm, p);
        dump_line(p, w, psm);
        ErrorF("\n");
    }
}

#endif /* DEBUG */


/* Get cell size in 64 GSWORD */
static inline
int getCTCellSize(int byte_per_pix, int tile_w, int tile_h)
{
    int sz;
    sz = (tile_w +63)/64 * 64 * tile_h * byte_per_pix;
    						// unit: byte
    sz /= GSWORD_SIZE;				// unit: GSWORD
    sz = (sz +63)/64*64;			// aligned to 64 GSWORD
    sz /= 64;					// unit: 64 GSWORD
    return sz;
}

/* Get num of cache-able cells and fbp points to the first cell */
static
int
gsxGetCTArea(ScreenPtr pScreen,
        int *cache_fbp,
    	int tile_w, int tile_h)
{

    int	gsbuffer_size;
    int	fbsize;
    int	fb_height;
    int	fbw;
    int	fbp;
    int	tbp;
    gsxScreenPtr pScreenPriv;
    int byte_per_pix;
    int page_height;

    pScreenPriv = gsxGetScreenPriv(pScreen);
    fb_height = pScreenPriv->fb_height;
    fbp = pScreenPriv->fbp;
    fbw = pScreenPriv->fbw;
    byte_per_pix =  gsxGetBytePerPixel(pScreenPriv);

    page_height = 8192/(64 * byte_per_pix);
    fb_height = (fb_height + page_height - 1)/page_height;
    fbsize = (fbw * 8192  * fb_height)/GSWORD_SIZE; 
    					// unit: GSWORD
    tbp = fbp*2048 + fbsize;		// unit: GSWORD
    tbp = (tbp+63)/64;			// unit: 64 GSWORD

    *cache_fbp = tbp;
    gsbuffer_size = gsxGetScreenPriv(pScreen)->gsbuffer_size;
    return (gsbuffer_size/64/GSWORD_SIZE - tbp)/2
    		/getCTCellSize(byte_per_pix, tile_w, tile_h);
}

static inline
void initCahedTileTextureInfo (
	ScreenPtr pScreen, 
	int base_tbp,
	gsxTileTexture *texinfop, 
	int tile_w, int tile_h, int i)
{

    int sz;
    gsxScreenPtr pScreenPriv;

    pScreenPriv = gsxGetScreenPriv(pScreen);
    /* Get tile ROOM size */

    sz = getCTCellSize(
    	    gsxGetBytePerPixel(gsxGetScreenPriv(pScreen)), 
    				tile_w, tile_h);

    /* Get Texture Base Pointer and Buffer Width */
    texinfop->tbp = base_tbp + sz * i;			// unit: 64 GSWORD
    texinfop->tbw = (tile_w + 63)/64;			// unit: 64 pixel

    texinfop->fake_w = 0;
    texinfop->fake_h = 0;
    texinfop->psm = pScreenPriv->psm;
}



static 
void copyTileToCache(gsxCachedTilePtr pCT, PixmapPtr pTile)
{
    int width, height, depth;
    int i;

    width = pTile->drawable.width;
    height = pTile->drawable.height;
    depth = pTile->drawable.depth;

#ifdef DEBUG
    if (depth != 16 && depth != 24) {
      FatalError("copyTileToCache: bad depth:%d\n", depth);
    }
#endif

    if (width*gsosBytePerPixel(pCT->texinfo.psm) == pTile->devKind) {
        gsosWriteImage(0, 0, width, height, 
              pCT->texinfo.tbp,
	      pCT->texinfo.tbw,
	      pCT->texinfo.psm,
	      (unsigned char *) pTile->devPrivate.ptr);
    } else {
        for(i=0; i<height; i++) {
          unsigned char * srcbp;
          srcbp =  ((unsigned char *) pTile->devPrivate.ptr)
      			+ pTile->devKind * i;
          gsosWriteImage(0, i, width, 1, 
              pCT->texinfo.tbp,
	      pCT->texinfo.tbw,
	      pCT->texinfo.psm,
	      srcbp);
      }
    }

    pCT->texinfo.log2tw = gsxGetLog2GE(width);
    pCT->texinfo.log2th = gsxGetLog2GE(height);

    pCT->texinfo.fake_w = (width & (width - 1)) ? width : 0;
    pCT->texinfo.fake_h = (height & (height - 1)) ? height : 0;

    //   expand texture pattern, if not aligned to log2.
    if (pCT->texinfo.fake_w) {
        unsigned int sr;
	unsigned int w = pCT->texinfo.fake_w;
	unsigned int h = pCT->texinfo.fake_h;
	unsigned int tbp = pCT->texinfo.tbp;
	unsigned int tbw = pCT->texinfo.tbw;
	unsigned int psm = pCT->texinfo.psm;
	unsigned int src_x, src_y;
	unsigned int dst_x, dst_y;

	for (; (pCT->texinfo.fake_w + w) < TILE_CACHE_WIDTH;
			pCT->texinfo.fake_w += w) {
          
	   // copy (0,0) -> (pCT->texinfo.fake_w,0) size (w, h)
	   src_x=src_y=0;
	   dst_x=pCT->texinfo.fake_w;
	   dst_y=0;

           gsosMakeGiftag(4,
		     GSOS_GIF_PRE_IGNORE, 0, GSOS_GIF_FLG_PACKED, 1,
		     GSOS_GIF_REG_AD);

           sr = tbw | psm << (24-16);
           gsosSetPacketAddrData4(GSOS_BITBLTBUF, tbp, sr, tbp, sr);
           sr = dst_y;
           if (src_y  > dst_y || 
             (src_y  == dst_y && src_x > dst_x) )
                ;
           else
             sr |= (3 << (59-48));
           gsosSetPacketAddrData4(GSOS_TRXPOS, src_x, src_y, dst_x, sr );
           gsosSetPacketAddrData4(GSOS_TRXREG, w, 0, h, 0);
           gsosSetPacketAddrData(GSOS_TRXDIR, 2);
           gsosExec();

	}
        pCT->texinfo.log2tw = gsxGetLog2GE(pCT->texinfo.fake_w);
    }

    //   expand texture pattern, if not aligned to log2.
    if (pCT->texinfo.fake_h) {
        unsigned int sr;
	unsigned int w = pCT->texinfo.fake_w;
	unsigned int h = pCT->texinfo.fake_h;
	unsigned int tbp = pCT->texinfo.tbp;
	unsigned int tbw = pCT->texinfo.tbw;
	unsigned int psm = pCT->texinfo.psm;
	unsigned int src_x, src_y;
	unsigned int dst_x, dst_y;

	for (; (pCT->texinfo.fake_h + h) < TILE_CACHE_HEIGHT;
			pCT->texinfo.fake_h += h) {

	   // copy (0,0) -> (0,pCT->texinfo.fake_h) size (w, h)

	   src_x=src_y=0;
	   dst_x=0;
	   dst_y=pCT->texinfo.fake_h;
	
           gsosMakeGiftag(4,
		     GSOS_GIF_PRE_IGNORE, 0, GSOS_GIF_FLG_PACKED, 1,
		     GSOS_GIF_REG_AD);

           sr = tbw | psm << (24-16);
           gsosSetPacketAddrData4(GSOS_BITBLTBUF, tbp, sr, tbp, sr);
           sr = dst_y;
           if (src_y  > dst_y || 
             (src_y  == dst_y && src_x > dst_x) )
                ;
           else
             sr |= (3 << (59-48));
           gsosSetPacketAddrData4(GSOS_TRXPOS, src_x, src_y, dst_x, sr );
           gsosSetPacketAddrData4(GSOS_TRXREG, w, 0, h, 0);
           gsosSetPacketAddrData(GSOS_TRXDIR, 2);
           gsosExec();

	}
        pCT->texinfo.log2th = gsxGetLog2GE(pCT->texinfo.fake_h);
    }

#ifdef DEBUG
    ErrorF("*copyTileToCache: tw:%d th:%d\n", 
		pCT->texinfo.log2tw, pCT->texinfo.log2th);
    dump_tex( pCT->texinfo.tbp, pCT->texinfo.tbw, pCT->texinfo.psm,
                 width, height);
#endif

}


#define WORD_ALIGN(x) (((x)+ sizeof(CARD32)-1) & ~(sizeof(CARD32)-1))
void
gsxInitCT(ScreenPtr pScreen)
{
    int i;
    int n;
    gsxCachedTilePtr pCT;
    gsxScreenPtr pScreenPriv;
    int cache_bp;
    int byte_per_pix;
    unsigned char *data;
    unsigned int cell_size;

    pScreenPriv = gsxGetScreenPriv(pScreen);
    pScreenPriv->reservedCTs = 0;
    n = gsxGetCTArea(pScreen, &cache_bp, TILE_CACHE_WIDTH, TILE_CACHE_HEIGHT);
    byte_per_pix =  gsxGetBytePerPixel(pScreenPriv);
    
    cell_size = WORD_ALIGN(byte_per_pix*TILE_CACHE_WIDTH)*TILE_CACHE_HEIGHT;
    data = (unsigned char *)xalloc(n*cell_size);
    pCT = (gsxCachedTile *)xalloc(n * sizeof(gsxCachedTile));

    if (!data || !pCT)
    	FatalError("gsxInitCT: can't allocate cached tile\n");

    for (i=0; i<n; i++) {
	pCT[i].data = (unsigned int *) &data[i*cell_size];
        pCT[i].refcnt = -1;
	initCahedTileTextureInfo (pScreen, cache_bp, &(pCT[i].texinfo), 
			TILE_CACHE_WIDTH, TILE_CACHE_HEIGHT, i);
    }
    pScreenPriv->cachedTiles = pCT;
    pScreenPriv->cachedTiles_data = data;

#if GSX_ACCL_DASH
    pScreenPriv->dashCT = pCT;
    pCT->refcnt = -1;
    pCT++;
    pScreenPriv->reservedCTs ++;
    pScreenPriv->lastNumInDashList = -1;
#endif

#if GSX_ACCL_SPRITE_DC
    pScreenPriv->saveDCArea = pCT;
    pCT->refcnt = -1;
    pCT++;
    pScreenPriv->reservedCTs ++;

    pScreenPriv->cursorCT = pCT;
    pCT->refcnt = -1;
    pCT++;
    pScreenPriv->reservedCTs ++;
#endif

    pScreenPriv->freedTiles = pCT;
    pScreenPriv->usedTiles = NULL;
    for (i=pScreenPriv->reservedCTs; i<n; i++) {
        pCT->prev = pCT - 1;
        pCT->next = pCT + 1;
        pCT++;
    }
    pScreenPriv->cachedTiles[pScreenPriv->reservedCTs].prev = NULL;
    pScreenPriv->cachedTiles[n-1].next = NULL;

    ErrorF("gsxtile.c: allocated cached tiles: %d+%d (%d Kbytes)\n", 
			n-pScreenPriv->reservedCTs, 
    			pScreenPriv->reservedCTs, 
			cell_size*n/1024);
}

void
gsxCloseCT(ScreenPtr pScreen)
{
    gsxScreenPtr pScreenPriv;

    pScreenPriv = gsxGetScreenPriv(pScreen);
    if(pScreenPriv->cachedTiles_data) {
    	xfree(pScreenPriv->cachedTiles_data);
	pScreenPriv->cachedTiles_data = NULL;
    }
    if(pScreenPriv->cachedTiles) {
    	xfree(pScreenPriv->cachedTiles);
	pScreenPriv->cachedTiles=NULL;
        pScreenPriv->freedTiles = NULL;
        pScreenPriv->usedTiles = NULL;
#if GSX_ACCL_DASH
        pScreenPriv->dashCT = NULL;
#endif
    }
}


inline
static gsxCachedTilePtr
gsxFindCT(gsxScreenPtr pScreenPriv, int fillStyle, PixmapPtr pTile, 
    Pixel fg, Pixel bg)
{
    int		width, height, depth;
    gsxCachedTilePtr	pCT, pPrev;

    pCT = NULL;
    width = pTile->drawable.width;
    height = pTile->drawable.height;
    depth = pTile->drawable.depth;

    if (width <= TILE_CACHE_WIDTH && height <= TILE_CACHE_HEIGHT) {
	/* Search same pattern in used-list, first */
	pCT = pScreenPriv->usedTiles;
	while (pCT) {
	    if ((pCT->width == width) &&
	        (pCT->height == height) &&
	        (pCT->depth == depth) &&
	        (pCT->fillStyle == fillStyle) &&
	        ((pCT->fillStyle == FillTiled) ? TRUE: 
		    (pCT->fg_pix == fg &&  pCT->bg_pix == bg)) &&
	        *pCT->data == *((unsigned int *)pTile->devPrivate.ptr) &&
		// XXX: Can we use sigle bcmp()?
	        !bcmp(pTile->devPrivate.ptr, pCT->data,
		      pTile->devKind * height)) {
		return (pCT);
	    }
	    pCT = pCT->next;
	}
	pCT = pScreenPriv->freedTiles;
	if (pCT) {
	    pPrev = NULL;
	    /* Try to reclaim a cached tile from freed-list  */
	    while (pCT) {
		if ((pCT->refcnt == 0) &&
		    (pCT->width == width) &&
		    (pCT->height == height) &&
		    (pCT->depth == depth) &&
	            (pCT->fillStyle == fillStyle) &&
	            ((pCT->fillStyle == FillTiled) ? TRUE: 
		        (pCT->fg_pix == fg &&  pCT->bg_pix == bg)) &&
		    *pCT->data == *((unsigned int *)pTile->devPrivate.ptr) &&
		    // XXX: Can we use sigle bcmp()?
		    !bcmp(pTile->devPrivate.ptr, pCT->data,
			  pTile->devKind * height)) {
		    break;
		}
		pPrev = pCT;
		pCT = pCT->next;
	    }
	    if (!pCT) {
		if (!pPrev)
		   return NULL;
	    	/*  can't reclaim, use tail entry */
		pCT = pPrev;
		pCT->refcnt = -1;
	    }

	    /* Remove form freed-list */
	    if (pCT->prev) {
		pCT->prev->next = pCT->next;
	    } else {
		pScreenPriv->freedTiles = pCT->next;
	    }
	    if (pCT->next) {
		pCT->next->prev = pCT->prev;
	    }

	    /* Insert to head of used-list */
	    pCT->next = pScreenPriv->usedTiles;
	    if (pCT->next) {
		pCT->next->prev = pCT;
	    }
	    pScreenPriv->usedTiles = pCT;
	    pCT->prev = NULL;
	}
    }
    return (pCT);
}

#if BITMAP_BIT_ORDER == LSBFirst
#define bit_test(bits)	((bits)&1)
#define bit_next(bits)	((bits)>>1)
#else
#define bit_test(bits)	((bits)<0)
#define bit_next(bits)	((bits)<<1)
#endif

static
void
copyStippleToCache(gsxCachedTilePtr pCT, PixmapPtr pTile, PixmapPtr tmpPix)
{
    int		width = tmpPix->drawable.width;
    int		height = tmpPix->drawable.height;
    int		h;
    CARD32	pix_0 = pCT->bg_pix;
    CARD32	pix_1 = pCT->fg_pix;
    pointer	tmpData = pCT->data;

    for (h = 0; h< height; h++) {
        INT32 bits;
        INT32 * bp32;
        CARD32 * pix32;
        CARD16 * pix16;
        int	remains;

        bp32 = (INT32 *) ((void *)pTile->devPrivate.ptr + 
    			h * pTile->devKind);
        pix16 = (CARD16 *)(((char *)tmpData)+h*tmpPix->devKind);
        pix32 = (CARD32 *)(((char *)tmpData)+h*tmpPix->devKind);

        for(remains = width; remains>0; remains-=32) {
            int k;

            bits = * (bp32++);
            for (k=0;k<min(remains,32);k++) {
                if (pCT->texinfo.psm == PS2_GS_PSMCT16) {
                    *(pix16++) = bit_test(bits)? pix_1 : pix_0;
                } else {
                    *(pix32++) = bit_test(bits)? pix_1 : pix_0;
                }
                bits = bit_next(bits);
            }
        }
    }
    // Copy tmpPix -> Texture
    copyTileToCache(pCT, tmpPix);
}

gsxCachedTilePtr
gsxGetCT(ScreenPtr pScreen, GCPtr  pGC, PixmapPtr pTile)
{
    gsxScreenPtr	pScreenPriv;
    gsxCachedTilePtr	pCT = NULL;
    int 		width;
    int 		height;
    int			fillStyle;
    int			depth;
    Pixel		fg_pix;
    Pixel		bg_pix;

    pScreenPriv = gsxGetScreenPriv(pScreen);
    width = pTile->drawable.width;
    height = pTile->drawable.height;
    depth = pScreenPriv->depth;

    if ((width > TILE_CACHE_WIDTH) || (height> TILE_CACHE_HEIGHT)) {
        return 0;
    }

    if (pGC) 
        fillStyle = pGC->fillStyle;
    else
        fillStyle = FillTiled;

    if ( fillStyle != FillTiled ) {
        if  (pGC->fillStyle == FillStippled) {
            bg_pix = ~ pScreenPriv->rgb_pix_mask;
        } else {
            bg_pix = pGC->bgPixel & pScreenPriv->rgb_pix_mask;
        }
        fg_pix =  pGC->fgPixel & pScreenPriv->rgb_pix_mask;
    } else {
    	bg_pix = fg_pix = 0;
    }

    pCT = gsxFindCT(pScreenPriv, fillStyle, pTile, fg_pix, bg_pix);
    if (pCT) {
	if (pCT->refcnt < 0) {
#ifdef DEBUG
            ErrorF("gsxGetCT:0x%8.8x pCT->refcnt:%d\n", pCT, pCT->refcnt);
#endif
	    // pCT->fg and pCT->bg must be set before copyTileToCache().
            pCT->fg_pix = fg_pix;
            pCT->bg_pix = bg_pix;

	    if (fillStyle != FillTiled) {
		PixmapPtr	tmpPix;

		if (pTile->drawable.depth != 1) {
		    ErrorF("gsxGetCT: strange fillStyle:%d and tile depth:%d\n",
			fillStyle, pTile->drawable.depth);
			return NULL;
		}
                tmpPix = GetScratchPixmapHeader(pScreen, width, height, depth,
                  	BitsPerPixel(depth), PixmapBytePad(width, depth),
			pCT->data);
                if( !tmpPix ){
                    return NULL;
                }
	        // Convert & Copy pTile -> Texture
	        copyStippleToCache(pCT, pTile, tmpPix);
                FreeScratchPixmapHeader(tmpPix);
	    } else {
		if (pTile->drawable.depth != depth) {
		    ErrorF("gsxGetCT: strange fillStyle:%d and tile depth:%d\n",
			fillStyle, pTile->drawable.depth);
			return NULL;
		}
	        // Copy pTile -> Texture
	        copyTileToCache(pCT, pTile);

		// XXX: Can we use single bcopy?
		bcopy (pTile->devPrivate.ptr, pCT->data,
		      pTile->devKind * height );
	    }

	    pCT->refcnt = 0;
	    pCT->fillStyle = fillStyle;
            pCT->height = height;
            pCT->width = width;
            pCT->depth = depth;
	}
	pCT->refcnt++;
    }
    return (pCT);
}

void
gsxDestroyCT(ScreenPtr pScreen, gsxCachedTilePtr pCT)
{
    gsxScreenPtr	pScreenPriv;

    if (pCT->refcnt <= 0) {
	ErrorF("gsxDestroyCT: Can't destory 0x%x.\n", pCT);
        return;
    }

    pCT->refcnt --;
    if (pCT->refcnt == 0) {
	pScreenPriv = gsxGetScreenPriv(pScreen);
	if (pCT->prev) {
	    pCT->prev->next = pCT->next;
	} else {
	    pScreenPriv->usedTiles = pCT->next;
	}
	if (pCT->next) {
	    pCT->next->prev = pCT->prev;
	}
	pCT->next = pScreenPriv->freedTiles;
	if (pCT->next){
	    pCT->next->prev = pCT;
	}
	pScreenPriv->freedTiles = pCT;
	pCT->prev = NULL;
    }
}

Bool
gsxReloadTileAndStippleCT(ScreenPtr pScreen)
{
    gsxCachedTilePtr	pCT;
    gsxScreenPtr	pScreenPriv;
    int		height;
    int		width;
    int		depth;
    PixmapPtr	tmpPix;

    pScreenPriv = gsxGetScreenPriv(pScreen);

    for (pCT =pScreenPriv->freedTiles; pCT; pCT=pCT->next) {
    	pCT->refcnt = -1;
    }

    for (pCT =pScreenPriv->usedTiles; pCT; pCT=pCT->next) {

        depth = pCT->depth;
        height = pCT->height;
        width = pCT->width;

	tmpPix = GetScratchPixmapHeader(pScreen, width, height, depth,
		    BitsPerPixel(depth), PixmapBytePad(width, depth),
		    pCT->data);
	if (!tmpPix ) {
	    return FALSE;
	}
	// Copy tmpPix -> Texture
	copyTileToCache(pCT, tmpPix);
	FreeScratchPixmapHeader(tmpPix);
    }

    return TRUE;
}

#if GSX_ACCL_DASH

Bool
gsxCheckDash(pGC, do_register)
    GCPtr       pGC;
    int		do_register;
{
    int i,j;
    int len;
    PixmapPtr  tmpDash;
    gsxScreenPtr pScreenPriv;
    CARD16 *u16p;
    CARD32 *u32p;
    CARD32 cpix[2];
    Pixel	fg_pix;
    Pixel	bg_pix;

    len = 0;
    for(i=0; i<pGC->numInDashList; i++) {
    	len += pGC->dash[i];
    }
    if (len >TILE_CACHE_WIDTH || len <=0)
    	return FALSE;
    if (len !=  (1 << gsxGetLog2GE(len)))
    	return FALSE;

   if (!do_register) 
        return TRUE;

    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);

    fg_pix = pGC->fgPixel & pScreenPriv->rgb_pix_mask;
    if  (pGC->lineStyle == LineDoubleDash) {
        bg_pix = pGC->bgPixel & pScreenPriv->rgb_pix_mask;
    } else {
        bg_pix = ~ pScreenPriv->rgb_pix_mask;
    }

    /// Check register needed.
    if (pScreenPriv->dashLen == len &&
    	pScreenPriv->lastDashStyle == pGC->lineStyle &&
	fg_pix == pScreenPriv->dashCT->fg_pix &&
	bg_pix == pScreenPriv->dashCT->bg_pix &&
    	! memcmp(pScreenPriv->lastDash, pGC->dash, len)) {
        return TRUE;
    }

    tmpDash = GetScratchPixmapHeader(pGC->pScreen, len, 1, pGC->depth,
                  BitsPerPixel(pGC->depth), PixmapBytePad(len, pGC->depth),
                  (pointer)pScreenPriv->dashCT->data);

    if( !tmpDash ){
        return FALSE;
    }

    u32p = (unsigned long *) tmpDash->devPrivate.ptr;
    u16p = (unsigned short *) tmpDash->devPrivate.ptr;

    cpix[0] = fg_pix;
    cpix[1] = bg_pix;

    for(i=0; i<pGC->numInDashList; i++) {
    	for (j=0; j<pGC->dash[i]; j++) {
	   if (pScreenPriv->dashCT->texinfo.psm  == PS2_GS_PSMCT16) {
	        *(u16p++) = cpix[i&1];
	   } else {
	        *(u32p++) = cpix[i&1];
	   }
	}
    }

#ifdef DEBUG
    ErrorF("dashLen:%d %d\n", len, (1 << gsxGetLog2GE(len)));
    for(i=0; i<pGC->numInDashList; i++) {
	ErrorF("%d ", pGC->dash[i]);
    }
    ErrorF("\n");
#endif

    // Copy pTile -> Texture
    copyTileToCache(pScreenPriv->dashCT, tmpDash);
    FreeScratchPixmapHeader(tmpDash);

    pScreenPriv->lastNumInDashList = pGC->numInDashList;
    pScreenPriv->lastDashStyle = pGC->lineStyle;
    memcpy(pScreenPriv->lastDash, pGC->dash, pGC->numInDashList);
    pScreenPriv->dashLen  = len;
    pScreenPriv->dashCT->refcnt = 1;
    pScreenPriv->dashCT->fg_pix = fg_pix;
    pScreenPriv->dashCT->bg_pix = bg_pix;

    return TRUE;
}


Bool
gsxReloadDashCT(ScreenPtr pScreen)
{
    PixmapPtr  tmpDash;
    gsxScreenPtr pScreenPriv;
    int depth;
    int len;

    pScreenPriv = gsxGetScreenPriv(pScreen);
    if (pScreenPriv->dashCT->refcnt<=0) 
    	return TRUE;

    len = pScreenPriv->dashLen;
    depth = pScreenPriv->depth;
    tmpDash = GetScratchPixmapHeader(pScreen, len, 1, depth,
                  BitsPerPixel(depth), PixmapBytePad(len, depth),
                  (pointer)pScreenPriv->dashCT->data);

    if( !tmpDash ){
        return FALSE;
    }
    copyTileToCache(pScreenPriv->dashCT, tmpDash);
    FreeScratchPixmapHeader(tmpDash);
    return TRUE;
}

#endif /* GSX_ACCL_DASH */


#if GSX_ACCL_SPRITE_DC

void
gsxBuildCursorCT(
	ScreenPtr pScreen,
	unsigned char * pSourceBits,
	unsigned char * pMaskBits,
	short	 sourceBitsWidth,
	short	 sourceBitsHeight,
	int	  width,
	int	  height,
	unsigned  long fgPix,
	unsigned  long bgPix
)
{
    PixmapPtr		tmpPix;
    gsxScreenPtr	pScreenPriv;
    int			depth;
    gsxCachedTilePtr	pCT;
    CARD32		alpha;
    int			h;

    pScreenPriv = gsxGetScreenPriv(pScreen);
    pCT = pScreenPriv->cursorCT;
    depth = pScreenPriv->depth;
    tmpPix = GetScratchPixmapHeader(pScreen, width, height, depth,
               	BitsPerPixel(depth), PixmapBytePad(width, depth),
		pCT->data);
    if (!tmpPix)
    	return;

    alpha = ~ pScreenPriv->rgb_pix_mask;

    for (h = 0; h< height; h++) {
        INT32 source_bits;
        INT32 mask_bits;
        INT32 * source_bp32;
        INT32 * mask_bp32;
        CARD32 * pix32;
        CARD16 * pix16;
        int	remains;

        source_bp32 = (INT32 *) ((void *)pSourceBits + 
    			h * PixmapBytePad(sourceBitsWidth, 1));
        mask_bp32 = (INT32 *) ((void *)pMaskBits + 
    			h * PixmapBytePad(sourceBitsHeight, 1));
        pix16 = (CARD16 *)(((char *)pCT->data)+h*tmpPix->devKind);
        pix32 = (CARD32 *)(((char *)pCT->data)+h*tmpPix->devKind);

        for(remains = width; remains>0; remains-=32) {
            int k;

            source_bits = * (source_bp32++);
            mask_bits = * (mask_bp32++);
            for (k=0;k<min(remains,32);k++) {
                if (pCT->texinfo.psm == PS2_GS_PSMCT16) {
                    *(pix16++) = (bit_test(mask_bits) ?
		    	(bit_test(source_bits)? fgPix : bgPix) :
			(CARD16) alpha);

                } else {
                    *(pix32++) = (bit_test(mask_bits) ?
		    	(bit_test(source_bits)? fgPix : bgPix) :
			(CARD32) alpha);
                }
                source_bits = bit_next(source_bits);
                mask_bits = bit_next(mask_bits);
            }
        }
    }
    copyTileToCache(pCT, tmpPix);
    FreeScratchPixmapHeader(tmpPix);
    pCT->refcnt=1;
    pCT->height = height;
    pCT->width = width;
    pCT->depth = depth;
}

Bool
gsxReloadCursorCT(ScreenPtr pScreen)
{
    gsxCachedTilePtr	pCT;
    gsxScreenPtr	pScreenPriv;
    int		height;
    int		width;
    int		depth;
    PixmapPtr	tmpPix;

    pScreenPriv = gsxGetScreenPriv(pScreen);
    pCT = pScreenPriv->cursorCT;
    depth = pCT->depth;
    height = pCT->height;
    width = pCT->width;

    tmpPix = GetScratchPixmapHeader(pScreen, width, height, depth,
		BitsPerPixel(depth), PixmapBytePad(width, depth),
		pCT->data);
    if (!tmpPix)
	return FALSE;

    copyTileToCache(pCT, tmpPix);
    FreeScratchPixmapHeader(tmpPix);
    pCT->refcnt=1;
    return TRUE;
}

void
gsxPutCursorBitsCT(
	ScreenPtr pScreen,
	INT16	screenX,
	INT16	screenY,
	INT16	width,
	INT16	height
)
{
    GSOSbit64   tex, test_1;
    gsxScreenPtr pScreenPriv;
    gsxCachedTilePtr	pCT;

    pScreenPriv = gsxGetScreenPriv(pScreen);
    pCT = pScreenPriv->cursorCT;

    if ((pScreenPriv)->cur_fbmask)
        gsosFrameInit((pScreenPriv)->fbw,
            (pScreenPriv)->psm,
            (pScreenPriv)->fbp,
            0 /* enable all planes */);

    // Setup
    gsosMakeGiftag(5,
        GSOS_GIF_PRE_IGNORE, 0, 
        GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);

    /* set SCISSOR rect */
    gsosSetPacketAddrData4(GSOS_SCISSOR_1,
              (GSOSbit64)0, (GSOSbit64)pScreen->width-1,
              (GSOSbit64)0, (GSOSbit64)pScreen->height-1);
    /* set texture */
    gsosSetPacketAddrData(GSOS_CACHEINVLD, 0) ;
    tex = GsosTex0Data( pCT->texinfo.tbp /*tbp0*/,
                        pCT->texinfo.tbw /*tbw*/,
                        pCT->texinfo.psm /*psm*/,
                        pCT->texinfo.log2tw /*tw*/,
                        pCT->texinfo.log2th /*th*/,
                        1 /*tcc: RGBA */,
                        1 /*tfx: DECAL */,
                        0 /*cbp*/,
                        0 /*cpsm*/,
                        0 /*csm*/,
                        0 /*csa*/,
                        0 /*cld*/);
    gsosSetPacketAddrData(GSOS_TEX0_1, tex) ;
    /* enable pmode on prim */
    gsosSetPacketAddrData(GSOS_PRMODECONT, 1) ;
    /* set alpha test */
    test_1 = GsosTestData( 1,   /*ATE:  ON*/
                        4,      /*ATST: EQUAL*/
                        0,      /*AREF: 0*/
                        0,      /*AFAIL: KEEP*/
                        0,      /*DATE: OFF*/
                        0,      /*DATM: 0*/
                        0,      /*ZTE: OFF*/
                        0       /*ZTST:NEVER*/
                        ) ;
    gsosSetPacketAddrData(GSOS_TEST_1, test_1 );

    // Draw Rect
    gsosMakeGiftag(2,
              GSOS_GIF_PRE_ENABLE, 
                (GSOS_PRIM_SPRITE|GSOS_GIF_PRI_TME|GSOS_GIF_PRI_FST),
              GSOS_GIF_FLG_PACKED, 2,
                      GSOS_GIF_REG_XYZ2<<4|GSOS_GIF_REG_UV);
    /* UV */
    gsosSetXyPackedXYZ2(GSOS_SUBTEX_OFST(0),
                GSOS_SUBTEX_OFST(0));
    /* XY */
    gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(screenX),
               GSOS_SUBPIX_OFST(screenY));
    /* UV */
    gsosSetXyPackedXYZ2(GSOS_SUBTEX_OFST(width),
                GSOS_SUBTEX_OFST(height));
    /* XY */
    gsosSetXyPackedXYZ2(GSOS_SUBPIX_OFST(screenX+width),
               GSOS_SUBPIX_OFST(screenY+height));


    // Clear
    gsosMakeGiftag(3,
        GSOS_GIF_PRE_IGNORE, 0, 
        GSOS_GIF_FLG_PACKED, 1, GSOS_GIF_REG_AD);
    /* disable pmode on prim */
    gsosSetPacketAddrData(GSOS_PRMODECONT, 0) ;
    /* clear pmode */
    gsosSetPacketAddrData(GSOS_PRMODE, 0) ;
    /* clear alpha test */
    test_1 = GsosTestData( 1, 1, 0, 0, 0, 0, 0, 0 ) ;
    gsosSetPacketAddrData(GSOS_TEST_1, test_1 );

    // restore fbmask
    if ((pScreenPriv)->cur_fbmask)
        gsosFrameInit((pScreenPriv)->fbw,
            (pScreenPriv)->psm,
            (pScreenPriv)->fbp,
            (pScreenPriv)->cur_fbmask);
}

void
gsxMoveDCAreaCT(
	ScreenPtr pScreen,
	INT16	srcX,
	INT16	srcY,
	INT16	width,
	INT16	height,
	INT16	dstX,
	INT16	dstY
) 
{
    unsigned int sr0, sr1;
    unsigned int psm;
    unsigned int dbp, dbw;
    unsigned int sbp, sbw;
    gsxCachedTilePtr	pCT;
    gsxScreenPtr pScreenPriv;

    if (width<=0 || height<=0) { 
#ifdef DEBUG
        ErrorF(" ....gsxMoveDCAreaCT: return\n");
#endif
	return;
    }

    pScreenPriv = gsxGetScreenPriv(pScreen);

    pCT = pScreenPriv->saveDCArea;

    psm = pCT->texinfo.psm;
    dbp = sbp = pCT->texinfo.tbp;
    dbw = sbw = pCT->texinfo.tbw;

    gsosMakeGiftag(4, GSOS_GIF_PRE_IGNORE, 0, GSOS_GIF_FLG_PACKED, 1,
			                        GSOS_GIF_REG_AD);

    sr0 = sbw | (psm << (24-16));
    sr1 = dbw | (psm << (24-16));
    gsosSetPacketAddrData4(GSOS_BITBLTBUF, sbp, sr0, dbp, sr1);

    sr0 = dstY;
    if ( srcY  > dstY ||
       (srcY  == dstY && srcX > dstX ) )
       ;
    else
       sr0 |= (3 << (59-48));
    gsosSetPacketAddrData4(GSOS_TRXPOS,
            srcX, srcY, dstX, sr0 );

    gsosSetPacketAddrData4(GSOS_TRXREG, width, 0, height, 0);

    gsosSetPacketAddrData(GSOS_TRXDIR, 2);

}

void
gsxRestoreDCAreaCT(
	ScreenPtr pScreen,
	INT16	srcX,
	INT16	srcY,
	INT16	width,
	INT16	height,
	INT16	screenX,
	INT16	screenY
) 
{
    
    unsigned int sr0, sr1;
    unsigned int psm;
    unsigned int dbp, dbw;
    unsigned int sbp, sbw;
    gsxCachedTilePtr	pCT;
    gsxScreenPtr pScreenPriv;

    pScreenPriv = gsxGetScreenPriv(pScreen);

    if (screenX <0) {
    	srcX -= screenX;
	width += screenX;
	screenX = 0;
    }
    else if ((screenX+width) >= pScreen->width) {
    	width = - screenX + pScreen->width;
    }
    if (screenY <0) {
    	srcY -= screenY;
	height += screenY;
	screenY = 0;
    }
    else if ((screenY+height) >= pScreen->height) {
    	height = - screenY + pScreen->height;
    }

    if (width<=0 || height<=0) { 
#ifdef DEBUG
        ErrorF(" ....gsxRestoreDCAreaCT: return\n");
#endif
	return;
    }


    pCT = pScreenPriv->saveDCArea;
    psm = pScreenPriv->psm;

    dbp = pScreenPriv->fbp;
    dbw = pScreenPriv->fbw;

    sbp = pCT->texinfo.tbp;
    sbw = pCT->texinfo.tbw;

    gsosMakeGiftag(4, GSOS_GIF_PRE_IGNORE, 0, GSOS_GIF_FLG_PACKED, 1,
			                        GSOS_GIF_REG_AD);
    sr0 = sbw | (psm << (24-16));
    sr1 = dbw | (psm << (24-16));

    gsosSetPacketAddrData4(GSOS_BITBLTBUF, sbp, sr0, dbp, sr1);

    gsosSetPacketAddrData4(GSOS_TRXPOS, srcX, srcY, screenX, screenY );

    gsosSetPacketAddrData4(GSOS_TRXREG, width, 0, height, 0);

    gsosSetPacketAddrData(GSOS_TRXDIR, 2);

}

void
gsxSaveDCAreaCT(
	ScreenPtr pScreen,
	INT16	screenX,
	INT16	screenY,
	INT16	width,
	INT16	height,
	INT16	dstX,
	INT16	dstY
) 
{
    
    unsigned int sr0, sr1;
    unsigned int psm;
    unsigned int dbp, dbw;
    unsigned int sbp, sbw;
    gsxCachedTilePtr	pCT;
    gsxScreenPtr pScreenPriv;

    if (width<=0 || height<=0) { 
#ifdef DEBUG
        ErrorF(" ....gsxSaveDCAreaCT: return\n");
#endif
	return;	
    }

    pScreenPriv = gsxGetScreenPriv(pScreen);

    pCT = pScreenPriv->saveDCArea;
    psm = pScreenPriv->psm;

    sbp = pScreenPriv->fbp;
    sbw = pScreenPriv->fbw;

    dbp = pCT->texinfo.tbp;
    dbw = pCT->texinfo.tbw;

    gsosMakeGiftag(4, GSOS_GIF_PRE_IGNORE, 0, GSOS_GIF_FLG_PACKED, 1,
			                        GSOS_GIF_REG_AD);
    sr0 = sbw | (psm << (24-16));
    sr1 = dbw | (psm << (24-16));

    gsosSetPacketAddrData4(GSOS_BITBLTBUF, sbp, sr0, dbp, sr1);

    gsosSetPacketAddrData4(GSOS_TRXPOS, screenX, screenY, dstX, dstY);

    gsosSetPacketAddrData4(GSOS_TRXREG, width, 0, height, 0);

    gsosSetPacketAddrData(GSOS_TRXDIR, 2);

}

#endif /* GSX_ACCL_SPRITE_DC */

#endif /* GSX_CACHED_TILE */
