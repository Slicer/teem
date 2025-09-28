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

#include "hest.h"
#include "privateHest.h"

#include <sys/ioctl.h> // for ioctl(), TIOCGWINSZ, struct winsize
#include <unistd.h>    // for STDOUT_FILENO and friends

const int hestPresent = 42;

const char *const _hestBiffKey = "hest";

// see note in hest.h about why airType things are here, renamed as hest
/* clang-format off */
const char
_hestTypeStr[_HEST_TYPE_MAX+1][AIR_STRLEN_SMALL+1] = {
  "(unknown)",
  "bool",
  "short",
  "unsigned short",
  "int",
  "unsigned int",
  "long int",
  "unsigned long int",
  "size_t",
  "float",
  "double",
  "char",
  "string",
  "enum",
  "other",
};

const size_t
_hestTypeSize[_HEST_TYPE_MAX+1] = {
  0,
  sizeof(int),
  sizeof(short),
  sizeof(unsigned short),
  sizeof(int),
  sizeof(unsigned int),
  sizeof(long int),
  sizeof(unsigned long int),
  sizeof(size_t),
  sizeof(float),
  sizeof(double),
  sizeof(char),
  sizeof(char*),
  sizeof(int),
  0   /* we don't know anything about size of type "other" */
};
/* clang-format on */

/* now (in 2025) that we've done all this work to preserve the command-line argv[]
tokens, and to properly tokenize default strings and response files, we should stop using
the airParseStrT functions that internally use airStrtok(): we have exactly one token to
parse. These functions thus return non-zero in case of error, instead of returning the
number of parsed values. */
static int
parseSingleB(void *_out, const char *str, _hestPPack *hpp) {
  if (!(_out && str && hpp)) return 1;
  int *out = (int *)_out;
  *out = airEnumVal(airBool, str);
  int ret = airEnumUnknown(airBool) /* which is -1 */ == *out;
  if (ret) {
    snprintf(hpp->err, AIR_STRLEN_HUGE + 1, "couldnt parse \"%s\" as %s", str,
             airBool->name);
  } else {
    hpp->err[0] = '\0';
  }
  return ret;
}

#define _PARSE_1_ARGS void *out, const char *str, _hestPPack *hpp
#define _PARSE_1_BODY(typstr, format)                                                   \
  if (!(out && str && hpp)) return 1;                                                   \
  int ret = (1 != airSingleSscanf(str, format, out));                                   \
  if (!strcmp("parseSingleI", __func__)) {                                              \
    printf("!%s: sscanf(|%s|, %s, %p)-> (ret=%d) %d\n", __func__, str, format, out,     \
           ret, *((int *)out));                                                         \
  }                                                                                     \
  if (ret) {                                                                            \
    snprintf(hpp->err, AIR_STRLEN_HUGE + 1, "couldn't parse \"%s\" as", typstr);        \
  } else {                                                                              \
    hpp->err[0] = '\0';                                                                 \
  }                                                                                     \
  return ret

// clang-format off
static int parseSingleH (_PARSE_1_ARGS) { _PARSE_1_BODY("short",          "%hd"); }
static int parseSingleUH(_PARSE_1_ARGS) { _PARSE_1_BODY("unsigned short", "%hu"); }
static int parseSingleI (_PARSE_1_ARGS) { _PARSE_1_BODY("int",            "%d" ); }
static int parseSingleUI(_PARSE_1_ARGS) { _PARSE_1_BODY("unsigned int",   "%u" ); }
static int parseSingleL (_PARSE_1_ARGS) { _PARSE_1_BODY("long",           "%ld"); }
static int parseSingleUL(_PARSE_1_ARGS) { _PARSE_1_BODY("unsigned long",  "%lu"); }
static int parseSingleZ (_PARSE_1_ARGS) { _PARSE_1_BODY("size_t",         "%z" ); }
static int parseSingleF (_PARSE_1_ARGS) { _PARSE_1_BODY("float",          "%f" ); }
static int parseSingleD (_PARSE_1_ARGS) { _PARSE_1_BODY("double",         "%lf"); }
// clang-format on
static int
parseSingleC(void *_out, const char *str, _hestPPack *hpp) {
  if (!(_out && str && hpp)) return 1;
  size_t slen = strlen(str);
  int ret;
  if (1 != slen) {
    snprintf(hpp->err, AIR_STRLEN_HUGE + 1,
             "expected single char but got string length %u", AIR_UINT(slen));
    ret = 1;
  } else {
    char *out = (char *)_out;
    *out = str[0];
    hpp->err[0] = '\0';
    ret = 0;
  }
  return ret;
}
static int
parseSingleS(void *_out, const char *str, _hestPPack *hpp) {
  if (!(_out && str && hpp)) return 1;
  char **out = (char **)_out;
  *out = airStrdup(str);
  int ret = !(*out); // a NULL pointer result of strdup is a problem
  if (ret) {
    snprintf(hpp->err, AIR_STRLEN_HUGE + 1, "airStrdup failed!");
  } else {
    hpp->alloc = 1;
    airMopMem(hpp->cmop, *out, airMopOnError);
    hpp->err[0] = '\0';
  }
  return ret;
}
static int
parseSingleE(void *_out, const char *str, _hestPPack *hpp) {
  if (!(_out && str && hpp)) return 1;
  int *out = (int *)_out;
  *out = airEnumVal(hpp->enm, str);
  int ret = (airEnumUnknown(hpp->enm) == *out);
  if (ret) {
    snprintf(hpp->err, AIR_STRLEN_HUGE + 1, "couldn't parse \"%s\" as %s", str,
             hpp->enm->name);
  } else {
    hpp->err[0] = '\0';
  }
  return ret;
}
static int
parseSingleO(void *out, const char *str, _hestPPack *hpp) {
  if (!(out && str && hpp)) return 1;
  char myerr[AIR_STRLEN_HUGE + 1];
  int ret = hpp->CB->parse(out, str, myerr);
  if (ret) {
    if (strlen(myerr)) {
      snprintf(hpp->err, AIR_STRLEN_HUGE + 1, "error parsing \"%s\" as %s:\n%s\n", str,
               hpp->CB->type, myerr);
    } else {
      snprintf(hpp->err, AIR_STRLEN_HUGE + 1,
               "error parsing \"%s\" as %s: returned %d\n", str, hpp->CB->type, ret);
    }
  } else {
    if (hpp->CB->destroy) {
      /* out is the address of a void*, we manage the void* */
      hpp->alloc = 1;
      airMopAdd(hpp->cmop, (void **)out, (airMopper)airSetNull, airMopOnError);
      airMopAdd(hpp->cmop, *((void **)out), hpp->CB->destroy, airMopOnError);
    }
  }
  return ret;
}

int (*const _hestParseSingle[_HEST_TYPE_MAX + 1])(void *, const char *, _hestPPack *) = {
  NULL,          //
  parseSingleB,  //
  parseSingleH,  //
  parseSingleUH, //
  parseSingleI,  //
  parseSingleUI, //
  parseSingleL,  //
  parseSingleUL, //
  parseSingleZ,  //
  parseSingleF,  //
  parseSingleD,  //
  parseSingleC,  //
  parseSingleS,  //
  parseSingleE,  //
  parseSingleO   //
};

#define _INVERT_SCALAR(TT, ctype)                                                       \
  static void _invertScalar##TT(void *_valP) {                                          \
    ctype *valP = (ctype *)_valP;                                                       \
    ctype val = *valP;                                                                  \
    *valP = !val;                                                                       \
  }
_INVERT_SCALAR(B, int)
_INVERT_SCALAR(H, short)
_INVERT_SCALAR(UH, unsigned short)
_INVERT_SCALAR(I, int)
_INVERT_SCALAR(UI, unsigned int)
_INVERT_SCALAR(L, long)
_INVERT_SCALAR(UL, unsigned long)
_INVERT_SCALAR(Z, size_t)
_INVERT_SCALAR(F, float)
_INVERT_SCALAR(D, double)
// not: C, char
// not: S, char *
// not: E, int
// not: ?, "other"

void (*const _hestInvertScalar[_HEST_TYPE_MAX + 1])(void *) = {
  NULL,            //
  _invertScalarB,  //
  _invertScalarH,  //
  _invertScalarUH, //
  _invertScalarI,  //
  _invertScalarUI, //
  _invertScalarL,  //
  _invertScalarUL, //
  _invertScalarZ,  //
  _invertScalarF,  //
  _invertScalarD,  //
  NULL,            // not C, char
  NULL,            // not S, char *
  NULL,            // not E, int
  NULL             // not ?, "other"
};

// HEY these are sticking around just for the old implementation of hestParse
unsigned int (*const _hestParseStr[_HEST_TYPE_MAX + 1])(void *, const char *,
                                                        const char *, unsigned int)
  = {NULL,
     (unsigned int (*)(void *, const char *, const char *, unsigned int))airParseStrB,
     (unsigned int (*)(void *, const char *, const char *, unsigned int))airParseStrH,
     (unsigned int (*)(void *, const char *, const char *, unsigned int))airParseStrUH,
     (unsigned int (*)(void *, const char *, const char *, unsigned int))airParseStrI,
     (unsigned int (*)(void *, const char *, const char *, unsigned int))airParseStrUI,
     (unsigned int (*)(void *, const char *, const char *, unsigned int))airParseStrL,
     (unsigned int (*)(void *, const char *, const char *, unsigned int))airParseStrUL,
     (unsigned int (*)(void *, const char *, const char *, unsigned int))airParseStrZ,
     (unsigned int (*)(void *, const char *, const char *, unsigned int))airParseStrF,
     (unsigned int (*)(void *, const char *, const char *, unsigned int))airParseStrD,
     (unsigned int (*)(void *, const char *, const char *, unsigned int))airParseStrC,
     (unsigned int (*)(void *, const char *, const char *, unsigned int))airParseStrS,
     // airParseStrE needs final airEnum* arg, no longer enforceing fake uniformity
     NULL,
     // no uniform of parsing type "other"; handled via hestCB in hest
     NULL};

// loving how C99 simplifies creating an airEnum at compile-time
static const airEnum _hestSource
  = {.name = "source",
     .M = 3,
     .str = (const char *[]){"(unknown_source)", // 0
                             "command-line",     // 1
                             "response-file",    // 2
                             "default"},         // 3
     .val = NULL,
     .desc = (const char *[]){"unknown source",         //
                              "argc/argv command-line", //
                              "a response file",        //
                              "default string in hestOpt"},
     .strEqv = (const char *[]){"command-line", "cmdline",   //
                                "response-file", "respfile", //
                                "default",                   //
                                ""},
     .valEqv = (const int[]){hestSourceCommandLine, hestSourceCommandLine,   //
                             hestSourceResponseFile, hestSourceResponseFile, //
                             hestSourceDefault},
     .sense = AIR_FALSE};
const airEnum *const hestSource = &_hestSource;

int
hestSourceUser(int src) {
  return (hestSourceCommandLine == src || hestSourceResponseFile == src);
}

/* INCR is like airArray->incr: granularity with which we (linearly) reallocate the
hestOpt array. Very few uses of hest within Teem use more than 32 options. Hopefully
this avoids all the reallocations in the past action of hestOptAdd and the like. */
#define INCR 32

hestParm *
hestParmNew() {
  hestParm *hparm;

  hparm = AIR_CALLOC(1, hestParm);
  assert(hparm);
  hparm->verbosity = hestDefaultVerbosity;
  hparm->responseFileEnable = hestDefaultResponseFileEnable;
  hparm->elideSingleEnumType = hestDefaultElideSingleEnumType;
  hparm->elideSingleOtherType = hestDefaultElideSingleOtherType;
  hparm->elideSingleNonExistFloatDefault = hestDefaultElideSingleNonExistFloatDefault;
  hparm->elideMultipleNonExistFloatDefault
    = hestDefaultElideMultipleNonExistFloatDefault;
  hparm->elideSingleEmptyStringDefault = hestDefaultElideSingleEmptyStringDefault;
  hparm->elideMultipleEmptyStringDefault = hestDefaultElideMultipleEmptyStringDefault;
  /* It would be really nice for parm->respectDashDashHelp to default to true:
  widespread conventions say what "--help" should mean e.g. https://clig.dev/#help
  HOWEVER, the problem is with how hestParse is called and how its return value
  is interpreted as a boolean:
  - zero has meant that hestParse could set values for all the options (either
    from the command-line or from supplied defaults), and
  - non-zero has meant that there was an error parsing the command-line arguments
  But seeing and recognizing "--help" means that options have NOT had values
  set, and, that's not an error, which is outside that binary.  But that binary
  is the precedent, so we have to work with it by default.
  Now, with parm->respectDashDashHelp, upon seeing "--help", hestParse returns 0,
  and sets helpWanted in the first hestOpt, and the caller will have to know
  to check for that.  This logic is handled by hestParse but maybe in the future there
  can be a different top-level parser function that turns on parm->respectDashDashHelp
  and knows how to check the results */
  hparm->respectDashDashHelp = AIR_FALSE;
  /* for these most recent addition to the hestParm,
     abstaining from adding yet another default global variable */
  hparm->respectDashBraceComments = AIR_TRUE;
  hparm->noArgsIsNoProblem = hestDefaultNoArgsIsNoProblem;
  hparm->cleverPluralizeOtherY = hestDefaultCleverPluralizeOtherY;
  /* here too: newer addition to hestParm avoid adding another default global */
  hparm->dieLessVerbose = AIR_FALSE;
  hparm->noBlankLineBeforeUsage = AIR_FALSE;
  hparm->columns = hestDefaultColumns;
  // see note in privateHest.h about the removal of these
  // hparm->responseFileFlag = hestDefaultRespFileFlag;
  // hparm->responseFileComment = hestDefaultRespFileComment;
  // hparm->varParamStopFlag = hestDefaultVarParamStopFlag;
  // hparm->multiFlagSep = hestDefaultMultiFlagSep;
  return hparm;
}

hestParm *
hestParmFree(hestParm *hparm) {

  airFree(hparm);
  return NULL;
}

/* hestParmColumnsIoctl:
Try to dynamically learn number of columns in the current terminal from ioctl(), and save
it in hparm->columns. Learning the terminal size from stdin will probably work if we're
not being piped into, else try learning it from stdout (but that won't work if we're
piping elsewhere), else try learning the terminal size from stderr.

If one of these works, and returns a reasonably large-enough value for #columns, then
then hparm->columns is set via the ioctl-generated info, and we return 0.  "large-enough"
means bigger than sanity threshold of max(20, hestDefaultColumns/2); if not above that
threshold, then hparm->columns is set to it and we return -1. Why bother with this
threshold: hest usage generation code isn't trusted to produce anything informative with
a tiny number of columns (and certainly hasn't been well-tested with that).

If ioctl() never worked, then hparm->columns gets the given nonIoctlColumns, and we
return 1 (but this 1 is not an error that needs any recovering from). */
int
hestParmColumnsIoctl(hestParm *hparm, unsigned int nonIoctlColumns) {
  struct winsize wsz;
  int ret;
  if (-1 != ioctl(STDIN_FILENO, TIOCGWINSZ, &wsz)
      || -1 != ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsz)
      || -1 != ioctl(STDERR_FILENO, TIOCGWINSZ, &wsz)) {
    /* one of the ioctl calls worked */
    unsigned int sanemin;
    /* the "- 2" here may be the sign of a hest bug; sometimes it seems the "\" for line
    continuation (in generated usage info) causes a line wrap when it shouldn't */
    hparm->columns = wsz.ws_col - 2;
    sanemin = AIR_MAX(20, hestDefaultColumns / 2);
    if (hparm->columns < sanemin) {
      /* will ignore the too-small value ioctl produced */
      hparm->columns = sanemin;
      ret = -1;
    } else {
      /* ioctl didn't say something crazy; we keep it */
      ret = 0;
    }
  } else {
    hparm->columns = nonIoctlColumns;
    ret = 1;
  }
  return ret;
}

// _hestMax(-1) == INT_MAX, otherwise _hestMax(m) == m
int
_hestMax(int max) {

  if (-1 == max) {
    max = INT_MAX;
  }
  return max;
}

/* minmaxKind determines the kind (1,2,3,4, or 5) of an opt,
   based on the min and max fields from the hestOpt */
static int
minmaxKind(unsigned int min, int _max) {
  int ret;
  int max = _hestMax(_max);
  if (AIR_INT(min) > max) {
    /* invalid */
    ret = -1;
  } else { // else min <= max
    if (AIR_INT(min) == max) {
      if (0 == min) {
        // stand-alone flag
        ret = 1;
      } else if (1 == min) {
        // single fixed parm
        ret = 2;
      } else { // min==max >= 2
        // multiple fixed parms
        ret = 3;
      }
    } else { // else min < max
      if (0 == min && 1 == max) {
        // weirdo: single optional parameter
        ret = 4;
      } else {
        // multiple variadic parameters
        ret = 5;
      }
    }
  }
  return ret;
}

/* initializes all of a hestOpt, even arrAlloc and arrLen */
static void
optInit(hestOpt *opt) {

  opt->flag = NULL;
  opt->name = NULL;
  opt->type = airTypeUnknown; /* h== 0 */
  opt->min = 0;
  opt->max = 0;
  opt->valueP = NULL;
  opt->dflt = opt->info = NULL;
  opt->sawP = NULL;
  opt->enm = NULL;
  opt->CB = NULL;
  opt->sawP = NULL;
  opt->kind = 0; /* means that this hestOpt has not been set */
  opt->alloc = 0;
  opt->havec = NULL;
  opt->arrAlloc = opt->arrLen = 0;
  opt->source = hestSourceUnknown;
  opt->parmStr = NULL;
  opt->helpWanted = AIR_FALSE;
}

/*
hestOptNum: returns the number of elements in the given hestOpt array

Unfortunately, unlike argv itself, there is no sense in which the hestOpt array can be
NULL-terminated, mainly because "opt" is an array of hestOpt structs, not an array of
pointers to hestOpt structs. Pre-2023, this function did clever things to detect the
terminating hestOpt, but with the introduction of arrAlloc and arrLen that is moot.
*/
unsigned int
hestOptNum(const hestOpt *opt) {
  return opt ? opt->arrLen : 0;
}

/* like airArrayNew: create an initial segment of the hestOpt array */
static void
optarrNew(hestOpt **optP) {
  unsigned int opi;
  hestOpt *ret = AIR_CALLOC(INCR, hestOpt);
  assert(ret);
  for (opi = 0; opi < INCR; opi++) {
    optInit(ret + opi);
  }
  ret->arrAlloc = INCR;
  ret->arrLen = 0;
  *optP = ret;
  return;
}

/* line airArrayLenIncr(1): increments logical length by 1,
and returns index of newly-available element */
static unsigned int
optarrIncr(hestOpt **optP) {
  unsigned int olen, nlen;
  olen = (*optP)->arrLen; /* == index of new element */
  nlen = olen + 1;
  if (nlen > (*optP)->arrAlloc) {
    unsigned int opi;
    /* just walked off end of allocated length: reallocate */
    hestOpt *nopt = AIR_CALLOC((*optP)->arrAlloc + INCR, hestOpt);
    assert(nopt);
    memcpy(nopt, *optP, olen * sizeof(hestOpt));
    nopt->arrAlloc = (*optP)->arrAlloc + INCR;
    for (opi = olen; opi < nopt->arrAlloc; opi++) {
      optInit(nopt + opi);
    }
    free(*optP);
    *optP = nopt;
  }
  (*optP)->arrLen = nlen;
  return olen;
}

/*
hestOptSingleSet: a completely generic setter for a single hestOpt
Note that this makes no attempt at error-checking; that is all in _hestOPCheck.
*THIS* is the function that sets opt->kind.
*/
void
hestOptSingleSet(hestOpt *opt, const char *flag, const char *name, int type,
                 unsigned int min, int max, void *valueP, const char *dflt,
                 const char *info, unsigned int *sawP, const airEnum *enm,
                 const hestCB *CB) {

  if (!opt) return;
  opt->flag = airStrdup(flag);
  opt->name = airStrdup(name);
  opt->type = type;
  opt->min = min;
  opt->max = max;
  opt->valueP = valueP;
  opt->dflt = airStrdup(dflt);
  opt->info = airStrdup(info);
  // need to set kind now so can be used in later conditionals
  opt->kind = minmaxKind(min, max);
  // deal with (what used to be) var args
  opt->sawP = (5 == opt->kind /* */
                 ? sawP
                 : NULL);
  opt->enm = (airTypeEnum == type /* */
                ? enm
                : NULL);
  opt->CB = (airTypeOther == type /* */
               ? CB
               : NULL);
  // alloc set by hestParse
  opt->havec = hestArgVecNew();
  // leave arrAlloc, arrLen untouched: managed by caller
  // yes, redundant with optInit()
  opt->source = hestSourceUnknown;
  opt->parmStr = NULL;
  opt->helpWanted = AIR_FALSE;

  if (airTypeInt == type && 1 == min && 1 == max) {
    printf("!something like %s: got valueP %p\n", "hestOptAdd_1_Int", AIR_VOIDP(valueP));
  }
  return;
}

/*
hestOptAdd_nva: A new (as of 2023) non-var-args ("_nva") version of hestOptAdd;
The per-hestOpt logic (including setting opt->kind) has now been moved to
hestOptSingleSet. The venerable var-args hestOptAdd is now a wrapper around this.
and all the 99 non-var-args fully typed hestOptAdd_* functions also call this.

Like hestOptAdd has done since 2013: returns UINT_MAX in case of error.

NOTE that we do NOT do here ANY error checking on the validity of the arguments passed,
e.g. enforcing that we have a non-NULL sawP iff this is a multi-variadic parameter
option, or that without a flag (`flag` is NULL) we must have min > 0.  All of that is
done later, in _hestOPCheck.
*/
unsigned int
hestOptAdd_nva(hestOpt **optP, const char *flag, const char *name, int type,
               unsigned int min, int max, void *valueP, const char *dflt,
               const char *info, unsigned int *sawP, const airEnum *enm,
               const hestCB *CB) {
  unsigned int retIdx;

  /* NULL address of opt array: can't proceed */
  if (!optP) return UINT_MAX;
  /* initialize hestOpt array if necessary */
  if (!(*optP)) {
    optarrNew(optP);
  }
  /* increment logical length of hestOpt array; return index of opt being set here */
  retIdx = optarrIncr(optP);
  /* set all elements of the opt */
  hestOptSingleSet(*optP + retIdx, flag, name, type, min, max, /* */
                   valueP, dflt, info,                         /* */
                   sawP, enm, CB);
  return retIdx;
}

/*
 * hestOptAdd
 *
 * Until 2023, this was the main way of using hest: a var-args function that could do no
 * useful type-checking, and was very easy to call incorrectly, leading to inscrutable
 * errors.
 *
 * Now, thankfully, you have 99 better hestOptAdd_ functions to use instead:
 * hestOptAdd_Flag, hestOptAdd_1_T, hestOptAdd_{2,3,4,N}_T, hestOptAdd_1v_T, or
 * hestOptAdd_Nv_T for T=Bool, Short, UShort, Int, UInt, LongInt, ULongInt, Size_t,
 * Float, Double, Char, String, Enum, or Other.
 *
 * This returns the index of the option just added, so the caller can remember it and
 * thus speed up later checking the `hestOpt->source` to learn where how the option was
 * parsed. Returns UINT_MAX in case of error.
 */
unsigned int
hestOptAdd(hestOpt **optP, const char *flag, const char *name, int type,
           unsigned int min, int max, void *valueP, const char *dflt, const char *info,
           ...) {
  unsigned int *sawP = NULL;
  const airEnum *enm = NULL;
  const hestCB *CB = NULL;
  va_list ap;

  if (!optP) return UINT_MAX;
  /* deal with var args */
  if (5 == minmaxKind(min, max)) {
    va_start(ap, info);
    sawP = va_arg(ap, unsigned int *);
    va_end(ap);
  }
  if (airTypeEnum == type) {
    va_start(ap, info);
    va_arg(ap, unsigned int *); /* skip sawP */
    enm = va_arg(ap, const airEnum *);
    va_end(ap);
  }
  if (airTypeOther == type) {
    va_start(ap, info);
    va_arg(ap, unsigned int *); /* skip sawP */
    va_arg(ap, airEnum *);      /* skip enm */
    CB = va_arg(ap, hestCB *);
    va_end(ap);
  }
  return hestOptAdd_nva(optP, flag, name, type, min, max, /* */
                        valueP, dflt, info,               /* */
                        sawP, enm, CB);
}

static void
_hestOptFree(hestOpt *opt) {

  opt->flag = (char *)airFree(opt->flag);
  opt->name = (char *)airFree(opt->name);
  opt->dflt = (char *)airFree(opt->dflt);
  opt->info = (char *)airFree(opt->info);
  opt->havec = hestArgVecNix(opt->havec);
  return;
}

hestOpt *
hestOptFree(hestOpt *opt) {
  if (!opt) return NULL;
  uint num = opt->arrLen;
  for (uint opi = 0; opi < num; opi++) {
    _hestOptFree(opt + opi);
  }
  free(opt);
  return NULL;
}

/* _hestOPCheck
New biff-based container for all logic that originated in _hestOptCheck (which is the
2025 rename of _hestPanic): the validation of the given hestOpt array `opt` itself (but
*not* anything about the command-line or its parsing), relative to the given (non-NULL)
hestParm `hparm`.

Pre-2025, hest did not depend on biff, and this instead took a 'char *err' that somehow
magically had to be allocated for the size of any possible error message generated here.
The 2025 re-write recognized that biff is the right way to accumulate error messages, but
the use of biff is internal to biff, and not (unusually for Teem) part of the the
expected use of biff's API. Thus, public functions hestOptCheck() and hestOptParmCheck(),
which are the expected way to access the functionality herein, take a `char **errP` arg
into which a message is sprintf'ed, after allocation.

The shift to using biff removed how this function used to fprintf(stderr) some messages
like "panic 0.5" which were totally uninformative.  Now, hestOptCheck() and
hestOptParmCheck(), which both call _hestOPCheck, will fprintf(stderr) the informative
biff message.

Prior to 2023 code revisit: this used to set the "kind" in all the opts, but now that is
more appropriately done at the time the option is added.
*/
int
_hestOPCheck(const hestOpt *opt, const hestParm *hparm) {
  if (!(opt && hparm)) {
    biffAddf(HEST, "%s%sgot NULL opt (%p) or hparm (%p)", _ME_, AIR_VOIDP(opt),
             AIR_VOIDP(hparm));
    return 1;
  }
  uint optNum = opt->arrLen;
  uint ufvarNum = 0; // number of unflagged variadic-parameter options
  for (uint opi = 0; opi < optNum; opi++) {
    if (!(AIR_IN_OP(airTypeUnknown, opt[opi].type, airTypeLast))) {
      biffAddf(HEST, "%s%sopt[%u].type (%d) not in valid range [%d,%d]", _ME_, opi,
               opt[opi].type, airTypeUnknown + 1, airTypeLast - 1);
      return 1;
    }
    // `kind` set by hestOptSingleSet
    if (-1 == opt[opi].kind) {
      biffAddf(HEST, "%s%sopt[%u]'s min (%d) and max (%d) incompatible", _ME_, opi,
               opt[opi].min, opt[opi].max);
      return 1;
    }
    if (!(opt[opi].valueP)) {
      biffAddf(HEST, "%s%sopt[%u]'s valueP is NULL!", _ME_, opi);
      return 1;
    }
    if (1 == opt[opi].kind) {
      if (!opt[opi].flag) {
        biffAddf(HEST, "%s%sstand-alone flag opt[%u] must have a flag", _ME_, opi);
        return 1;
      }
      if (opt[opi].dflt) {
        biffAddf(HEST, "%s%sstand-alone flag (opt[%u] %s) should not have a default",
                 _ME_, opi, opt[opi].flag);
        return 1;
      }
      if (opt[opi].name) {
        biffAddf(HEST, "%s%sstand-alone flag (opt[%u] %s) should not have a name", _ME_,
                 opi, opt[opi].flag);
        return 1;
      }
    } else { // ------ end of if (1 == opt[opi].kind)
      if (!opt[opi].name) {
        biffAddf(HEST, "%s%sopt[%u] isn't stand-alone flag: must have \"name\"", _ME_,
                 opi);
        return 1;
      }
    }
    if (opt[opi].flag) {
      const char *flag = opt[opi].flag;
      uint fslen = AIR_UINT(strlen(flag));
      if (fslen > AIR_STRLEN_SMALL / 2) {
        biffAddf(HEST, "%s%sstrlen(opt[%u].flag) %u is too big", _ME_, opi, fslen);
        return 1;
      }
      if (strchr(flag, '-')) {
        biffAddf(HEST, "%s%sopt[%u].flag \"%s\" contains '-', which will confuse things",
                 _ME_, opi, flag);
        return 1;
      }
      for (uint chi = 0; chi < fslen; chi++) {
        if (!isprint(flag[chi])) {
          biffAddf(HEST, "%s%sopt[%u].flag \"%s\" char %u '%c' non-printing", _ME_, opi,
                   flag, chi, flag[chi]);
          return 1;
        }
        if (strchr(AIR_WHITESPACE, flag[chi])) {
          biffAddf(HEST, "%s%sopt[%u].flag \"%s\" char %u '%c' is whitespace", _ME_, opi,
                   flag, chi, flag[chi]);
          return 1;
        }
      }
      char *tbuff = airStrdup(flag);
      assert(tbuff);
      // no mop, have to call free(tbuff) !
      char *sep;
      if ((sep = strchr(tbuff, MULTI_FLAG_SEP))) {
        *sep = '\0';
        if (!(strlen(tbuff) && strlen(sep + 1))) {
          biffAddf(HEST,
                   "%s%seither short (\"%s\") or long (\"%s\") flag"
                   " of opt[%u] is zero length",
                   _ME_, tbuff, sep + 1, opi);
          return (free(tbuff), 1);
        }
        if (hparm->respectDashDashHelp && !strcmp("help", sep + 1)) {
          biffAddf(HEST,
                   "%s%slong \"--%s\" flag of opt[%u] is same as \"--help\" "
                   "that requested hparm->respectDashDashHelp handles separately",
                   _ME_, sep + 1, opi);
          return (free(tbuff), 1);
        }
        if (strchr(sep + 1, MULTI_FLAG_SEP)) {
          biffAddf(HEST,
                   "%s%sopt[%u] flag string \"%s\" has more than one instance of "
                   "short/long separation character '%c'",
                   _ME_, opi, flag, MULTI_FLAG_SEP);
          return (free(tbuff), 1);
        }
      } else {
        if (!strlen(opt[opi].flag)) {
          biffAddf(HEST, "%s%sopt[%u].flag is zero length", _ME_, opi);
          return (free(tbuff), 1);
        }
      }
      free(tbuff);
      if (hparm->respectDashBraceComments && (strchr(flag, '{') || strchr(flag, '}'))) {
        biffAddf(HEST,
                 "%s%srequested hparm->respectDashBraceComments but opt[%u]'s flag "
                 "\"%s\" confusingly contains '{' or '}'",
                 _ME_, opi, flag);
        return (free(tbuff), 1);
      }
      if (4 == opt[opi].kind) {
        if (!opt[opi].dflt) {
          biffAddf(HEST,
                   "%s%sflagged single variadic parameter must "
                   "specify a default",
                   _ME_);
          return 1;
        }
        if (!strlen(opt[opi].dflt)) {
          biffAddf(HEST,
                   "%s%sflagged single variadic parameter default "
                   "must be non-zero length",
                   _ME_);
          return 1;
        }
      }
    } else { // ------ end of if (opt[opi].flag)
      // opt[opi] is unflagged
      if (!opt[opi].min) {
        // this rules out all unflagged kind 1 and kind 4
        // and prevents unflagged kind 5 w/ min=0
        biffAddf(HEST, "%s%sunflagged opt[%u] (name %s) must have min >= 1, not 0", _ME_,
                 opi, opt[opi].name ? opt[opi].name : "not set");
        return 1;
      }
    }
    if (4 == opt[opi].kind) { // single variadic parameter
      // immediately above have ruled out unflagged kind 4
      if (!opt[opi].dflt) {
        biffAddf(HEST,
                 "%s%sopt[%u] -%s is single variadic parameter, but "
                 "no default set",
                 _ME_, opi, opt[opi].flag);
        return 1;
      }
      /* pre 2025, these types were allowed kind 4, but the semantics are just so weird
         and thus hard to test + debug, that it no longer makes sense to support them */
      if (airTypeChar == opt[opi].type || airTypeString == opt[opi].type
          || airTypeEnum == opt[opi].type || airTypeOther == opt[opi].type) {
        biffAddf(HEST,
                 "%s%sopt[%u] -%s is single variadic parameter, but sorry, "
                 "type %s no longer supported",
                 _ME_, opi, opt[opi].flag, _hestTypeStr[opt[opi].type]);
        return 1;
      }
    }
    if (5 == opt[opi].kind && !(opt[opi].sawP)) {
      biffAddf(HEST,
               "%s%sopt[%u] has multiple variadic parameters (min=%u,max=%d), "
               "but sawP is NULL",
               _ME_, opi, opt[opi].min, opt[opi].max);
      return 1;
    }
    if (opt[opi].sawP && 5 != opt[opi].kind) {
      biffAddf(HEST,
               "%s%sopt[%u] has non-NULL sawP but is not a (kind=5) "
               "multiple variadic parm option (min=%u,max=%d)",
               _ME_, opi, opt[opi].min, opt[opi].max);
      return 1;
    }
    if (airTypeEnum == opt[opi].type && !(opt[opi].enm)) {
      biffAddf(HEST,
               "%s%sopt[%u] (%s) is type \"enum\", but no "
               "airEnum pointer given",
               _ME_, opi, opt[opi].flag ? opt[opi].flag : "unflagged");
      return 1;
    }
    if (opt[opi].enm && airTypeEnum != opt[opi].type) {
      biffAddf(HEST,
               "%s%sopt[%u] (%s) has non-NULL airEnum pointer, but is not airTypeEnum",
               _ME_, opi, opt[opi].flag ? opt[opi].flag : "unflagged");
      return 1;
    }
    if (airTypeOther == opt[opi].type) {
      if (!(opt[opi].CB)) {
        biffAddf(HEST,
                 "%s%sopt[%u] (%s) is type \"other\", but no "
                 "callbacks given",
                 _ME_, opi, opt[opi].flag ? opt[opi].flag : "unflagged");
        return 1;
      }
      if (!(opt[opi].CB->size > 0)) {
        biffAddf(HEST, "%s%sopt[%u]'s \"size\" (%u) invalid", _ME_, opi,
                 (uint)(opt[opi].CB->size));
        return 1;
      }
      if (!(opt[opi].CB->type)) {
        biffAddf(HEST, "%s%sopt[%u]'s \"type\" is NULL", _ME_, opi);
        return 1;
      }
      if (!(opt[opi].CB->parse)) {
        biffAddf(HEST, "%s%sopt[%u]'s \"parse\" callback NULL", _ME_, opi);
        return 1;
      }
      if (opt[opi].CB->destroy && (sizeof(void *) != opt[opi].CB->size)) {
        biffAddf(HEST,
                 "%s%sopt[%u] has a \"destroy\", but size %lu isn't "
                 "sizeof(void*)",
                 _ME_, opi, (unsigned long)(opt[opi].CB->size));
        return 1;
      }
    }
    if (opt[opi].CB && airTypeOther != opt[opi].type) {
      biffAddf(HEST, "%s%sopt[%u] (%s) has non-NULL callbacks, but is not airTypeOther",
               _ME_, opi, opt[opi].flag ? opt[opi].flag : "unflagged");
      return 1;
    }
    // kind 4 = single variadic parm;  kind 5 = multiple variadic parm
    ufvarNum += (opt[opi].kind > 3 && (!opt[opi].flag));
  }
  if (ufvarNum > 1) {
    biffAddf(HEST, "%s%scan have at most 1 unflagged min<max options, not %u", _ME_,
             ufvarNum);
    return 1;
  }
  return 0;
}

/* experiments in adding a nixer/free-er that exactly matches the airMopper type,
   as part of trying to avoid all "undefined behavior" */
void *
hestParmFree_vp(void *_hparm) {
  return AIR_VOIDP(hestParmFree((hestParm *)_hparm));
}
void *
hestOptFree_vp(void *_opt) {
  return AIR_VOIDP(hestOptFree((hestOpt *)_opt));
}

/*
 * hestOptCheck: check given hestOpt array `opt`, using the default hestParm.
 * Puts any errors into newly allocated (caller responsible to free) `*errP`.
 */
int
hestOptCheck(const hestOpt *opt, char **errP) {
  hestParm *hparm = hestParmNew();
  if (_hestOPCheck(opt, hparm)) {
    char *err = biffGetDone(HEST);
    if (errP) {
      /* they did give a pointer address; they'll free it */
      *errP = err;
    } else {
      /* they didn't give a pointer address; we dump to stderr */
      fprintf(stderr, "%s: problem with given hestOpt array:\n%s", __func__, err);
      free(err);
    }
    hestParmFree(hparm);
    return 1;
  }
  /* else, no problems */
  if (errP) *errP = NULL;
  hestParmFree(hparm);
  return 0;
}

/*
 * hestOptParmCheck: check given hestOpt array `opt` in combination with the given
 * hestParm `hparm`. Puts any errors into newly allocated (caller responsible to free)
 * `*errP`.
 * HEY copy-pasta
 */
int
hestOptParmCheck(const hestOpt *opt, const hestParm *hparm, char **errP) {
  if (_hestOPCheck(opt, hparm)) {
    char *err = biffGetDone(HEST);
    if (errP) {
      /* they did give a pointer address; they'll free it */
      *errP = err;
    } else {
      /* they didn't give a pointer address; we dump to stderr */
      fprintf(stderr, "%s: problem with given hestOpt array:\n%s", __func__, err);
      free(err);
    }
    return 1;
  }
  /* else, no problems */
  if (errP) *errP = NULL;
  return 0;
}
