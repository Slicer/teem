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

/*
******** nrrdSample_nva()
**
** given coordinates within a nrrd, copies the 
** single element into given *val
*/
int
nrrdSample_nva(void *val, Nrrd *nrrd, int *coord) {
  char me[]="nrrdSample_nva", err[NRRD_STRLEN_MED];
  int typeSize, size[NRRD_DIM_MAX], d;
  nrrdBigInt I;
  
  if (!(nrrd && coord && val)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nrrd)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }
  
  typeSize = nrrdElementSize(nrrd);
  nrrdAxesGet_nva(nrrd, nrrdAxesInfoSize, size);
  for (d=0; d<=nrrd->dim-1; d++) {
    if (!(AIR_INSIDE(0, coord[d], size[d]-1))) {
      sprintf(err, "%s: coordinate %d on axis %d out of bounds (0 to %d)", 
	      me, coord[d], d, size[d]-1);
      biffSet(NRRD, err); return 1;
    }
  }

  NRRD_COORD_INDEX(I, coord, size, nrrd->dim, d);

  memcpy(val, (char*)(nrrd->data) + I*typeSize, typeSize);
  return 0;
}

/*
******** nrrdSample()
**
** var-args version of nrrdSample_nva()
*/
int
nrrdSample(void *val, Nrrd *nrrd, ...) {
  char me[]="nrrdSample", err[NRRD_STRLEN_MED];
  int d, coord[NRRD_DIM_MAX];
  va_list ap;
  
  if (!(nrrd && val)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }

  va_start(ap, nrrd);
  for (d=0; d<=nrrd->dim-1; d++) {
    coord[d] = va_arg(ap, int);
  }
  va_end(ap);
  
  if (nrrdSample(val, nrrd, coord)) {
    sprintf(err, "%s: trouble", me);
    biffSet(NRRD, err); return 1;
  }
  return 0;
}

/*
******** nrrdSlice()
**
** slices a nrrd along a given axis, at a given position.
**
** will allocate memory for the new slice only if NULL==nout->data,
** otherwise assumes that the pointer there is pointing to something
** big enough to hold the slice
** 
** This is a newer version of the procedure, which is simpler, faster,
** and requires less memory overhead than the first one.  It is based
** on the observation that any slice is a periodic square-wave pattern
** in the original data (viewed as a one- dimensional array).  The
** characteristics of that periodic pattern are how far from the
** beginning it starts (offset), the length of the "on" part (length),
** the period (period), and the number of periods (numper). 
*/
int
nrrdSlice(Nrrd *nout, Nrrd *nin, int axis, int pos) {
  char me[]="nrrdSlice", err[NRRD_STRLEN_MED];
  nrrdBigInt 
    I, 
    offset,                  /* index of first segment of slice */
    length,                  /* length of segment */
    period,                  /* distance between start of each segment */
    numper;                  /* number of periods */
  int i, map[NRRD_DIM_MAX], size[NRRD_DIM_MAX];
  char *src, *dest;

  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (1 == nin->dim) {
    sprintf(err, "%s: can't slice a 1-D nrrd (use nrrd{I|F|D}Lookup)", me);
    biffSet(NRRD, err); return 1;
  }
  if (!(AIR_INSIDE(0, axis, nin->dim-1))) {
    sprintf(err, "%s: slice axis %d out of bounds (0 to %d)", 
	    me, axis, nin->dim-1);
    biffSet(NRRD, err); return 1;
  }
  if (!(AIR_INSIDE(0, pos, nin->axis[axis].size-1) )) {
    sprintf(err, "%s: position %d out of bounds (0 to %d)", 
	    me, axis, nin->axis[axis].size-1);
    biffSet(NRRD, err); return 1;
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nin)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }

  /* set up control variables */
  length = numper = 1;
  for (i=0; i<=nin->dim-1; i++) {
    if (i < axis) {
      length *= nin->axis[i].size;
    }
    else if (i > axis) {
      numper *= nin->axis[i].size;
    }
  }
  length *= nrrdElementSize(nin);
  offset = length*pos;
  period = length*nin->axis[axis].size;

  /* allocate space if necessary */
  nrrdAxesGet_nva(nin, nrrdAxesInfoSize, size);
  for (i=0; i<=nout->dim-1; i++) {
    map[i] = i + (i >= axis);
    size[i] = nin->axis[map[i]].size;
  }
  nout->blockSize = nin->blockSize;
  if (nrrdMaybeAlloc_nva(nout, nin->type, nin->dim-1, size)) {
    sprintf(err, "%s: failed to create slice", me);
    biffAdd(NRRD, err); return 1;
  }

  /* the skinny */
  src = nin->data;
  dest = nout->data;
  src += offset;
  for (I=1; I<=numper; I++) {
    /* HEY: replace with AIR_MEMCPY() or similar, when applicable */
    memcpy(dest, src, length);
    src += period;
    dest += length;
  }

  /* copy the peripheral information */
  if (nrrdAxesCopy(nout, nin, map, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("slice(,,)")
			   + strlen(nin->content)
			   + 11
			   + 11
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "slice(%s,%d,%d)", nin->content, axis, pos);
    }
    else {
      sprintf(err, "%s: couldn't alloc content string", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  nout->blockSize = nin->blockSize;
  nout->min = nout->max = AIR_NAN;
  nout->oldMin = nout->oldMax = AIR_NAN;
  /* leave comments alone */

  return(0);
}

/*
******** nrrdCrop()
**
** select some sub-volume inside a given nrrd, producing an output
** nrrd with the same dimensions, but with equal or smaller sizes
** along each axis.
*/
int
nrrdCrop(Nrrd *nout, Nrrd *nin, int *min, int *max) {
  char me[]="nrrdCrop", err[NRRD_STRLEN_MED], buff[NRRD_STRLEN_SMALL];
  int d, dim,
    lineSize,                /* #bytes in one scanline to be copied */
    typeSize,                /* size of data type */
    cIn[NRRD_DIM_MAX],       /* coords for line start, in input */
    cOut[NRRD_DIM_MAX],      /* coords for line start, in output */
    szIn[NRRD_DIM_MAX],
    szOut[NRRD_DIM_MAX];
  nrrdBigInt I,
    idxIn, idxOut,           /* linear indices for input and output */
    numLines;                /* number of scanlines in output nrrd */
  char *dataIn, *dataOut;

  /* errors */
  if (!(nout && nin && min && max)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  dim = nin->dim;
  for (d=0; d<=dim-1; d++) {
    if (!(min[d] <= max[d])) {
      sprintf(err, "%s: axis %d min (%d) not <= max (%d)", 
	      me, d, min[d], max[d]);
      biffSet(NRRD, err); return 1;
    }
    if (!(AIR_INSIDE(0, min[d], nin->axis[d].size-1) &&
	  AIR_INSIDE(0, max[d], nin->axis[d].size-1))) {
      sprintf(err, "%s: axis %d min (%d) or max (%d) out of bounds [0,%d]",
	      me, d, min[d], max[d], nin->axis[d].size-1);
      biffSet(NRRD, err); return 1;
    }
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nin)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }

  /* allocate */
  nrrdAxesGet_nva(nin, nrrdAxesInfoSize, szIn);
  numLines = 1;
  for (d=0; d<=dim-1; d++) {
    szOut[d] = max[d] - min[d] + 1;
    if (d)
      numLines *= szOut[d];
  }
  nout->blockSize = nin->blockSize;
  if (nrrdMaybeAlloc_nva(nout, nin->type, dim, szOut)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  lineSize = szOut[0]*nrrdElementSize(nin);
  
  /* the skinny */
  typeSize = nrrdElementSize(nin);
  dataIn = nin->data;
  dataOut = nout->data;
  memset(cOut, 0, NRRD_DIM_MAX*sizeof(int));
  for (I=0; I<=numLines-1; I++) {
    for (d=0; d<=dim-1; d++)
      cIn[d] = cOut[d] + min[d];
    NRRD_COORD_INDEX(idxOut, cOut, szOut, dim, d);
    NRRD_COORD_INDEX(idxIn, cIn, szIn, dim, d);
    memcpy(dataOut + idxOut*typeSize, dataIn + idxIn*typeSize, lineSize);
    /* the lowest coordinate in cOut[] will stay zero, since we are 
       copying one scanline at a time */
    cOut[1]++; 
    NRRD_COORD_UPDATE(cOut+1, szOut+1, dim-1, d);
  }
  if (nrrdAxesCopy(nout, nin, NULL, (NRRD_AXESINFO_SIZE
				     | NRRD_AXESINFO_AMINMAX ))) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  for (d=0; d<=dim-1; d++) {
    nrrdAxisPosRange(&(nout->axis[d].min), &(nout->axis[d].max),
		     nin, d, min[d], max[d]);
  }
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("crop(,)")
			   + strlen(nin->content)
			   + dim*(strlen("[,]x") + 2*11)
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "crop(%s,", nin->content);
      for (d=0; d<=dim-1; d++) {
	sprintf(buff, "%s[%d,%d]", (d ? "x" : ""), min[d], max[d]);
	strcat(nout->content, buff);
      }
      sprintf(buff, ")");
      strcat(nout->content, buff);    
    }
    else {
      sprintf(err, "%s: couldn't alloc content string", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  nout->min = nout->max = AIR_NAN;
  nout->oldMin = nout->oldMax = AIR_NAN;
  /* leave comments alone */

  return 0;
}

/*
******** nrrdPad()
**
** strictly for padding
*/
int
nrrdPad(Nrrd *nout, Nrrd *nin, int *min, int *max, int boundary, ...) {
  char me[]="nrrdPad", err[NRRD_STRLEN_MED], buff[NRRD_STRLEN_SMALL];
  double padValue;
  int d, outside, dim, typeSize,
    cIn[NRRD_DIM_MAX],       /* coords for line start, in input */
    cOut[NRRD_DIM_MAX],      /* coords for line start, in output */
    szIn[NRRD_DIM_MAX],
    szOut[NRRD_DIM_MAX];
  nrrdBigInt
    idxIn, idxOut,           /* linear indices for input and output */
    numOut;                  /* number of elements in output nrrd */
  va_list ap;
  char *dataIn, *dataOut;
  
  if (!(nout && nin && min && max)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (!AIR_BETWEEN(nrrdBoundaryUnknown, boundary, nrrdBoundaryLast)) {
    sprintf(err, "%s: boundary behavior %d invalid", me, boundary);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdBoundaryWeight == boundary) {
    sprintf(err, "%s: boundary %s non-sensical here", me,
	    nrrdEnumValToStr(nrrdEnumBoundary, boundary));
    biffSet(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == nin->type
      && (nrrdBoundaryPad == boundary)) {
    sprintf(err, "%s: with nrrd type %s, boundary %s not valid", me,
	    nrrdEnumValToStr(nrrdEnumType, nrrdTypeBlock),
	    nrrdEnumValToStr(nrrdEnumBoundary, nrrdBoundaryPad));
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdBoundaryPad == boundary) {
    va_start(ap, boundary);
    padValue = va_arg(ap, double);
    va_end(ap);
  }
  switch(boundary) {
  case nrrdBoundaryPad: case nrrdBoundaryBleed: case nrrdBoundaryWrap:
    break;
  default:
    fprintf(stderr, "%s: PANIC: boundary %d unimplemented\n", 
	    me, boundary); exit(1); break;
  }
  dim = nin->dim;
  for (d=0; d<=dim-1; d++) {
    if (!(min[d] <= 0)) {
      sprintf(err, "%s: axis %d min (%d) not <= 0", 
	      me, d, min[d]);
      biffSet(NRRD, err); return 1;
    }
    if (!(max[d] >= nin->axis[d].size-1)) {
      sprintf(err, "%s: axis %d max (%d) not >= size-1 (%d)", 
	      me, d, max[d], nin->axis[d].size-1);
      biffSet(NRRD, err); return 1;
    }
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nin)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }

  /* allocate */
  numOut = 1;
  for (d=0; d<=dim-1; d++) {
    /* here we take care of axis fields: size, center, min/max */
    nout->axis[d].size = -min[d] + max[d] + 1;
    numOut *= nout->axis[d].size;
    nout->axis[d].center = nin->axis[d].center;
    nrrdAxisPosRange(&(nout->axis[d].min), &(nout->axis[d].max),
		     nin, d, min[d], max[d]);
  }
  nout->blockSize = nin->blockSize;
  if (nrrdMaybeAlloc(nout, numOut, nin->type, dim)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  
  /* the skinny */
  typeSize = nrrdElementSize(nin);
  dataIn = nin->data;
  dataOut = nout->data;
  nrrdAxesGet_nva(nin, nrrdAxesInfoSize, szIn);
  nrrdAxesGet_nva(nout, nrrdAxesInfoSize, szOut);
  memset(cOut, 0, NRRD_DIM_MAX*sizeof(int));
  for (idxOut=0; idxOut<=numOut-1; idxOut++) {
    outside = 0;
    for (d=0; d<=dim-1; d++) {
      cIn[d] = cOut[d] + min[d];
      switch(boundary) {
      case nrrdBoundaryPad:
      case nrrdBoundaryBleed:
	if (!AIR_INSIDE(0, cIn[d], szIn[d]-1)) {
	  cIn[d] = AIR_CLAMP(0, cIn[d], szIn[d]-1);
	  outside = 1;
	}
	break;
      case nrrdBoundaryWrap:
	if (!AIR_INSIDE(0, cIn[d], szIn[d]-1)) {
	  cIn[d] = AIR_MOD(cIn[d], szIn[d]);
	  outside = 1;
	}
	break;
      }
    }
    NRRD_COORD_INDEX(idxIn, cIn, szIn, dim, d);
    if (!outside) {
      /* the cIn coords are within the input nrrd: memcpy() of scanline,
	 and then artificially bump for-loop */
      memcpy(dataOut + idxOut*typeSize, dataIn + idxIn*typeSize,
	     nin->axis[0].size*typeSize);
      idxOut += nin->axis[0].size-1;
      cOut[0] += nin->axis[0].size-1;
    }
    else {
      /* we copy only a single value */
      if (nrrdBoundaryPad == boundary) {
	nrrdDInsert[nout->type](dataOut, idxOut, padValue);
      }
      else {
	memcpy(dataOut + idxOut*typeSize, dataIn + idxIn*typeSize, typeSize);
      }
    }
    NRRD_COORD_INCR(cOut, szOut, dim, d);
  }
  if (nrrdAxesCopy(nout, nin, NULL, (NRRD_AXESINFO_SIZE
				     | NRRD_AXESINFO_CENTER
				     | NRRD_AXESINFO_AMINMAX ))) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("pad(,)")
			   + strlen(nin->content)
			   + dim*(strlen("[,]x") + 2*11)
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "pad(%s,", nin->content);
      for (d=0; d<=dim-1; d++) {
	sprintf(buff, "%s[%d,%d]", (d ? "x" : ""), min[d], max[d]);
	strcat(nout->content, buff);
      }
      sprintf(buff, ")");
      strcat(nout->content, buff);    
    }
    else {
      sprintf(err, "%s: couldn't alloc content string", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  nout->blockSize = nin->blockSize;
  nout->min = nout->max = AIR_NAN;
  nout->oldMin = nout->oldMax = AIR_NAN;
  /* leave comments alone */

  return 0;
}

/*
******** nrrdSubvolume
**
** THIS FUNCTION IS OFFICIALLY DEPRECATED.  ITS CORRECT OPERATION WITH
** RESPECT TO ALL NRRD STRUCT AND AXIS STRUCT FIELDS IS NOT ASSURED.
**
** This is a bit of a swiss-army function- allowing cropping, padding,
** and reversal of slices along axes.  The min and max arrays give
** the coordinates of the lowest and highest corner of the subvolume,
** but if min[d] is greater than max[d], then the subvolume is flipped
** along axis d.  Also, neither min[d] or max[d] need be limited to 
** the size of the nrrd along that axis; the volume will be padded
** as necessary.  The clamp argument controls the behavior of padding:
** if clamp is non-zero, then the boundary of the original nrrd will
** be flooded outwards to create new values.  Otherwise, regions outside
** the original volume will have value 0.0
*/
int
nrrdSubvolume(Nrrd *nout, Nrrd *nin, int *min, int *max, int clamp) {
  char me[]="nrrdSubvolume", err[NRRD_STRLEN_MED], 
    tmpS[NRRD_STRLEN_BIG];
  nrrdBigInt num,            /* number of elements in volume */
    i,                       /* index through elements in slice */
    idx;                     /* index into original array */
  char *dataIn, *dataOut;
  int len[NRRD_DIM_MAX],
    outside,                 /* current subvolume point is outside original */
    dim,                     /* dimension of volume */
    d,                       /* running index along dimensions */
    elSize,                  /* size of one element */

    coord[NRRD_DIM_MAX];     /* array of original array coordinates */
  double half, amin, amax;

  fprintf(stderr, 
	  "%s: WARNING: Deprecated. (use nrrdCrop() and/or nrrdPad())\n", me);

  if (!(nin && nout && min && max)) {
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  dim = nin->dim;
  dataIn = nin->data;
  num = 1;
  for (d=0; d<=dim-1; d++) {
    /* 
    ** we do NOT enforce that min[d] < max[d] because having 
    ** min[d] > max[d] is how the caller requests flipping along
    ** that axis
    */
    len[d] = AIR_ABS(max[d] - min[d]) + 1;
    num *= len[d];
  }
  nout->blockSize = nin->blockSize;
  if (nrrdMaybeAlloc(nout, num, nin->type, dim)) {
    sprintf(err, "%s: failed to create slice", me);
    biffAdd(NRRD, err); return 1;
  }
  dataOut = nout->data;
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nin)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }
  elSize = nrrdElementSize(nin);

  /* produce array of coordinates inside original array of the
  ** elements that comprise the slice.  We go linearly through the
  ** indices of the cropped volume, and then div and mod this to
  ** produce the necessary coordinates
  */
  for (i=0; i<=num-1; i++) {
    /* we go from idx (subvolume linear index) to create 
       coord[] array of coordinates in original volume */
    idx = i;
    for (d=0; d<=dim-1; d++) {
      coord[d] = idx % len[d];
      if (max[d] < min[d]) {
	/* flip along this axis */
	coord[d] = len[d] - 1 - coord[d];
      }
      coord[d] += AIR_MIN(max[d], min[d]);
      idx /= len[d];
    }
    /* we go from coord[] to idx (original volume linear index) */
    idx = 0;
    outside = 0;
    for (d=dim-1; d>=0; d--) {
      if (!AIR_INSIDE(0, coord[d], nin->axis[d].size-1)) {
	if (clamp) {
	  coord[d] = AIR_CLAMP(0, coord[d], nin->axis[d].size-1);
	}
	else {
	  /* we allow the coord[] array and idx to hold bogus values 
	     (which we'll ignore) */
	  outside = 1;
	}
      }
      idx = coord[d] + nin->axis[d].size*idx;
    }
    if (outside) {
      nrrdFInsert[nin->type](dataOut, i, 0.0);
    }
    else {
      /* calling memcpy vs. AIR_MEMCPY won't make a significant difference */
      memcpy(dataOut + i*elSize, dataIn + idx*elSize, elSize);
    }
  }

  /* set information in subvolume */
  nrrdAxesCopy(nout, nin, NULL, NRRD_AXESINFO_AMINMAX | NRRD_AXESINFO_SIZE);
  for (d=0; d<=dim-1; d++) {
    amin = nin->axis[d].min;
    amax = nin->axis[d].max;
    nout->axis[d].size = len[d];
    if (nrrdCenterCell == nin->axis[d].center) {
      half = (amax - amin)/len[d];
      amin += half;  amax -= half;
      nout->axis[d].min = AIR_AFFINE(0, min[d], len[d]-1, amin, amax);
      nout->axis[d].max = AIR_AFFINE(0, max[d], len[d]-1, amin, amax);
      nout->axis[d].min -= half;  nout->axis[d].max += half;
    }
    else {
      nout->axis[d].min = AIR_AFFINE(0, min[d], len[d]-1, amin, amax);
      nout->axis[d].max = AIR_AFFINE(0, max[d], len[d]-1, amin, amax);
    }
  }
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("crop()")
			   + strlen(nin->content)
			   + nin->dim*(11 + 1 + 11 + 1)
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "crop(%s,", nin->content);
      for (d=0; d<=dim-1; d++) {
	sprintf(tmpS, "%d-%d%c", min[d], max[d], d < dim-1 ? ',' : ')');
	strcat(nout->content, tmpS);
      }
    }
    else {
      sprintf(err, "%s: couldn't allocate output content string", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  nout->blockSize = nin->blockSize;

  return 0;
}
