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

#include <limits.h>

/*
******** nrrdDescribe
** 
** writes verbose description of nrrd to given file
*/
void
nrrdDescribe(FILE *file, Nrrd *nrrd) {
  int i;

  if (file && nrrd) {
    fprintf(file, "Nrrd at 0x%lx:\n", (unsigned long)nrrd);
    fprintf(file, "Data at 0x%lx is " NRRD_BIG_INT_PRINTF 
	    " elements of type %s.\n",
	    (unsigned long)nrrd->data, nrrd->num, 
	    nrrdEnumValToStr(nrrdEnumType, nrrd->type));
    if (nrrdTypeBlock == nrrd->type) 
      fprintf(file, "The blocks have size %d\n", nrrd->blockSize);
    if (airStrlen(nrrd->content))
      fprintf(file, "Content = \"%s\"\n", nrrd->content);
    fprintf(file, "%d-dimensional array, with axes:\n", nrrd->dim);
    for (i=0; i<=nrrd->dim-1; i++) {
      if (airStrlen(nrrd->axis[i].label))
	fprintf(file, "%d: (\"%s\") ", i, nrrd->axis[i].label);
      else
	fprintf(file, "%d: ", i);
      fprintf(file, "%s-centered, size=%d, ",
	      nrrdEnumValToStr(nrrdEnumCenter, nrrd->axis[i].center),
	      nrrd->axis[i].size);
      airSinglePrintf(file, NULL, "spacing=%lg, \n", nrrd->axis[i].spacing);
      airSinglePrintf(file, NULL, "    axis(Min,Max) = (%lg,",
		       nrrd->axis[i].min);
      airSinglePrintf(file, NULL, "%lg)\n", nrrd->axis[i].max);
    }
    airSinglePrintf(file, NULL, "The min, max values are %lg",
		     nrrd->min);
    airSinglePrintf(file, NULL, ", %lg\n", nrrd->max);
    airSinglePrintf(file, NULL, "The old min, old max values are %lg",
		     nrrd->oldMin);
    airSinglePrintf(file, NULL, ", %lg\n", nrrd->oldMax);
    if (nrrd->cmtArr->len) {
      fprintf(file, "Comments:\n");
      for (i=0; i<=nrrd->cmtArr->len-1; i++) {
	fprintf(file, "%s\n", nrrd->cmt[i]);
      }
    }
    fprintf(file, "\n");
  }
}

/*
******** nrrdValid()
**
** does some consistency checks for things that can go wrong in a nrrd
*/
int
nrrdValid(Nrrd *nrrd) {
  char me[] = "nrrdValid", err[NRRD_STRLEN_MED];
  nrrdBigInt mult;
  int i;

  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 0;
  }
  if (!(0 < nrrd->num)) {
    sprintf(err, "%s: number of elements is not > zero", me);
    biffSet(NRRD, err); return 0;
  }
  if (!AIR_BETWEEN(nrrdTypeUnknown, nrrd->type, nrrdTypeLast)) {
    sprintf(err, "%s: type (%d) of array is invalid", me, nrrd->type);
    biffSet(NRRD, err); return 0;
  }
  if (nrrdTypeBlock == nrrd->type && (!(0 < nrrd->blockSize)) ) {
    sprintf(err, "%s: nrrd type is %s but nrrd->blockSize (%d) invalid", me,
	    nrrdEnumValToStr(nrrdEnumType, nrrdTypeBlock),
	    nrrd->blockSize);
    biffSet(NRRD, err); return 0;
  }
  if (!AIR_INSIDE(1, nrrd->dim, NRRD_DIM_MAX)) {
    sprintf(err, "%s: dimension %d is outside valid range [1,%d]",
	    me, nrrd->dim, NRRD_DIM_MAX);
    biffSet(NRRD, err); return 0;
  }
  mult = 1;
  for (i=0; i<=nrrd->dim-1; i++) {
    if (!(1 <= nrrd->axis[i].size)) {
      sprintf(err, "%s: axis %d has invalid size (%d)", me, i,
	      nrrd->axis[i].size);
      biffSet(NRRD, err); return 0;
    }
    mult *= nrrd->axis[i].size;
  }
  if (mult != nrrd->num) {
    sprintf(err, "%s: # elements (" NRRD_BIG_INT_PRINTF
	    ") != product of axes sizes (" NRRD_BIG_INT_PRINTF ")", me,
	    nrrd->num, mult);
    biffSet(NRRD, err); return 0;
  }
  return 1;
}

/*
******** nrrdSameSize()
**
** returns 1 iff given two nrrds have same dimension and axes sizes.
** This does NOT look at the type of the elements.
*/
int
nrrdSameSize(Nrrd *n1, Nrrd *n2, int useBiff) {
  char me[]="nrrdSameSize", err[NRRD_STRLEN_MED];
  int i;

  if (!(n1 && n2)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffMaybeAdd(NRRD, err, useBiff); 
    return 0;
  }
  if (n1->dim != n2->dim) {
    sprintf(err, "%s: n1->dim (%d) != n2->dim (%d)", me, n1->dim, n2->dim);
    biffMaybeAdd(NRRD, err, useBiff); 
    return 0;
  }
  for (i=0; i<=n1->dim-1; i++) {
    if (n1->axis[i].size != n2->axis[i].size) {
      sprintf(err, "%s: n1->axis[%d].size (%d) != n2->axis[%d].size (%d)", 
	      me, i, n1->axis[i].size, i, n2->axis[i].size);
      biffMaybeAdd(NRRD, err, useBiff); 
      return 0;
    }
  }
  return 1;
}

/*
******** nrrdElementSize()
**
** So just how many bytes long is one element in this nrrd?  This is
** needed (over the simple nrrdTypeSize[] array) because some nrrds
** may be of "block" type, and because it does bounds checking on
** nrrd->type.  Returns 0 if given a bogus nrrd->type, or if the block
** size isn't greater than zero (in which case it sets nrrd->blockSize
** to 0, just out of spite).  This function never returns a negative
** value; using (!nrrdElementSize(nrrd)) is a sufficient check for
** invalidity
*/
int
nrrdElementSize(Nrrd *nrrd) {

  if (!(nrrd && AIR_BETWEEN(nrrdTypeUnknown, nrrd->type, nrrdTypeLast))) {
    return 0;
  }
  if (nrrdTypeBlock != nrrd->type) {
    return nrrdTypeSize[nrrd->type];
  }
  else {
    if (nrrd->blockSize > 0) {
      return nrrd->blockSize;
    }
    else {
      nrrd->blockSize = 0;
      return 0;
    }
  }
}

/*
******** nrrdFitsInFormat()
**
** Indicates if the given nrrd can be saved in the given format:
** returns AIR_TRUE if it could fit, AIR_FALSE otherwise.  Mostly.
** nrrdFormatPNM covers PPM and PGM, so there is some slop here
** stating whether or not a nrrd would fit.  To help with this,
** instead of returning AIR_TRUE, we return 2 if given nrrd would fit
** in a PGM image, and 3 if it would fit in a PPM image.
*/
int
nrrdFitsInFormat(Nrrd *nrrd, int format, int useBiff) {
  char me[]="nrrdFitsInFormat", err[NRRD_STRLEN_MED];
  int ret;

  if (!(nrrd)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffMaybeAdd(NRRD, err, useBiff); 
    return AIR_FALSE;
  }
  if (!AIR_BETWEEN(nrrdFormatUnknown, format, nrrdFormatLast)) {
    sprintf(err, "%s: format %d invalid", me, format);
    biffMaybeAdd(NRRD, err, useBiff); 
    return AIR_FALSE;
  }
  switch (format) {
  case nrrdFormatNRRD:
    /* everything fits in a nrrd */
    ret = AIR_TRUE;
    break;
  case nrrdFormatPNM:
    if (nrrdTypeUChar != nrrd->type) {
      sprintf(err, "%s: type is %s, not %s", me,
	      nrrdEnumValToStr(nrrdEnumType, nrrd->type),
	      nrrdEnumValToStr(nrrdEnumType, nrrdTypeUChar));
      biffMaybeAdd(NRRD, err, useBiff); 
      return AIR_FALSE;
    }
    /* else */
    if (2 == nrrd->dim) {
      /* its a gray-scale image */
      ret = 2;
    }
    else if (3 == nrrd->dim) {
      if (3 != nrrd->axis[0].size) {
	sprintf(err, "%s: dimension is 3, but first axis size is %d, not 3",
		me, nrrd->axis[0].size);
	biffMaybeAdd(NRRD, err, useBiff); 
	return AIR_FALSE;
      }
      /* else its an RGB image */
      ret = 3;
    }
    else {
      sprintf(err, "%s: dimension is %d, not 2 or 3", me, nrrd->dim);
      biffMaybeAdd(NRRD, err, useBiff); 
      return AIR_FALSE;
    }
    break;
  case nrrdFormatTable:
    if (2 != nrrd->dim) {
      sprintf(err, "%s: dimension is %d, not 2", me, nrrd->dim);
      biffMaybeAdd(NRRD, err, useBiff); 
      return AIR_FALSE;
    }
    /* any type is good for writing to a table, but it will be
       read back in as floats (unless # headers say otherwise) */
    ret = AIR_TRUE;
    break;
  }
  return ret;
}

/*
******** nrrdFixedType()
**
** returns non-zero iff type is a fixed-point scalar
*/
int
nrrdFixedType(Nrrd *nrrd) {
  int t, ret;
  
  if (nrrd) {
    t = nrrd->type;
    if (t == nrrdTypeChar ||
	t == nrrdTypeUChar ||
	t == nrrdTypeShort ||
	t == nrrdTypeUShort ||
	t == nrrdTypeInt ||
	t == nrrdTypeUInt ||
	t == nrrdTypeLLong ||
	t == nrrdTypeULLong) {
      t = AIR_TRUE;
    }
    else {
      ret = AIR_FALSE;
    }
  }
  else {
    ret = AIR_FALSE;
  }
  
  return ret;
}

/*
******** nrrdFloatingType()
**
** returns non-zero iff type is a floating-point scalar
*/
int
nrrdFloatingType(Nrrd *nrrd) {
  int t, ret;
  
  if (nrrd) {
    t = nrrd->type;
    if (t == nrrdTypeFloat ||
	t == nrrdTypeDouble) {
      t = AIR_TRUE;
    }
    else {
      ret = AIR_FALSE;
    }
  }
  else {
    ret = AIR_FALSE;
  }
  
  return ret;
}

/*
******** nrrdSanity()
**
** makes sure that all the basic assumptions of nrrd hold for
** the architecture/etc which we're currently running on.  
** 
** returns 1 if all is okay, 0 if there is a problem
*/
int
nrrdSanity(void) {
  char me[]="nrrdSanity", err[NRRD_STRLEN_MED];
  float nan, inf;
  int type, tmpI, sign, exp, frac, maxsize;
  char endian;
  long long int tmpLLI;
  unsigned long long int tmpULLI;
  static int sanity = 0;

  if (sanity) {
    /* we've been through this once before and things looked okay ... */
    /* Is this thread-safe?  I think so.  If we assume that any two
       threads are going to compute the same value, isn't it the case
       that, at worse, both of them will go through all the tests and
       then set sanity to the same thing? */
    return 1;
  }

  if (!( nrrdTypeSize[nrrdTypeChar] == sizeof(char)
	 && nrrdTypeSize[nrrdTypeUChar] == sizeof(unsigned char)
	 && nrrdTypeSize[nrrdTypeShort] == sizeof(short)
	 && nrrdTypeSize[nrrdTypeUShort] == sizeof(unsigned short)
	 && nrrdTypeSize[nrrdTypeInt] == sizeof(int)
	 && nrrdTypeSize[nrrdTypeUInt] == sizeof(unsigned int)
	 && nrrdTypeSize[nrrdTypeLLong] == sizeof(long long int)
	 && nrrdTypeSize[nrrdTypeULLong] == sizeof(unsigned long long int)
	 && nrrdTypeSize[nrrdTypeFloat] == sizeof(float)
	 && nrrdTypeSize[nrrdTypeDouble] == sizeof(double) )) {
    sprintf("%s: sizeof() for nrrd types has problem: "
	    "expected (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d) "
	    "but got (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)", me,
	    nrrdTypeSize[nrrdTypeChar],
	    nrrdTypeSize[nrrdTypeUChar],
	    nrrdTypeSize[nrrdTypeShort],
	    nrrdTypeSize[nrrdTypeUShort],
	    nrrdTypeSize[nrrdTypeInt],
	    nrrdTypeSize[nrrdTypeUInt],
	    nrrdTypeSize[nrrdTypeLLong],
	    nrrdTypeSize[nrrdTypeULLong],
	    nrrdTypeSize[nrrdTypeFloat],
	    nrrdTypeSize[nrrdTypeDouble],
	    sizeof(char),
	    sizeof(unsigned char),
	    sizeof(short),
	    sizeof(unsigned short),
	    sizeof(int),
	    sizeof(unsigned int),
	    sizeof(long long int),
	    sizeof(unsigned long long int),
	    sizeof(float),
	    sizeof(double));
    biffAdd(NRRD, err); return 0;
  }

  /* check on NRRD_TYPE_SIZE_MAX */
  maxsize = 0;
  for (type=nrrdTypeUnknown+1; type<=nrrdTypeLast-2; type++) {
    maxsize = AIR_MAX(maxsize, nrrdTypeSize[type]);
  }
  if (maxsize != NRRD_TYPE_SIZE_MAX) {
    sprintf(err, "%s: actual max type size is %d != %d == NRRD_TYPE_SIZE_MAX",
	    me, maxsize, NRRD_TYPE_SIZE_MAX);
    biffAdd(NRRD, err); return 0;
  }
  
  /* run-time endian check */
  tmpI = 1;
  endian = !(*((char*)(&tmpI)));
  if (endian) {
    /* big endian */
    if (4321 != airMyEndian) {
      sprintf(err, "%s: airMyEndian (%d) should be 4321", me, airMyEndian);
      biffAdd(NRRD, err); return 0;
    }
  }
  else {
    if (1234 != airMyEndian) {
      sprintf(err, "%s: airMyEndian (%d) should be 1234", me, airMyEndian);
      biffAdd(NRRD, err); return 0;
    }
  }    

  /* run-time NaN checks */
  inf = 3e+38F;                 /* close to FLT_MAX */
  inf = inf * inf * inf * inf;  /* this generates infinity even for doubles */
  inf = inf * inf * inf * inf;
  inf = inf * inf * inf * inf;
  inf = inf * inf * inf * inf;
  if (AIR_EXISTS(inf)) {
    sprintf(err, "%s: AIR_EXISTS() failed to detect a positive infinity", me);
    biffAdd(NRRD, err); return 0;
  }
  nan = inf/inf;
  if (AIR_EXISTS(nan)) {
    sprintf(err, "%s: AIR_EXISTS() failed to detect a NaN", me);
    biffAdd(NRRD, err); return 0;
  }
  airFPValToParts(&sign, &exp, &frac, nan);
  frac >>= 22;
  if (airMyQNaNHiBit != frac) {
    sprintf(err, "%s: QNAN hi bit seems to be %d, not %d", me,
	    frac, airMyQNaNHiBit);
    biffAdd(NRRD, err); return 0;
  }
  
  /* just make sure airMyDio is reasonably set */
  switch (airMyDio) {
  case 0: break;
  case 1: break;
  default:
    sprintf(err, "%s: airMyDio value (%d) invalid", me, airMyDio);
    biffAdd(NRRD, err); return 0;
  }

  /* nrrd-defined type min/max values */
  tmpLLI = NRRD_LLONG_MAX;
  if (tmpLLI != NRRD_LLONG_MAX) {
    sprintf(err, "%s: long long int can't hold NRRD_LLONG_MAX (%lld)", me,
	    NRRD_LLONG_MAX);
    biffAdd(NRRD, err); return 0;
  }
  tmpLLI += 1;
  if (NRRD_LLONG_MIN != tmpLLI) {
    sprintf(err, "%s: long long int min (%lld) or max (%lld) incorrect", me,
	    NRRD_LLONG_MIN, NRRD_LLONG_MAX);
    biffAdd(NRRD, err); return 0;
  }
  tmpULLI = NRRD_ULLONG_MAX;
  if (tmpULLI != NRRD_ULLONG_MAX) {
    sprintf(err, 
	    "%s: unsigned long long int can't hold NRRD_ULLONG_MAX (%llu)",
	    me, NRRD_ULLONG_MAX);
    biffAdd(NRRD, err); return 0;
  }
  tmpULLI += 1;
  if (tmpULLI != 0) {
    sprintf(err, "%s: unsigned long long int max (%llu) incorrect", me,
	    NRRD_ULLONG_MAX);
    biffAdd(NRRD, err); return 0;
  }
  
  /* HEY: any other assumptions built into teem? */
  /* perhaps check that all the _MAX #defines agree with the 
     highest valid enum values? */
  
  sanity = 1;
  return 1;
}
