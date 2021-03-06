/* $XConsortium: XGetBMap.c,v 1.7 94/04/17 20:18:01 rws Exp $ */
/* $XFree86: xc/lib/Xi/XGetBMap.c,v 3.0 1996/08/25 13:52:38 dawes Exp $ */

/************************************************************

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Hewlett-Packard not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

HEWLETT-PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
HEWLETT-PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/***********************************************************************
 *
 * XGetDeviceButtonMapping - Get the button mapping of an extension device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"
#include "XIint.h"

#ifdef MIN			/* some systems define this in <sys/param.h> */
#undef MIN
#endif
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int
XGetDeviceButtonMapping (dpy, device, map, nmap)
    register 	Display	*dpy;
    XDevice		*device;
    unsigned 	char 	map[];
    unsigned 	int 	nmap; 
    {
    int	status = 0;
    unsigned char mapping[256];				/* known fixed size */
    long nbytes;
    XExtDisplayInfo *info = XInput_find_display (dpy);

    register xGetDeviceButtonMappingReq *req;
    xGetDeviceButtonMappingReply rep;

    LockDisplay(dpy);
    if (_XiCheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);
    GetReq(GetDeviceButtonMapping, req);

    req->reqType = info->codes->major_opcode;
    req->ReqType = X_GetDeviceButtonMapping;
    req->deviceid = device->device_id;

    status = _XReply (dpy, (xReply *)&rep, 0, xFalse);
    if (status == 1)
	{
	nbytes = (long)rep.length << 2;
	_XRead (dpy, (char *)mapping, nbytes);

	/* don't return more data than the user asked for. */
	if (rep.nElts) 
	    memcpy ((char *) map, (char *) mapping, MIN((int)rep.nElts, nmap));
	status = rep.nElts;
	}
    else
	status = 0;
    UnlockDisplay(dpy);
    SyncHandle();
    return (status);
    }
