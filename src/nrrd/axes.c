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

void
_nrrdAxisInit(nrrdAxis *axis) {
  
  if (axis) {
    axis->size = -1;
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

  return airFree(axis);
}

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
** permutation array is nout->dim.  If map is NULL, the identity permutation
** is assumed.  The "flags" field controls which per-axis fields will
** NOT be copied; if flags==0, then all fields are copied
**
** Decided to Not use Biff, since many times map will be NULL, in
** which case the only error is getting a NULL nrrd, which will
** probably be unlikely given the contexts in which this is called.
** Still, for the paranoid, the integer return value indicates error.
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
      airFree(nout->axis[d].label);
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
	 && AIR_BETWEEN(1, nrrd->dim, NRRD_DIM_MAX) 
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
      airFree(nrrd->axis[d].label);
      nrrd->axis[d].label = airStrdup(info.C[d]);
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
	 && AIR_BETWEEN(1, nrrd->dim, NRRD_DIM_MAX) 
	 && AIR_BETWEEN(nrrdAxesInfoUnknown, axInfo, nrrdAxesInfoLast) )) {
    return;
  }

  info.P = space;
  va_start(ap, axInfo);
  for (d=0; d<=nrrd->dim-1; d++) {
    switch (axInfo) {
    case nrrdAxesInfoSize:
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
      info.C[d] = va_arg(ap, char *);
      printf("!%s: got char*[%d] = |%s|\n", "nrrdAxesSet_va", d, info.C[d]);
      break;
    }
  }
  va_end(ap);

  nrrdAxesSet(nrrd, axInfo, info.P);
  
  return;
}

void
nrrdAxesGet(Nrrd *nrrd, int axInfo, void *_info) {
  _nrrdAxesInfoPtrs info;
  int d;
  
  if (!( nrrd 
	 && AIR_BETWEEN(1, nrrd->dim, NRRD_DIM_MAX) 
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
      info.C[d] = nrrd->axis[d].label;
      break;
    }
  }

  return;
}

