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

static int /* Biff: 1 */
_rangePositive_Answer(double *ominP, double *omaxP, double imin, double imax) {
  static const char me[] = "_rangePositive_Answer";

  if (!(AIR_EXISTS(imin) && AIR_EXISTS(imax))) {
    biffAddf(BANE, "%s: imin and imax don't both exist", me);
    return 1;
  }
  *ominP = 0;
  *omaxP = imax;
  return 0;
}

static int /* Biff: 1 */
_rangeNegative_Answer(double *ominP, double *omaxP, double imin, double imax) {
  static const char me[] = "_rangeNegative_Answer";

  if (!(AIR_EXISTS(imin) && AIR_EXISTS(imax))) {
    biffAddf(BANE, "%s: imin and imax don't both exist", me);
    return 1;
  }
  *ominP = imin;
  *omaxP = 0;
  return 0;
}

/*
** _rangeZeroCentered_Answer
**
** Unlike the last version of this function, this is conservative: we
** choose the smallest zero-centered range that includes the original
** min and max.  Previously the average of the min and max magnitude
** were used.
*/
static int /* Biff: 1 */
_rangeZeroCentered_Answer(double *ominP, double *omaxP, double imin, double imax) {
  static const char me[] = "_rangeZeroCentered_Answer";

  if (!(AIR_EXISTS(imin) && AIR_EXISTS(imax))) {
    biffAddf(BANE, "%s: imin and imax don't both exist", me);
    return 1;
  }
  imin = AIR_MIN(imin, 0);
  imax = AIR_MAX(imax, 0);
  /* now the signs of imin and imax aren't wrong */
  *ominP = AIR_MIN(-imax, imin);
  *omaxP = AIR_MAX(imax, -imin);
  return 0;
}

static int /* Biff: 1 */
_rangeAnywhere_Answer(double *ominP, double *omaxP, double imin, double imax) {
  static const char me[] = "_rangeAnywhere_Answer";

  if (!(AIR_EXISTS(imin) && AIR_EXISTS(imax))) {
    biffAddf(BANE, "%s: imin and imax don't both exist", me);
    return 1;
  }
  *ominP = imin;
  *omaxP = imax;
  return 0;
}

baneRange * /* Biff: NULL */
baneRangeNew(int type) {
  static const char me[] = "baneRangeNew";
  baneRange *range = NULL;

  if (!AIR_IN_OP(baneRangeUnknown, type, baneRangeLast)) {
    biffAddf(BANE, "%s: baneRange %d not valid", me, type);
    return NULL;
  }
  range = (baneRange *)calloc(1, sizeof(baneRange));
  if (!range) {
    biffAddf(BANE, "%s: couldn't allocate baneRange!", me);
    return NULL;
  }
  range->type = type;
  range->center = AIR_NAN;
  switch (type) {
  case baneRangePositive:
    sprintf(range->name, "positive");
    range->answer = _rangePositive_Answer;
    break;
  case baneRangeNegative:
    sprintf(range->name, "negative");
    range->answer = _rangeNegative_Answer;
    break;
  case baneRangeZeroCentered:
    sprintf(range->name, "zero-centered");
    range->answer = _rangeZeroCentered_Answer;
    break;
  case baneRangeAnywhere:
    sprintf(range->name, "anywhere");
    range->answer = _rangeAnywhere_Answer;
    break;
  default:
    biffAddf(BANE, "%s: Sorry, baneRange %d not implemented", me, type);
    baneRangeNix(range);
    return NULL;
  }
  return range;
}

baneRange * /* Biff: NULL */
baneRangeCopy(baneRange *range) {
  static const char me[] = "baneRangeCopy";
  baneRange *ret = NULL;

  ret = baneRangeNew(range->type);
  if (!ret) {
    biffAddf(BANE, "%s: couldn't make new range", me);
    return NULL;
  }
  ret->center = range->center;
  return ret;
}

int /* Biff: 1 */
baneRangeAnswer(baneRange *range, double *ominP, double *omaxP, double imin,
                double imax) {
  static const char me[] = "baneRangeAnswer";

  if (!(range && ominP && omaxP)) {
    biffAddf(BANE, "%s: got NULL pointer", me);
    return 1;
  }
  if (range->answer(ominP, omaxP, imin, imax)) {
    biffAddf(BANE, "%s: trouble", me);
    return 1;
  }
  return 0;
}

baneRange * /* Biff: nope */
baneRangeNix(baneRange *range) {

  if (range) {
    airFree(range);
  }
  return NULL;
}
