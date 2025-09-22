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
#include <limits.h>
#include <assert.h>

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
  0   /* we don't know anything about type "other" */
};
/* clang-format on */

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

/* _hestMax(-1) == INT_MAX, otherwise _hestMax(m) == m */
int
_hestMax(int max) {

  if (-1 == max) {
    max = INT_MAX;
  }
  return max;
}

/* opt_kind determines the kind (1,2,3,4, or 5) of an opt,
  from being passed its min and max fields */
static int
opt_kind(unsigned int min, int _max) {
  int max;

  max = _hestMax(_max);
  if (!(AIR_INT(min) <= max)) {
    /* invalid */
    return -1;
  }

  if (0 == min && 0 == max) {
    /* flag */
    return 1;
  }

  if (1 == min && 1 == max) {
    /* single fixed parameter */
    return 2;
  }

  if (2 <= min && 2 <= max && AIR_INT(min) == max) {
    /* multiple fixed parameters */
    return 3;
  }

  if (0 == min && 1 == max) {
    /* single optional parameter */
    return 4;
  }

  /* else multiple variable parameters */
  return 5;
}

/* "private" wrapper around opt_kind, taking a hestOpt pointer */
int
_hestKind(const hestOpt *opt) {

  return opt_kind(opt->min, opt->max);
}

/* opt_init initializes all of a hestOpt, even arrAlloc and arrLen */
static void
opt_init(hestOpt *opt) {

  opt->flag = opt->name = NULL;
  opt->type = airTypeUnknown; /* == 0 */
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
optarr_new(hestOpt **optP) {
  unsigned int opi;
  hestOpt *ret = AIR_CALLOC(INCR, hestOpt);
  assert(ret);
  for (opi = 0; opi < INCR; opi++) {
    opt_init(ret + opi);
  }
  ret->arrAlloc = INCR;
  ret->arrLen = 0;
  *optP = ret;
  return;
}

/* line airArrayLenIncr(1): increments logical length by 1,
and returns index of newly-available element */
static unsigned int
optarr_incr(hestOpt **optP) {
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
      opt_init(nopt + opi);
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
  opt->kind = opt_kind(min, max);
  /* deal with (what used to be) var args */
  opt->sawP = (5 == opt->kind /* */
                 ? sawP
                 : NULL);
  opt->enm = (airTypeEnum == type /* */
                ? enm
                : NULL);
  opt->CB = (airTypeOther == type /* */
               ? CB
               : NULL);
  /* alloc set by hestParse */
  /* leave arrAlloc, arrLen untouched: managed by caller */
  /* yes, redundant with opt_init() */
  opt->source = hestSourceUnknown;
  opt->parmStr = NULL;
  opt->helpWanted = AIR_FALSE;
  return;
}

/*
hestOptAdd_nva: A new (as of 2023) non-var-args ("_nva") version of hestOptAdd;
The per-hestOpt logic (including setting opt->kind) has now been moved to
hestOptSingleSet. The venerable var-args hestOptAdd is now a wrapper around this.
and the 99 non-var-args hestOptAdd_* functions also all call this.

Like hestOptAdd has done since 2013: returns UINT_MAX in case of error.
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
    optarr_new(optP);
  }
  /* increment logical length of hestOpt array; return index of opt being set here */
  retIdx = optarr_incr(optP);
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
 * This returns the index of the option just added, to so the caller can remember it and
 * this speed up later checking the `hestOpt->source` to learn where how the option was
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
  if (5 == opt_kind(min, max)) {
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
  return;
}

hestOpt *
hestOptFree(hestOpt *opt) {
  int opi, num;

  if (!opt) return NULL;

  num = opt->arrLen;
  for (opi = 0; opi < num; opi++) {
    _hestOptFree(opt + opi);
  }
  free(opt);
  return NULL;
}

/*
 * _hestOPCheck
 *
 * new biff-based container for all logic that used to be in _hestOptCheck (which is
 * the 2025 rename of _hestPanic): the validation of the given hestOpt array `opt` itself
 * (but *not* anything about the command-line or its parsing), relative to the given
 * (non-NULL) hestParm `hparm`.
 *
 * Pre-2025, hest did not depend on biff, and this instead took a 'char *err' that
 * somehow magically had to be allocated for the size of any possible error message
 * generated here.  The 2025 re-write recognized that biff is the right way to accumulate
 * error messages, but the use of biff is internal to biff, but not (unusually for Teem)
 * part of the the expected use of biff's API. Thus, public functions hestOptCheck() and
 * hestOptParmCheck(), which are the expected way to access the functionality herein,
 * take a `char **errP` arg into which a message is sprintf'ed, after allocation.
 *
 * The shift to using biff removed how this function used to fprintf(stderr) some message
 * like "panic 0.5" which as completely uninformative.  Now, hestOptCheck() and
 * hestOptParmCheck() fprintf(stderr) the biff message.
 *
 * Prior to 2023 code revisit: this used to set the "kind" in all the opts, but now that
 * is more appropriately done at the time the option is added.
 */
int
_hestOPCheck(const hestOpt *opt, const hestParm *hparm) {
  if (!(opt && hparm)) {
    biffAddf(HEST, "%s: got NULL opt (%p) or hparm (%p)", __func__, AIR_VOIDP(opt),
             AIR_VOIDP(hparm));
    return 1;
  }
  uint optNum = opt->arrLen;
  uint varNum = 0; // number of variable-parameter options
  for (uint opi = 0; opi < optNum; opi++) {
    if (!(AIR_IN_OP(airTypeUnknown, opt[opi].type, airTypeLast))) {
      biffAddf(HEST, "%s: opt[%u].type (%d) not in valid range [%d,%d]", __func__, opi,
               opt[opi].type, airTypeUnknown + 1, airTypeLast - 1);
      return 1;
    }
    if (!(opt[opi].valueP)) {
      biffAddf(HEST, "%s: opt[%u]'s valueP is NULL!", __func__, opi);
      return 1;
    }
    // `kind` set by hestOptSingleSet
    if (-1 == opt[opi].kind) {
      biffAddf(HEST, "%s: opt[%u]'s min (%d) and max (%d) incompatible", __func__, opi,
               opt[opi].min, opt[opi].max);
      return 1;
    }
    if (5 == opt[opi].kind && !(opt[opi].sawP)) {
      biffAddf(HEST,
               "%s: have multiple variable parameters, "
               "but sawP is NULL",
               __func__);
      return 1;
    }
    if (airTypeEnum == opt[opi].type) {
      if (!(opt[opi].enm)) {
        biffAddf(HEST,
                 "%s: opt[%u] (%s) is type \"enum\", but no "
                 "airEnum pointer given",
                 __func__, opi, opt[opi].flag ? opt[opi].flag : "?");
        return 1;
      }
    }
    if (airTypeOther == opt[opi].type) {
      if (!(opt[opi].CB)) {
        biffAddf(HEST,
                 "%s: opt[%u] (%s) is type \"other\", but no "
                 "callbacks given",
                 __func__, opi, opt[opi].flag ? opt[opi].flag : "?");
        return 1;
      }
      if (!(opt[opi].CB->size > 0)) {
        biffAddf(HEST, "%s: opt[%u]'s \"size\" (%u) invalid", __func__, opi,
                 (uint)(opt[opi].CB->size));
        return 1;
      }
      if (!(opt[opi].CB->type)) {
        biffAddf(HEST, "%s: opt[%u]'s \"type\" is NULL", __func__, opi);
        return 1;
      }
      if (!(opt[opi].CB->parse)) {
        biffAddf(HEST, "%s: opt[%u]'s \"parse\" callback NULL", __func__, opi);
        return 1;
      }
      if (opt[opi].CB->destroy && (sizeof(void *) != opt[opi].CB->size)) {
        biffAddf(HEST,
                 "%sopt[%u] has a \"destroy\", but size %lu isn't "
                 "sizeof(void*)",
                 __func__, opi, (unsigned long)(opt[opi].CB->size));
        return 1;
      }
    }
    if (opt[opi].flag) {
      char *tbuff = airStrdup(opt[opi].flag);
      if (!tbuff) {
        biffAddf(HEST, "%s: could not strdup() opi[%u].flag", __func__, opi);
        return 1;
      }
      // no map, have to call free(tbuff) !
      char *sep;
      if ((sep = strchr(tbuff, MULTI_FLAG_SEP))) {
        *sep = '\0';
        if (!(strlen(tbuff) && strlen(sep + 1))) {
          biffAddf(HEST,
                   "%s: either short (\"%s\") or long (\"%s\") flag"
                   " of opt[%u] is zero length",
                   __func__, tbuff, sep + 1, opi);
          return (free(tbuff), 1);
        }
        if (hparm->respectDashDashHelp && !strcmp("help", sep + 1)) {
          biffAddf(HEST,
                   "%s: long \"--%s\" flag of opt[%u] is same as \"--help\" "
                   "that requested hparm->respectDashDashHelp handles separately",
                   __func__, sep + 1, opi);
          return (free(tbuff), 1);
        }
      } else {
        if (!strlen(opt[opi].flag)) {
          biffAddf(HEST, "%s: opt[%u].flag is zero length", __func__, opi);
          return (free(tbuff), 1);
        }
      }
      if (hparm->respectDashBraceComments
          && (strchr(opt[opi].flag, '{') || strchr(opt[opi].flag, '}'))) {
        biffAddf(HEST,
                 "%s: requested hparm->respectDashBraceComments but opt[%u]'s flag "
                 "\"%s\" confusingly contains '{' or '}'",
                 __func__, opi, opt[opi].flag);
        return (free(tbuff), 1);
      }
      if (4 == opt[opi].kind) {
        if (!opt[opi].dflt) {
          biffAddf(HEST,
                   "%s: flagged single variable parameter must "
                   "specify a default",
                   __func__);
          return (free(tbuff), 1);
        }
        if (!strlen(opt[opi].dflt)) {
          biffAddf(HEST,
                   "%s: flagged single variable parameter default "
                   "must be non-zero length",
                   __func__);
          return (free(tbuff), 1);
        }
      }
      /*
      sprintf(tbuff, "-%s", opt[op].flag);
      if (1 == sscanf(tbuff, "%f", &tmpF)) {
        if (err)
          sprintf(err, "%sopt[%u].flag (\"%s\") is numeric, bad news",
                  ME, op, opt[op].flag);
        return 1;
      }
      */
    }
    // ------ end of if (opt[opi].flag)
    if (1 == opt[opi].kind) {
      if (!opt[opi].flag) {
        biffAddf(HEST, "%s: opt[%u] flag must have a flag", __func__, opi);
        return 1;
      }
    } else {
      if (!opt[opi].name) {
        biffAddf(HEST, "%s: opt[%u] isn't a flag: must have \"name\"", __func__, opi);
        return 1;
      }
    }
    if (4 == opt[opi].kind && !opt[opi].dflt) {
      biffAddf(HEST,
               "%s: opt[%u] is single variable parameter, but "
               "no default set",
               __func__, opi);
      return 1;
    }
    varNum += ((int)opt[opi].min < _hestMax(opt[opi].max)
               && (NULL == opt[opi].flag)); /* HEY scrutinize casts */
  }
  if (varNum > 1) {
    biffAddf(HEST, "%s: can't have %u unflagged min<max options, only one", __func__,
             varNum);
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
