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

void
_baneIncInitUnknown(Nrrd *n, double val) {
  char me[]="_baneIncInitUnknown";

  fprintf(stderr, "%s: Need To Specify An Inclusion Method !!!\n", me);
}

void
_baneIncInitMinMax(Nrrd *n, double val) {

  if (AIR_EXISTS(n->axis[0].min))
    n->axis[0].min = AIR_MIN(n->axis[0].min, val);
  else
    n->axis[0].min = val;
  if (AIR_EXISTS(n->axis[0].max))
    n->axis[0].max = AIR_MAX(n->axis[0].max, val);
  else
    n->axis[0].max = val;
}

void
_baneIncInitHisto(Nrrd *n, double val) {
  int idx;
  
  AIR_INDEX(n->axis[0].min, val, n->axis[0].max, n->axis[0].size, idx);
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
  
  nrrdAlloc(nhist=nrrdNew(), nrrdTypeInt, 1, (int)(parm[0]));
  return nhist;
}

Nrrd *
_baneIncStdvNrrd(double *parm) {
  Nrrd *nhist;
  
  nrrdAlloc(nhist=nrrdNew(), nrrdTypeInt, 1, (int)(parm[0]));
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
  baneRange[range](minP, maxP, n->axis[0].min, n->axis[0].max);
  /*
  printf("_baneIncRangeRatio: [%g,%g] -> [%g,%g]\n",
	 n->axis[0].min, n->axis[0].max, *minP, *maxP);
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
  nrrdBigInt sum, out, outsum;
  
  sum = 0;
  hist = n->data;
  for (i=0; i<=n->axis[0].size-1; i++) {
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
  top = n->axis[0].size-1;
  outsum = 0;
  for (i=0; i<=n->axis[0].size-1; i++) {
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
		   AIR_AFFINE(0, bot, n->axis[0].size-1, 
			      n->axis[0].min, n->axis[0].max),
		   AIR_AFFINE(0, top, n->axis[0].size-1, 
			      n->axis[0].min, n->axis[0].max));
}

void
_baneIncStdv(double *minP, double *maxP, 
	     Nrrd *n, double *parm, int range) {
  int *hist, i;
  double val, mean, stdv;
  nrrdBigInt sum;
  
  hist = n->data;
  sum = 0;
  mean = 0;
  for (i=0; i<=n->axis[0].size-1; i++) {
    val = AIR_AFFINE(0, i, n->axis[0].size-1, n->axis[0].min, n->axis[0].max);
    sum += hist[i];
    mean += val*hist[i];
  }
  mean /= sum;
  /* printf("%s: HEY sum = %d, mean = %g\n", "_baneIncStdv",(int)sum,mean); */
  stdv = 0;
  for (i=0; i<=n->axis[0].size-1; i++) {
    val = AIR_AFFINE(0, i, n->axis[0].size-1, n->axis[0].min, n->axis[0].max);
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
    *minP = *maxP = AIR_NAN;
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
