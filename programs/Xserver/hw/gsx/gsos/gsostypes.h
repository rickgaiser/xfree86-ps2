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

#ifndef GSOSTYPES_H
#define GSOSTYPES_H

typedef int                GSOSint ;
typedef unsigned int       GSOSuint ;
typedef short              GSOSshort ;
typedef unsigned short     GSOSushort ;
typedef char               GSOSchar ;
typedef unsigned char      GSOSuchar ;
typedef long               GSOSlong ;
typedef unsigned long      GSOSulong ;
typedef float              GSOSfloat ;
typedef double             GSOSdouble ;
typedef unsigned long long GSOSbit64 ;
typedef unsigned int 	   GSOSbit128 __attribute__((mode(TI)));


typedef union {
    GSOSbit64 ul64[2];
    GSOSuint  ui32[4];
} GSOSpacket ;

typedef union {
	struct { unsigned short 
		lo_lo0, lo_lo1, lo_hi0, lo_hi1,
		hi_lo0, hi_lo1, hi_hi0, hi_hi1; } halfi;
	struct { unsigned int lo0, lo1, hi0, hi1; } si;
	struct { GSOSbit64 lo, hi; } di;
	GSOSbit128 ti;
} GSOSQword;

typedef union {
	GSOSbit64 di;
	struct {unsigned int lo, hi;} si;
} GSOSDword;

typedef union {
	unsigned int si;
	struct {unsigned short lo, hi;} halfi;
} GSOSSword;

#endif /* GSOSTYPES_H */
