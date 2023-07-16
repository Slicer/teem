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

#include "bane.h"
#include "privateBane.h"

/* ----------------------------------------------------------- */
/* ----------------------------------------------------------- */
/* ----------------------------------------------------------- */

/*
** baneGkmsParseIncStrategy
**
** inc[0]: member of baneInc* enum
** inc[1], inc[2] ... : incParm[0], incParm[1] ...
*/
static int
baneGkmsParseIncStrategy(void *ptr, const char *str, char err[AIR_STRLEN_HUGE + 1]) {
  static const char me[] = "baneGkmsParseIncStrategy";
  double *inc, *incParm;
  int i, bins;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  inc = (double *)ptr;
  incParm = inc + 1;
  for (i = 0; i < BANE_PARM_NUM; i++) {
    incParm[i] = AIR_NAN;
  }
  if (1 == sscanf(str, "f:%lg", incParm + 0)
      || 2 == sscanf(str, "f:%lg,%lg", incParm + 0, incParm + 1)) {
    inc[0] = baneIncRangeRatio;
    return 0;
  }
  if (1 == sscanf(str, "p:%lg", incParm + 1)
      || 2 == sscanf(str, "p:%lg,%lg", incParm + 1, incParm + 2)) {
    inc[0] = baneIncPercentile;
    incParm[0] = baneDefPercHistBins;
    return 0;
  }
  if (3 == sscanf(str, "p:%d,%lg,%lg", &bins, incParm + 1, incParm + 2)) {
    inc[0] = baneIncPercentile;
    incParm[0] = bins;
    return 0;
  }
  if (2 == sscanf(str, "a:%lg,%lg", incParm + 0, incParm + 1)) {
    inc[0] = baneIncAbsolute;
    return 0;
  }
  if (1 == sscanf(str, "s:%lg", incParm + 0)
      || 2 == sscanf(str, "s:%lg,%lg", incParm + 1, incParm + 2)) {
    inc[0] = baneIncStdv;
    return 0;
  }

  /* else its no good */

  sprintf(err, "%s: \"%s\" not recognized", me, str);
  return 1;
}

static const hestCB _baneGkmsHestIncStrategy = {(1 + BANE_PARM_NUM) * sizeof(double),
                                                "inclusion strategy",
                                                baneGkmsParseIncStrategy, NULL};

const hestCB *const baneGkmsHestIncStrategy = &_baneGkmsHestIncStrategy;

/* ----------------------------------------------------------- */
/* ----------------------------------------------------------- */
/* ----------------------------------------------------------- */

static int
baneGkmsParseBEF(void *ptr, const char *str, char err[AIR_STRLEN_HUGE + 1]) {
  static const char me[] = "baneGkmsParseBEF";
  char mesg[AIR_STRLEN_MED + 1], *nerr;
  float cent, width, shape, alpha, off, *bef;
  Nrrd **nrrdP;
  airArray *mop;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  mop = airMopNew();
  nrrdP = (Nrrd **)ptr;
  airMopAdd(mop, *nrrdP = nrrdNew(), (airMopper)nrrdNuke, airMopOnError);
  if (4 == sscanf(str, "%g,%g,%g,%g", &shape, &width, &cent, &alpha)) {
    /* its a valid BEF specification, we make the nrrd ourselves */
    if (nrrdMaybeAlloc_va(*nrrdP, nrrdTypeFloat, 2, AIR_SIZE_T(2), AIR_SIZE_T(6))) {
      airMopAdd(mop, nerr = biffGetDone(NRRD), airFree, airMopOnError);
      airStrcpy(err, AIR_STRLEN_HUGE + 1, nerr);
      airMopError(mop);
      return 1;
    }
    bef = (float *)((*nrrdP)->data);
    off = AIR_FLOAT(AIR_AFFINE(0.0, shape, 1.0, 0.0, width / 2));
    /* positions */
    bef[0 + 2 * 0] = cent - 2 * width;
    bef[0 + 2 * 1] = cent - width / 2 - off;
    bef[0 + 2 * 2] = cent - width / 2 + off;
    bef[0 + 2 * 3] = cent + width / 2 - off;
    bef[0 + 2 * 4] = cent + width / 2 + off;
    bef[0 + 2 * 5] = cent + 2 * width;
    if (bef[0 + 2 * 1] == bef[0 + 2 * 2]) bef[0 + 2 * 1] -= 0.001f;
    if (bef[0 + 2 * 2] == bef[0 + 2 * 3]) bef[0 + 2 * 2] -= 0.001f;
    if (bef[0 + 2 * 3] == bef[0 + 2 * 4]) bef[0 + 2 * 4] += 0.001f;
    /* opacities */
    bef[1 + 2 * 0] = 0.0;
    bef[1 + 2 * 1] = 0.0;
    bef[1 + 2 * 2] = alpha;
    bef[1 + 2 * 3] = alpha;
    bef[1 + 2 * 4] = 0.0;
    bef[1 + 2 * 5] = 0.0;
    /* to tell gkms opac that this came from four floats */
    (*nrrdP)->ptr = *nrrdP;
  } else {
    if (nrrdLoad(*nrrdP, str, NULL)) {
      airMopAdd(mop, nerr = biffGetDone(NRRD), airFree, airMopOnError);
      sprintf(mesg,
              "%s: couldn't parse \"%s\" as four-parameter BEF or "
              "as a nrrd filename\n",
              me, str);
      strcpy(err, mesg);
      strncat(err, nerr, AIR_STRLEN_HUGE - strlen(mesg) - 1);
      airMopError(mop);
      return 1;
    }
    (*nrrdP)->ptr = NULL;
  }

  airMopOkay(mop);
  return 0;
}

static const hestCB _baneGkmsHestBEF = {sizeof(Nrrd *), "boundary emphasis function",
                                        baneGkmsParseBEF, (airMopper)nrrdNuke};

const hestCB *const baneGkmsHestBEF = &_baneGkmsHestBEF;

/* ----------------------------------------------------------- */
/* ----------------------------------------------------------- */
/* ----------------------------------------------------------- */

/*
** gthr[0] = 1: some scaling of max grad "x<float>"
**           0: absolute                 "<float>"
** gthr[1] = the scaling, or the absolute
*/
static int
baneGkmsParseGthresh(void *ptr, const char *str, char err[AIR_STRLEN_HUGE + 1]) {
  static const char me[] = "baneGkmsParseGthresh";
  float *gthr;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  gthr = (float *)ptr;

  if ('x' == str[0]) {
    if (1 != sscanf(str + 1, "%f", gthr + 1)) {
      sprintf(err, "%s: can't parse \"%s\" as x<float>", me, str);
      return 1;
    }
    gthr[0] = 1;
  } else {
    if (1 != sscanf(str, "%f", gthr + 1)) {
      sprintf(err, "%s: can't parse \"%s\" as float", me, str);
      return 1;
    }
    gthr[0] = 0;
  }
  return 0;
}

static const hestCB _baneGkmsHestGthresh = {2 * sizeof(float), "gthresh specification",
                                            baneGkmsParseGthresh, NULL};

const hestCB *const baneGkmsHestGthresh = &_baneGkmsHestGthresh;

/* ----------------------------------------------------------- */
/* ----------------------------------------------------------- */
/* ----------------------------------------------------------- */

/*
******** baneGkmsCmdList[]
**
** NULL-terminated array of unrrduCmd pointers, as ordered by
** BANE_GKMS_MAP macro
*/
const unrrduCmd *const baneGkmsCmdList[] = {BANE_GKMS_MAP(BANE_GKMS_LIST) NULL};

/*
******** baneGkmsUsage
**
** prints out a little banner, and a listing of all available commands
** with their one-line descriptions
*/
void
baneGkmsUsage(const char *me, hestParm *hparm) {
  char buff[AIR_STRLEN_LARGE + 1], fmt[AIR_STRLEN_LARGE + 1];
  unsigned int ci, si, len, maxlen;

  maxlen = 0;
  for (ci = 0; baneGkmsCmdList[ci]; ci++) {
    maxlen = AIR_MAX(maxlen, AIR_UINT(strlen(baneGkmsCmdList[ci]->name)));
  }

  sprintf(buff, "--- Semi-Automatic Generation of Transfer Functions ---");
  sprintf(fmt, "%%%ds\n", (int)((hparm->columns - strlen(buff)) / 2 + strlen(buff) - 1));
  fprintf(stderr, fmt, buff);

  for (ci = 0; baneGkmsCmdList[ci]; ci++) {
    len = AIR_UINT(strlen(baneGkmsCmdList[ci]->name));
    strcpy(buff, "");
    for (si = len; si < maxlen; si++)
      strcat(buff, " ");
    strcat(buff, me);
    strcat(buff, " ");
    strcat(buff, baneGkmsCmdList[ci]->name);
    strcat(buff, " ... ");
    len = AIR_UINT(strlen(buff));
    fprintf(stderr, "%s", buff);
    _hestPrintStr(stderr, len, len, hparm->columns, baneGkmsCmdList[ci]->info,
                  AIR_FALSE);
  }
}

/* clang-format off */
static const char *
_baneGkmsMeasrStr[] = {
  "(unknown measr)",
  "min",
  "max",
  "mean",
  "median",
  "mode"
};

static const int
_baneGkmsMeasrVal[] = {
  nrrdMeasureUnknown,
  nrrdMeasureHistoMin,
  nrrdMeasureHistoMax,
  nrrdMeasureHistoMean,
  nrrdMeasureHistoMedian,
  nrrdMeasureHistoMode
};

static const airEnum
_baneGkmsMeasr = {
  "measurement",
  5,
  _baneGkmsMeasrStr, _baneGkmsMeasrVal,
  NULL,
  NULL, NULL,
  AIR_FALSE
};

const airEnum *const
baneGkmsMeasr = &_baneGkmsMeasr;
/* clang-format on */
