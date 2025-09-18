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

// enjoying how C99 greatly simplifies creating an airEnum at compile-time
static const airEnum _hestSource
  = {.name = "source",
     .M = 3,
     .str = (const char *[]){"(unknown_source)", // 0
                             "default",          // 1
                             "command-line",     // 2
                             "response-file"},   // 3
     .val = NULL,
     .desc = (const char *[]){"unknown source",            //
                              "default string in hestOpt", //
                              "argc/argv command-line",    //
                              "a response file"},
     .strEqv = (const char *[]){"default",                   //
                                "command-line", "cmdline",   //
                                "response-file", "respfile", //
                                ""},
     .valEqv = (const int[]){hestSourceDefault,                            //
                             hestSourceCommandLine, hestSourceCommandLine, //
                             hestSourceResponseFile, hestSourceResponseFile},
     .sense = AIR_FALSE};
const airEnum *const hestSource = &_hestSource;

// see documentation in parseHest.c
#define ME ((hparm && hparm->verbosity) ? me : "")

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
  hparm->respFileEnable = hestDefaultRespFileEnable;
  hparm->elideSingleEnumType = hestDefaultElideSingleEnumType;
  hparm->elideSingleOtherType = hestDefaultElideSingleOtherType;
  hparm->elideSingleOtherDefault = hestDefaultElideSingleOtherDefault;
  hparm->greedySingleString = hestDefaultGreedySingleString;
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
  hparm->greedySingleString = hestDefaultGreedySingleString;
  hparm->cleverPluralizeOtherY = hestDefaultCleverPluralizeOtherY;
  /* here too: newer addition to hestParm avoid adding another default global */
  hparm->dieLessVerbose = AIR_FALSE;
  hparm->noBlankLineBeforeUsage = AIR_FALSE;
  hparm->columns = hestDefaultColumns;
  hparm->respFileFlag = hestDefaultRespFileFlag;
  hparm->respFileComment = hestDefaultRespFileComment;
  hparm->varParamStopFlag = hestDefaultVarParamStopFlag;
  hparm->multiFlagSep = hestDefaultMultiFlagSep;
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
Note that this makes no attempt at error-checking; that is all in hestOptCheck
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
now hestOptAdd is a wrapper around this. And, the per-hestOpt logic has now
been moved to hestOptSingleSet.

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
** as of Sept 2013 this returns information: the index of the
** option just added.  Returns UINT_MAX in case of error.
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
_hestOptCheck()   (formerly _hestPanic, in parseHest.c)

This performs the validation of the given hestOpt array itself (not the command line to
be parsed), with descriptive error messages sprintf'ed into err, if given. hestOptCheck()
is the expected way for users to access this.

Prior to 2023 code revisit; this used to set the "kind" in all the opts but now that is
more appropriately done at the time the option is added (by hestOptAdd, hestOptAdd_nva,
hestOptSingleSet, or hestOptAdd_*_*)
*/
int
_hestOptCheck(const hestOpt *opt, char *err, const hestParm *hparm) {
  /* see note on ME (at top) for why me[] ends with ": " */
  static const char me[] = "_hestOptCheck: ";
  char tbuff[AIR_STRLEN_HUGE + 1], *sep;
  int numvar, opi, optNum;

  optNum = hestOptNum(opt);
  numvar = 0;
  for (opi = 0; opi < optNum; opi++) {
    if (!(AIR_IN_OP(airTypeUnknown, opt[opi].type, airTypeLast))) {
      if (err)
        sprintf(err, "%sopt[%d].type (%d) not in valid range [%d,%d]", ME, opi,
                opt[opi].type, airTypeUnknown + 1, airTypeLast - 1);
      else
        fprintf(stderr, "%s: panic 0\n", me);
      return 1;
    }
    if (!(opt[opi].valueP)) {
      if (err)
        sprintf(err, "%sopt[%d]'s valueP is NULL!", ME, opi);
      else
        fprintf(stderr, "%s: panic 0.5\n", me);
      return 1;
    }
    if (-1 == opt[opi].kind) {
      if (err)
        sprintf(err, "%sopt[%d]'s min (%d) and max (%d) incompatible", ME, opi,
                opt[opi].min, opt[opi].max);
      else
        fprintf(stderr, "%s: panic 1\n", me);
      return 1;
    }
    if (5 == opt[opi].kind && !(opt[opi].sawP)) {
      if (err)
        sprintf(err,
                "%shave multiple variable parameters, "
                "but sawP is NULL",
                ME);
      else
        fprintf(stderr, "%s: panic 2\n", me);
      return 1;
    }
    if (airTypeEnum == opt[opi].type) {
      if (!(opt[opi].enm)) {
        if (err) {
          sprintf(err,
                  "%sopt[%d] (%s) is type \"enum\", but no "
                  "airEnum pointer given",
                  ME, opi, opt[opi].flag ? opt[opi].flag : "?");
        } else {
          fprintf(stderr, "%s: panic 3\n", me);
        }
        return 1;
      }
    }
    if (airTypeOther == opt[opi].type) {
      if (!(opt[opi].CB)) {
        if (err) {
          sprintf(err,
                  "%sopt[%d] (%s) is type \"other\", but no "
                  "callbacks given",
                  ME, opi, opt[opi].flag ? opt[opi].flag : "?");
        } else {
          fprintf(stderr, "%s: panic 4\n", me);
        }
        return 1;
      }
      if (!(opt[opi].CB->size > 0)) {
        if (err)
          sprintf(err, "%sopt[%d]'s \"size\" (%d) invalid", ME, opi,
                  (int)(opt[opi].CB->size));
        else
          fprintf(stderr, "%s: panic 5\n", me);
        return 1;
      }
      if (!(opt[opi].CB->type)) {
        if (err)
          sprintf(err, "%sopt[%d]'s \"type\" is NULL", ME, opi);
        else
          fprintf(stderr, "%s: panic 6\n", me);
        return 1;
      }
      if (!(opt[opi].CB->parse)) {
        if (err)
          sprintf(err, "%sopt[%d]'s \"parse\" callback NULL", ME, opi);
        else
          fprintf(stderr, "%s: panic 7\n", me);
        return 1;
      }
      if (opt[opi].CB->destroy && (sizeof(void *) != opt[opi].CB->size)) {
        if (err)
          sprintf(err,
                  "%sopt[%d] has a \"destroy\", but size %lu isn't "
                  "sizeof(void*)",
                  ME, opi, (unsigned long)(opt[opi].CB->size));
        else
          fprintf(stderr, "%s: panic 8\n", me);
        return 1;
      }
    }
    if (opt[opi].flag) {
      strcpy(tbuff, opt[opi].flag);
      if ((sep = strchr(tbuff, hparm->multiFlagSep))) {
        *sep = '\0';
        if (!(strlen(tbuff) && strlen(sep + 1))) {
          if (err)
            sprintf(err,
                    "%seither short (\"%s\") or long (\"%s\") flag"
                    " of opt[%d] is zero length",
                    ME, tbuff, sep + 1, opi);
          else
            fprintf(stderr, "%s: panic 9\n", me);
          return 1;
        }
        if (hparm->respectDashDashHelp && !strcmp("help", sep + 1)) {
          if (err)
            sprintf(err,
                    "%slong \"--%s\" flag of opt[%d] is same as \"--help\" "
                    "that requested hparm->respectDashDashHelp handles separately",
                    ME, sep + 1, opi);
          else
            fprintf(stderr, "%s: panic 9.5\n", me);
          return 1;
        }
      } else {
        if (!strlen(opt[opi].flag)) {
          if (err)
            sprintf(err, "%sopt[%d].flag is zero length", ME, opi);
          else
            fprintf(stderr, "%s: panic 10\n", me);
          return 1;
        }
      }
      if (hparm->respectDashBraceComments
          && (strchr(opt[opi].flag, '{') || strchr(opt[opi].flag, '}'))) {
        if (err)
          sprintf(err,
                  "%srequested hparm->respectDashBraceComments but opt[%d]'s flag "
                  "\"%s\" confusingly contains '{' or '}'",
                  ME, opi, opt[opi].flag);
        else
          fprintf(stderr, "%s: panic 10.5\n", me);
        return 1;
      }
      if (4 == opt[opi].kind) {
        if (!opt[opi].dflt) {
          if (err)
            sprintf(err,
                    "%sflagged single variable parameter must "
                    "specify a default",
                    ME);
          else
            fprintf(stderr, "%s: panic 11\n", me);
          return 1;
        }
        if (!strlen(opt[opi].dflt)) {
          if (err)
            sprintf(err,
                    "%sflagged single variable parameter default "
                    "must be non-zero length",
                    ME);
          else
            fprintf(stderr, "%s: panic 12\n", me);
          return 1;
        }
      }
      /*
      sprintf(tbuff, "-%s", opt[op].flag);
      if (1 == sscanf(tbuff, "%f", &tmpF)) {
        if (err)
          sprintf(err, "%sopt[%d].flag (\"%s\") is numeric, bad news",
                  ME, op, opt[op].flag);
        return 1;
      }
      */
    }
    if (1 == opt[opi].kind) {
      if (!opt[opi].flag) {
        if (err)
          sprintf(err, "%sflags must have flags", ME);
        else
          fprintf(stderr, "%s: panic 13\n", me);
        return 1;
      }
    } else {
      if (!opt[opi].name) {
        if (err)
          sprintf(err, "%sopt[%d] isn't a flag: must have \"name\"", ME, opi);
        else
          fprintf(stderr, "%s: panic 14\n", me);
        return 1;
      }
    }
    if (4 == opt[opi].kind && !opt[opi].dflt) {
      if (err)
        sprintf(err,
                "%sopt[%d] is single variable parameter, but "
                "no default set",
                ME, opi);
      else
        fprintf(stderr, "%s: panic 15\n", me);
      return 1;
    }
    numvar += ((int)opt[opi].min < _hestMax(opt[opi].max)
               && (NULL == opt[opi].flag)); /* HEY scrutinize casts */
  }
  if (numvar > 1) {
    if (err)
      sprintf(err, "%scan't have %d unflagged min<max opts, only one", ME, numvar);
    else
      fprintf(stderr, "%s: panic 16\n", me);
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

int
hestOptCheck(hestOpt *opt, char **errP) {
  static const char me[] = "hestOptCheck";
  char *err;
  hestParm *hparm;
  int big;

  big = _hestErrStrlen(opt, 0, NULL);
  if (!(err = AIR_CALLOC(big, char))) {
    fprintf(stderr,
            "%s PANIC: couldn't allocate error message "
            "buffer (size %d)\n",
            me, big);
    if (errP) *errP = NULL;
    return 1;
  }
  hparm = hestParmNew();
  if (_hestOptCheck(opt, err, hparm)) {
    /* problems */
    if (errP) {
      /* they did give a pointer address; they'll free it */
      *errP = err;
    } else {
      /* they didn't give a pointer address; their loss */
      free(err);
    }
    hestParmFree(hparm);
    return 1;
  }
  /* else, no problems */
  if (errP) *errP = NULL;
  free(err);
  hestParmFree(hparm);
  return 0;
}
