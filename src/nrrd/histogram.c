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
#include <math.h>

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
  if (nrrdRange(&nin->min, &nin->max, nin)) {
    sprintf(err, "%s: couldn't find value range", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(nout->data)) {
    if (nrrdAlloc(nout, (nin->num/nin->size[axis])*bins, 
		  nrrdTypeUChar, nin->dim)) {
      sprintf(err, "%s: failed to alloc output nrrd", me);
      biffAdd(NRRD, err); return 1;
    }
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
  char err[NRRD_MED_STRLEN], me[] = "nrrdHisto";
  int idx, *hist;
  NRRD_BIG_INT I;
  float min, max, val;
  char *data;

  if (!(nin && nout && bins > 0)) {
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  if (!(nout->data)) {
    if (nrrdAlloc(nout, bins, nrrdTypeUInt, 1)) {
      sprintf(err, "%s: failed to alloc histo array (len %d)", me, bins);
      biffAdd(NRRD, err); return 1;
    }
  }
  nout->size[0] = bins;
  hist = nout->data;
  data = nin->data;

  /* we only learn the range from the data if min or max is NaN */
  if (!AIR_EXISTS(nin->min) || !AIR_EXISTS(nin->max)) {
    if (nrrdRange(&nin->min, &nin->max, nin)) {
      sprintf(err, "%s: couldn't determine value range", me);
      biffSet(NRRD, err); return 1;
    }
  }
  min = nin->min;
  max = nin->max;
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
nrrdMultiHisto(Nrrd *nout, Nrrd **nin, 
	       int num, int *bin, 
	       float *min, float *max, int *clamp) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdMultiHisto";
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
  if (!(nout->data)) {
    if (nrrdAlloc(nout, size, nrrdTypeInt, num)) {
      sprintf("%s: couldn't allocate multi-dimensional histogram", me);
      biffAdd(NRRD, err); return 1;
    }
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
nrrdDrawHisto(Nrrd *nout, Nrrd *nin, int sy) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdDrawHisto", cmt[NRRD_MED_STRLEN];
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
  if (!(nout->data)) {
    if (nrrdAlloc(nout, nin->size[0]*sy, nrrdTypeUChar, 2)) {
      sprintf(err, "%s: nrrdAlloc() failed to allocate histogram image", me);
      biffAdd(NRRD, err); return 1;
    }
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
  sprintf(cmt, "min value: %g\n", nin->axisMin[0]);
  nrrdAddComment(nout, cmt);
  sprintf(cmt, "max value: %g\n", nin->axisMax[0]);
  nrrdAddComment(nout, cmt);
  sprintf(cmt, "max hits: %d, around value %g\n", maxhits, 
	  AIR_AFFINE(0, maxhitidx, sx-1, nin->axisMin[0], nin->axisMax[0]));
  nrrdAddComment(nout, cmt);
  free(Y);
  free(logY);
  free(ticks);
  return 0;
}

/*
** _nrrdHistoEqCompare
**
** used by nrrdHistoEq in smart mode to sort the "steady" array
** in _descending_ order
*/
int 
_nrrdHistoEqCompare(const void *a, const void *b) {

  return(*((int*)b) - *((int*)a));
}

/*
******** nrrdHistoEq
**
** performs histogram equalization on given nrrd, treating it as a
** big one-dimensional array.  The procedure is as follows: 
** - create a histogram of nrrd (using "bins" bins)
** - integrate the histogram, and normalize and shift this so it is 
**   a monotonically increasing function from min to max, where
**   (min,max) is the range of values in the nrrd
** - map the values in the nrrd through the adjusted histogram integral
** 
** If the histogram of the given nrrd is already as flat as can be,
** the histogram integral will increase linearly, and the adjusted
** histogram integral should be close to the identity function, so
** the values shouldn't change much.
**
** If the nhistP arg is non-NULL, then it is set to point to
** the histogram that was used for calculation. Otherwise this
** histogram is deleted on return.
**
** This is all that is done normally, when "smart" is <= 0.  In
** "smart" mode (activated by setting "smart" to something greater
** than 0), the histogram is analyzed during its creation to detect if
** there are a few bins which keep getting hit with the same value
** over and over.  It may be desirable to ignore these bins in the
** histogram integral because they may not contain any useful
** information, and so they should not effect how values are
** re-mapped.  The value of "smart" is the number of bins that will be
** ignored.  For instance, use the value 1 if the problem with naive
** histogram equalization is a large amount of background (which is
** exactly one fixed value).  
*/
int
nrrdHistoEq(Nrrd *nin, Nrrd **nhistP, int bins, int smart) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdHistoEq";
  Nrrd *nhist;
  double val, min, max, *xcoord = NULL, *ycoord = NULL, *last = NULL;
  int i, idx, *respect = NULL, *steady = NULL;
  unsigned int *hist;
  NRRD_BIG_INT I;

  if (!nin) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (!(bins > 2)) {
    sprintf(err, "%s: need # bins > 2 (not %d)", me, bins);
    biffSet(NRRD, err); return 1;
  }
  if (smart <= 0) {
    nhist = nrrdNew();
    if (nrrdHisto(nhist, nin, bins)) {
      sprintf(err, "%s: failed to create histogram", me);
      biffAdd(NRRD, err); return 1;
    }
    hist = nhist->data;
    min = nhist->axisMin[0];
    max = nhist->axisMax[0];
  }
  else {
    /* for "smart" mode, we have to some extra work in creating
       the histogram to look for bins always hit with the same value */
    if (!(nhist = nrrdNewAlloc(bins, nrrdTypeUInt, 1))) {
      sprintf(err, "%s: failed to allocate histogram", me);
      biffAdd(NRRD, err); return 1;
    }
    hist = nhist->data;
    nhist->size[0] = bins;
    /* allocate the respect, steady, and last arrays */
    if ( !(respect = calloc(bins, sizeof(int))) ||
	 !(steady = calloc(bins*2, sizeof(int))) ||
	 !(last = calloc(bins, sizeof(double))) ) {
      sprintf(err, "%s: couldn't allocate smart arrays", me);
      biffSet(NRRD, err); return 1;
    }
    for (i=0; i<=bins-1; i++) {
      last[i] = airNand();
      respect[i] = 1;
      steady[1 + 2*i] = i;
    }
    /* now create the histogram */
    if (nrrdRange(&nin->min, &nin->max, nin)) {
      sprintf(err, "%s: couldn't find value range in nrrd", me);
      biffAdd(NRRD, err); return 1;
    }
    min = nin->min;
    max = nin->max;
    for (I=0; I<=nin->num-1; I++) {
      val = nrrdDLookup[nin->type](nin->data, I);
      if (AIR_EXISTS(val)) {
	AIR_INDEX(min, val, max, bins, idx);
	/*
	if (!AIR_INSIDE(0, idx, bins-1)) {
	  printf("%s: I=%d; val=%g, [%g,%g] ===> %d\n", 
		 me, (int)I, val, min, max, idx);
	}
	*/
	++hist[idx];
	if (AIR_EXISTS(last[idx])) {
	  steady[0 + 2*idx] = (last[idx] == val
			       ? steady[0 + 2*idx]+1
			       : 0);
	}
	last[idx] = val;
      }
    }
    /*
    for (i=0; i<=bins-1; i++) {
      printf("steady(%d) = %d\n", i, steady[0 + 2*i]);
    }
    */
    /* now sort the steady array */
    qsort(steady, bins, 2*sizeof(int), _nrrdHistoEqCompare);
    /*
    for (i=0; i<=20; i++) {
      printf("sorted steady(%d/%d) = %d\n", i, steady[1+2*i], steady[0+2*i]);
    }
    */
    /* we ignore some of the bins according to "smart" arg */
    for (i=0; i<=smart-1; i++) {
      respect[steady[1+2*i]] = 0;
    }
  }
  if (!( (xcoord = calloc(bins + 1, sizeof(double))) &&
	 (ycoord = calloc(bins + 1, sizeof(double))) )) {
    sprintf(err, "%s: failed to create xcoord, ycoord arrays", me);
    biffSet(NRRD, err); return 1;
  }

  /* integrate the histogram then normalize it */
  for (i=0; i<=bins; i++) {
    xcoord[i] = AIR_AFFINE(0, i, bins, min, max);
    if (i == 0) {
      ycoord[i] = 0;
    }
    else {
      ycoord[i] = ycoord[i-1] + hist[i-1]*(smart 
					   ? respect[i-1] 
					   : 1);
    }
  }
  for (i=0; i<=bins; i++) {
    ycoord[i] = AIR_AFFINE(0, ycoord[i], ycoord[bins], min, max);
  }

  /* map the nrrd values through the normalized histogram integral */
  for (i=0; i<=nin->num-1; i++) {
    val = nrrdDLookup[nin->type](nin->data, i);
    AIR_INDEX(min, val, max, bins, idx);
    val = AIR_AFFINE(xcoord[idx], val, xcoord[idx+1], 
		      ycoord[idx], ycoord[idx+1]);
    nrrdDInsert[nin->type](nin->data, i, val);
  }
  
  /* if user is interested, set pointer to histogram nrrd,
     otherwise destroy it */
  if (nhistP) {
    *nhistP = nhist;
  }
  else {
    nrrdNuke(nhist);
  }
  
  /* clean up, bye */
  if (xcoord)
    free(xcoord);
  if (ycoord)
    free(ycoord);
  if (respect)
    free(respect);
  if (steady)
    free(steady);
  if (last)
    free(last);
  return(0);
}
