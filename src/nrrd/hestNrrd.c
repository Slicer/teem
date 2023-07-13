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

#include "nrrd.h"
#include "privateNrrd.h"

#include <unistd.h> /* for isatty() and STDIN_FILENO */

/* ---------------------------- Nrrd ----------------------------- */

/*
_nrrdHestNrrdParseOkTTY() and _nrrdHestNrrdParseNoTTY ()

Converts a filename into a nrrd for the sake of hest.
There is no HestMaybeNrrdParse because this already does that:
when we get an empty string, we give back a NULL pointer, and
that is just fine
*/

static int
parserBoth(void *ptr,
           const char *filename,
           char err[AIR_STRLEN_HUGE + 1],
           const char *me,
           int disallowTTY) {
  char *nerr;
  Nrrd **nrrdP;
  airArray *mop;

  if (!(ptr && filename)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  nrrdP = (Nrrd **)ptr;
  *nrrdP = NULL;
  if (airStrlen(filename)) {
    if (disallowTTY && !strcmp("-", filename) && isatty(STDIN_FILENO)) {
      sprintf(err, "%s: declining to try reading Nrrd from stdin as tty (terminal)", me);
      return 1;
    }
    mop = airMopNew();
    *nrrdP = nrrdNew();
    airMopAdd(mop, *nrrdP, (airMopper)nrrdNuke, airMopOnError);
    /* else no concerns about filename being "-" and stdin being a terminal */
    if (nrrdLoad(*nrrdP, filename, NULL)) {
      airMopAdd(mop, nerr = biffGetDone(NRRD), airFree, airMopOnError);
      airStrcpy(err, AIR_STRLEN_HUGE + 1, nerr);
      airMopError(mop);
      *nrrdP = NULL;
      return (strstr(err, "EOF") ? 2 : 1);
    }
    airMopOkay(mop);
  }
  /* else they gave us an empty string filename, so we give back no nrrd,
     and its not an error condition */
  return 0;
}

static int
parserOkTTY(void *ptr, const char *filename, char err[AIR_STRLEN_HUGE + 1]) {
  return parserBoth(ptr,
                    filename,
                    err,
                    "_nrrdHestNrrdParse", /* mimic old name */
                    AIR_FALSE /* not disallowing TTY */);
}

static int
parserNoTTY(void *ptr, const char *filename, char err[AIR_STRLEN_HUGE + 1]) {
  return parserBoth(ptr,
                    filename,
                    err,
                    "_nrrdHestNrrdParse(NoTTY)",
                    AIR_TRUE /* yes disallow TTY */);
}

static const hestCB _nrrdHestNrrd = {sizeof(Nrrd *), "nrrd", parserOkTTY,
                                     (airMopper)nrrdNuke};

const hestCB *const nrrdHestNrrd = &_nrrdHestNrrd;

static const hestCB _nrrdHestNrrdNoTTY = {sizeof(Nrrd *), "nrrd", parserNoTTY,
                                          (airMopper)nrrdNuke};

const hestCB *const nrrdHestNrrdNoTTY = &_nrrdHestNrrdNoTTY;

/* ------------------------ NrrdKernelSpec -------------------------- */

static int
_nrrdHestKernelSpecParse(void *ptr, const char *str, char err[AIR_STRLEN_HUGE + 1]) {
  NrrdKernelSpec **ksP;
  static const char me[] = "_nrrdHestKernelSpecParse";
  char *nerr;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  ksP = (NrrdKernelSpec **)ptr;
  *ksP = nrrdKernelSpecNew();
  if (nrrdKernelParse(&((*ksP)->kernel), (*ksP)->parm, str)) {
    nerr = biffGetDone(NRRD);
    airStrcpy(err, AIR_STRLEN_HUGE + 1, nerr);
    free(nerr);
    return 1;
  }
  return 0;
}

static const hestCB _nrrdHestKernelSpec = {sizeof(NrrdKernelSpec *),
                                           "kernel specification",
                                           _nrrdHestKernelSpecParse,
                                           (airMopper)nrrdKernelSpecNix};

const hestCB *const nrrdHestKernelSpec = &_nrrdHestKernelSpec;

/* ------------------------ NrrdBoundarySpec -------------------------- */

static int
_nrrdHestBoundarySpecParse(void *ptr, const char *str, char err[AIR_STRLEN_HUGE + 1]) {
  NrrdBoundarySpec **bsp;
  static const char me[] = "_nrrdHestBoundarySpecParse";
  char *nerr;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  bsp = (NrrdBoundarySpec **)ptr;
  *bsp = nrrdBoundarySpecNew();
  if (nrrdBoundarySpecParse(*bsp, str)) {
    nerr = biffGetDone(NRRD);
    airStrcpy(err, AIR_STRLEN_HUGE + 1, nerr);
    /* HEY: why not freeing bsp? */
    free(nerr);
    return 1;
  }
  return 0;
}

static const hestCB _nrrdHestBoundarySpec = {sizeof(NrrdBoundarySpec *),
                                             "boundary specification",
                                             _nrrdHestBoundarySpecParse,
                                             (airMopper)nrrdBoundarySpecNix};

const hestCB *const nrrdHestBoundarySpec = &_nrrdHestBoundarySpec;

/* --------------------------- NrrdIter ----------------------------- */

static int
_nrrdLooksLikeANumber(const char *str) {
  /* 0: -+                (no restriction, but that's a little daft)
     1: 0123456789        n > 0
     2: .                 0 <= n <= 1
     3: eE                0 <= n <= 1
     4: everything else   0 == n
  */
  int count[5];

  count[0] = count[1] = count[2] = count[3] = count[4] = 0;
  while (*str) {
    int lwc, cc = *str;
    lwc = tolower(cc);
    switch (lwc) {
    case '-':
    case '+':
      count[0]++;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      count[1]++;
      break;
    case '.':
      count[2]++;
      break;
    case 'e':
      count[3]++;
      break;
    default:
      count[4]++;
      break;
    }
    str++;
  }
  if (count[1] > 0 && AIR_IN_CL(0, count[2], 1) && AIR_IN_CL(0, count[3], 1)
      && count[4] == 0) {
    return AIR_TRUE;
  } else {
    return AIR_FALSE;
  }
}

static int
_nrrdHestIterParse(void *ptr, const char *str, char err[AIR_STRLEN_HUGE + 1]) {
  static const char me[] = "_nrrdHestIterParse";
  char *nerr;
  Nrrd *nrrd;
  NrrdIter **iterP;
  airArray *mop;
  double val;
  int ret;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  iterP = (NrrdIter **)ptr;
  mop = airMopNew();
  *iterP = nrrdIterNew();
  airMopAdd(mop, *iterP, (airMopper)nrrdIterNix, airMopOnError);

  /* the challenge here is determining if a given string represents a
     filename or a number.  Obviously there are cases where it could
     be both, so we'll assume its a filename first.  Because: there
     are different ways of writing the same number, such as "3" -->
     "+3", "3.1" --> "3.10", so someone accidently using the file when
     they mean to use the number has easy ways of changing the number
     representation, since these trivial transformations will probably
     not all result in valid filenames. Another problem is that one
     really wants a general robust test to see if a given string is a
     valid number representation AND NOTHING BUT THAT, and sscanf() is
     not that test.  In any case, if there are to be improved smarts
     about this matter, they need to be implemented below and nowhere
     else. */

  nrrd = nrrdNew();
  ret = nrrdLoad(nrrd, str, NULL);
  if (!ret) {
    /* first attempt at nrrdLoad() was SUCCESSFUL */
    nrrdIterSetOwnNrrd(*iterP, nrrd);
  } else {
    /* so it didn't load as a nrrd- if its because fopen() failed,
       then we'll try it as a number.  If its for another reason,
       then we complain */
    nrrdNuke(nrrd);
    if (2 != ret) {
      /* it failed because of something besides the fopen(), so complain */
      nerr = biffGetDone(NRRD);
      airStrcpy(err, AIR_STRLEN_HUGE + 1, nerr);
      airMopError(mop);
      return 1;
    } else {
      /* fopen() failed, so it probably wasn't meant to be a filename */
      free(biffGetDone(NRRD));
      ret = airSingleSscanf(str, "%lf", &val);
      if (_nrrdLooksLikeANumber(str)
          || (1 == ret
              && (!AIR_EXISTS(val) || AIR_ABS(AIR_PI - val) < 0.0001
                  || AIR_ABS(-AIR_PI - val) < 0.0001))) {
        /* either it patently looks like a number, or,
           it already parsed as a number and it is a special value */
        if (1 == ret) {
          nrrdIterSetValue(*iterP, val);
        } else {
          /* oh, this is bad. */
          fprintf(stderr, "%s: PANIC, is it a number or not?", me);
          exit(1);
        }
      } else {
        /* it doesn't look like a number, but the fopen failed, so
           we'll let it fail again and pass back the error messages */
        if (nrrdLoad(nrrd = nrrdNew(), str, NULL)) {
          nerr = biffGetDone(NRRD);
          airStrcpy(err, AIR_STRLEN_HUGE + 1, nerr);
          airMopError(mop);
          return 1;
        } else {
          /* what the hell? */
          fprintf(stderr, "%s: PANIC, is it a nrrd or not?", me);
          exit(1);
        }
      }
    }
  }
  airMopAdd(mop, iterP, (airMopper)airSetNull, airMopOnError);
  airMopOkay(mop);
  return 0;
}

static const hestCB _nrrdHestIter = {sizeof(NrrdIter *), "nrrd/value",
                                     _nrrdHestIterParse, (airMopper)nrrdIterNix};

const hestCB *const nrrdHestIter = &_nrrdHestIter;
