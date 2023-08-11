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
** for when the thing you want to parse from the command-line is airTypeOther: not a
** simple boolean, number, string, or airEnum.  hestParse() will not allocate anything to
** store individual things, though it may allocate an array in the case of a multiple
** variable parameter option.  If your things are actually pointers to things, then you
** do the allocation in the parse() callback.  In this case, you set destroy() to be
** your "destructor", and it will be called on the result of derefencing the argument
** to parse().
*/
typedef struct {
  size_t size;      /* sizeof() one thing */
  const char *type; /* used by hestGlossary() to describe the type */
  int (*parse)(void *ptr, const char *str, char err[AIR_STRLEN_HUGE + 1]);
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
** information which specifies one command-line option,
** and describes it how it was parsed
*/
typedef struct {
  /* --------------------- "input" fields
  set by user, possibly directly, more likely indirectly via one of the various
  functions (like hestOptAdd or hestOptAdd_nva or hestOptSingleSet ... ) */
  char *flag,         /* how the option is identified on the cmd line */
    *name;            /* simple description of option's parameter(s) */
  int type;           /* type of option (from airType enum) */
  unsigned int min;   /* min # of parameters for option */
  int max;            /* max # of parameters for option,
                         or -1 for "there is no max; # parms is unbounded" */
  void *valueP;       /* storage of parsed values */
  char *dflt,         /* default value written out as string */
    *info;            /* description to be printed with "glossary" info */
  unsigned int *sawP; /* used ONLY for multiple variable parameter options
                         (min < max >= 2): storage of # of parsed values */
  const airEnum *enm; /* used ONLY for airTypeEnum options */
  const hestCB *CB;   /* used ONLY for airTypeOther options */

  /* --------------------- end of user-defined fields
  These are set by hest functions to remember state for the sake of other hest functions.
  It is probably a drawback of the simple design of hest that this internal state ends
  up in the same struct as the input parameters above, because it blocks some
  const-correctness opportunities that would otherwise make sense. */
  int kind, /* What kind of option is this, based on min and max:
               0:                       (invalid; unset)
               1: min == max == 0       stand-alone flag; no parameters
               2: min == max == 1       single fixed parameter
               3: min == max >= 2       multiple fixed parameters
               4: min == 0; max == 1;   single variable parameter
               5: min < max; max >= 2   multiple variable parameters
              This is set by hest functions as part of building up an array of hestOpt,
              and informs the later action of hestOptFree */
    alloc;  /* Information (set by hestParse) about whether flag is non-NULL, and what
               parameters were used, that determines whether or not memory was allocated
               by hestParse(). Informs later action of hestParseFree():
               0: no free()ing needed
               1: free(*valueP), either because it is a single string, or because was a
                  dynamically allocated array of non-strings
               2: free((*valueP)[i]), because they are elements of a fixed-length
                  array of strings
               3: free((*valueP)[i]) and free(*valueP), because it is a dynamically
                  allocated array of strings */
  /* Since hest's beginning, the basic container for a set of options was an array of
  hestOpt structs (not pointers to them, which rules out argv-style NULL-termination of
  the array), also unfortunately with no other top-level container (which is why
  helpWanted below is set only in the first hestOpt of the array). hestOptAdd has
  historically reallocated the entire array, incrementing the length only by one with
  each call, while maintaining a single terminating hestOpt, wherein some fields were set
  to special values to indicate termination. With the 2023 code revisit, that was deemed
  even uglier than this hack: the first hestOpt now stores here in arrAlloc the allocated
  length of the hestOpt array, and in arrLen the number of hestOpts actually used and
  set. This facilitates implementing something much like an airArray, but without the
  burden of extra calls for the user (like airArrayLenIncr), nor new kinds of containers
  for hest and its users to manage: it is just the same array of hestOpt structs */
  unsigned int arrAlloc, arrLen;

  /* --------------------- Output
  Things set/allocated by hestParse. */

  int source;     /* from the hestSource* enum; from whence was this information learned,
                  else hestSourceUnknown if not */
  char *parmStr;  /* if non-NULL: a string (freed by hestParseFree) from which hestParse
                  ultimately parsed whatever values were set in *valueP. All the
                  parameters associated with this option are joined (with " " separation)
                  into this single string. hestParse has always formed this string
                  internally as part of its operation, but only belatedly (in 2023) is a
                  copy of that string being made available here to the caller. Note that
                  in the case of single variable parameter options used without a
                  parameter, the value stored will be "inverted" from the string here. */
  int helpWanted; /* hestParse() saw something (like "--help") in one of the given
                  arguments that looks like a call for help (and respectDashDashHelp is
                  set in the hestParm), so it recorded that here. There is unfortunately
                  no other top-level output container for info generated by hestParse(),
                  so this field is going to be set only in the *first* hestOpt passed to
                  hestParse(), even though that hestOpt has no particular relation to
                  where hestParse() saw the call for help. */
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
HEST_EXPORT int hestParmColumnsIoctl(hestParm *hparm, unsigned int nonIoctlColumns);
HEST_EXPORT void hestOptSingleSet(hestOpt *opt, const char *flag, const char *name,
                                  int type, unsigned int min, int max, void *valueP,
                                  const char *dflt, const char *info, unsigned int *sawP,
                                  const airEnum *enm, const hestCB *CB);
HEST_EXPORT unsigned int hestOptAdd_nva(hestOpt **optP, const char *flag,
                                        const char *name, int type, unsigned int min,
                                        int max, void *valueP, const char *dflt,
                                        const char *info, unsigned int *sawP,
                                        const airEnum *enm, const hestCB *CB);
HEST_EXPORT unsigned int hestOptAdd(hestOpt **optP,
                                    const char *flag, const char *name,
                                    int type, unsigned int min, int max,
                                    void *valueP, const char *dflt,
                                    const char *info,
                                    ... /* unsigned int *sawP,
                                           const airEnum *enm,
                                           const hestCB *CB */);
/* see also all the special-purpose and type-checked versions in adders.c, below */
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

/* adders.c */
HEST_EXPORT void hestOptAddDeclsPrint(FILE *f);
/* Many many non-var-args alternatives to hestOptAdd, also usefully type-specific for the
type of value to be parsed in a way that hestOptAdd_nva cannot match. These capture all
the common uses (and them some) of hest within Teem. They can be categorized, like
hestOpt->kind, in terms of the min, max number of (type T) parameters to the option:

  min == max == 0       hestOptAdd_Flag         (stand-alone flag; no parameters)
  min == max == 1       hestOptAdd_1_T          single fixed parameter
  min == max >= 2       hestOptAdd_{2,3,4,N}_T  multiple fixed parameters
  min == 0; max == 1    hestOptAdd_1v_T         single variable parameter
  min < max; max >= 2   hestOptAdd_Nv_T         multiple variable parameters

An airEnum* is passed for _Enum options; or a hestCB* for _Other options. The number of
parameters *sawP that hestParm saw on the command-line is passed for the _Nv_ options.

All declarations below were automatically generated via hestOptAddDeclsPrint (followed by
clang-format), which (like the implementation of all the functions) is done via lots of
#define macro tricks. */
HEST_EXPORT unsigned int hestOptAdd_Flag(hestOpt **optP, const char *flag, int *valueP,
                                         const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_Bool(hestOpt **hoptP, const char *flag,
                                            const char *name, int *valueP,
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_Int(hestOpt **hoptP, const char *flag,
                                           const char *name, int *valueP,
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_UInt(hestOpt **hoptP, const char *flag,
                                            const char *name, unsigned int *valueP,
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_LongInt(hestOpt **hoptP, const char *flag,
                                               const char *name, long int *valueP,
                                               const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_ULongInt(hestOpt **hoptP, const char *flag,
                                                const char *name,
                                                unsigned long int *valueP,
                                                const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_Size_t(hestOpt **hoptP, const char *flag,
                                              const char *name, size_t *valueP,
                                              const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_Float(hestOpt **hoptP, const char *flag,
                                             const char *name, float *valueP,
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_Double(hestOpt **hoptP, const char *flag,
                                              const char *name, double *valueP,
                                              const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_Char(hestOpt **hoptP, const char *flag,
                                            const char *name, char *valueP,
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_String(hestOpt **hoptP, const char *flag,
                                              const char *name, char **valueP,
                                              const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_Enum(hestOpt **hoptP, const char *flag,
                                            const char *name, int *valueP,
                                            const char *dflt, const char *info,
                                            const airEnum *enm);
HEST_EXPORT unsigned int hestOptAdd_1v_Other(hestOpt **hoptP, const char *flag,
                                             const char *name, void *valueP,
                                             const char *dflt, const char *info,
                                             const hestCB *CB);
HEST_EXPORT unsigned int hestOptAdd_1_Bool(hestOpt **hoptP, const char *flag,
                                           const char *name, int *valueP,
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_Int(hestOpt **hoptP, const char *flag,
                                          const char *name, int *valueP,
                                          const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_UInt(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int *valueP,
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_LongInt(hestOpt **hoptP, const char *flag,
                                              const char *name, long int *valueP,
                                              const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_ULongInt(hestOpt **hoptP, const char *flag,
                                               const char *name,
                                               unsigned long int *valueP,
                                               const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_Size_t(hestOpt **hoptP, const char *flag,
                                             const char *name, size_t *valueP,
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_Float(hestOpt **hoptP, const char *flag,
                                            const char *name, float *valueP,
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_Double(hestOpt **hoptP, const char *flag,
                                             const char *name, double *valueP,
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_Char(hestOpt **hoptP, const char *flag,
                                           const char *name, char *valueP,
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_String(hestOpt **hoptP, const char *flag,
                                             const char *name, char **valueP,
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_Enum(hestOpt **hoptP, const char *flag,
                                           const char *name, int *valueP,
                                           const char *dflt, const char *info,
                                           const airEnum *enm);
HEST_EXPORT unsigned int hestOptAdd_1_Other(hestOpt **hoptP, const char *flag,
                                            const char *name, void *valueP,
                                            const char *dflt, const char *info,
                                            const hestCB *CB);
HEST_EXPORT unsigned int hestOptAdd_2_Bool(hestOpt **hoptP, const char *flag,
                                           const char *name, int valueP[2],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_Int(hestOpt **hoptP, const char *flag,
                                          const char *name, int valueP[2],
                                          const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_UInt(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int valueP[2],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_LongInt(hestOpt **hoptP, const char *flag,
                                              const char *name, long int valueP[2],
                                              const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_ULongInt(hestOpt **hoptP, const char *flag,
                                               const char *name,
                                               unsigned long int valueP[2],
                                               const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_Size_t(hestOpt **hoptP, const char *flag,
                                             const char *name, size_t valueP[2],
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_Float(hestOpt **hoptP, const char *flag,
                                            const char *name, float valueP[2],
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_Double(hestOpt **hoptP, const char *flag,
                                             const char *name, double valueP[2],
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_Char(hestOpt **hoptP, const char *flag,
                                           const char *name, char valueP[2],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_String(hestOpt **hoptP, const char *flag,
                                             const char *name, char *valueP[2],
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_Enum(hestOpt **hoptP, const char *flag,
                                           const char *name, int valueP[2],
                                           const char *dflt, const char *info,
                                           const airEnum *enm);
HEST_EXPORT unsigned int hestOptAdd_2_Other(hestOpt **hoptP, const char *flag,
                                            const char *name, void *valueP,
                                            const char *dflt, const char *info,
                                            const hestCB *CB);
HEST_EXPORT unsigned int hestOptAdd_3_Bool(hestOpt **hoptP, const char *flag,
                                           const char *name, int valueP[3],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_Int(hestOpt **hoptP, const char *flag,
                                          const char *name, int valueP[3],
                                          const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_UInt(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int valueP[3],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_LongInt(hestOpt **hoptP, const char *flag,
                                              const char *name, long int valueP[3],
                                              const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_ULongInt(hestOpt **hoptP, const char *flag,
                                               const char *name,
                                               unsigned long int valueP[3],
                                               const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_Size_t(hestOpt **hoptP, const char *flag,
                                             const char *name, size_t valueP[3],
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_Float(hestOpt **hoptP, const char *flag,
                                            const char *name, float valueP[3],
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_Double(hestOpt **hoptP, const char *flag,
                                             const char *name, double valueP[3],
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_Char(hestOpt **hoptP, const char *flag,
                                           const char *name, char valueP[3],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_String(hestOpt **hoptP, const char *flag,
                                             const char *name, char *valueP[3],
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_Enum(hestOpt **hoptP, const char *flag,
                                           const char *name, int valueP[3],
                                           const char *dflt, const char *info,
                                           const airEnum *enm);
HEST_EXPORT unsigned int hestOptAdd_3_Other(hestOpt **hoptP, const char *flag,
                                            const char *name, void *valueP,
                                            const char *dflt, const char *info,
                                            const hestCB *CB);
HEST_EXPORT unsigned int hestOptAdd_4_Bool(hestOpt **hoptP, const char *flag,
                                           const char *name, int valueP[4],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_Int(hestOpt **hoptP, const char *flag,
                                          const char *name, int valueP[4],
                                          const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_UInt(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int valueP[4],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_LongInt(hestOpt **hoptP, const char *flag,
                                              const char *name, long int valueP[4],
                                              const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_ULongInt(hestOpt **hoptP, const char *flag,
                                               const char *name,
                                               unsigned long int valueP[4],
                                               const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_Size_t(hestOpt **hoptP, const char *flag,
                                             const char *name, size_t valueP[4],
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_Float(hestOpt **hoptP, const char *flag,
                                            const char *name, float valueP[4],
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_Double(hestOpt **hoptP, const char *flag,
                                             const char *name, double valueP[4],
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_Char(hestOpt **hoptP, const char *flag,
                                           const char *name, char valueP[4],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_String(hestOpt **hoptP, const char *flag,
                                             const char *name, char *valueP[4],
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_Enum(hestOpt **hoptP, const char *flag,
                                           const char *name, int valueP[4],
                                           const char *dflt, const char *info,
                                           const airEnum *enm);
HEST_EXPORT unsigned int hestOptAdd_4_Other(hestOpt **hoptP, const char *flag,
                                            const char *name, void *valueP,
                                            const char *dflt, const char *info,
                                            const hestCB *CB);
HEST_EXPORT unsigned int hestOptAdd_N_Bool(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int N, int *valueP,
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_Int(hestOpt **hoptP, const char *flag,
                                          const char *name, unsigned int N, int *valueP,
                                          const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_UInt(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int N,
                                           unsigned int *valueP, const char *dflt,
                                           const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_LongInt(hestOpt **hoptP, const char *flag,
                                              const char *name, unsigned int N,
                                              long int *valueP, const char *dflt,
                                              const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_ULongInt(hestOpt **hoptP, const char *flag,
                                               const char *name, unsigned int N,
                                               unsigned long int *valueP,
                                               const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_Size_t(hestOpt **hoptP, const char *flag,
                                             const char *name, unsigned int N,
                                             size_t *valueP, const char *dflt,
                                             const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_Float(hestOpt **hoptP, const char *flag,
                                            const char *name, unsigned int N,
                                            float *valueP, const char *dflt,
                                            const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_Double(hestOpt **hoptP, const char *flag,
                                             const char *name, unsigned int N,
                                             double *valueP, const char *dflt,
                                             const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_Char(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int N,
                                           char *valueP, const char *dflt,
                                           const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_String(hestOpt **hoptP, const char *flag,
                                             const char *name, unsigned int N,
                                             char **valueP, const char *dflt,
                                             const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_Enum(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int N, int *valueP,
                                           const char *dflt, const char *info,
                                           const airEnum *enm);
HEST_EXPORT unsigned int hestOptAdd_N_Other(hestOpt **hoptP, const char *flag,
                                            const char *name, unsigned int N,
                                            void *valueP, const char *dflt,
                                            const char *info, const hestCB *CB);
HEST_EXPORT unsigned int hestOptAdd_Nv_Bool(hestOpt **hoptP, const char *flag,
                                            const char *name, unsigned int min, int max,
                                            int **valueP, const char *dflt,
                                            const char *info, unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_Int(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int min, int max,
                                           int **valueP, const char *dflt,
                                           const char *info, unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_UInt(hestOpt **hoptP, const char *flag,
                                            const char *name, unsigned int min, int max,
                                            unsigned int **valueP, const char *dflt,
                                            const char *info, unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_LongInt(hestOpt **hoptP, const char *flag,
                                               const char *name, unsigned int min,
                                               int max, long int **valueP,
                                               const char *dflt, const char *info,
                                               unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_ULongInt(hestOpt **hoptP, const char *flag,
                                                const char *name, unsigned int min,
                                                int max, unsigned long int **valueP,
                                                const char *dflt, const char *info,
                                                unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_Size_t(hestOpt **hoptP, const char *flag,
                                              const char *name, unsigned int min,
                                              int max, size_t **valueP, const char *dflt,
                                              const char *info, unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_Float(hestOpt **hoptP, const char *flag,
                                             const char *name, unsigned int min, int max,
                                             float **valueP, const char *dflt,
                                             const char *info, unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_Double(hestOpt **hoptP, const char *flag,
                                              const char *name, unsigned int min,
                                              int max, double **valueP, const char *dflt,
                                              const char *info, unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_Char(hestOpt **hoptP, const char *flag,
                                            const char *name, unsigned int min, int max,
                                            char **valueP, const char *dflt,
                                            const char *info, unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_String(hestOpt **hoptP, const char *flag,
                                              const char *name, unsigned int min,
                                              int max, char ***valueP, const char *dflt,
                                              const char *info, unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_Enum(hestOpt **hoptP, const char *flag,
                                            const char *name, unsigned int min, int max,
                                            int **valueP, const char *dflt,
                                            const char *info, unsigned int *sawP,
                                            const airEnum *enm);
HEST_EXPORT unsigned int hestOptAdd_Nv_Other(hestOpt **hoptP, const char *flag,
                                             const char *name, unsigned int min, int max,
                                             void *valueP, const char *dflt,
                                             const char *info, unsigned int *sawP,
                                             const hestCB *CB);

#ifdef __cplusplus
}
#endif

#endif /* HEST_HAS_BEEN_INCLUDED */
