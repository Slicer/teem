/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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

Nrrd *
_baneInc_EmptyHistNew(double *incParm) {

  return nrrdNew();
}

Nrrd *
_baneInc_HistNew(double *incParm) {
  Nrrd *nhist;
  
  nrrdAlloc(nhist=nrrdNew(), nrrdTypeInt, 1, (int)(incParm[0]));
  return nhist;
}

void
_baneInc_LearnMinMax(Nrrd *n, double val, double *incParm) {

  if (AIR_EXISTS(n->axis[0].min)) {
    n->axis[0].min = AIR_MIN(n->axis[0].min, val);
  } else {
    n->axis[0].min = val;
  }
  if (AIR_EXISTS(n->axis[0].max)) {
    n->axis[0].max = AIR_MAX(n->axis[0].max, val);
  } else {
    n->axis[0].max = val;
  }
}

void
_baneInc_HistFill(Nrrd *n, double val, double *incParm) {
  int idx, *hist;

  AIR_INDEX(n->axis[0].min, val, n->axis[0].max, n->axis[0].size, idx);
  if (AIR_INSIDE(0, idx, n->axis[0].size-1)) {
    hist = (int*)n->data;
    hist[idx]++;
  }
}

/* ----------------- baneIncAbsolute -------------------- */

void
_baneIncAbsolute_Ans(double *minP, double *maxP,
		     Nrrd *hist, double *incParm,
		     baneRange *range) {
  *minP = incParm[0];
  *maxP = incParm[1];
}

baneInc
_baneIncAbsolute = {
  "absolute",
  baneIncAbsolute_e,
  2,
  _baneInc_EmptyHistNew,
  NULL,
  NULL,
  _baneIncAbsolute_Ans
};
baneInc *
baneIncAbsolute = &_baneIncAbsolute;
  
/* ----------------- baneIncRangeRatio -------------------- */

/*
** _baneIncRangeRatio_Ans
**
** uses incParm[0] to scale min and max, after they've been sent through
** the range function.  For baneRangeFloat, if incParm[1] exists, it
** is used as the midpoint of the scaling, otherwise the average is
** used.  For all other ranges, zero is used as the scaling midpoint.
*/
void
_baneIncRangeRatio_Ans(double *minP, double *maxP, 
		       Nrrd *hist, double *incParm,
		       baneRange *range) {
  double mid;
  
  range->ans(minP, maxP, hist->axis[0].min, hist->axis[0].max);

  if (baneRangeFloat == range) {
    mid = AIR_EXISTS(incParm[1]) ? incParm[1] : (*minP + *maxP)/2;
    *minP = AIR_AFFINE(-1, -incParm[0], 0, *minP, mid);
    *maxP = AIR_AFFINE(0, incParm[0], 1, mid, *maxP);
  }
  else {
    *minP *= incParm[0];
    *maxP *= incParm[0];
  }
}

baneInc
_baneIncRangeRatio = {
  "range-ratio",
  baneIncRangeRatio_e,
  2,
  _baneInc_EmptyHistNew,
  NULL,
  NULL,
  _baneIncRangeRatio_Ans
};
baneInc *
baneIncRangeRatio = &_baneIncRangeRatio;


/* ----------------- baneIncPercentile -------------------- */

void
_baneIncPercentile_Ans(double *minP, double *maxP,
		       Nrrd *nhist, double *incParm,
		       baneRange *range) {
  char me[]="_baneIncPercentile_Ans";
  int *hist, i, histSize, sum;
  float minIncr, maxIncr, out, outsofar, mid, minIdx, maxIdx;
  double min, max;
  
  /* integrate histogram and determine how many hits to exclude */
  sum = 0;
  hist = nhist->data;
  histSize = nhist->axis[0].size;
  for (i=0; i<histSize; i++) {
    sum += hist[i];
  }
  out = sum*incParm[0]/100.0;
  fprintf(stderr, "##%s: hist's size=%d, sum=%d --> out = %g\n", me,
	  histSize, sum, out);
  range->ans(&min, &max, nhist->axis[0].min, nhist->axis[0].max);
  if (baneRangeFloat) {
    mid = AIR_EXISTS(incParm[1]) ? incParm[1] : (min + max)/2;
  } else {
    mid = 0;
  }
  fprintf(stderr, "##%s: hist (%g,%g) --> min,max = (%g,%g) --> %g\n", me,
	  nhist->axis[0].min, nhist->axis[0].max, min, max, mid);
  if (max-mid > mid-min) {
    /* the max is further from the mid than the min */
    maxIncr = 1;
    minIncr = (mid-min)/(max-mid);
  } else {
    /* the min is further */
    minIncr = 1;
    maxIncr = (max-mid)/(mid-min);
  }
  fprintf(stderr, "##%s: --> {min,max}Incr = %g,%g\n", me, minIncr, maxIncr);
  minIdx = AIR_AFFINE(nhist->axis[0].min, min, nhist->axis[0].max, 0, histSize-1);
  maxIdx = AIR_AFFINE(nhist->axis[0].min, max, nhist->axis[0].max, 0, histSize-1);
  outsofar = 0;
  while (outsofar < out) {
    if (AIR_INSIDE(0, minIdx, histSize-1)) {
      outsofar += minIncr*hist[AIR_ROUNDUP(minIdx)];
    }
    if (AIR_INSIDE(0, maxIdx, histSize-1)) {
      outsofar += maxIncr*hist[AIR_ROUNDUP(maxIdx)];
    }
    minIdx += minIncr;
    maxIdx -= maxIncr;
    if (minIdx > maxIdx) {
      fprintf(stderr, "%s: PANIC: something has gone terribly wrong !!! \n", me);
      exit(-1);
    }
  }
  *minP = AIR_AFFINE(0, minIdx, histSize-1, nhist->axis[0].min, nhist->axis[0].max);
  *maxP = AIR_AFFINE(0, maxIdx, histSize-1, nhist->axis[0].min, nhist->axis[0].max);
  fprintf(stderr, "##%s: --> output min, max = %g, %g\n", me, *minP, *maxP);
  return;

}

baneInc
_baneIncPercentile = {
  "percentile",
  baneIncPercentile_e,
  2,
  _baneInc_HistNew,
  _baneInc_LearnMinMax,
  _baneInc_HistFill,
  _baneIncPercentile_Ans,
};

/* ----------------- baneIncStdv -------------------- */

Nrrd *
_baneIncStdv_HistNew(double *incParm) {
  Nrrd *hist;

  hist = nrrdNew();
  /* this is a total horrid sham and a hack:  we're going to use 
     axis[1].min to store the sum of all values, and 
     axis[1].max to store the sum of all squared values, and 
     axis[1].size to store the number of values.
     The road to hell ... 
  */
  hist->axis[1].min = 0.0;
  hist->axis[1].max = 0.0;
  hist->axis[1].size = 0;
  return hist;
}

void 
_baneIncStdv_Pass(Nrrd *hist, double val, double *incParm) {
  
  _baneInc_LearnMinMax(hist, val, incParm);
  hist->axis[1].min += val;
  hist->axis[1].max += val*val;
  hist->axis[1].size += 1;
}


void
_baneIncStdv_Ans(double *minP, double *maxP,
		 Nrrd *hist, double *incParm,
		 baneRange *range) {
  float SS, stdv, mid, mean;
  int count;

  count = hist->axis[1].size;
  mean = hist->axis[1].min/count;
  SS = hist->axis[1].max/count;
  stdv = sqrt(SS - mean*mean);
  switch (range->which) {
  case baneRangePos_e:
    *minP = 0;
    *maxP = incParm[0]*stdv;
    break;
  case baneRangeNeg_e:
    *minP = -incParm[0]*stdv;
    *maxP = 0;
    break;
  case baneRangeZeroCent_e:
    *minP = -incParm[0]*stdv/2;
    *maxP = incParm[0]*stdv/2;
    break;
  case baneRangeFloat_e:
    mid = AIR_EXISTS(incParm[1]) ? incParm[1] : mean;
    *minP = mid - incParm[0]*stdv/2;
    *maxP = mid + incParm[1]*stdv/2;
    break;
  default:
    *minP = *maxP = AIR_NAN;
    break;
  }
  
}

baneInc 
_baneIncStdv = {
  "stdv",
  baneIncStdv_e,
  2,
  _baneIncStdv_HistNew,
  NULL,
  _baneIncStdv_Pass,
  _baneIncStdv_Ans
};
baneInc *
baneIncStdv = &_baneIncStdv;
     
/* -------------------------------------------------- */

baneInc *baneIncArray[BANE_INC_MAX+1] = {
  &_baneIncAbsolute,
  &_baneIncRangeRatio,
  &_baneIncPercentile,
  &_baneIncStdv
};
