/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/

#ifndef HEST_HAS_BEEN_INCLUDED
#define HEST_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <air.h>

/*
******** hestCB struct
**
** for when the thing you want to parse from the command-line is not a
** simple boolean, number, or string.  hestParse() will not allocate
** anything to store individual things, though it may allocate an
** array in the case of a multiple variable parameter option.  If your
** things are actually pointers to things, then you do the allocation
** in the parse() callback.  In this case, you set delete() to be your
** "destructor", and it will be called on the result of derefencing the
** argument to parse().
*/
typedef struct {
  size_t size;          /* sizeof() one thing */
  char *type;           /* used by hestGlossary() to describe the type */
  void *(*parse)(void *ptr, char *str);
                        /* how to parse one thing from a string.  This will
			   be called multiple times for multiple parameter
			   options.  Using a void* as the return value does
			   not preclude storing an integral value in the void*
			   (see K+R pg. 199) but its up to you to insure that
			   the integral value fits in the void*. */
  char *(*error)(void *ret);
                        /* what to call if parse() has a non-zero return,
			   so as to generate an error message.  strlen() of
			   this string must be < AIR_STRLEN_MED, and it should
			   not contain any carriage returns or newlines. */
  void *(*delete)(void *ptr);
                        /* if non-NULL, this is the destructor that will be
			   called by hestParseFree() (or by hestParse() if
			   there is an error midway through parsing).  The
			   argument is NOT the same as passed to parse():
			   it is the result of dereferencing the argument
			   to parse() */
  int freeErrorStr;     /* whether to free() the return of error() */
} hestCB;

/*
******** hestOpt struct
**
** information which specifies one command-line option
*/
typedef struct {
  char *flag,           /* how the option is identified on the cmd line */
    *name;              /* simple description of option's parameter(s) */
  int type,             /* type of option (from airType enum) */
    min, max;           /* min and max # of parameters for option */
  void *valueP;         /* storage of parsed values */
  char *dflt,           /* default value written out as string */
    *info;              /* description to be printed with "glossary" info */
  int *sawP;            /* used ONLY for multiple variable parameter options
			   (min < max > 2): storage of # of parsed values */
  hestCB *CB;           /* used ONLY for airTypeOther options */

  /* --------------------- end of user-defined fields */

  int kind,             /* what kind of option is this, based on min and max,
			   set by hestParse() (actually _hestPanic()),
			   later used by hestFree():
			   1: min == max == 0
			      stand-alone flag; no parameters
			   2: min == max == 1
                              single fixed parameter
			   3: min == max >= 2
                              multiple fixed parameters
			   4: min == 0; max == 1;
                              single variable parameter
			   5: max - min >= 1; max >= 2 
                              multiple variable parameters */
    alloc;              /* information about whether flag is non-NULL, and what
			   parameters were used, that determines whether or
			   not memory was allocated by hestParse(); info
			   later used by hestParseFree():
			   0: no free()ing needed
			   1: free(*valueP), either because it is a single
			      string, or because was a dynamically allocated
			      array of non-strings
			   2: free((*valueP)[i]), because they are elements
			      of a fixed-length array of strings
			   3: free((*valueP)[i]) and free(*valueP), because
 			      it is a dynamically allocated array of strings */
} hestOpt;

/*
******** hestParm struct
**
** parameters to control behavior of hest functions. 
**
** GK: Don't even think about storing per-parse state in here.
*/
typedef struct {
  int verbosity,        /* verbose diagnostic messages to stdout */
    respFileEnable,     /* whether or not to use response files */
    columns;            /* number of printable columns in output */
  char respFileFlag,    /* the character at the beginning of an argument
			   indicating that this is a response file name */
    respFileComment,    /* comment character for the repose files */
    varParamStopFlag,   /* prefixed by '-' to form the flag which signals
			   the end of a flagged variable parameter option
			   (single or multiple) */
    multiFlagSep;       /* character in flag which signifies that there is
			   a long and short version, and which seperates
			   the two.  Or, can be set to '\0' to disable this
			   behavior entirely. */
} hestParm;

/* defaults.c */
extern int hestVerbosity;
extern int hestRespFileEnable;
extern int hestColumns;
extern char hestRespFileFlag;
extern char hestRespFileComment;
extern char hestVarParamStopFlag;
extern char hestMultiFlagSep;

/* methods.c */
extern hestParm *hestParmNew(void);
extern hestParm *hestParmFree(hestParm *parm);
extern void hestOptAdd(hestOpt **optP, 
		       char *flag, char *name,
		       int type, int min, int max,
		       void *valueP, char *dflt, char *info, ...);
extern hestOpt *hestOptFree(hestOpt *opt);
extern int hestOptCheck(hestOpt *opt, char **errP);

/* parse.c */
extern int hestParse(hestOpt *opt, int argc, char **argv,
		     char **errP, hestParm *parm);
extern void hestParseFree(hestOpt *opt);

/* usage.c */
extern void hestUsage(FILE *file, hestOpt *opt, char *argv0, hestParm *parm);
extern void hestGlossary(FILE *file, hestOpt *opt, hestParm *parm);
extern void hestInfo(FILE *file, char *argv0, char *info, hestParm *parm);

#ifdef __cplusplus
}
#endif
#endif /* HEST_HAS_BEEN_INCLUDED */
