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
#include "private.h"

/*
******** nrrdConvert()
**
** copies values from one type of nrrd to another, without any
** transformation, except what you get with a cast.  The point is to
** make available on Nrrds the exact same behavior as you have in C
** with casts and assignments.
*/
int
nrrdConvert(Nrrd *nout, Nrrd *nin, int type) {
  char me[] = "nrrdConvert", err[AIR_STRLEN_MED];
  int size[NRRD_DIM_MAX];
  nrrdBigInt num;

  if (!( nin && nout 
	 && airEnumValidVal(nrrdType, nin->type)
	 && airEnumValidVal(nrrdType, type) )) {
    sprintf(err, "%s: invalid args", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nin->type == nrrdTypeBlock || type == nrrdTypeBlock) {
    sprintf(err, "%s: can't convert to or from nrrd type %s", me,
	    airEnumStr(nrrdType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }

  /* if we're actually converting to the same type, just do a copy */
  if (type == nin->type) {
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s: couldn't copy input to output", me);
      biffAdd(NRRD, err); return 1;
    }
    return 0;
  }

  /* allocate space if necessary */
  nrrdAxesGet_nva(nin, nrrdAxesInfoSize, size);
  if (nrrdMaybeAlloc_nva(nout, type, nin->dim, size)) {
    sprintf(err, "%s: failed to allocate output", me);
    biffAdd(NRRD, err); return 1;
  }

  /* call the appropriate converter */
  num = nrrdElementNumber(nin);
  _nrrdConv[nout->type][nin->type](nout->data, nin->data, num);

  /* copy peripheral information */
  nrrdAxesCopy(nout, nin, NULL, NRRD_AXESINFO_NONE);
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("()()")
			   + strlen(airEnumStr(nrrdType, nout->type))
			   + strlen(nin->content)
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "(%s)(%s)",
	      airEnumStr(nrrdType, nout->type),
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

/*
******** nrrdSetMinMax()
**
** Sets nrrd->min and nrrd->max to the extremal (existant) values in
** the given nrrd, by calling the appropriate member of nrrdFindMinMax[]
**
** calling this function will result in nrrd->hasNonExist being set
** (because of the nrrdFindMinMax[] functions)
*/
int
nrrdSetMinMax(Nrrd *nrrd) {
  char me[] = "nrrdSetMinMax", err[AIR_STRLEN_MED];
  NRRD_TYPE_BIGGEST _min, _max;

  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!airEnumValidVal(nrrdType, nrrd->type)) {
    sprintf(err, "%s: input nrrd has invalid type (%d)", me, nrrd->type);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == nrrd->type) {
    sprintf(err, "%s: don't know how to find range for nrrd type %s", me,
	    airEnumStr(nrrdType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }
  /* else should be one of the scalar types */
  nrrdFindMinMax[nrrd->type](&_min, &_max, nrrd);
  nrrd->min = nrrdDLoad[nrrd->type](&_min);
  nrrd->max = nrrdDLoad[nrrd->type](&_max);
  return 0;
}

/*
** nrrdCleverMinMax()
**
** basically a wrapper around nrrdSetMinMax(), with bells + whistles:
** 1) will call nrrdSetMinMax only when one of nrrd->min and nrrd->max
**    are non-existent, with the end result that only the non-existent
**    values are over-written
** 2) obeys the nrrdStateClever8BitMinMax global state to short-cut
**    finding min and max for 8-bit data.
** 3) reports error if there are no existent values in nrrd (AIR_EXISTS()
**    fails on every value)
**
** Like nrrdSetMinMax(), this will always set nrrd->hasNonExist.
**
** Uses biff.
*/
int
nrrdCleverMinMax(Nrrd *nrrd) {
  char me[]="nrrdCleverMinMax", err[AIR_STRLEN_MED];
  double min, max;

  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == nrrd->type) {
    sprintf(err, "%s: can't find min/max of type %s", me,
	    airEnumStr(nrrdType, nrrdTypeBlock));
  }
  if (AIR_EXISTS(nrrd->min) && AIR_EXISTS(nrrd->max)) {
    /* both of min and max already set, so we won't look for those, but
       we have to comply with stated behavior of always setting hasNonExist */
    nrrdHasNonExist(nrrd);
    return 0;
  }
  if (nrrdStateClever8BitMinMax
      && (nrrdTypeChar == nrrd->type || nrrdTypeUChar == nrrd->type)) {
    if (nrrdTypeChar == nrrd->type) {
      nrrd->min = SCHAR_MIN;
      nrrd->max = SCHAR_MAX;
    }
    else {
      nrrd->min = 0;
      nrrd->max = UCHAR_MAX;
    }
    nrrdHasNonExist(nrrd);
    return 0;
  }

  /* at this point we need to find either min and/or max (at least
     one of them was non-existent on the way in) */

  /* save incoming values in case they exist */
  min = nrrd->min;
  max = nrrd->max;
  /* this will set hasNonExist */
  if (nrrdSetMinMax(nrrd)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!( AIR_EXISTS(nrrd->min) && AIR_EXISTS(nrrd->max) )) {
    sprintf(err, "%s: no existent values!", me);
    biffAdd(NRRD, err); return 1;
  }
  /* re-enstate the existent incoming min and/or max values */
  if (AIR_EXISTS(min))
    nrrd->min = min;
  if (AIR_EXISTS(max))
    nrrd->max = max;

  return 0;
}

/*
******** nrrdQuantize()
**
** convert values to 8, 16, or 32 bit unsigned quantities
** by mapping the value range delimited by the nrrd's min
** and max to the representable range 
**
** NOTE: for the time being, this uses a "double" as the intermediate
** value holder, which may mean needless loss of precision
*/
int
nrrdQuantize(Nrrd *nout, Nrrd *nin, int bits) {
  char me[] = "nrrdQuantize", err[AIR_STRLEN_MED];
  double valIn, min, max, eps;
  int valOut, type=nrrdTypeUnknown, size[NRRD_DIM_MAX];
  unsigned long long int valOutll;
  nrrdBigInt I, num;

  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(8 == bits || 16 == bits || 32 == bits)) {
    sprintf(err, "%s: bits has to be 8, 16, or 32 (not %d)", me, bits);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == nin->type) {
    sprintf(err, "%s: can't quantize type %s", me,
	    airEnumStr(nrrdType, nrrdTypeBlock));
  }
  if (nrrdCleverMinMax(nin)) {
    sprintf(err, "%s: trouble setting min, max", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nin->hasNonExist) {
    sprintf(err, "%s: can't quantize non-existent values (NaN, +/-inf)", me);
    biffAdd(NRRD, err); return 1;
  }

  /* determine nrrd type from number of bits */
  switch (bits) {
  case 8:  type = nrrdTypeUChar;  break;
  case 16: type = nrrdTypeUShort; break;
  case 32: type = nrrdTypeUInt;   break;
  }
  
  /* allocate space if necessary */
  nrrdAxesGet_nva(nin, nrrdAxesInfoSize, size);
  if (nrrdMaybeAlloc_nva(nout, type, nin->dim, size)) {
    sprintf(err, "%s: failed to create output", me);
    biffAdd(NRRD, err); return 1;
  }

  /* the skinny */
  num = nrrdElementNumber(nin);
  min = nin->min; 
  max = nin->max;
  eps = (min == max ? 1.0 : 0.0);
  for (I=0; I<=num-1; I++) {
    valIn = nrrdDLookup[nin->type](nin->data, I);
    valIn = AIR_CLAMP(min, valIn, max);
    switch (bits) {
    case 8:
      AIR_INDEX(min, valIn, max+eps, 1 << 8, valOut);
      nrrdDInsert[nrrdTypeUChar](nout->data, I, valOut);
      break;
    case 16:
      AIR_INDEX(min, valIn, max+eps, 1 << 16, valOut);
      nrrdDInsert[nrrdTypeUShort](nout->data, I, valOut);
      break;
    case 32:
      AIR_INDEX(min, valIn, max+eps, 1LLU << 32, valOutll);
      nrrdDInsert[nrrdTypeUInt](nout->data, I, valOutll);
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
  char me[]="nrrdHistoEq", err[AIR_STRLEN_MED];
  Nrrd *nhist;
  double val, min, max, *xcoord = NULL, *ycoord = NULL, *last = NULL;
  int i, idx, *respect = NULL, *steady = NULL;
  unsigned int *hist;
  nrrdBigInt I, num;
  airArray *mop;

  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == nrrd->type) {
    sprintf(err, "%s: can't histogram equalize type %s", me,
	    airEnumStr(nrrdType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }
  num = nrrdElementNumber(nrrd);
  if (!(bins > 2)) {
    sprintf(err, "%s: need # bins > 2 (not %d)", me, bins);
    biffAdd(NRRD, err); return 1;
  }
  mop = airMopInit();
  if (smart <= 0) {
    nhist = nrrdNew();
    if (nrrdHisto(nhist, nrrd, bins, nrrdTypeInt)) {
      sprintf(err, "%s: failed to create histogram", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    hist = nhist->data;
    min = nhist->axis[0].min;
    max = nhist->axis[0].max;
  }
  else {
    /* for "smart" mode, we have to some extra work in creating
       the histogram to look for bins always hit with the same value */
    if (nrrdAlloc(nhist=nrrdNew(), nrrdTypeUInt, 1, bins)) {
      sprintf(err, "%s: failed to allocate histogram", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    hist = nhist->data;
    nhist->axis[0].size = bins;
    /* allocate the respect, steady, and last arrays */
    respect = calloc(bins, sizeof(int));
    steady = calloc(bins*2, sizeof(int));
    last = calloc(bins, sizeof(double));
    airMopMem(mop, &respect, airMopAlways);
    airMopMem(mop, &steady, airMopAlways);
    airMopMem(mop, &last, airMopAlways);
    if (!(respect && steady && last)) {
      sprintf(err, "%s: couldn't allocate smart arrays", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    for (i=0; i<=bins-1; i++) {
      last[i] = AIR_NAN;
      respect[i] = 1;
      steady[1 + 2*i] = i;
    }
    /* now create the histogram */
    nrrd->min = nrrd->max = AIR_NAN;
    if (nrrdCleverMinMax(nrrd)) {
      sprintf(err, "%s: couldn't find value range in nrrd", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    min = nrrd->min;
    max = nrrd->max;
    for (I=0; I<=num-1; I++) {
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
  xcoord = calloc(bins + 1, sizeof(double));
  ycoord = calloc(bins + 1, sizeof(double));
  airMopMem(mop, &xcoord, airMopAlways);
  airMopMem(mop, &ycoord, airMopAlways);
  if (!(xcoord && ycoord)) {
    sprintf(err, "%s: failed to create xcoord, ycoord arrays", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
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
  for (i=0; i<=num-1; i++) {
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
  
  airMopOkay(mop);
  return(0);
}
