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
nrrdHistoAxis(Nrrd *nout, Nrrd *nin, int axis, int bins) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdHistoAxis";
  int coord[NRRD_MAX_DIM], hcoord[NRRD_MAX_DIM], hidx, d;
  NRRD_BIG_INT I, hI;
  float val;
  unsigned char *hdata;

  if (!(nin && nout && bins > 0)) {
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  if (!AIR_INSIDE(0, axis, nin->dim-1)) {
    sprintf(err, "%s: axis %d is not in range [0,%d]", me, axis, nin->dim-1);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdMinMaxFind(&nin->min, &nin->max, nin)) {
    sprintf(err, "%s: couldn't find value range", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdMaybeAlloc(nout, (nin->num/nin->size[axis])*bins, 
		     nrrdTypeUChar, nin->dim)) {
    sprintf(err, "%s: failed to alloc output nrrd", me);
    biffAdd(NRRD, err); return 1;
  }
  memcpy(nout->size, nin->size, nin->dim*sizeof(int));
  nout->size[axis] = bins;
  nout->axisMin[axis] = nin->min;
  nout->axisMax[axis] = nin->max;
  hdata = nout->data;

  /* Memory locality?  Cache hits?  Who? */
  memset(coord, 0, NRRD_MAX_DIM*sizeof(int));
  for (I=0; I<=nin->num-1; I++) {
    /* update coord[] to be coordinates in input nrrd */
    coord[0]++;
    for (d=0; d<=nin->dim-1; d++) {
      if (coord[d] == nin->size[d]) {
	coord[d] = 0;
	coord[d+1]++;
      }
      else {
	break;
      }
    }

    /* get input nrrd value and compute its histogram index */
    val = nrrdFLookup[nin->type](nin->data, I);
    AIR_INDEX(nin->min, val, nin->max, bins, hidx);

    /* determine coordinate in output nrrd, update bin count */
    memcpy(hcoord, coord, nin->dim*sizeof(int));
    hcoord[axis] = hidx;
    hI = 0;
    for (d=nin->dim-1; d>=0; d--) {
      hI = hcoord[d] + nout->size[d]*hI;
    }
    if (hdata[hI] < 255) {
      hdata[hI]++;
    }
  }

  /* set information in output */
  for (d=0; d<=nin->dim-1; d++) {
    if (d != axis) {
      nout->spacing[d] = nin->spacing[d];
      nout->axisMin[d] = nin->axisMin[d];
      nout->axisMax[d] = nin->axisMax[d];
      strcpy(nout->label[d], nin->label[d]);
    }
  }
  nout->spacing[axis] = 1.0;
  sprintf(nout->label[axis], "histax(%s,%d)", nin->label[axis], bins);
  return(0);
}

int
nrrdHisto(Nrrd *nout, Nrrd *nin, int bins) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdHisto", cmt[NRRD_MED_STRLEN];
  int idx, *hist;
  NRRD_BIG_INT I;
  float min, max, val;
  char *data;

  if (!(nin && nout && bins > 0)) {
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdMaybeAlloc(nout, bins, nrrdTypeUInt, 1)) {
    sprintf(err, "%s: failed to alloc histo array (len %d)", me, bins);
    biffAdd(NRRD, err); return 1;
  }
  nout->size[0] = bins;
  hist = nout->data;
  data = nin->data;

  /* we only learn the range from the data if min or max is NaN */
  if (!AIR_EXISTS(nin->min) || !AIR_EXISTS(nin->max)) {
    if (nrrdMinMaxFind(&nin->min, &nin->max, nin)) {
      sprintf(err, "%s: couldn't determine value range", me);
      biffSet(NRRD, err); return 1;
    }
  }
  min = nin->min;
  max = nin->max;
  if (max == min) {
    /* need this to insure that index generation isn't confused */
    max++;
    sprintf(cmt, "%s: artificially increasing max from %g to %g", 
	    me, min, max);
    nrrdCommentAdd(nout, cmt);
  }
  nout->axisMin[0] = min;
  nout->axisMax[0] = max;
  
  /* make histogram */
  for (I=0; I<=nin->num-1; I++) {
    val = nrrdFLookup[nin->type](data, I);
    if (AIR_EXISTS(val)) {
      AIR_INDEX(min, val, max, bins, idx);
      ++hist[idx];
    }
  }

  return 0;
}

int 
nrrdHistoMulti(Nrrd *nout, Nrrd **nin, 
	       int num, int *bin, 
	       float *min, float *max, int *clamp) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdHistoMulti";
  int i, d, size, coord[NRRD_MAX_DIM], idx, *out, skip;
  float val;

  /* error checking */
  if (!(num >= 1)) {
    sprintf("%s: need num >= 1 (not %d)", me, num);
    biffSet(NRRD, err); return 1;
  }
  if (!(nin && bin && min && max && clamp && nout)) {
    sprintf("%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (num > NRRD_MAX_DIM) {
    sprintf("%s: can only deal with up to %d nrrds (not %d)", me,
	    NRRD_MAX_DIM, num);
    biffSet(NRRD, err); return 1;
  }
  for (d=0; d<=num-1; d++) {
    if (!nin[d]) {
      sprintf("%s: input nrrd[%d] NULL", me, d);
      biffSet(NRRD, err); return 1;
    }
    if (!(bin[d] >= 1)) {
      sprintf("%s: need bin[%d] >= 1 (not %d)", me, d, bin[d]);
      biffSet(NRRD, err); return 1;
    }
    if (!(AIR_EXISTS(min[d]) && AIR_EXISTS(max[d]))) {
      sprintf("%s: must have non-NaN values for all min and max", me);
      biffSet(NRRD, err); return 1;
    }
    if (i && !nrrdSameSize(nin[0], nin[d])) {
      sprintf("%s: nin[%d] size mismatch with nin[0]", me, d);
      biffSet(NRRD, err); return 1;
    }
  }

  /* allocate output nrrd */
  size = 1;
  for (d=0; d<=num-1; d++) {
    size *= bin[d];
  }
  fprintf(stderr, "%s: size = %d\n", me, size);
  if (nrrdMaybeAlloc(nout, size, nrrdTypeInt, num)) {
    sprintf("%s: couldn't allocate multi-dimensional histogram", me);
    biffAdd(NRRD, err); return 1;
  }
  for (d=0; d<=num-1; d++) {
    nout->size[d] = bin[d];
    nout->axisMin[d] = min[d];
    nout->axisMax[d] = max[d];
    sprintf(nout->label[d], "histo(%s,%d)", nin[d]->content, bin[d]);
  }
  out = nout->data;

  /* calculate histogram */
  fprintf(stderr, "%s: will histogram %d %d-tuples\n", 
	  me, (int)(nin[0]->num), d);
  for (i=0; i<=nin[0]->num-1; i++) {
    /*
    printf("i = %d\n", i);
    */
    skip = 0;
    for (d=0; d<=num-1; d++) {
      val = nrrdFLookup[nin[d]->type](nin[d]->data, i);
      if (!AIR_INSIDE(min[d], val, max[d])) {
	if (clamp[d]) {
	  val = AIR_CLAMP(min[d], val, max[d]);
	}
	else {
	  skip = 1;
	}
      }
      AIR_INDEX(min[d], val, max[d], bin[d], coord[d]);
    }
    /*
    printf("   --> coord = %d %d %d (skip = %d)\n", 
	   coord[0], coord[1], coord[2], skip);
    */
    if (skip)
      continue;
    idx = coord[num-1];
    for (d=num-2; d>=0; d--) {
      idx = coord[d] + bin[d]*idx;
    }
    /*
    printf("   --> idx = %d\n", idx);
    */
    ++out[idx];
  }

  return 0;
}

int
nrrdHistoDraw(Nrrd *nout, Nrrd *nin, int sy) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdHistoDraw", cmt[NRRD_MED_STRLEN];
  int *hist, k, sx, x, y, maxhits, maxhitidx,
    numticks, *Y, *logY, tick, *ticks;
  unsigned char *idata;

  if (!(nin && nout && sy > 0)) {
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  if (!(1 == nin->dim && nrrdTypeUInt == nin->type)) {
    sprintf(err, "%s: given nrrd can\'t be a histogram (dim %d, type %d)", 
	    me, nin->dim, nin->type);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdMaybeAlloc(nout, nin->size[0]*sy, nrrdTypeUChar, 2)) {
    sprintf(err, "%s: failed to allocate histogram image", me);
    biffAdd(NRRD, err); return 1;
  }
  idata = nout->data;
  nout->size[0] = sx = nin->size[0];
  nout->size[1] = sy;
  hist = nin->data;
  maxhits = maxhitidx = 0;
  for (x=0; x<=sx-1; x++) {
    if (maxhits < hist[x]) {
      maxhits = hist[x];
      maxhitidx = x;
    }
  }
  numticks = log10(maxhits + 1);
  ticks = (int*)calloc(numticks, sizeof(int));
  Y = (int*)calloc(sx, sizeof(int));
  logY = (int*)calloc(sx, sizeof(int));
  for (k=0; k<=numticks-1; k++) {
    AIR_INDEX(0, log10(pow(10,k+1) + 1), log10(maxhits+1), sy, ticks[k]);
  }
  for (x=0; x<=sx-1; x++) {
    AIR_INDEX(0, hist[x], maxhits, sy, Y[x]);
    AIR_INDEX(0, log10(hist[x]+1), log10(maxhits+1), sy, logY[x]);
    /* printf("%d -> %d,%d", x, Y[x], logY[x]); */
  }
  for (y=0; y<=sy-1; y++) {
    tick = 0;
    for (k=0; k<=numticks-1; k++)
      tick |= ticks[k] == y;
    for (x=0; x<=sx-1; x++) {
      idata[x + sx*(sy-1-y)] = 
	(y >= logY[x]       /* above log curve                       */
	 ? (!tick ? 0       /*                    not on tick mark   */
	    : 255)          /*                    on tick mark       */
	 : (y >= Y[x]       /* below log curve, above normal curve   */
	    ? (!tick ? 128  /*                    not on tick mark   */
	       : 0)         /*                    on tick mark       */
	    :255            /* below log curve, below normal curve */
	    )
	 );
    }
  }
  for (k=0; k<=nin->numComments-1; k++) {
    sprintf(cmt, "(%s)", nin->comment[k]);
    nrrdCommentAdd(nout, cmt);
  }
  sprintf(cmt, "min value: %g\n", nin->axisMin[0]);
  nrrdCommentAdd(nout, cmt);
  sprintf(cmt, "max value: %g\n", nin->axisMax[0]);
  nrrdCommentAdd(nout, cmt);
  sprintf(cmt, "max hits: %d, around value %g\n", maxhits, 
	  AIR_AFFINE(0, maxhitidx, sx-1, nin->axisMin[0], nin->axisMax[0]));
  nrrdCommentAdd(nout, cmt);
  Y = airFree(Y);
  logY = airFree(logY);
  ticks = airFree(ticks);
  return 0;
}

