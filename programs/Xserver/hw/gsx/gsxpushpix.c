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

void gsxPushPixelsSoft(pGC,pBitmap,dst,dx,dy,xOrg,yOrg)
    GCPtr   pGC;
    PixmapPtr pBitmap;
    DrawablePtr dst;
    int     dx, dy, xOrg, yOrg;
{
    gsxScreenPtr pScreenPriv;
    xRectangle aRect ;
    PixmapPtr dstLop ;
    GCPtr pGC1;

    if(!dx*dy) return ;

    /* get region for LOP */
    aRect.x = xOrg ;
    aRect.y = yOrg ;
    aRect.width = dx ;
    aRect.height = dy ;

    dstLop = (*dst->pScreen->CreatePixmap)
        (dst->pScreen, aRect.width, aRect.height, dst->depth ) ;
    if( !dstLop ){
        return ;
    }
    /* copy destination */
    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    gsxReadImage(aRect.x, aRect.y, aRect.width, aRect.height,
	pScreenPriv->fbp,
	pScreenPriv->fbw,
	pScreenPriv->psm,
        dstLop);

    /* ValidateGC( destination = PIXMAP ) */
    pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
    CopyGC(pGC, pGC1, ~GCClipMask);
    GSXVALIDATEGC( (DrawablePtr)dstLop, pGC1 ) ;

    /* draw lines */
    (*pGC1->ops->PushPixels)( pGC1, pBitmap, (DrawablePtr)dstLop, 
        dx, dy, xOrg-(aRect.x-dst->x), yOrg-(aRect.y-dst->y) ) ;

    FreeScratchGC(pGC1);

    /* ValidateGC( destination = WINDOW ) */
    GSXVALIDATEGC( dst, pGC ) ;

    /* post-proccess for LOP */
    cfbBitBlt( (DrawablePtr)dstLop, dst, pGC, 0, 0, aRect.width, aRect.height,
        aRect.x-dst->x, aRect.y-dst->y, gsxDoBitBlt, 0 ) ;

    (*dst->pScreen->DestroyPixmap)(dstLop) ;
}
