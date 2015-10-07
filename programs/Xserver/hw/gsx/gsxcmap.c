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


typedef struct {
    char *name;
    char class;
    char depth;
    char type;
} gsxVisualType;

#define GSX_NUM_VISUAL 1
static const gsxVisualType gsxVisualTypes15[] = {
{"TrueColor16", TrueColor,  16,TrueColor},
};

static const gsxVisualType gsxVisualTypes24[] = {
{"TrueColor24", TrueColor,  24,TrueColor},
};

static int gsxGetVisualTypes(const gsxVisualType **pVisualtypes, int depth)
{
    int i;
    int nvisual;
    const gsxVisualType *vtp;

    if(depth == 15 || depth == 16) {
      nvisual = (sizeof(gsxVisualTypes15)/sizeof(gsxVisualType));
      vtp = gsxVisualTypes15;
    } else {
      nvisual = (sizeof(gsxVisualTypes24)/sizeof(gsxVisualType));
      vtp = gsxVisualTypes24;
    }
    for (i = 0; i < nvisual; ++i)
        pVisualtypes[i] = &vtp[i];
    return i;
}


int gsxCreateVisuals( ScreenPtr pScreen,
       VisualPtr *ppVisuals, int *pnVisuals,
       DepthPtr  *ppDepths,  int *pnDepths )
{
    int i,j;
    int r,g,b,d;
    int nVisuals;
    int nDepths;
    unsigned int haveDepth;
    const gsxVisualType *vtypes[GSX_NUM_VISUAL];
    VisualPtr pVisuals;
    DepthPtr  pDepths;
    VisualID  *pVisualIDs;

    nVisuals = gsxGetVisualTypes(vtypes, gsxGetScreenPriv(pScreen)->depth);
    *pnVisuals = nVisuals;

    haveDepth = 1;
    for (i = 0; i < nVisuals; ++i)
        haveDepth |= 1<<(vtypes[i]->depth-1);
    
    nDepths = 0;
    for (i = 0; i < 32; ++i)
        if (haveDepth & (1<<i)) nDepths++;
    *pnDepths = nDepths;

    // allocation 
    d = sizeof(VisualID)  * nVisuals
      + sizeof(VisualRec) * nVisuals
      + sizeof(DepthRec)  * nDepths ;
    pVisualIDs = (VisualID *)xalloc(sizeof(VisualID)  * nVisuals +
                                    sizeof(VisualRec) * nVisuals +
                                    sizeof(DepthRec)  * nDepths );

    if (!pVisualIDs) return 0;
    gsxGetScreenPriv(pScreen)->pVisualIDs = pVisualIDs;
    *ppVisuals = (VisualRec *)(pVisualIDs + nVisuals);
    *ppDepths  = (DepthRec  *)(*ppVisuals + nVisuals);

    // setting for depth
    pDepths = *ppDepths;
    pVisuals = *ppVisuals;
    for (i = 31; i > 1; --i)
        if (haveDepth & 1<<i) {
            pDepths->depth = i+1;
            pDepths->numVids = 0;
            pDepths->vids = pVisualIDs;
            for (j = 0; j < nVisuals; ++j)
                if (vtypes[j]->depth == pDepths->depth) {
                    pVisuals[j].vid = *pVisualIDs++ = FakeClientID(0);
                    ++pDepths->numVids;
                }
            ++pDepths;
        }
    // i = 1
    pDepths->depth = 1;
    pDepths->numVids = 0;
    pDepths->vids = NULL;
    
    // setting for visual
    for (i = 0; i < nVisuals; ++i, ++pVisuals) {
        pVisuals->class = vtypes[i]->class;
        pVisuals->nplanes = vtypes[i]->depth;
        pVisuals->bitsPerRGBValue = 8;
        pVisuals->ColormapEntries = 256;

        switch (pVisuals->class) {
        case DirectColor:
        case TrueColor:
        case StaticColor:
            d = pVisuals->nplanes;
            r = (d + 2) / 3;
            g = (d - r + 1) / 2;
            b = d - r - g;
            if (pVisuals->nplanes == 15 || pVisuals->nplanes == 16) {
	      pVisuals->bitsPerRGBValue = 5;
	      pVisuals->ColormapEntries = 32;
	      if(r > 5) r = 5;
	      pVisuals->redMask = (1<<r)-1;
	      pVisuals->greenMask = ((1<<g)-1)<<r;
	      pVisuals->blueMask = ((1<<b)-1)<<(r+g);
	      pVisuals->offsetRed = 0;
	      pVisuals->offsetGreen = r;
	      pVisuals->offsetBlue = r+g;
            } else {		/* pVisuals->nplanes == 32 */
	      pVisuals->redMask = (1<<r)-1;
	      pVisuals->greenMask = ((1<<g)-1)<<r;
	      pVisuals->blueMask = ((1<<b)-1)<<(r+g);
	      pVisuals->offsetRed = 0;
	      pVisuals->offsetGreen = r;
	      pVisuals->offsetBlue = r+g;
            }
            break;

        case PseudoColor:
        case GrayScale:
        case StaticGray:
            pVisuals->redMask = 0;
            pVisuals->greenMask = 0;
            pVisuals->blueMask = 0;
            pVisuals->offsetRed = 0;
            pVisuals->offsetGreen = 0;
            pVisuals->offsetBlue = 0;
            break;
        }
    }
    
    return nVisuals;
}

VisualPtr gsxGetVisual(int nVisuals,VisualPtr pVisuals,int class,int depth)
{
    VisualPtr   pVisual = NULL;

    while (nVisuals--) {
        if (pVisuals->class == class) {
            if (pVisuals->nplanes == depth) return pVisuals;
            if (!pVisual || pVisuals->nplanes > pVisual->nplanes)
                 pVisual = pVisuals;
        }
        pVisuals ++;
    }
    return (depth == -1)? pVisual:0;
}

Bool gsxCreateDefColormap(ScreenPtr pScreen)
{
    unsigned short      zero = 0, ones = ~0;
    VisualPtr   pVisual;
    ColormapPtr pCmap;

    /* get rootVisual */
    for (pVisual = pScreen->visuals; pVisual->vid != pScreen->rootVisual;
         pVisual++)
        ;

    if (CreateColormap(pScreen->defColormap, pScreen, pVisual,
        &pCmap, (pVisual->class & DynamicClass) ? AllocNone : AllocAll, 0)
        != Success) {
            return FALSE;
    }

    pScreen->blackPixel = (Pixel)0;
    pScreen->whitePixel = (Pixel)1;

    if (AllocColor(pCmap,&ones,&ones,&ones,&(pScreen->whitePixel),0)
        != Success) {
            return FALSE;
    }
    if (AllocColor(pCmap, &zero, &zero, &zero, &(pScreen->blackPixel), 0)
        != Success) {
            return FALSE;
    }

    (*pScreen->InstallColormap)(pCmap);
    return TRUE;
}

