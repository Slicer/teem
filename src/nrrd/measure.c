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
_nrrdMeasureUnknown(void *line, int lineType, int len, 
		    float axmin, float axmax, 
		    void *ans, int ansType) {
  char me[]="_nrrdMeasureUnknown";
  
  fprintf(stderr, "%s: Need To Specify A Measure !!! \n", me);
  nrrdDStore[ansType](ans, AIR_NAN);
}

void
_nrrdMeasureMin(void *line, int lineType, int len,
		float axmin, float axmax, 
		void *ans, int ansType) {
  double val, M;
  int i;

  if (nrrdTypeFixed[lineType]) {
    M = nrrdDLookup[lineType](line, 0);
    for (i=1; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      M = AIR_MIN(M, val);
    }
  }
  else {
    M = AIR_NAN;
    for (i=0; !AIR_EXISTS(M) && i<len; i++)
      M = nrrdDLookup[lineType](line, i);
    for (; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      if (AIR_EXISTS(val)) {
	M = AIR_MIN(M, val);
      }
    }
  }
  nrrdDStore[ansType](ans, M);
}

void
_nrrdMeasureMax(void *line, int lineType, int len, 
		float axmin, float axmax, 
		void *ans, int ansType) {
  double val, M;
  int i;

  if (nrrdTypeFixed[lineType]) {
    M = nrrdDLookup[lineType](line, 0);
    for (i=1; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      M = AIR_MAX(M, val);
    }
  }
  else {
    M = AIR_NAN;
    for (i=0; !AIR_EXISTS(M) && i<len; i++)
      M = nrrdDLookup[lineType](line, i);
    for (; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      if (AIR_EXISTS(val)) {
	M = AIR_MAX(M, val);
      }
    }
  }
  nrrdDStore[ansType](ans, M);
}

void
_nrrdMeasureProduct(void *line, int lineType, int len, 
		    float axmin, float axmax,
		    void *ans, int ansType) {
  double val, P;
  int i;

  if (nrrdTypeFixed[lineType]) {
    P = 1.0;
    for (i=0; i<len; i++) {
      P *= nrrdDLookup[lineType](line, i);
    }
  }
  else {
    P = AIR_NAN;
    /* the point of this is to ensure that that if there are NO
       existant values, then the return is NaN */
    for (i=0; !AIR_EXISTS(P) && i<len; i++)
      P = nrrdDLookup[lineType](line, i);
    for (; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      if (AIR_EXISTS(val)) {
	P *= val;
      }
    }
  }
  nrrdDStore[ansType](ans, P);
}

void
_nrrdMeasureSum(void *line, int lineType, int len, 
		float axmin, float axmax,
		void *ans, int ansType) {
  double val, S;
  int i;

  if (nrrdTypeFixed[lineType]) {
    S = 0.0;
    for (i=0; i<len; i++) {
      S += nrrdDLookup[lineType](line, i);
    }
  }
  else {
    S = AIR_NAN;
    for (i=0; !AIR_EXISTS(S) && i<len; i++)
      S = nrrdDLookup[lineType](line, i);
    for (; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      if (AIR_EXISTS(val)) {
	S += val;
      }
    }
  }
  nrrdDStore[ansType](ans, S);
}

void
_nrrdMeasureMean(void *line, int lineType, int len, 
		 float axmin, float axmax,
		 void *ans, int ansType) {
  double val, S;
  int i;

  if (nrrdTypeFixed[lineType]) {
    S = 0.0;
    for (i=0; i<len; i++) {
      S += nrrdDLookup[lineType](line, i);
    }
  }
  else {
    S = AIR_NAN;
    for (i=0; !AIR_EXISTS(S) && i<len; i++)
      S = nrrdDLookup[lineType](line, i);
    for (; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      if (AIR_EXISTS(val)) {
	S += val;
      }
    }
  }
  nrrdDStore[ansType](ans, S/len);
}

/* stupid little forward declaration */
void
_nrrdMeasureHistoMode(void *line, int lineType, int len, 
		      float axmin, float axmax, 
		      void *ans, int ansType);

void
_nrrdMeasureMode(void *line, int lineType, int len, 
		 float axmin, float axmax,
		 void *ans, int ansType) {
  Nrrd *nline, *nhist;
  airArray *mop;

  mop = airMopInit();
  nline = nrrdNew();
  airMopAdd(mop, nline, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdWrap(nline, line, lineType, 1, len)) {
    free(biffGetDone(NRRD));
    airMopError(mop);
    nrrdDStore[ansType](ans, AIR_NAN);
    return;
  }
  nhist = nrrdNew();
  airMopAdd(mop, nhist, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdHisto(nhist, nline, nrrdStateMeasureModeBins, nrrdTypeInt)) {
    free(biffGetDone(NRRD));
    airMopError(mop);
    nrrdDStore[ansType](ans, AIR_NAN);
    return;
  }

  /* now we pass this histogram off to histo-mode */
  _nrrdMeasureHistoMode(nhist->data, nrrdTypeInt, nrrdStateMeasureModeBins,
			nhist->axis[0].min, nhist->axis[0].max,
			ans, ansType);
  airMopOkay(mop);
  return;
}

void
_nrrdMeasureMedian(void *line, int lineType, int len, 
		   float axmin, float axmax,
		   void *ans, int ansType) {
  double M;
  int i, mid;
  
  /* we're allowed to reorder the given line because we copied */
  qsort(line, len, nrrdTypeSize[lineType], nrrdValCompare[lineType]);
  M = AIR_NAN;
  for (i=0; !AIR_EXISTS(M) && i<len; i++)
    M = nrrdDLookup[lineType](line, i);

  if (AIR_EXISTS(M)) {
    /* i is index AFTER first existant value */
    i--;
    len -= i;
    mid = len/2;
    if (len % 2) {
      M = nrrdDLookup[lineType](line, i+mid);
      M += nrrdDLookup[lineType](line, i+mid+1);
      M /= 2.0;
    }
    else {
      M = nrrdDLookup[lineType](line, i+mid);
    }
  }
  nrrdDStore[ansType](ans, M);
}

void
_nrrdMeasureL1(void *line, int lineType, int len, 
	       float axmin, float axmax,
	       void *ans, int ansType) {
  double val, S;
  int i;

  if (nrrdTypeFixed[lineType]) {
    S = 0.0;
    for (i=0; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      S += AIR_ABS(val);
    }
  }
  else {
    S = AIR_NAN;
    for (i=0; !AIR_EXISTS(S) && i<len; i++)
      S = nrrdDLookup[lineType](line, i);
    for (; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      if (AIR_EXISTS(val)) {
	S += AIR_ABS(val);
      }
    }
  }
  nrrdDStore[ansType](ans, S);
}

void
_nrrdMeasureL2(void *line, int lineType, int len, 
	       float axmin, float axmax,
	       void *ans, int ansType) {
  double val, S;
  int i;

  if (nrrdTypeFixed[lineType]) {
    S = 0.0;
    for (i=0; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      S += val*val;
    }
  }
  else {
    S = AIR_NAN;
    for (i=0; !AIR_EXISTS(S) && i<len; i++)
      S = nrrdDLookup[lineType](line, i);
    for (; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      if (AIR_EXISTS(val)) {
	S += val*val;
      }
    }
  }
  nrrdDStore[ansType](ans, sqrt(S));
}

void
_nrrdMeasureLinf(void *line, int lineType, int len, 
		 float axmin, float axmax,
		 void *ans, int ansType) {
  double val, M;
  int i;

  if (nrrdTypeFixed[lineType]) {
    val = nrrdDLookup[lineType](line, 0);
    M = AIR_ABS(val);
    for (i=1; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      val = AIR_ABS(val);
      M = AIR_MAX(M, val);
    }
  }
  else {
    M = AIR_NAN;
    for (i=0; !AIR_EXISTS(M) && i<len; i++)
      M = nrrdDLookup[lineType](line, i);
    M = AIR_ABS(M);
    for (; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      if (AIR_EXISTS(val)) {
	val = AIR_ABS(val);
	M = AIR_MAX(M, val);
      }
    }
  }
  nrrdDStore[ansType](ans, M);
}

void
_nrrdMeasureVariance(void *line, int lineType, int len, 
		     float axmin, float axmax,
		     void *ans, int ansType) {
  double val, S, SS;
  int i, count;

  SS = S = 0.0;
  if (nrrdTypeFixed[lineType]) {
    for (i=0; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      S += val;
      SS += val*val;
    }
    S /= len;
    SS /= len;
  }
  else {
    count = 0;
    for (i=0; i<len; i++) {
      val = nrrdDLookup[lineType](line, i);
      if (AIR_EXISTS(val)) {
	count++;
	S += val;
	SS += val*val;
      }
    }
    S /= count;
    SS /= count;
  }
  nrrdDStore[ansType](ans, SS - S*S);
}

/*
** one thing which ALL the _nrrdMeasureHisto measures assume is that,
** being a histogram, the input array will not have any non-existant
** values.  It can be floating point, because it is plausible to have
** some histogram composed of fractionally weighted hits, but there is
** no way that it is reasonable to have NaN in a bit, and it is extremely
** unlikely that Inf could actually be created in a floating point
** histogram.
**
** Another property that is sometimes assumed is that the values in
** the histogram are non-negative.
**
** All the the  _nrrdMeasureHisto measures assume that if not both
** axmin and axmax are existant, then (axmin,axmax) = (-0.5,len-0.5).
** Exercise for the reader:  Show that
**
**    i == NRRD_AXIS_POS(nrrdCenterCell, 0, len-1, len, i)
**
** This justifies that fact that when axmin and axmax are not both
** existant, then we can simply calculate the answer in index space,
** and not have to do any shifting or scaling at the end to account
** for the fact that we assume (axmin,axmax) = (-0.5,len-0.5).
*/

void
_nrrdMeasureHistoMedian(void *line, int lineType, int len, 
			float axmin, float axmax, 
			void *ans, int ansType) {
  double sum, half, ansD;
  int i;
  
  sum = 0;
  for (i=0; i<=len-1; i++) {
    sum += nrrdDLookup[lineType](line, i);
  }
  if (!sum) {
    nrrdDStore[ansType](ans, AIR_NAN);
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
  ansD = i;
  if (AIR_EXISTS(axmin) && AIR_EXISTS(axmax)) 
    ansD = NRRD_AXIS_POS(nrrdCenterCell, axmin, axmax, len, ansD);
  nrrdDStore[ansType](ans, ansD);
}

void
_nrrdMeasureHistoMode(void *line, int lineType, int len, 
		      float axmin, float axmax, 
		      void *ans, int ansType) {
  double val, max, idxsum, ansD;
  int i, idxcount;
  
  max = 0;
  for (i=0; i<=len-1; i++) {
    val = nrrdDLookup[lineType](line, i);
    max = AIR_MAX(max, val);
  }
  if (!max) {
    nrrdDStore[ansType](ans, AIR_NAN);
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
  ansD = idxsum/idxcount;
  /*
  printf("idxsum = %g; idxcount = %d --> ansD = %g --> ",
	 (float)idxsum, idxcount, ansD);
  */
  if (AIR_EXISTS(axmin) && AIR_EXISTS(axmax)) 
    ansD = NRRD_AXIS_POS(nrrdCenterCell, axmin, axmax, len, ansD);
  /*
  printf("%g\n", ansD);
  */
  nrrdDStore[ansType](ans, ansD);
}

void
_nrrdMeasureHistoMean(void *line, int lineType, int len, 
		      float axmin, float axmax,
		      void *ans, int ansType) {
  double count, hits, ansD;
  int i;
  
  ansD = count = 0;
  for (i=0; i<=len-1; i++) {
    hits = nrrdDLookup[lineType](line, i);
    count += hits;
    ansD += hits*i;
  }
  if (!count) {
    nrrdDStore[ansType](ans, AIR_NAN);
    return;
  }
  ansD /= count;
  if (AIR_EXISTS(axmin) && AIR_EXISTS(axmax)) 
    ansD = NRRD_AXIS_POS(nrrdCenterCell, axmin, axmax, len, ansD);
  nrrdDStore[ansType](ans, ansD);
}

void
_nrrdMeasureHistoVariance(void *line, int lineType, int len, 
			  float axmin, float axmax,
			  void *ans, int ansType) {
  double S, SS, count, hits, val;
  int i;
  
  count = 0;
  SS = S = 0.0;
  /* we fix axmin, axmax now because GK is better safe than sorry */
  if (!(AIR_EXISTS(axmin) && AIR_EXISTS(axmax))) {
    axmin = -0.5;
    axmax = len-0.5;
  }
  for (i=0; i<=len-1; i++) {
    val = NRRD_AXIS_POS(nrrdCenterCell, axmin, axmax, len, i);
    hits = nrrdDLookup[lineType](line, i);
    count += hits;
    S += hits*val;
    SS += hits*hits*val*val;
  }
  if (!count) {
    nrrdDStore[ansType](ans, AIR_NAN);
    return;
  }
  S /= count;
  SS /= count;
  nrrdDStore[ansType](ans, SS - S*S);
}

void
_nrrdMeasureHistoProduct(void *line, int lineType, int len, 
			 float axmin, float axmax,
			 void *ans, int ansType) {
  double val, product, count, hits;
  int i;
  
  if (!(AIR_EXISTS(axmin) && AIR_EXISTS(axmax))) {
    axmin = -0.5;
    axmax = len-0.5;
  }
  product = 1.0;
  for (i=0; i<=len-1; i++) {
    val = NRRD_AXIS_POS(nrrdCenterCell, axmin, axmax, len, i);
    hits = nrrdDLookup[lineType](line, i);
    count += hits;
    product *= pow(val, hits);
  }
  if (!count) {
    nrrdDStore[ansType](ans, AIR_NAN);
    return;
  }
  nrrdDStore[ansType](ans, product);
}

void
_nrrdMeasureHistoSum(void *line, int lineType, int len, 
		     float axmin, float axmax,
		     void *ans, int ansType) {
  double sum, count, hits, val;
  int i;
  
  if (!(AIR_EXISTS(axmin) && AIR_EXISTS(axmax))) {
    axmin = -0.5;
    axmax = len-0.5;
  }
  sum = count = 0;
  for (i=0; i<=len-1; i++) {
    val = NRRD_AXIS_POS(nrrdCenterCell, axmin, axmax, len, i);
    hits = nrrdDLookup[lineType](line, i);
    count += hits;
    sum += hits*val;
  }
  if (!count) {
    nrrdDStore[ansType](ans, AIR_NAN);
    return;
  }
  nrrdDStore[ansType](ans, sum);
}

void
_nrrdMeasureHistoMax(void *line, int lineType, int len, 
		     float axmin, float axmax,
		     void *ans, int ansType) {
  int i;
  double val;

  if (!(AIR_EXISTS(axmin) && AIR_EXISTS(axmax))) {
    axmin = -0.5;
    axmax = len-0.5;
  }
  for (i=len-1; i>=0; i--) {
    if (nrrdDLookup[lineType](line, i))
      break;
  }
  if (i==-1) {
    nrrdDStore[ansType](ans, AIR_NAN);
    return;
  }
  val = NRRD_AXIS_POS(nrrdCenterCell, axmin, axmax, len, i);
  nrrdDStore[ansType](ans, val);
}

void
_nrrdMeasureHistoMin(void *line, int lineType, int len, 
		     float axmin, float axmax,
		     void *ans, int ansType) {
  int i;
  double val;

  if (!(AIR_EXISTS(axmin) && AIR_EXISTS(axmax))) {
    axmin = 0;
    axmax = len-1;
  }
  for (i=0; i<=len-1; i++) {
    if (nrrdDLookup[lineType](line, i))
      break;
  }
  if (i==len) {
    nrrdDStore[ansType](ans, AIR_NAN);
    return;
  }
  val = NRRD_AXIS_POS(nrrdCenterCell, axmin, axmax, len, i);
  nrrdDStore[ansType](ans, val);
}

void (*_nrrdMeasureAxis[NRRD_MEASURE_MAX+1])(void *, int, int, 
					     float, float, 
					     void *, int) = {
  _nrrdMeasureUnknown,
  _nrrdMeasureMin,
  _nrrdMeasureMax,
  _nrrdMeasureMean,
  _nrrdMeasureMedian,
  _nrrdMeasureMode,
  _nrrdMeasureProduct,
  _nrrdMeasureSum,
  _nrrdMeasureL1,
  _nrrdMeasureL2,
  _nrrdMeasureLinf,
  _nrrdMeasureVariance,
  _nrrdMeasureHistoMin,
  _nrrdMeasureHistoMax,
  _nrrdMeasureHistoMean,
  _nrrdMeasureHistoMedian,
  _nrrdMeasureHistoMode,
  _nrrdMeasureHistoProduct,
  _nrrdMeasureHistoSum,
  _nrrdMeasureHistoVariance
};

int
_nrrdMeasureType(Nrrd *nin, int measr) {
  int type=nrrdTypeUnknown;

  switch(measr) {
  case nrrdMeasureMin:
  case nrrdMeasureMax:
  case nrrdMeasureMedian:
  case nrrdMeasureMode:
    type = nin->type;
    break;
  case nrrdMeasureMean:
    /* the rational for this is that if you're after the average value
       along a scanline, you probably want it in the same format as
       what you started with, and if you really want an exact answer
       than you can always use nrrdMeasureSum and then divide.  This may
       well be bone-headed, so is subject to change */
    type = nin->type;
    break;
  case nrrdMeasureProduct:
  case nrrdMeasureSum:
  case nrrdMeasureL1:
  case nrrdMeasureL2:
  case nrrdMeasureLinf:
  case nrrdMeasureVariance:
    type = nrrdStateMeasureType;
    break;
  case nrrdMeasureHistoMin:
  case nrrdMeasureHistoMax:
  case nrrdMeasureHistoProduct:
  case nrrdMeasureHistoSum:
  case nrrdMeasureHistoMean:
  case nrrdMeasureHistoMedian:
  case nrrdMeasureHistoMode:
  case nrrdMeasureHistoVariance:
    /* We (currently) don't keep track of the type of the original
       values which generated the histogram, and we may not even
       have access to that information.  So we end up choosing one
       type for all these histogram-based measures */
    type = nrrdStateMeasureHistoType;
    break;
  }

  return type;
}

int
nrrdProject(Nrrd *nout, Nrrd *nin, int axis, int measr) {
  char me[] = "nrrdProject", err[NRRD_STRLEN_MED];
  int type;
  int i, j, length, numperiod, lambda, period, inElSize, outElSize, 
    map[NRRD_DIM_MAX], size[NRRD_DIM_MAX];
  char *line, *src, *dest, *lineSrc, *lineDest;
  float axmin, axmax;
  
  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == nin->type) {
    sprintf(err, "%s: can't project nrrd type %s", me,
	    nrrdEnumValToStr(nrrdEnumType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_BETWEEN(nrrdMeasureUnknown, measr, nrrdMeasureLast)) {
    sprintf(err, "%s: measure %d not recognized", me, measr);
    biffAdd(NRRD, err); return 1;
  }
  if (!(AIR_INSIDE(0, axis, nin->dim-1))) {
    sprintf(err, "%s: axis %d not in range [0,%d]", me, axis, nin->dim-1);
    biffAdd(NRRD, err); return 1;
  }
  
  type = _nrrdMeasureType(nin, measr);

  /* set up control variables */
  length = numperiod = 1;
  for (i=0; i<=nin->dim-1; i++) {
    if (i < axis) {
      length *= nin->axis[i].size;
    }
    else if (i > axis) {
      numperiod *= nin->axis[i].size;
    }
  }
  lambda = length*nin->axis[axis].size;
  
  /* allocate space if necessary */
  nrrdAxesGet_nva(nin, nrrdAxesInfoSize, size);
  for (i=0; i<=nout->dim-1; i++) {
    size[i] = size[i + (i >= axis)];
  }
  if (nrrdMaybeAlloc_nva(nout, type, nin->dim-1, size)) {
    sprintf(err, "%s: failed to create output", me);
    biffAdd(NRRD, err); return 1;
  }

  /* allocate a scanline buffer */
  /* using nrrdTypeSize[] instead of nrrdElementSize() here is 
     justified by the above prohibition on using the block type */
  inElSize = nrrdTypeSize[nin->type];
  outElSize = nrrdTypeSize[type];
  if (!(line = calloc(nin->axis[axis].size, inElSize))) {
    sprintf(err, "%s: couldn't calloc(%d,%d) scanline buffer",
	    me, nin->axis[axis].size, inElSize);
    biffAdd(NRRD, err); return 1;
  }

  /* the skinny */
  src = nin->data;
  dest = nout->data;
  axmin = nin->axis[axis].min;
  axmax = nin->axis[axis].max;
  for (period=0; period<=numperiod-1; period++) {
    /* printf("%s: period = %d of %d\n", me, period, numperiod-1); */
    /* within each period of the square wave, we traverse the successive
       elements, copy the scanline starting at each element, and perform
       the measurement */
    for (j=0; j<=length-1; j++) {
      for (i=0; i<=nin->axis[axis].size-1; i++) {
	lineDest = line + inElSize*i;
	lineSrc = src + inElSize*(period*lambda + length*i + j);
	memcpy(lineDest, lineSrc, inElSize);
      }
      _nrrdMeasureAxis[measr](line, nin->type, nin->axis[axis].size, 
			      axmin, axmax,
			      dest, nout->type);
      dest += outElSize;
    }
  }
  
  /* copy the peripheral information */
  for (i=0; i<=nout->dim-1; i++) {
    map[i] = i + (i >= axis);
  }
  if (nrrdAxesCopy(nout, nin, map, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s:", me); 
    biffAdd(NRRD, err); return 1;
  }
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("measure(,,)")
			   + strlen(nin->content)
			   + 11
			   + strlen(nrrdEnumValToStr(nrrdEnumMeasure, measr))
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "measure(%s,%d,%s)", 
	      nin->content, axis,
	      nrrdEnumValToStr(nrrdEnumMeasure, measr));
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
  }

  line = airFree(line);
  return 0;
}
