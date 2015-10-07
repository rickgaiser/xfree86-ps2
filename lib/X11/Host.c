/* $XConsortium: Host.c /main/7 1996/10/22 14:19:38 kaleb $ */
/*

Copyright (c) 1986  X Consortium

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

*/

/* this might be rightly reguarded an os dependent file */

#include "Xlibint.h"

XAddHost (dpy, host)
    register Display *dpy;
    XHostAddress *host;
    {
    register xChangeHostsReq *req;
    register int length = (host->length + 3) & ~0x3;	/* round up */

    LockDisplay(dpy);
    GetReqExtra (ChangeHosts, length, req);
    req->mode = HostInsert;
    req->hostFamily = host->family;
    req->hostLength = host->length;
    memcpy((char *) NEXTPTR(req,xChangeHostsReq), host->address, host->length);
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
    }

XRemoveHost (dpy, host)
    register Display *dpy;
    XHostAddress *host;
    {
    register xChangeHostsReq *req;
    register int length = (host->length + 3) & ~0x3;	/* round up */

    LockDisplay(dpy);
    GetReqExtra (ChangeHosts, length, req);
    req->mode = HostDelete;
    req->hostFamily = host->family;
    req->hostLength = host->length;
    memcpy((char *) NEXTPTR(req,xChangeHostsReq), host->address, host->length);
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
    }


XAddHosts (dpy, hosts, n)
    register Display *dpy;
    XHostAddress *hosts;
    int n;
{
    register int i;
    for (i = 0; i < n; i++) {
	(void) XAddHost(dpy, &hosts[i]);
      }
    return 1;
}

XRemoveHosts (dpy, hosts, n)
    register Display *dpy;
    XHostAddress *hosts;
    int n;
{
    register int i;
    for (i = 0; i < n; i++) {
	(void) XRemoveHost(dpy, &hosts[i]);
      }
    return 1;
}
