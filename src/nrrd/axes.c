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

int nrrdAxisCopyStrings = AIR_TRUE;

/* ------------------------------------------------------------ */

void
_nrrdAxisInit(nrrdAxis *axis) {
  
  if (axis) {
    axis->size = 0;
    axis->spacing = AIR_NAN;
    axis->min = axis->max = AIR_NAN;
    axis->label = airFree(axis->label);
    axis->center = nrrdCenterUnknown;
  }
}

nrrdAxis *
nrrdAxisNew(void) {
  nrrdAxis *axis;

  axis = calloc(1, sizeof(nrrdAxis));
  axis->label = NULL;
  if (axis) {
    _nrrdAxisInit(axis);
  }
  return axis;
}

nrrdAxis *
nrrdAxisNix(nrrdAxis *axis) {

  if (axis) {
    axis->label = airFree(axis->label);
    airFree(axis);
  }
  return NULL;
}

/* ------------------------------------------------------------ */

/*
void
nrrdAxisCopy(nrrdAxis *ax1, nrrdAxis *ax0) {

  if (ax1 && ax0) {
    ax1->size = ax0->size;
    ax1->spacing = ax0->spacing;
    ax1->min = ax0->min;
    ax1->max = ax0->max;
    ax1->label = airStrdup(ax0->label);
    ax1->center = ax0->center;
  }
}
*/

/*
******** nrrdAxesCopy()
**
** For copying all the per-axis peripheral information.  Takes a
** permutation "map"; map[d] tells from which axis in input should the
** output axis d should copy its information.  The length of this
** permutation array is nout->dim.  If map is NULL, the identity
** permutation is assumed.  The "bitflag" field controls which
** per-axis fields will NOT be copied; if bitflag==0, then all fields
** are copied.  The value of bitflag should be ||'s of NRRD_AXESINFO_*
** defines. If map[i]==-1 for any i in [0,dim-1], then nothing for
** that dimension is copied.
**
** Decided to Not use Biff, since many times map will be NULL, in
** which case the only error is getting a NULL nrrd, which will
** probably be unlikely given the contexts in which this is called.
** The given map array can have invalid entries. For the paranoid, the
** integer return value indicates error.
*/
int
nrrdAxesCopy(Nrrd *nout, Nrrd *nin, int *map, int bitflag) {
  int d, from;
  
  if (!(nout && nin)) {
    return 1;
  }
  if (map) {
    for (d=0; d<=nout->dim-1; d++) {
      if (!AIR_INSIDE(-1, map[d], nin->dim-1)) {
	return 1;
      }
    }
  }
  
  for (d=0; d<=nout->dim-1; d++) {
    from = map ? map[d] : d;
    if (-1 == from) {
      /* for this axis, we don't touch a thing */
      continue;
    }
    if (!(NRRD_AXESINFO_SIZE & bitflag)) {
      nout->axis[d].size = nin->axis[from].size;
    }
    if (!(NRRD_AXESINFO_SPACING & bitflag)) {
      nout->axis[d].spacing = nin->axis[from].spacing;
    }
    if (!(NRRD_AXESINFO_AMIN & bitflag)) {
      nout->axis[d].min = nin->axis[from].min;
    }
    if (!(NRRD_AXESINFO_AMAX & bitflag)) {
      nout->axis[d].max = nin->axis[from].max;
    }
    if (!(NRRD_AXESINFO_CENTER & bitflag)) {
      nout->axis[d].center = nin->axis[from].center;
    }
    if (!(NRRD_AXESINFO_LABEL & bitflag)) {
      nout->axis[d].label = airFree(nout->axis[d].label);
      nout->axis[d].label = airStrdup(nin->axis[from].label);
    }
  }
  return 0;
}

/*
******** nrrdAxesSet()
**
** Simple means of setting fields of the axis array in the nrrd.
**
** NOTE: for kicks, when you set nrrdAxesInfoSize, nrrd->num
** will be updated accordingly
*/
void
nrrdAxesSet(Nrrd *nrrd, int axInfo, void *_info) {
  _nrrdAxesInfoPtrs info;
  int d;
  
  if (!( nrrd 
	 && AIR_INSIDE(1, nrrd->dim, NRRD_DIM_MAX) 
	 && AIR_BETWEEN(nrrdAxesInfoUnknown, axInfo, nrrdAxesInfoLast) 
	 && _info )) {
    return;
  }
  info.P = _info;

  if (nrrdAxesInfoSize == axInfo) {
    nrrd->num = 1;
  }
  for (d=0; d<=nrrd->dim-1; d++) {
    switch (axInfo) {
    case nrrdAxesInfoSize:
      nrrd->num *= (nrrd->axis[d].size = info.I[d]);
      break;
    case nrrdAxesInfoSpacing:
      nrrd->axis[d].spacing = info.D[d];
      break;
    case nrrdAxesInfoMin:
      nrrd->axis[d].min = info.D[d];
      break;
    case nrrdAxesInfoMax:
      nrrd->axis[d].max = info.D[d];
      break;
    case nrrdAxesInfoCenter:
      nrrd->axis[d].center = info.I[d];
      break;
    case nrrdAxesInfoLabel:
      nrrd->axis[d].label = airFree(nrrd->axis[d].label);
      nrrd->axis[d].label = airStrdup(info.CP[d]);
      break;
    }
  }
  return;
}

/*
******** nrrdAxesSet_va()
**
** var args front-end for nrrdAxesSet
*/
void
nrrdAxesSet_va(Nrrd *nrrd, int axInfo, ...) {
  void *space[NRRD_DIM_MAX];
  _nrrdAxesInfoPtrs info;
  int d;
  va_list ap;

  if (!( nrrd 
	 && AIR_INSIDE(1, nrrd->dim, NRRD_DIM_MAX) 
	 && AIR_BETWEEN(nrrdAxesInfoUnknown, axInfo, nrrdAxesInfoLast) )) {
    return;
  }

  info.P = space;
  va_start(ap, axInfo);
  for (d=0; d<=nrrd->dim-1; d++) {
    switch (axInfo) {
    case nrrdAxesInfoSize:
      info.I[d] = va_arg(ap, int);
      printf("!%s: got int[%d] = %d\n", "nrrdAxesSet_va", d, info.I[d]);
      break;
    case nrrdAxesInfoCenter:
      info.I[d] = va_arg(ap, int);
      printf("!%s: got int[%d] = %d\n", "nrrdAxesSet_va", d, info.I[d]);
      break;
    case nrrdAxesInfoSpacing:
    case nrrdAxesInfoMin:
    case nrrdAxesInfoMax:
      info.D[d] = va_arg(ap, double);
      printf("!%s: got double[%d] = %lg\n", "nrrdAxesSet_va", d, info.D[d]);
      break;
    case nrrdAxesInfoLabel:
      /* we DO NOT do the airStrdup() here because this pointer value is
	 just going to be handed to nrrdAxesSet(), which WILL do the
	 airStrdup(); we're not violating the rules for axis labels */
      info.CP[d] = va_arg(ap, char *);
      printf("!%s: got char*[%d] = |%s|\n", "nrrdAxesSet_va", d, info.CP[d]);
      break;
    }
  }
  va_end(ap);

  /* now set the quantities which we've gotten from the var args */
  nrrdAxesSet(nrrd, axInfo, info.P);
  
  return;
}

/*
******** nrrdAxesGet()
**
** get any of the axis fields into an array
**
** Note that getting axes labels involves implicitly allocating space
** for them, due to the action of airStrdup().  The user is
** responsible for freeing these strings when done with them.  
*/
void
nrrdAxesGet(Nrrd *nrrd, int axInfo, void *_info) {
  _nrrdAxesInfoPtrs info;
  int d;
  
  if (!( nrrd 
	 && AIR_INSIDE(1, nrrd->dim, NRRD_DIM_MAX) 
	 && AIR_BETWEEN(nrrdAxesInfoUnknown, axInfo, nrrdAxesInfoLast) )) {
    return;
  }
  
  info.P = _info;
  for (d=0; d<=nrrd->dim-1; d++) {
    switch (axInfo) {
    case nrrdAxesInfoSize:
      info.I[d] = nrrd->axis[d].size;
      break;
    case nrrdAxesInfoSpacing:
      info.D[d] = nrrd->axis[d].spacing;
      break;
    case nrrdAxesInfoMin:
      info.D[d] = nrrd->axis[d].min;
      break;
    case nrrdAxesInfoMax:
      info.D[d] = nrrd->axis[d].max;
      break;
    case nrrdAxesInfoCenter:
      info.I[d] = nrrd->axis[d].center;
      break;
    case nrrdAxesInfoLabel:
      /* note airStrdup()! */
      info.CP[d] = airStrdup(nrrd->axis[d].label);
      break;
    }
  }

  return;
}

void
nrrdAxesGet_va(Nrrd *nrrd, int axInfo, ...) {
  void *space[NRRD_DIM_MAX], *ptr;
  _nrrdAxesInfoPtrs info;
  int d;
  va_list ap;

  if (!( nrrd 
	 && AIR_INSIDE(1, nrrd->dim, NRRD_DIM_MAX) 
	 && AIR_BETWEEN(nrrdAxesInfoUnknown, axInfo, nrrdAxesInfoLast) )) {
    return;
  }

  info.P = space;
  nrrdAxesGet(nrrd, axInfo, info.P);

  va_start(ap, axInfo);
  for (d=0; d<=nrrd->dim-1; d++) {
    ptr = va_arg(ap, void*);
    switch (axInfo) {
    case nrrdAxesInfoSize:
      *((int*)ptr) = info.I[d];
      printf("!%s: got int[%d] = %d\n", "nrrdAxesGet_va", d, *((int*)ptr));
      break;
    case nrrdAxesInfoCenter:
      *((int*)ptr) = info.I[d];
      printf("!%s: got int[%d] = %d\n", "nrrdAxesGet_va", d, *((int*)ptr));
      break;
    case nrrdAxesInfoSpacing:
    case nrrdAxesInfoMin:
    case nrrdAxesInfoMax:
      *((double*)ptr) = info.D[d];
      printf("!%s: got double[%d] = %lg\n", "nrrdAxesGet_va", d, 
	     *((double*)ptr));
      break;
    case nrrdAxesInfoLabel:
      /* we DO NOT do the airStrdup() here because this pointer value just
	 came from nrrdAxesGet(), which already did the airStrdup() */
      *((char**)ptr) = info.CP[d];
      printf("!%s: got char*[%d] = |%s|\n", "nrrdAxesSet_va", d, 
	     *((char**)ptr));
      break;
    }
  }
  va_end(ap);
  
  return;
}

int
_nrrdAxisMinMaxHelp(double *minIdxP, double *maxIdxP, int *centerP,
		    Nrrd *nrrd, int ax) {
  char me[]="_nrrdAxisMinMaxHelp";
  int center;
  int size;
  double tmp;

  /* bail on invalid input */
  if (!( nrrd 
	 && AIR_INSIDE(0, ax, nrrd->dim-1)
	 && AIR_EXISTS(nrrd->axis[ax].min)
	 && AIR_EXISTS(nrrd->axis[ax].max) )) {
    return 1;
  }
  
  /* set center and size */
  if (nrrdCenterUnknown == nrrd->axis[ax].center) {
    center = nrrdDefCenter;
  }
  else {
    center = nrrd->axis[ax].center;
  }
  center = AIR_CLAMP(nrrdCenterUnknown + 1, center, nrrdCenterLast - 1);
  *centerP = center;
  size = AIR_MAX(1, nrrd->axis[ax].size);

  /* determine min and max index space locations corresponding to the
     min and max axis values */
  if (nrrdCenterNode == center) {
    *minIdxP = 0;
    *maxIdxP = size - 1;
  }
  else if (nrrdCenterCell == center) {
    *minIdxP = -0.5;
    *maxIdxP = size - 0.5;
  }
  else {
    fprintf(stderr, "%s: PANIC: center %d unimplemented\n", me, center);
    exit(1);
  }
  
  /* flip minIdx and maxIdx if axis min > axis max */
  if (nrrd->axis[ax].min > nrrd->axis[ax].max) {
    tmp = *minIdxP; *minIdxP = *maxIdxP; *maxIdxP = tmp;
  }
  return 0;
}

double
nrrdAxisLocation(Nrrd *nrrd, int ax, double idx) {
  int center;
  double minIdx, maxIdx;
  
  /* might as well return NaN if we can't get the min
     and max indices right, or if given idx doesn't exist */
  if ( _nrrdAxisMinMaxHelp(&minIdx, &maxIdx, &center, nrrd, ax)
       || !AIR_EXISTS(idx) ) {
    return AIR_NAN;
  }

  return AIR_AFFINE(minIdx, idx, maxIdx, 
		    nrrd->axis[ax].min, nrrd->axis[ax].max);
}

void
nrrdAxisRange(double *loP, double *hiP,
	      Nrrd *nrrd, int ax, 
	      double loIdx, double hiIdx) {
  int center;
  double min, max, minIdx, maxIdx, half;
  
  if (_nrrdAxisMinMaxHelp(&minIdx, &maxIdx, &center, nrrd, ax)
      || !AIR_EXISTS(loIdx)
      || !AIR_EXISTS(hiIdx)) {
    *loP = *hiP = AIR_NAN;
    return;
  }

  min = nrrd->axis[ax].min;
  max = nrrd->axis[ax].max;
  *loP = AIR_AFFINE(minIdx, loIdx, maxIdx, min, max);
  *hiP = AIR_AFFINE(minIdx, hiIdx, maxIdx, min, max);
  
  if (nrrdCenterCell == center) {
    half = (max - min)/nrrd->axis[ax].size;
    if (*loP <= *hiP) {
      *loP -= half;
      *hiP += half;
    }
    else {
      *loP += half;
      *hiP -= half;
    }
  }
  
  return;
}
