#ifndef OS2
XCOMM! /bin/sh
#else
/* REXX script */
#endif
XCOMM $XConsortium: server.cpp,v 1.4 94/02/06 19:52:05 mor Exp $
XCOMM $XFree86: xc/programs/rstart/server.cpp,v 3.0 1995/03/11 14:20:38 dawes Exp $
XCOMM

XCOMM Copyright (c) 1993 Quarterdeck Office Systems
XCOMM
XCOMM Permission to use, copy, modify, distribute, and sell this software
XCOMM and software and its documentation for any purpose is hereby granted
XCOMM without fee, provided that the above copyright notice appear in all
XCOMM copies and that both that copyright notice and this permission
XCOMM notice appear in supporting documentation, and that the name
XCOMM Quarterdeck Office Systems, Inc. not be used in advertising or
XCOMM publicity pertaining to distribution of this software without
XCOMM specific, written prior permission.
XCOMM
XCOMM THIS SOFTWARE IS PROVIDED "AS-IS".  QUARTERDECK OFFICE SYSTEMS,
XCOMM INC., DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
XCOMM INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF
XCOMM MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
XCOMM NONINFRINGEMENT.  IN NO EVENT SHALL QUARTERDECK OFFICE SYSTEMS,
XCOMM INC., BE LIABLE FOR ANY DAMAGES WHATSOEVER, INCLUDING SPECIAL,
XCOMM INCIDENTAL OR CONSEQUENTIAL DAMAGES, INCLUDING LOSS OF USE, DATA, OR
XCOMM PROFITS, EVEN IF ADVISED OF THE POSSIBILITY THEREOF, AND REGARDLESS
XCOMM OF WHETHER IN AN ACTION IN CONTRACT, TORT OR NEGLIGENCE, ARISING OUT
XCOMM OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef OS2
exec LIBDIR/SERVERNAME.real -c LIBDIR/config
#else
start LIBDIR/SERVERNAME -c LIBDIR/config
#endif
