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

static
inline
void gsxPolyRectangleSoftWhole(dst,pGC,n,pRect, pDrawArea)
    DrawablePtr dst;
    GCPtr   pGC;
    int     n;
    xRectangle  *pRect;
    xRectangle  *pDrawArea;
{
    gsxGCPtr pPriv = gsxGetGCPriv(pGC) ;
    gsxScreenPtr pScreenPriv;

    xRectangle aRect ;
    PixmapPtr dstLop ;
    int nn ;
    GCPtr pGC1;
    int lineWidth;
    int drawRegion_x;
    int drawRegion_y;

    if(!n) return ;

    /* get region for LOP */
    aRect = *pDrawArea;
    lineWidth = pGC->lineWidth;
    lineWidth = (((lineWidth < 2) ? 2 : lineWidth) + 1);
    aRect.x += dst->x - lineWidth;
    aRect.y += dst->y - lineWidth;
    aRect.width  += lineWidth * 2;
    aRect.height += lineWidth * 2;
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

    /* draw lines */
    for( nn = 0 ; nn < n ; nn++ ) {
        pRect[nn].x -= drawRegion_x;
        pRect[nn].y -= drawRegion_y;
    }
    (*pGC1->ops->PolyRectangle)( (DrawablePtr)dstLop, pGC1, n, pRect ) ;

    FreeScratchGC(pGC1);

    /* ValidateGC( destination = WINDOW ) */
    GSXVALIDATEGC( dst, pGC ) ;

    /* post-proccess for LOP */
    cfbBitBlt( (DrawablePtr)dstLop, dst, pGC, 0, 0, aRect.width, aRect.height,
        drawRegion_x, drawRegion_y, gsxDoBitBlt, 0 ) ;

    (*dst->pScreen->DestroyPixmap)(dstLop) ;
}

static int CalcDrawPrimitiveRegion(pRect, n, margin)
    int     n;
    xRectangle  *pRect;
    int	    margin;
{
    int i;
    int	rect = 0;

    for(i = 0; i < n; i++, pRect++) {
        rect += 2*(pRect->width * 2*margin) + 2*(pRect->height * 2*margin);
    }
    return rect;
}

void gsxPolyRectangleSoft(dst,pGC,n,pRect)
    DrawablePtr dst;
    GCPtr   pGC;
    int     n;
    xRectangle  *pRect;
{
    int i;
    DDXPointRec points[5];
    gsxScreenPtr pScreenPriv;
    int dr;
    int margin;
    xRectangle aRect ;

    if(!n) return ;

    /* select faster method */
    pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    margin = gsxGetLineMargin(pGC);
    gsxCalcDrawRegionRect( pRect, n, &aRect ) ;
    dr = aRect.width * aRect.height; /*  area of every line */
    if (IS_SINGLE_OP_COVERED(dr,gsxGetBytePerPixel(pScreenPriv)) || 
    	      dr < CalcDrawPrimitiveRegion(pRect, n, margin)) {
	gsxPolyRectangleSoftWhole(dst,pGC,n,pRect, &aRect);
        return;
    }

    if (pGC->lineWidth<=1) {
        xSegment *p, *s;
	s = p = ALLOCATE_LOCAL(n*4*sizeof(xSegment));

        for(i = 0; i < n; i++, pRect++) {

            p->x1 = pRect->x;
            p->y1 = pRect->y;
            p->x2 = pRect->x + pRect->width;
            p->y2 = p->y1;
	    p++;

            p->x1 = pRect->x + pRect->width;
            p->y1 = p[-1].y1;
            p->x2 = p->x1;
            p->y2 = pRect->y + pRect->height;
	    p++;

            p->x1 = p[-1].x1;
            p->y1 = pRect->y + pRect->height;
            p->x2 = p[-2].x1;
            p->y2 = p->y1;
	    p++;

            p->x1 = p[-3].x1;
            p->y1 = p[-1].y1;
            p->x2 = pRect->x;
            p->y2 = pRect->y;
	    p++;
    
        }
        gsxPolySegmentSoft(dst, pGC, 4*n, s);
	DEALLOCATE_LOCAL(p);
	return;
    	
    }
    for(i = 0; i < n; i++, pRect++) {

        points[0].x = pRect->x;
        points[0].y = pRect->y;
        points[1].x = pRect->x + pRect->width;
        points[1].y = points[0].y;
        points[2].x = points[1].x;
        points[2].y = pRect->y + pRect->height;
        points[3].x = points[0].x;
        points[3].y = points[2].y;
        points[4].x = pRect->x;
        points[4].y = pRect->y;

        gsxPolyLinesSoft(dst, pGC, CoordModeOrigin, 5, points);

    }
}

void gsxPolyRectangleGxcpy(dst,pGC,n,pRect)
    DrawablePtr dst;
    GCPtr   pGC;
    int     n;
    xRectangle  *pRect;
{
    int i;
    xSegment *p, *s;
    s = p = ALLOCATE_LOCAL(n*4*sizeof(xSegment));

    for(i = 0; i < n; i++, pRect++) {

        p->x1 = pRect->x;
        p->y1 = pRect->y;
        p->x2 = pRect->x + pRect->width;
        p->y2 = p->y1;
        p++;

        p->x1 = pRect->x + pRect->width;
        p->y1 = p[-1].y1;
        p->x2 = p->x1;
        p->y2 = pRect->y + pRect->height;
        p++;

        p->x1 = p[-1].x1;
        p->y1 = pRect->y + pRect->height;
        p->x2 = p[-2].x1;
        p->y2 = p->y1;
        p++;

        p->x1 = p[-3].x1;
        p->y1 = p[-1].y1;
        p->x2 = pRect->x;
        p->y2 = pRect->y;
        p++;
    }
    gsxPolySegmentGxcpy(dst, pGC, 4*n, s);
    DEALLOCATE_LOCAL(p);
    return;
}

#if GSX_ACCL_DASH

void gsxPolyRectangleDashCT(dst,pGC,n,pRect)
    DrawablePtr dst;
    GCPtr   pGC;
    int     n;
    xRectangle  *pRect;
{
    int i;
    gsxScreenPtr pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    int		dashLen = pScreenPriv->dashLen;
    Bool	doSoft = FALSE;
    xSegment *p, *s;

    s = p = ALLOCATE_LOCAL(n*4*sizeof(xSegment));

    for(i = 0; i < n; i++, pRect++) {

        p->x1 = pRect->x;
        p->y1 = pRect->y;
        p->x2 = pRect->x + pRect->width;
        p->y2 = p->y1;
        p++;

        p->x1 = pRect->x + pRect->width;
        p->y1 = p[-1].y1;
        p->x2 = p->x1;
        p->y2 = pRect->y + pRect->height;
        p++;

        p->x1 = p[-1].x1;
        p->y1 = pRect->y + pRect->height;
        p->x2 = p[-2].x1;
        p->y2 = p->y1;
        p++;

        p->x1 = p[-3].x1;
        p->y1 = p[-1].y1;
        p->x2 = pRect->x;
        p->y2 = pRect->y;
        p++;

        if (!doSoft &&
	   ((pRect->width + dashLen)>(GSX_TEXTURE_CORD_MAX_X-1) ||
            (pRect->height + dashLen)>(GSX_TEXTURE_CORD_MAX_Y-1))) {
	    doSoft=TRUE;
        }
    }

    if (doSoft)
        gsxPolySegmentSoft(dst, pGC, 4*n, s);
    else
        gsxPolySegmentDashCT(dst, pGC, 4*n, s);

    DEALLOCATE_LOCAL(p);
    return;
}

#endif /* GSX_ACCL_DASH */
