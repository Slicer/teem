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

int
_baneClipUnknown(Nrrd *rhv, double *parm) {
  char me[]="_baneClipUnknown", err[128];

  sprintf(err, "%s: Need To Specify a Clipping Method !!!\n", me);
  biffSet(BANE, err);
  return -1;
}

int
_baneClipAbsolute(Nrrd *rhv, double *parm) {

  return parm[0];
}

int
_baneClipPeakRatio(Nrrd *rhv, double *parm) {
  int *hits, maxhits;
  nrrdBigInt idx, num;
  
  hits = rhv->data;
  maxhits = 0;
  num = nrrdElementNumber(rhv);
  for (idx=0; idx<=num-1; idx++) {
    maxhits = AIR_MAX(maxhits, hits[idx]);
  }
  return maxhits*parm[0];
}

int
_baneClipCompare(const void *a, const void *b) {
  return((*(int*)a < *(int*)b 
	  ? -1 
	  : (*(int*)a > *(int*)b 
	     ? 1 
	     : 0)));
}

int
_baneClipPercentile(Nrrd *rhv, double *parm) {
  char me[]="_baneClipPercentile", err[128];
  Nrrd *copy;
  int *hits, clip;
  nrrdBigInt num, sum, out, outsum, hi;
  
  if (nrrdCopy(copy=nrrdNew(), rhv)) {
    sprintf(err, "%s: couldn't create copy of histovol", me);
    biffMove(BANE, err, NRRD); return -1;
  }
  hits = copy->data;
  num = nrrdElementNumber(copy);
  qsort(hits, num, sizeof(int), nrrdValCompare[nrrdTypeInt]);
  sum = 0;
  for (hi=0; hi<=num-1; hi++) {
    sum += hits[hi];
  }
  out = sum*parm[0]/100;
  outsum = 0;
  for (hi=0; hi<=num-1; hi++) {
    if (outsum >= out)
      break;
    outsum += hits[hi];
  }
  clip = hits[hi];
  nrrdNuke(copy);
  return clip;
}

int
_baneClipTopN(Nrrd *rhv, double *parm) {
  char me[]="_baneClipTopN", err[128];
  Nrrd *copy;
  int *hits, clip;
  nrrdBigInt num;

  if (nrrdCopy(copy=nrrdNew(), rhv)) {
    sprintf(err, "%s: couldn't create copy of histovol", me);
    biffMove(BANE, err, NRRD); return -1;
  }
  hits = copy->data;
  num = nrrdElementNumber(copy);
  qsort(hits, num, sizeof(int), &_baneClipCompare);
  clip = hits[(int)parm[0]];
  nrrdNuke(copy);
  return clip;
}

baneClipType
baneClip[BANE_CLIP_MAX+1] = {
  _baneClipUnknown,
  _baneClipAbsolute,
  _baneClipPeakRatio,
  _baneClipPercentile,
  _baneClipTopN
};

