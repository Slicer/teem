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
#include "private.h"

/*
******** nrrdConvert()
**
** copies values from one type of nrrd to another, without
** any transformation, except what you get with a cast
*/
int
nrrdConvert(Nrrd *nout, Nrrd *nin, int type) {
  char me[] = "nrrdConvert", err[NRRD_STRLEN_MED];

  if (!( nin && nout 
	 && AIR_BETWEEN(nrrdTypeUnknown, nin->type, nrrdTypeLast)
	 && AIR_BETWEEN(nrrdTypeUnknown, type, nrrdTypeLast) )) {
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  if (nin->type == nrrdTypeBlock || type == nrrdTypeBlock) {
    sprintf(err, "%s: can't convert to or from nrrd type %s", me,
	    nrrdEnumValToStr(nrrdEnumType, nrrdTypeBlock));
    biffSet(NRRD, err); return 1;
  }

  /* if we're actually converting to the same type, just do a copy */
  if (type == nin->type) {
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s: couldn't copy input to output", me);
      biffSet(NRRD, err); return 1;
    }
    return 0;
  }

  /* allocate space if necessary */
  if (nrrdMaybeAlloc(nout, nin->num, type, nin->dim)) {
    sprintf(err, "%s: failed to create slice", me);
    biffSet(NRRD, err); return 1;
  }

  /* call the appropriate converter */
  _nrrdConv[nout->type][nin->type](nout->data, nin->data, nin->num);

  /* copy peripheral information */
  nrrdAxesCopy(nout, nin, NULL, NRRD_AXESINFO_NONE);
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("()()")
			   + strlen(nrrdEnumValToStr(nrrdEnumType, nout->type))
			   + strlen(nin->content)
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "(%s)(%s)",
	      nrrdEnumValToStr(nrrdEnumType, nout->type),
	      nin->content);
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
  }

  /* bye */
  return 0;
}

int
nrrdMinMaxFind(double *minP, double *maxP, Nrrd *nrrd) {
  char me[] = "nrrdMinMaxFind", err[NRRD_STRLEN_MED];
  char _min[NRRD_TYPE_SIZE_MAX], _max[NRRD_TYPE_SIZE_MAX];

  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdTypeUChar == nrrd->type) {
    *minP = 0;
    *maxP = UCHAR_MAX;
  }
  else if (nrrdTypeChar == nrrd->type) {
    *minP = SCHAR_MIN;
    *maxP = SCHAR_MAX;
  }
  else if (nrrdTypeChar < nrrd->type &&
	   nrrd->type < nrrdTypeBlock) {
    _nrrdMinMaxFind[nrrd->type](_min, _max, nrrd->num, nrrd->data);
    *minP = nrrdDLoad[nrrd->type](_min);
    *maxP = nrrdDLoad[nrrd->type](_max);
  }
  else {
    sprintf(err, "%s: don't know how to find range for nrrd type %s", me,
	    nrrdEnumValToStr(nrrdEnumType, nrrdTypeBlock));
    biffSet(NRRD, err); return 1;
  }
  return 0;
}  

int
nrrdMinMaxDo(double *minP, double *maxP, Nrrd *nrrd, int minmax, ...) {
  char me[]="nrrdMinMaxDo", err[NRRD_STRLEN_MED];
  int E;
  va_list ap;
  
  if (!AIR_BETWEEN(nrrdMinMaxUnknown, minmax, nrrdMinMaxLast)) {
    sprintf(err, "%s: minmax behavior %d invalid", me, minmax);
    biffSet(NRRD, err); return 1;
  }
  E = 0;
  switch (minmax) {
  case nrrdMinMaxSearch:
    E = nrrdMinMaxFind(minP, maxP, nrrd);
    break;
  case nrrdMinMaxSearchSet:
    E = nrrdMinMaxFind(&nrrd->min, &nrrd->max, nrrd);
    *minP = nrrd->min;
    *maxP = nrrd->max;
    break;
  case nrrdMinMaxUse:
    *minP = nrrd->min;
    *maxP = nrrd->max;
    break;
  case nrrdMinMaxInsteadUse:
    va_start(ap, minmax);
    *minP = va_arg(ap, double);
    *maxP = va_arg(ap, double);
    va_end(ap);
    break;
  default:
    sprintf(err, "%s: sorry, minmax behavior %d not implemented", me, minmax);
    biffSet(NRRD, err); return 1;
    break;
  }
  if (E) {
    sprintf(err, "%s: trouble finding min, max", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(AIR_EXISTS(*minP) && AIR_EXISTS(*maxP))) {
    sprintf(err, "%s: final min and max values don't both exist", me);
    biffAdd(NRRD, err); return 1;
  }
  
  return 0;
}

/*
******** nrrdQuantize
**
** convert values to 8, 16, or 32 bit unsigned quantities
** by mapping the value range delimited by the nrrd's min
** and max to the representable range 
**
** NOTE: for the time being, this uses a "double" as the intermediate
** value holder, which may mean needless loss of precision
*/
int
nrrdQuantize(Nrrd *nout, Nrrd *nin, int bits, int minmax) {
  char me[] = "nrrdQuantize", err[NRRD_STRLEN_MED];
  double valIn, min, max;
  int valOut;
  long long valOutll;
  nrrdBigInt I;
  int type;

  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (!(8 == bits || 16 == bits || 32 == bits)) {
    sprintf(err, "%s: bits has to be 8, 16, or 32 (not %d)", me, bits);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdMinMaxDo(&min, &max, nin, minmax)) {
    sprintf(err, "%s: trouble setting min, max", me);
    biffAdd(NRRD, err); return 1;
  }

  /* determine nrrd type from number of bits */
  switch (bits) {
  case 8:
    type = nrrdTypeUChar;
    break;
  case 16:
    type = nrrdTypeUShort;
    break;
  case 32:
    type = nrrdTypeUInt;
    break;
  default:
    sprintf(err, "%s: %d bits not implemented, sorry", me, bits);
    biffAdd(NRRD, err); return 1;
    break;
  }
  
  /* allocate space if necessary */
  if (nrrdMaybeAlloc(nout, nin->num, type, nin->dim)) {
    sprintf(err, "%s: failed to create slice", me);
    biffSet(NRRD, err); return 1;
  }

  /* copy the values */
  for (I=0; I<=nin->num-1; I++) {
    valIn = nrrdDLookup[nin->type](nin->data, I);
    switch (bits) {
    case 8:
    case 16:
      AIR_INDEX(min, valIn, max, 1 << bits, valOut);
      valOut = AIR_CLAMP(0, valOut, (1 << bits)-1);
      nrrdDInsert[nout->type](nout->data, I, valOut);
      break;
    case 32:
      AIR_INDEX(min, valIn, max, 1LLU << 32, valOutll);
      /* this line isn't compiling on my intel laptop */
      valOutll = AIR_CLAMP(0, valOutll, (1LLU << 32)-1);
      nrrdDInsert[nout->type](nout->data, I, valOutll);
      break;
    }
  }

  /* set information in new volume */
  nrrdAxesCopy(nout, nin, NULL, NRRD_AXESINFO_NONE);
  nout->oldMin = min;
  nout->oldMax = max;
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("quantize(,)")
			   + strlen(nin->content)
			   + 11
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "quantize(%s,%d)",
	      nin->content, bits);
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
  }

  /* bye */
  return 0;
}


/*
** _nrrdHistoEqCompare()
**
** used by nrrdHistoEq in smart mode to sort the "steady" array
** in _descending_ order
*/
int 
_nrrdHistoEqCompare(const void *a, const void *b) {

  return(*((int*)b) - *((int*)a));
}

/*
******** nrrdHistoEq()
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
nrrdHistoEq(Nrrd *nrrd, Nrrd **nhistP, int bins, int smart) {
  char me[]="nrrdHistoEq", err[NRRD_STRLEN_MED];
  Nrrd *nhist;
  double val, min, max, *xcoord = NULL, *ycoord = NULL, *last = NULL;
  int i, idx, *respect = NULL, *steady = NULL;
  unsigned int *hist;
  nrrdBigInt I;

  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (!(bins > 2)) {
    sprintf(err, "%s: need # bins > 2 (not %d)", me, bins);
    biffSet(NRRD, err); return 1;
  }
  if (smart <= 0) {
    nhist = nrrdNew();
    if (nrrdHisto(nhist, nrrd, bins)) {
      sprintf(err, "%s: failed to create histogram", me);
      biffAdd(NRRD, err); return 1;
    }
    hist = nhist->data;
    min = nhist->axis[0].min;
    max = nhist->axis[0].max;
  }
  else {
    /* for "smart" mode, we have to some extra work in creating
       the histogram to look for bins always hit with the same value */
    if (nrrdAlloc(nhist=nrrdNew(), bins, nrrdTypeUInt, 1)) {
      sprintf(err, "%s: failed to allocate histogram", me);
      biffAdd(NRRD, err); return 1;
    }
    hist = nhist->data;
    nhist->axis[0].size = bins;
    /* allocate the respect, steady, and last arrays */
    if ( !(respect = calloc(bins, sizeof(int))) ||
	 !(steady = calloc(bins*2, sizeof(int))) ||
	 !(last = calloc(bins, sizeof(double))) ) {
      sprintf(err, "%s: couldn't allocate smart arrays", me);
      biffSet(NRRD, err); return 1;
    }
    for (i=0; i<=bins-1; i++) {
      last[i] = AIR_NAN;
      respect[i] = 1;
      steady[1 + 2*i] = i;
    }
    /* now create the histogram */
    if (nrrdMinMaxFind(&nrrd->min, &nrrd->max, nrrd)) {
      sprintf(err, "%s: couldn't find value range in nrrd", me);
      biffAdd(NRRD, err); return 1;
    }
    min = nrrd->min;
    max = nrrd->max;
    for (I=0; I<=nrrd->num-1; I++) {
      val = nrrdDLookup[nrrd->type](nrrd->data, I);
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
  for (i=0; i<=nrrd->num-1; i++) {
    val = nrrdDLookup[nrrd->type](nrrd->data, i);
    if (AIR_EXISTS(val)) {
      AIR_INDEX(min, val, max, bins, idx);
      val = AIR_AFFINE(xcoord[idx], val, xcoord[idx+1], 
		       ycoord[idx], ycoord[idx+1]);
      nrrdDInsert[nrrd->type](nrrd->data, i, val);
    }
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
  xcoord = airFree(xcoord);
  ycoord = airFree(ycoord);
  respect = airFree(respect);
  steady = airFree(steady);
  last = airFree(last);
  return(0);
}
