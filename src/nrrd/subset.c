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
** this isn't used any more.  Thank God.  Terrible, terrible idea.
*/
void
_nrrdSelectElements(void *dataIn, void *dataOut, int size,
	       NRRD_BIG_INT *idx, NRRD_BIG_INT num) {
  NRRD_BIG_INT i, o;

  o = 0;
  for (i=0; i<=num-1; i++) {
    memcpy((char*)dataOut + (o++)*size, 
	   (char*)dataIn + idx[i]*size, size);
  }
}

/*
******** nrrdSample()
**
** given coordinates within a nrrd, copies the 
** single element into given *val
*/
int
nrrdSample(Nrrd *nrrd, int *coord, void *val) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdSample";
  int size, dim, i, idx;
  
  if (!(nrrd && coord && val && 
	nrrd->type > nrrdTypeUnknown &&
	nrrd->type < nrrdTypeLast)) {
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  size = nrrdElementSize(nrrd);
  dim = nrrd->dim;
  for (i=0; i<=dim-1; i++) {
    if (!(AIR_INSIDE(0, coord[i], nrrd->size[i]-1))) {
      sprintf(err, "%s: coordinate %d on axis %d out of bounds (0 to %d)", 
	      me, coord[i], i, nrrd->size[i]-1);
      biffSet(NRRD, err); return 1;
    }
  }
  idx = coord[dim-1];
  for (i=dim-2; i>=0; i--) {
    idx = coord[i] + nrrd->size[i]*idx;
  }
  memcpy((char*)val, (char*)(nrrd->data) + idx*size, size);
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
nrrdSlice(Nrrd *nin, Nrrd *nout, int axis, int pos) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdSlice";
  NRRD_BIG_INT 
    I, 
    offset,                  /* index of first segment of slice */
    length,                  /* length of segment */
    period,                  /* distance between start of each segment */
    numper;                  /* number of periods */
  int i;
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
  if (!(AIR_INSIDE(0, pos, nin->size[axis]-1) )) {
    sprintf(err, "%s: position %d out of bounds (0 to %d)", 
	    me, axis, nin->size[axis]-1);
    biffSet(NRRD, err); return 1;
  }

  /* set up control variables */
  length = numper = 1;
  for (i=0; i<=nin->dim-1; i++) {
    if (i < axis) {
      length *= nin->size[i];
    }
    else if (i > axis) {
      numper *= nin->size[i];
    }
  }
  length *= nrrdElementSize(nin);
  offset = length*pos;
  period = length*nin->size[axis];

  /* allocate space if necessary */
  if (!(nout->data)) {
    if (nrrdAlloc(nout, nin->num/nin->size[axis], 
		  nin->type, nin->dim-1)) {
      sprintf(err, "%s: nrrdAlloc() failed to create slice", me);
      biffAdd(NRRD, err); return 1;
    }
  }

  /* here's the skinny: copy the data */
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
    nout->size[i] = nin->size[i + (i >= axis)];
    nout->spacing[i] = nin->spacing[i + (i >= axis)];
    nout->axisMin[i] = nin->axisMin[i + (i >= axis)];
    nout->axisMax[i] = nin->axisMax[i + (i >= axis)];
    strcpy(nout->label[i], nin->label[i + (i >= axis)]);
  }
  sprintf(nout->content, "slice(%s,%d,%d)", 
	  nin->content, axis, pos);
  nout->blockSize = nin->blockSize;
  nin->min = airNand();
  nin->max = airNand();

  return(0);
}

/*
******** nrrdNewSlice()
**
** slicer which calls nrrdNew first
*/
Nrrd *
nrrdNewSlice(Nrrd *nin, int axis, int pos) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdNewSlice";
  Nrrd *nout;

  if (!(nout = nrrdNew())) {
    sprintf(err, "%s: nrrdNew() failed", me);
    biffAdd(NRRD, err);
    return NULL;
  }
  if (nrrdSlice(nin, nout, axis, pos)) {
    sprintf(err, "%s: nrrdSlice() failed", me);
    biffAdd(NRRD, err);
    nrrdNuke(nout);
    return NULL;
  }
  return nout;
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
nrrdSubvolume(Nrrd *nin, Nrrd *nout, int *min, int *max, int clamp) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdSubvolume", tmpstr[512];
  NRRD_BIG_INT num,          /* number of elements in volume */
    i,                       /* index through elements in slice */
    idx;                     /* index into original array */
  char *dataIn, *dataOut;
  int len[NRRD_MAX_DIM],
    outside,                 /* current subvolume point is outside original */
    dim,                     /* dimension of volume */
    d,                       /* running index along dimensions */
    elSize,                  /* size of one element */

    coord[NRRD_MAX_DIM];     /* array of original array coordinates */

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
  if (!(nout->data)) {
    if (nrrdAlloc(nout, num, nin->type, dim)) {
      sprintf(err, "%s: nrrdAlloc() failed to create slice", me);
      biffAdd(NRRD, err); return 1;
    }
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
      if (!AIR_INSIDE(0, coord[d], nin->size[d]-1)) {
	if (clamp) {
	  coord[d] = AIR_CLAMP(0, coord[d], nin->size[d]-1);
	}
	else {
	  /* we allow the coord[] array and idx to hold bogus values 
	     (which we'll ignore) */
	  outside = 1;
	}
      }
      idx = coord[d] + nin->size[d]*idx;
    }
    if (outside) {
      nrrdFInsert[nin->type](dataOut, i, 0.0);
    }
    else {
      memcpy(dataOut + i*elSize, dataIn + idx*elSize, elSize);
    }
  }

  /* set information in subvolume */
  for (d=0; d<=dim-1; d++) {
    nout->size[d] = len[d];
    nout->spacing[d] = nin->spacing[d];
    nout->axisMin[d] = AIR_AFFINE(0, min[d], len[d]-1,
				  nin->axisMin[d], nin->axisMax[d]);
    nout->axisMax[d] = AIR_AFFINE(0, max[d], len[d]-1,
				  nin->axisMin[d], nin->axisMax[d]);
    strcpy(nout->label[d], nin->label[d]);
  }
  sprintf(nout->content, "crop(%s,", nin->content);
  for (d=0; d<=dim-1; d++) {
    sprintf(tmpstr, "%d-%d%c", min[d], max[d], d == dim-1 ? ')' : ',');
    strcat(nout->content, tmpstr);
  }
  nout->blockSize = nin->blockSize;
  nin->min = airNand();
  nin->max = airNand();

  return 0;
}

/*
******** nrrdNewSubvolume
**
*/
Nrrd *
nrrdNewSubvolume(Nrrd *nin, int *min, int *max, int clamp) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdNewSubvolume";
  Nrrd *nout;

  if (!(nout = nrrdNew())) {
    sprintf(err, "%s: nrrdNew() failed", me);
    biffAdd(NRRD, err);
    return NULL;
  }
  if (nrrdSubvolume(nin, nout, min, max, clamp)) {
    sprintf(err, "%s: nrrdSubvolume() failed", me);
    biffAdd(NRRD, err);
    nrrdNuke(nout);
    return NULL;
  }
  return nout;
}

