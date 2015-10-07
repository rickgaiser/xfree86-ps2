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

void gsxImageGlyphBltSoft(dst,pGC,x,y,nglyph,ppci,pglyphBase)
    DrawablePtr dst;
    GCPtr   pGC;
    int     x, y;
    unsigned int nglyph;
    CharInfoRec **ppci;
    pointer pglyphBase;
{
    gsxScreenPtr pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    xRectangle backrect ;
    xRectangle pmrect ;
    PixmapPtr dstLop ;
    GCPtr pGC1;
    int ascent;
    int dstx, dsty;
    ExtentInfoRec info;

    if(!nglyph) return ;

    QueryGlyphExtents(pGC->font, ppci, (unsigned long)nglyph, &info);

    if (info.overallWidth >= 0) {
      backrect.x = x;
      backrect.width = info.overallWidth;
    } else {
      backrect.x = x + info.overallWidth;
      backrect.width = -info.overallWidth;
    }
    ascent = FONTASCENT(pGC->font);
    backrect.y = y - ascent;
    backrect.height = ascent + FONTDESCENT(pGC->font);

    /* Pixmap rect */
    pmrect.x = 0;
    pmrect.y = 0;
    pmrect.width = backrect.width;
    pmrect.height = backrect.height;

    dstx = dst->x;
    backrect.x += dstx;
    dsty = dst->y;
    backrect.y += dsty;
    dstLop = (*dst->pScreen->CreatePixmap)
        (dst->pScreen, pmrect.width, pmrect.height, dst->depth ) ;
    if( !dstLop ){
        return ;
    }
    /* pixmap fill from framebuffer */
    gsxReadImage(backrect.x, backrect.y, backrect.width, backrect.height,
        pScreenPriv->fbp, pScreenPriv->fbw, pScreenPriv->psm,
        dstLop) ;

    /* ValidateGC( destination = PIXMAP ) */
    pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
    CopyGC(pGC, pGC1, ~GCClipMask);
    GSXVALIDATEGC( (DrawablePtr)dstLop, pGC1 ) ;

    /* draw  */
    (*pGC1->ops->ImageGlyphBlt)( (DrawablePtr)dstLop, pGC1, 
        0, ascent, nglyph, ppci, pglyphBase ) ;

    FreeScratchGC(pGC1);

    /* post-proccess for LOP */
    cfbBitBlt( (DrawablePtr)dstLop, dst, pGC, 0, 0,
	backrect.width, backrect.height,
        backrect.x - dstx, backrect.y - dsty, gsxDoBitBlt, 0 ) ;

    (*dst->pScreen->DestroyPixmap)(dstLop) ;
}

void gsxImageGlyphBltGxcpy(dst,pGC,x,y,nglyph,ppci,pglyphBase)
    DrawablePtr dst;
    GCPtr   pGC;
    int     x, y;
    unsigned int nglyph;
    CharInfoRec **ppci;
    pointer pglyphBase;
{
    xRectangle backrect;
    xRectangle pmrect;
    PixmapPtr dstLop;
    GCPtr pGC1;
    int ascent;
    ExtentInfoRec info;

    if(!nglyph) return ;

    QueryGlyphExtents(pGC->font, ppci, (unsigned long)nglyph, &info);

    if (info.overallWidth >= 0) {
      backrect.x = x;
      backrect.width = info.overallWidth;
    } else {
      backrect.x = x + info.overallWidth;
      backrect.width = -info.overallWidth;
    }
    ascent = FONTASCENT(pGC->font);
    backrect.y = y - ascent;
    backrect.height = ascent + FONTDESCENT(pGC->font);

    /* Pixmap rect */
    pmrect.x = 0;
    pmrect.y = 0;
    pmrect.width = backrect.width;
    pmrect.height = backrect.height;

    /* */
    dstLop = (*dst->pScreen->CreatePixmap)
        (dst->pScreen, pmrect.width, pmrect.height, dst->depth );
    if(!dstLop){
        return ;
    }

    /* ValidateGC( destination = PIXMAP ) */
    pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
    CopyGC(pGC, pGC1, ~GCClipMask);
    GSXVALIDATEGC( (DrawablePtr)dstLop, pGC1 ) ;

    /* draw  */
    (*pGC1->ops->ImageGlyphBlt)((DrawablePtr)dstLop, pGC1, 
        0, ascent, nglyph, ppci, pglyphBase);

    FreeScratchGC(pGC1);

    /* post-proccess for LOP */
    cfbBitBlt( (DrawablePtr)dstLop, dst, pGC, 0, 0,
	backrect.width, backrect.height,
        backrect.x, backrect.y, gsxDoBitBlt, 0 ) ;

    (*dst->pScreen->DestroyPixmap)(dstLop) ;
}

void gsxPolyGlyphBltSoft(dst,pGC,x,y,nglyph,ppci,pglyphBase)
    DrawablePtr dst;
    GCPtr   pGC;
    int     x, y;
    unsigned int nglyph;
    CharInfoRec **ppci;
    pointer pglyphBase;
{
    gsxScreenPtr pScreenPriv = gsxGetScreenPriv(pGC->pScreen);
    xRectangle backrect ;
    PixmapPtr dstLop ;
    GCPtr pGC1;
    int ascent;
    int dstx, dsty;
    ExtentInfoRec info;

    if(!nglyph) return ;

    QueryGlyphExtents(pGC->font, ppci, (unsigned long)nglyph, &info);

    if (info.overallWidth >= 0) {
      backrect.x = x;
      backrect.width = info.overallWidth;
    } else {
      backrect.x = x + info.overallWidth;
      backrect.width = -info.overallWidth;
    }
    ascent = FONTASCENT(pGC->font);
    backrect.y = y - ascent;
    backrect.height = ascent + FONTDESCENT(pGC->font);
    dstx = dst->x;
    backrect.x += dstx;
    dsty = dst->y;
    backrect.y += dsty;

    dstLop = (*dst->pScreen->CreatePixmap)
        (dst->pScreen, backrect.width, backrect.height, dst->depth ) ;
    if( !dstLop ){
        return ;
    }
    /* pixmap fill from framebuffer */
    gsxReadImage(backrect.x, backrect.y, backrect.width, backrect.height,
        pScreenPriv->fbp, pScreenPriv->fbw, pScreenPriv->psm,
        dstLop) ;

    /* ValidateGC( destination = PIXMAP ) */
    pGC1 = GetScratchGC(dst->depth, pGC->pScreen);
    CopyGC(pGC, pGC1, ~GCClipMask);
    GSXVALIDATEGC( (DrawablePtr)dstLop, pGC1 ) ;

    /* draw  */
    (*pGC1->ops->PolyGlyphBlt)( (DrawablePtr)dstLop, pGC1, 
        0, ascent, nglyph, ppci, pglyphBase ) ;

    FreeScratchGC(pGC1);

    /* post-proccess for LOP */
    cfbBitBlt( (DrawablePtr)dstLop, dst, pGC, 0, 0,
	backrect.width, backrect.height,
        backrect.x - dstx, backrect.y - dsty, gsxDoBitBlt, 0 ) ;
    (*dst->pScreen->DestroyPixmap)(dstLop) ;
}


