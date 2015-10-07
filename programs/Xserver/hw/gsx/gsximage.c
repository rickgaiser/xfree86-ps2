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


#include "servermd.h"
#include "pixmap.h"
#include "mi.h"
#include "gsx.h"

void
gsxGetImage(src, x, y, w, h, format, planeMask, pBinImage)
    DrawablePtr     src;
    int         x, y, w, h;
    unsigned int    format;
    unsigned long   planeMask;
    char *pBinImage ;
{
    PixmapPtr aPix;
    unsigned int bp;
    int bw, psm;

    if (w == 0 || h == 0) return;

    if (src->type == DRAWABLE_PIXMAP) {
        switch (src->bitsPerPixel) {
        case 1:
            mfbGetImage(src,x,y,w,h,format,planeMask,pBinImage);
            break;
        case 8:
            cfbGetImage(src,x,y,w,h,format,planeMask,pBinImage);
            break;
        case 16:
            cfb16GetImage(src,x,y,w,h,format,planeMask,pBinImage);
            break;
        case 32:
            cfb32GetImage(src,x,y,w,h,format,planeMask,pBinImage);
            break;
        default:
            ErrorF("gsxGetImage:Unkown pixmap depth\n");
	    break;
        }
        return;
    } else {
	gsxScreenPtr pScreenPriv;

	pScreenPriv = gsxGetScreenPriv(src->pScreen);
        bp = pScreenPriv->fbp;
        bw = pScreenPriv->fbw;
        psm = pScreenPriv->psm;
	if ( gsxCanIgnorePlMask(src->depth, planeMask) && format == ZPixmap) {

	    aPix = GetScratchPixmapHeader(src->pScreen, w, h, src->depth,
		      BitsPerPixel(src->depth), PixmapBytePad(w, src->depth),
		      (pointer)pBinImage);

	    if (!aPix) return;
	    gsxReadImage(x + src->x, y + src->y, w, h, bp, bw, psm, aPix);
            FreeScratchPixmapHeader(aPix);
	    return;
	}
        /* get pixel data from FB */
        aPix = (*src->pScreen->CreatePixmap)
            (src->pScreen, w, h, src->depth ) ;
        if( aPix == NULL ){
            return ;
        }
	gsxReadImage(x + src->x, y + src->y, w, h, bp, bw, psm, aPix);

        switch (src->bitsPerPixel) {
        case 16:
            cfb16GetImage(&aPix->drawable,0,0,w,h,format,planeMask,pBinImage);
            break;
        case 32:
            cfb32GetImage(&aPix->drawable,0,0,w,h,format,planeMask,pBinImage);
            break;
        default:
            ErrorF("gsxGetImage:Unkown window depth\n");
            break;
        }
        (*src->pScreen->DestroyPixmap)( aPix ) ;
    }
}


void
gsxGetSpans(src, wMax, ppt, pwidth, nspans, pdstStart)
    DrawablePtr     src;
    int         wMax;
    DDXPointPtr     ppt;
    int         *pwidth;
    int         nspans;
    unsigned long   *pdstStart;
{
    DDXPointPtr pptLast = ppt + nspans;
    int     xorg = src->x, yorg = src->y;
    GSXuchar    *data;
    int     depth = src->depth;

    data = (GSXuchar*)pdstStart;
    while(ppt < pptLast) {
        (*src->pScreen->GetImage)
            (src, ppt->x-xorg, ppt->y-yorg, *pwidth, 1, 
            ZPixmap, 0xffffffff, data );
        data += PixmapBytePad(*pwidth,depth);
        ppt++;
        pwidth++;
    }
}


void gsxSetSpansSoft(dst, pGC, pSrc, pPoints, pWidths, nSpans, sorted)
    DrawablePtr dst;
    GCPtr   pGC;
    char *pSrc;
    DDXPointPtr pPoints;
    int     *pWidths;
    int     nSpans;
    int     sorted;
{
    gsxScreenPtr pScreenPriv;
    xRectangle aRect ;
    PixmapPtr dstLop ;
    int nn ;
    GCPtr pGC1;

    if(!nSpans) return ;


    /* get region for LOP */
    gsxCalcDrawRegionSpan( pPoints, pWidths, nSpans, &aRect ) ;

    dstLop = (*dst->pScreen->CreatePixmap)
        (dst->pScreen, aRect.width, aRect.height, dst->depth ) ;
    if( !dstLop ){
        return ;
    }
    /* copy destination */
    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    gsxReadImage(aRect.x, aRect.y, aRect.width, aRect.height,
	pScreenPriv->fbp, pScreenPriv->fbw, pScreenPriv->psm,
        dstLop);

    /* ValidateGC( destination = PIXMAP ) */
    pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
    CopyGC(pGC, pGC1, ~GCClipMask);
    GSXVALIDATEGC( (DrawablePtr)dstLop, pGC1 ) ;

    /* draw lines */
    for( nn = 0 ; nn < nSpans ; nn++ ) {
        pPoints[nn].x -= (aRect.x -dst->x) ;
        pPoints[nn].y -= (aRect.y -dst->y) ;
    }
    (*pGC1->ops->SetSpans)( (DrawablePtr)dstLop, pGC1, pSrc, pPoints, pWidths, nSpans, sorted ) ;

    FreeScratchGC(pGC1);

    /* ValidateGC( destination = WINDOW ) */
    GSXVALIDATEGC( dst, pGC ) ;

    /* post-proccess for LOP */
    cfbBitBlt( (DrawablePtr)dstLop, dst, pGC, 0, 0, aRect.width, aRect.height,
        aRect.x-dst->x, aRect.y-dst->y, gsxDoBitBlt, 0 ) ;

    (*dst->pScreen->DestroyPixmap)(dstLop) ;
}

static inline void doSetSpans(dst, pGC, pSrc, pPoints, pWidths, nSpans, sorted)
    DrawablePtr dst;
    GCPtr   pGC;
    char *pSrc;
    DDXPointPtr pPoints;
    int     *pWidths;
    int     nSpans;
    int     sorted;
{
    gsxGCPtr     pPriv = gsxGetGCPriv(pGC);
    BoxPtr       pbox  = REGION_RECTS(pPriv->pCompositeClip);
    BoxPtr       plast = pbox + REGION_NUM_RECTS(pPriv->pCompositeClip);
    DDXPointPtr  p = pPoints, pLast = pPoints + nSpans;
    int          i, ps, pw ;
    GSXuchar     *pDat, *pd ;
    gsxScreenPtr pScreenPriv;
    unsigned int fbp ;
    int          fbw, psm ;

    if (!nSpans) return;

    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    fbp = pScreenPriv->fbp;
    fbw = pScreenPriv->fbw;
    psm = pScreenPriv->psm;
    while (pbox < plast) {
        i = 0 ;
        pDat = pSrc ;
        p = pPoints ;
        while( p < pLast ){
            if( p->y >= pbox->y1 && p->y <= pbox->y2 ){
                ps = p->x ;
                pw = pWidths[i] ;
                pd = pDat ;
                if( p->x < pbox->x1 ){
                    ps = pbox->x1 ;
                    pw -= pbox->x1 - p->x ;
                    pd += PixmapBytePad( pbox->x1 - p->x, pGC->depth ) ;
                }
                if( p->x + pWidths[i] - 1 > pbox->x2 ){
                    pw -= p->x + pWidths[i] - pbox->x2 ;
                }
                if( pw > 0 ){
                    gsosWriteImage( ps, p->y, pw, 1, fbp, fbw, psm, pd ) ;
                }
            }
            pDat += PixmapBytePad( pWidths[i], pGC->depth ) ;
            i++;
            p++;
        }
        pbox++;
    }

}

void gsxSetSpansGxcpy(dst, pGC, pSrc, pPoints, pWidths, nSpans, sorted)
    DrawablePtr dst;
    GCPtr   pGC;
    char *pSrc;
    DDXPointPtr pPoints;
    int     *pWidths;
    int     nSpans;
    int     sorted;
{
    doSetSpans( dst, pGC, pSrc, pPoints, pWidths, nSpans, sorted ) ;
}


void gsxPutImage(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		depth, x, y, w, h;
    int		leftPad;
    int		format;
    char 	*pImage;
{
    PixmapPtr   pPixmap;

    if ((w == 0) || (h == 0))
	return;

/*#ifdef GSOS_SPR_IMAGEBUFFER
    if (format == ZPixmap) {
        if (pDraw->type == DRAWABLE_WINDOW && !gsxIsPlanemask(pGC)) {
            RegionPtr	clip;
            BoxRec		bbox;
            gsxGCPtr	pGCPriv = gsxGetGCPriv(pGC);

            clip = pGCPriv->pCompositeClip;
            bbox.x1=x+pDraw->x;
            bbox.y1=y+pDraw->y;
            bbox.x2=bbox.x1+w;
            bbox.y2=bbox.y1+h;

	    if (REGION_NUM_RECTS(clip) < 1)
	        return;

            if (RECT_IN_REGION(pGC->pScreen, clip, &bbox) == rgnIN) {
                char	*srcp;
                char	*dstp;
                int	size;
                int	nlines;
                int	maxlines;
                int	txfline;
                int	linebytewidth;
                int	i;
                struct ps2_image img;
                struct ps2_image *imgp = &img;
                gsxScreenPtr pScreenPriv;
                int bp;
                int bw;
                int psm;
                int bytepp;
	        int srcdevKind;
	        int dstdevKind;
	        int wqc;

                pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
                bp = pScreenPriv->fbp;
                bw = pScreenPriv->fbw;
                psm = pScreenPriv->psm;
	        bytepp = gsxGetBytePerPixel(pScreenPriv);

                gsosKick();

                srcdevKind = PixmapBytePad(w + leftPad, depth);
                dstdevKind = w * bytepp;

                imgp->ptr = ps2dbp_spr[ps2dbf_spr];
                imgp->fbp = bp;
                imgp->fbw = bw;
                imgp->psm = psm;

                srcp = pImage + leftPad * bytepp;
                linebytewidth = w * bytepp;
	        maxlines = gsos_maximage_size / linebytewidth;
                imgp->x = pDraw->x + x;
                imgp->w = w;

                nlines = h;
                while(nlines > 0) {
              	    txfline = (maxlines < nlines) ? maxlines : nlines;
                    size = txfline * linebytewidth;
                    imgp->y = pDraw->y + y;
                    imgp->h = txfline ;
                    y += txfline;
               	    nlines -= txfline;

                    dstp = imgp->ptr;
                    if(leftPad == 0 && srcdevKind == dstdevKind) {
                        memcpy((void *)dstp, (void *)srcp, size);
                        srcp += size;
                    } else {
                        for(i = 0; i < txfline; i++) {
                          memcpy((void *)dstp, (void *)srcp, dstdevKind);
                          dstp  += dstdevKind;
                          srcp += srcdevKind ;
		        }
                     }
#ifdef GSOS_LAZY_EXEC
                    wqc = 2;
#else
                    wqc = (nlines > 0) ? 2 : 1;
#endif
                    if(ioctl(ps2gsfd, PS2IOC_LOADIMAGEA, &img) < 0) {
                        ErrorF("gsxDoBitbltMW: PS2IOC_LOADIMAGEA Error\n");
                        return;
                    }
#ifdef GSOS_LAZY_EXEC
		    if(!ps2dma_limit)
#else
		    if(!ps2dma_limit || wqc != 2)
#endif
			ioctl(ps2gsfd, PS2IOC_SENDQCT, wqc);
                    ps2dbf_spr = 1 - ps2dbf_spr;
                    imgp->ptr = ps2dbp_spr[ps2dbf_spr];
                }
	        return;
            }
        }
    }
#endif /* #ifdef GSOS_SPR_IMAGEBUFFER */

    if (format != XYPixmap)
    {
        gsxGCPtr pPriv = gsxGetGCPriv(pGC) ;
        pPixmap = GetScratchPixmapHeader(pDraw->pScreen, w+leftPad, h, depth,
		      BitsPerPixel(depth), PixmapBytePad(w+leftPad, depth),
		      (pointer)pImage);
        if (!pPixmap)
            return;

	pPriv->fExpose = FALSE;
        if (format == ZPixmap) {
                (void)(*pGC->ops->CopyArea)((DrawablePtr)pPixmap, pDraw, pGC,
	    				leftPad, 0, w, h, x, y);
        } else {
	    (void)(*pGC->ops->CopyPlane)((DrawablePtr)pPixmap, pDraw, pGC,
					 leftPad, 0, w, h, x, y, 1);
        }
	pPriv->fExpose = TRUE;
        FreeScratchPixmapHeader(pPixmap);
    }
    else
    {
	miPutImage(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage);
    }
}



#ifdef MITSHM

PixmapPtr
gsxShmCreatePixmap (pScreen, width, height, depth, addr)
    ScreenPtr   pScreen;
    int         width;
    int         height;
    int         depth;
    char        *addr;
{
    register PixmapPtr pPixmap;

    pPixmap = (*pScreen->CreatePixmap)(pScreen, 0, 0, pScreen->rootDepth);
    if (!pPixmap)
        return NullPixmap;

    if (!(*pScreen->ModifyPixmapHeader)(pPixmap, width, height, depth,
                  BitsPerPixel(depth), PixmapBytePad(width, depth), 
		  (pointer)addr))
        return NullPixmap;
    return pPixmap;
}

void
gsxShmPutImage(dst, pGC, depth, format, w, h, sx, sy, sw, sh, dx, dy, data)
    DrawablePtr dst;
    GCPtr	pGC;
    int		depth, w, h, sx, sy, sw, sh, dx, dy;
    unsigned int format;
    char 	*data;
{
    PixmapPtr   pPixmap;

    if ((w == 0) || (h == 0))
	return;

/*#ifdef GSOS_SPR_IMAGEBUFFER
    if (format == ZPixmap) {
        if (dst->type == DRAWABLE_WINDOW && !gsxIsPlanemask(pGC)) {
            RegionPtr	clip;
            BoxRec		bbox;
            gsxGCPtr	pGCPriv = gsxGetGCPriv(pGC);

            clip = pGCPriv->pCompositeClip;
            bbox.x1=dx+dst->x;
            bbox.y1=dy+dst->y;
            bbox.x2=bbox.x1+sw;
            bbox.y2=bbox.y1+sh;

	    if (REGION_NUM_RECTS(clip) < 1)
	        return;

            if (RECT_IN_REGION(pGC->pScreen, clip, &bbox) == rgnIN) {
                char	*srcp;
                char	*dstp;
                int	size;
                int	nlines;
                int	maxlines;
                int	txfline;
                int	linebytewidth;
                int	i;
                struct ps2_image img;
                struct ps2_image *imgp = &img;
                gsxScreenPtr pScreenPriv;
                int bp;
                int bw;
                int psm;
                int bytepp;
	        int srcdevKind;
	        int dstdevKind;
	        int wqc;

                pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
                bp = pScreenPriv->fbp;
                bw = pScreenPriv->fbw;
                psm = pScreenPriv->psm;
	        bytepp = gsxGetBytePerPixel(pScreenPriv);

                gsosKick();

                srcdevKind = PixmapBytePad(w, depth);
                dstdevKind = sw * bytepp;

                imgp->ptr = ps2dbp_spr[ps2dbf_spr];
                imgp->fbp = bp;
                imgp->fbw = bw;
                imgp->psm = psm;

                srcp = data + sy * srcdevKind + sx * bytepp;
                linebytewidth = sw * bytepp;
	        maxlines = gsos_maximage_size / linebytewidth;

                imgp->x = dst->x + dx;
                imgp->w = sw;

                nlines = sh;
                while(nlines > 0) {
              	    txfline = (maxlines < nlines) ? maxlines : nlines;
                    size = txfline * linebytewidth;
                    imgp->y = dst->y + dy;
                    imgp->h = txfline ;
                    dy += txfline;
               	    nlines -= txfline;

                    dstp = imgp->ptr;
                    if(sx == 0 && srcdevKind == dstdevKind) {
                        memcpy((void *)dstp, (void *)srcp, size);
                        srcp += size;
                    } else {
                        for(i = 0; i < txfline; i++) {
                          memcpy((void *)dstp, (void *)srcp, dstdevKind);
                          dstp  += dstdevKind;
                          srcp += srcdevKind ;
		        }
                     }
#ifdef GSOS_LAZY_EXEC
                    wqc = 2;
#else
                    wqc = (nlines > 0) ? 2 : 1;
#endif
                    if(ioctl(ps2gsfd, PS2IOC_LOADIMAGEA, &img) < 0) {
                        ErrorF("gsxDoBitbltMW: PS2IOC_LOADIMAGEA Error\n");
                        return;
                    }
#ifdef GSOS_LAZY_EXEC
		    if(!ps2dma_limit)
#else
		    if(!ps2dma_limit || wqc != 2)
#endif
			ioctl(ps2gsfd, PS2IOC_SENDQCT, wqc);
                    ps2dbf_spr = 1 - ps2dbf_spr;
                    imgp->ptr = ps2dbp_spr[ps2dbf_spr];
                }
	        return;
            }
        }
    }
#endif*/

    if (format != XYPixmap)
    {
        gsxGCPtr pPriv = gsxGetGCPriv(pGC) ;
        pPixmap = GetScratchPixmapHeader(dst->pScreen, w, h, depth,
		      BitsPerPixel(depth), PixmapBytePad(w, depth),
		      (pointer)data);
        if (!pPixmap)
            return;

	pPriv->fExpose = FALSE;
        if (format == ZPixmap) {
                (void)(*pGC->ops->CopyArea)((DrawablePtr)pPixmap, dst, pGC,
	    				sx, sy, sw, sh, dx, dy);
        } else {
	    (void)(*pGC->ops->CopyPlane)((DrawablePtr)pPixmap, dst, pGC,
					 sx, sy, sw, sh, dx, dy, 1);
        }
	pPriv->fExpose = TRUE;
        FreeScratchPixmapHeader(pPixmap);
    } else {
        PixmapPtr pmap;
        GCPtr putGC;

        putGC = GetScratchGC(depth, dst->pScreen);
        if (!putGC)
            return;
        pmap = (*dst->pScreen->CreatePixmap)(dst->pScreen, sw, sh, depth);
        if (!pmap) {
            FreeScratchGC(putGC);
            return;
        }
        ValidateGC((DrawablePtr)pmap, putGC);
        (*putGC->ops->PutImage)((DrawablePtr)pmap, putGC, depth, 
      				-sx, -sy, w, h, 0, XYPixmap, data);
        FreeScratchGC(putGC);
        (void)(*pGC->ops->CopyArea)((DrawablePtr)pmap, dst, pGC, 
				0, 0, sw, sh, dx, dy);
        (*pmap->drawable.pScreen->DestroyPixmap)(pmap);
    }
}

#endif /* MITSHM */

