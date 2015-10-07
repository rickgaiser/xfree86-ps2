/* $TOG: lcFile.c /main/9 1997/06/03 15:52:47 kaleb $ */
/*
 *
 * Copyright IBM Corporation 1993
 *
 * All Rights Reserved
 *
 * License to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS, AND 
 * NONINFRINGEMENT OF THIRD PARTY RIGHTS, IN NO EVENT SHALL
 * IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
*/
/* $XFree86: xc/lib/X11/lcFile.c,v 3.6.2.8 1998/10/04 13:36:25 hohndel Exp $ */

#include <stdio.h>
#include <ctype.h>
#include "Xlibint.h"
#include "XlcPubI.h"
#include <X11/Xos.h>
#ifdef X_NOT_STDC_ENV
extern char *getenv();
#endif

/************************************************************************/

#define	iscomment(ch)	((ch) == '#' || (ch) == '\0')
#define isreadable(f)	((access((f), R_OK) != -1) ? 1 : 0)

#ifndef __EMX__
#define LC_PATHDELIM ':'
#else
#define LC_PATHDELIM ';'
#endif

#define XLC_BUFSIZE 256

#ifndef X_NOT_POSIX
#ifdef _POSIX_SOURCE
#include <limits.h>
#else
#define _POSIX_SOURCE
#include <limits.h>
#undef _POSIX_SOURCE
#endif
#endif
#ifndef PATH_MAX
#ifdef WIN32
#define PATH_MAX 512
#else
#include <sys/param.h>
#endif
#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif
#endif

#define NUM_LOCALEDIR	64

static int
parse_line(line, argv, argsize)
    char *line;
    char **argv;
    int argsize;
{
    int argc = 0;
    char *p = line;

    while(argc < argsize){
	while(isspace(*p)){
	    ++p;
	}
	if(*p == '\0'){
	    break;
	}
	argv[argc++] = p;
	while(! isspace(*p) && *p != '\0'){
	    ++p;
	}
	if(*p == '\0'){
	    break;
	}
	*p++ = '\0';
    }

    return argc;
}

/* parse the colon separated list in path into argv */
int
_XlcParsePath(path, argv, argsize)
    char *path;
    char **argv;
    int argsize;
{
    char *p = path;
    int i, n;

    while((p = strchr(p, LC_PATHDELIM)) != NULL){
	*p = ' ';	/* place space on delimter */
    }
    n = parse_line(path, argv, argsize);
    if(n == 0){
	return 0;
    }
    for(i = 0; i < n; ++i){
	int len;
	p = argv[i];
	len = strlen(p);
	if(p[len - 1] == '/'){
	    /* eliminate slash */
	    p[len - 1] = '\0';
	}
    }
    return n;
}

#ifndef XLOCALEDIR
#define XLOCALEDIR "/usr/lib/X11/locale"
#endif

static void
xlocaledir(buf, buf_len)
    char *buf;
    int buf_len;
{
    char *dir, *p = buf;
    int len = 0;

    dir = getenv("XLOCALEDIR");
    if(dir != NULL){
	len = strlen(dir);
	strncpy(p, dir, buf_len);
	if (len < buf_len) {
	    p[len++] = LC_PATHDELIM;
	    p += len;
	}
    }
    if (len < buf_len)
#ifndef __EMX__
      strncpy(p, XLOCALEDIR, buf_len - len);
#else
      strncpy(p,__XOS2RedirRoot(XLOCALEDIR), buf_len - len);
#endif
    buf[buf_len-1] = '\0';
}

enum { LtoR, RtoL };

static char *
resolve_name(lc_name, file_name, direction)
    char *lc_name;
    char *file_name;
    int direction;	/* mapping direction */
{
    FILE *fp;
    char buf[XLC_BUFSIZE], *name = NULL;

    fp = fopen(file_name, "r");
    if(fp == (FILE *)NULL){
	return NULL;
    }

    while(fgets(buf, XLC_BUFSIZE, fp) != NULL){
	char *p = buf;
	int n;
	char *args[2], *from, *to;
#ifdef __EMX__  /* Take out CR under OS/2 */
	int len;

	len=strlen(p);
	if (len>1) {
	  if (*(p+len-2) == '\r' && *(p+len-1) == '\n') {
		*(p+len-2) = '\n';
		*(p+len-1) = '\0';
	  }
	}
#endif
	while(isspace(*p)){
	    ++p;
	}
	if(iscomment(*p)){
	    continue;
	}
	n = parse_line(p, args, 2);		/* get first 2 fields */
	if(n != 2){
	    continue;
	}
	if(direction == LtoR){
	    from = args[0], to = args[1];	/* left to right */
	}else{
	    from = args[1], to = args[0];	/* right to left */
	}
	if(! strcmp(from, lc_name)){
	    name = Xmalloc(strlen(to) + 1);
	    if(name != NULL){
		strcpy(name, to);
	    }
	    break;
	}
    }
    if(fp != (FILE *)NULL){
	fclose(fp);
    }
    return name;
}

/*
#define	isupper(ch)	('A' <= (ch) && (ch) <= 'Z')
#define	tolower(ch)	((ch) - 'A' + 'a')
*/
static char *
lowercase(dst, src)
    char *dst;
    char *src;
{
    char *s, *t;

    for(s = src, t = dst; *s; ++s, ++t){
	*t = isupper(*s) ? tolower(*s) : *s;
    }
    *t = '\0';
    return dst;
}

/************************************************************************/
char *
_XlcFileName(lcd, category)
    XLCd lcd;
    char *category;
{
    char *siname;
    char cat[XLC_BUFSIZE], dir[XLC_BUFSIZE];
    int i, n;
    char *args[NUM_LOCALEDIR];
    char *file_name = NULL;

    if(lcd == (XLCd)NULL){
	return NULL;
    }

    siname = XLC_PUBLIC(lcd, siname);

    lowercase(cat, category);
    xlocaledir(dir,XLC_BUFSIZE);
    n = _XlcParsePath(dir, args, NUM_LOCALEDIR);
    for(i = 0; i < n; ++i){
	char buf[PATH_MAX], *name;

	name = NULL;
	if ((5 + (args[i] ? strlen (args[i]) : 0) +
	    (cat ? strlen (cat) : 0)) < PATH_MAX) {
	    sprintf(buf, "%s/%s.dir", args[i], cat);
	    name = resolve_name(siname, buf, RtoL);
	}
	if(name == NULL){
	    continue;
	}
	if(*name == '/'){
	    /* supposed to be absolute path name */
	    file_name = name;
	}else{
	    file_name = Xmalloc(2 + (args[i] ? strlen (args[i]) : 0) +
				(name ? strlen (name) : 0));
	    if (file_name != NULL)
		sprintf(file_name, "%s/%s", args[i], name);
	    Xfree(name);
	}
	if(isreadable(file_name)){
	    break;
	}
	Xfree(file_name);
	file_name = NULL;
	/* Then, try with next dir */
    }
    return file_name;
}

/************************************************************************/
#ifndef LOCALE_ALIAS
#define LOCALE_ALIAS    "locale.alias"
#endif

int
_XlcResolveLocaleName(lc_name, pub)
    char* lc_name;
    XLCdPublicPart* pub;
{
    char dir[PATH_MAX], buf[PATH_MAX], *name = NULL;
    char *dst;
    int i, n, len, sinamelen;
    char *args[NUM_LOCALEDIR];
    static char locale_alias[] = LOCALE_ALIAS;
    char *tmp_siname;

    xlocaledir (dir, PATH_MAX);
    n = _XlcParsePath(dir, args, NUM_LOCALEDIR);
    for(i = 0; i < n; ++i){
	if ((2 + (args[i] ? strlen (args[i]) : 0) + 
	    strlen (locale_alias)) < PATH_MAX) {
	    sprintf (buf, "%s/%s", args[i], locale_alias);
	    name = resolve_name (lc_name, buf, LtoR);
	}
	if(name != NULL){
	    break;
	}
    }

    if (name == NULL) {
	/* vendor locale name == Xlocale name, no expansion of alias */
	pub->siname = Xmalloc (strlen (lc_name) + 1);
	strcpy (pub->siname, lc_name);
    } else {
	pub->siname = name;
    }

    sinamelen = strlen (pub->siname);
    if (sinamelen == 1 && pub->siname[0] == 'C') {
	pub->language = pub->siname;
	pub->territory = pub->codeset = NULL;
	return 1;
    }

    /* 
     * pub->siname is in the format <lang>_<terr>.<codeset>, typical would
     * be "en_US.ISO8859-1", "en_US.utf8", or "ru_RU.KOI-8"
     */
    tmp_siname = Xrealloc (pub->siname, 2 * (sinamelen + 1));
    if (tmp_siname == NULL) {
	return 0;
    } 
    pub->siname = tmp_siname;

    /* language */
    dst = &pub->siname[sinamelen + 1];
    strcpy (dst, pub->siname);
    pub->language = dst;

    /* territory */
    dst = strchr (dst, '_');
    if (dst) {
	*dst = '\0';
	pub->territory = ++dst;

	/* codeset */
	dst = strchr (dst, '.');
	if (dst) {
	    *dst = '\0';
	    pub->codeset = ++dst;
	}
    }

    return (pub->siname[0] != '\0') ? 1 : 0;
}

/************************************************************************/
int
_XlcResolveI18NPath(buf, buf_len)
    char *buf;
    int buf_len;
{
    if(buf != NULL){
	xlocaledir(buf, buf_len);
    }
    return 1;
}
