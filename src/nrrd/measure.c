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


#include "nrrd.h"


/* the non-histogram measures assume that there will be no NaNs in data */

void
_nrrdMeasureUnknown(void *line, int lineType, int len, 
		    double axmin, double axmax, 
		    void *ans, int ansType) {
  char me[]="_nrrdMeasureUnknown";
  
  fprintf(stderr, "%s: Need To Specify A Measure !!! \n", me);
  nrrdDStore[ansType](ans, AIR_NAN);
}

void
_nrrdMeasureMin(void *line, int lineType, int len,
		double axmin, double axmax, 
		void *ans, int ansType) {
  double val, M;
  int i;

  if (nrrdTypeFixed[lineType]) {
    M = nrrdDLookup[lineType](line, 0);
    for (i=1; i<3; i++) {
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
		double axmin, double axmax, 
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
		    double axmin, double axmax,
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
		double axmin, double axmax,
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
		 double axmin, double axmax,
		 void *ans, int ansType) {
  double val, S, M;
  int i, count;

  if (nrrdTypeFixed[lineType]) {
    S = 0.0;
    for (i=0; i<len; i++) {
      S += nrrdDLookup[lineType](line, i);
    }
    M = S/len;
  }
  else {
    S = AIR_NAN;
    for (i=0; !AIR_EXISTS(S) && i<len; i++)
      S = nrrdDLookup[lineType](line, i);
    if (i<len) {
      /* there was an existant value */
      count = 1;
      for (; i<len; i++) {
	val = nrrdDLookup[lineType](line, i);
	if (AIR_EXISTS(val)) {
	  count++;
	  S += val;
	}
      }
      M = S/count;
    }
    else {
      /* there were NO existant values */
      M = AIR_NAN;
    }
  }
  nrrdDStore[ansType](ans, M);
}

/* stupid little forward declaration */
void
_nrrdMeasureHistoMode(void *line, int lineType, int len, 
		      double axmin, double axmax, 
		      void *ans, int ansType);

void
_nrrdMeasureMode(void *line, int lineType, int len, 
		 double axmin, double axmax,
		 void *ans, int ansType) {
  Nrrd *nline, *nhist;

  nline = nrrdNew();
  if (nrrdWrap(nline, line, lineType, 1, len)) {
    free(biffGetDone(NRRD));
    nrrdNix(nline);
    nrrdDStore[ansType](ans, AIR_NAN);
    return;
  }
  nhist = nrrdNew();
  if (nrrdHisto(nhist, nline, nrrdStateMeasureModeBins, nrrdTypeInt)) {
    free(biffGetDone(NRRD));
    nrrdNuke(nhist);
    nrrdNix(nline);
    nrrdDStore[ansType](ans, AIR_NAN);
    return;
  }

  /* now we pass this histogram off to histo-mode */
  _nrrdMeasureHistoMode(nhist->data, nrrdTypeInt, nrrdStateMeasureModeBins,
			nhist->axis[0].min, nhist->axis[0].max,
			ans, ansType);
  nrrdNuke(nhist);
  nrrdNix(nline);
  return;
}

void
_nrrdMeasureMedian(void *line, int lineType, int len, 
		   double axmin, double axmax,
		   void *ans, int ansType) {
  double M;
  int i, mid;
  
  /* yes, I know, this is not the fastest median.  I'll get to it ... */
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
      /* len is odd, there is a middle value, its at mid */
      M = nrrdDLookup[lineType](line, i+mid);
    }
    else {
      /* len is even, two middle values are at mid-1 and mid */
      M = nrrdDLookup[lineType](line, i+mid-1);
      M += nrrdDLookup[lineType](line, i+mid);
      M /= 2.0;
    }
  }
  nrrdDStore[ansType](ans, M);
}

void
_nrrdMeasureL1(void *line, int lineType, int len, 
	       double axmin, double axmax,
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
    S = AIR_ABS(S);
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
	       double axmin, double axmax,
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
    S *= S;
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
		 double axmin, double axmax,
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
		     double axmin, double axmax,
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
    if (count) {
      S /= count;
      SS /= count;
    }
    else {
      S = SS = AIR_NAN;
    }
  }
  nrrdDStore[ansType](ans, SS - S*S);
}

void
_nrrdMeasureSD(void *line, int lineType, int len, 
	       double axmin, double axmax,
	       void *ans, int ansType) {
  double var;

  _nrrdMeasureVariance(line, lineType, len, axmin, axmax, ans, ansType);
  var = nrrdDLoad[ansType](ans);
  nrrdDStore[ansType](ans, sqrt(var));
}

/*
** one thing which ALL the _nrrdMeasureHisto measures assume is that,
** being a histogram, the input array will not have any non-existant
** values.  It can be floating point, because it is plausible to have
** some histogram composed of fractionally weighted hits, but there is
** no way that it is reasonable to have NaN in a bin, and it is extremely
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
			double axmin, double axmax, 
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
		      double axmin, double axmax, 
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
		      double axmin, double axmax,
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
			  double axmin, double axmax,
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
			 double axmin, double axmax,
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
		     double axmin, double axmax,
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
		     double axmin, double axmax,
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
		     double axmin, double axmax,
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
					     double, double,
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
  _nrrdMeasureSD,
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
  case nrrdMeasureSD:
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
  char me[] = "nrrdProject", err[AIR_STRLEN_MED];
  int d, i, iType, oType, row, rowNum, col, colNum, colStep, 
    linLen, iElSz, oElSz,
    map[NRRD_DIM_MAX], iSize[NRRD_DIM_MAX], oSize[NRRD_DIM_MAX];
  char *_line, *line, *ptr, *iData, *oData;
  double axmin, axmax;
  
  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == nin->type) {
    sprintf(err, "%s: can't project nrrd type %s", me,
	    airEnumStr(nrrdType, nrrdTypeBlock));
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
  
  iType = nin->type;
  oType = _nrrdMeasureType(nin, measr);
  iElSz = nrrdTypeSize[iType];
  oElSz = nrrdTypeSize[oType];
  nrrdAxesGet_nva(nin, nrrdAxesInfoSize, iSize);
  colNum = rowNum = 1;
  for (d=0; d<=nin->dim-1; d++) {
    if (d < axis) {
      colNum *= iSize[d];
    }
    else if (d > axis) {
      rowNum *= iSize[d];
    }
  }
  linLen = iSize[axis];
  colStep = linLen*colNum;
  /*
  fprintf(stderr, "!%s: {i,o}Type = %d, %d\n", me, iType, oType);
  fprintf(stderr, "!%s: {i,o}ElSz = %d, %d\n", me, iElSz, oElSz);
  fprintf(stderr, "!%s: {col,row}Num = %d, %d\n", me, colNum, rowNum);
  fprintf(stderr, "!%s: linLen = %d\n", me, linLen);
  fprintf(stderr, "!%s: colStep = %d\n", me, colStep);
  fprintf(stderr, "!%s: iSize = %d %d %d\n", me, iSize[0], iSize[1], iSize[2]);
  fprintf(stderr, "!%s: axis = %d\n", me, axis); 
  */
  for (d=0; d<=nin->dim-2; d++) {
    map[d] = d + (d >= axis);
  }
  for (d=0; d<=nin->dim-2; d++) {
    oSize[d] = iSize[map[d]];
  }
  if (nrrdMaybeAlloc_nva(nout, oType, nin->dim-1, oSize)) {
    sprintf(err, "%s: failed to create output", me);
    biffAdd(NRRD, err); return 1;
  }

  /* allocate a scanline buffer */
  if (!(_line = calloc(linLen, iElSz))) {
    sprintf(err, "%s: couldn't calloc(%d,%d) scanline buffer",
	    me, linLen, iElSz);
    biffAdd(NRRD, err); return 1;
  }

  /* the skinny */
  axmin = nin->axis[axis].min;
  axmax = nin->axis[axis].max;
  iData = nin->data;
  oData = nout->data;
  for (row=0; row<rowNum; row++) {
    for (col=0; col<colNum; col++) {
      line = _line;
      ptr = iData + iElSz*(col + row*colStep);
      for (i=0; i<=linLen-1; i++) {
	memcpy(line, ptr, iElSz);
	ptr += iElSz*colNum;
	line += iElSz;
      }
      _nrrdMeasureAxis[measr](_line, iType, linLen,
			      axmin, axmax, oData, oType);
      oData += oElSz;
    }
  }
  
  /* copy the peripheral information */
  if (nrrdAxesCopy(nout, nin, map, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s:", me); 
    biffAdd(NRRD, err); return 1;
  }
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("measure(,,)")
			   + strlen(nin->content)
			   + 11
			   + strlen(airEnumStr(nrrdMeasure, measr))
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "measure(%s,%d,%s)", 
	      nin->content, axis,
	      airEnumStr(nrrdMeasure, measr));
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
  }

  airFree(_line);
  return 0;
}
