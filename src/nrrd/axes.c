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
#include "privateNrrd.h"

/* ------------------------------------------------------------ */

void
_nrrdAxisInit(NrrdAxis *axis) {
  
  if (axis) {
    axis->size = 0;
    axis->spacing = AIR_NAN;
    axis->min = axis->max = AIR_NAN;
    axis->label = airFree(axis->label);
    axis->unit = airFree(axis->unit);
    axis->center = nrrdCenterUnknown;
  }
}

/*
nrrdAxisNew(void) {
  nrrdAxis *axis;

  axis = calloc(1, sizeof(NrrdAxis));
  axis->label = NULL;
  axis->unit = NULL;
  if (axis) {
    _nrrdAxisInit(axis);
  }
  return axis;
}

NrrdAxis *
nrrdAxisNix(NrrdAxis *axis) {

  if (axis) {
    axis->label = airFree(axis->label);
    axis->unit = airFree(axis->unit);
    airFree(axis);
  }
  return NULL;
}
*/

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
    ax1->unit = airStrdup(ax0->unit);
    ax1->center = ax0->center;
  }
}
*/

/*
******** nrrdAxesCopy()
**
** For copying all the per-axis peripheral information.  Takes a
** permutation "map"; map[d] tells from which axis in input should the
** output axis d copy its information.  The length of this permutation
** array is nout->dim.  If map is NULL, the identity permutation is
** assumed.  If map[i]==-1 for any i in [0,dim-1], then nothing for
** that dimension is copied.  The "bitflag" field controls which
** per-axis fields will NOT be copied; if bitflag==0, then all fields
** are copied.  The value of bitflag should be |'s of NRRD_AXESINFO_*
** defines.
**
** Decided to Not use Biff, since many times map will be NULL, in
** which case the only error is getting a NULL nrrd, or an invalid map
** permutation, which will probably be unlikely given the contexts in
** which this is called.  For the paranoid, the integer return value
** indicates error.
*/
int
nrrdAxesCopy(Nrrd *nout, Nrrd *nin, int *map, int bitflag) {
  int d, from;
  
  if (!(nout && nin)) {
    return 1;
  }
  if (nout == nin) {
    /* this is a problem because we don't want to have to deal with
       temp variables to re-arrange the axes */
    return 2;
  }
  if (map) {
    for (d=0; d<nout->dim; d++) {
      if (-1 == map[d]) {
	continue;
      }
      if (!AIR_IN_CL(0, map[d], nin->dim-1)) {
	return 3;
      }
    }
  }
  
  for (d=0; d<nout->dim; d++) {
    from = map ? map[d] : d;
    if (-1 == from) {
      /* for this axis, we don't touch a thing */
      continue;
    }
    if (!(NRRD_AXESINFO_SIZE_BIT & bitflag)) {
      nout->axis[d].size = nin->axis[from].size;
    }
    if (!(NRRD_AXESINFO_SPACING_BIT & bitflag)) {
      nout->axis[d].spacing = nin->axis[from].spacing;
    }
    if (!(NRRD_AXESINFO_MIN_BIT & bitflag)) {
      nout->axis[d].min = nin->axis[from].min;
    }
    if (!(NRRD_AXESINFO_MAX_BIT & bitflag)) {
      nout->axis[d].max = nin->axis[from].max;
    }
    if (!(NRRD_AXESINFO_CENTER_BIT & bitflag)) {
      nout->axis[d].center = nin->axis[from].center;
    }
    if (!(NRRD_AXESINFO_LABEL_BIT & bitflag)) {
      nout->axis[d].label = airFree(nout->axis[d].label);
      nout->axis[d].label = airStrdup(nin->axis[from].label);
    }
    if (!(NRRD_AXESINFO_UNIT_BIT & bitflag)) {
      nout->axis[d].unit = airFree(nout->axis[d].unit);
      nout->axis[d].unit = airStrdup(nin->axis[from].unit);
    }
  }
  return 0;
}

/*
******** nrrdAxesSet_nva()
**
** Simple means of setting fields of the axis array in the nrrd.
*/
void
nrrdAxesSet_nva(Nrrd *nrrd, int axInfo, void *_info) {
  _nrrdAxesInfoPtrs info;
  int d;
  
  if (!( nrrd 
	 && AIR_IN_CL(1, nrrd->dim, NRRD_DIM_MAX) 
	 && AIR_IN_OP(nrrdAxesInfoUnknown, axInfo, nrrdAxesInfoLast) 
	 && _info )) {
    return;
  }
  info.P = _info;

  for (d=0; d<nrrd->dim; d++) {
    switch (axInfo) {
    case nrrdAxesInfoSize:
      nrrd->axis[d].size = info.I[d];
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
    case nrrdAxesInfoUnit:
      nrrd->axis[d].unit = airFree(nrrd->axis[d].unit);
      nrrd->axis[d].unit = airStrdup(info.CP[d]);
      break;
    }
  }
  return;
}

/*
******** nrrdAxesSet()
**
** var args front-end for nrrdAxesSet_nva
*/
void
nrrdAxesSet(Nrrd *nrrd, int axInfo, ...) {
  NRRD_TYPE_BIGGEST *space[NRRD_DIM_MAX];
  _nrrdAxesInfoPtrs info;
  int d;
  va_list ap;

  if (!( nrrd 
	 && AIR_IN_CL(1, nrrd->dim, NRRD_DIM_MAX) 
	 && AIR_IN_OP(nrrdAxesInfoUnknown, axInfo, nrrdAxesInfoLast) )) {
    return;
  }

  info.P = space;
  va_start(ap, axInfo);
  for (d=0; d<nrrd->dim; d++) {
    switch (axInfo) {
    case nrrdAxesInfoSize:
      info.I[d] = va_arg(ap, int);
      /*
      printf("!%s: got int[%d] = %d\n", "nrrdAxesSet_va", d, info.I[d]);
      */
      break;
    case nrrdAxesInfoCenter:
      info.I[d] = va_arg(ap, int);
      /*
      printf("!%s: got int[%d] = %d\n", 
	     "nrrdAxesSet_va", d, info.I[d]);
      */
      break;
    case nrrdAxesInfoSpacing:
    case nrrdAxesInfoMin:
    case nrrdAxesInfoMax:
      info.D[d] = va_arg(ap, double);
      /*
      printf("!%s: got double[%d] = %g\n", 
	     "nrrdAxesSet_va", d, info.D[d]); 
      */
      break;
    case nrrdAxesInfoLabel:
      /* we DO NOT do the airStrdup() here because this pointer value is
	 just going to be handed to nrrdAxesSet_nva(), which WILL do the
	 airStrdup(); we're not violating the rules for axis labels */
      info.CP[d] = va_arg(ap, char *);
      /*
      printf("!%s: got char*[%d] = |%s|\n", 
	     "nrrdAxesSet_va", d, info.CP[d]);
      */
      break;
    case nrrdAxesInfoUnit:
      /* same explanation as above */
      info.CP[d] = va_arg(ap, char *);
      break;
    }
  }
  va_end(ap);

  /* now set the quantities which we've gotten from the var args */
  nrrdAxesSet_nva(nrrd, axInfo, info.P);
  
  return;
}

/*
******** nrrdAxesGet_nva()
**
** get any of the axis fields into an array
**
** Note that getting axes labels and untis involves implicitly
** allocating space for them, due to the action of airStrdup().  The
** user is responsible for free()ing these strings when done with
** them.
*/
void
nrrdAxesGet_nva(Nrrd *nrrd, int axInfo, void *_info) {
  _nrrdAxesInfoPtrs info;
  int d;
  
  if (!( nrrd 
	 && AIR_IN_CL(1, nrrd->dim, NRRD_DIM_MAX) 
	 && AIR_IN_OP(nrrdAxesInfoUnknown, axInfo, nrrdAxesInfoLast) )) {
    return;
  }
  
  info.P = _info;
  for (d=0; d<nrrd->dim; d++) {
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
    case nrrdAxesInfoUnit:
      /* note airStrdup()! */
      info.CP[d] = airStrdup(nrrd->axis[d].unit);
      break;
    }
  }

  return;
}

void
nrrdAxesGet(Nrrd *nrrd, int axInfo, ...) {
  void *space[NRRD_DIM_MAX], *ptr;
  _nrrdAxesInfoPtrs info;
  int d;
  va_list ap;

  if (!( nrrd 
	 && AIR_IN_CL(1, nrrd->dim, NRRD_DIM_MAX) 
	 && AIR_IN_OP(nrrdAxesInfoUnknown, axInfo, nrrdAxesInfoLast) )) {
    return;
  }

  info.P = space;
  nrrdAxesGet_nva(nrrd, axInfo, info.P);

  va_start(ap, axInfo);
  for (d=0; d<nrrd->dim; d++) {
    ptr = va_arg(ap, void*);
    switch (axInfo) {
    case nrrdAxesInfoSize:
      *((int*)ptr) = info.I[d];
      /* printf("!%s: got int[%d] = %d\n", 
	 "nrrdAxesGet_va", d, *((int*)ptr)); */
      break;
    case nrrdAxesInfoSpacing:
    case nrrdAxesInfoMin:
    case nrrdAxesInfoMax:
      *((double*)ptr) = info.D[d];
      /* printf("!%s: got double[%d] = %lg\n", "nrrdAxesGet_va", d,
       *((double*)ptr)); */
      break;
    case nrrdAxesInfoCenter:
      *((int*)ptr) = info.I[d];
      /* printf("!%s: got int[%d] = %d\n", 
	 "nrrdAxesGet_va", d, *((int*)ptr)); */
      break;
    case nrrdAxesInfoLabel:
      /* we DO NOT do the airStrdup() here because this pointer value just
	 came from nrrdAxesGet(), which already did the airStrdup() */
      *((char**)ptr) = info.CP[d];
      /* printf("!%s: got char*[%d] = |%s|\n", "nrrdAxesSet_va", d, 
       *((char**)ptr)); */
      break;
    case nrrdAxesInfoUnit:
      /* same explanation as above */
      *((char**)ptr) = info.CP[d];
      break;
    }
  }
  va_end(ap);
  
  return;
}

/*
** _nrrdCenter()
**
** for nrrdCenterCell and nrrdCenterNode, return will be the same
** as input.  Converts nrrdCenterUnknown into nrrdDefCenter,
** and then clamps to (nrrdCenterUnknown+1, nrrdCenterLast-1).
**
** Thus, this ALWAYS returns nrrdCenterNode or nrrdCenterCell
** (as long as those are the only two centering schemes).
*/
int
_nrrdCenter(int center) {
  
  center =  (nrrdCenterUnknown == center
	     ? nrrdDefCenter
	     : center);
  center = AIR_CLAMP(nrrdCenterUnknown+1, center, nrrdCenterLast-1);
  return center;
}

int
_nrrdCenter2(int center, int defCenter) {
  
  center =  (nrrdCenterUnknown == center
	     ? defCenter
	     : center);
  center = AIR_CLAMP(nrrdCenterUnknown+1, center, nrrdCenterLast-1);
  return center;
}


/*
******** nrrdAxisPos()
** 
** given a nrrd, an axis, and a (floating point) index space position,
** return the position implied the axis's min, max, and center
** Does the opposite of nrrdAxisIdx().
**
** does not use biff
*/
double
nrrdAxisPos(Nrrd *nrrd, int ax, double idx) {
  int center, size;
  double min, max;
  
  if (!( nrrd && AIR_IN_CL(0, ax, nrrd->dim-1) )) {
    return AIR_NAN;
  }
  center = _nrrdCenter(nrrd->axis[ax].center);
  min = nrrd->axis[ax].min;
  max = nrrd->axis[ax].max;
  size = nrrd->axis[ax].size;
  
  return NRRD_POS(center, min, max, size, idx);
}

/*
******** nrrdAxisIdx()
** 
** given a nrrd, an axis, and a (floating point) world space position,
** return the index implied the axis's min, max, and center.
** Does the opposite of nrrdAxisPos().
**
** does not use biff
*/
double
nrrdAxisIdx(Nrrd *nrrd, int ax, double pos) {
  int center, size;
  double min, max;
  
  if (!( nrrd && AIR_IN_CL(0, ax, nrrd->dim-1) )) {
    return AIR_NAN;
  }
  center = _nrrdCenter(nrrd->axis[ax].center);
  min = nrrd->axis[ax].min;
  max = nrrd->axis[ax].max;
  size = nrrd->axis[ax].size;

  return NRRD_IDX(center, min, max, size, pos);
}

/*
******** nrrdAxisPosRange()
**
** given a nrrd, an axis, and two (floating point) index space positions,
** return the range of positions implied the axis's min, max, and center
** The opposite of nrrdAxisIdxRange()
*/
void
nrrdAxisPosRange(double *loP, double *hiP, Nrrd *nrrd, int ax, 
		 double loIdx, double hiIdx) {
  int center, size, flip = 0;
  double min, max, tmp;

  if (!( loP && hiP && nrrd && AIR_IN_CL(0, ax, nrrd->dim-1) )) {
    *loP = *hiP = AIR_NAN;
    return;
  }
  center = _nrrdCenter(nrrd->axis[ax].center);
  min = nrrd->axis[ax].min;
  max = nrrd->axis[ax].max;
  size = nrrd->axis[ax].size;

  if (loIdx > hiIdx) {
    flip = 1;
    tmp = loIdx; loIdx = hiIdx; hiIdx = tmp;
  }
  if (nrrdCenterCell == center) {
    *loP = AIR_AFFINE(0, loIdx, size, min, max);
    *hiP = AIR_AFFINE(0, hiIdx+1, size, min, max);
  } else {
    *loP = AIR_AFFINE(0, loIdx, size-1, min, max);
    *hiP = AIR_AFFINE(0, hiIdx, size-1, min, max);
  }
  if (flip) {
    tmp = *loP; *loP = *hiP; *hiP = tmp;
  }

  return;
}

/*
******** nrrdAxisIdxRange()
**
** given a nrrd, an axis, and two (floating point) world space positions,
** return the range of index space implied the axis's min, max, and center
** The opposite of nrrdAxisPosRange().
**
** Actually- there are situations where sending an interval through
** nrrdAxisIdxRange -> nrrdAxisPosRange -> nrrdAxisIdxRange
** such as in cell centering, when the range of positions given does
** not even span one sample.  Such as:
** axis->size = 4, axis->min = -4, axis->max = 4, loPos = 0, hiPos = 1
** --> nrrdAxisIdxRange == (2, 1.5) --> nrrdAxisPosRange == (2, -1)
** The basic problem is that because of the 0.5 offset inherent in
** cell centering, there are situations where (in terms of the arguments
** to nrrdAxisIdxRange()) loPos < hiPos, but *loP > *hiP.
*/
void
nrrdAxisIdxRange(double *loP, double *hiP, Nrrd *nrrd, int ax, 
		 double loPos, double hiPos) {
  int center, size, flip = 0;
  double min, max, tmp;

  if (!( loP && hiP && nrrd && AIR_IN_CL(0, ax, nrrd->dim-1) )) {
    *loP = *hiP = AIR_NAN;
    return;
  }
  center = _nrrdCenter(nrrd->axis[ax].center);
  min = nrrd->axis[ax].min;
  max = nrrd->axis[ax].max;
  size = nrrd->axis[ax].size;

  if (loPos > hiPos) {
    flip = 1;
    tmp = loPos; loPos = hiPos; hiPos = tmp;
  }
  if (nrrdCenterCell == center) {
    if (min < max) {
      *loP = AIR_AFFINE(min, loPos, max, 0, size);
      *hiP = AIR_AFFINE(min, hiPos, max, -1, size-1);
    } else {
      *loP = AIR_AFFINE(min, loPos, max, -1, size-1);
      *hiP = AIR_AFFINE(min, hiPos, max, 0, size);
    }
  } else {
    *loP = AIR_AFFINE(min, loPos, max, 0, size-1);
    *hiP = AIR_AFFINE(min, hiPos, max, 0, size-1);
  }
  if (flip) {
    tmp = *loP; *loP = *hiP; *hiP = tmp;
  }

  return;
}

void
nrrdAxisSpacingSet(Nrrd *nrrd, int ax) {
  int sign;
  double min, max, tmp;

  if (!( nrrd && AIR_IN_CL(0, ax, nrrd->dim-1) ))
    return;
  
  min = nrrd->axis[ax].min;
  max = nrrd->axis[ax].max;
  if (!( AIR_EXISTS(min) && AIR_EXISTS(max) )) {
    /* there's no actual basis on which to set the spacing information,
       but we have to set it something, so here goes ... */
    nrrd->axis[ax].spacing = nrrdDefSpacing;
    return;
  }

  if (min > max) {
    tmp = min; min = max; max = tmp;
    sign = -1;
  } else {
    sign = 1;
  }

  /* the skinny */
  nrrd->axis[ax].spacing = NRRD_SPACING(_nrrdCenter(nrrd->axis[ax].center),
					min, max, nrrd->axis[ax].size);

  return;
}

void
nrrdAxisMinMaxSet(Nrrd *nrrd, int ax, int defCenter) {
  int center;
  double spacing;

  if (!( nrrd && AIR_IN_CL(0, ax, nrrd->dim-1) ))
    return;
  
  center = _nrrdCenter2(nrrd->axis[ax].center, defCenter);
  spacing = nrrd->axis[ax].spacing;
  if (!AIR_EXISTS(spacing))
    spacing = nrrdDefSpacing;
  if (nrrdCenterCell == center) {
    nrrd->axis[ax].min = 0;
    nrrd->axis[ax].max = spacing*nrrd->axis[ax].size;
  } else {
    nrrd->axis[ax].min = 0;
    nrrd->axis[ax].max = spacing*(nrrd->axis[ax].size - 1);
  }
  
  return;
}
