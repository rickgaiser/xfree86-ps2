/* $TOG: keycodes.c /main/13 1997/06/13 05:59:12 kaleb $ */
/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be 
 used in advertising or publicity pertaining to distribution 
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability 
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.
 
 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#include "xkbcomp.h"
#include "tokens.h"
#include "expr.h"
#include "keycodes.h"
#include "misc.h"
#include "alias.h"

char *
#if NeedFunctionPrototypes
longText(unsigned long val,unsigned format)
#else
longText(val,format)
    unsigned long val;
    unsigned format;
#endif
{
char buf[4];

    LongToKeyName(val,buf);
    return XkbKeyNameText(buf,format);
}

/***====================================================================***/

void
#if NeedFunctionPrototypes
LongToKeyName(unsigned long val,char *name)
#else
LongToKeyName(val,name)
    unsigned long 	val;
    char *		name;
#endif
{
    name[0]= ((val>>24)&0xff);
    name[1]= ((val>>16)&0xff);
    name[2]= ((val>>8)&0xff);
    name[3]= (val&0xff);
    return;
}

/***====================================================================***/

typedef struct _IndicatorNameInfo {
    CommonInfo		defs;
    int			ndx;
    Atom		name;
    Bool		virtual;
} IndicatorNameInfo;

typedef struct _KeyNamesInfo {
    char *		name;
    int			errorCount;
    unsigned		fileID;
    unsigned		merge;
    int			computedMin;
    int			computedMax;
    int			explicitMin;
    int			explicitMax;
    int			effectiveMin;
    int			effectiveMax;
    unsigned long	names[XkbMaxLegalKeyCode+1];
    unsigned 		files[XkbMaxLegalKeyCode+1];
    unsigned char	has_alt_forms[XkbMaxLegalKeyCode+1];
    IndicatorNameInfo *	leds;
    AliasInfo *		aliases;
} KeyNamesInfo;

static void
#if NeedFunctionPrototypes
InitIndicatorNameInfo(IndicatorNameInfo *ii,KeyNamesInfo *info)
#else
InitIndicatorNameInfo(ii,info)
    IndicatorNameInfo *	ii;
    KeyNamesInfo *		info;
#endif
{
    ii->defs.defined= 0;
    ii->defs.merge= info->merge;
    ii->defs.fileID= info->fileID;
    ii->defs.next= NULL;
    ii->ndx= 0;
    ii->name= None;
    ii->virtual= False;
    return;
}

static void 
#if NeedFunctionPrototypes
ClearIndicatorNameInfo(IndicatorNameInfo *ii,KeyNamesInfo *info)
#else
ClearIndicatorNameInfo(ii,info)
    IndicatorNameInfo *	ii;
    KeyNamesInfo *		info;
#endif
{
    if (ii==info->leds) {
	ClearCommonInfo(&ii->defs);
	info->leds= NULL;
    }
    return;
}

static IndicatorNameInfo *
#if NeedFunctionPrototypes
NextIndicatorName(KeyNamesInfo *info)
#else
NextIndicatorName(info)
    KeyNamesInfo *	info;
#endif
{
IndicatorNameInfo *	ii;

    ii= uTypedAlloc(IndicatorNameInfo);
    if (ii) {
	InitIndicatorNameInfo(ii,info);
	info->leds= (IndicatorNameInfo *)AddCommonInfo(&info->leds->defs,
							(CommonInfo *)ii);
    }
    return ii;
}

static IndicatorNameInfo *
#if NeedFunctionPrototypes
FindIndicatorByIndex(KeyNamesInfo *info,int ndx)
#else
FindIndicatorByIndex(info,ndx)
    KeyNamesInfo *	info;
    int			ndx;
#endif
{
IndicatorNameInfo *	old;

    for (old= info->leds;old!=NULL;old=(IndicatorNameInfo *)old->defs.next) {
	if (old->ndx==ndx)
	    return old;
    }
    return NULL;
}

static IndicatorNameInfo *
#if NeedFunctionPrototypes
FindIndicatorByName(KeyNamesInfo *info,Atom name)
#else
FindIndicatorByName(info,name)
    KeyNamesInfo *	info;
    Atom		name;
#endif
{
IndicatorNameInfo *	old;

    for (old= info->leds;old!=NULL;old=(IndicatorNameInfo *)old->defs.next) {
	if (old->name==name)
	    return old;
    }
    return NULL;
}

static Bool
#if NeedFunctionPrototypes
AddIndicatorName(KeyNamesInfo *info,IndicatorNameInfo *new)
#else
AddIndicatorName(info,new)
    KeyNamesInfo *	info;
    IndicatorNameInfo *new;
#endif
{
IndicatorNameInfo *old;
Bool 		   replace;
char *		   action;

    replace= (new->defs.merge==MergeReplace)||
		 (new->defs.merge==MergeOverride);
    old= FindIndicatorByName(info,new->name);
    if (old) {
	if (((old->defs.fileID==new->defs.fileID)&&(warningLevel>0))||
		    					(warningLevel>9)) {
	    WARN1("Multiple indicators named %s\n",
	    				XkbAtomText(NULL,new->name,XkbMessage));
	    if (old->ndx==new->ndx) {
		if (old->virtual!=new->virtual) {
		    if (replace)
			 old->virtual= new->virtual;
		    action= "Using %s instead of %s\n";
		}
		else {
		    action= "Identical definitions ignored\n";
		}
		ACTION2(action,(old->virtual?"virtual":"real"),
				(old->virtual?"real":"virtual"));
		return True;
	    }
	    else {
		if (replace)	action= "Ignoring %d, using %d\n";
		else		action= "Using %d, ignoring %d\n";
		ACTION2(action,old->ndx,new->ndx);
	    }
	    if (replace) {
		if (info->leds==old)
		    info->leds= (IndicatorNameInfo *)old->defs.next;
		else {
		    IndicatorNameInfo *tmp;
		    tmp= info->leds;
		    for (;tmp!=NULL;tmp=(IndicatorNameInfo *)tmp->defs.next) {
			if (tmp->defs.next==(CommonInfo *)old) {
			    tmp->defs.next= old->defs.next;
			    break;
			}
		    }
		}
		uFree(old);
	    }
	}
    }
    old= FindIndicatorByIndex(info,new->ndx);
    if (old) {
	if (((old->defs.fileID==new->defs.fileID)&&(warningLevel>0))||
		    					(warningLevel>9)) {
	    WARN1("Multiple names for indicator %d\n",new->ndx);
	    if ((old->name==new->name)&&(old->virtual==new->virtual))
		action= "Identical definitions ignored\n";
	    else {
		char   *oldType,*newType;
		Atom	using,ignoring;
		if (old->virtual)	oldType= "virtual indicator";
		else			oldType= "real indicator";
		if (new->virtual)	newType= "virtual indicator";
		else			newType= "real indicator";
		if (replace) {
		    using= new->name;
		    ignoring= old->name;
		}
		else {
		    using= old->name;
		    ignoring= new->name;
		}
		ACTION4("Using %s %s, ignoring %s %s\n",
				oldType,XkbAtomText(NULL,using,XkbMessage),
				newType,XkbAtomText(NULL,ignoring,XkbMessage));
	    }
	}
	if (replace) {
	    old->name= new->name;
	    old->virtual= new->virtual;
	}
	return True;
    }
    old= new;
    new= NextIndicatorName(info);
    if (!new) {
	WSGO1("Couldn't allocate name for indicator %d\n",new->ndx);
	ACTION("Ignored\n");
	return False;
    }
    new->name= old->name;
    new->ndx= old->ndx;
    new->virtual= old->virtual;
    return True;
}

static void
#if NeedFunctionPrototypes
ClearKeyNamesInfo(KeyNamesInfo *info)
#else
ClearKeyNamesInfo(info)
    KeyNamesInfo *info;
#endif
{
    if (info->name!=NULL)
	uFree(info->name);
    info->name= NULL;
    info->computedMax= info->explicitMax= info->explicitMin= -1;
    info->computedMin= 256;
    info->effectiveMin= 8;
    info->effectiveMax= 255;
    bzero((char *)info->names,sizeof(info->names));
    bzero((char *)info->files,sizeof(info->files));
    bzero((char *)info->has_alt_forms,sizeof(info->has_alt_forms));
    if (info->leds)
	ClearIndicatorNameInfo(info->leds,info);
    if (info->aliases)
	ClearAliases(&info->aliases);
    return;
}

static void
#if NeedFunctionPrototypes
InitKeyNamesInfo(KeyNamesInfo *info)
#else
InitKeyNamesInfo(info)
    KeyNamesInfo *info;
#endif
{
    info->name= NULL;
    info->leds= NULL;
    info->aliases= NULL;
    ClearKeyNamesInfo(info);
    info->errorCount= 0;
    return;
}

int
#if NeedFunctionPrototypes
FindKeyByLong(KeyNamesInfo *info,unsigned long name)
#else
FindKeyByLong(info,name)
    KeyNamesInfo *	info;
    unsigned long	name;
#endif
{
register int i;

    for (i=info->effectiveMin;i<=info->effectiveMax;i++) {
	if (info->names[i]==name)
	    return i;
    }
    return 0;
}

static Bool
#if NeedFunctionPrototypes
AddKeyName(	KeyNamesInfo *	info,
		int		kc,
		char *		name,
		unsigned	merge,
		unsigned	fileID,
		Bool		reportCollisions)
#else
AddKeyName(info,kc,name,merge,fileID,reportCollisions)
    KeyNamesInfo *	info;
    int			kc;
    char *		name;
    unsigned		merge;
    unsigned		fileID;
    Bool		reportCollisions;
#endif
{
int		old;
unsigned long	lval;

    if ((kc<info->effectiveMin)||(kc>info->effectiveMax)) {
	ERROR2("Illegal keycode %d for name <%s>\n",kc,name);
	ACTION2("Must be in the range %d-%d inclusive\n",info->effectiveMin,
							 info->effectiveMax);
	return False;
    }
    if (kc<info->computedMin)	info->computedMin= kc;
    if (kc>info->computedMax)	info->computedMax= kc;
    lval= KeyNameToLong(name);

    if (reportCollisions) {
	reportCollisions= ((warningLevel>7)||
			   ((warningLevel>0)&&(fileID==info->files[kc])));
    }

    if (info->names[kc]!=0) {
	char buf[6];

	LongToKeyName(info->names[kc],buf);
	buf[4]= '\0';
	if (info->names[kc]==lval) {
	    if (info->has_alt_forms[kc] || (merge==MergeAltForm)) {
		info->has_alt_forms[kc]= True;
	    }
	    else if (reportCollisions) {
		WARN("Multiple identical key name definitions\n");
		ACTION2("Later occurences of \"<%s> = %d\" ignored\n",buf,kc);
	    }
	    return True;
	}
	if (merge==MergeAugment) {
	    if (reportCollisions) {
		WARN1("Multiple names for keycode %d\n",kc);
		ACTION2("Using <%s>, ignoring <%s>\n",buf,name);
	    }
	    return True;
	}
	else {
	    int old;
	    if (reportCollisions) {
		WARN1("Multiple names for keycode %d\n",kc);
		ACTION2("Using <%s>, ignoring <%s>\n",name,buf);
	    }
	    info->names[kc]= 0;
	    info->files[kc]= 0;
	}
    }
    old= FindKeyByLong(info,lval);
    if ((old!=0)&&(old!=kc)) {
	if (merge==MergeOverride) {
	    info->names[old]= 0;
	    info->files[old]= 0;
	    info->has_alt_forms[old]= True;
	    if (reportCollisions) {
		WARN1("Key name <%s> assigned to multiple keys\n",name);
		ACTION2("Using %d, ignoring %d\n",kc,old);
	    }
	}
	else if (merge!=MergeAltForm) {
	    if ((reportCollisions)&&(warningLevel>3)) {
		WARN1("Key name <%s> assigned to multiple keys\n",name);
		ACTION2("Using %d, ignoring %d\n",old,kc);
		ACTION("Use 'alternate' keyword to assign the same name to multiple keys\n");
	    }
	    return True;
	}
	else {
	    info->has_alt_forms[old]= True;
	}
    }
    info->names[kc]= lval;
    info->files[kc]= fileID;
    info->has_alt_forms[kc]= (merge==MergeAltForm);
    return True;
}

/***====================================================================***/

static void
#if NeedFunctionPrototypes
MergeIncludedKeycodes(KeyNamesInfo *into,KeyNamesInfo *from,unsigned merge)
#else
MergeIncludedKeycodes(into,from,merge)
    KeyNamesInfo *	into;
    KeyNamesInfo *	from;
    unsigned		merge;
#endif
{
register int i;
char buf[5];

    if (from->errorCount>0) {
	into->errorCount+= from->errorCount;
	return;
    }
    if (into->name==NULL) {
	into->name= from->name;
	from->name= NULL;
    }
    for (i=from->computedMin;i<=from->computedMax;i++) {
	unsigned thisMerge;
	if (from->names[i]==0)
	    continue;
	LongToKeyName(from->names[i],buf);
	buf[4]= '\0';
	if (from->has_alt_forms[i])
	     thisMerge= MergeAltForm;
	else thisMerge= merge;
	if (!AddKeyName(into,i,buf,thisMerge,from->fileID,False))
	    into->errorCount++;
    }
    if (from->leds) {
	IndicatorNameInfo *led,*next;
	for (led=from->leds;led!=NULL;led=next) {
	    if (merge!=MergeDefault)
		led->defs.merge= merge;
	    if (!AddIndicatorName(into,led))
		into->errorCount++;
	    next= (IndicatorNameInfo *)led->defs.next;
	}
    }
    if (!MergeAliases(&into->aliases,&from->aliases,merge))
	into->errorCount++;
    if (from->explicitMin>0) {
	if ((into->explicitMin<0)||(into->explicitMin>from->explicitMin))
	    into->effectiveMin= into->explicitMin= from->explicitMin;
    }
    if (from->explicitMax>0) {
	if ((into->explicitMax<0)||(into->explicitMax<from->explicitMax))
	     into->effectiveMax= into->explicitMax= from->explicitMax;
    }
    return;
}

typedef void (*FileHandler)(
#if NeedFunctionPrototypes
    XkbFile *		/* rtrn */,
    XkbDescPtr		/* xkb */,
    unsigned		/* merge */,
    KeyNamesInfo *	/* included */
#endif
);

static Bool
#if NeedFunctionPrototypes
HandleIncludeKeycodes(	IncludeStmt *	stmt,
			XkbDescPtr	xkb,
			KeyNamesInfo *	info,
			FileHandler	hndlr)
#else
HandleIncludeKeycodes(stmt,xkb,info,hndlr)
    IncludeStmt	*	stmt;
    XkbDescPtr		xkb;
    KeyNamesInfo *	info;
    FileHandler		hndlr;
#endif
{
unsigned 	newMerge;
XkbFile	*	rtrn;
KeyNamesInfo 	included;
Bool		haveSelf;

    haveSelf= False;
    if ((stmt->file==NULL)&&(stmt->map==NULL)) {
	haveSelf= True;
	included= *info;
	bzero(info,sizeof(KeyNamesInfo));
    }
    else if (strcmp(stmt->file,"computed")==0) {
	xkb->flags|= AutoKeyNames;
	info->explicitMin= XkbMinLegalKeyCode;
	info->explicitMax= XkbMaxLegalKeyCode;
	return (info->errorCount==0);
    }
    else if (ProcessIncludeFile(stmt,XkmKeyNamesIndex,&rtrn,&newMerge)) {
	InitKeyNamesInfo(&included);
	(*hndlr)(rtrn,xkb,MergeOverride,&included);
	if (stmt->stmt!=NULL) {
	    if (included.name!=NULL)
		uFree(included.name);
	    included.name= stmt->stmt;
	    stmt->stmt= NULL;
	}
    }
    else {
	info->errorCount+= 10;
	return False;
    }
    if ((stmt->next!=NULL)&&(included.errorCount<1)) {
	IncludeStmt *	next;
	unsigned	op;
	KeyNamesInfo	next_incl;

        for (next=stmt->next;next!=NULL;next=next->next) {
	    if ((next->file==NULL)&&(next->map==NULL)) {
		haveSelf= True;
		MergeIncludedKeycodes(&included,info,next->merge);
		ClearKeyNamesInfo(info);
	    }
	    else if (ProcessIncludeFile(next,XkmKeyNamesIndex,&rtrn,&op)) {
		InitKeyNamesInfo(&next_incl);
		(*hndlr)(rtrn,xkb,MergeOverride,&next_incl);
		MergeIncludedKeycodes(&included,&next_incl,op);
		ClearKeyNamesInfo(&next_incl);
	    }
	    else {
		info->errorCount+= 10;
		return False;
	    }
	}
    }
    if (haveSelf)
	*info= included;
    else {
	MergeIncludedKeycodes(info,&included,newMerge);
	ClearKeyNamesInfo(&included);
    }
    return (info->errorCount==0);
}

static int
#if NeedFunctionPrototypes
HandleKeycodeDef(	KeycodeDef *	stmt,
			XkbDescPtr	xkb,
			unsigned 	merge,
			KeyNamesInfo *	info)
#else
HandleKeycodeDef(stmt,xkb,merge,info)
    KeycodeDef *	stmt;
    XkbDescPtr		xkb;
    unsigned 		merge;
    KeyNamesInfo *	info;
#endif
{
int		code;
ExprResult	result;

    if (!ExprResolveInteger(stmt->value,&result,NULL,NULL)) {
	ACTION1("No value keycode assigned to name <%s>\n",stmt->name);
	return 0;
    }
    code= result.ival;
    if ((code<info->effectiveMin)||(code>info->effectiveMax)) {
	ERROR2("Illegal keycode %d for name <%s>\n",code,stmt->name);
	ACTION2("Must be in the range %d-%d inclusive\n",info->effectiveMin,
							 info->effectiveMax);
	return 0;
    }
    if (stmt->merge!=MergeDefault) {
	if (stmt->merge==MergeReplace)
	     merge= MergeOverride;
	else merge= stmt->merge;
    }
    return AddKeyName(info,code,stmt->name,merge,info->fileID,True);
}

#define	MIN_KEYCODE_DEF		0
#define	MAX_KEYCODE_DEF		1

static int
#if NeedFunctionPrototypes
HandleKeyNameVar(VarDef *stmt,XkbDescPtr xkb,unsigned merge,KeyNamesInfo *info)
#else
HandleKeyNameVar(stmt,xkb,merge,info)
    VarDef *		stmt;
    XkbDescPtr		xkb;
    unsigned 		merge;
    KeyNamesInfo *	info;
#endif
{
ExprResult	tmp,field;
ExprDef	*	arrayNdx;
int		which;

    if (ExprResolveLhs(stmt->name,&tmp,&field,&arrayNdx)==0) 
	return 0; /* internal error, already reported */

    if (tmp.str!=NULL) {
	ERROR1("Unknown element %s encountered\n",tmp.str);
	ACTION1("Default for field %s ignored\n",field.str);
	return 0;
    }
    if (uStrCaseCmp(field.str,"minimum")==0)	 which= MIN_KEYCODE_DEF;
    else if (uStrCaseCmp(field.str,"maximum")==0) which= MAX_KEYCODE_DEF;
    else {
	ERROR("Unknown field encountered\n");
	ACTION1("Assigment to field %s ignored\n",field.str);
	return 0;
    }
    if (arrayNdx!=NULL) {
	ERROR1("The %s setting is not an array\n",field.str);
	ACTION("Illegal array reference ignored\n");
	return 0;
    }

    if (ExprResolveInteger(stmt->value,&tmp,NULL,NULL)==0) {
	ACTION1("Assignment to field %s ignored\n",field.str);
	return 0;
    }
    if ((tmp.ival<XkbMinLegalKeyCode)||(tmp.ival>XkbMaxLegalKeyCode)) {
	ERROR3("Illegal keycode %d (must be in the range %d-%d inclusive)\n",
			tmp.ival,XkbMinLegalKeyCode,XkbMaxLegalKeyCode);
	ACTION1("Value of \"%s\" not changed\n",field.str);
	return 0;
    }
    if (which==MIN_KEYCODE_DEF) {
	if ((info->explicitMax>0)&&(info->explicitMax<tmp.ival)) {
	    ERROR2("Minimum key code (%d) must be <= maximum key code (%d)\n",
						tmp.ival,info->explicitMax);
	    ACTION("Minimum key code value not changed\n");
	    return 0;
	}
	if ((info->computedMax>0)&&(info->computedMin<tmp.ival)) {
	    ERROR2("Minimum key code (%d) must be <= lowest defined key (%d)\n",
						tmp.ival,info->computedMin);
	    ACTION("Minimum key code value not changed\n");
	    return 0;
	}
	info->explicitMin= tmp.ival;
	info->effectiveMin= tmp.ival;
    }
    if (which==MAX_KEYCODE_DEF) {
	if ((info->explicitMin>0)&&(info->explicitMin>tmp.ival)) {
	    ERROR2("Maximum code (%d) must be >= minimum key code (%d)\n",
						tmp.ival,info->explicitMin);
	    ACTION("Maximum code value not changed\n");
	    return 0;
	}
	if ((info->computedMax>0)&&(info->computedMax>tmp.ival)) {
	    ERROR2("Maximum code (%d) must be >= highest defined key (%d)\n",
						tmp.ival,info->computedMax);
	    ACTION("Maximum code value not changed\n");
	    return 0;
	}
	info->explicitMax= tmp.ival;
	info->effectiveMax= tmp.ival;
    }
    return 1;
}

static int
#if NeedFunctionPrototypes
HandleIndicatorNameDef(	IndicatorNameDef *	def,
			XkbDescPtr		xkb,
			unsigned 		merge,
			KeyNamesInfo *		info)
#else
HandleIndicatorNameDef(def,xkb,merge,info)
    IndicatorNameDef *	def;
    XkbDescPtr		xkb;
    unsigned 		merge;
    KeyNamesInfo *	info;
#endif
{
IndicatorNameInfo 	ii;
ExprResult		tmp;

    if ((def->ndx<1)||(def->ndx>XkbNumIndicators)) {
	info->errorCount++;
	ERROR1("Name specified for illegal indicator index %d\n",def->ndx);
	ACTION("Ignored\n");
	return False;
    }
    InitIndicatorNameInfo(&ii,info);
    ii.ndx= def->ndx;
    if (!ExprResolveString(def->name,&tmp,NULL,NULL)) {
	char buf[20];
	sprintf(buf,"%d",def->ndx);
	info->errorCount++;
	return ReportBadType("indicator","name",buf,"string");
    }
    ii.name= XkbInternAtom(NULL,tmp.str,False);
    ii.virtual= def->virtual;
    if (!AddIndicatorName(info,&ii))
	return False;
    return True;
}

static void
#if NeedFunctionPrototypes
HandleKeycodesFile(	XkbFile *	file,
			XkbDescPtr 	xkb,
			unsigned	merge,
			KeyNamesInfo *	info)
#else
HandleKeycodesFile(file,xkb,merge,info)
    XkbFile		*file;
    XkbDescPtr	 	 xkb;
    unsigned		 merge;
    KeyNamesInfo	*info;
#endif
{
ParseCommon	*stmt;

    info->name= uStringDup(file->name);
    stmt= file->defs;
    while (stmt) {
	switch (stmt->stmtType) {
	    case StmtInclude:
		if (!HandleIncludeKeycodes((IncludeStmt *)stmt,xkb,info,
						HandleKeycodesFile))
		    info->errorCount++;
		break;
	    case StmtKeycodeDef:
		if (!HandleKeycodeDef((KeycodeDef *)stmt,xkb,merge,info))
		    info->errorCount++;
		break;
	    case StmtKeyAliasDef:
		if (!HandleAliasDef((KeyAliasDef *)stmt,
					merge,info->fileID,&info->aliases))
		    info->errorCount++;
		break;
	    case StmtVarDef:
		if (!HandleKeyNameVar((VarDef *)stmt,xkb,merge,info))
		    info->errorCount++;
		break;
	    case StmtIndicatorNameDef:
		if (!HandleIndicatorNameDef((IndicatorNameDef *)stmt,xkb,
								merge,info)) {
		    info->errorCount++;
		}
		break;
	    case StmtInterpDef:
	    case StmtVModDef:
		ERROR("Keycode files may define key and indicator names only\n");
		ACTION1("Ignoring definition of %s\n",
				((stmt->stmtType==StmtInterpDef)?
					"a symbol interpretation":
					"virtual modifiers"));
		info->errorCount++;
		break;
	    default:
		WSGO1("Unexpected statement type %d in HandleKeycodesFile\n",
			stmt->stmtType);
		break;
	}
	stmt= stmt->next;
	if (info->errorCount>10) {
#ifdef NOISY
	    ERROR("Too many errors\n");
#endif
	    ACTION1("Abandoning keycodes file \"%s\"\n",file->topName);
	    break;
	}
    }
    return;
}

Bool
#if NeedFunctionPrototypes
CompileKeycodes(XkbFile	*file,XkbFileInfo *result,unsigned merge)
#else
CompileKeycodes(file,result,merge)
    XkbFile	*	file;
    XkbFileInfo *	result;
    unsigned	 	merge;
#endif
{
KeyNamesInfo	info;
XkbDescPtr	xkb;

    xkb= result->xkb;
    InitKeyNamesInfo(&info);
    HandleKeycodesFile(file,xkb,merge,&info);

    if (info.errorCount==0) {
	if (info.explicitMin>0)
	     xkb->min_key_code= info.effectiveMin;
	else xkb->min_key_code= info.computedMin;
	if (info.explicitMax>0)
	     xkb->max_key_code= info.effectiveMax;
	else xkb->max_key_code= info.computedMax;
	if (XkbAllocNames(xkb,XkbKeyNamesMask|XkbIndicatorNamesMask,0,0)==Success) {
	    register int i;
	    xkb->names->keycodes= XkbInternAtom(xkb->dpy,info.name,False);
	    uDEBUG2(1,"key range: %d..%d\n",xkb->min_key_code,xkb->max_key_code);
	    for (i=info.computedMin;i<=info.computedMax;i++) {
		LongToKeyName(info.names[i],xkb->names->keys[i].name);
		uDEBUG2(2,"key %d = %s\n",i,
			XkbKeyNameText(xkb->names->keys[i].name,XkbMessage));
	    }
	}
	else {
	    WSGO("Cannot create XkbNamesRec in CompileKeycodes\n");
	    return False;
	}
	if (info.leds) {
	    IndicatorNameInfo	*ii;
	    if (XkbAllocIndicatorMaps(xkb)!=Success) {
		WSGO("Couldn't allocate IndicatorRec in CompileKeycodes\n");
		ACTION("Physical indicators not set\n");
	    }
	    for (ii=info.leds;ii!=NULL;ii=(IndicatorNameInfo *)ii->defs.next){
		xkb->names->indicators[ii->ndx-1]= 
			XkbInternAtom(xkb->dpy,
					XkbAtomGetString(NULL,ii->name),False);
		if (xkb->indicators!=NULL) {
		    register unsigned bit;
		    bit= 1<<(ii->ndx-1);
		    if (ii->virtual)	
			 xkb->indicators->phys_indicators&= ~bit;
		    else xkb->indicators->phys_indicators|= bit;
		}
	    }
	}
	if (info.aliases)
	    ApplyAliases(xkb,False,&info.aliases);
	return True;
    }
    ClearKeyNamesInfo(&info);
    return False;
}
