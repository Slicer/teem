/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "bane.h"
#include "privateBane.h"

int
_baneRangePositive_Answer(double *ominP, double *omaxP,
                          double imin, double imax) {
  char me[]="_baneRangePositive_Answer", err[AIR_STRLEN_MED];

  if (!( AIR_EXISTS(imin) && AIR_EXISTS(imax) )) {
    sprintf(err, "%s: imin and imax don't both exist", me);
    biffAdd(BANE, err); return 1;
  }
  *ominP = 0;
  *omaxP = imax;
  return 0;
}

int
_baneRangeNegative_Answer(double *ominP, double *omaxP,
                          double imin, double imax) {
  char me[]="_baneRangeNegative_Answer", err[AIR_STRLEN_MED];
  
  if (!( AIR_EXISTS(imin) && AIR_EXISTS(imax) )) {
    sprintf(err, "%s: imin and imax don't both exist", me);
    biffAdd(BANE, err); return 1;
  }
  *ominP = imin;
  *omaxP = 0;
  return 0;
}

/*
** _baneRangeZeroCentered_Answer
**
** Unlike the last version of this function, this is conservative: we
** choose the smallest zero-centered range that includes the original
** min and max.  Previously the average of the min and max magnitude
** were used.
*/
int
_baneRangeZeroCentered_Answer(double *ominP, double *omaxP,
                              double imin, double imax) {
  char me[]="_baneRangeZeroCentered_Answer", err[AIR_STRLEN_MED];

  if (!( AIR_EXISTS(imin) && AIR_EXISTS(imax) )) {
    sprintf(err, "%s: imin and imax don't both exist", me);
    biffAdd(BANE, err); return 1;
  }
  imin = AIR_MIN(imin, 0);
  imax = AIR_MAX(imax, 0);
  /* now the signs of imin and imax aren't wrong */
  *ominP = AIR_MIN(-imax, imin);
  *omaxP = AIR_MAX(imax, -imin);
  return 0;
}

int
_baneRangeAnywhere_Answer(double *ominP, double *omaxP,
                          double imin, double imax) {
  char me[]="_baneRangeAnywhere_Answer", err[AIR_STRLEN_MED];

  if (!( AIR_EXISTS(imin) && AIR_EXISTS(imax) )) {
    sprintf(err, "%s: imin and imax don't both exist", me);
    biffAdd(BANE, err); return 1;
  }
  *ominP = imin;
  *omaxP = imax;
  return 0;
}

baneRange *
baneRangeNew(int type) {
  char me[]="baneRangeNew", err[AIR_STRLEN_MED];
  baneRange *range = NULL;
  
  if (!AIR_IN_OP(baneRangeUnknown, type, baneRangeLast)) {
    sprintf(err, "%s: baneRange %d not valid", me, type);
    biffAdd(BANE, err); return NULL;
  }
  range = (baneRange *)calloc(1, sizeof(baneRange));
  if (!range) {
    sprintf(err, "%s: couldn't allocate baneRange!", me);
    biffAdd(BANE, err); return NULL;
  }
  range->type = type;
  range->center = AIR_NAN;
  switch(type) {
  case baneRangePositive:
    sprintf(range->name, "positive");
    range->answer = _baneRangePositive_Answer;
    break;
  case baneRangeNegative:
    sprintf(range->name, "negative");
    range->answer = _baneRangeNegative_Answer;
    break;
  case baneRangeZeroCentered:
    sprintf(range->name, "zero-centered");
    range->answer = _baneRangeZeroCentered_Answer;
    break;
  case baneRangeAnywhere:
    sprintf(range->name, "anywhere");
    range->answer = _baneRangeAnywhere_Answer;
    break;
  default:
    sprintf(err, "%s: Sorry, baneRange %d not implemented", me, type);
    biffAdd(BANE, err); baneRangeNix(range); return NULL;
  }
  return range;
}

baneRange *
baneRangeCopy(baneRange *range) {
  char me[]="baneRangeCopy", err[AIR_STRLEN_MED];
  baneRange *ret = NULL;
  
  ret = baneRangeNew(range->type);
  if (!ret) {
    sprintf(err, "%s: couldn't make new range", me);
    biffAdd(BANE, err); return NULL;
  }
  ret->center = range->center;
  return ret;
}

int
baneRangeAnswer(baneRange *range,
                double *ominP, double *omaxP,
                double imin, double imax) {
  char me[]="baneRangeAnswer", err[AIR_STRLEN_MED];
  
  if (!(range && ominP && omaxP)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(BANE, err); return 1;
  }
  if (range->answer(ominP, omaxP, imin, imax)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(BANE, err); return 1;
  }
  return 0;
}

baneRange *
baneRangeNix(baneRange *range) {

  if (range) {
    airFree(range);
  }
  return NULL;
}
