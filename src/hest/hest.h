/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef HEST_HAS_BEEN_INCLUDED
#define HEST_HAS_BEEN_INCLUDED

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#include <teem/air.h>

#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(TEEM_STATIC)
#  if defined(TEEM_BUILD) || defined(hest_EXPORTS) || defined(teem_EXPORTS)
#    define HEST_EXPORT extern __declspec(dllexport)
#  else
#    define HEST_EXPORT extern __declspec(dllimport)
#  endif
#else /* TEEM_STATIC || UNIX */
#  define HEST_EXPORT extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
******** hestSource* enum
**
** records whether the info to satisfy a particular option came from the default or from
** the user (command-line or response file). Distinguishing command-line from response
** file would take a much more significant code restructuring
*/
enum {
  hestSourceUnknown, /* 0 */
  hestSourceDefault, /* 1 */
  hestSourceUser,    /* 2 */
  hestSourceLast
};

/*
******** hestCB struct
**
** for when the thing you want to parse from the command-line is not a simple boolean,
** number, airEnum, or string.  hestParse() will not allocate anything to store
** individual things, though it may allocate an array in the case of a multiple
** variable parameter option.  If your things are actually pointers to things, then you
** do the allocation in the parse() callback.  In this case, you set destroy() to be
** your "destructor", and it will be called on the result of derefencing the argument
** to parse().
*/
typedef struct {
  size_t size;      /* sizeof() one thing */
  const char *type; /* used by hestGlossary() to describe the type */
  int (*parse)(void *ptr, const char *str, char err[AIR_STRLEN_HUGE]);
  /* how to parse one thing from a string.  This will be called multiple times for
     multiple parameter options.  A non-zero return value is considered an error.  Error
     message goes in the err string */
  void *(*destroy)(void *ptr);
  /* if non-NULL, this is the destructor that will be called by hestParseFree() (or by
     hestParse() if there is an error midway through parsing).  The argument is NOT the
     same as passed to parse(): it is the result of dereferencing the argument to parse()
   */
} hestCB;

/*
******** hestOpt struct
**
** information which specifies one command-line option
*/
typedef struct {
  char *flag,         /* how the option is identified on the cmd line */
    *name;            /* simple description of option's parameter(s) */
  int type;           /* type of option (from airType enum) */
  unsigned int min;   /* min # of parameters for option */
  int max;            /* max # of parameters for option,
                         or -1 for "there is no max" */
  void *valueP;       /* storage of parsed values */
  char *dflt,         /* default value written out as string */
    *info;            /* description to be printed with "glossary" info */
  unsigned int *sawP; /* used ONLY for multiple variable parameter options
                         (min < max >= 2): storage of # of parsed values */
  airEnum *enm;       /* used ONLY for airTypeEnum options */
  hestCB *CB;         /* used ONLY for airTypeOther options */

  /* --------------------- end of user-defined fields; the following fields must be set
     by hestParse() as part of its operation. This does prevent adding const to much
     usage of the hestOpt */

  int kind, /* what kind of option is this, based on min and max,set by hestParse()
               (actually _hestPanic()), later used by hestFree():
               1: min == max == 0       stand-alone flag; no parameters
               2: min == max == 1       single fixed parameter
               3: min == max >= 2       multiple fixed parameters
               4: min == 0; max == 1;   single variable parameter
               5: min < max; max >= 2   multiple variable parameters */
    alloc;  /* information about whether flag is non-NULL, and what parameters were used,
               that determines whether or not memory was allocated by hestParse(); info
               later used by hestParseFree():
               0: no free()ing needed
               1: free(*valueP), either because it is a single string, or because was a
                  dynamically allocated array of non-strings
               2: free((*valueP)[i]), because they are elements of a fixed-length
                  array of strings
               3: free((*valueP)[i]) and free(*valueP), because it is a dynamically
                  allocated array of strings */

  /* --------------------- Output */

  int source;     /* from the hestSource* enum; from whence was this information learned,
                     else hestSourceUnknown if not */
  char *parmStr;  /* if non-NULL: a string (allocated by hestParse, and freed by
                     hestParseFree) from which hestParse ultimately parsed whatever values
                     were set in *valueP. All the parameters associated with this option
                     are joined (with " " separation) into this single string. hestParse
                     has always formed this string internally as part of its operation,
                     but only belatedly (in 2023) is a copy of that string being made
                     available here to the caller. Note that in the case of single
                     variable parameter  options used without a parameter, the value
                     stored  will be "inverted" from the string here. */
  int helpWanted; /* hestParse() saw something (like "--help") in one of the given
                     arguments that looks like a call for help (and respectDashDashHelp
                     is set in the hestParm), so it recorded that here. There is
                     unfortunately no other top-level output container for info generated
                     by hestParse(), so this field is going to be set only in the *first*
                     hestOpt passed to hestParse(), even though that hestOpt has no
                     particular relation to where hestParse() saw the call for help. */
} hestOpt;

/*
******** hestParm struct
**
** parameters to control behavior of hest functions.
**
** GK: Don't even think about storing per-parse state in here.
*/
typedef struct {
  int verbosity,          /* verbose diagnostic messages to stdout */
    respFileEnable,       /* whether or not to use response files */
    elideSingleEnumType,  /* if type is airTypeEnum, and if it's a single fixed parameter
                             option, then don't bother printing the  type information as
                             part of hestGlossary() */
    elideSingleOtherType, /* like above, but for airTypeOther */
    elideSingleOtherDefault, /* don't display default for single fixed airTypeOther
                                parameter */
    elideSingleNonExistFloatDefault, /* if default for a single fixed floating point
                                        (float or double) parameter doesn't AIR_EXIST,
                                        then don't display the default */
    elideMultipleNonExistFloatDefault,
    elideSingleEmptyStringDefault, /* if default for a single string is empty
                                      (""), then don't display default */
    elideMultipleEmptyStringDefault,
    respectDashDashHelp,   /* hestParse interprets seeing "--help" as not an
                              error, but as a request to print usage info,
                              so sets helpWanted in the (first) hestOpt */
    noArgsIsNoProblem,     /* if non-zero, having no arguments to parse is not in and
                              of itself a problem; this means that if all options have
                              defaults, it would be *ok* to invoke the problem without
                              any further command-line options. This is counter to
                              pre-Teem-1.11 behavior (for which no arguments *always*
                              meant "show me usage info"). */
    greedySingleString,    /* when parsing a single string, whether or not to be greedy
                              (as per airParseStrS) */
    cleverPluralizeOtherY, /* when printing the type for airTypeOther, when the min
                              number of items is > 1, and the type string ends with "y",
                              then pluralize with "ies" instead of "ys" */
    dieLessVerbose, /* on parse failure, hestParseOrDie prints less than it otherwise
                       might: only print info and glossary when they "ask for it" */
    noBlankLineBeforeUsage; /* like it says */
  unsigned int columns;     /* number of printable columns in output */
  char respFileFlag,        /* the character at the beginning of an argument
                               indicating that this is a response file name */
    respFileComment,        /* comment character for the response files */
    varParamStopFlag, /* prefixed by '-' to form the flag (usually "--") that signals the
                         end of a *flagged* variable parameter option (single or
                         multiple). This is important to use if there is a flagged
                         variable parameter option preceeding an unflagged variable
                         parameter option, because otherwise how will you know where the
                         first stops and the second begins */
    multiFlagSep;     /* character in flag which signifies that there is a long and short
                         version, and which separates the two.  Or, can be set to '\0' to
                         disable this behavior entirely. */
} hestParm;

/* defaultsHest.c */
HEST_EXPORT int hestDefaultVerbosity;
HEST_EXPORT int hestDefaultRespFileEnable;
HEST_EXPORT int hestDefaultElideSingleEnumType;
HEST_EXPORT int hestDefaultElideSingleOtherType;
HEST_EXPORT int hestDefaultElideSingleOtherDefault;
HEST_EXPORT int hestDefaultElideSingleNonExistFloatDefault;
HEST_EXPORT int hestDefaultElideMultipleNonExistFloatDefault;
HEST_EXPORT int hestDefaultElideSingleEmptyStringDefault;
HEST_EXPORT int hestDefaultElideMultipleEmptyStringDefault;
HEST_EXPORT int hestDefaultNoArgsIsNoProblem;
HEST_EXPORT int hestDefaultGreedySingleString;
HEST_EXPORT int hestDefaultCleverPluralizeOtherY;
HEST_EXPORT unsigned int hestDefaultColumns;
HEST_EXPORT char hestDefaultRespFileFlag;
HEST_EXPORT char hestDefaultRespFileComment;
HEST_EXPORT char hestDefaultVarParamStopFlag;
HEST_EXPORT char hestDefaultMultiFlagSep;

/* methodsHest.c */
HEST_EXPORT const int hestPresent;
HEST_EXPORT hestParm *hestParmNew(void);
HEST_EXPORT hestParm *hestParmFree(hestParm *parm);
HEST_EXPORT void *hestParmFree_vp(void *parm);
HEST_EXPORT unsigned int hestOptAdd(hestOpt **optP,
                                    const char *flag, const char *name,
                                    int type, int min, int max,
                                    void *valueP, const char *dflt,
                                    const char *info,
                                    ... /* unsigned int *sawP,
                                           airEnum *enm,
                                           const hestCB *CB */);
HEST_EXPORT unsigned int hestOptNum(const hestOpt *opt);
HEST_EXPORT hestOpt *hestOptFree(hestOpt *opt);
HEST_EXPORT void *hestOptFree_vp(void *opt);
HEST_EXPORT int hestOptCheck(hestOpt *opt, char **errP);

/* parseHest.c */
HEST_EXPORT int hestParse(hestOpt *opt, int argc, const char **argv, char **errP,
                          const hestParm *parm);
HEST_EXPORT void *hestParseFree(hestOpt *opt);
HEST_EXPORT void hestParseOrDie(hestOpt *opt, int argc, const char **argv,
                                hestParm *parm, const char *me, const char *info,
                                int doInfo, int doUsage, int doGlossary);

/* usage.c */
HEST_EXPORT void _hestPrintStr(FILE *f, unsigned int indent, unsigned int already,
                               unsigned int width, const char *_str, int bslash);
HEST_EXPORT int hestMinNumArgs(hestOpt *opt);
HEST_EXPORT void hestUsage(FILE *file, hestOpt *opt, const char *argv0,
                           const hestParm *parm);
HEST_EXPORT void hestGlossary(FILE *file, hestOpt *opt, const hestParm *parm);
HEST_EXPORT void hestInfo(FILE *file, const char *argv0, const char *info,
                          const hestParm *parm);

#ifdef __cplusplus
}
#endif

#endif /* HEST_HAS_BEEN_INCLUDED */
