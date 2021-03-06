/*
 *	$XConsortium: ptyx.h /main/67 1996/11/29 10:34:19 swick $
 *	$XFree86: xc/programs/xterm/ptyx.h,v 3.19.2.8 1999/07/28 13:38:05 hohndel Exp $
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

#ifndef included_ptyx_h
#define included_ptyx_h 1

#ifdef HAVE_CONFIG_H
#include <xtermcfg.h>
#endif

/* ptyx.h */
/* @(#)ptyx.h	X10/6.6	11/10/86 */

#include <X11/IntrinsicP.h>
#include <X11/Xmu/Misc.h>	/* For Max() and Min(). */
#include <X11/Xfuncs.h>
#include <X11/Xosdefs.h>
#include <X11/Xmu/Converters.h>

/* adapted from IntrinsicI.h */
#define MyStackAlloc(size, stack_cache_array)     \
    ((size) <= sizeof(stack_cache_array)	  \
    ?  (XtPointer)(stack_cache_array)		  \
    :  (XtPointer)malloc((unsigned)(size)))

#define MyStackFree(pointer, stack_cache_array) \
    if ((pointer) != ((XtPointer)(stack_cache_array))) free(pointer)

#ifdef AMOEBA
/* Avoid name clashes with standard Amoeba types: */
#define event    am_event_t
#define interval am_interval_t
#include <amoeba.h>
#include <semaphore.h>
#include <circbuf.h>
#undef event
#undef interval
#endif

/* Extra Xlib definitions */
#define AllButtonsUp(detail, ignore)  (\
		((ignore) == Button1) ? \
				(((detail)&(Button2Mask|Button3Mask)) == 0) \
				: \
		 (((ignore) == Button2) ? \
		  		(((detail)&(Button1Mask|Button3Mask)) == 0) \
				: \
		  		(((detail)&(Button1Mask|Button2Mask)) == 0)) \
		)

/*
** System V definitions
*/

#ifdef SYSV
#ifdef X_NOT_POSIX
#ifndef CRAY
#define	dup2(fd1,fd2)	((fd1 == fd2) ? fd1 : \
				(close(fd2), fcntl(fd1, F_DUPFD, fd2)))
#endif
#endif
#endif /* SYSV */

/*
** allow for mobility of the pty master/slave directories
*/
#ifndef PTYDEV
#ifdef __hpux 
#define	PTYDEV		"/dev/ptym/ptyxx"
#else	/* !__hpux */ 
#ifndef __osf__
#define	PTYDEV		"/dev/ptyxx"
#endif
#endif	/* !__hpux */ 
#endif	/* !PTYDEV */

#ifndef TTYDEV
#ifdef __hpux 
#define TTYDEV		"/dev/pty/ttyxx"
#else	/* !__hpux */ 
#if defined(__osf__) || (defined(linux) && (__GLIBC__ >= 2) && (__GLIBC_MINOR__ >= 1))
#define TTYDEV		"/dev/ttydirs/xxx/xxxxxxxxxxxxxx"
#else
#define	TTYDEV		"/dev/ttyxx"
#endif
#endif	/* !__hpux */ 
#endif	/* !TTYDEV */

#ifndef PTYCHAR1
#ifdef __hpux 
#define PTYCHAR1	"zyxwvutsrqp"
#else	/* !__hpux */ 
#ifdef __EMX__
#define PTYCHAR1	"pq"
#else
#define	PTYCHAR1	"pqrstuvwxyzPQRSTUVWXYZ"
#endif  /* !__EMX__ */
#endif	/* !__hpux */ 
#endif	/* !PTYCHAR1 */

#ifndef PTYCHAR2
#ifdef __hpux 
#define	PTYCHAR2	"fedcba9876543210"
#else	/* !__hpux */ 
#ifdef __FreeBSD__
#define	PTYCHAR2	"0123456789abcdefghijklmnopqrstuv"
#else /* !__FreeBSD__ */
#define	PTYCHAR2	"0123456789abcdef"
#endif /* !__FreeBSD__ */
#endif	/* !__hpux */ 
#endif	/* !PTYCHAR2 */

#ifndef TTYFORMAT
#ifdef CRAY
#define TTYFORMAT "/dev/ttyp%03d"
#else
#define TTYFORMAT "/dev/ttyp%d"
#endif
#endif

#ifndef PTYFORMAT
#ifdef CRAY
#define PTYFORMAT "/dev/pty/%03d"
#else
#define PTYFORMAT "/dev/ptyp%d"
#endif
#endif

#ifndef MAXPTTYS
#ifdef CRAY
#define MAXPTTYS 256
#else
#define MAXPTTYS 2048
#endif
#endif

/* Until the translation manager comes along, I have to do my own translation of
 * mouse events into the proper routines. */

typedef enum {NORMAL, LEFTEXTENSION, RIGHTEXTENSION} EventMode;

/*
 * The origin of a screen is 0, 0.  Therefore, the number of rows
 * on a screen is screen->max_row + 1, and similarly for columns.
 */

typedef unsigned char Char;		/* to support 8 bit chars */
typedef Char **ScrnBuf;

/*
 * ANSI emulation.
 */
#define INQ	0x05
#define BEL	0x07
#define	FF	0x0C			/* C0, C1 control names		*/
#define	LS1	0x0E
#define	LS0	0x0F
#define	NAK	0x15
#define	CAN	0x18
#define	SUB	0x1A
#define	ESC	0x1B
#define US	0x1F
#define	DEL	0x7F
#define HTS     ('H'+0x40)
#define	RI	0x8D
#define	SS2	0x8E
#define	SS3	0x8F
#define	DCS	0x90
#define	SPA	0x96
#define	EPA	0x97
#define	SOS	0x98
#define	OLDID	0x9A			/* ESC Z			*/
#define	CSI	0x9B
#define	ST	0x9C
#define	OSC	0x9D
#define	PM	0x9E
#define	APC	0x9F
#define	RDEL	0xFF

#define MIN_DECID  52			/* can emulate VT52 */
#define MAX_DECID 420			/* ...through VT420 */

#ifndef DFT_DECID
#define DFT_DECID 100			/* default VT100 */
#endif

#define NMENUFONTS 9			/* entries in fontMenu */

#define	NBOX	5			/* Number of Points in box	*/
#define	NPARAM	10			/* Max. parameters		*/

#define	MINHILITE	32

typedef struct {
	unsigned char	a_type;
	unsigned char	a_pintro;
	unsigned char	a_final;
	unsigned char	a_inters;
	char	a_nparam;		/* # of parameters		*/
	char	a_dflt[NPARAM];		/* Default value flags		*/
	short	a_param[NPARAM];	/* Parameters			*/
	char	a_nastyf;		/* Error flag			*/
} ANSI;

#define TEK_FONT_LARGE 0
#define TEK_FONT_2 1
#define TEK_FONT_3 2
#define TEK_FONT_SMALL 3
#define	TEKNUMFONTS 4

/* Actually there are 5 types of lines, but four are non-solid lines */
#define	TEKNUMLINES	4

typedef struct {
	int	x;
	int	y;
	int	fontsize;
	int	linetype;
} Tmodes;

typedef struct {
	int Twidth;
	int Theight;
} T_fontsize;

typedef struct {
	short *bits;
	int x;
	int y;
	int width;
	int height;
} BitmapBits;

#define	SAVELINES		64      /* default # lines to save      */
#define SCROLLLINES 1			/* default # lines to scroll    */

/***====================================================================***/

#define	TEXT_FG		0
#define	TEXT_BG		1
#define	TEXT_CURSOR	2
#define	MOUSE_FG	3
#define	MOUSE_BG	4
#define	TEK_FG		5
#define	TEK_BG		6
#define	HIGHLIGHT_BG	7
#define	NCOLORS		8

#define EXCHANGE(a,b,tmp) tmp = a; a = b; b = tmp;

#define	COLOR_DEFINED(s,w)	((s)->which&(1<<(w)))
#define	COLOR_VALUE(s,w)	((s)->colors[w])
#define	SET_COLOR_VALUE(s,w,v)	(((s)->colors[w]=(v)),((s)->which|=(1<<(w))))

#define	COLOR_NAME(s,w)		((s)->names[w])
#define	SET_COLOR_NAME(s,w,v)	(((s)->names[w]=(v)),((s)->which|=(1<<(w))))

#define	UNDEFINE_COLOR(s,w)	((s)->which&=(~((w)<<1)))
#define	OPPOSITE_COLOR(n)	(((n)==TEXT_FG?TEXT_BG:\
				 ((n)==TEXT_BG?TEXT_FG:\
				 ((n)==MOUSE_FG?MOUSE_BG:\
				 ((n)==MOUSE_BG?MOUSE_FG:\
				 ((n)==TEK_FG?TEK_BG:\
				 ((n)==TEXT_BG?TEK_FG:(n))))))))

typedef struct {
	unsigned	which;	/* must have NCOLORS bits */
	Pixel		colors[NCOLORS];
	char		*names[NCOLORS];
} ScrnColors;

/***====================================================================***/

#if (XtSpecificationRelease < 6)
#ifndef NO_ACTIVE_ICON
#define NO_ACTIVE_ICON 1 /* Note: code relies on an X11R6 function */
#endif
#endif

#ifndef OPT_AIX_COLORS
#define OPT_AIX_COLORS  1 /* true if xterm is configured with AIX (16) colors */
#endif

#define OPT_BLINK_CURS  0 /* FIXME: do this later (96/7/31) */

#ifndef OPT_DEC_CHRSET
#define OPT_DEC_CHRSET  1 /* true if xterm is configured for DEC charset */
#endif

#ifndef OPT_I18N_SUPPORT
#if (XtSpecificationRelease >= 6)
#define OPT_I18N_SUPPORT 1 /* true if xterm uses internationalization support */
#else
#define OPT_I18N_SUPPORT 0
#endif
#endif

#ifndef OPT_INITIAL_ERASE
#define OPT_INITIAL_ERASE 1 /* use pty's erase character if it's not 128 */
#endif

#ifndef OPT_INPUT_METHOD
#if (XtSpecificationRelease >= 6)
#define OPT_INPUT_METHOD 1 /* true if xterm uses input-method support */
#else
#define OPT_INPUT_METHOD 0
#endif
#endif

#ifndef OPT_ISO_COLORS
#define OPT_ISO_COLORS  1 /* true if xterm is configured with ISO colors */
#endif

#ifndef OPT_HIGHLIGHT_COLOR
#define OPT_HIGHLIGHT_COLOR 1 /* true if xterm supports color highlighting */
#endif

#ifndef OPT_SAME_NAME
#define OPT_SAME_NAME   1 /* suppress redundant updates of title, icon, etc. */
#endif

#ifndef OPT_PC_COLORS
#define OPT_PC_COLORS   1 /* true if xterm supports PC-style (bold) colors */
#endif

#ifndef OPT_PRINT_COLORS
#define OPT_PRINT_COLORS 1 /* true if we print color information */
#endif

#ifndef OPT_SUNPC_KBD
#define OPT_SUNPC_KBD	1 /* true if xterm supports Sun/PC keyboard map */
#endif

#ifndef OPT_TEK4014
#define OPT_TEK4014     1 /* true if we're using tek4014 emulation */
#endif

#ifndef OPT_TRACE
#define OPT_TRACE       0 /* true if we're using debugging traces */
#endif

#ifndef OPT_VT52_MODE
#define OPT_VT52_MODE   1 /* true if xterm supports VT52 emulation */
#endif

#ifndef OPT_XMC_GLITCH
#define OPT_XMC_GLITCH	0 /* true if xterm supports xmc (magic cookie glitch) */
#endif

#ifndef OPT_ZICONBEEP
#define OPT_ZICONBEEP   1 /* true if xterm supports "-ziconbeep" option */
#endif

/***====================================================================***/

#if OPT_AIX_COLORS && !OPT_ISO_COLORS
fixme: You must have ANSI/ISO colors to support AIX colors
#endif

/***====================================================================***/

#if OPT_ISO_COLORS
#define if_OPT_ISO_COLORS(screen, code) if(screen->colorMode) code
#define TERM_COLOR_FLAGS (term->flags & (FG_COLOR|BG_COLOR))
#define MAXCOLORS 19
#define COLOR_0		0
#define COLOR_1		1
#define COLOR_2		2
#define COLOR_3		3
#define COLOR_4		4
#define COLOR_5		5
#define COLOR_6		6
#define COLOR_7		7
#define COLOR_8		8
#define COLOR_9		9
#define COLOR_10	10
#define COLOR_11	11
#define COLOR_12	12
#define COLOR_13	13
#define COLOR_14	14
#define COLOR_15	15
#define COLOR_BD	16	/* BOLD */
#define COLOR_UL	17	/* UNDERLINE */
#define COLOR_BL	18	/* BLINK */
#ifndef DFT_COLORMODE
#define DFT_COLORMODE TRUE	/* default colorMode resource */
#endif
#else
#define if_OPT_ISO_COLORS(screen, code) /* nothing */
#define TERM_COLOR_FLAGS 0
#endif	/* OPT_ISO_COLORS */

#if OPT_AIX_COLORS
#define if_OPT_AIX_COLORS(screen, code) if(screen->colorMode) code
#else
#define if_OPT_AIX_COLORS(screen, code) /* nothing */
#endif

/***====================================================================***/

#if OPT_DEC_CHRSET
#define if_OPT_DEC_CHRSET(code) code
	/* Use 3 bits for encoding the double high/wide sense of characters */
#define CSET_SWL        0
#define CSET_DHL_TOP    1
#define CSET_DHL_BOT    2
#define CSET_DWL        4
	/* Use remaining bits for encoding the other character-sets */
#define CSET_NORMAL(code)  ((code) == CSET_SWL)
#define CSET_DOUBLE(code)  (!CSET_NORMAL(code) && !CSET_EXTEND(code))
#define CSET_EXTEND(code)  ((code) >= 8)
#define CurMaxCol(screen, row) \
	(CSET_DOUBLE(SCRN_BUF_CSETS(screen, row)[0]) \
	? (screen->max_col / 2) \
	: (screen->max_col))
#define CurCursorX(screen, row, col) \
	(CSET_DOUBLE(SCRN_BUF_CSETS(screen, row)[0]) \
	? CursorX(screen, 2*(col)) \
	: CursorX(screen, (col)))
#define CurFontWidth(screen, row) \
	(CSET_DOUBLE(SCRN_BUF_CSETS(screen, row)[0]) \
	? 2*FontWidth(screen) \
	: FontWidth(screen))
#else
#define if_OPT_DEC_CHRSET(code) /*nothing*/
#define CurMaxCol(screen, row) screen->max_col
#define CurCursorX(screen, row, col) CursorX(screen, col)
#define CurFontWidth(screen, row) FontWidth(screen)
#endif

	/* the number of pointers per row in 'ScrnBuf' */
#if OPT_ISO_COLORS || OPT_DEC_CHRSET
#define MAX_PTRS term->num_ptrs
#else
#define MAX_PTRS 3
#endif

#define BUF_HEAD 1
	/* the number that point to Char data */
#define BUF_PTRS (MAX_PTRS - BUF_HEAD)

/***====================================================================***/

#if OPT_TEK4014
#define TEK4014_ACTIVE(screen) ((screen)->TekEmu)
#define CURRENT_EMU_VAL(screen,tek,vt) (TEK4014_ACTIVE(screen) ? tek : vt)
#define CURRENT_EMU(screen) CURRENT_EMU_VAL(screen, (Widget)tekWidget, (Widget)term)
#else
#define TEK4014_ACTIVE(screen) 0
#define CURRENT_EMU_VAL(screen,tek,vt) (vt)
#define CURRENT_EMU(screen) ((Widget)term)
#endif

/***====================================================================***/

#if OPT_TRACE
#include <trace.h>
#else
#define TRACE(p) /*nothing*/
#define TRACE_CHILD /*nothing*/
#endif

/***====================================================================***/

#if OPT_VT52_MODE
#define if_OPT_VT52_MODE(screen, code) if(screen->ansi_level == 0) code
#else
#define if_OPT_VT52_MODE(screen, code) /* nothing */
#endif

/***====================================================================***/

#if OPT_XMC_GLITCH
#define if_OPT_XMC_GLITCH(screen, code) if(screen->xmc_glitch) code
#define XMC_GLITCH 1	/* the character we'll show */
#define XMC_FLAGS (INVERSE|UNDERLINE|BOLD)
#else
#define if_OPT_XMC_GLITCH(screen, code) /* nothing */
#endif

/***====================================================================***/

#define OFF_CHARS (BUF_HEAD + 0)
#define OFF_ATTRS (BUF_HEAD + 1)
#define OFF_COLOR (BUF_HEAD + 2)
#define OFF_CSETS (BUF_HEAD + 3)

	/* ScrnBuf-level macros */
#define BUF_FLAGS(buf, row) (buf[MAX_PTRS * (row) + 0])
#define BUF_CHARS(buf, row) (buf[MAX_PTRS * (row) + OFF_CHARS])
#define BUF_ATTRS(buf, row) (buf[MAX_PTRS * (row) + OFF_ATTRS])

#if OPT_ISO_COLORS
#define BUF_COLOR(buf, row) (buf[MAX_PTRS * (row) + OFF_COLOR])
#endif

#if OPT_DEC_CHRSET
#define _BUF_CSETS(buf, row) (buf[MAX_PTRS * (row) + OFF_CSETS])
#endif

	/* TScreen-level macros */
#define SCRN_BUF_FLAGS(screen, row) BUF_FLAGS(screen->visbuf, row)
#define SCRN_BUF_CHARS(screen, row) BUF_CHARS(screen->visbuf, row)
#define SCRN_BUF_ATTRS(screen, row) BUF_ATTRS(screen->visbuf, row)

#if OPT_ISO_COLORS
#define SCRN_BUF_COLOR(screen, row) BUF_COLOR(screen->visbuf, row)
#endif

#if OPT_DEC_CHRSET
#define SCRN_BUF_CSETS(screen, row) _BUF_CSETS(screen->visbuf, row)
#endif

	/* indices into save_modes[] */
typedef enum {
	DP_DECCKM,
	DP_DECANM,
	DP_DECCOLM,	/* IN132COLUMNS */
	DP_DECSCLM,
	DP_DECSCNM,
	DP_DECOM,
	DP_DECAWM,
	DP_DECARM,
	DP_X_X10MSE,
	DP_DECPFF,
	DP_DECPEX,
	DP_DECTCEM,
	DP_DECTEK,
	DP_X_DECCOLM,
	DP_X_MORE,
	DP_X_MARGIN,
	DP_X_REVWRAP,
	DP_X_LOGGING,
	DP_X_ALTSCRN,
	DP_DECBKM,
	DP_X_MOUSE,
	DP_LAST
	} SaveModes;

#define DoSM(code,value) screen->save_modes[code] = value
#define DoRM(code,value) value = screen->save_modes[code]

typedef struct {
	Boolean		saved;
	int		row;
	int		col;
	unsigned	flags;		/* VTxxx saves graphics rendition */
	char		curgl;
	char		curgr;
	char		gsets[4];
#if OPT_ISO_COLORS
	int		cur_foreground; /* current foreground color	*/
	int		cur_background; /* current background color	*/
	int		sgr_foreground; /* current SGR foreground color */
#endif
} SavedCursor;

struct _vtwin {
	Window	window;			/* X window id			*/
	int	width;			/* width of columns		*/
	int	height;			/* height of rows		*/
	int	fullwidth;		/* full width of window		*/
	int	fullheight;		/* full height of window	*/
	int	f_width;		/* width of fonts in pixels	*/
	int	f_height;		/* height of fonts in pixels	*/
	int	scrollbar;		/* if > 0, width of scrollbar, and
						scrollbar is showing	*/
	GC	normalGC;		/* normal painting		*/
	GC	reverseGC;		/* reverse painting		*/
	GC	normalboldGC;		/* normal painting, bold font	*/
	GC	reverseboldGC;		/* reverse painting, bold font	*/
};

typedef struct {
/* These parameters apply to both windows */
	Display		*display;	/* X display for screen		*/
	int		respond;	/* socket for responses
					   (position report, etc.)	*/
#ifdef AMOEBA
	capability      proccap;        /* process capability           */
	struct circbuf  *tty_inq;       /* tty server input queue       */
	struct circbuf  *tty_outq;      /* tty server output queue      */
#endif
	long		pid;		/* pid of process on far side   */
	int		uid;		/* user id of actual person	*/
	int		gid;		/* group id of actual person	*/
	GC		cursorGC;	/* normal cursor painting	*/
	GC		fillCursorGC;	/* special cursor painting	*/
	GC		reversecursorGC;/* reverse cursor painting	*/
	GC		cursoroutlineGC;/* for painting lines around    */
	Pixel		foreground;	/* foreground color		*/
	Pixel		cursorcolor;	/* Cursor color			*/
	Pixel		mousecolor;	/* Mouse color			*/
	Pixel		mousecolorback;	/* Mouse color background	*/
#if OPT_ISO_COLORS
	Pixel		Acolors[MAXCOLORS]; /* ANSI color emulation	*/
	Boolean		boldColors;	/* can we make bold colors?	*/
	Boolean		colorMode;	/* are we using color mode?	*/
	Boolean		colorULMode;	/* use color for underline?	*/
	Boolean		colorBDMode;	/* use color for bold?		*/
	Boolean		colorBLMode;	/* use color for blink?		*/
	Boolean		colorAttrMode;	/* prefer colorUL/BD to SGR	*/
#endif
#if OPT_HIGHLIGHT_COLOR
	Pixel		highlightcolor;	/* Highlight background color	*/
#endif
#if OPT_DEC_CHRSET
	Char		chrset;		/* character-set index & code	*/
#endif
	int		border;		/* inner border			*/
	Cursor		arrow;		/* arrow cursor			*/
	unsigned long	event_mask;
	unsigned short	send_mouse_pos;	/* user wants mouse transition  */
					/* and position information	*/
	int		mouse_button;	/* current button pressed	*/
	int		mouse_row;	/* ...and its row		*/
	int		mouse_col;	/* ...and its column		*/
	int		select;		/* xterm selected		*/
	Boolean		visualbell;	/* visual bell mode		*/
	Boolean		allowSendEvents;/* SendEvent mode		*/
	Boolean		awaitInput;	/* select-timeout mode		*/
	Boolean		grabbedKbd;	/* keyboard is grabbed		*/
#ifdef ALLOWLOGGING
	int		logging;	/* logging mode			*/
	int		logfd;		/* file descriptor of log	*/
	char		*logfile;	/* log file name		*/
	unsigned char	*logstart;	/* current start of log buffer	*/
#endif
	int		inhibit;	/* flags for inhibiting changes	*/

/* VT window parameters */
	Boolean		Vshow;		/* VT window showing		*/
	struct _vtwin fullVwin;
#ifndef NO_ACTIVE_ICON
	struct _vtwin iconVwin;
	struct _vtwin *whichVwin;
#endif /* NO_ACTIVE_ICON */
	Cursor pointer_cursor;		/* pointer cursor in window	*/

	String	printer_command;	/* pipe/shell command string	*/
	Boolean printer_autoclose;	/* close printer when offline	*/
	Boolean printer_extent;		/* print complete page		*/
	Boolean printer_formfeed;	/* print formfeed per function	*/
	int	printer_controlmode;	/* 0=off, 1=auto, 2=controller	*/
#ifdef OPT_PRINT_COLORS
	int	print_attributes;	/* 0=off, 1=normal, 2=color	*/
#endif

	Boolean		fnt_prop;	/* true if proportional fonts	*/
	XFontStruct	*fnt_norm;	/* normal font of terminal	*/
	XFontStruct	*fnt_bold;	/* bold font of terminal	*/
#ifndef NO_ACTIVE_ICON
	XFontStruct	*fnt_icon;	/* icon font */
#endif /* NO_ACTIVE_ICON */
	int		enbolden;	/* overstrike for bold font	*/
	XPoint		*box;		/* draw unselected cursor	*/

	int		cursor_state;	/* ON, OFF, or BLINKED_OFF	*/
#if OPT_BLINK_CURS
	int		cursor_blink;	/* blink-rate (0=off) msecs	*/
	XtIntervalId	cursor_timer;	/* timer-id for cursor-proc	*/
#endif
	int		cursor_set;	/* requested state		*/
	int		cursor_col;	/* previous cursor column	*/
	int		cursor_row;	/* previous cursor row		*/
	int		cur_col;	/* current cursor column	*/
	int		cur_row;	/* current cursor row		*/
	int		max_col;	/* rightmost column		*/
	int		max_row;	/* bottom row			*/
	int		top_marg;	/* top line of scrolling region */
	int		bot_marg;	/* bottom line of  "	    "	*/
	Widget		scrollWidget;	/* pointer to scrollbar struct	*/
	int		topline;	/* line number of top, <= 0	*/
	int		savedlines;     /* number of lines that've been saved */
	int		savelines;	/* number of lines off top to save */
	int		scrolllines;	/* number of lines to button scroll */
	Boolean		scrollttyoutput; /* scroll to bottom on tty output */
	Boolean		scrollkey;	/* scroll to bottom on key	*/

	ScrnBuf		visbuf;		/* ptr to visible screen buf (main) */
	ScrnBuf		allbuf;		/* screen buffer (may include
					   lines scrolled off top)	*/
	Char		*sbuf_address;	/* main screen memory address   */
	ScrnBuf		altbuf;		/* alternate screen buffer	*/
	Char		*abuf_address;	/* alternate screen memory address */
	Char		**save_ptr;	/* workspace for save-pointers  */
	size_t		save_len;	/* ...and its length		*/
	Boolean		alternate;	/* true if using alternate buf	*/
	unsigned short	do_wrap;	/* true if cursor in last column
					    and character just output    */
	int		incopy;		/* 0 idle; 1 XCopyArea issued;
					    -1 first GraphicsExpose seen,
					    but last not seen		*/
	int		copy_src_x;	/* params from last XCopyArea ... */
	int		copy_src_y;
	unsigned int	copy_width;
	unsigned int	copy_height;
	int		copy_dest_x;
	int		copy_dest_y;
	Boolean		c132;		/* allow change to 132 columns	*/
	Boolean		curses;		/* kludge line wrap for more	*/
	Boolean		hp_ll_bc;	/* kludge HP-style ll for xdb	*/
	Boolean		marginbell;	/* true if margin bell on	*/
	int		nmarginbell;	/* columns from right margin	*/
	int		bellarmed;	/* cursor below bell margin	*/
	Boolean 	multiscroll;	/* true if multi-scroll		*/
	int		scrolls;	/* outstanding scroll count,
					    used only with multiscroll	*/
	SavedCursor	sc;		/* data for restore cursor	*/
	int		save_modes[24];	/* save dec/xterm private modes	*/

	/* Improved VT100 emulation stuff.				*/
	char		gsets[4];	/* G0 through G3.		*/
	char		curgl;		/* Current GL setting.		*/
	char		curgr;		/* Current GR setting.		*/
	char		curss;		/* Current single shift.	*/
	int		terminal_id;	/* 100=vt100, 220=vt220, etc.	*/
	int		ansi_level;	/* 0=vt100, 1,2,3 = vt100 ... vt320 */
	int		scroll_amt;	/* amount to scroll		*/
	int		refresh_amt;	/* amount to refresh		*/
	int		protected_mode;	/* 0=off, 1=DEC, 2=ISO		*/
	Boolean		old_fkeys;	/* true for compatible fkeys	*/
	Boolean		jumpscroll;	/* whether we should jumpscroll */
	Boolean         always_highlight; /* whether to highlight cursor */
	Boolean		underline;	/* whether to underline text	*/

	/* Testing */
#if OPT_XMC_GLITCH
	int		xmc_glitch;	/* # of spaces to pad on SGR's	*/
	int		xmc_attributes;	/* attrs that make a glitch	*/
	Boolean		xmc_inline;	/* SGR's propagate only to eol	*/
	Boolean		move_sgr_ok;	/* SGR is reset on move		*/
#endif

#if OPT_TEK4014
/* Tektronix window parameters */
	GC		TnormalGC;	/* normal painting		*/
	GC		TcursorGC;	/* normal cursor painting	*/
	Pixel		Tforeground;	/* foreground color		*/
	Pixel		Tbackground;	/* Background color		*/
	Pixel		Tcursorcolor;	/* Cursor color			*/
	int		Tcolor;		/* colors used			*/
	Boolean		Tshow;		/* Tek window showing		*/
	Boolean		waitrefresh;	/* postpone refresh		*/
	struct _tekwin {
		Window	window;		/* X window id			*/
		int	width;		/* width of columns		*/
		int	height;		/* height of rows		*/
		int	fullwidth;	/* full width of window		*/
		int	fullheight;	/* full height of window	*/
		double	tekscale;	/* scale factor Tek -> vs100	*/
	} fullTwin;
#ifndef NO_ACTIVE_ICON
	struct _tekwin iconTwin;
	struct _tekwin *whichTwin;
#endif /* NO_ACTIVE_ICON */
	int		xorplane;	/* z plane for inverts		*/
	GC		linepat[TEKNUMLINES]; /* line patterns		*/
	Boolean		TekEmu;		/* true if Tektronix emulation	*/
	int		cur_X;		/* current x			*/
	int		cur_Y;		/* current y			*/
	Tmodes		cur;		/* current tek modes		*/
	Tmodes		page;		/* starting tek modes on page	*/
	int		margin;		/* 0 -> margin 1, 1 -> margin 2	*/
	int		pen;		/* current Tektronix pen 0=up, 1=dn */
	char		*TekGIN;	/* nonzero if Tektronix GIN mode*/
	int		gin_terminator; /* Tek strap option */
#endif /* OPT_TEK4014 */

	int		multiClickTime;	 /* time between multiclick selects */
	int		bellSuppressTime; /* msecs after Bell before another allowed */
	Boolean		bellInProgress; /* still ringing/flashing prev bell? */
	char		*charClass;	/* for overriding word selection */
	Boolean		cutNewline;	/* whether or not line cut has \n */
	Boolean		cutToBeginningOfLine;  /* line cuts to BOL? */
	Boolean		highlight_selection; /* controls appearance of selection */
	char		*selection;	/* the current selection */
	int		selection_size; /* size of allocated buffer */
	int		selection_length; /* number of significant bytes */
	Time		selection_time;	/* latest event timestamp */
	int		startHRow, startHCol, /* highlighted text */
			endHRow, endHCol,
			startHCoord, endHCoord;
	Atom*		selection_atoms; /* which selections we own */
	Cardinal	sel_atoms_size;	/*  how many atoms allocated */
	Cardinal	selection_count; /* how many atoms in use */
	Boolean		input_eight_bits;/* use 8th bit instead of ESC prefix */
	Boolean		output_eight_bits; /* honor all bits or strip */
	Boolean		control_eight_bits; /* send CSI as 8-bits */
	Boolean		backarrow_key;		/* backspace/delete */
	Pixmap		menu_item_bitmap;	/* mask for checking items */
	Widget		mainMenu, vtMenu, tekMenu, fontMenu;
	String		menu_font_names[NMENUFONTS];
	int		menu_font_number;
	XIC		xic;
} TScreen;

typedef struct _TekPart {
    XFontStruct *Tfont[TEKNUMFONTS];
    int		tobaseline[TEKNUMFONTS]; /* top to baseline for each font */
    char	*initial_font;		/* large, 2, 3, small */
    char	*gin_terminator_str;	/* ginTerminator resource */
} TekPart;



/* meaning of bits in screen.select flag */
#define	INWINDOW	01	/* the mouse is in one of the windows */
#define	FOCUS		02	/* one of the windows is the focus window */

#define MULTICLICKTIME 250	/* milliseconds */

typedef struct
{
	unsigned	flags;
} TKeyboard;

typedef struct _Misc {
    char *geo_metry;
    char *T_geometry;
    char *f_n;
    char *f_b;
#ifdef ALLOWLOGGING
    Boolean log_on;
#endif
    Boolean login_shell;
    Boolean re_verse;
    XtGravity resizeGravity;
    Boolean reverseWrap;
    Boolean autoWrap;
    Boolean logInhibit;
    Boolean signalInhibit;
#if OPT_TEK4014
    Boolean tekInhibit;
    Boolean tekSmall;	/* start tek window in small size */
#endif
    Boolean scrollbar;
#ifdef SCROLLBAR_RIGHT
    Boolean useRight;
#endif
    Boolean titeInhibit;
    Boolean appcursorDefault;
    Boolean appkeypadDefault;
#if OPT_INPUT_METHOD
    char* input_method;
    char* preedit_type;
    Boolean open_im;
#endif
    Boolean dynamicColors;
    Boolean shared_ic;
#ifndef NO_ACTIVE_ICON
    Boolean active_icon;	/* use application icon window  */
    int icon_border_width;
    Pixel icon_border_pixel;
#endif /* NO_ACTIVE_ICON */
} Misc;

typedef struct {int foo;} XtermClassPart, TekClassPart;

typedef struct _XtermClassRec {
    CoreClassPart  core_class;
    XtermClassPart xterm_class;
} XtermClassRec;

extern WidgetClass xtermWidgetClass;

#define IsXtermWidget(w) (XtClass(w) == xtermWidgetClass)

#if OPT_TEK4014
typedef struct _TekClassRec {
    CoreClassPart core_class;
    TekClassPart tek_class;
} TekClassRec;
#endif

/* define masks for keyboard.flags */
#define MODE_KAM	0x01	/* keyboard action mode */
#define MODE_DECKPAM	0x02	/* keypad application mode */
#define MODE_DECCKM	0x04	/* cursor keys */
#define MODE_SRM	0x08	/* send-receive mode */
#define MODE_DECBKM	0x10	/* backarrow */


#define N_MARGINBELL	10
#define MAX_TABS	320
#define TAB_ARRAY_SIZE	10	/* number of ints to provide MAX_TABS bits */

typedef unsigned Tabs [TAB_ARRAY_SIZE];

typedef struct _XtermWidgetRec {
    CorePart	core;
    TKeyboard	keyboard;	/* terminal keyboard		*/
    TScreen	screen;		/* terminal screen		*/
    unsigned	flags;		/* mode flags			*/
    int         cur_foreground;	/* current foreground color	*/
    int         cur_background;	/* current background color	*/
    Pixel       dft_foreground;	/* default foreground color	*/
    Pixel       dft_background;	/* default background color	*/
#if OPT_ISO_COLORS
    int         sgr_foreground;	/* current SGR foreground color	*/
#endif
#if OPT_ISO_COLORS || OPT_DEC_CHRSET
    int         num_ptrs;	/* number of pointers per row in 'ScrnBuf' */
#endif
    unsigned	initflags;	/* initial mode flags		*/
    Tabs	tabs;		/* tabstops of the terminal	*/
    Misc	misc;		/* miscellaneous parameters	*/
} XtermWidgetRec, *XtermWidget;

#if OPT_TEK4014
typedef struct _TekWidgetRec {
    CorePart core;
    TekPart tek;
} TekWidgetRec, *TekWidget;
#endif /* OPT_TEK4014 */

#define BUF_SIZE 4096

/*
 * terminal flags
 * There are actually two namespaces mixed together here.
 * One is the set of flags that can go in screen->visbuf attributes
 * and which must fit in a char.
 * The other is the global setting stored in
 * term->flags and screen->save_modes.  This need only fit in an unsigned.
 */

/* global flags and character flags (visible character attributes) */
#define INVERSE		0x01	/* invert the characters to be output */
#define UNDERLINE	0x02	/* true if underlining */
#define BOLD		0x04
#define BLINK		0x08
/* global flags (also character attributes) */
#define BG_COLOR	0x10	/* true if background set */
#define FG_COLOR	0x20	/* true if foreground set */

/* character flags (internal attributes) */
#define PROTECTED	0x40	/* a character is drawn that cannot be erased */
#define CHARDRAWN	0x80    /* a character has been drawn here on the
				   screen.  Used to distinguish blanks from
				   empty parts of the screen when selecting */

			/* mask: user-visible attributes */
#define	ATTRIBUTES	(INVERSE|UNDERLINE|BOLD|BLINK|BG_COLOR|FG_COLOR|INVISIBLE|PROTECTED)

#define WRAPAROUND	0x400	/* true if auto wraparound mode */
#define	REVERSEWRAP	0x800	/* true if reverse wraparound mode */
#define REVERSE_VIDEO	0x1000	/* true if screen white on black */
#define LINEFEED	0x2000	/* true if in auto linefeed mode */
#define ORIGIN		0x4000	/* true if in origin mode */
#define INSERT		0x8000	/* true if in insert mode */
#define SMOOTHSCROLL	0x10000	/* true if in smooth scroll mode */
#define IN132COLUMNS	0x20000	/* true if in 132 column mode */
#define INVISIBLE	0x40000	/* true if writing invisible text */
#define NATIONAL       0x100000	/* true if writing national charset */

/*
 * Per-line flags
 */
#define LINEWRAPPED	0x01	/* used once per line to indicate that it wraps
				 * onto the next line so we can tell the
				 * difference between lines that have wrapped
				 * around and lines that have ended naturally
				 * with a CR at column max_col.
				 */
/*
 * If we've set protected attributes with the DEC-style DECSCA, then we'll have
 * to use DECSED or DECSEL to erase preserving protected text.  (The normal ED,
 * EL won't preserve protected-text).  If we've used SPA, then normal ED and EL
 * will preserve protected-text.  To keep things simple, just remember the last
 * control that was used to begin protected-text, and use that to determine how
 * erases are performed (otherwise we'd need 2 bits per protected character).
 */
#define OFF_PROTECT 0
#define DEC_PROTECT 1
#define ISO_PROTECT 2

#ifdef SCROLLBAR_RIGHT
#define OriginX(screen) (((term->misc.useRight)?0:Scrollbar(screen)) + screen->border)
#else
#define OriginX(screen) (Scrollbar(screen) + screen->border)
#endif

#define CursorX(screen,col) ((col) * FontWidth(screen) + OriginX(screen))
#define CursorY(screen,row) ((((row) - screen->topline) * FontHeight(screen)) \
			+ screen->border)

#ifndef NO_ACTIVE_ICON
#define IsIcon(screen)		((screen)->whichVwin == &(screen)->iconVwin)
#define VWindow(screen)		((screen)->whichVwin->window)
#define VShellWindow		term->core.parent->core.window
#define TextWindow(screen)      ((screen)->whichVwin->window)
#define TWindow(screen)		((screen)->whichTwin->window)
#define TShellWindow		tekWidget->core.parent->core.window
#define Width(screen)		((screen)->whichVwin->width)
#define Height(screen)		((screen)->whichVwin->height)
#define FullWidth(screen)	((screen)->whichVwin->fullwidth)
#define FullHeight(screen)	((screen)->whichVwin->fullheight)
#define FontWidth(screen)	((screen)->whichVwin->f_width)
#define FontHeight(screen)	((screen)->whichVwin->f_height)
#define FontAscent(screen)	(IsIcon(screen) ? (screen)->fnt_icon->ascent \
						: (screen)->fnt_norm->ascent)
#define FontDescent(screen)	(IsIcon(screen) ? (screen)->fnt_icon->descent \
						: (screen)->fnt_norm->descent)
#define Scrollbar(screen)	((screen)->whichVwin->scrollbar)
#define NormalGC(screen)	((screen)->whichVwin->normalGC)
#define ReverseGC(screen)	((screen)->whichVwin->reverseGC)
#define NormalBoldGC(screen)	((screen)->whichVwin->normalboldGC)
#define ReverseBoldGC(screen)	((screen)->whichVwin->reverseboldGC)
#define TWidth(screen)		((screen)->whichTwin->width)
#define THeight(screen)		((screen)->whichTwin->height)
#define TFullWidth(screen)	((screen)->whichTwin->fullwidth)
#define TFullHeight(screen)	((screen)->whichTwin->fullheight)
#define TekScale(screen)	((screen)->whichTwin->tekscale)

#else /* NO_ACTIVE_ICON */

#define IsIcon(screen)		(False)
#define VWindow(screen)		((screen)->fullVwin.window)
#define VShellWindow		term->core.parent->core.window
#define TextWindow(screen)      ((screen)->fullVwin.window)
#define TWindow(screen)		((screen)->fullTwin.window)
#define TShellWindow		tekWidget->core.parent->core.window
#define Width(screen)		((screen)->fullVwin.width)
#define Height(screen)		((screen)->fullVwin.height)
#define FullWidth(screen)	((screen)->fullVwin.fullwidth)
#define FullHeight(screen)	((screen)->fullVwin.fullheight)
#define FontWidth(screen)	((screen)->fullVwin.f_width)
#define FontHeight(screen)	((screen)->fullVwin.f_height)
#define FontAscent(screen)	((screen)->fnt_norm->ascent)
#define FontDescent(screen)	((screen)->fnt_norm->descent)
#define Scrollbar(screen)	((screen)->fullVwin.scrollbar)
#define NormalGC(screen)	((screen)->fullVwin.normalGC)
#define ReverseGC(screen)	((screen)->fullVwin.reverseGC)
#define NormalBoldGC(screen)	((screen)->fullVwin.normalboldGC)
#define ReverseBoldGC(screen)	((screen)->fullVwin.reverseboldGC)
#define TWidth(screen)		((screen)->fullTwin.width)
#define THeight(screen)		((screen)->fullTwin.height)
#define TFullWidth(screen)	((screen)->fullTwin.fullwidth)
#define TFullHeight(screen)	((screen)->fullTwin.fullheight)
#define TekScale(screen)	((screen)->fullTwin.tekscale)

#endif /* NO_ACTIVE_ICON */

#define	TWINDOWEVENTS	(KeyPressMask | ExposureMask | ButtonPressMask |\
			 ButtonReleaseMask | StructureNotifyMask |\
			 EnterWindowMask | LeaveWindowMask | FocusChangeMask)

#define	WINDOWEVENTS	(TWINDOWEVENTS | PointerMotionMask)

#if OPT_TEK4014
#define TEK_LINK_BLOCK_SIZE 1024

typedef struct Tek_Link
{
	struct Tek_Link	*next;	/* pointer to next TekLink in list
				   NULL <=> this is last TekLink */
	short fontsize;		/* character size, 0-3 */
	short count;		/* number of chars in data */
	char *ptr;		/* current pointer into data */
	char data [TEK_LINK_BLOCK_SIZE];
} TekLink;
#endif /* OPT_TEK4014 */

/* flags for cursors */
#define	OFF		0
#define	ON		1
#define	BLINKED_OFF	2
#define	CLEAR		0
#define	TOGGLE		1

/* flags for inhibit */
#ifdef ALLOWLOGGING
#define	I_LOG		0x01
#endif
#define	I_SIGNAL	0x02
#define	I_TEK		0x04

#endif /* included_ptyx_h */
