/*
 * $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Config.c,v 3.113.2.19 1998/10/18 20:42:11 hohndel Exp $
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $XConsortium: xf86Config.c /main/58 1996/12/28 14:46:17 kaleb $ */

#include <linux/ps2/gs.h>

#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#else
extern double atof();
extern char *getenv();
#endif

#define NEED_EVENTS 1
#include "X.h"
#include "Xproto.h"
#include "Xmd.h"
#include "input.h"
#include "servermd.h"
#include "scrnintstr.h"

#ifdef DPMSExtension
#include "opaque.h"
#endif

#define NO_COMPILER_H_EXTRAS
#include "xf86Procs.h"
#include "xf86_OSlib.h"

#define INIT_CONFIG
#include "xf86_Config.h"

#ifdef XKB
#include "inputstr.h"
#include "XKBsrv.h"
#endif

#ifdef XINPUT
#include "xf86Xinput.h"

extern DeviceAssocRec	mouse_assoc;
#endif

#ifdef NEED_RETURN_VALUE
#define HANDLE_RETURN(xx)	if (xx == RET_ERROR) return RET_ERROR
#else
#define HANDLE_RETURN(xx)	xx
#endif

#define CONFIG_BUF_LEN     1024

static FILE * configFile   = NULL;
static int    configStart  = 0;           /* start of the current token */
static int    configPos    = 0;           /* current readers position */
static int    configLineNo = 0;           /* linenumber */
static char   *configBuf,*configRBuf;     /* buffer for lines */
static char   *configPath;                /* path to config file */
static char   *fontPath = NULL;           /* font path */
static char   *modulePath = NULL;         /* module path */
static int    pushToken = LOCK_TOKEN;
static LexRec val;                        /* global return value */

extern char *defaultFontPath;
extern char *rgbPath;

extern Bool xf86fpFlag, xf86coFlag, xf86sFlag;
extern Bool xf86ScreensOpen;

extern int defaultColorVisualClass;
extern CARD32 defaultScreenSaverTime, ScreenSaverTime;

/* gsxinit.c */
extern void SetGSXVesaScreen(int , int , int , int, int);
extern void SetGSXTVScreen(int , int, int , int , int, int);
extern void SetGSXFrameRate(int, int);
extern void SetGSXInterlaceMode(int , int);
extern void SetGSXInterlaceMix(int , int );
extern int checkResolution(int, int);


static CONFIG_RETURN_TYPE configFilesSection(
#if NeedFunctionPrototypes
    void
#endif
);
static CONFIG_RETURN_TYPE configKeyboardSection(
#if NeedFunctionPrototypes
    void
#endif
);
static CONFIG_RETURN_TYPE configScreenSection(
#if NeedFunctionPrototypes
    void
#endif
);
static CONFIG_RETURN_TYPE configDisplaySubsection(
#if NeedFunctionPrototypes
    DispPtr disp
#endif
);
static CONFIG_RETURN_TYPE configDynamicModuleSection(
#if NeedFunctionPrototypes
    void
#endif
);

static
CONFIG_RETURN_TYPE findConfigFile(
#if NeedFunctionPrototypes
      char *filename,
      FILE **fp
#endif
);
static int getScreenIndex(
#if NeedFunctionPrototypes
     int token
#endif
);
static int getStringToken(
#if NeedFunctionPrototypes
     SymTabRec tab[]
#endif
);
static CONFIG_RETURN_TYPE readVerboseMode(
#if NeedFunctionPrototypes
    MonPtr monp
#endif
);
static Bool validateGraphicsToken(
#if NeedFunctionPrototypes
     int *validTokens,
     int token
#endif
);
extern char * xf86GetPathElem(
#if NeedFunctionPrototypes
     char **pnt
#endif
);
static char * xf86ValidateFontPath(
#if NeedFunctionPrototypes
     char * /* path */
#endif
);
#ifdef XINPUT
extern CONFIG_RETURN_TYPE xf86ConfigExtendedInputSection(
#if NeedFunctionPrototypes
    LexPtr      pval
#endif
);
#endif

#ifdef XKB
extern char *XkbInitialMap;
#endif

#define DIR_FILE	"/fonts.dir"

/*
 * xf86GetPathElem --
 *	Extract a single element from the font path string starting at
 *	pnt.  The font path element will be returned, and pnt will be
 *	updated to point to the start of the next element, or set to
 *	NULL if there are no more.
 */
char *
xf86GetPathElem(pnt)
     char **pnt;
{
  char *p1;

  p1 = *pnt;
  *pnt = index(*pnt, ',');
  if (*pnt != NULL) {
    **pnt = '\0';
    *pnt += 1;
  }
  return(p1);
}

/*
 * StrToUL --
 *
 *	A portable, but restricted, version of strtoul().  It only understands
 *	hex, octal, and decimal.  But it's good enough for our needs.
 */
unsigned int StrToUL(str)
char *str;
{
  int base = 10;
  char *p = str;
  unsigned int tot = 0;

  if (*p == '0') {
    p++;
    if (*p == 'x') {
      p++;
      base = 16;
    }
    else
      base = 8;
  }
  while (*p) {
    if ((*p >= '0') && (*p <= ((base == 8)?'7':'9'))) {
      tot = tot * base + (*p - '0');
    }
    else if ((base == 16) && (*p >= 'a') && (*p <= 'f')) {
      tot = tot * base + 10 + (*p - 'a');
    }
    else if ((base == 16) && (*p >= 'A') && (*p <= 'F')) {
      tot = tot * base + 10 + (*p - 'A');
    }
    else {
      return(tot);
    }
    p++;
  }
  return(tot);
}

/*
 * xf86ValidateFontPath --
 *	Validates the user-specified font path.  Each element that
 *	begins with a '/' is checked to make sure the directory exists.
 *	If the directory exists, the existence of a file named 'fonts.dir'
 *	is checked.  If either check fails, an error is printed and the
 *	element is removed from the font path.
 */
#define CHECK_TYPE(mode, type) ((S_IFMT & (mode)) == (type))
static char *
xf86ValidateFontPath(path)
     char *path;
{
  char *tmp_path, *out_pnt, *path_elem, *next, *p1, *dir_elem;
  struct stat stat_buf;
  int flag;
  int dirlen;

  tmp_path = (char *)xcalloc(1,strlen(path)+1);
  out_pnt = tmp_path;
  path_elem = NULL;
  next = path;
  while (next != NULL) {
    path_elem = xf86GetPathElem(&next);
#ifndef __EMX__
    if (*path_elem == '/') {
      dir_elem = (char *)xcalloc(1, strlen(path_elem) + 1);
      if ((p1 = strchr(path_elem, ':')) != 0)
#else
    /* OS/2 must prepend X11ROOT */
    if (*path_elem == '/') {
      path_elem = (char*)__XOS2RedirRoot(path_elem);
      dir_elem = (char*)xcalloc(1, strlen(path_elem) + 1);
      if (p1 = strchr(path_elem+2, ':'))
#endif
	dirlen = p1 - path_elem;
      else
	dirlen = strlen(path_elem);
      strncpy(dir_elem, path_elem, dirlen);
      dir_elem[dirlen] = '\0';
      flag = stat(dir_elem, &stat_buf);
      if (flag == 0)
	if (!CHECK_TYPE(stat_buf.st_mode, S_IFDIR))
	  flag = -1;
      if (flag != 0) {
        ErrorF("Warning: The directory \"%s\" does not exist.\n", dir_elem);
	ErrorF("         Entry deleted from font path.\n");
	continue;
      }
      else {
	p1 = (char *)xalloc(strlen(dir_elem)+strlen(DIR_FILE)+1);
	strcpy(p1, dir_elem);
	strcat(p1, DIR_FILE);
	flag = stat(p1, &stat_buf);
	if (flag == 0)
	  if (!CHECK_TYPE(stat_buf.st_mode, S_IFREG))
	    flag = -1;
#ifndef __EMX__
	xfree(p1);
#endif
	if (flag != 0) {
	  ErrorF("Warning: 'fonts.dir' not found (or not valid) in \"%s\".\n", 
		 dir_elem);
	  ErrorF("          Entry deleted from font path.\n");
	  ErrorF("          (Run 'mkfontdir' on \"%s\").\n", dir_elem);
	  continue;
	}
      }
      xfree(dir_elem);
    }

    /*
     * Either an OK directory, or a font server name.  So add it to
     * the path.
     */
    if (out_pnt != tmp_path)
      *out_pnt++ = ',';
    strcat(out_pnt, path_elem);
    out_pnt += strlen(path_elem);
  }
  return(tmp_path);
}

/*
 * xf86GetToken --
 *      Read next Token form the config file. Handle the global variable
 *      pushToken.
 */
int
xf86GetToken(tab)
     SymTabRec tab[];
{
  int          c, i;

  /*
   * First check whether pushToken has a different value than LOCK_TOKEN.
   * In this case rBuf[] contains a valid STRING/TOKEN/NUMBER. But in the other
   * case the next token must be read from the input.
   */
  if (pushToken == EOF) return(EOF);
  else if (pushToken == LOCK_TOKEN)
    {
      
      c = configBuf[configPos];
      
      /*
       * Get start of next Token. EOF is handled, whitespaces & comments are
       * skipped. 
       */
      do {
	if (!c)  {
	  if (fgets(configBuf,CONFIG_BUF_LEN-1,configFile) == NULL)
	    {
	      return( pushToken = EOF );
	    }
	  configLineNo++;
	  configStart = configPos = 0;
	}
#ifndef __EMX__
	while (((c=configBuf[configPos++])==' ') || ( c=='\t') || ( c=='\n'));
#else
	while (((c=configBuf[configPos++])==' ') || ( c=='\t') || ( c=='\n') 
		|| (c=='\r'));
#endif
	if (c == '#') c = '\0'; 
      } while (!c);
      
      /* GJA -- handle '-' and ',' 
       * Be careful: "-hsync" is a keyword.
       */
      if ( (c == ',') && !isalpha(configBuf[configPos]) ) {
         configStart = configPos; return COMMA;
      } else if ( (c == '-') && !isalpha(configBuf[configPos]) ) {
         configStart = configPos; return DASH;
      }

      configStart = configPos;
      /*
       * Numbers are returned immediately ...
       */
      if (isdigit(c))
	{
	  int base;

	  if (c == '0')
	    if ((configBuf[configPos] == 'x') || 
		(configBuf[configPos] == 'X'))
	      base = 16;
	    else
	      base = 8;
	  else
	    base = 10;

	  configRBuf[0] = c; i = 1;
	  while (isdigit(c = configBuf[configPos++]) || 
		 (c == '.') || (c == 'x') || 
		 ((base == 16) && (((c >= 'a') && (c <= 'f')) ||
				   ((c >= 'A') && (c <= 'F')))))
            configRBuf[i++] = c;
          configPos--; /* GJA -- one too far */
	  configRBuf[i] = '\0';
	  val.num = StrToUL(configRBuf);
          val.realnum = atof(configRBuf);
	  return(NUMBER);
	}
      
      /*
       * All Strings START with a \" ...
       */
      else if (c == '\"')
	{
	  i = -1;
	  do {
	    configRBuf[++i] = (c = configBuf[configPos++]);
#ifndef __EMX__
	  } while ((c != '\"') && (c != '\n') && (c != '\0'));
#else
	  } while ((c != '\"') && (c != '\n') && (c != '\r') && (c != '\0'));
#endif
	  configRBuf[i] = '\0';
	  val.str = (char *)xalloc(strlen(configRBuf) + 1);
	  strcpy(val.str, configRBuf);      /* private copy ! */
	  return(STRING);
	}
      
      /*
       * ... and now we MUST have a valid token.  The search is
       * handled later along with the pushed tokens.
       */
      else
	{
          configRBuf[0] = c;
          i = 0;
	  do {
	    configRBuf[++i] = (c = configBuf[configPos++]);;
#ifndef __EMX__
	  } while ((c != ' ') && (c != '\t') && (c != '\n') && (c != '\0'));
#else
	  } while ((c != ' ') && (c != '\t') && (c != '\n') && (c != '\r') && (c != '\0') );
#endif
	  configRBuf[i] = '\0'; i=0;
	}
      
    }
  else
    {
    
      /*
       * Here we deal with pushed tokens. Reinitialize pushToken again. If
       * the pushed token was NUMBER || STRING return them again ...
       */
      int temp = pushToken;
      pushToken = LOCK_TOKEN;
    
      if (temp == COMMA || temp == DASH) return(temp);
      if (temp == NUMBER || temp == STRING) return(temp);
    }
  
  /*
   * Joop, at last we have to lookup the token ...
   */
  if (tab)
    {
      i = 0;
      while (tab[i].token != -1)
	if (StrCaseCmp(configRBuf,tab[i].name) == 0)
	  return(tab[i].token);
	else
	  i++;
    }
  
  return(ERROR_TOKEN);       /* Error catcher */
}

/*
 * xf86GetToken --
 *	Lookup a string if it is actually a token in disguise.
 */
static int
getStringToken(tab)
     SymTabRec tab[];
{
  int i;

  for ( i = 0 ; tab[i].token != -1 ; i++ ) {
    if ( ! StrCaseCmp(tab[i].name,val.str) ) return tab[i].token;
  }
  return(ERROR_TOKEN);
}

/*
 * xf86TokenToString --
 *	returns the string corresponding to token
 */
char *
xf86TokenToString(table, token)
     SymTabPtr table;
     int token;
{
  int i;

  for (i = 0; table[i].token >= 0 && table[i].token != token; i++)
    ;
  if (table[i].token < 0)
    return("unknown");
  else
    return(table[i].name);
}
 
/*
 * xf86StringToToken --
 *	returns the string corresponding to token
 */
int
xf86StringToToken(table, string)
     SymTabPtr table;
     char *string;
{
  int i;

  for (i = 0; table[i].token >= 0 && StrCaseCmp(string, table[i].name); i++)
    ;
  return(table[i].token);
}
 
/*
 * xf86ConfigError --
 *      Print a READABLE ErrorMessage!!!  All information that is 
 *      interesting is printed.  Even a pointer to the erroneous place is
 *      printed.  Maybe our e-mail will be less :-)
 */
void
xf86ConfigError(msg)
     char *msg;
{
  int i,j;

  ErrorF( "\nConfig Error: %s:%d\n\n%s", configPath, configLineNo, configBuf);
  for (i = 1, j = 1; i < configStart; i++, j++) 
    if (configBuf[i-1] != '\t')
      ErrorF(" ");
    else
      do
	ErrorF(" ");
      while (((j++)%8) != 0);
  for (i = configStart; i <= configPos; i++) ErrorF("^");
  ErrorF("\n%s\n", msg);
#ifdef NEED_RETURN_VALUE
  return RET_ERROR;
#else
  exit(-1);                 /* simple exit ... */
#endif
}

/*
 * findConfigFile --
 * 	Locate the SERVER_CONFIG_FILE file.  Abort if not found.
 */
static
CONFIG_RETURN_TYPE
findConfigFile(filename, fp)
      char *filename;
      FILE **fp;
{
#define configFile (*fp)
#define MAXPTRIES   6
  char           *home = NULL;
  char           *xconfig = NULL;
  char           *xwinhome = NULL;
  char           *configPaths[MAXPTRIES];
  int            pcount = 0, idx;

  /*
   * First open if necessary the config file.
   * If the -xf86config flag was used, use the name supplied there (root only).
   * If $XF86CONFIG is a pathname, use it as the name of the config file (root)
   * If $XF86CONFIG is set but doesn't contain a '/', append it to 'XF86Config'
   *   and search the standard places (root only).
   * If $XF86CONFIG is not set, just search the standard places.
   */
  while(!configFile) {
    
    /*
     * configPaths[0]			is used as a buffer for -xf86config
     *					and $XF86CONFIG if it contains a path
     * configPaths[1...MAXPTRIES-1]	is used to store the paths of each of
     *					the other attempts
     */
    for(pcount = idx = 0; idx < MAXPTRIES; idx++) {
      configPaths[idx] = NULL;
    }

    /*
     * First check if the -xgsconfig option was used.
     */
    configPaths[pcount] = (char *)xalloc(PATH_MAX);
    if(getuid() == 0 && xf86ConfigFile[0]) {
      strcpy(configPaths[pcount], xf86ConfigFile);
      if ((configFile = fopen(configPaths[pcount], "r")) != 0) {
        break;
      } else {
        FatalError(
             "Cannot read file \"%s\" specified by the -xgsconfig flag\n",
             configPaths[pcount]);
      }
    }
    /*
     * Check if XCONFIGFILE is set.
     */
    if(getuid() == 0 &&
       (xconfig = getenv(CONFIG_ENV_NAME)) != 0 && index(xconfig, '/')) {
      strcpy(configPaths[pcount], xconfig);
      if ((configFile = fopen(configPaths[pcount], "r")) != 0) {
	break;
      } else {
	FatalError("Cannot read file \"%s\" specified by %s variable\n",
		   configPaths[pcount], XCONFIGFILE);
      }
    }

    /*
     * ~/XCONFIGFILE ...
     */
    if(getuid() == 0 && (home = getenv("HOME"))) {
      configPaths[++pcount] = (char *)xalloc(PATH_MAX);
      strcpy(configPaths[pcount],home);
      strcat(configPaths[pcount],"/");
      strcat(configPaths[pcount],XCONFIGFILE);
      if(xconfig) {
	strcat(configPaths[pcount],xconfig);
      }
      if((configFile = fopen( configPaths[pcount], "r" )) != 0) {
	break;
      }
    }
    
    /*
     * /etc/XCONFIGFILE
     */
    configPaths[++pcount] = (char *)xalloc(PATH_MAX);
    strcpy(configPaths[pcount], "/etc/");
    strcat(configPaths[pcount], XCONFIGFILE);
    if(xconfig) {
      strcat(configPaths[pcount],xconfig);
    }
    if((configFile = fopen( configPaths[pcount], "r" )) != 0) {
      break;
    }
    
    /*
     * $(LIBDIR)/SERVER_CONFIG_FILE.<hostname>
     */
    configPaths[++pcount] = (char *)xalloc(PATH_MAX);
    if(getuid() == 0 && (xwinhome = getenv("XWINHOME")) != NULL) {
      sprintf(configPaths[pcount], "%s/lib/X11/%s",
	      xwinhome, SERVER_CONFIG_FILE);
    } else {
      strcpy(configPaths[pcount], SERVER_CONFIG_FILE);
    }
    if(getuid() == 0 && xconfig) {
      strcat(configPaths[pcount],xconfig);
    }
    strcat(configPaths[pcount], ".");
    gethostname(configPaths[pcount]+strlen(configPaths[pcount]),
                MAXHOSTNAMELEN);
    if((configFile = fopen( configPaths[pcount], "r" )) != 0) {
      break;
    }
    
    /*
     * $(LIBDIR)/SERVER_CONFIG_FILE
     */
    configPaths[++pcount] = (char *)xalloc(PATH_MAX);
    if(getuid() == 0 && xwinhome) {
      sprintf(configPaths[pcount], "%s/lib/X11/%s",
	      xwinhome, XCONFIGFILE);
    } else {
      strcpy(configPaths[pcount], SERVER_CONFIG_FILE);
    }
    if(getuid() == 0 && xconfig) {
      strcat(configPaths[pcount],xconfig);
    }

    if((configFile = fopen( configPaths[pcount], "r" )) != 0) {
      break;
    }
    
    ErrorF("\nCould not find config file!\n");
    ErrorF("- Tried:\n");
    for(idx = 1; idx <= pcount; idx++) {
      if(configPaths[idx] != NULL) {
	ErrorF("      %s\n", configPaths[idx]);
      }
    }
    FatalError("No config file found!\n%s", getuid() == 0 ? "" :
      "Note, the X server no longer looks for XCONFIGFILE in $HOME");
  }
  strcpy(filename, configPaths[pcount]);
  if(xf86Verbose) {
    ErrorF("%s: %s\n", XCONFIGFILE, filename);
    ErrorF("%s stands for supplied, %s stands for probed/default values\n",
	   XCONFIG_GIVEN, XCONFIG_PROBED);
  }
  for(idx = 0; idx <= pcount; idx++) {
    if(configPaths[idx] != NULL) {
      xfree(configPaths[idx]);
    }
  }
#undef configFile
#undef MAXPTRIES
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}

static DisplayModePtr pNew, pLast;
static Bool graphFound = FALSE;

/*
 * xf86GetNearestClock --
 *	Find closest clock to given frequency (in kHz).  This assumes the
 *	number of clocks is greater than zero.
 */
int
xf86GetNearestClock(Screen, Frequency)
	ScrnInfoPtr Screen;
	int Frequency;
{
  int NearestClock = 0;
  int MinimumGap = abs(Frequency - Screen->clock[0]);
  int i;
  for (i = 1;  i < Screen->clocks;  i++)
  {
    int Gap = abs(Frequency - Screen->clock[i]);
    if (Gap < MinimumGap)
    {
      MinimumGap = Gap;
      NearestClock = i;
    }
  }
  return NearestClock;
}

/*
 * xf86Config --
 *	Fill some internal structure with userdefined setups. Many internal
 *      Structs are initialized.  The drivers are selected and initialized.
 *	if (! vtopen), XF86Config is read, but devices are not probed.
 *	if (vtopen), devices are probed (and modes resolved).
 *	The vtopen argument was added so that XF86Config information could be
 *	made available before the VT is opened.
 */
CONFIG_RETURN_TYPE
xf86Config (vtopen)
     int vtopen;
{
  int            token;
  int            i, j;
#if defined(SYSV) || defined(linux)
  int            xcpipe[2];
#endif
#ifdef XINPUT
  LocalDevicePtr	local;
#endif
  
 if (!vtopen)
 {

  OFLG_ZERO(&GenericXF86ConfigFlag);
  configBuf  = (char*)xalloc(CONFIG_BUF_LEN);
  configRBuf = (char*)xalloc(CONFIG_BUF_LEN);
  configPath = (char*)xalloc(PATH_MAX);
  
  configBuf[0] = '\0';                    /* sanity ... */
  
  /*
   * Read the XF86Config file with the real uid to avoid security problems
   *
   * For SYSV we fork, and send the data back to the parent through a pipe
   */
#if defined(SYSV) || defined(linux)
  if (getuid() != 0) {
    if (pipe(xcpipe))
      FatalError("Pipe failed (%s)\n", strerror(errno));
    switch (fork()) {
      case -1:
        FatalError("Fork failed (%s)\n", strerror(errno));
        break;
      case 0:   /* child */
        close(xcpipe[0]);
        setuid(getuid());  
        HANDLE_RETURN(findConfigFile(configPath, &configFile));
        {
          unsigned char pbuf[CONFIG_BUF_LEN];
          int nbytes;

          /* Pass the filename back as the first line */
          strcat(configPath, "\n");
          if (write(xcpipe[1], configPath, strlen(configPath)) < 0)
            FatalError("Child error writing to pipe (%s)\n", strerror(errno));
          while ((nbytes = fread(pbuf, 1, CONFIG_BUF_LEN, configFile)) > 0)
            if (write(xcpipe[1], pbuf, nbytes) < 0)
              FatalError("Child error writing to pipe (%s)\n", strerror(errno));
        }
        close(xcpipe[1]);
        fclose(configFile);
        exit(0);
        break;
      default: /* parent */
        close(xcpipe[1]);
        configFile = (FILE *)fdopen(xcpipe[0], "r");
        if (fgets(configPath, PATH_MAX, configFile) == NULL)
          FatalError("Error reading config file\n");
        configPath[strlen(configPath) - 1] = '\0';
    }
  }
  else {
    HANDLE_RETURN(findConfigFile(configPath, &configFile));
  }
#else /* ! (SYSV || linux) */
  {
    int real_uid = getuid();

    if (real_uid) {
      setruid(0);
      seteuid(real_uid);
    }

    HANDLE_RETURN(findConfigFile(configPath, &configFile));
    if (real_uid) {
      seteuid(0);
      setruid(real_uid);
    }
  }
#endif /* SYSV || linux */
  xf86Info.sharedMonitor = FALSE;
  xf86Info.kbdProc = NULL;
  xf86Info.notrapSignals = FALSE;
  xf86Info.caughtSignal = FALSE;

  /* Allocate mouse device */
#if defined(XINPUT) && !defined(XF86SETUP)
  local = mouse_assoc.device_allocate();
  xf86Info.mouseLocal = (pointer) local;
  xf86Info.mouseDev = (MouseDevPtr) local->private;
  xf86Info.mouseDev->mseProc = NULL;
#else
  xf86Info.mouseDev = (MouseDevPtr) xcalloc(1, sizeof(MouseDevRec));
#endif
  
  while ((token = xf86GetToken(TopLevelTab)) != EOF) {
      switch(token) {
      case SECTION:
	  if (xf86GetToken(NULL) != STRING)
	      xf86ConfigError("section name string expected");
	  if ( StrCaseCmp(val.str, "files") == 0 ) {
	      HANDLE_RETURN(configFilesSection());
	  } else if ( StrCaseCmp(val.str, "keyboard") == 0 ) {
	      HANDLE_RETURN(configKeyboardSection());
	  } else if ( StrCaseCmp(val.str, "pointer") == 0 ) {
	      HANDLE_RETURN(configPointerSection(xf86Info.mouseDev, ENDSECTION, NULL));
	  } else if ( StrCaseCmp(val.str, "screen") == 0 ) {
	      HANDLE_RETURN(configScreenSection());
#ifdef XINPUT
	  } else if ( StrCaseCmp(val.str, "xinput") == 0 ) {
	      HANDLE_RETURN(xf86ConfigExtendedInputSection(&val));
#endif
          } else if ( StrCaseCmp(val.str, "module") == 0 ) {
              HANDLE_RETURN(configDynamicModuleSection());
	  } else {
	      xf86ConfigError("not a recognized section name");
	  }
	  break;
      }
  }
  
  fclose(configFile);
  xfree(configBuf);
  xfree(configRBuf);
  xfree(configPath);

  if (modulePath)
      xfree(modulePath);

#if defined(SYSV) || defined(linux)
  if (getuid() != 0) {
    /* Wait for the child */
    wait(NULL);
  }
#endif
  
  /* Try XF86Config FontPath first */
  if (!xf86fpFlag)
    if (fontPath) {
      char *f = xf86ValidateFontPath(fontPath);
      if (*f)
        defaultFontPath = f;
      else
        ErrorF(
        "Warning: FontPath is completely invalid.  Using compiled-in default.\n"
              );
      xfree(fontPath);
      fontPath = (char *)NULL;
    }
    else
      ErrorF("Warning: No FontPath specified, using compiled-in default.\n");
  else    /* Use fontpath specified with '-fp' */
  {
    OFLG_CLR (XCONFIG_FONTPATH, &GenericXF86ConfigFlag);
    if (fontPath)
    {
      xfree(fontPath);
      fontPath = (char *)NULL;
    }
  }
  if (!fontPath) {
    /* xf86ValidateFontPath will write into it's arg, but defaultFontPath
       could be static, so we make a copy. */
    char *f = (char *)xalloc(strlen(defaultFontPath) + 1);
    f[0] = '\0';
    strcpy (f, defaultFontPath);
    defaultFontPath = xf86ValidateFontPath(f);
    xfree(f);
  }
  else
    xfree(fontPath);

  /* If defaultFontPath is still empty, exit here */

  if (! *defaultFontPath)
    FatalError("No valid FontPath could be found\n");
  if (xf86Verbose)
    ErrorF("%s FontPath set to \"%s\"\n", 
      OFLG_ISSET(XCONFIG_FONTPATH, &GenericXF86ConfigFlag) ? XCONFIG_GIVEN :
      XCONFIG_PROBED, defaultFontPath);

  if (!xf86Info.kbdProc)
    FatalError("You must specify a keyboard in %s", XCONFIGFILE);
  if (!xf86Info.mouseDev->mseProc)
    FatalError("You must specify a mouse in %s", XCONFIGFILE);

 }

  RegisterBlockAndWakeupHandlers( xf86Block, xf86Wakeup, (void *)0 ) ;

#ifdef NEED_RETURN_VALUE
 return RET_OKAY;
#endif
}

static char* prependRoot(char *pathname) 
{
#ifndef __EMX__
	return pathname;
#else
	/* XXXX caveat: multiple path components in line */
	return (char*)__XOS2RedirRoot(pathname);
#endif
}
    
static CONFIG_RETURN_TYPE
configFilesSection()
{
  int            token;
  int            i, j;
  int            k, l;
  char           *str;

  while ((token = xf86GetToken(FilesTab)) != ENDSECTION) {
    switch (token) {
    case FONTPATH:
      OFLG_SET(XCONFIG_FONTPATH,&GenericXF86ConfigFlag);
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("Font path component expected");
      }
      j = FALSE;
      str = prependRoot(val.str);
      if(fontPath == NULL) {
	fontPath = (char *)xalloc(1);
	fontPath[0] = '\0';
	i = strlen(str) + 1;
      } else {
	i = strlen(fontPath) + strlen(str) + 1;
	if(fontPath[strlen(fontPath)-1] != ',') {
	  i++;
	  j = TRUE;
	}
      }
      fontPath = (char *)xrealloc(fontPath, i);
      if(j) {
        strcat(fontPath, ",");
      }

      strcat(fontPath, str);
      xfree(val.str);
      break;
      
    case RGBPATH:
      OFLG_SET(XCONFIG_RGBPATH, &GenericXF86ConfigFlag);
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("RGB path expected");
      }
      if(!xf86coFlag) {
        rgbPath = val.str;
      }
      break;

    case MODULEPATH:
      OFLG_SET(XCONFIG_MODULEPATH, &GenericXF86ConfigFlag);
      if (xf86GetToken(NULL) != STRING)
        xf86ConfigError("Module path expected");
      l = FALSE;
      str = prependRoot(val.str);
      if (modulePath == NULL) {
        modulePath = (char *)xalloc(1);
          modulePath[0] = '\0';
          k = strlen(str) + 1;
        }
      else
        {
          k = strlen(modulePath) + strlen(str) + 1;
          if (modulePath[strlen(modulePath)-1] != ',') 
            {
              k++;
              l = TRUE;
            }
        }
      modulePath = (char *)xrealloc(modulePath, k);
      if (l)
        strcat(modulePath, ",");

      strcat(modulePath, str);
      xfree(val.str);
      break;

    case EOF:
      FatalError("Unexpected EOF (missing EndSection?)");
      break; /* :-) */
    default:
      xf86ConfigError("File section keyword expected");
      break;
    }
  }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}


static CONFIG_RETURN_TYPE
configKeyboardSection()
{
  int            token, ntoken;
 
  /* Initialize defaults */
  xf86Info.serverNumLock = FALSE;
  xf86Info.xleds         = 0L;
  xf86Info.kbdDelay      = 500;
  xf86Info.kbdRate       = 30;
  xf86Info.kbdProc       = (DeviceProc)0;
  xf86Info.vtinit        = NULL;
  xf86Info.vtSysreq      = VT_SYSREQ_DEFAULT;
  xf86Info.specialKeyMap = (int *)xalloc((RIGHTCTL - LEFTALT + 1) *
                                            sizeof(int));
  xf86Info.specialKeyMap[LEFTALT - LEFTALT] = KM_META;
  xf86Info.specialKeyMap[RIGHTALT - LEFTALT] = KM_META;
  xf86Info.specialKeyMap[SCROLLLOCK - LEFTALT] = KM_COMPOSE;
  xf86Info.specialKeyMap[RIGHTCTL - LEFTALT] = KM_CONTROL;
#ifdef XKB
  xf86Info.xkbkeymap   = NULL;
  xf86Info.xkbtypes    = "default";
  xf86Info.xkbcompat   = "default";
  xf86Info.xkbkeycodes = "xfree86";
  xf86Info.xkbsymbols  = "us(pc101)";
  xf86Info.xkbgeometry = "pc";
  xf86Info.xkbcomponents_specified    = False;
  xf86Info.xkbrules    = "xfree86";
  xf86Info.xkbmodel    = NULL;
  xf86Info.xkblayout   = NULL;
  xf86Info.xkbvariant  = NULL;
  xf86Info.xkboptions  = NULL;
#endif

  while((token = xf86GetToken(KeyboardTab)) != ENDSECTION) {
    switch(token) {
    case KPROTOCOL:
      if(xf86GetToken(NULL) != STRING) {
        xf86ConfigError("Keyboard protocol name expected");
      }
      if(StrCaseCmp(val.str,"standard") == 0) {
         xf86Info.kbdProc    = xf86KbdProc;
         xf86Info.kbdEvents  = xf86KbdEvents;
      } else if(StrCaseCmp(val.str,"xqueue") == 0) {
#ifdef XQUEUE
        xf86Info.kbdProc = xf86XqueKbdProc;
        xf86Info.kbdEvents = xf86XqueEvents;
        xf86Info.mouseDev->xqueSema  = 0;
        if (xf86Verbose) {
          ErrorF("%s Xqueue selected for keyboard input\n", XCONFIG_GIVEN);
	}
#endif
      } else {
        xf86ConfigError("Not a valid keyboard protocol name");
      }
      break;
    case AUTOREPEAT:
      if(xf86GetToken(NULL) != NUMBER) {
	xf86ConfigError("Autorepeat delay expected");
      }
      xf86Info.kbdDelay = val.num;
      if(xf86GetToken(NULL) != NUMBER) {
	xf86ConfigError("Autorepeat rate expected");
      }
      xf86Info.kbdRate = val.num;
      break;
    case SERVERNUM:
      xf86Info.serverNumLock = TRUE;
      break;

    case XLEDS:
      while((token= xf86GetToken(NULL)) == NUMBER) {
	xf86Info.xleds |= 1L << (val.num-1);
      }
      pushToken = token;
      break;
    case LEFTALT:
    case RIGHTALT:
    case SCROLLLOCK:
    case RIGHTCTL:
      ntoken = xf86GetToken(KeyMapTab);
      if((ntoken == EOF) || (ntoken == STRING) || (ntoken == NUMBER)) {
	xf86ConfigError("KeyMap type token expected");
      } else {
	switch(ntoken) {
	case KM_META:
	case KM_COMPOSE:
	case KM_MODESHIFT:
	case KM_MODELOCK:
	case KM_SCROLLLOCK:
	case KM_CONTROL:
          xf86Info.specialKeyMap[token - LEFTALT] = ntoken;
	  break;
	default:
	  xf86ConfigError("Illegal KeyMap type");
	  break;
	}
      }
      break;
    case VTINIT:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("VTInit string expected");
      }
      xf86Info.vtinit = val.str;
      if(xf86Verbose) {
        ErrorF("%s VTInit: \"%s\"\n", XCONFIG_GIVEN, val.str);
      }
      break;

    case VTSYSREQ:
#ifdef USE_VT_SYSREQ
      xf86Info.vtSysreq = TRUE;
      if(xf86Verbose && !VT_SYSREQ_DEFAULT) {
        ErrorF("%s VTSysReq enabled\n", XCONFIG_GIVEN);
      }
#else
      xf86ConfigError("VTSysReq not supported on this OS");
#endif
      break;

#ifdef XKB
    case XKBDISABLE:
      noXkbExtension = TRUE;
      if(xf86Verbose) {
        ErrorF("%s XKB: disabled\n", XCONFIG_GIVEN);
      }
      break;

    case XKBKEYMAP:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("XKBKeymap string expected");
      }
      xf86Info.xkbkeymap = val.str;
      if(xf86Verbose && !XkbInitialMap) {
        ErrorF("%s XKB: keymap: \"%s\" (overrides other XKB settings)\n",
	       XCONFIG_GIVEN, val.str);
      }
      break;

    case XKBCOMPAT:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("XKBCompat string expected");
      }
      xf86Info.xkbcompat = val.str;
      xf86Info.xkbcomponents_specified = True;
      if(xf86Verbose && !XkbInitialMap) {
        ErrorF("%s XKB: compat: \"%s\"\n", XCONFIG_GIVEN, val.str);
      }
      break;

    case XKBTYPES:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("XKBTypes string expected");
      }
      xf86Info.xkbtypes = val.str;
      xf86Info.xkbcomponents_specified = True;
      if(xf86Verbose && !XkbInitialMap) {
        ErrorF("%s XKB: types: \"%s\"\n", XCONFIG_GIVEN, val.str);
      }
      break;

    case XKBKEYCODES:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("XKBKeycodes string expected");
      }
      xf86Info.xkbkeycodes = val.str;
      xf86Info.xkbcomponents_specified = True;
      if(xf86Verbose && !XkbInitialMap) {
        ErrorF("%s XKB: keycodes: \"%s\"\n", XCONFIG_GIVEN, val.str);
      }
      break;

    case XKBGEOMETRY:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("XKBGeometry string expected");
      }
      xf86Info.xkbgeometry = val.str;
      xf86Info.xkbcomponents_specified = True;
      if(xf86Verbose && !XkbInitialMap) {
        ErrorF("%s XKB: geometry: \"%s\"\n", XCONFIG_GIVEN, val.str);
      }
      break;

    case XKBSYMBOLS:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("XKBSymbols string expected");
      }
      xf86Info.xkbsymbols = val.str;
      xf86Info.xkbcomponents_specified = True;
      if(xf86Verbose && !XkbInitialMap) {
        ErrorF("%s XKB: symbols: \"%s\"\n", XCONFIG_GIVEN, val.str);
      }
      break;

    case XKBRULES:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("XKBRules string expected");
      }
      xf86Info.xkbrules = val.str;
      if(xf86Verbose && !XkbInitialMap) {
        ErrorF("%s XKB: rules: \"%s\"\n", XCONFIG_GIVEN, val.str);
      }
      break;

    case XKBMODEL:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("XKBModel string expected");
      }
      xf86Info.xkbmodel = val.str;
      if(xf86Verbose && !XkbInitialMap) {
        ErrorF("%s XKB: model: \"%s\"\n", XCONFIG_GIVEN, val.str);
      }
      break;

    case XKBLAYOUT:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("XKBLayout string expected");
      }
      xf86Info.xkblayout = val.str;
      if(xf86Verbose && !XkbInitialMap) {
        ErrorF("%s XKB: layout: \"%s\"\n", XCONFIG_GIVEN, val.str);
      }
      break;

    case XKBVARIANT:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("XKBVariant string expected");
      }
      xf86Info.xkbvariant = val.str;
      if(xf86Verbose && !XkbInitialMap) {
        ErrorF("%s XKB: variant: \"%s\"\n", XCONFIG_GIVEN, val.str);
      }
      break;

    case XKBOPTIONS:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("XKBOptions string expected");
      }
      xf86Info.xkboptions = val.str;
      if(xf86Verbose && !XkbInitialMap) {
        ErrorF("%s XKB: options: \"%s\"\n", XCONFIG_GIVEN, val.str);
      }
      break;
#endif

    case EOF:
      FatalError("Unexpected EOF (missing EndSection?)");
      break; /* :-) */

    default:
      xf86ConfigError("Keyboard section keyword expected");
      break;
    }
  }
  if(xf86Info.kbdProc == (DeviceProc)0) {
    xf86ConfigError("No keyboard device given");
  }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}
      
CONFIG_RETURN_TYPE
configPointerSection(
  MouseDevPtr	mouse_dev,
  int		end_tag,
  char		**devicename) /* used by extended device */
{
  int            token;
  int		 mtoken;
  int            i;
  char *mouseType = "unknown";

  /* Set defaults */
  mouse_dev->baudRate        = 1200;
  mouse_dev->oldBaudRate     = -1;
  mouse_dev->sampleRate      = 0;
  mouse_dev->resolution      = 0;
  mouse_dev->buttons         = MSE_DFLTBUTTONS;
  mouse_dev->emulate3Buttons = FALSE;
  mouse_dev->emulate3Timeout = 50;
  mouse_dev->chordMiddle     = FALSE;
  mouse_dev->mouseFlags      = 0;
  mouse_dev->mseProc         = (DeviceProc)0;
  mouse_dev->mseDevice       = NULL;
  mouse_dev->mseType         = -1;
  mouse_dev->mseModel        = 0;
  mouse_dev->negativeZ       = 0;
  mouse_dev->positiveZ       = 0;
      
  while((token = xf86GetToken(PointerTab)) != end_tag) {
    switch (token) {

    case PROTOCOL:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("Mouse name expected");
      }
#ifdef XQUEUE
      if ( StrCaseCmp(val.str,"xqueue") == 0 ) {
        mouse_dev->mseProc   = xf86XqueMseProc;
        mouse_dev->mseEvents = (void(*)(MouseDevPtr))xf86XqueEvents;
        mouse_dev->xqueSema  = 0;
        if (xf86Verbose)
          ErrorF("%s Xqueue selected for mouse input\n",
	         XCONFIG_GIVEN);
        break;
      }
#endif

      mouseType = (char *)strdup(val.str); /* GJA -- should we free this? */

      mtoken = getStringToken(MouseTab); /* Which mouse? */
      mouse_dev->mseProc    = xf86MseProc;
      mouse_dev->mseEvents  = xf86MseEvents;

      mouse_dev->mseType    = mtoken - MICROSOFT;
      if(!xf86MouseSupported(mouse_dev->mseType)) {
        xf86ConfigError("Mouse type not supported by this OS");
      }
      break;

    case PDEVICE:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("Mouse device expected");
      }
      mouse_dev->mseDevice  = val.str;
      break;
    case BAUDRATE:
      if(xf86GetToken(NULL) != NUMBER) {
	xf86ConfigError("Baudrate expected");
      }
      if(mouse_dev->mseType + MICROSOFT == LOGIMAN) {
	if ((val.num != 1200) && (val.num != 9600))
	  xf86ConfigError("Only 1200 or 9600 Baud are supported by MouseMan");
      } else if (val.num%1200 != 0 || val.num < 1200 || val.num > 9600) {
	xf86ConfigError("Baud rate must be one of 1200, 2400, 4800, or 9600");
      }
      mouse_dev->baudRate = val.num;
      break;

    case SAMPLERATE:
      if(xf86GetToken(NULL) != NUMBER) {
	xf86ConfigError("Sample rate expected");
      }
      mouse_dev->sampleRate = val.num;
      break;

    case PRESOLUTION:
      if(xf86GetToken(NULL) != NUMBER) {
	xf86ConfigError("Resolution expected");
      }
      if(val.num <= 0) {
	xf86ConfigError("Resolution must be a positive value");
      }
      mouse_dev->resolution = val.num;
      break;

    case EMULATE3:
      if (mouse_dev->chordMiddle)
        xf86ConfigError("Can't use Emulate3Buttons with ChordMiddle");
      mouse_dev->emulate3Buttons = TRUE;
      break;

    case EM3TIMEOUT:
      if(xf86GetToken(NULL) != NUMBER) {
        xf86ConfigError("3 button emulation timeout expected");
      }
      mouse_dev->emulate3Timeout = val.num;
      break;

    case CHORDMIDDLE:
      if (mouse_dev->mseType + MICROSOFT == MICROSOFT ||
          mouse_dev->mseType + MICROSOFT == LOGIMAN) {
        if (mouse_dev->emulate3Buttons) {
          xf86ConfigError("Can't use ChordMiddle with Emulate3Buttons");
	}
        mouse_dev->chordMiddle = TRUE;
      } else {
        xf86ConfigError("ChordMiddle is only supported for Microsoft and MouseMan");
      }
      break;

    case CLEARDTR:
      xf86ConfigError("ClearDTR not supported on this OS");
      break;

    case CLEARRTS:
      xf86ConfigError("ClearRTS not supported on this OS");
      break;

    case DEVICE_NAME:
      if(!devicename) {	/* not called for an extended device */
	xf86ConfigError("Pointer section keyword expected");
      }

      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("Option string expected");
      }
      *devicename = strdup(val.str);
      break;

#ifdef XINPUT
    case ALWAYSCORE:
      xf86AlwaysCore(mouse_dev->local, TRUE);
      break;
#endif
	
    case ZAXISMAPPING:
      switch (xf86GetToken(ZMapTab)) {
      case NUMBER:
        if(val.num <= 0 || val.num > MSE_MAXBUTTONS) {
	  xf86ConfigError("Button number (1..12) expected");
	}
        mouse_dev->negativeZ = 1 << (val.num - 1);
        if(xf86GetToken(NULL) != NUMBER || 
	   val.num <= 0 || val.num > MSE_MAXBUTTONS) {
	  xf86ConfigError("Button number (1..12) expected");
	}
        mouse_dev->positiveZ = 1 << (val.num - 1);
        break;
      case XAXIS:
        mouse_dev->negativeZ = mouse_dev->positiveZ = MSE_MAPTOX;
	break;
      case YAXIS:
        mouse_dev->negativeZ = mouse_dev->positiveZ = MSE_MAPTOY;
	break;
      default:
	xf86ConfigError("Button number (1..12), X or Y expected");
      }
      break;

    case PBUTTONS:
      if(xf86GetToken(NULL) != NUMBER) {
	xf86ConfigError("Number of buttons (1..12) expected");
      }
      if(val.num <= 0 || val.num > MSE_MAXBUTTONS) {
	xf86ConfigError("Number of buttons must be a positive value (1..12)");
      }
      mouse_dev->buttons = val.num;
      break;

    case EOF:
      FatalError("Unexpected EOF (missing EndSection?)");
      break; /* :-) */
      
    default:
      xf86ConfigError("Pointer section keyword expected");
      break;
    }

  }
  /* Print log and make sanity checks */

  if(mouse_dev->mseProc == (DeviceProc)0) {
    xf86ConfigError("No mouse protocol given");
  }
  
  /*
   * if mseProc is set and mseType isn't, then using Xqueue or OSmouse.
   * Otherwise, a mouse device is required.
   */
  if(mouse_dev->mseType >= 0 && !mouse_dev->mseDevice) {
    xf86ConfigError("No mouse device given");
  }

  switch (mouse_dev->negativeZ) {
  case 0: /* none */
  case MSE_MAPTOX:
  case MSE_MAPTOY:
    break;
  default: /* buttons */
    for (i = 0; mouse_dev->negativeZ != (1 << i); ++i)
      ;
    if(i + 1 > mouse_dev->buttons) {
      mouse_dev->buttons = i + 1;
    }
    for (i = 0; mouse_dev->positiveZ != (1 << i); ++i)
      ;
    if(i + 1 > mouse_dev->buttons) {
      mouse_dev->buttons = i + 1;
    }
    break;
  }

  if(xf86Verbose && mouse_dev->mseType >= 0)
  {
    Bool formatFlag = FALSE;
    ErrorF("%s Mouse: type: %s, device: %s", 
	   XCONFIG_GIVEN, mouseType, mouse_dev->mseDevice);
    if (mouse_dev->mseType != P_BM
	&& mouse_dev->mseType != P_PS2
	&& mouse_dev->mseType != P_IMPS2
	&& mouse_dev->mseType != P_THINKINGPS2
	&& mouse_dev->mseType != P_MMANPLUSPS2
	&& mouse_dev->mseType != P_GLIDEPOINTPS2
	&& mouse_dev->mseType != P_NETPS2
	&& mouse_dev->mseType != P_NETSCROLLPS2
	&& mouse_dev->mseType != P_SYSMOUSE) {
      formatFlag = TRUE;
      ErrorF(", baudrate: %d", mouse_dev->baudRate);
    }
    if(mouse_dev->sampleRate) {
      ErrorF(formatFlag ? "\n%s Mouse: samplerate: %d" : "%ssamplerate: %d", 
	     formatFlag ? XCONFIG_GIVEN : ", ", mouse_dev->sampleRate);
      formatFlag = !formatFlag;
    }
    if(mouse_dev->resolution) {
      ErrorF(formatFlag ? "\n%s Mouse: resolution: %d" : "%sresolution: %d", 
	     formatFlag ? XCONFIG_GIVEN : ", ", mouse_dev->resolution);
      formatFlag = !formatFlag;
    }
    ErrorF(formatFlag ? "\n%s Mouse: buttons: %d" : "%sbuttons: %d",
	   formatFlag ? XCONFIG_GIVEN : ", ", mouse_dev->buttons);
    formatFlag = !formatFlag;
    if(mouse_dev->emulate3Buttons) {
      ErrorF(formatFlag ? "\n%s Mouse: 3 button emulation (timeout: %dms)" :
	                  "%s3 button emulation (timeout: %dms)",
             formatFlag ? XCONFIG_GIVEN : ", ", mouse_dev->emulate3Timeout);
      formatFlag = !formatFlag;
    }
    if(mouse_dev->chordMiddle) {
      ErrorF(formatFlag ? "\n%s Mouse: Chorded middle button" : 
                          "%sChorded middle button",
             formatFlag ? XCONFIG_GIVEN : ", ");
    }
    ErrorF("\n");

    switch (mouse_dev->negativeZ) {
    case 0: /* none */
      break;
    case MSE_MAPTOX:
      ErrorF("%s Mouse: zaxismapping: X\n", XCONFIG_GIVEN);
      break;
    case MSE_MAPTOY:
      ErrorF("%s Mouse: zaxismapping: Y\n", XCONFIG_GIVEN);
      break;
    default: /* buttons */
      for (i = 0; mouse_dev->negativeZ != (1 << i); ++i)
	;
      ErrorF("%s Mouse: zaxismapping: (-)%d", XCONFIG_GIVEN, i + 1);
      for (i = 0; mouse_dev->positiveZ != (1 << i); ++i)
	;
      ErrorF(" (+)%d\n", i + 1);
      break;
    }
  }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}
      

static int CheckVideoMode(char *VideoMode)
{
  int i;

  for(i = 0; VideoModeTab[i].token != -1; i++) {
    if(StrCaseCmp(VideoMode, VideoModeTab[i].name) == 0) {
      return VideoModeTab[i].token;
    }
  }
  return ERROR_TOKEN;
}

static CONFIG_RETURN_TYPE
configDynamicModuleSection()
{
    int		token;
 
    while ((token = xf86GetToken(ModuleTab)) != ENDSECTION) {
	switch (token) {
	case LOAD:
	    if (xf86GetToken(NULL) != STRING)
		xf86ConfigError("Dynamic module expected");
	    else {
#ifdef DYNAMIC_MODULE
		if (!modulePath) {
		    static Bool firstTime = TRUE;

		    modulePath = (char*)xcalloc(1, strlen(DEFAULT_MODULE_PATH)+1);
		    strcpy(modulePath, DEFAULT_MODULE_PATH);
		
		    if (xf86Verbose && firstTime) {
			ErrorF("%s no ModulePath specified using default: %s\n",
			       XCONFIG_PROBED, DEFAULT_MODULE_PATH);
			firstTime = FALSE;
		    }
		}
		xf86LoadModule(val.str, modulePath);
#else
		ErrorF("Dynamic modules not supported. \"%s\" not loaded\n",
		       val.str);
#endif
	    }
	    break;

	case EOF:
	    FatalError("Unexpected EOF. Missing EndSection?");
	    break; /* :-) */
	    
	default:
	    xf86ConfigError("Module section keyword expected");
	    break;
	}    
    }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}

static CONFIG_RETURN_TYPE
configScreenSection()
{
  int i, j;
  int driverno;
  int had_monitor = 0, had_device = 0;
  int dispIndex = 0;
  int numDisps = 0;
  DispPtr dispList = NULL;
  DispPtr dispp;
  int depth = 0;
  int screenno = 0;
  int scrn_width = 0, scrn_height = 0;
  int framerate = 0;
  int interlacemix = -1;
  int interlace = -1;
  char *VideoMode = NULL;
  char *ScanMode  = NULL;
  int videomode = -1;
  int vmode = -1;
      
  int token;
  ScrnInfoPtr screen = NULL;
  int textClockValue = -1;

  token = xf86GetToken(ScreenTab);
  if(token != DRIVER) {
    xf86ConfigError("The screen section must begin with the 'driver' line");
  }
  if(xf86GetToken(NULL) != STRING) {
    xf86ConfigError("Driver name expected");
  }
  driverno = getStringToken(DriverTab);
  switch(driverno) {
  case GS:
    break;
  default:
    xf86ConfigError("Not a recognized driver name");
  }

  while((token = xf86GetToken(ScreenTab)) != ENDSECTION) {
    switch(token) {
    case DEFBPP:
      if (xf86GetToken(NULL) != NUMBER) {
        xf86ConfigError("Default color depth expected");
      }
      depth = val.num;
      break;

    case SCREENNO:
      if (xf86GetToken(NULL) != NUMBER) {
	xf86ConfigError("Screen number expected");
      }
      screenno = val.num;
      break;

    case SUBSECTION:
      if((xf86GetToken(NULL) != STRING) ||
	 (StrCaseCmp(val.str, "display") != 0)) {
        xf86ConfigError("You must say \"Display\" here");
      }
      if (dispList == NULL) {
        dispList = (DispPtr)xalloc(sizeof(DispRec));
      } else {
        dispList = (DispPtr)xrealloc(dispList,
				     (numDisps + 1) * sizeof(DispRec));
      }
      dispp = dispList + numDisps;
      numDisps++;
      dispp->depth = -1;
      dispp->modes = NULL;
      dispp->defaultVisual = -1;

      configDisplaySubsection(dispp);
      break;

    case EOF:
      FatalError("Unexpected EOF (missing EndSection?)");
      break; /* :-) */

    case MDEVICE:
      if (xf86GetToken(NULL) != STRING) {
	xf86ConfigError("Device name expected");
      }
      break;
    case STANDBYTIME:
      if(xf86GetToken(NULL) != NUMBER) {
	xf86ConfigError("Screensaver standby time expected");
      }
#ifdef DPMSExtension
      defaultDPMSStandbyTime = DPMSStandbyTime = val.num * MILLI_PER_MIN;
#endif
      break;

    case SUSPENDTIME:
      if(xf86GetToken(NULL) != NUMBER) {
	xf86ConfigError("Screensaver suspend time expected");
      }
#ifdef DPMSExtension
        defaultDPMSSuspendTime = DPMSSuspendTime = val.num * MILLI_PER_MIN;
#endif
      break;

    case OFFTIME:
      if(xf86GetToken(NULL) != NUMBER) {
	xf86ConfigError("Screensaver off time expected");
      }
#ifdef DPMSExtension
      defaultDPMSOffTime = DPMSOffTime = val.num * MILLI_PER_MIN;
#endif
      break;
    case FRAMERATE:
      if(xf86GetToken(NULL) != NUMBER) {
	xf86ConfigError("FrameRate is 60Hz or 75Hz");
      }
      framerate = val.num;
      break;

    case INTERLACEMIX:
      if(xf86GetToken(NULL) != NUMBER) {
	xf86ConfigError("Interlace-Mix is 0 - 100");
      }
      interlacemix = val.num;
      break;

    case VIDEOMODE:
      if(xf86GetToken(NULL) != STRING) {
	xf86ConfigError("VideoMode Mode ScanMethod");
      }
      VideoMode = val.str;
      if((token = xf86GetToken(TimingTab)) == STRING) {
	ScanMode = val.str;
      } else {
	pushToken = token;
      }
      break;

    default:
      xf86ConfigError("Screen section keyword expected");
      break;
    }
  }

  if(dispList == NULL) {
    FatalError(
	"A \"Display\" subsection is required in each \"Screen\" section\n");
  }

  if (xf86bpp < 0) { 
    if (numDisps == 1) {
      if(dispList[0].depth > 0) {
	depth = dispList[0].depth;
      }
      dispIndex = 0;
    } else {
      /* Look for a section which matches the driver's default depth */
      for (dispIndex = 0; dispIndex < numDisps; dispIndex++) {
	if (dispList[dispIndex].depth == depth)
	  break;
      }
      if (dispIndex == numDisps) {
	/* No match.  This time, allow 15/16 and 24/32 to match */
	for (dispIndex = 0; dispIndex < numDisps; dispIndex++) {
	  if ((depth == 15 && dispList[dispIndex].depth == 16) ||
	      (depth == 16 && dispList[dispIndex].depth == 15) ||
	      (depth == 24 && dispList[dispIndex].depth == 32) ||
	      (depth == 32 && dispList[dispIndex].depth == 24))
	    break;
	}
      }
      if (dispIndex == numDisps) {
	/* Still no match, so exit */
	FatalError("No \"Display\" subsection for default depth %d\n",
		   depth);
      }
    }
  } else {
#if 0	/* enable command line option */
    /* xf86bpp is set */
    if (numDisps == 1 && dispList[0].depth < 0) {
      dispIndex = 0;
    } else {
      /* find Display subsection matching xf86bpp */
      for (dispIndex = 0; dispIndex < numDisps; dispIndex++) {
	if (dispList[dispIndex].depth == xf86bpp)
	  break;
      }
      if (dispIndex == numDisps) {
	/* No match.  This time, allow 15/16 and 24/32 to match */
	for (dispIndex = 0; dispIndex < numDisps; dispIndex++) {
	  if ((xf86bpp == 15 && dispList[dispIndex].depth == 16) ||
	      (xf86bpp == 16 && dispList[dispIndex].depth == 15) ||
	      (xf86bpp == 24 && dispList[dispIndex].depth == 32) ||
	      (xf86bpp == 32 && dispList[dispIndex].depth == 24))
	    break;
	}
      }
    }
#endif
  }

  /* Now copy the info across to the screen rec */
  dispp = dispList + dispIndex;
  if(xf86bpp <= 0) {	/* xf86bpp > 0 is enable command line option */
    int res;
    char c;

    if (dispp->depth > 0) {
      depth = dispp->depth;
    }
    vmode = CheckVideoMode(VideoMode);
    if(vmode < 0) {
      FatalError("VideoMode Mode ScanMethod");
    }
    switch(vmode) {
    case VM_NTSC:
    case VM_PAL:
#if 0	/* STRICT Check */
      if(dispp->modes != NULL) {
	FatalError("Don't Need Modes");
      }
#endif
      interlace = PS2_GS_INTERLACE;
      if(ScanMode != NULL) {
	if(StrCaseCmp(ScanMode, "nointerlace") == 0) {
	  interlace = PS2_GS_NOINTERLACE;
	}
      }
      scrn_width = 640;
      if(vmode == VM_NTSC) {
	scrn_height = 448;
	videomode = PS2_GS_NTSC;
      } else {
	scrn_height = 512;
	videomode = PS2_GS_PAL;
      }
      if(interlace == PS2_GS_NOINTERLACE) {
	scrn_height /= 2;
      }
      SetGSXTVScreen(screenno, videomode, scrn_width, scrn_height, depth, interlace);
      if(interlacemix >= 0 && interlacemix <= 100) {
	SetGSXInterlaceMix(screenno, interlacemix);
      }
      break;

    case VM_DTV:
      if(sscanf(dispp->modes->name, "%d%c", &scrn_height, &c) != 2) {
	FatalError("A \"Display\" subsection: Illegale Modes Format");
      }
      c = tolower(c);
      switch(scrn_height) {
      case 480:
	if(c != 'p') {
	  FatalError("A \"Display\" subsection: Illegale Modes Format");
	}
	scrn_width = 720;
	break;
      case 720:
	if(c != 'p') {
	  FatalError("A \"Display\" subsection: Illegale Modes Format");
	}
	scrn_width = 1024;
	break;
      case 1080:
	if(c != 'i') {
	  FatalError("A \"Display\" subsection: Illegale Modes Format");
	}
	if(depth != 16) {
	  FatalError("Illegale depth");
	}
	scrn_width = 1920;
	break;
      default:
	FatalError("A \"Display\" subsection: Illegale Modes Format");
	break;
      }
      SetGSXTVScreen(screenno, PS2_GS_DTV, scrn_width, scrn_height, depth, -1);
      if(interlacemix >= 0 && interlacemix <= 100) {
	SetGSXInterlaceMix(screenno, interlacemix);
      }
      break;

    case VM_VESA:
      if(ScanMode != NULL) {
	FatalError("Don't Need ScanMode");
      }
      if(sscanf(dispp->modes->name, "%dx%d", &scrn_width, &scrn_height) != 2) {
	FatalError("A \"Display\" subsection: Illegale Modes Format");
      }
      if((res = checkResolution(scrn_width, scrn_height)) < 0) {
	FatalError("Can't Support Resolution");
      }
      SetGSXVesaScreen(screenno, scrn_width, scrn_height, depth, res);
      if(framerate > 0) {
	SetGSXFrameRate(screenno, framerate);
      }
      break;

    default:
      FatalError("Unknown VideoMode");
      break;
    }
  }

  /* Don't need them any more */
  xfree(dispList);

#ifdef NEED_RETURN_VALUE
    return RET_OKAY;
#endif
}

static CONFIG_RETURN_TYPE
configDisplaySubsection(disp)
DispPtr disp;
{
  int token;
  int i;

  while ((token = xf86GetToken(DisplayTab)) != ENDSUBSECTION) {
    switch (token) {
    case DEPTH:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Display depth expected");
      disp->depth = val.num;
      break;

    case MODES:
      for (pLast=NULL; (token = xf86GetToken(NULL)) == STRING; pLast = pNew)
	{
	  pNew = (DisplayModePtr)xalloc(sizeof(DisplayModeRec));
	  pNew->name = val.str;
	  pNew->PrivSize = 0;
	  pNew->Private = NULL;

	  if (pLast) 
	    {
	      pLast->next = pNew;
	      pNew->prev  = pLast;
	    }
	  else
	    disp->modes = pNew;
	}
      /* Make sure at least one mode was present */
      if (!pLast)
	 xf86ConfigError("Mode name expected");
      pNew->next = disp->modes;
      disp->modes->prev = pLast;
      pushToken = token;
      break;

    case BLACK:
    case WHITE:
      {
        unsigned char rgb[3];
        int ii;
        
        for (ii = 0; ii < 3; ii++)
        {
          if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("RGB value expected");
          rgb[ii] = val.num & 0x3F;
        }
        if (token == BLACK)
        {
          disp->blackColour.red = rgb[0];
          disp->blackColour.green = rgb[1];
          disp->blackColour.blue = rgb[2];
        }
        else
        {
          disp->whiteColour.red = rgb[0];
          disp->whiteColour.green = rgb[1];
          disp->whiteColour.blue = rgb[2];
        }
      }
      break;

    case EOF:
      FatalError("Unexpected EOF (missing EndSubSection)");
      break; /* :-) */

    default:
      xf86ConfigError("Display subsection keyword expected");
      break;
    }
  }
#ifdef NEED_RETURN_VALUE
  return RET_OKAY;
#endif
}

/* Note, the characters '_', ' ', and '\t' are ignored in the comparison */
int
StrCaseCmp(s1, s2)
     const char *s1, *s2;
{
  char c1, c2;

  if (*s1 == 0)
    if (*s2 == 0)
      return(0);
    else
      return(1);
  while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
    s1++;
  while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
    s2++;
  c1 = (isupper(*s1) ? tolower(*s1) : *s1);
  c2 = (isupper(*s2) ? tolower(*s2) : *s2);
  while (c1 == c2)
    {
      if (c1 == '\0')
	return(0);
      s1++; s2++;
      while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
	s1++;
      while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
	s2++;
      c1 = (isupper(*s1) ? tolower(*s1) : *s1);
      c2 = (isupper(*s2) ? tolower(*s2) : *s2);
    }
  return(c1 - c2);
}
