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
      fprintf(file, "It is \"%s\"\n", nrrd->content);
    fprintf(file, "It is a %d-dimensional array, with axes:\n", nrrd->dim);
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
    if (nrrd->cmt) {
      fprintf(file, "Comments:\n");
      for (i=0; i<=nrrd->cmtArr->len-1; i++) {
	fprintf(file, "%s\n", nrrd->cmt[i]);
      }
    }
    fprintf(file, "\n");
  }
}

int
nrrdValid(Nrrd *nrrd) {
  char me[] = "nrrdValid", err[NRRD_STRLEN_MED];
  nrrdBigInt mult;
  int i;

  if (!(nrrd->num >= 1)) {
    sprintf(err, "%s: number of elements is %d", me, (int)nrrd->num);
    biffSet(NRRD, err); return 0;
  }
  if (!AIR_BETWEEN(nrrdTypeUnknown, nrrd->type, nrrdTypeLast)) {
    sprintf(err, "%s: type (%d) of array is invalid", me, nrrd->type);
    biffSet(NRRD, err); return 0;
  }
  if (nrrdTypeBlock == nrrd->type && -1 == nrrd->blockSize) {
    sprintf(err, "%s: type is \"block\" but no blocksize given", me);
    biffSet(NRRD, err); return 0;
  }
  if (!AIR_INSIDE(1, nrrd->dim, NRRD_DIM_MAX)) {
    sprintf(err, "%s: dimension %d is outside valid range [1,%d]",
	    me, nrrd->dim, NRRD_DIM_MAX);
    biffSet(NRRD, err); return 0;
  }
  mult = 1;
  for (i=0; i<=nrrd->dim-1; i++) {
    if (!(1 <= nrrd->axis[i].size))
      mult = -1;
    else
      mult *= nrrd->axis[i].size;
  }
  if (mult != nrrd->num) {
    sprintf(err, "%s: # elements != product of axes sizes ", me);
    biffSet(NRRD, err); return 0;
  }
  return 1;
}

int
nrrdSameSize(Nrrd *n1, Nrrd *n2, int useBiff) {
  char me[]="nrrdSameSize", err[NRRD_STRLEN_MED];
  int i;

  if (!(n1 && n2)) {
    if (useBiff) {
      sprintf(err, "%s: got NULL pointer", me);
      biffSet(NRRD, err); 
    }
    return 0;
  }
  if (n1->dim != n2->dim) {
    if (useBiff) {
      sprintf(err, "%s: n1->dim (%d) != n2->dim (%d)", me, n1->dim, n2->dim);
      biffSet(NRRD, err); 
    }
    return 0;
  }
  for (i=0; i<=n1->dim-1; i++) {
    if (n1->axis[i].size != n2->axis[i].size) {
      if (useBiff) {
	sprintf(err, "%s: n1->axis[%d].size (%d) != n2->axis[%d].size (%d)", 
		me, i, n1->axis[i].size, i, n2->axis[i].size);
	biffSet(NRRD, err); 
      }
      return 0;
    }
  }
  return 1;
}

/*
******** nrrdElementSize()
**
** So just how many bytes long is one element in this nrrd?
** This is needed (over the simple nrrdTypeSize[] array) 
** because some nrrds may be of "block" type.
*/
int
nrrdElementSize(Nrrd *nrrd) {

  if (!(nrrd && AIR_BETWEEN(nrrdTypeUnknown, nrrd->type, nrrdTypeLast))) {
    return -1;
  }
  if (nrrdTypeBlock != nrrd->type) {
    return nrrdTypeSize[nrrd->type];
  }
  else {
    return nrrd->blockSize;
  }
}

/*
******** nrrdFitsInFormat()
**
** Indicates if the given nrrd can be saved in the given format:
** returns AIR_TRUE if it could fit, AIR_FALSE otherwise.
** nrrdFormatPNM covers PPM and PGM, so there is some slop here
** stating whether or not a nrrd would fit.  To help with this
** instead of returning AIR_TRUE, we return 2 if given nrrd would
** fit in a PGM image, and 3 if it would fit in a PPM image
*/
int
nrrdFitsInFormat(Nrrd *nrrd, int format, int useBiff) {
  char me[]="nrrdFitsInFormat", err[NRRD_STRLEN_MED];
  int ret;

  if (!(nrrd)) {
    sprintf(err, "%s: got NULL pointer", me);
    if (useBiff) biffAdd(NRRD, err); return AIR_FALSE;
  }
  if (!AIR_BETWEEN(nrrdFormatUnknown, format, nrrdFormatLast)) {
    sprintf(err, "%s: format %d invalid", me, format);
    if (useBiff) biffAdd(NRRD, err); return AIR_FALSE;
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
      if (useBiff) biffAdd(NRRD, err); return AIR_FALSE;
    }
    /* else */
    if (2 == nrrd->dim) {
      ret = 2;
    }
    else if (3 == nrrd->dim) {
      if (3 != nrrd->axis[0].size) {
	sprintf(err, "%s: dimension is 3, but first axis size is %d, not 3",
		me, nrrd->axis[0].size);
	if (useBiff) biffAdd(NRRD, err); return AIR_FALSE;
      }
      /* else */
      ret = 3;
    }
    else {
      sprintf(err, "%s: dimension is %d, not 2 or 3", me, nrrd->dim);
      if (useBiff) biffAdd(NRRD, err); return AIR_FALSE;
    }
    break;
  case nrrdFormatTable:
    if (2 != nrrd->dim) {
      sprintf(err, "%s: dimension is %d, not 2", me, nrrd->dim);
      if (useBiff) biffAdd(NRRD, err); return AIR_FALSE;
    }
    /* else */
    /* any type is good */
    ret = AIR_TRUE;
    break;
  }
  return ret;
}

void
nrrdAxisMinMax(double *nMinP, double *nMaxP, 
	       double lo, double hi, int nSize,
	       double oMin, double oMax, int oSize, int center) {
  char me[]="nrrdAxisMinMax";
  double tmp;

  if (!(nMinP && nMaxP))
    return;
  
  if (!(lo <= hi && nSize > 0
	&& oMin <= oMax && oSize > 0
	&& AIR_BETWEEN(nrrdCenterUnknown, center, nrrdCenterLast) )) {
    *nMinP = *nMaxP = AIR_NAN;
  }
  
  if (nrrdCenterNode == center) {
    tmp = AIR_AFFINE(-0.5, lo, oSize-0.5, oMin, oMax);
  }
  else if (nrrdCenterCell == center) {
    
  }
  else {
    fprintf(stderr, "%s: WARNING: center %d unimplemented\n", me, center);
    *nMinP = *nMaxP = AIR_NAN;
  }
}
