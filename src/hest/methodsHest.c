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

#include "hest.h"
#include "privateHest.h"
#include <limits.h>
#include <assert.h>

#include <sys/ioctl.h> /* for ioctl(), TIOCGWINSZ, struct winsize */
#include <unistd.h>    /* for STDOUT_FILENO and friends */

const int hestPresent = 42;

/* INCR is like airArray->incr: granularity with which we (linearly) reallocate the
hestOpt array. Very few uses of hest within Teem use more than 32 options. Hopefully
this avoids all the reallocations in the past action of hestOptAdd and the like. */
#define INCR 32

hestParm *
hestParmNew() {
  hestParm *parm;

  parm = AIR_CALLOC(1, hestParm);
  assert(parm);
  parm->verbosity = hestDefaultVerbosity;
  parm->respFileEnable = hestDefaultRespFileEnable;
  parm->elideSingleEnumType = hestDefaultElideSingleEnumType;
  parm->elideSingleOtherType = hestDefaultElideSingleOtherType;
  parm->elideSingleOtherDefault = hestDefaultElideSingleOtherDefault;
  parm->greedySingleString = hestDefaultGreedySingleString;
  parm->elideSingleNonExistFloatDefault = hestDefaultElideSingleNonExistFloatDefault;
  parm->elideMultipleNonExistFloatDefault = hestDefaultElideMultipleNonExistFloatDefault;
  parm->elideSingleEmptyStringDefault = hestDefaultElideSingleEmptyStringDefault;
  parm->elideMultipleEmptyStringDefault = hestDefaultElideMultipleEmptyStringDefault;
  parm->cleverPluralizeOtherY = hestDefaultCleverPluralizeOtherY;
  parm->columns = hestDefaultColumns;
  parm->respFileFlag = hestDefaultRespFileFlag;
  parm->respFileComment = hestDefaultRespFileComment;
  parm->varParamStopFlag = hestDefaultVarParamStopFlag;
  parm->multiFlagSep = hestDefaultMultiFlagSep;
  /* for these most recent addition to the hestParm,
     abstaining from added yet another default global variable */
  parm->dieLessVerbose = AIR_FALSE;
  parm->noBlankLineBeforeUsage = AIR_FALSE;
  /* It would be really nice for parm->respectDashDashHelp to default to true:
  widespread conventions say what "--help" should mean e.g. https://clig.dev/#help
  HOWEVER, the problem is with how hestParse is called and how the return
  is interpreted as a boolean:
  - zero has meant that hestParse could set values for all the options (either
    from the command-line or from supplied defaults), and
  - non-zero has meant that there was an error parsing the command-line arguments
  But seeing and recognizing "--help" means that options have NOT had values
  set, and, that's not an error, which is outside that binary.  But that binary
  is the precedent, so we have to work with it by default.
  Now, with parm->respectDashDashHelp, upon seeing "--help", hestParse returns 0,
  and sets helpWanted in the first hestOpt, and the caller will have to know
  to check for that.  This logic is handled by hestParseOrDie, but maybe in
  the future there can be a different top-level parser function that turns on
  parm->respectDashDashHelp and knows how to check the results */
  parm->respectDashDashHelp = AIR_FALSE;
  return parm;
}

hestParm *
hestParmFree(hestParm *parm) {

  airFree(parm);
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
unsigned int
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

/* experiments in adding a nixer/free-er that exactly matches the airMopper type,
   as part of trying to avoid all "undefined behavior" */
void *
hestParmFree_vp(void *_parm) {
  return AIR_VOIDP(hestParmFree((hestParm *)_parm));
}
void *
hestOptFree_vp(void *_opt) {
  return AIR_VOIDP(hestOptFree((hestOpt *)_opt));
}

int
hestOptCheck(hestOpt *opt, char **errP) {
  static const char me[] = "hestOptCheck";
  char *err;
  hestParm *parm;
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
  parm = hestParmNew();
  if (_hestPanic(opt, err, parm)) {
    /* problems */
    if (errP) {
      /* they did give a pointer address; they'll free it */
      *errP = err;
    } else {
      /* they didn't give a pointer address; their loss */
      free(err);
    }
    hestParmFree(parm);
    return 1;
  }
  /* else, no problems */
  if (errP) *errP = NULL;
  free(err);
  hestParmFree(parm);
  return 0;
}
