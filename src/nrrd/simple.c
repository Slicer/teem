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
    fprintf(file, "Nrrd at 0x%p:\n", (void*)nrrd);
    fprintf(file, "Data at 0x%p is " NRRD_BIG_INT_PRINTF 
	    " elements of type %s.\n",
	    nrrd->data, nrrdElementNumber(nrrd), 
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
  int  size[NRRD_DIM_MAX];

  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 0;
  }
  if (!AIR_BETWEEN(nrrdTypeUnknown, nrrd->type, nrrdTypeLast)) {
    sprintf(err, "%s: type (%d) of array is invalid", me, nrrd->type);
    biffAdd(NRRD, err); return 0;
  }
  if (nrrdTypeBlock == nrrd->type && (!(0 < nrrd->blockSize)) ) {
    sprintf(err, "%s: nrrd type is %s but nrrd->blockSize (%d) invalid", me,
	    nrrdEnumValToStr(nrrdEnumType, nrrdTypeBlock),
	    nrrd->blockSize);
    biffAdd(NRRD, err); return 0;
  }
  if (!AIR_INSIDE(1, nrrd->dim, NRRD_DIM_MAX)) {
    sprintf(err, "%s: dimension %d is outside valid range [1,%d]",
	    me, nrrd->dim, NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 0;
  }
  nrrdAxesGet_nva(nrrd, nrrdAxesInfoSize, size);
  if (!_nrrdSizeValid(nrrd->dim, size)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 0;
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
** invalidity.
*/
int
nrrdElementSize(Nrrd *nrrd) {

  if (!(nrrd && AIR_BETWEEN(nrrdTypeUnknown, nrrd->type, nrrdTypeLast))) {
    return 0;
  }
  if (nrrdTypeBlock != nrrd->type) {
    return nrrdTypeSize[nrrd->type];
  }
  /* else its block type */
  if (nrrd->blockSize > 0) {
    return nrrd->blockSize;
  }
  /* else we got an invalid block size */
  nrrd->blockSize = 0;
  return 0;
}

/*
******** nrrdElementNumber()
**
** takes the place of old "nrrd->num": the number of elements in the
** nrrd, which is just the product of the axis sizes.
*/
nrrdBigInt
nrrdElementNumber(Nrrd *nrrd) {
  nrrdBigInt num;
  int d;

  if (!nrrd) {
    return 0;
  }
  /* else */
  num = 1;
  for (d=0; d<=nrrd->dim-1; d++) {
    num *= nrrd->axis[d].size;
  }
  return num;
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
  int ret=AIR_FALSE;

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
******** nrrdHasNonExist()
**
** This function will always (assuming type is valid) set the value of
** nrrd->hasNonExist to either nrrdNonExistTrue or nrrdNonExistFalse,
** and it will return that value.  Blocks are considered to be
** existant values.  This function will ALWAYS determine the correct
** answer and set the value of nrrd->hasNonExist: it ignores the value
** of nrrd->hasNonExist on the input nrrd.  */
int
nrrdHasNonExist(Nrrd *nrrd) {
  nrrdBigInt I, N;
  float val;

  if (!nrrd)
    return nrrdNonExistUnknown;
  if (!( AIR_BETWEEN(nrrdTypeUnknown, nrrd->type, nrrdTypeLast) ))
    return nrrdNonExistUnknown;
  if (nrrdTypeFixed[nrrd->type]) {
    nrrd->hasNonExist = nrrdNonExistFalse;
  }
  else {
    nrrd->hasNonExist = nrrdNonExistFalse;
    N = nrrdElementNumber(nrrd);
    for (I=0; I<=N-1; I++) {
      val = nrrdFLookup[nrrd->type](nrrd->data, I);
      if (!AIR_EXISTS(val)) {
	nrrd->hasNonExist = nrrdNonExistTrue;
	break;
      }
    }
  }
  return nrrd->hasNonExist;
}

int
_nrrdCheckEnums() {
  char me[]="_nrrdCheckEnums", err[NRRD_STRLEN_MED],
    which[NRRD_STRLEN_SMALL];

  if (nrrdFormatLast-1 != NRRD_FORMAT_MAX) {
    strcpy(which, "nrrdFormat"); goto err;
  }
  if (nrrdBoundaryLast-1 != NRRD_BOUNDARY_MAX) {
    strcpy(which, "nrrdBoundary"); goto err;
  }
  if (nrrdMagicLast-1 != NRRD_MAGIC_MAX) {
    strcpy(which, "nrrdMagic"); goto err;
  }
  if (nrrdTypeLast-1 != NRRD_TYPE_MAX) {
    strcpy(which, "nrrdType"); goto err;
  }
  if (nrrdEncodingLast-1 != NRRD_ENCODING_MAX) {
    strcpy(which, "nrrdEncoding"); goto err;
  }
  if (nrrdMeasureLast-1 != NRRD_MEASURE_MAX) {
    strcpy(which, "nrrdMeasure"); goto err;
  }
  if (nrrdCenterLast-1 != NRRD_CENTER_MAX) {
    strcpy(which, "nrrdCenter"); goto err;
  }
  if (nrrdAxesInfoLast-1 != NRRD_AXESINFO_MAX) {
    strcpy(which, "nrrdAxesInfo"); goto err;
  }
  /* can't really check on endian enum */
  if (nrrdField_last-1 != NRRD_FIELD_MAX) {
    strcpy(which, "nrrdField"); goto err;
  }
  if (nrrdNonExistLast-1 != NRRD_NON_EXIST_MAX) {
    strcpy(which, "nrrdNonExist"); goto err;
  }
  if (nrrdEnumLast-1 != NRRD_ENUM_MAX) {
    strcpy(which, "nrrdEnum"); goto err;
  }
  
  /* no errors so far */
  return 0;

 err:
  sprintf(err, "%s: Last vs. MAX incompatibility for %s enum", me, which);
  biffAdd(NRRD, err); return 1;
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
  int aret, type, maxsize;
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
  
  aret = airSanity();
  if (aret != airInsane_not) {
    sprintf(err, "%s: airSanity() failed: %s", me, airInsaneErr(aret));
    biffAdd(NRRD, err); return 0;
  }

  if (!( AIR_BETWEEN(nrrdEncodingUnknown, 
		     nrrdDefWrtEncoding,
		     nrrdEncodingLast) )) {
    sprintf(err, "%s: nrrdDefWrtEncoding (%d) not in valid range [%d,%d]",
	    me, nrrdDefWrtEncoding,
	    nrrdEncodingUnknown+1, nrrdEncodingLast-1);
    biffAdd(NRRD, err); return 0;
  }
  if (!( AIR_BETWEEN(nrrdBoundaryUnknown,
		     nrrdDefRsmpBoundary,
		     nrrdBoundaryLast) )) {
    sprintf(err, "%s: nrrdDefRsmpBoundary (%d) not in valid range [%d,%d]",
	    me, nrrdDefRsmpBoundary,
	    nrrdBoundaryUnknown+1, nrrdBoundaryLast-1);
    biffAdd(NRRD, err); return 0;
  }
  if (!( AIR_BETWEEN(nrrdCenterUnknown,
		     nrrdDefCenter,
		     nrrdCenterLast) )) {
    sprintf(err, "%s: nrrdDefCenter (%d) not in valid range [%d,%d]",
	    me, nrrdDefCenter,
	    nrrdCenterUnknown+1, nrrdCenterLast-1);
    biffAdd(NRRD, err); return 0;
  }
  if (!( AIR_BETWEEN(nrrdTypeUnknown-1,
		     nrrdDefRsmpType,
		     nrrdTypeLast) )) {
    sprintf(err, "%s: nrrdDefRsmpType (%d) not in valid range [%d,%d]",
	    me, nrrdDefRsmpType,
	    nrrdTypeUnknown, nrrdTypeLast-1);
    biffAdd(NRRD, err); return 0;
  }
  if (!( AIR_BETWEEN(nrrdTypeUnknown,
		     nrrdStateMeasureType,
		     nrrdTypeLast) )) {
    sprintf(err, "%s: nrrdStateMeasureType (%d) not in valid range [%d,%d]",
	    me, nrrdStateMeasureType,
	    nrrdTypeUnknown+1, nrrdTypeLast-1);
    biffAdd(NRRD, err); return 0;
  }
  if (!( AIR_BETWEEN(nrrdTypeUnknown,
		     nrrdStateMeasureHistoType,
		     nrrdTypeLast) )) {
    sprintf(err,
	    "%s: nrrdStateMeasureHistoType (%d) not in valid range [%d,%d]",
	    me, nrrdStateMeasureType,
	    nrrdTypeUnknown+1, nrrdTypeLast-1);
    biffAdd(NRRD, err); return 0;
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

  /* check on NRRD_TYPE_BIGGEST */
  if (maxsize != sizeof(NRRD_TYPE_BIGGEST)) {
    sprintf(err, "%s: actual max type size is %d != "
	    "%d == sizeof(NRRD_TYPE_BIGGEST)",
	    me, maxsize, (int)sizeof(NRRD_TYPE_BIGGEST));
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

  if (_nrrdCheckEnums()) {
    sprintf(err, "%s: problem with enum definition", me);
    biffAdd(NRRD, err); return 0;
  }

  /* HEY: any other assumptions built into teem? */

  sanity = 1;
  return 1;
}
