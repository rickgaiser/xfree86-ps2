/* $XConsortium: xf86Jstk.c /main/14 1996/10/25 14:11:36 kaleb $ */
/*
 * Copyright 1995-1997 by Frederic Lepied, France. <Frederic.Lepied@sugix.frmug.org>       
 *                                                                            
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is  hereby granted without fee, provided that
 * the  above copyright   notice appear  in   all  copies and  that both  that
 * copyright  notice   and   this  permission   notice  appear  in  supporting
 * documentation, and that   the  name of  Frederic   Lepied not  be  used  in
 * advertising or publicity pertaining to distribution of the software without
 * specific,  written      prior  permission.     Frederic  Lepied   makes  no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.                   
 *                                                                            
 * FREDERIC  LEPIED DISCLAIMS ALL   WARRANTIES WITH REGARD  TO  THIS SOFTWARE,
 * INCLUDING ALL IMPLIED   WARRANTIES OF MERCHANTABILITY  AND   FITNESS, IN NO
 * EVENT  SHALL FREDERIC  LEPIED BE   LIABLE   FOR ANY  SPECIAL, INDIRECT   OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA  OR PROFITS, WHETHER  IN  AN ACTION OF  CONTRACT,  NEGLIGENCE OR OTHER
 * TORTIOUS  ACTION, ARISING    OUT OF OR   IN  CONNECTION  WITH THE USE    OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Jstk.c,v 3.18.2.2 1998/11/12 11:32:04 dawes Exp $ */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "XI.h"
#include "XIproto.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86_Config.h"
#include "xf86Procs.h"
#include "xf86Xinput.h"
#include "xf86_OSlib.h"
#include "atKeynames.h"
#include "xf86Version.h"

#include <linux/ps2/pad.h>

#ifdef XFree86LOADER
#define strdup(a) xf86strdup(a)
#endif

/******************************************************************************
 * debugging macro
 *****************************************************************************/
#ifdef DBG
#undef DBG
#endif
#ifdef DEBUG
#undef DEBUG
#endif

static int      debug_level = 0;
#define DEBUG 1
#if DEBUG
#define DBG(lvl, f) {if ((lvl) <= debug_level) f;}
#else
#define DBG(lvl, f)
#endif

/******************************************************************************
 * device records
 *****************************************************************************/

typedef struct 
{
  int           padFd;         /* Ps2Pad File Descriptor */
  OsTimerPtr    padTimer;      /* timer object */
  int           padTimeout;    /* timeout to poll device */
  char          *padDevice;    /* device file name */

  int           button1;       /* Button1 bit mask */
  int           button2;       /* Button2 bit mask */
  int           button3;       /* Button3 bit mask */

  int           CenterX;       /* Analog stick X center value */
  int           CenterY;       /* Analog stick Y center value */
  int           DeadZone;      /* Analog stick DeadZone value */
  int           DgtCount;      /* Digital stick count value */
  float         Scale;         /* sensitivity scaling factor */
  unsigned short saveButton;   /* previous buttons state */
  float         saveX;         /* previous X position */
  float         saveY;         /* previous Y position */
  int		ErrRetryCnt;   /* Error retry counter */
} Ps2PadDevRec, *Ps2PadDevPtr;

/******************************************************************************
 * configuration stuff
 *****************************************************************************/
#define DEVICENAME   1
#define TIMEOUT      2
#define MSBUTTON1    3
#define MSBUTTON2    4
#define MSBUTTON3    5
#define CENTERX      6
#define CENTERY      7
#define DEADZONE     8
#define DGTCOUNT     9
#define SCALE        10
#define PORT         11
#define DEBUG_LEVEL  12
#define ALWAYS_CORE  13

static SymTabRec Ps2padTab[] = {
  { ENDSUBSECTION,	"endsubsection" },
  { DEVICENAME,		"devicename" },
  { TIMEOUT,		"timeout" },
  { MSBUTTON1,		"button1" },
  { MSBUTTON2,		"button2" },
  { MSBUTTON3,		"button3" },
  { CENTERX,		"centerx" },
  { CENTERY,		"centery" },
  { DEADZONE,		"deadzone" },
  { DGTCOUNT,           "dgtcount" },
  { SCALE,              "scale" },
  { PORT,		"port" },
  { DEBUG_LEVEL,	"debuglevel" },
  { ALWAYS_CORE,        "alwayscore" },
  { -1,			"" },
};

static SymTabRec Ps2padButtonTab[] = {
  { PS2PAD_BUTTON_LEFT,		"left" },
  { PS2PAD_BUTTON_DOWN,		"down" },
  { PS2PAD_BUTTON_RIGHT,	"right" },
  { PS2PAD_BUTTON_UP,		"up" },
  { PS2PAD_BUTTON_START,	"start" },
  { PS2PAD_BUTTON_SELECT,	"select" },
  { PS2PAD_BUTTON_CIRCLE,	"circle" },
  { PS2PAD_BUTTON_TRIANGLE,	"triangle" },
  { PS2PAD_BUTTON_CROSS,	"cross" },
  { PS2PAD_BUTTON_SQUARE,	"square" },
  { PS2PAD_BUTTON_R1,		"r1" },
  { PS2PAD_BUTTON_L1,		"l1" },
  { PS2PAD_BUTTON_R2,		"r2" },
  { PS2PAD_BUTTON_L2,		"l2" },
  { PS2PAD_BUTTON_R3,		"r3" },
  { PS2PAD_BUTTON_L3,		"l3" },
  { -1,			"" },
};


/******************************************************************************
 * Function/Macro keys variables
 *****************************************************************************/
static KeySym wacom_map[] = 
{
    XK_A,   /* 0x00 */
    NoSymbol,   /* 0x01 */
    NoSymbol,   /* 0x02 */
    NoSymbol,   /* 0x03 */
    NoSymbol,   /* 0x04 */
    NoSymbol,   /* 0x05 */
    NoSymbol,   /* 0x06 */
    XK_A,   /* 0x07 */
    XK_Return ,      /* 0x08 */
    XK_B ,      /* 0x09 */
    XK_C ,      /* 0x0a */
    XK_F4,      /* 0x0b */
    XK_F5,      /* 0x0c */
    XK_F6,      /* 0x0d */
    XK_F7,      /* 0x0e */
    XK_F8,      /* 0x0f */
    XK_F8,      /* 0x10 */
    XK_F10,     /* 0x11 */
    XK_F11,     /* 0x12 */
    XK_F12,     /* 0x13 */
    XK_F13,     /* 0x14 */
    XK_F14,     /* 0x15 */
    XK_F15,     /* 0x16 */
    XK_F16,     /* 0x17 */
    XK_F17,     /* 0x18 */
    XK_F18,     /* 0x19 */
    XK_F19,     /* 0x1a */
    XK_F20,     /* 0x1b */
    XK_F21,     /* 0x1c */
    XK_F22,     /* 0x1d */
    XK_F23,     /* 0x1e */
    XK_F24,     /* 0x1f */
    XK_F25,     /* 0x20 */
    XK_F26,     /* 0x21 */
    XK_F27,     /* 0x22 */
    XK_F28,     /* 0x23 */
    XK_F29,     /* 0x24 */
    XK_F30,     /* 0x25 */
    XK_F31,     /* 0x26 */
    XK_F32      /* 0x27 */
};
/* minKeyCode = 8 because this is the min legal key code */
static KeySymsRec wacom_keysyms = {
  /* map        minKeyCode      maxKC   width */
  wacom_map,    8,              0x27,   1
};


/*
 * xf86Ps2padConfig --
 *      Configure the device.
 */
static Bool
xf86Ps2padConfig(LocalDevicePtr    *array,
               int               index,
               int               max,
               LexPtr            val)
{
  LocalDevicePtr        dev = array[index];
  Ps2PadDevPtr        priv = (Ps2PadDevPtr)(dev->private);
  int token;
  
  DBG(1, ErrorF("xf86Ps2padConfig\n"));
      
  /* Set defaults */
  priv->saveX = 0;
  priv->saveY = 0;
  priv->saveButton = 0xffff;
  priv->padFd = -1;
  priv->padTimeout = 16;

  while ((token = xf86GetToken(Ps2padTab)) != ENDSUBSECTION) {
    switch(token) {
    case DEVICENAME:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("Option string expected");
      dev->name = strdup(val->str);
      break;
      
    case PORT:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("Option string expected");
      priv->padDevice = strdup(val->str);
      break;

    case TIMEOUT:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Ps2Pad Timeout expected");
      priv->padTimeout = val->num;
     break;

    case MSBUTTON1:
      if ((token = xf86GetToken(Ps2padButtonTab)) == ERROR_TOKEN)
        xf86ConfigError("Ps2Pad Button1 expected");
      priv->button1 = token;
     break;

    case MSBUTTON2:
      if ((token = xf86GetToken(Ps2padButtonTab)) == ERROR_TOKEN)
        xf86ConfigError("Ps2Pad Button2 expected");
      priv->button2 = token;
     break;

    case MSBUTTON3:
      if ((token = xf86GetToken(Ps2padButtonTab)) == ERROR_TOKEN)
        xf86ConfigError("Ps2Pad Button3 expected");
      priv->button3 = token;
     break;

    case CENTERX:
      if (xf86GetToken(NULL) != NUMBER)
        xf86ConfigError("Ps2Pad CenterX expected");
      priv->CenterX = val->num;
     break;
      
    case CENTERY:
      if (xf86GetToken(NULL) != NUMBER)
        xf86ConfigError("Ps2Pad CenterY expected");
      priv->CenterY = val->num;
     break;
      
    case DEADZONE:
      if (xf86GetToken(NULL) != NUMBER)
        xf86ConfigError("Ps2Pad DeadZone expected");
      priv->DeadZone = val->num;
     break;
      
    case DGTCOUNT:
      if (xf86GetToken(NULL) != NUMBER)
        xf86ConfigError("Ps2Pad DgtCount expected");
      priv->DgtCount = val->num;
     break;
      
    case SCALE:
      if (xf86GetToken(NULL) != NUMBER)
        xf86ConfigError("Ps2Pad Scale expected");
      priv->Scale = val->realnum;
     break;
      
    case DEBUG_LEVEL:
	if (xf86GetToken(NULL) != NUMBER)
	    xf86ConfigError("Option number expected");
	debug_level = val->num;
	if (xf86Verbose) {
#if DEBUG
	    ErrorF("%s Ps2Pad debug level sets to %d\n", XCONFIG_GIVEN,
		   debug_level);      
#else
	    ErrorF("%s Ps2Pad debug level not sets to %d because debugging is not compiled\n",
		   XCONFIG_GIVEN, debug_level);
#endif
	}
        break;

    case ALWAYS_CORE:
	xf86AlwaysCore(dev, TRUE);
	if (xf86Verbose)
	    ErrorF("%s Ps2Pad device always stays core pointer\n",
		   XCONFIG_GIVEN);
	break;

    case EOF:
      FatalError("Unexpected EOF (missing EndSubSection)");
      break; /* :-) */

    default:
      xf86ConfigError("Ps2Pad subsection keyword expected");
      break;
    }
  }

/*  if (xf86Verbose) {
    ErrorF("%s %s: timeout=%d port=%s maxx=%d maxy=%d minx=%d miny=%d\n"
	   "\tcenterx=%d centery=%d delta=%d\n", XCONFIG_GIVEN, dev->name,
	   priv->padTimeout, priv->padDevice, priv->padMaxX, priv->padMaxY,
	   priv->padMinX, priv->padMinY, priv->padCenterX, priv->padCenterY,
	   priv->padDelta);
  }*/
  return Success;
}


/******************************************************************************
 * external declarations
 *****************************************************************************/

extern void xf86eqEnqueue(
    xEventPtr /*e*/
);


/***********************************************************************
 *
 * xf86Ps2PadOn --
 *
 * open the device and init timeout according to the device value.
 *
 ***********************************************************************
 */
static int
xf86Ps2PadOn(char *name)
{
  int i,fd;
  int stat;

  DBG(1, ErrorF("xf86Ps2PadOn %s\n", name));

  if ((fd = open(name, O_RDWR)) < 0)
    {
      ErrorF("Cannot open ps2pad '%s' (%s)\n", name, strerror(errno));
      return -1;
    }

  for (i=0;i<30;i++) {
    if (ioctl(fd, PS2PAD_IOCGETSTAT, &stat) < 0) {
      ErrorF("ioctl(GETSTAT) '%s' (%s)\n", name, strerror(errno));
      close(fd); return -1;
    }
    if (stat == PS2PAD_STAT_READY)
	return fd;	// Success

    usleep(16);		// retrying 0.5 seconds.
  }

  ErrorF("PS2PAD not ready. %d\n", stat);
  return -1;
}

/***********************************************************************
 *
 * xf86Ps2PadInit --
 *
 * called when X device is initialized.
 *
 ***********************************************************************
 */
static void
xf86Ps2PadInit()
{
	return;
}

/***********************************************************************
 *
 * xf86Ps2PadOff --
 *
 * close the handle.
 *
 ***********************************************************************
 */
static int
xf86Ps2PadOff(fd, doclose)
int *fd;
int doclose;
{
  int   oldfd;
  
  if (((oldfd = *fd) >= 0) && doclose) {
    close(*fd);
    *fd = -1;
  }
  return oldfd;
}


/*
 ***************************************************************************
 *
 * xf86Ps2padConvert --
 *	Convert valuators to X and Y.
 *
 ***************************************************************************
 */
static Bool
xf86Ps2padConvert(LocalDevicePtr	local,
		int		first,
		int		num,
		int		v0,
		int		v1,
		int		v2,
		int		v3,
		int		v4,
		int		v5,
		int*		x,
		int*		y)
{
    if (first != 0 || num != 2)
      return FALSE;

    *x = v0;
    *y = v1;

    return TRUE;
}

/*
 * xf86Ps2padEvents --
 *      Read the new events from the device, and enqueue them.
 */
static CARD32
xf86Ps2padEvents(OsTimerPtr        timer,
               CARD32            atime,
               pointer           arg)
{
  DeviceIntPtr          device = (DeviceIntPtr)arg;
  Ps2PadDevPtr        priv = (Ps2PadDevPtr) PRIVATE(device);
  int                   timeout = priv->padTimeout;
  int                   x, y, l, X, Y;
  unsigned char buf[PS2PAD_DATASIZE];

  const int button1 = priv->button1;
  const int button2 = priv->button2;
  const int button3 = priv->button3;
  const int CenterX = priv->CenterX;
  const int CenterY = priv->CenterY;
  const int DeadZone = priv->DeadZone;
  const int DgtCnt = priv->DgtCount;
  const float Scale = priv->Scale;

  DBG(5, ErrorF("xf86Ps2padEvents BEGIN device=0x%x priv=0x%x"
                " timeout=%d timer=0x%x\n",
                device, priv, timeout, priv->padTimer));

  if (priv->padFd < 0)
    return 0;

  l = read(priv->padFd, buf, PS2PAD_DATASIZE);
  if (l<0 || buf[0]!=0) {
    if (priv->ErrRetryCnt++ > 30) {
      if (l<0)  Error("Ps2Pad read");
      else      Error("Ps2Pad error");
      return 0;
    }
    return timeout;	// retrying 0.5 seconds.
  }
  if (priv->ErrRetryCnt) {
    DBG(1, ErrorF("Ps2Pad error retry %d\n", priv->ErrRetryCnt));
    priv->ErrRetryCnt = 0;
  }

  x = y = 0;

  // Analog input
  if ((buf[1] & 0xf) > 1) {
    //if (ShowValue)
    //printf("%03d %03d\n", buf[6], buf[7]);
    x = (-CenterX + (int)buf[4]) + (-CenterX + (int)buf[6]);
    y = (-CenterY + (int)buf[5]) + (-CenterY + (int)buf[7]);
    if (x <= DeadZone && x >= -DeadZone) x = 0;
    else    x -= (x>0 ? +1 : -1) * DeadZone;
    if (y <= DeadZone && y >= -DeadZone) y = 0;
    else    y -= (y>0 ? +1 : -1) * DeadZone;
  }

  // Digital input
  {
    unsigned short button, chg;
    button = ((unsigned short)buf[2])<<8 | buf[3];

    if (!(button & PS2PAD_BUTTON_LEFT))     x -= DgtCnt;
    if (!(button & PS2PAD_BUTTON_RIGHT))    x += DgtCnt;
    if (!(button & PS2PAD_BUTTON_UP))       y -= DgtCnt;
    if (!(button & PS2PAD_BUTTON_DOWN))     y += DgtCnt;

/*    if (!(button & PS2PAD_BUTTON_R1)) {x*=2;y*=2;}
    if (!(button & PS2PAD_BUTTON_L1)) {x*=2;y*=2;}
    if (!(button & PS2PAD_BUTTON_R2)) {x*=2;y*=2;}
    if (!(button & PS2PAD_BUTTON_L2)) {x*=2;y*=2;}*/

    if (!(button & PS2PAD_BUTTON_R1)) {
	ErrorF("PostKey\n");
	xf86PostKeyEvent(device, 79, 1,
				0, 0, 2,
				0, 0);
	xf86PostKeyEvent(device, 79, 0,
				0, 0, 2,
				0, 0);
    }

    {       // float
      float fx,fy;
      fx = priv->saveX + (float)(x>0 ? x*x : -x*x) * Scale;
      fy = priv->saveY + (float)(y>0 ? y*y : -y*y) * Scale;
      X = (int)fx;
      Y = (int)fy;
      priv->saveX = fx - X;
      priv->saveY = fy - Y;

      if (X != 0 || Y != 0)
	xf86PostMotionEvent(device, 0, 0, 2, X, Y);
    }

    // Button Press Event
    chg = button ^ priv->saveButton;
    if (chg & button1)
      xf86PostButtonEvent(device, 0, 1, ((button & button1) == 0), 0, 2, X, Y);
    if (chg & button2)
      xf86PostButtonEvent(device, 0, 2, ((button & button2) == 0), 0, 2, X, Y);
    if (chg & button3)
      xf86PostButtonEvent(device, 0, 3, ((button & button3) == 0), 0, 2, X, Y);

    priv->saveButton = button;
  }

  DBG(3, ErrorF("xf86Ps2padEvents END   device=0x%x priv=0x%x"
                " timeout=%d timer=0x%x\n",
                device, priv, timeout, priv->padTimer));

  return timeout;
}

static void
xf86Ps2padControlProc(DeviceIntPtr	device,
                    PtrCtrl		*ctrl)
{
  DBG(2, ErrorF("xf86Ps2padControlProc\n"));
}

/*
 * xf86Ps2padProc --
 *      Handle the initialization, etc. of a joystick
 */
static int
xf86Ps2padProc(pPad, what)
     DeviceIntPtr       pPad;
     int                what;
{
  CARD8                 map[5];
  int                   nbaxes;
  int                   nbbuttons;
  int                   padfd;
  int                   loop;
  LocalDevicePtr        local = (LocalDevicePtr)pPad->public.devicePrivate;
  Ps2PadDevPtr        priv = (Ps2PadDevPtr)PRIVATE(pPad);

  DBG(2, ErrorF("BEGIN xf86Ps2padProc dev=0x%x priv=0x%x xf86Ps2padEvents=0x%x\n",
                pPad, priv, xf86Ps2padEvents));
  
  switch (what)
    {
    case DEVICE_INIT: 
      DBG(1, ErrorF("xf86Ps2padProc pPad=0x%x what=INIT\n", pPad));
  
      map[1] = 1;
      map[2] = 2;
      map[3] = 3;

      nbaxes = 2;
      nbbuttons = 3;

      if (InitButtonClassDeviceStruct(pPad,
                                      nbbuttons,
                                      map) == FALSE) 
        {
          ErrorF("unable to allocate Button class device\n");
          return !Success;
        }
      
      if (InitFocusClassDeviceStruct(pPad) == FALSE)
        {
          ErrorF("unable to init Focus class device\n");
          return !Success;
        }
          
      if (InitPtrFeedbackClassDeviceStruct(pPad,
                                           xf86Ps2padControlProc) == FALSE)
        {
          ErrorF("unable to init ptr feedback\n");
          return !Success;
        }
          
	// Key
        if (InitKeyClassDeviceStruct(pPad, &wacom_keysyms, NULL) == FALSE) {                ErrorF("unable to init key class device\n"); 
                return !Success;
        }

      if (InitValuatorClassDeviceStruct(pPad, 
                                    nbaxes,
                                    xf86GetMotionEvents, 
				    0,/*local->history_size,*/
                                    Relative) /* relatif ou absolute */
          == FALSE) 
        {
          ErrorF("unable to allocate Valuator class device\n"); 
          return !Success;
        }
      else 
        {
	    InitValuatorAxisStruct(pPad,
				   0,
				   0, /* min val */
				   screenInfo.screens[0]->width, /* max val */
				   1, /* resolution */
				   0, /* min_res */
				   1); /* max_res */
	    InitValuatorAxisStruct(pPad,
				   1,
				   0, /* min val */
				   screenInfo.screens[0]->height, /* max val */
				   1, /* resolution */
				   0, /* min_res */
				   1); /* max_res */
	    
	  /* allocate the motion history buffer if needed */
	  xf86MotionHistoryAllocate(local);

          xf86Ps2PadInit();
          AssignTypeAndName(pPad, local->atom, local->name);
        }

      break; 
      
    case DEVICE_ON:
      priv->padFd = padfd = xf86Ps2PadOn(priv->padDevice);

      DBG(1, ErrorF("xf86Ps2padProc  pPad=0x%x what=ON name=%s\n", pPad,
                    priv->padDevice));

      if (padfd != -1)
      {
        priv->padTimer = TimerSet(NULL, 0, /*TimerAbsolute,*/
                                   priv->padTimeout,
                                   xf86Ps2padEvents,
                                   (pointer)pPad);
        pPad->public.on = TRUE;
        DBG(2, ErrorF("priv->padTimer=0x%x\n", priv->padTimer));
      }
      else
        return !Success;
    break;
      
    case DEVICE_OFF:
    case DEVICE_CLOSE:
      DBG(1, ErrorF("xf86Ps2padProc  pPad=0x%x what=%s\n", pPad,
                    (what == DEVICE_CLOSE) ? "CLOSE" : "OFF"));

      padfd = xf86Ps2PadOff(&(priv->padFd), TRUE);
      pPad->public.on = FALSE;
    break;

    default:
      ErrorF("unsupported mode=%d\n", what);
      return !Success;
      break;
    }
  DBG(2, ErrorF("END   xf86Ps2padProc dev=0x%x priv=0x%x xf86Ps2padEvents=0x%x\n",
                pPad, priv, xf86Ps2padEvents));
  return Success;
}

/*
 * xf86Ps2padAllocate --
 *      Allocate Ps2Pad device structures.
 */
static LocalDevicePtr
xf86Ps2padAllocate()
{
  LocalDevicePtr        local = (LocalDevicePtr) xalloc(sizeof(LocalDeviceRec));
  Ps2PadDevPtr        priv = (Ps2PadDevPtr) xalloc(sizeof(Ps2PadDevRec));
  
  local->name = "PS2PAD";
  local->flags = XI86_NO_OPEN_ON_INIT;
  local->device_config = xf86Ps2padConfig;
  local->device_control = xf86Ps2padProc;
  local->read_input = NULL;
  local->close_proc = NULL;
  local->control_proc = NULL;
  local->switch_mode = NULL;
  local->conversion_proc = xf86Ps2padConvert;
  local->reverse_conversion_proc = NULL;
  local->fd = -1;
  local->atom = 0;
  local->dev = NULL;
  local->private = priv;
  local->type_name = "Ps2Pad";
  local->history_size  = 0;
  
  priv->padFd = -1;
  priv->padTimer = NULL;
  priv->padTimeout = 16;
  priv->padDevice = "/dev/ps2pad00";

  priv->button1 = PS2PAD_BUTTON_SQUARE;
  priv->button2 = PS2PAD_BUTTON_TRIANGLE;
  priv->button3 = PS2PAD_BUTTON_CIRCLE;
  priv->CenterX = 128;
  priv->CenterY = 128;
  priv->DeadZone = 20;
  priv->DgtCount = 20;
  priv->Scale = 0.001f;
  priv->saveButton = 0xffff;
  priv->saveX = 0;
  priv->saveY = 0;
  priv->ErrRetryCnt = 0;
  
  return local;
}

/*
 * joystick association
 */
DeviceAssocRec ps2pad_assoc =
{
  "Ps2Pad",                   /* config_section_name */
  xf86Ps2padAllocate              /* device_allocate */
};

#ifdef DYNAMIC_MODULE
/*
 * entry point of dynamic loading
 */
int
init_module(unsigned long	server_version)
{
    xf86AddDeviceAssoc(&ps2pad_assoc);

    if (server_version != XF86_VERSION_CURRENT) {
	ErrorF("Warning: Ps2Pad module compiled for version%s\n", XF86_VERSION);
	return 0;
    } else {
	return 1;
    }
}
#endif

#ifdef XFree86LOADER
/*
 * Entry point for the loader code
 */
XF86ModuleVersionInfo xf86Ps2padVersion = {
    "xf86Ps2pad",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    0x00010000,
    {0,0,0,0}
};

void
xf86Ps2padModuleInit(data, magic)
    pointer *data;
    INT32 *magic;
{
    static int cnt = 0;

    switch (cnt) {
      case 0:
	*magic = MAGIC_VERSION;
	*data = &xf86Ps2padVersion;
	cnt++;
	break;
	
      case 1:
	*magic = MAGIC_ADD_XINPUT_DEVICE;
	*data = &ps2pad_assoc;
	cnt++;
	break;

      default:
	*magic = MAGIC_DONE;
	*data = NULL;
	break;
    } 
}
#endif


/* end of xf86Ps2pad.c */
