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

const int hestPresent = 42;

hestParm *
hestParmNew() {
  hestParm *parm;

  parm = AIR_CALLOC(1, hestParm);
  if (parm) {
    parm->verbosity = hestDefaultVerbosity;
    parm->respFileEnable = hestDefaultRespFileEnable;
    parm->elideSingleEnumType = hestDefaultElideSingleEnumType;
    parm->elideSingleOtherType = hestDefaultElideSingleOtherType;
    parm->elideSingleOtherDefault = hestDefaultElideSingleOtherDefault;
    parm->greedySingleString = hestDefaultGreedySingleString;
    parm->elideSingleNonExistFloatDefault = hestDefaultElideSingleNonExistFloatDefault;
    parm->elideMultipleNonExistFloatDefault
      = hestDefaultElideMultipleNonExistFloatDefault;
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
  }
  return parm;
}

hestParm *
hestParmFree(hestParm *parm) {

  airFree(parm);
  return NULL;
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
opt_kind(unsigned int _min, int _max) {
  int min, max;

  min = AIR_CAST(int, _min);
  if (min < 0) {
    /* invalid */
    return -1;
  }

  max = _hestMax(_max);
  if (!(min <= max)) {
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

  if (2 <= min && 2 <= max && min == max) {
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

static void
opt_init(hestOpt *opt) {

  opt->flag = opt->name = NULL;
  opt->type = 0;
  opt->min = 0;
  opt->max = 0;
  opt->valueP = NULL;
  opt->dflt = opt->info = NULL;
  opt->sawP = NULL;
  opt->enm = NULL;
  opt->CB = NULL;
  opt->sawP = NULL;
  opt->kind = opt->alloc = 0;
  opt->source = hestSourceUnknown;
  opt->parmStr = NULL;
  opt->helpWanted = AIR_FALSE;
}

/*
hestOptAdd_nva: A new (as of 2023) non-var-args ("_nva") version of hestOptAdd, which now
contains its main functionality (and it has become just a wrapper around this).

Like hestOptAdd has done since 2013: returns UINT_MAX in case of error.
*/
unsigned int
hestOptAdd_nva(hestOpt **optP, const char *flag, const char *name, int type, int min,
               int max, void *valueP, const char *dflt, const char *info,
               unsigned int *sawP, const airEnum *enm, const hestCB *CB) {
  hestOpt *ret = NULL; /* not the function return; but what *optP is set to */
  int num;
  unsigned int retIdx;

  if (!optP) return UINT_MAX;

  num = *optP ? hestOptNum(*optP) : 0;
  if (!(ret = AIR_CALLOC(num + 2, hestOpt))) {
    return UINT_MAX;
  }
  if (num) memcpy(ret, *optP, num * sizeof(hestOpt));
  retIdx = AIR_UINT(num);
  ret[num].flag = airStrdup(flag);
  ret[num].name = airStrdup(name);
  ret[num].type = type;
  ret[num].min = min;
  ret[num].max = max;
  ret[num].valueP = valueP;
  ret[num].dflt = airStrdup(dflt);
  ret[num].info = airStrdup(info);
  /* initialize the things that may be set below */
  ret[num].sawP = NULL;
  ret[num].enm = NULL;
  ret[num].CB = NULL;
  /* yes, redundant with opt_init() */
  ret[num].source = hestSourceUnknown;
  ret[num].parmStr = NULL;
  ret[num].helpWanted = AIR_FALSE;
  /* deal with (what used to be) var args */
  if (5 == opt_kind(min, max)) {
    ret[num].sawP = sawP;
  }
  if (airTypeEnum == type) {
    ret[num].enm = enm;
  }
  if (airTypeOther == type) {
    ret[num].CB = CB;
  }
  opt_init(&(ret[num + 1]));
  ret[num + 1].min = 1;
  if (*optP) free(*optP);
  *optP = ret;
  return retIdx;
}

/*
** as of Sept 2013 this returns information: the index of the
** option just added.  Returns UINT_MAX in case of error.
*/
unsigned int
hestOptAdd(hestOpt **optP, const char *flag, const char *name, int type, int min,
           int max, void *valueP, const char *dflt, const char *info, ...) {
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
  int op, num;

  if (!opt) return NULL;

  num = hestOptNum(opt);
  if (opt[num].min) {
    /* we only try to free this array if it looks like something we allocated;
       this is leveraging how opt_init leaves things */
    for (op = 0; op < num; op++) {
      _hestOptFree(opt + op);
    }
    free(opt);
  }
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

/*
hestOptNum: returns the number of elements in the given hestOpt array, *assuming* it is
set up like hestOptAdd does it.

Unfortunately, unlike argv itself, there is no sense in which the hestOpt array can be
NULL-terminated, mainly because "opt" is an array of hestOpt structs, not an array of
pointers to hestOpt structs.
*/
unsigned int
hestOptNum(const hestOpt *opt) {
  unsigned int num = 0;

  while (opt[num].flag || opt[num].name || opt[num].type) {
    num++;
  }
  return num;
}
