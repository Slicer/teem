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
  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, see <https://www.gnu.org/licenses/>.
*/

#ifndef HEST_HAS_BEEN_INCLUDED
#define HEST_HAS_BEEN_INCLUDED

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#include <teem/air.h>
// and see privateHest.h for why we internally also use biff

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
 * The airType values are here in hest as a transition hack for TeemV2. The parsing needs
 * of hest are what motivated creating airTypes in the first place. The fresh perspective
 * of TeemV2 recognizes they should have been in hest from the outset, not in air.
 *
 * The blissfully-type-unaware hestOptAdd() has always relied on the airTypeT enum values
 * below. Since that function is not being removed, to avoid needless code breakage with
 * TeemV2, these values now live in this hest header. hest users should instead be using
 * the properly typed hestOptAdd_X_T functions, which have no need for airType
 * pseudo-types.
 *
 * Other things that used to be in air, but which really only mattered to implement hest
 * functions have been moved into privateHest.h, but with air --> hest renaming.
 *    #define AIR_TYPE_MAX
 *    const char airTypeStr[AIR_TYPE_MAX + 1][AIR_STRLEN_SMALL + 1];
 *    const size_t airTypeSize[AIR_TYPE_MAX + 1];
 *    unsigned int (*const airParseStr[AIR_TYPE_MAX + 1])(void *, const char *,const char
 *                                                        unsigned int)
 */
enum {
  airTypeUnknown, /*  0 */
  airTypeBool,    /*  1 */
  airTypeShort,   /*  2 (added for TeemV2) */
  airTypeUShort,  /*  3 (added for TeemV2) */
  airTypeInt,     /*  4 */
  airTypeUInt,    /*  5 */
  airTypeLong,    /*  6 (for TeemV2 renamed from airTypeLongInt) */
  airTypeULong,   /*  7 (for TeemV2 renamed from airTypeULongInt) */
  airTypeSize_t,  /*  8 */
  airTypeFloat,   /*  9 */
  airTypeDouble,  /* 10 */
  airTypeChar,    /* 11 */
  airTypeString,  /* 12 */
  airTypeEnum,    /* 13 */
  airTypeOther,   /* 14 */
  airTypeLast
};

/*
******** hestSource* enum
**
** way of identifying where the info to satisfy a particular option came from.
*/
enum {
  hestSourceUnknown,      /* 0 */
  hestSourceCommandLine,  /* 1 (formerly called hestSourceUser) */
  hestSourceResponseFile, /* 2 */
  hestSourceDefault,      /* 3 */
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
  /* if non-NULL, the destructor that will be called by hestParseFree() (or by
     hestParse() if there is an error during parsing). The argument is NOT the same as
     passed to parse(): it is the result of dereferencing the argument to parse() */
} hestCB;

/*
******** hestOpt struct
**
** information which specifies one command-line option, records state used during
** parsing, and provides summary output info following parsing.
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
  char *dflt,         /* default value(s) written out as string */
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
  /* Since hest's beginning in 2002, the basic container for a set of options was an
  array of hestOpt structs (not pointers to them, which rules out argv-style
  NULL-termination of the array), also unfortunately with no other top-level container
  or hestContext (which is why helpWanted below is set only in the first hestOpt of the
  array). hestOptAdd has historically reallocated the entire array, incrementing the
  length only by one with each call, while maintaining a single terminating hestOpt,
  wherein some fields were set to special values to indicate termination. With the 2023
  code revisit, that was deemed even uglier than the new and current hack: the first
  hestOpt now stores here in arrAlloc the allocated length of the hestOpt array, and in
  arrLen the number of hestOpts actually used and set. This facilitates implementing
  something much like an airArray, but without the burden of extra calls for the user
  (like airArrayLenIncr), nor new kinds of containers for hest and its users to manage:
  it is just the same array of hestOpt structs */
  unsigned int arrAlloc, arrLen;

  /* --------------------- Output
  Things set/allocated by hestParse. */

  /* from the hestSource* enum; from whence was this information learned. Can use
  hestSourceUser(opt->source) to test for the sources associated with the user:
  hestSourceCommandLine or hestSourceResponseFile */
  int source;
  /* if parseStr is non-NULL: a string (freed by hestParseFree) that is a lot like the
  string (storing zero or many parameters), from which hestParse ultimately parsed
  whatever values were set in *valueP above. Internally, hest maintains an argc,argv-like
  representation of the info to parse, but here it is joined back together into a
  space-delimited single string. Note that in the case of single variable parameter
  options used without a parameter, the value stored will be "inverted" from the string
  here. */
  char *parmStr;
  /* helpWanted indicates that hestParse() saw something (like "--help") in one of the
  given arguments that looks like a call for help, and that respectDashDashHelp is set in
  the hestParm. There is unfortunately no other top-level output container for info
  generated by hestParse(), so this field is going to be set only in the *first* hestOpt
  passed to hestParse(), even though that hestOpt has no particular relation to where
  hestParse() saw the call for help. */
  int helpWanted;
} hestOpt;

/*
******** hestParm struct
**
** parameters to control behavior of hest functions. Not to be confused with the
** "parameters" to a hestOpt from which it parses values. Code should use "hparm" for
** the pointer to this struct.
**
** GK: Don't even think about storing per-parse state in here.
*/
typedef struct {
  int verbosity,          /* verbose diagnostic messages to stdout */
    responseFileEnable,   /* whether or not to use response files */
    elideSingleEnumType,  /* if type is airTypeEnum, and if it's a single fixed parameter
                             option, then don't bother printing the type information as
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
    respectDashDashHelp, /* (new with TeemV2) hestParse interprets seeing "--help" as not
                         an error, but as a request to print usage info, so sets
                         helpWanted in the (first) hestOpt */
    respectDashBraceComments, /* (new with TeemV2) Sometimes in a huge command-line
                              invocation you want to comment part of it out. With this
                              set, hestParse recognizes hest-specific "-{" and "}-"
                              args as comment delimiters: these args and all args they
                              enclose are ignored. These must be stand-alone args, and
                              cannot be touching anything else, lest brace expansion
                              kick in. */
    noArgsIsNoProblem,     /* if non-zero, having no arguments to parse is not in and of
                           itself a problem; this means that if all options have defaults, it
                           would be *ok* to invoke the problem without any further
                           command-line options. This is counter to pre-Teem-1.11 behavior
                           (for which no arguments *always* meant "show me usage info"). */
    cleverPluralizeOtherY, /* when printing the type for airTypeOther, when the min
                              number of items is > 1, and the type string ends with "y",
                              then pluralize with "ies" instead of "ys" */
    dieLessVerbose, /* on parse failure, hestParseOrDie prints less than it otherwise
                       might: only print info and glossary when they "ask for it" */
    noBlankLineBeforeUsage; /* like it says */
  unsigned int columns;     /* number of printable columns in output */
} hestParm;

/*
The hestArg, hestArgVec, hestInput, and hestInputStack were all created for the TeemV2
rewrite of hestParse, to fix bugs and limits on how the code previously worked:
- Command-line arguments containing spaces were fully never correctly handled: the
  internal representation of one argument, amidst a space-seperated string of all
  arguments (why?!?) put back in ""-quoting, but it was never correctly implemented.
  Now the internal representation of argv is with an array data structure, not a single
  string that has to be retokenized.
- When parsing response files, ""-quoted strings were never handled at all (nor was "#"
  appearing within a string), and response files could not invoke other response files.
- Can now support long-wanted feature: commenting out of some span of arguments, with
  new hest-specific "-{" "}-" delimiters.  As long as these are space-separated from
  other args, these are left intact by sh, bash, and zsh (csh and tcsh get confused).
  They must be kept as separate args to avoid brace expansion.
*/

// hestArg: for building up and representing one argument
typedef struct {
  char *str;
  unsigned int len; // NOT strlen; this includes '\0'-termination
  airArray *strArr;
} hestArg;

// hestArgVec: for building up a "vector" of arguments
typedef struct {
  hestArg *harg; /* array of hestArgs */
  unsigned int len;
  airArray *hargArr;
} hestArgVec;

// hestInput: what is the thing we're processing now to build up an arg vec
typedef struct {
  int source; // from the hestSource* enum
  // ------ if source == hestSourceCommandLine ------
  unsigned int argc;
  const char **argv; // we do NOT own
  unsigned int argIdx;
  // ------ if source == hestSourceResponseFile ------
  char *rfname; // we DO own
  FILE *rfile;  // user opens and closes this
  // ------ if source == hestSourceDefault ------
  const char *dfltStr;  // we do NOT own
  unsigned int dfltLen; // strlen(dfltStr)
  // for both hestSourceResponseFile and hestSourceDefault
  unsigned int carIdx; // which character are we on
  // ------ general for all inputs ------
  unsigned int dashBraceComment; /* not a boolean: how many -{ }- comment levels
                                    deep are we currently; tracked this way to
                                    permit nested commenting */
} hestInput;

typedef struct {
  hestInput *hin; // array of hestInputs
  unsigned int len;
  airArray *hinArr;
  int stdinRead; // while processing this stack we have read in "-" aka stdin
} hestInputStack;

// defaultsHest.c
HEST_EXPORT int hestDefaultVerbosity;
HEST_EXPORT int hestDefaultResponseFileEnable;
HEST_EXPORT int hestDefaultElideSingleEnumType;
HEST_EXPORT int hestDefaultElideSingleOtherType;
HEST_EXPORT int hestDefaultElideSingleOtherDefault;
HEST_EXPORT int hestDefaultElideSingleNonExistFloatDefault;
HEST_EXPORT int hestDefaultElideMultipleNonExistFloatDefault;
HEST_EXPORT int hestDefaultElideSingleEmptyStringDefault;
HEST_EXPORT int hestDefaultElideMultipleEmptyStringDefault;
HEST_EXPORT int hestDefaultNoArgsIsNoProblem;
HEST_EXPORT int hestDefaultCleverPluralizeOtherY;
HEST_EXPORT unsigned int hestDefaultColumns;

// argvHest.c
HEST_EXPORT hestArg *hestArgNew(void);
HEST_EXPORT hestArg *hestArgNix(hestArg *harg);
HEST_EXPORT void hestArgReset(hestArg *harg);
HEST_EXPORT void hestArgAddChar(hestArg *harg, char cc);
HEST_EXPORT void hestArgAddString(hestArg *harg, const char *str);
HEST_EXPORT hestArgVec *hestArgVecNew(void);
HEST_EXPORT hestArgVec *hestArgVecNix(hestArgVec *havec);
HEST_EXPORT void hestArgVecAppendString(hestArgVec *havec, const char *str);
HEST_EXPORT void hestArgVecPrint(const char *caller, const hestArgVec *havec);
HEST_EXPORT hestInput *hestInputNew(void);
HEST_EXPORT hestInput *hestInputNix(hestInput *hin);
HEST_EXPORT hestInputStack *hestInputStackNew(void);
HEST_EXPORT hestInputStack *hestInputStackNix(hestInputStack *hist);

// parsest.c
HEST_EXPORT int hestParse2(hestOpt *opt, int argc, const char **argv, char **errP,
                           const hestParm *hparm);

// methodsHest.c
HEST_EXPORT const int hestPresent;
HEST_EXPORT const airEnum *const hestSource;
HEST_EXPORT int hestSourceUser(int src);
HEST_EXPORT hestParm *hestParmNew(void);
HEST_EXPORT hestParm *hestParmFree(hestParm *hparm);
HEST_EXPORT void *hestParmFree_vp(void *hparm);
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
// SEE ALSO (from adders.c) all the 99 type-checked versions of hestOptAdd_, below!
HEST_EXPORT unsigned int hestOptNum(const hestOpt *opt);
HEST_EXPORT hestOpt *hestOptFree(hestOpt *opt);
HEST_EXPORT void *hestOptFree_vp(void *opt);
HEST_EXPORT int hestOptCheck(const hestOpt *opt, char **errP);
HEST_EXPORT int hestOptParmCheck(const hestOpt *opt, const hestParm *hparm, char **errP);

// parseHest.c
HEST_EXPORT int hestParse(hestOpt *opt, int argc, const char **argv, char **errP,
                          const hestParm *hparm);
HEST_EXPORT void *hestParseFree(hestOpt *opt);
HEST_EXPORT void hestParseOrDie(hestOpt *opt, int argc, const char **argv,
                                hestParm *hparm, const char *me, const char *info,
                                int doInfo, int doUsage, int doGlossary);

// usage.c
HEST_EXPORT void _hestPrintStr(FILE *f, unsigned int indent, unsigned int already,
                               unsigned int width, const char *_str, int bslash);
HEST_EXPORT void hestUsage(FILE *file, const hestOpt *opt, const char *argv0,
                           const hestParm *hparm);
HEST_EXPORT void hestGlossary(FILE *file, const hestOpt *opt, const hestParm *hparm);
HEST_EXPORT void hestInfo(FILE *file, const char *argv0, const char *info,
                          const hestParm *hparm);

// adders.c
HEST_EXPORT void hestOptAddDeclsPrint(FILE *f);
/* The 99 (!) non-var-args alternatives to hestOptAdd, also usefully type-specific for
the type of value to be parsed in a way that hestOptAdd_nva cannot match. These capture
all the common uses (and then some) of hest within Teem. They are named according to
kind, and according to the type T of the parms to be parsed for each option:

min, max             function family       kind  description
min == max == 0      hestOptAdd_Flag         1   (stand-alone flag; no parameters)
min == max == 1      hestOptAdd_1_T          2   single fixed parameter
min == max >= 2      hestOptAdd_{2,3,4,N}_T  3   multiple fixed parameters
min == 0; max == 1   hestOptAdd_1v_T         4   single variable parameter
min < max; max >= 2  hestOptAdd_Nv_T         5   multiple variable parameters

The type T can be: Bool, Short, UShort, Int, UInt, Long, ULong, Size_t, Float, Double,
Char, String, Enum, or Other. An airEnum* is passed with the T=Enum functions, or a
hestCB* is passed for the T=Other functions. The number of parameters *sawP that hestParm
saw on the command-line is passed for the _Nv_ options.

All declarations below were automatically generated via hest/test/decls (which calls
hestOptAddDeclsPrint), followed by clang-format. */
HEST_EXPORT unsigned int hestOptAdd_Flag(hestOpt **optP, const char *flag, int *valueP,
                                         const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_Bool(hestOpt **hoptP, const char *flag,
                                            const char *name, int *valueP,
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_Short(hestOpt **hoptP, const char *flag,
                                             const char *name, short int *valueP,
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_UShort(hestOpt **hoptP, const char *flag,
                                              const char *name,
                                              unsigned short int *valueP,
                                              const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_Int(hestOpt **hoptP, const char *flag,
                                           const char *name, int *valueP,
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_UInt(hestOpt **hoptP, const char *flag,
                                            const char *name, unsigned int *valueP,
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_Long(hestOpt **hoptP, const char *flag,
                                            const char *name, long int *valueP,
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1v_ULong(hestOpt **hoptP, const char *flag,
                                             const char *name, unsigned long int *valueP,
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
HEST_EXPORT unsigned int hestOptAdd_1_Short(hestOpt **hoptP, const char *flag,
                                            const char *name, short int *valueP,
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_UShort(hestOpt **hoptP, const char *flag,
                                             const char *name,
                                             unsigned short int *valueP,
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_Int(hestOpt **hoptP, const char *flag,
                                          const char *name, int *valueP,
                                          const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_UInt(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int *valueP,
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_Long(hestOpt **hoptP, const char *flag,
                                           const char *name, long int *valueP,
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_1_ULong(hestOpt **hoptP, const char *flag,
                                            const char *name, unsigned long int *valueP,
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
HEST_EXPORT unsigned int hestOptAdd_2_Short(hestOpt **hoptP, const char *flag,
                                            const char *name, short int valueP[2],
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_UShort(hestOpt **hoptP, const char *flag,
                                             const char *name,
                                             unsigned short int valueP[2],
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_Int(hestOpt **hoptP, const char *flag,
                                          const char *name, int valueP[2],
                                          const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_UInt(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int valueP[2],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_Long(hestOpt **hoptP, const char *flag,
                                           const char *name, long int valueP[2],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_2_ULong(hestOpt **hoptP, const char *flag,
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
HEST_EXPORT unsigned int hestOptAdd_3_Short(hestOpt **hoptP, const char *flag,
                                            const char *name, short int valueP[3],
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_UShort(hestOpt **hoptP, const char *flag,
                                             const char *name,
                                             unsigned short int valueP[3],
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_Int(hestOpt **hoptP, const char *flag,
                                          const char *name, int valueP[3],
                                          const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_UInt(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int valueP[3],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_Long(hestOpt **hoptP, const char *flag,
                                           const char *name, long int valueP[3],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_3_ULong(hestOpt **hoptP, const char *flag,
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
HEST_EXPORT unsigned int hestOptAdd_4_Short(hestOpt **hoptP, const char *flag,
                                            const char *name, short int valueP[4],
                                            const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_UShort(hestOpt **hoptP, const char *flag,
                                             const char *name,
                                             unsigned short int valueP[4],
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_Int(hestOpt **hoptP, const char *flag,
                                          const char *name, int valueP[4],
                                          const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_UInt(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int valueP[4],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_Long(hestOpt **hoptP, const char *flag,
                                           const char *name, long int valueP[4],
                                           const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_4_ULong(hestOpt **hoptP, const char *flag,
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
HEST_EXPORT unsigned int hestOptAdd_N_Short(hestOpt **hoptP, const char *flag,
                                            const char *name, unsigned int N,
                                            short int *valueP, const char *dflt,
                                            const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_UShort(hestOpt **hoptP, const char *flag,
                                             const char *name, unsigned int N,
                                             unsigned short int *valueP,
                                             const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_Int(hestOpt **hoptP, const char *flag,
                                          const char *name, unsigned int N, int *valueP,
                                          const char *dflt, const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_UInt(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int N,
                                           unsigned int *valueP, const char *dflt,
                                           const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_Long(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int N,
                                           long int *valueP, const char *dflt,
                                           const char *info);
HEST_EXPORT unsigned int hestOptAdd_N_ULong(hestOpt **hoptP, const char *flag,
                                            const char *name, unsigned int N,
                                            unsigned long int *valueP, const char *dflt,
                                            const char *info);
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
HEST_EXPORT unsigned int hestOptAdd_Nv_Short(hestOpt **hoptP, const char *flag,
                                             const char *name, unsigned int min, int max,
                                             short int **valueP, const char *dflt,
                                             const char *info, unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_UShort(hestOpt **hoptP, const char *flag,
                                              const char *name, unsigned int min,
                                              int max, unsigned short int **valueP,
                                              const char *dflt, const char *info,
                                              unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_Int(hestOpt **hoptP, const char *flag,
                                           const char *name, unsigned int min, int max,
                                           int **valueP, const char *dflt,
                                           const char *info, unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_UInt(hestOpt **hoptP, const char *flag,
                                            const char *name, unsigned int min, int max,
                                            unsigned int **valueP, const char *dflt,
                                            const char *info, unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_Long(hestOpt **hoptP, const char *flag,
                                            const char *name, unsigned int min, int max,
                                            long int **valueP, const char *dflt,
                                            const char *info, unsigned int *sawP);
HEST_EXPORT unsigned int hestOptAdd_Nv_ULong(hestOpt **hoptP, const char *flag,
                                             const char *name, unsigned int min, int max,
                                             unsigned long int **valueP,
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
