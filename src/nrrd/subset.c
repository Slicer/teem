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
******** nrrdSample()
**
** given coordinates within a nrrd, copies the 
** single element into given *val
*/
int
nrrdSample(void *val, Nrrd *nrrd, int *coord) {
  char me[]="nrrdSample", err[NRRD_STRLEN_MED];
  int typeSize, size[NRRD_DIM_MAX], d;
  nrrdBigInt I;
  
  if (!(nrrd && coord && val && 
	AIR_BETWEEN(nrrdTypeUnknown, nrrd->type, nrrdTypeLast))) {
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  typeSize = nrrdElementSize(nrrd);
  for (d=0; d<=nrrd->dim-1; d++) {
    if (!(AIR_INSIDE(0, coord[d], nrrd->axis[d].size-1))) {
      sprintf(err, "%s: coordinate %d on axis %d out of bounds (0 to %d)", 
	      me, coord[d], d, nrrd->axis[d].size-1);
      biffSet(NRRD, err); return 1;
    }
  }

  nrrdAxesGet(nrrd, nrrdAxesInfoSize, size);
  NRRD_COORD_INDEX(coord, size, nrrd->dim, d, I);

  memcpy((char*)val, (char*)(nrrd->data) + I*typeSize, typeSize);
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
  int i, map[NRRD_DIM_MAX];
  char *src, *dest;

  if (!(nin && nout)) {
    sprintf(err, "%s: invalid args", me);
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
  if (nrrdMaybeAlloc(nout, nin->num/nin->axis[axis].size, 
		     nin->type, nin->dim-1)) {
    sprintf(err, "%s: failed to create slice", me);
    biffAdd(NRRD, err); return 1;
  }

  /* the skinny */
  src = nin->data;
  dest = nout->data;
  src += offset;
  for (I=1; I<=numper; I++) {
    memcpy(dest, src, length);
    src += period;
    dest += length;
  }

  /* copy the peripheral information */
  for (i=0; i<=nout->dim-1; i++) {
    map[i] = i + (i >= axis);
  }
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

  return(0);
}

int
nrrdCrop(Nrrd *nout, Nrrd *nin, int *min, int *max) {
  char me[]="nrrdCrop", err[NRRD_STRLEN_MED];
  int d,                     /* tmp */   
    lineSize,                /* #bytes in one scanline to be copied */
    cIn[NRRD_DIM_MAX],       /* coords for line start, in input */
    cOut[NRRD_DIM_MAX],      /* coords for line start, in output */
    szIn[NRRD_DIM_MAX],
    szOut[NRRD_DIM_MAX];
  nrrdBigInt I,
    idxIn, idxOut,           /* linear indices for input and output */
    numOut,                  /* number of elements in output nrrd */
    numLines;                /* number of scanlines in output nrrd */
  char *dataIn, *dataOut;

  /* errors */
  if (!(nout && nin && min && max)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  for (d=0; d<=nin->dim-1; d++) {
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

  /* allocate */
  numOut = numLines = 1;
  for (d=0; d<=nin->dim-1; d++) {
    nout->axis[d].size = max[d] - min[d] + 1;
    numOut *= nout->axis[d].size;
    if (d)
      numLines *= nout->axis[d].size;
  }
  if (nrrdMaybeAlloc(nout, numOut, nin->type, nin->dim)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  lineSize = nout->axis[0].size*nrrdElementSize(nin);
  
  /* the skinny */
  memset(cOut, 0, NRRD_DIM_MAX*sizeof(int));
  dataIn = nin->data;
  dataOut = nout->data;
  nrrdAxesGet(nin, nrrdAxesInfoSize, szIn);
  nrrdAxesGet(nout, nrrdAxesInfoSize, szOut);
  for (I=0; I<=numLines-1; I++) {
    cOut[1]++; 
    NRRD_COORD_UPDATE(cOut, szOut, nout->dim, d);
    NRRD_COORD_INDEX(cOut, szOut, nout->dim, d, idxOut);
    for (d=0; d<=nin->dim-1; d++)
      cIn[d] = cOut[d] + min[d];
    NRRD_COORD_INDEX(cIn, szIn, nin->dim, d, idxIn);
    memcpy(dataOut + idxOut, dataIn + idxIn, lineSize);
  }
  
  if (nrrdAxesCopy(nout, nin, NULL, 
		   NRRD_AXESINFO_MINMAX | NRRD_AXESINFO_SIZE)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
    
  
  
  return 0;
}

/*
******** nrrdSubvolume
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
  if (nrrdMaybeAlloc(nout, num, nin->type, dim)) {
    sprintf(err, "%s: failed to create slice", me);
    biffAdd(NRRD, err); return 1;
  }
  dataOut = nout->data;
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
  nrrdAxesCopy(nout, nin, NULL, NRRD_AXESINFO_MINMAX | NRRD_AXESINFO_SIZE);
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
