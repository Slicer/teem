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
#include <air.h>

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
  int *sawP,            /* used ONLY for multiple variable parameter options
			   (min < max > 2): storage of # of parsed values */

  /* --------------------- end of user-defined fields */

    kind,               /* what kind of option is this, based on min and max,
			   set by hestParse() (actually _hestPanic()),
			   later used by hestFree():
			   1: min == max == 0
			      a binary flag, no parameters
			   2: min == max == 1
                              one required parameter
			   3: min == max >= 2
                              multiple required parameters
			   4: min == 0; max == 1;
                              one optional parameter
			   5: max - min >= 1; max >= 2 
                              multiple optional parameter */
    alloc;              /* information about whether flag is non-NULL, and what
			   parameters were used, which determines whether or
			   not memory was allocated by hestParse(); info
			   later used by hestFree():
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
** parameters to control behavior of hest functions
*/
typedef struct {
  int verbosity,        /* verbose diagnostic messages to stdout */
    respFileEnable,     /* whether or not to use response files */
    columns;            /* number of printable columns in output */
  char respFileFlag,    /* the character at the beginning of an argument
			   indicating that this is a response file name */
    respFileComment;    /* comment character for the repose files */
} hestParm;

/* defaults.c */
extern int hestVerbosity;
extern int hestRespFileEnable;
extern int hestColumns;
extern char hestRespFileFlag;
extern char hestRespFileComment;

/* methods.c */
extern hestParm *hestParmNew(void);
extern hestParm *hestParmNix(hestParm *parm);
extern void hestFree(hestOpt *opt, hestParm *parm);

/* usage.c */
extern void hestInfo(FILE *file, char *argv0, char *info, hestParm *parm);
extern void hestUsage(FILE *file, hestOpt *opt, char *argv0, hestParm *parm);
extern void hestGlossary(FILE *file, hestOpt *opt, hestParm *parm);

/* parse.c */
extern int hestParse(hestOpt *opt, char **argv, char *err, hestParm *parm);

#ifdef __cplusplus
}
#endif
#endif /* HEST_HAS_BEEN_INCLUDED */
