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


#include "bane.h"

void
_baneIncInitUnknown(Nrrd *n, double val) {
  char me[]="_baneIncInitUnknown";

  fprintf(stderr, "%s: Need To Specify An Inclusion Method !!!\n", me);
}

void
_baneIncInitMinMax(Nrrd *n, double val) {

  if (AIR_EXISTS(n->axisMin[0]))
    n->axisMin[0] = AIR_MIN(n->axisMin[0], val);
  else
    n->axisMin[0] = val;
  if (AIR_EXISTS(n->axisMax[0]))
    n->axisMax[0] = AIR_MAX(n->axisMax[0], val);
  else
    n->axisMax[0] = val;
}

void
_baneIncInitHisto(Nrrd *n, double val) {
  int idx;
  
  AIR_INDEX(n->axisMin[0], val, n->axisMax[0], n->size[0], idx);
  ((int*)n->data)[idx]++;
}

void
(*baneIncInitA[BANE_INC_MAX+1])(Nrrd *n, double val) = {
  _baneIncInitUnknown,
  NULL, 
  NULL, 
  _baneIncInitMinMax,
  _baneIncInitMinMax
};

void
(*baneIncInitB[BANE_INC_MAX+1])(Nrrd *n, double val) = {
  _baneIncInitUnknown,
  NULL, 
  _baneIncInitMinMax,
  _baneIncInitHisto,
  _baneIncInitHisto
};

Nrrd *
_baneIncUnknownNrrd(double *parm) {
  char me[]="_baneIncUnknownNrrd";
  
  fprintf(stderr, "%s: Need To Specify An Inclusion Method !!!\n", me);
  return NULL;
}

Nrrd *
_baneIncAbsoluteNrrd(double *parm) {
  /* this inclusion method doesn't need a nrrd, but we return one
     anyway to signify non-error */
  return nrrdNew();
}

Nrrd *
_baneIncRangeRatioNrrd(double *parm) {

  return nrrdNew();
}

Nrrd *
_baneIncPercentileNrrd(double *parm) {
  Nrrd *nhist;
  
  if (nhist = nrrdNewAlloc(parm[0], nrrdTypeInt, 1))
    nhist->size[0] = parm[0];
  return nhist;
}

Nrrd *
_baneIncStdvNrrd(double *parm) {
  Nrrd *nhist;
  
  if (nhist = nrrdNewAlloc(parm[0], nrrdTypeInt, 1))
    nhist->size[0] = parm[0];
  return nhist;
}

Nrrd *
(*baneIncNrrd[BANE_INC_MAX+1])(double *parm) = {
  _baneIncUnknownNrrd,
  _baneIncAbsoluteNrrd,
  _baneIncRangeRatioNrrd,
  _baneIncPercentileNrrd,
  _baneIncStdvNrrd
};

void
_baneIncUnknown(double *minP, double *maxP, 
		Nrrd *n, double *parm, int range) {
  char me[]="_baneIncUnknown";

  fprintf(stderr, "%s: Need To Specify An Inclusion Method !!!\n", me);
}

/*
** _baneIncAbsolute()
**
** sets *minP to parm[0], *maxP to parm[1]
** does no messing with these via a range method- its called "absolute",
** after all
*/
void
_baneIncAbsolute(double *minP, double *maxP, 
		 Nrrd *n, double *parm, int range) {
  
  *minP = parm[0];
  *maxP = parm[1];
}

/*
** _baneIncRangeRatio
**
** uses parm[0] to scale min and max, after they've been sent through 
** the range method
*/
void
_baneIncRangeRatio(double *minP, double *maxP, 
		   Nrrd *n, double *parm, int range) {
  double mid;

  /*
  printf("_baneIncRangeRatio, minP=%lu, maxP=%lu\n", 
	 (unsigned long)minP, (unsigned long)maxP);
  */
  baneRange[range](minP, maxP, n->axisMin[0], n->axisMax[0]);
  /*
  printf("_baneIncRangeRatio: [%g,%g] -> [%g,%g]\n",
	 n->axisMin[0], n->axisMax[0], *minP, *maxP);
  */
  if (baneRangeFloat == range) {
    mid = (*minP + *maxP)/2;
    *minP = AIR_AFFINE(-1, -parm[0], 0, *minP, mid);
    *maxP = AIR_AFFINE(0, parm[0], 1, mid, *maxP);
  }
  else {
    *minP *= parm[0];
    *maxP *= parm[0];
  }
}

/*
** _baneInc
*/
void
_baneIncPercentile(double *minP, double *maxP, 
		   Nrrd *n, double *parm, int range) {
  char me[]="_baneIncPercentile";
  int *hist, i, bot, top, botIncr, topIncr;
  NRRD_BIG_INT sum, out, outsum;
  
  sum = 0;
  hist = n->data;
  for (i=0; i<=n->size[0]-1; i++) {
    sum += hist[i];
  }
  out = sum*parm[1]/100;
  switch (range) {
  case baneRangePos:
    botIncr = 0;
    topIncr = 1;
    break;
  case baneRangeNeg:
    botIncr = 1;
    topIncr = 0;
    break;
  case baneRangeZeroCent:
  case baneRangeFloat:
  default:
    botIncr = 1;
    topIncr = 1;
    break;
  }
  bot = 0;
  top = n->size[0]-1;
  outsum = 0;
  for (i=0; i<=n->size[0]-1; i++) {
    if (outsum >= out)
      break;
    outsum += botIncr*hist[bot];
    outsum += topIncr*hist[top];
    bot += botIncr;
    top -= topIncr;
    if (bot == top) {
      fprintf(stderr, "%s: WARNING: something has gone wrong !!! \n", me);
      return;
    }
  }
  baneRange[range](minP, maxP, 
		   AIR_AFFINE(0, bot, n->size[0]-1, 
			      n->axisMin[0], n->axisMax[0]),
		   AIR_AFFINE(0, top, n->size[0]-1, 
			      n->axisMin[0], n->axisMax[0]));
}

void
_baneIncStdv(double *minP, double *maxP, 
	     Nrrd *n, double *parm, int range) {
  int *hist, i;
  double val, mean, stdv;
  NRRD_BIG_INT sum;
  
  hist = n->data;
  sum = 0;
  mean = 0;
  for (i=0; i<=n->size[0]-1; i++) {
    val = AIR_AFFINE(0, i, n->size[0]-1, n->axisMin[0], n->axisMax[0]);
    sum += hist[i];
    mean += val*hist[i];
  }
  mean /= sum;
  /* printf("%s: HEY sum = %d, mean = %g\n", "_baneIncStdv",(int)sum,mean); */
  stdv = 0;
  for (i=0; i<=n->size[0]-1; i++) {
    val = AIR_AFFINE(0, i, n->size[0]-1, n->axisMin[0], n->axisMax[0]);
    stdv += (mean-val)*(mean-val);
  }
  stdv /= sum;
  /* printf("%s: HEY stdv = %g\n", "_baneIncStdv", stdv); */
  switch (range) {
  case baneRangePos:
    *minP = 0;
    *maxP = parm[1]*stdv;
    break;
  case baneRangeNeg:
    *minP = -parm[1]*stdv;
    *maxP = 0;
    break;
  case baneRangeZeroCent:
    *minP = -parm[1]*stdv/2;
    *maxP = parm[1]*stdv/2;
    break;
  case baneRangeFloat:
    *minP = mean - parm[1]*stdv/2;
    *maxP = mean + parm[1]*stdv/2;
    break;
  default:
    *minP = *maxP = airNand();
    break;
  }
  /* printf("%s: HEY min, max = %g, %g\n", "_baneIncStdv", *minP, *maxP); */
}

baneIncType
baneInc[BANE_INC_MAX+1] = {
  _baneIncUnknown,
  _baneIncAbsolute,
  _baneIncRangeRatio,
  _baneIncPercentile,
  _baneIncStdv
};
