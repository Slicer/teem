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

int
_median(unsigned char *hist, int half) {
  int sum = 0;
  unsigned char *hpt;
  
  hpt = hist;
  
  while(sum < half)
    sum += *hpt++;
  
  return(hpt - 1 - hist);
}

int
_index(Nrrd *nin, nrrdBigInt I, int bins) {
  double val;
  int idx;
  
  val = nrrdDLookup[nin->type](nin->data, I);
  AIR_INDEX(nin->min, val, nin->max, bins, idx);
  return(idx);
}

void
_printhist(unsigned char *hist, int bins, char *desc) {
  int i;

  printf("%s:\n", desc);
  for (i=0; i<=bins-1; i++) {
    if (hist[i]) {
      printf("   %d: %d\n", i, hist[i]);
    }
  }
}

void
_nrrdCheapMedian1D(Nrrd *nout, Nrrd *nin, int radius, 
	      int bins, unsigned char *hist) {
  /* char me[] = "_nrrdCheapMedian1D"; */
  nrrdBigInt X, num;
  int idx, diam, half;
  double val;

  diam = 2*radius + 1;
  half = diam/2 + 1;
  /* initialize histogram */
  memset(hist, 0, bins*sizeof(unsigned char));
  for (X=0; X<=diam-1; X++) {
    hist[_index(nin, X, bins)]++;
  }
  /* _printhist(hist, bins, "after init"); */
  /* find median at each point using existing histogram */
  num = nrrdElementNumber(nin);
  for (X=radius; X<=num-radius-1; X++) {
    idx = _median(hist, half);
    val = AIR_AFFINE(0, idx, bins-1, nin->min, nin->max);
    nrrdDInsert[nout->type](nout->data, X, val);
    /* probably update histogram for next iteration */
    if (X < num-radius-1) {
      hist[_index(nin, X+radius+1, bins)]++;
      hist[_index(nin, X-radius, bins)]--;
    }
  }
}

void
_nrrdCheapMedian2D(Nrrd *nout, Nrrd *nin, int radius, 
	      int bins, unsigned char *hist) {
  /* char me[] = "_nrrdCheapMedian2D"; */
  nrrdBigInt X, Y, I, J;
  int sx, sy, idx, diam, half;
  double val;

  diam = 2*radius + 1;
  half = diam*diam/2 + 1;
  sx = nin->axis[0].size;
  sy = nin->axis[1].size;
  for (Y=radius; Y<=sy-radius-1; Y++) {
    /* initialize histogram */
    memset(hist, 0, bins*sizeof(unsigned char));
    for (J=-radius; J<=radius; J++) {
      for (I=0; I<=diam-1; I++) {
	hist[_index(nin, I + sx*(J+Y), bins)]++;
      }
    }
    /* find median at each point using existing histogram */
    for (X=radius; X<=sx-radius-1; X++) {
      idx = _median(hist, half);
      val = AIR_AFFINE(0, idx, bins-1, nin->min, nin->max);
      nrrdDInsert[nout->type](nout->data, X + sx*Y, val);
      /* probably update histogram for next iteration */
      if (X < sx-radius-1) {
	for (J=-radius; J<=radius; J++) {
	  hist[_index(nin, X+radius+1 + sx*(J+Y), bins)]++;
	  hist[_index(nin, X-radius + sx*(J+Y), bins)]--;
	}
      }
    }
  }
}

void
_nrrdCheapMedian3D(Nrrd *nout, Nrrd *nin, int radius, 
	      int bins, unsigned char *hist) {
  /* char me[] = "_nrrdCheapMedian3D"; */
  nrrdBigInt X, Y, Z, I, J, K;
  int sx, sy, sz, idx, diam, half;
  double val;

  diam = 2*radius + 1;
  half = diam*diam*diam/2 + 1;
  sx = nin->axis[0].size;
  sy = nin->axis[1].size;
  sz = nin->axis[2].size;
  for (Z=radius; Z<=sz-radius-1; Z++) {
    for (Y=radius; Y<=sy-radius-1; Y++) {
      /* initialize histogram */
      memset(hist, 0, bins*sizeof(unsigned char));
      for (K=-radius; K<=radius; K++) {
	for (J=-radius; J<=radius; J++) {
	  for (I=0; I<=diam-1; I++) {
	    hist[_index(nin, I + sx*(J+Y + sy*(K+Z)), bins)]++;
	  }
	}
      }
      /* find median at each point using existing histogram */
      for (X=radius; X<=sx-radius-1; X++) {
	idx = _median(hist, half);
	val = AIR_AFFINE(0, idx, bins-1, nin->min, nin->max);
	nrrdDInsert[nout->type](nout->data, X + sx*(Y + sy*Z), val);
	/* probably update histogram for next iteration */
	if (X < sx-radius-1) {
	  for (K=-radius; K<=radius; K++) {
	    for (J=-radius; J<=radius; J++) {
	      hist[_index(nin, X+radius+1 + sx*(J+Y + sy*(K+Z)), bins)]++;
	      hist[_index(nin, X-radius + sx*(J+Y + sy*(K+Z)), bins)]--;
	    }
	  }
	}
      }
    }
  }
}


int
nrrdCheapMedian(Nrrd *nout, Nrrd *nin, int radius, int bins) {
  char err[NRRD_STRLEN_MED], me[] = "nrrdCheapMedian";
  unsigned char *hist;

  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(radius >= 1)) {
    sprintf(err, "%s: need radius >= 1 (got %d)", me, radius);
    biffAdd(NRRD, err); return 1;
  }
  if (!(bins >= 1)) {
    sprintf(err, "%s: need bins >= 1 (got %d)", me, bins);
    biffAdd(NRRD, err); return 1;
  }
  if (!(AIR_INSIDE(1, nin->dim, 3))) {
    sprintf(err, "%s: sorry, can only handle dim 1, 2, 3 (not %d)", 
	    me, nin->dim);
    biffAdd(NRRD, err); return 1;    
  }
  if (nrrdCopy(nout, nin)) {
    sprintf(err, "%s: failed to create copy of input", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(AIR_EXISTS(nin->min) && AIR_EXISTS(nin->max))) {
    if (nrrdSetMinMax(nin)) {
      sprintf(err, "%s: couldn't learn value range", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  if (!(hist = calloc(bins, sizeof(unsigned char)))) {
    sprintf(err, "%s: couldn't allocate histogram (%d bins)", me, bins);
    biffAdd(NRRD, err); return 1;
  }
  switch (nin->dim) {
  case 1:
    _nrrdCheapMedian1D(nout, nin, radius, bins, hist);
    break;
  case 2:
    _nrrdCheapMedian2D(nout, nin, radius, bins, hist);
    break;
  case 3:
    _nrrdCheapMedian3D(nout, nin, radius, bins, hist);
    break;
  default:
    sprintf(err, "%s: can't handle dimensions %d", me, nin->dim);
    biffAdd(NRRD, err); return 1;
  }

  nrrdAxesCopy(nout, nin, NULL, NRRD_AXESINFO_NONE);
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("cheapmedian(,,)")
			   + strlen(nin->content)
			   + 11
			   + 11
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "cheapmedian(%s,%d,%d)", 
	      nin->content, radius, bins);
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
  }

  hist = airFree(hist);
  return 0;
}

