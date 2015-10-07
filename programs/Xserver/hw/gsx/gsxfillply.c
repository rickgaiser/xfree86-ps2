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

void gsxFillPolygonSoft(dst,pGC,shape,mode,count,pPoint)
    DrawablePtr dst;
    GCPtr   pGC;
    int     shape, mode, count;
    DDXPointPtr pPoint;
{
    gsxGCPtr pPriv = gsxGetGCPriv(pGC) ;
    xRectangle aRect ;
    PixmapPtr dstLop ;
    int nn ;
    GCPtr pGC1;
    int margin;
    int drawRegion_x;
    int drawRegion_y;
    gsxScreenPtr pScreenPriv;

    if(!count) return ;
    if (mode == CoordModePrevious) {
        DDXPointPtr p = pPoint;
	DDXPointPtr pLast = pPoint + count;
	int	x, y;
        x = p->x; y = p->y;
        while (++p < pLast) {
            x = p->x += x;
            y = p->y += y;
        }
    }

    /* get region for LOP */
    gsxCalcDrawRegion( pPoint, count, &aRect ) ;

    /* get region for LOP */
    margin = gsxGetLineMargin(pGC);
    aRect.x += dst->x - margin;
    aRect.y += dst->y - margin;
    aRect.width  += margin * 2;
    aRect.height += margin * 2;
    drawRegion_x = aRect.x - dst->x;
    drawRegion_y = aRect.y - dst->y;

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

    /* adjust pattern origin */
    if (pGC->fillStyle != FillSolid) {
        ChangeGCVal gcval[2];
        gcval[0].val = pGC1->patOrg.x + (pPriv->patW - drawRegion_x % pPriv->patW);
        gcval[1].val = pGC1->patOrg.y + (pPriv->patH - drawRegion_y % pPriv->patH);
        DoChangeGC (pGC1, GCTileStipXOrigin|GCTileStipYOrigin, 
      			(XID *)gcval, TRUE);
    }
    GSXVALIDATEGC( (DrawablePtr)dstLop, pGC1 ) ;

    /* draw polygon */
    for( nn = 0 ; nn < count ; nn++ ) {
        pPoint[nn].x -= drawRegion_x;
        pPoint[nn].y -= drawRegion_y;
    }
    (*pGC1->ops->FillPolygon)( (DrawablePtr)dstLop, pGC1, shape, CoordModeOrigin, count, pPoint ) ;

    FreeScratchGC(pGC1);

    /* ValidateGC( destination = WINDOW ) */
    GSXVALIDATEGC( dst, pGC ) ;

    /* post-proccess for LOP */
    cfbBitBlt( (DrawablePtr)dstLop, dst, pGC, 0, 0, aRect.width, aRect.height,
        drawRegion_x, drawRegion_y, gsxDoBitBlt, 0 ) ;

    (*dst->pScreen->DestroyPixmap)(dstLop) ;
}

