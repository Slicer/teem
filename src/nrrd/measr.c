/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/


#include "nrrd.h"


/* the non-histogram measures assume that there will be no NaNs in data */

void
_nrrdMeasrUnknown(void *line, int lineType, int len, float axmin, float axmax, 
		  void *ans, int ansType) {
  char me[]="_nrrdMeasrUnknown";
  
  fprintf(stderr, "%s: Need To Specify A Measure !!! \n", me);
  nrrdDStore[ansType](ans, airNand());
}

void
_nrrdMeasrMin(void *line, int lineType, int len, float axmin, float axmax, 
	      void *ans, int ansType) {
  double val, min;
  int i;

  min = nrrdDLookup[lineType](line, 0);
  for (i=1; i<=len-1; i++) {
    val = nrrdDLookup[lineType](line, i);
    min = AIR_MIN(min, val);
  }
  nrrdDStore[ansType](ans, min);
}

void
_nrrdMeasrMax(void *line, int lineType, int len, float axmin, float axmax, 
	      void *ans, int ansType) {
  double val, max;
  int i;

  max = nrrdDLookup[lineType](line, 0);
  for (i=1; i<=len-1; i++) {
    val = nrrdDLookup[lineType](line, i);
    max = AIR_MAX(max, val);
  }
  nrrdDStore[ansType](ans, max);
}

void
_nrrdMeasrProduct(void *line, int lineType, int len, float axmin, float axmax,
		  void *ans, int ansType) {
  double val, prod;
  int i;

  prod = 1;
  for (i=0; i<=len-1; i++) {
    val = nrrdDLookup[lineType](line, i);
    val = AIR_EXISTS(val) ? val : 0;
    prod *= val;
  }
  nrrdDStore[ansType](ans, prod);
}

void
_nrrdMeasrSum(void *line, int lineType, int len, float axmin, float axmax,
	      void *ans, int ansType) {
  double sum;
  int i;

  sum = 0;
  for (i=0; i<=len-1; i++) {
    sum += nrrdDLookup[lineType](line, i);
  }
  nrrdDStore[ansType](ans, sum);
}

void
_nrrdMeasrMean(void *line, int lineType, int len, float axmin, float axmax,
	       void *ans, int ansType) {
  double sum;
  int i;

  sum = 0;
  for (i=0; i<=len-1; i++) {
    sum += nrrdDLookup[lineType](line, i);
  }
  nrrdDStore[ansType](ans, sum/len);
}

void
_nrrdMeasrMode(void *line, int lineType, int len, float axmin, float axmax,
	       void *ans, int ansType) {
  char me[]="_nrrdMeasrMode";
  
  fprintf(stderr, "%s: sorry, not implemented\n", me);
  nrrdDStore[ansType](ans, airNand());
}

void
_nrrdMeasrMedian(void *line, int lineType, int len, float axmin, float axmax,
		 void *ans, int ansType) {
  char me[]="_nrrdMeasrMedian";
  
  fprintf(stderr, "%s: sorry, not implemented\n", me);
  nrrdDStore[ansType](ans, airNand());
}

void
_nrrdMeasrL1(void *line, int lineType, int len, float axmin, float axmax,
	     void *ans, int ansType) {
  double sum, val;
  int i;

  sum = 0;
  for (i=0; i<=len-1; i++) {
    val = nrrdDLookup[lineType](line, i);
    if (AIR_EXISTS(val)) {
      sum += AIR_ABS(val);
    }
  }
  nrrdDStore[ansType](ans, sum);
}

void
_nrrdMeasrL2(void *line, int lineType, int len, float axmin, float axmax,
	     void *ans, int ansType) {
  double sum, val;
  int i;

  sum = 0;
  for (i=0; i<=len-1; i++) {
    val = nrrdDLookup[lineType](line, i);
    if (AIR_EXISTS(val)) {
      sum += val*val;
    }
  }
  nrrdDStore[ansType](ans, sqrt(sum));
}

void
_nrrdMeasrLinf(void *line, int lineType, int len, float axmin, float axmax,
	       void *ans, int ansType) {
  double max, val;
  int i;

  max = 0;
  for (i=0; i<=len-1; i++) {
    val = nrrdDLookup[lineType](line, i);
    if (AIR_EXISTS(val)) {
      val = AIR_ABS(val);
      max = AIR_MAX(max, val);
    }
  }
  nrrdDStore[ansType](ans, max);
}

void
_nrrdMeasrHistoMedian(void *line, int lineType, int len, 
		      float axmin, float axmax, 
		      void *ans, int ansType) {
  double sum, half;
  int i;
  float ansF;
  
  sum = 0;
  for (i=0; i<=len-1; i++) {
    sum += nrrdDLookup[lineType](line, i);
  }
  if (!sum) {
    nrrdDStore[ansType](ans, airNand());
    return;
  }
  /* else there was something in the histogram */
  half = sum/2;
  sum = 0;
  for (i=0; i<=len-1; i++) {
    sum += nrrdDLookup[lineType](line, i);
    if (sum >= half) {
      break;
    }
  }
  ansF = i;
  if (AIR_EXISTS(axmin) && AIR_EXISTS(axmax)) 
    ansF = AIR_AFFINE(0, ansF, len-1, axmin, axmax);
  nrrdFStore[ansType](ans, ansF);
}

void
_nrrdMeasrHistoMode(void *line, int lineType, int len, 
		    float axmin, float axmax, 
		    void *ans, int ansType) {
  double val, max, idxsum;
  int i, idxcount;
  float ansF;
  
  max = 0;
  for (i=0; i<=len-1; i++) {
    val = nrrdDLookup[lineType](line, i);
    max = AIR_MAX(max, val);
  }
  if (!max) {
    nrrdDStore[ansType](ans, airNand());
    return;
  }
  /* else there was something in the histogram */
  /* we assume that there may be multiple bins which reach
     the maximum height, and we average all those indices.
     This may well be bone-headed, and is subject to change */
  idxsum = 0;
  idxcount = 0;
  for (i=0; i<=len-1; i++) {
    val = nrrdDLookup[lineType](line, i);
    if (val == max) {
      idxcount++;
      idxsum += i;
    }
  }
  ansF = idxsum/idxcount;
  if (AIR_EXISTS(axmin) && AIR_EXISTS(axmax)) 
    ansF = AIR_AFFINE(0, ansF, len-1, axmin, axmax);
  
  nrrdDStore[ansType](ans, ansF);
}

void
_nrrdMeasrHistoMean(void *line, int lineType, int len, 
		    float axmin, float axmax,
		    void *ans, int ansType) {
  double mean, count, hits;
  int i;
  
  if (!(AIR_EXISTS(axmin) && AIR_EXISTS(axmax))) {
    axmin = 0;
    axmax = len-1;
  }
  mean = count = 0;
  for (i=0; i<=len-1; i++) {
    hits = nrrdDLookup[lineType](line, i);
    count += hits;
    mean += hits*AIR_AFFINE(0, i, len-1, axmin, axmax);
  }
  if (!count) {
    nrrdDStore[ansType](ans, airNand());
    return;
  }
  mean /= count;
  nrrdDStore[ansType](ans, mean);
}

void
_nrrdMeasrHistoVariance(void *line, int lineType, int len, 
			float axmin, float axmax,
			void *ans, int ansType) {
  double mean, count, hits, val, vari, diff;
  int i;
  
  if (!(AIR_EXISTS(axmin) && AIR_EXISTS(axmax))) {
    axmin = 0;
    axmax = len-1;
  }
  mean = count = 0;
  for (i=0; i<=len-1; i++) {
    hits = nrrdDLookup[lineType](line, i);
    count += hits;
    mean += hits*AIR_AFFINE(0, i, len-1, axmin, axmax);
  }
  if (!count) {
    nrrdDStore[ansType](ans, airNand());
    return;
  }
  mean /= count;
  vari = 0;
  for (i=0; i<=len-1; i++) {
    hits = nrrdDLookup[lineType](line, i);
    count += hits;
    val = AIR_AFFINE(0, i, len-1, axmin, axmax);
    diff = val - mean;
    vari += hits*diff*diff;
  }
  if (count > 1) {
    vari /= count-1;
  }
  nrrdDStore[ansType](ans, vari);
}

void
_nrrdMeasrHistoProduct(void *line, int lineType, int len, 
		       float axmin, float axmax,
		       void *ans, int ansType) {
  double product, count, hits;
  int i;
  
  if (!(AIR_EXISTS(axmin) && AIR_EXISTS(axmax))) {
    axmin = 0;
    axmax = len-1;
  }
  count = 0;
  product = 1;
  for (i=0; i<=len-1; i++) {
    hits = nrrdDLookup[lineType](line, i);
    hits = AIR_EXISTS(hits) ? hits : 0;
    count += hits;
    product *= pow(AIR_AFFINE(0, i, len-1, axmin, axmax), hits);
  }
  if (!count) {
    nrrdDStore[ansType](ans, airNand());
    return;
  }
  nrrdDStore[ansType](ans, product);
}

void
_nrrdMeasrHistoSum(void *line, int lineType, int len, 
		   float axmin, float axmax,
		   void *ans, int ansType) {
  double sum, count, hits;
  int i;
  
  if (!(AIR_EXISTS(axmin) && AIR_EXISTS(axmax))) {
    axmin = 0;
    axmax = len-1;
  }
  sum = count = 0;
  for (i=0; i<=len-1; i++) {
    hits = nrrdDLookup[lineType](line, i);
    count += hits;
    sum += hits*AIR_AFFINE(0, i, len-1, axmin, axmax);
  }
  if (!count) {
    nrrdDStore[ansType](ans, airNand());
    return;
  }
  nrrdDStore[ansType](ans, sum);
}

void
_nrrdMeasrHistoMax(void *line, int lineType, int len, 
		   float axmin, float axmax,
		   void *ans, int ansType) {
  int i;

  if (!(AIR_EXISTS(axmin) && AIR_EXISTS(axmax))) {
    axmin = 0;
    axmax = len-1;
  }
  for (i=len-1; i>=0; i--) {
    if (nrrdDLookup[lineType](line, i))
      break;
  }
  if (i==-1) {
    nrrdDStore[ansType](ans, airNand());
    return;
  }
  nrrdDStore[ansType](ans, AIR_AFFINE(0, i, len-1, axmin, axmax));
}

void
_nrrdMeasrHistoMin(void *line, int lineType, int len, 
		   float axmin, float axmax,
		   void *ans, int ansType) {
  int i;

  if (!(AIR_EXISTS(axmin) && AIR_EXISTS(axmax))) {
    axmin = 0;
    axmax = len-1;
  }
  for (i=0; i<=len-1; i++) {
    if (nrrdDLookup[lineType](line, i))
      break;
  }
  if (i==len) {
    nrrdDStore[ansType](ans, airNand());
    return;
  }
  nrrdDStore[ansType](ans, AIR_AFFINE(0, i, len-1, axmin, axmax));
}

void (*nrrdMeasr[NRRD_MEASR_MAX+1])(void *, int, int, float, float, 
				    void *, int) = {
  _nrrdMeasrUnknown,
  _nrrdMeasrMin,
  _nrrdMeasrMax,
  _nrrdMeasrProduct,
  _nrrdMeasrSum,
  _nrrdMeasrMean,
  _nrrdMeasrMedian,
  _nrrdMeasrMode,
  _nrrdMeasrL1,
  _nrrdMeasrL2,
  _nrrdMeasrLinf,
  _nrrdMeasrHistoMin,
  _nrrdMeasrHistoMax,
  _nrrdMeasrHistoProduct,
  _nrrdMeasrHistoSum,
  _nrrdMeasrHistoMean,
  _nrrdMeasrHistoMedian,
  _nrrdMeasrHistoMode,
  _nrrdMeasrHistoVariance
};

int
_nrrdMeasureType(Nrrd *nin, int measr) {
  int type;

  switch(measr) {
  case nrrdMeasrMin:
  case nrrdMeasrMax:
  case nrrdMeasrMedian:
  case nrrdMeasrMode:
    type = nin->type;
    break;
  case nrrdMeasrMean:
    /* the rational for this is that if you're after the average
       value along a scanline, you probably want it in the same
       format as what you started with, and if you really want
       an exact answer than you can always use nrrdMeasrSum.
       This may well be bone-headed, so is subject to change */
    type = nin->type;
    break;
  case nrrdMeasrProduct:
  case nrrdMeasrSum:
  case nrrdMeasrL1:
  case nrrdMeasrL2:
  case nrrdMeasrLinf:
    type = nrrdTypeFloat;
    break;
  case nrrdMeasrHistoMin:
  case nrrdMeasrHistoMax:
  case nrrdMeasrHistoProduct:
  case nrrdMeasrHistoSum:
  case nrrdMeasrHistoMean:
  case nrrdMeasrHistoMedian:
  case nrrdMeasrHistoMode:
  case nrrdMeasrHistoVariance:
    /* We (currently) don't keep track of the type of the original
       values which generated the histogram, and we may not even
       have access to that information.  Float is a defensible 
       default choice, no? */
    type = nrrdTypeFloat;
    break;
  }

  return type;
}

int
nrrdMeasureAxis(Nrrd *nout, Nrrd *nin, int axis, int measr) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdMeasureAxis";
  int type;
  int i, j, length, numperiod, lambda, period, inElSize, outElSize;
  char *line, *src, *dest;
  float axmin, axmax;
  
  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (!(nrrdMeasrUnknown < measr && measr < nrrdMeasrLast)) {
    sprintf(err, "%s: measure %d not recognized", me, measr);
    biffSet(NRRD, err); return 1;
  }
  if (!(AIR_INSIDE(0, axis, nin->dim-1))) {
    sprintf(err, "%s: axis %d not in range [0,%d]", me, axis, nin->dim-1);
    biffSet(NRRD, err); return 1;
  }
  
  type = _nrrdMeasureType(nin, measr);

  /* set up control variables */
  length = numperiod = 1;
  for (i=0; i<=nin->dim-1; i++) {
    if (i < axis) {
      length *= nin->size[i];
    }
    else if (i > axis) {
      numperiod *= nin->size[i];
    }
  }
  lambda = length*nin->size[axis];
  
  /* allocate space if necessary */
  if (!(nout->data)) {
    if (nrrdAlloc(nout, nin->num/nin->size[axis], type, nin->dim-1)) {
      sprintf(err, "%s: failed to create output", me);
      biffAdd(NRRD, err); return 1;
    }
  }

  /* allocate a scanline buffer */
  inElSize = nrrdTypeSize[nin->type];
  outElSize = nrrdTypeSize[type];
  if (!(line = calloc(nin->size[axis], inElSize))) {
    sprintf(err, "%s: couldn't calloc(%d,%d) scanline buffer",
	    me, nin->size[axis], nrrdElementSize(nin));
    biffSet(NRRD, err); return 1;
  }

  /* here's the skinny */
  src = nin->data;
  dest = nout->data;
  axmin = nin->axisMin[axis];
  axmax = nin->axisMax[axis];
  for (period=0; period<=numperiod-1; period++) {
    /* printf("%s: period = %d of %d\n", me, period, numperiod-1); */
    /* within each period of the square wave, we traverse the successive
       elements, copy the scanline starting at each element, and perform
       the measurement */
    for (j=0; j<=length-1; j++) {
      for (i=0; i<=nin->size[axis]-1; i++) {
	memcpy(line + inElSize*i, 
	       src + inElSize*(period*lambda + length*i + j), 
	       inElSize);
      }
      nrrdMeasr[measr](line, nin->type, nin->size[axis], axmin, axmax,
			   dest, nout->type);
      dest += outElSize;
    }
  }
  
  /* copy the peripheral information */
  for (i=0; i<=nout->dim-1; i++) {
    nout->size[i] = nin->size[i + (i >= axis)];
    nout->spacing[i] = nin->spacing[i + (i >= axis)];
    nout->axisMin[i] = nin->axisMin[i + (i >= axis)];
    nout->axisMax[i] = nin->axisMax[i + (i >= axis)];
    strcpy(nout->label[i], nin->label[i + (i >= axis)]);
  }
  sprintf(nout->content, "measr(%s,%d,%d)", 
	  nin->content, axis, measr);

  /* bye */
  free(line);
  return 0;
}
