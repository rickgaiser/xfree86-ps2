/*
 *	$TOG: screen.c /main/37 1997/08/26 14:13:55 kaleb $
 */

/*
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 *
 *                         All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Digital Equipment
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 *
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/* $XFree86: xc/programs/xterm/screen.c,v 3.12.2.6 1999/10/21 12:08:16 hohndel Exp $ */

/* screen.c */

#include <xterm.h>
#include <error.h>
#include <data.h>
#include <xcharmouse.h>

#include <signal.h>

#ifdef att
#define ATT
#endif

#ifdef SVR4
#undef SYSV
#define SYSV
#include <termios.h>
#else
#ifndef __CYGWIN__
#include <sys/ioctl.h>
#endif
#endif

#if defined(__CYGWIN__) && !defined(TIOCSPGRP)
#include <termios.h>
#define TIOCSPGRP (_IOW('t', 118, pid_t))
#endif

#ifdef __hpux
#include <sys/termio.h>
#endif

#if (defined(ATT) && !defined(__sgi)) || (defined(SYSV) && defined(i386)) || (defined (__GLIBC__) && (__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 1))
#define USE_USG_PTYS
#endif

#ifdef SYSV
#if !defined(DGUX)    /* Intel DG/ux uses termios.h */
#include <sys/termio.h>
#endif /* DGUX */
#ifdef USE_USG_PTYS
#include <sys/stream.h>			/* get typedef used in ptem.h */
#include <sys/ptem.h>
#endif
#elif defined(sun) && !defined(SVR4)
#include <sys/ttycom.h>
#ifdef TIOCSWINSZ
#undef TIOCSSIZE
#endif
#endif

#ifdef MINIX
#include <termios.h>
#endif

#ifdef ISC
#ifndef SYSV
#include <sys/termio.h>
#endif
#define TIOCGPGRP TCGETPGRP
#define TIOCSPGRP TCSETPGRP
#endif

#ifdef __EMX__
extern int ptioctl(int fd, int func, void* data);
#define ioctl ptioctl
#define TIOCSWINSZ	113
#define TIOCGWINSZ	117
struct winsize {
        unsigned short  ws_row;         /* rows, in characters */
        unsigned short  ws_col;         /* columns, in characters */
        unsigned short  ws_xpixel;      /* horizontal size, pixels */
        unsigned short  ws_ypixel;      /* vertical size, pixels */
};
#endif

/*
 * Allocates memory for a 2-dimensional array of chars and returns a pointer
 * thereto.  Each line is formed from a set of char arrays, with an index
 * (i.e., the ScrnBuf type).  The first pointer in the index is reserved for
 * per-line flags, and does not point to data.
 *
 * After the per-line flags, we have a series of pointers to char arrays:  The
 * first one is the actual character array, the second one is the attributes,
 * the third is the foreground and background colors, and the fourth denotes
 * the character set.
 *
 * We store it all as pointers, because of alignment considerations, together
 * with the intention of being able to change the total number of pointers per
 * row according to whether the user wants color or not.
 */
ScrnBuf Allocate (register int nrow, register int ncol, Char **addr)
{
	register ScrnBuf base;
	register Char *tmp;
	register int i, j, k;
	size_t entries = MAX_PTRS * nrow;
	size_t length  = BUF_PTRS * nrow * ncol;

	if ((base = (ScrnBuf) calloc (entries, sizeof (char *))) == 0)
		SysError (ERROR_SCALLOC);

	if ((tmp = (Char *)calloc (length, sizeof(Char))) == 0)
		SysError (ERROR_SCALLOC2);

	*addr = tmp;
	for (i = k = 0; i < nrow; i++) {
		base[k] = 0;	/* per-line flags */
		k += BUF_HEAD;
		for (j = BUF_HEAD; j < MAX_PTRS; j++) {
			base[k++] = tmp;
			tmp += ncol;
		}
	}

	return (base);
}

/*
 *  This is called when the screen is resized.
 *  Returns the number of lines the text was moved down (neg for up).
 *  (Return value only necessary with SouthWestGravity.)
 */
static int
Reallocate(
	ScrnBuf *sbuf,
	Char **sbufaddr,
	int nrow,
	int ncol,
	int oldrow,
	int oldcol)
{
	register ScrnBuf base;
	register Char *tmp;
	register int i, j, k, minrows;
	register size_t mincols;
	Char *oldbuf;
	int move_down = 0, move_up = 0;
	size_t entries = MAX_PTRS * nrow;
	size_t length  = BUF_PTRS * nrow * ncol;

	if (sbuf == NULL || *sbuf == NULL) {
		return 0;
	}

	oldbuf = *sbufaddr;

	/*
	 * Special case if oldcol == ncol - straight forward realloc and
	 * update of the additional lines in sbuf
	 *
	 * FIXME: this is a good idea, but doesn't seem to be implemented.
	 * -gildea
	 */

	/*
	 * realloc sbuf, the pointers to all the lines.
	 * If the screen shrinks, remove lines off the top of the buffer
	 * if resizeGravity resource says to do so.
	 */
	if (nrow < oldrow
	 && term->misc.resizeGravity == SouthWestGravity) {
	    /* Remove lines off the top of the buffer if necessary. */
	    move_up = (oldrow - nrow)
		        - (term->screen.max_row - term->screen.cur_row);
	    if (move_up < 0)
		move_up = 0;
	    /* Overlapping memmove here! */
	    memmove( *sbuf, *sbuf + (move_up * MAX_PTRS),
		  MAX_PTRS * (oldrow - move_up) * sizeof((*sbuf)[0]) );
	}
	*sbuf = (ScrnBuf) realloc((char *) (*sbuf), entries * sizeof(char *));
	if (*sbuf == 0)
	    SysError(ERROR_RESIZE);
	base = *sbuf;

	/*
	 *  create the new buffer space and copy old buffer contents there
	 *  line by line.
	 */
	if ((tmp = (Char *)calloc(length, sizeof(Char))) == 0)
		SysError(ERROR_SREALLOC);
	*sbufaddr = tmp;
	minrows = (oldrow < nrow) ? oldrow : nrow;
	mincols = (oldcol < ncol) ? oldcol : ncol;
	if (nrow > oldrow
	 && term->misc.resizeGravity == SouthWestGravity) {
	    /* move data down to bottom of expanded screen */
	    move_down = Min(nrow - oldrow, term->screen.savedlines);
	    tmp += (ncol * move_down * BUF_PTRS);
	}

	for (i = k = 0; i < minrows; i++) {
		k += BUF_HEAD;
		for (j = BUF_HEAD; j < MAX_PTRS; j++) {
			memcpy(tmp, base[k++], mincols);
			tmp += ncol;
		}
	}

	/*
	 * update the pointers in sbuf
	 */
	for (i = k = 0, tmp = *sbufaddr; i < nrow; i++) {
		for (j = 0; j < BUF_HEAD; j++)
			base[k++] = 0;
		for (j = BUF_HEAD; j < MAX_PTRS; j++) {
			base[k++] = tmp;
			tmp += ncol;
		}
	}

        /* Now free the old buffer */
	free(oldbuf);

	return move_down ? move_down : -move_up; /* convert to rows */
}

/*
 * Writes str into buf at screen's current row and column.  Characters are set
 * to match flags.
 */
void
ScreenWrite (
	TScreen *screen,
	Char *str,
	register unsigned flags,
	register unsigned cur_fg_bg,
	register int length)		/* length of string */
{
#if OPT_ISO_COLORS
	register Char *fb = 0;
#endif
#if OPT_DEC_CHRSET
	register Char *cb = 0;
#endif
	register Char *attrs;
	register int avail  = screen->max_col - screen->cur_col + 1;
	register Char *col;
	register int wrappedbit;

	if (length > avail)
	    length = avail;
	if (length <= 0)
		return;

	col   = SCRN_BUF_CHARS(screen, screen->cur_row) + screen->cur_col;
	attrs = SCRN_BUF_ATTRS(screen, screen->cur_row) + screen->cur_col;

	if_OPT_ISO_COLORS(screen,{
		fb = SCRN_BUF_COLOR(screen, screen->cur_row) + screen->cur_col;
	})
	if_OPT_DEC_CHRSET({
		cb = SCRN_BUF_CSETS(screen, screen->cur_row) + screen->cur_col;
	})

	wrappedbit = ScrnTstWrapped(screen, screen->cur_row);

	/* write blanks if we're writing invisible text */
	if (flags & INVISIBLE) {
		memset(col, ' ', length);
	} else {
		memcpy(col, str, length);
	}

	flags &= ATTRIBUTES;
	flags |= CHARDRAWN;
	memset( attrs, flags,  length);

	if_OPT_ISO_COLORS(screen,{
		memset( fb,   cur_fg_bg, length);
	})
	if_OPT_DEC_CHRSET({
		memset( cb,   curXtermChrSet(screen->cur_row), length);
	})

	if (wrappedbit)
	    ScrnSetWrapped(screen, screen->cur_row);
	else
	    ScrnClrWrapped(screen, screen->cur_row);

	if_OPT_XMC_GLITCH(screen,{
		Resolve_XMC(screen);
	})
}

/*
 * Saves pointers to the n lines beginning at sb + where, and clears the lines
 */
static void
ScrnClearLines (TScreen *screen, ScrnBuf sb, int where, int n, int size)
{
	register int i, j;
	size_t len = ScrnPointers(screen, n);
	int last = (n * MAX_PTRS);

	/* save n lines at where */
	memcpy ( (char *) screen->save_ptr,
		 (char *) &sb[MAX_PTRS * where],
		 len);

	/* clear contents of old rows */
	if (TERM_COLOR_FLAGS) {
		int flags = TERM_COLOR_FLAGS;
		for (i = 0; i < last; i += MAX_PTRS) {
			screen->save_ptr[i] = 0;
			bzero( screen->save_ptr[i+ OFF_CHARS], size);
			memset(screen->save_ptr[i+ OFF_ATTRS], flags, size);
			memset(screen->save_ptr[i+ OFF_COLOR], xtermColorPair(), size);
		}
	} else {
		for (i = 0; i < last; i += MAX_PTRS) {
			for (j = 0; j < BUF_HEAD; j++)
				screen->save_ptr[i+j] = 0;
			for (j = BUF_HEAD; j < MAX_PTRS; j++)
				bzero (screen->save_ptr[i+j], size);
		}
	}
}

size_t
ScrnPointers (TScreen *screen, size_t len)
{
	len *= (MAX_PTRS * sizeof(Char *));

	if (len > screen->save_len) {
		if (screen->save_len)
			screen->save_ptr = (Char **)realloc(screen->save_ptr, len);
		else
			screen->save_ptr = (Char **)malloc(len);
		screen->save_len = len;
		if (screen->save_ptr == 0)
			SysError (ERROR_SAVE_PTR);
	}
	return len;
}

/*
 * Inserts n blank lines at sb + where, treating last as a bottom margin.
 * Size is the size of each entry in sb.
 * Requires: 0 <= where < where + n <= last
 */
void
ScrnInsertLine (
	TScreen *screen,
	register ScrnBuf sb,
	int last,
	register int where,
	register int n,
	register int size)
{
	size_t len = ScrnPointers(screen, n);

	/* save n lines at bottom */
	ScrnClearLines(screen, sb, (last -= n - 1), n, size);

	/*
	 * WARNING, overlapping copy operation.  Move down lines (pointers).
	 *
	 *   +----|---------|--------+
	 *
	 * is copied in the array to:
	 *
	 *   +--------|---------|----+
	 */
	memmove( (char *) &sb [MAX_PTRS * (where + n)],
		 (char *) &sb [MAX_PTRS * where],
		 MAX_PTRS * sizeof (char *) * (last - where));

	/* reuse storage for new lines at where */
	memcpy ( (char *) &sb[MAX_PTRS * where],
		 (char *) screen->save_ptr,
		 len);
}

/*
 * Deletes n lines at sb + where, treating last as a bottom margin.
 * Size is the size of each entry in sb.
 * Requires 0 <= where < where + n < = last
 */
void
ScrnDeleteLine (
	TScreen *screen,
	register ScrnBuf sb,
	register int last,
	int where,
	register int n,
	register int size)
{
	ScrnClearLines(screen, sb, where, n, size);

	/* move up lines */
	memmove( (char *) &sb[MAX_PTRS * where],
		 (char *) &sb[MAX_PTRS * (where + n)],
		MAX_PTRS * sizeof (char *) * ((last -= n - 1) - where));

	/* reuse storage for new bottom lines */
	memcpy ( (char *) &sb[MAX_PTRS * last],
		 (char *)screen->save_ptr,
		MAX_PTRS * sizeof(char *) * n);
}

/*
 * Inserts n blanks in screen at current row, col.  Size is the size of each
 * row.
 */
void
ScrnInsertChar (
	register TScreen *screen,
	register int n,
	int size)
{
	ScrnBuf sb = screen->visbuf;
	int row = screen->cur_row;
	int col = screen->cur_col;
	register int i, j;
	register Char *ptr = BUF_CHARS(sb, row);
	register Char *attrs = BUF_ATTRS(sb, row);
	register size_t nbytes = (size - n - col);
	int wrappedbit = ScrnTstWrapped(screen, row);
	int flags = CHARDRAWN | TERM_COLOR_FLAGS;

	ScrnClrWrapped(screen, row); /* make sure the bit isn't moved */
	for (i = size - 1; i >= col + n; i--) {
		ptr[i] = ptr[j = i - n];
		attrs[i] = attrs[j];
	}

	for (i=col; i<col+n; i++)
	    ptr[i] = ' ';
	for (i=col; i<col+n; i++)
	    attrs[i] = flags;
	if_OPT_ISO_COLORS(screen,{
	    Char *colors = BUF_COLOR(sb, row);
	    memmove(colors + col + n, colors + col, nbytes);
	    memset(colors + col, xtermColorPair(), n);
	})
	if_OPT_DEC_CHRSET({
	    Char *csets = _BUF_CSETS(sb, row);
	    memmove(csets + col + n, csets + col, nbytes);
	    memset(csets + col, curXtermChrSet(row), n);
	})

	if (wrappedbit)
	    ScrnSetWrapped(screen, row);
	else
	    ScrnClrWrapped(screen, row);
}

/*
 * Deletes n characters at current row, col. Size is the size of each row.
 */
void
ScrnDeleteChar (
	register TScreen *screen,
	register int n,
	register int size)
{
	ScrnBuf sb = screen->visbuf;
	int row = screen->cur_row;
	int col = screen->cur_col;
	register Char *ptr = BUF_CHARS(sb, row);
	register Char *attrs = BUF_ATTRS(sb, row);
	register size_t nbytes = (size - n - col);

	memmove (ptr   + col, ptr   + col + n, nbytes);
	memmove (attrs + col, attrs + col + n, nbytes);
	bzero  (ptr + size - n, n);
	memset (attrs + size - n, TERM_COLOR_FLAGS, n);

	if_OPT_ISO_COLORS(screen,{
	    Char *colors = BUF_COLOR(sb, row);
	    memmove(colors + col, colors + col + n, nbytes);
	    memset(colors + size - n, xtermColorPair(), n);
	})
	if_OPT_DEC_CHRSET({
	    Char *csets = _BUF_CSETS(sb, row);
	    memmove(csets + col, csets + col + n, nbytes);
	    memset(csets + size - n, curXtermChrSet(row), n);
	})
	ScrnClrWrapped(screen, row);
}

/*
 * Repaints the area enclosed by the parameters.
 * Requires: (toprow, leftcol), (toprow + nrows, leftcol + ncols) are
 * 	     coordinates of characters in screen;
 *	     nrows and ncols positive.
 *	     all dimensions are based on single-characters.
 */
void
ScrnRefresh (
	register TScreen *screen,
	int toprow,
	int leftcol,
	int nrows,
	int ncols,
	Bool force)			/* ... leading/trailing spaces */
{
	int y = toprow * FontHeight(screen) + screen->border;
	register int row;
	register int topline = screen->topline;
	int maxrow = toprow + nrows - 1;
	int scrollamt = screen->scroll_amt;
	int max = screen->max_row;
	int gc_changes = 0;
#ifdef __CYGWIN__
	static char first_time = 1;
#endif

	TRACE(("ScrnRefresh (%d,%d) - (%d,%d)%s\n",
		toprow, leftcol,
		nrows, ncols,
		force ? " force" : ""))

	if(screen->cursor_col >= leftcol
	&& screen->cursor_col <= (leftcol + ncols - 1)
	&& screen->cursor_row >= toprow + topline
	&& screen->cursor_row <= maxrow + topline)
		screen->cursor_state = OFF;

	for (row = toprow; row <= maxrow; y += FontHeight(screen), row++) {
#if OPT_ISO_COLORS
	   register Char *fb = 0;
#endif
#if OPT_DEC_CHRSET
	   register Char *cb = 0;
#endif
	   Char cs = 0;
	   register Char *chars;
	   register Char *attrs;
	   register int col = leftcol;
	   int maxcol = leftcol + ncols - 1;
	   int hi_col = maxcol;
	   int lastind;
	   int flags;
	   int fg_bg = 0, fg = 0, bg = 0;
	   int x;
	   GC gc;
	   Boolean hilite;

	   if (row < screen->top_marg || row > screen->bot_marg)
		lastind = row;
	   else
		lastind = row - scrollamt;

	   if (lastind < 0 || lastind > max)
	   	continue;

	   chars = SCRN_BUF_CHARS(screen, lastind + topline);
	   attrs = SCRN_BUF_ATTRS(screen, lastind + topline);

	   if_OPT_DEC_CHRSET({
		cb = SCRN_BUF_CSETS(screen, lastind + topline);
	   })

	   if (row < screen->startHRow || row > screen->endHRow ||
	       (row == screen->startHRow && maxcol < screen->startHCol) ||
	       (row == screen->endHRow && col >= screen->endHCol))
	   {
#if OPT_DEC_CHRSET
	       /*
	        * Temporarily change dimensions to double-sized characters so
		* we can reuse the recursion on this function.
	        */
	       if (CSET_DOUBLE(*cb)) {
		   col /= 2;
		   maxcol /= 2;
	       }
#endif
	       /* row does not intersect selection; don't hilite */
	       if (!force) {
		   while (col <= maxcol && (attrs[col] & ~BOLD) == 0 &&
			  (chars[col] & ~040) == 0)
		       col++;

		   while (col <= maxcol && (attrs[maxcol] & ~BOLD) == 0 &&
			  (chars[maxcol] & ~040) == 0)
		       maxcol--;
	       }
#if OPT_DEC_CHRSET
	       if (CSET_DOUBLE(*cb)) {
		   col *= 2;
		   maxcol *= 2;
	       }
#endif
	       hilite = False;
	   }
	   else {
	       /* row intersects selection; split into pieces of single type */
	       if (row == screen->startHRow && col < screen->startHCol) {
		   ScrnRefresh(screen, row, col, 1, screen->startHCol - col,
			       force);
		   col = screen->startHCol;
	       }
	       if (row == screen->endHRow && maxcol >= screen->endHCol) {
		   ScrnRefresh(screen, row, screen->endHCol, 1,
			       maxcol - screen->endHCol + 1, force);
		   maxcol = screen->endHCol - 1;
	       }

	       /*
		* If we're highlighting because the user is doing cut/paste,
		* trim the trailing blanks from the highlighted region so we're
		* showing the actual extent of the text that'll be cut.  If
		* we're selecting a blank line, we'll highlight one column
		* anyway.
		*
		* We don't do this if the mouse-hilite mode is set because that
		* would be too confusing.
		*
		* The default if the highlightSelection resource isn't set will
		* highlight the whole width of the terminal, which is easy to
		* see, but harder to use (because trailing blanks aren't as
		* apparent).
	        */
	       if (screen->highlight_selection
		&& screen->send_mouse_pos != VT200_HIGHLIGHT_MOUSE) {
		   hi_col = screen->max_col;
	           while (hi_col > 0 && !(attrs[hi_col] & CHARDRAWN))
                       hi_col--;
	       }

	       /* remaining piece should be hilited */
	       hilite = True;
	   }

	   if (col > maxcol) continue;

	   /*
	    * Go back to double-sized character dimensions if the line has
	    * double-width characters.  Note that 'hi_col' is already in the
	    * right units.
	    */
	   if_OPT_DEC_CHRSET({
		if (CSET_DOUBLE(*cb)) {
			col /= 2;
			maxcol /= 2;
		}
		cs = cb[col];
	   })

	   flags = attrs[col];
	   if_OPT_ISO_COLORS(screen,{
		fb = SCRN_BUF_COLOR(screen, lastind + topline);
		fg_bg = fb[col];
		fg = extract_fg(fg_bg, flags);
		bg = extract_bg(fg_bg);
	   })
	   gc = updatedXtermGC(screen, flags, fg_bg, hilite);
	   gc_changes |= (flags & (FG_COLOR|BG_COLOR));

	   x = CurCursorX(screen, row, col);
	   lastind = col;

	   for (; col <= maxcol; col++) {
		if ((attrs[col] != flags)
		 || (hilite && (col > hi_col))
#if OPT_ISO_COLORS
		 || ((flags & FG_COLOR) && (extract_fg(fb[col],attrs[col]) != fg))
		 || ((flags & BG_COLOR) && (extract_bg(fb[col]) != bg))
#endif
#if OPT_DEC_CHRSET
		 || (cb[col] != cs)
#endif
		 ) {
		   TRACE(("%s @%d, calling drawXtermText %d..%d\n", __FILE__, __LINE__, lastind, col))
		   x = drawXtermText(screen, flags, gc, x, y,
		   	cs,
			&chars[lastind], col - lastind);
		   resetXtermGC(screen, flags, hilite);

		   lastind = col;

		   if (hilite && (col > hi_col))
			hilite = False;

		   flags = attrs[col];
		   if_OPT_ISO_COLORS(screen,{
			fg_bg = fb[col];
		        fg = extract_fg(fg_bg, flags);
		        bg = extract_bg(fg_bg);
		   })
		   if_OPT_DEC_CHRSET({
		        cs = cb[col];
		   })
	   	   gc = updatedXtermGC(screen, flags, fg_bg, hilite);
	   	   gc_changes |= (flags & (FG_COLOR|BG_COLOR));
		}

		if(chars[col] == 0)
			chars[col] = ' ';
	   }

	   TRACE(("%s @%d, calling drawXtermText %d..%d\n", __FILE__, __LINE__, lastind, col))
	   drawXtermText(screen, flags, gc, x, y,
	   	cs,
		&chars[lastind], col - lastind);
	   resetXtermGC(screen, flags, hilite);
	}

	/*
	 * If we're in color mode, reset the various GC's to the current
	 * screen foreground and background so that other functions (e.g.,
	 * ClearRight) will get the correct colors.
	 */
	if_OPT_ISO_COLORS(screen,{
	    if (gc_changes & FG_COLOR)
		SGR_Foreground(term->cur_foreground);
	    if (gc_changes & BG_COLOR)
		SGR_Background(term->cur_background);
	})

#if defined(__CYGWIN__) && defined(TIOCSWINSZ)
	if (first_time == 1) {
		struct winsize ws;

		first_time = 0;
		ws.ws_row = nrows;
		ws.ws_col = ncols;
		ws.ws_xpixel = term->core.width;
		ws.ws_ypixel = term->core.height;
		ioctl (screen->respond, TIOCSWINSZ, (char *)&ws);
	}
#endif
}

/*
 * Sets the rows first though last of the buffer of screen to spaces.
 * Requires first <= last; first, last are rows of screen->buf.
 */
void
ClearBufRows (
	register TScreen *screen,
	register int first,
	register int last)
{
	ScrnBuf	buf = screen->visbuf;
	int	len = screen->max_col + 1;
	register int row;
	register int flags = TERM_COLOR_FLAGS;

	TRACE(("ClearBufRows %d..%d\n", first, last))
	for (row = first; row <= last; row++) {
	    ScrnClrWrapped(screen, row);
	    bzero (BUF_CHARS(buf, row), len);
	    memset(BUF_ATTRS(buf, row), flags, len);
	    if_OPT_ISO_COLORS(screen,{
		memset(BUF_COLOR(buf, row), xtermColorPair(), len);
	    })
	    if_OPT_DEC_CHRSET({
		memset(_BUF_CSETS(buf, row), 0, len);
	    })
	}
}

/*
  Resizes screen:
  1. If new window would have fractional characters, sets window size so as to
  discard fractional characters and returns -1.
  Minimum screen size is 1 X 1.
  Note that this causes another ExposeWindow event.
  2. Enlarges screen->buf if necessary.  New space is appended to the bottom
  and to the right
  3. Reduces  screen->buf if necessary.  Old space is removed from the bottom
  and from the right
  4. Cursor is positioned as closely to its former position as possible
  5. Sets screen->max_row and screen->max_col to reflect new size
  6. Maintains the inner border (and clears the border on the screen).
  7. Clears origin mode and sets scrolling region to be entire screen.
  8. Returns 0
  */
int
ScreenResize (
	register TScreen *screen,
	int width,
	int height,
	unsigned *flags)
{
	int code;
	int rows, cols;
	int border = 2 * screen->border;
	int move_down_by;
#if defined(TIOCSSIZE) && (defined(sun) && !defined(SVR4))
	struct ttysize ts;
#elif defined(TIOCSWINSZ)
	struct winsize ws;
#endif	/* sun vs TIOCSWINSZ */
	Window tw = TextWindow (screen);

	TRACE(("ScreenResize %dx%d\n", height, width))

	/* clear the right and bottom internal border because of NorthWest
	   gravity might have left junk on the right and bottom edges */
	if (width >= FullWidth(screen)) {
		XClearArea (screen->display, tw,
			    FullWidth(screen), 0,             /* right edge */
			    0, height,                /* from top to bottom */
			    False);
	}
	if (height >= FullHeight(screen)) {
		XClearArea (screen->display, tw,
		    	0, FullHeight(screen),	                  /* bottom */
		    	width, 0,                  /* all across the bottom */
		    	False);
	}

	/* round so that it is unlikely the screen will change size on  */
	/* small mouse movements.					*/
	rows = (height + FontHeight(screen) / 2 - border) /
	 FontHeight(screen);
	cols = (width + FontWidth(screen) / 2 - border - Scrollbar(screen)) /
	 FontWidth(screen);
	if (rows < 1) rows = 1;
	if (cols < 1) cols = 1;

	/* update buffers if the screen has changed size */
	if (screen->max_row != rows - 1 || screen->max_col != cols - 1) {
		register int savelines = screen->scrollWidget ?
		 screen->savelines : 0;
		int delta_rows = rows - (screen->max_row + 1);

		if(screen->cursor_state)
			HideCursor();
		if ( screen->alternate
		     && term->misc.resizeGravity == SouthWestGravity )
		    /* swap buffer pointers back to make all this hair work */
		    SwitchBufPtrs(screen);
		if (screen->altbuf)
		    (void) Reallocate(&screen->altbuf, &screen->abuf_address,
			 rows, cols, screen->max_row + 1, screen->max_col + 1);
		move_down_by = Reallocate(&screen->allbuf,
					  &screen->sbuf_address,
					  rows + savelines, cols,
					  screen->max_row + 1 + savelines,
					  screen->max_col + 1);
		screen->visbuf = &screen->allbuf[MAX_PTRS * savelines];

		screen->max_row += delta_rows;
		screen->max_col = cols - 1;

		if (term->misc.resizeGravity == SouthWestGravity) {
		    screen->savedlines -= move_down_by;
		    if (screen->savedlines < 0)
			screen->savedlines = 0;
		    if (screen->savedlines > screen->savelines)
			screen->savedlines = screen->savelines;
		    if (screen->topline < -screen->savedlines)
			screen->topline = -screen->savedlines;
		    screen->cur_row += move_down_by;
		    screen->cursor_row += move_down_by;
		    ScrollSelection(screen, move_down_by);

		    if (screen->alternate)
			SwitchBufPtrs(screen); /* put the pointers back */
		}

		/* adjust scrolling region */
		screen->top_marg = 0;
		screen->bot_marg = screen->max_row;
		*flags &= ~ORIGIN;

		if (screen->cur_row > screen->max_row)
			screen->cur_row = screen->max_row;
		if (screen->cur_col > screen->max_col)
			screen->cur_col = screen->max_col;

		screen->fullVwin.height = height - border;
		screen->fullVwin.width = width - border - screen->fullVwin.scrollbar;

	} else if(FullHeight(screen) == height && FullWidth(screen) == width)
	 	return(0);	/* nothing has changed at all */

	screen->fullVwin.fullheight = height;
	screen->fullVwin.fullwidth = width;

	if(screen->scrollWidget)
		ResizeScrollBar(screen);

	ResizeSelection (screen, rows, cols);

#ifndef NO_ACTIVE_ICON
	if (screen->iconVwin.window) {
	    XWindowChanges changes;
	    screen->iconVwin.width =
		(screen->max_col + 1) * screen->iconVwin.f_width;

	    screen->iconVwin.height =
		(screen->max_row + 1) * screen->iconVwin.f_height;

	    changes.width = screen->iconVwin.fullwidth =
		screen->iconVwin.width + 2 * screen->border;

	    changes.height = screen->iconVwin.fullheight =
		screen->iconVwin.height + 2 * screen->border;

	    XConfigureWindow(XtDisplay(term), screen->iconVwin.window,
			     CWWidth|CWHeight,&changes);
	}
#endif /* NO_ACTIVE_ICON */

#if defined(TIOCSSIZE) && (defined(sun) && !defined(SVR4))
	/* Set tty's idea of window size */
	ts.ts_lines = rows;
	ts.ts_cols = cols;
	code = ioctl (screen->respond, TIOCSSIZE, &ts);
	TRACE(("return %d from TIOCSSIZE %dx%d\n", code, rows, cols))
#ifdef SIGWINCH
	if(screen->pid > 1) {
		int	pgrp;

		if (ioctl (screen->respond, TIOCGPGRP, &pgrp) != -1)
			kill_process_group(pgrp, SIGWINCH);
	}
#endif	/* SIGWINCH */
#elif defined(TIOCSWINSZ)
	/* Set tty's idea of window size */
	ws.ws_row = rows;
	ws.ws_col = cols;
	ws.ws_xpixel = width;
	ws.ws_ypixel = height;
	code = ioctl (screen->respond, TIOCSWINSZ, (char *)&ws);
	TRACE(("return %d from TIOCSWINSZ %dx%d\n", code, rows, cols))
#ifdef notdef	/* change to SIGWINCH if this doesn't work for you */
	if(screen->pid > 1) {
		int	pgrp;

		if (ioctl (screen->respond, TIOCGPGRP, &pgrp) != -1)
		    kill_process_group(pgrp, SIGWINCH);
	}
#endif	/* SIGWINCH */
#else
	TRACE(("ScreenResize cannot do anything to pty\n"))
#endif	/* sun vs TIOCSWINSZ */
	return (0);
}

void
ScrnClrWrapped(TScreen *screen, int row)
{
	long value = (long)SCRN_BUF_FLAGS(screen, row + screen->topline) & ~ LINEWRAPPED;
	SCRN_BUF_FLAGS(screen, row + screen->topline) = (Char *)value;
}

void
ScrnSetWrapped(TScreen *screen, int row)
{
	long value = (long)SCRN_BUF_FLAGS(screen, row + screen->topline) | LINEWRAPPED;
	SCRN_BUF_FLAGS(screen, row + screen->topline) = (Char *)value;
}

Bool
ScrnTstWrapped(TScreen *screen, int row)
{
	return (long)SCRN_BUF_FLAGS(screen, row + screen->topline) & LINEWRAPPED;
}

Bool
non_blank_line(
	ScrnBuf sb,
	register int row,
	register int col,
	register int len)
{
	register int	i;
	register Char *ptr = BUF_CHARS(sb, row);

	for (i = col; i < len; i++) {
		if (ptr[i])
			return True;
	}
	return False;
}
