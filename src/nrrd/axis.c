/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "nrrd.h"
#include "privateNrrd.h"

/* ------------------------------------------------------------ */

void
_nrrdAxisInfoInit(NrrdAxisInfo *axis) {
  int dd;

  if (axis) {
    axis->size = 0;
    axis->spacing = axis->thickness = AIR_NAN;
    axis->min = axis->max = AIR_NAN;
    for (dd = 0; dd < NRRD_SPACE_DIM_MAX; dd++) {
      axis->spaceDirection[dd] = AIR_NAN;
    }
    axis->center = nrrdCenterUnknown;
    axis->kind = nrrdKindUnknown;
    axis->label = (char *)airFree(axis->label);
    axis->units = (char *)airFree(axis->units);
  }
}

void
_nrrdAxisInfoNewInit(NrrdAxisInfo *axis) {

  if (axis) {
    axis->label = NULL;
    axis->units = NULL;
    _nrrdAxisInfoInit(axis);
  }
}

/* ------------------------------------------------------------ */

/*
******** nrrdKindIsDomain
**
** returns non-zero for kinds (from nrrdKind* enum) that are domain
** axes, or independent variable axes, or resample-able axes, all
** different ways of describing the same thing
*/
int /* Biff: nope */
nrrdKindIsDomain(int kind) {

  return (nrrdKindDomain == kind || nrrdKindSpace == kind || nrrdKindTime == kind);
}

/*
******** nrrdKindSize
**
** returns suggested size (length) of an axis with the given kind, or,
** 0 if either (1) there is no suggested size because the axis is the
** kind of an independent or domain variable or (2) the kind is invalid
*/
unsigned int /* Biff: nope */
nrrdKindSize(int kind) {
  static const char me[] = "nrrdKindSize";
  unsigned int ret;

  if (!(AIR_IN_OP(nrrdKindUnknown, kind, nrrdKindLast))) {
    /* they gave us invalid or unknown kind */
    return 0;
  }

  switch (kind) {
  case nrrdKindDomain:
  case nrrdKindSpace:
  case nrrdKindTime:
  case nrrdKindList:
  case nrrdKindPoint:
  case nrrdKindVector:
  case nrrdKindCovariantVector:
  case nrrdKindNormal:
    ret = 0;
    break;
  case nrrdKindStub:
  case nrrdKindScalar:
    ret = 1;
    break;
  case nrrdKindComplex:
  case nrrdKind2Vector:
    ret = 2;
    break;
  case nrrdKind3Color:
  case nrrdKindRGBColor:
  case nrrdKindHSVColor:
  case nrrdKindXYZColor:
    ret = 3;
    break;
  case nrrdKind4Color:
  case nrrdKindRGBAColor:
    ret = 4;
    break;
  case nrrdKind3Vector:
  case nrrdKind3Normal:
    ret = 3;
    break;
  case nrrdKind4Vector:
  case nrrdKindQuaternion:
    ret = 4;
    break;
  case nrrdKind2DSymMatrix:
    ret = 3;
    break;
  case nrrdKind2DMaskedSymMatrix:
    ret = 4;
    break;
  case nrrdKind2DMatrix:
    ret = 4;
    break;
  case nrrdKind2DMaskedMatrix:
    ret = 5;
    break;
  case nrrdKind3DSymMatrix:
    ret = 6;
    break;
  case nrrdKind3DMaskedSymMatrix:
    ret = 7;
    break;
  case nrrdKind3DMatrix:
    ret = 9;
    break;
  case nrrdKind3DMaskedMatrix:
    ret = 10;
    break;
  default:
    fprintf(stderr, "%s: PANIC: nrrdKind %d not implemented!\n", me, kind);
    ret = UINT_MAX;
  }

  return ret;
}

/*
** _nrrdKindAltered:
**
** implements logic for how kind should be updated when samples
** along the axis are altered
*/
int /* Biff: (private) nope */
_nrrdKindAltered(int kindIn, int resampling) {
  int kindOut;

  if (nrrdStateKindNoop) {
    kindOut = nrrdKindUnknown;
    /* HEY: setting the kindOut to unknown is arguably not a no-op.
       It is more like pointedly and stubbornly simplistic. So maybe
       nrrdStateKindNoop could be renamed .. */
  } else {
    if (nrrdKindIsDomain(kindIn) || (0 == nrrdKindSize(kindIn) && !resampling)) {
      kindOut = kindIn;
    } else {
      kindOut = nrrdKindUnknown;
    }
  }
  return kindOut;
}

/*
** _nrrdAxisInfoCopy
**
** HEY: we have a void return even though this function potentially
** involves calling airStrdup!!
*/
void
_nrrdAxisInfoCopy(NrrdAxisInfo *dest, const NrrdAxisInfo *src, int bitflag) {
  int ii;

  if (!(NRRD_AXIS_INFO_SIZE_BIT & bitflag)) {
    dest->size = src->size;
  }
  if (!(NRRD_AXIS_INFO_SPACING_BIT & bitflag)) {
    dest->spacing = src->spacing;
  }
  if (!(NRRD_AXIS_INFO_THICKNESS_BIT & bitflag)) {
    dest->thickness = src->thickness;
  }
  if (!(NRRD_AXIS_INFO_MIN_BIT & bitflag)) {
    dest->min = src->min;
  }
  if (!(NRRD_AXIS_INFO_MAX_BIT & bitflag)) {
    dest->max = src->max;
  }
  if (!(NRRD_AXIS_INFO_SPACEDIRECTION_BIT & bitflag)) {
    for (ii = 0; ii < NRRD_SPACE_DIM_MAX; ii++) {
      dest->spaceDirection[ii] = src->spaceDirection[ii];
    }
  }
  if (!(NRRD_AXIS_INFO_CENTER_BIT & bitflag)) {
    dest->center = src->center;
  }
  if (!(NRRD_AXIS_INFO_KIND_BIT & bitflag)) {
    dest->kind = src->kind;
  }
  if (!(NRRD_AXIS_INFO_LABEL_BIT & bitflag)) {
    if (dest->label != src->label) {
      dest->label = (char *)airFree(dest->label);
      dest->label = (char *)airStrdup(src->label);
    }
  }
  if (!(NRRD_AXIS_INFO_UNITS_BIT & bitflag)) {
    if (dest->units != src->units) {
      dest->units = (char *)airFree(dest->units);
      dest->units = (char *)airStrdup(src->units);
    }
  }

  return;
}

/*
******** nrrdAxisInfoCopy()
**
** For copying all the per-axis peripheral information.  Takes a
** permutation "map"; map[d] tells from which axis in input should the
** output axis d copy its information.  The length of this permutation
** array is nout->dim.  If map is NULL, the identity permutation is
** assumed.  If map[i]==-1 for any i in [0,dim-1], then nothing is
** copied into axis i of output.  The "bitflag" field controls which
** per-axis fields will NOT be copied; if bitflag==0, then all fields
** are copied.  The value of bitflag should be |'s of NRRD_AXIS_INFO_*
** defines.
**
** Decided to Not use Biff, since many times map will be NULL, in
** which case the only error is getting a NULL nrrd, or an invalid map
** permutation, which will probably be unlikely given the contexts in
** which this is called.  For the paranoid, the integer return value
** indicates error.
**
** Sun Feb 27 21:12:57 EST 2005: decided to allow nout==nin, so now
** use a local array of NrrdAxisInfo as buffer.
*/
int /* Biff: nope */
nrrdAxisInfoCopy(Nrrd *nout, const Nrrd *nin, const int *axmap, int bitflag) {
  NrrdAxisInfo axisBuffer[NRRD_DIM_MAX];
  const NrrdAxisInfo *axis;
  unsigned int from, axi;

  if (!(nout && nin)) {
    return 1;
  }
  if (axmap) {
    for (axi = 0; axi < nout->dim; axi++) {
      if (-1 == axmap[axi]) {
        continue;
      }
      if (!AIR_IN_CL(0, axmap[axi], (int)nin->dim - 1)) {
        return 3;
      }
    }
  }
  if (nout == nin) {
    /* copy axis info to local buffer */
    for (axi = 0; axi < nin->dim; axi++) {
      _nrrdAxisInfoNewInit(axisBuffer + axi);
      _nrrdAxisInfoCopy(axisBuffer + axi, nin->axis + axi, bitflag);
    }
    axis = axisBuffer;
  } else {
    axis = nin->axis;
  }
  for (axi = 0; axi < nout->dim; axi++) {
    if (axmap && -1 == axmap[axi]) {
      /* for this axis, we don't touch a thing */
      continue;
    }
    from = axmap ? (unsigned int)axmap[axi] : axi;
    _nrrdAxisInfoCopy(nout->axis + axi, axis + from, bitflag);
  }
  if (nout == nin) {
    /* free dynamically allocated stuff */
    for (axi = 0; axi < nin->dim; axi++) {
      _nrrdAxisInfoInit(axisBuffer + axi);
    }
  }
  return 0;
}

/*
******** nrrdAxisInfoSet_nva()
**
** Simple means of setting fields of the axis array in the nrrd.
**
** type to pass for third argument:
**           nrrdAxisInfoSize: size_t*
**        nrrdAxisInfoSpacing: double*
**      nrrdAxisInfoThickness: double*
**            nrrdAxisInfoMin: double*
**            nrrdAxisInfoMax: double*
** nrrdAxisInfoSpaceDirection: double (*var)[NRRD_SPACE_DIM_MAX]
**         nrrdAxisInfoCenter: int*
**           nrrdAxisInfoKind: int*
**          nrrdAxisInfoLabel: char**
**          nrrdAxisInfoUnits: char**
**
** Note that in the case of nrrdAxisInfoSpaceDirection, we only access
** spaceDim elements of info.V[ai] (so caller can allocate it for less
** than NRRD_SPACE_DIM_MAX if they know what they're doing)
*/
void
nrrdAxisInfoSet_nva(Nrrd *nrrd, int axInfo, const void *_info) {
  _nrrdAxisInfoSetPtrs info;
  int exists;
  unsigned int ai, si, minsi;

  if (!(nrrd && AIR_IN_CL(1, nrrd->dim, NRRD_DIM_MAX)
        && AIR_IN_OP(nrrdAxisInfoUnknown, axInfo, nrrdAxisInfoLast) && _info)) {
    return;
  }
  info.P = _info;

  for (ai = 0; ai < nrrd->dim; ai++) {
    switch (axInfo) {
    case nrrdAxisInfoSize:
      nrrd->axis[ai].size = info.ST[ai];
      break;
    case nrrdAxisInfoSpacing:
      nrrd->axis[ai].spacing = info.D[ai];
      break;
    case nrrdAxisInfoThickness:
      nrrd->axis[ai].thickness = info.D[ai];
      break;
    case nrrdAxisInfoMin:
      nrrd->axis[ai].min = info.D[ai];
      break;
    case nrrdAxisInfoMax:
      nrrd->axis[ai].max = info.D[ai];
      break;
    case nrrdAxisInfoSpaceDirection:
      /* we won't allow setting an invalid direction */
      exists = AIR_EXISTS(info.V[ai][0]);
      minsi = nrrd->spaceDim;
      for (si = 0; si < nrrd->spaceDim; si++) {
        nrrd->axis[ai].spaceDirection[si] = info.V[ai][si];
        if (exists ^ AIR_EXISTS(info.V[ai][si])) {
          minsi = 0;
          break;
        }
      }
      for (si = minsi; si < NRRD_SPACE_DIM_MAX; si++) {
        nrrd->axis[ai].spaceDirection[si] = AIR_NAN;
      }
      break;
    case nrrdAxisInfoCenter:
      nrrd->axis[ai].center = info.I[ai];
      break;
    case nrrdAxisInfoKind:
      nrrd->axis[ai].kind = info.I[ai];
      break;
    case nrrdAxisInfoLabel:
      nrrd->axis[ai].label = (char *)airFree(nrrd->axis[ai].label);
      nrrd->axis[ai].label = (char *)airStrdup(info.CP[ai]);
      break;
    case nrrdAxisInfoUnits:
      nrrd->axis[ai].units = (char *)airFree(nrrd->axis[ai].units);
      nrrd->axis[ai].units = (char *)airStrdup(info.CP[ai]);
      break;
    }
  }
  if (nrrdAxisInfoSpaceDirection == axInfo) {
    for (ai = nrrd->dim; ai < NRRD_DIM_MAX; ai++) {
      for (si = 0; si < NRRD_SPACE_DIM_MAX; si++) {
        nrrd->axis[ai].spaceDirection[si] = AIR_NAN;
      }
    }
  }
  return;
}

/*
******** nrrdAxisInfoSet_va()
**
** var args front-end for nrrdAxisInfoSet_nva
**
** types to pass, one for each dimension:
**           nrrdAxisInfoSize: size_t
**        nrrdAxisInfoSpacing: double
**      nrrdAxisInfoThickness: double
**            nrrdAxisInfoMin: double
**            nrrdAxisInfoMax: double
** nrrdAxisInfoSpaceDirection: double*
**         nrrdAxisInfoCenter: int
**           nrrdAxisInfoKind: int
**          nrrdAxisInfoLabel: char*
**          nrrdAxisInfoUnits: char*
*/
void
nrrdAxisInfoSet_va(Nrrd *nrrd, int axInfo, ...) {
  NRRD_TYPE_BIGGEST *buffer[NRRD_DIM_MAX];
  _nrrdAxisInfoSetPtrs info;
  unsigned int ai, si;
  va_list ap;
  double *dp, svec[NRRD_DIM_MAX][NRRD_SPACE_DIM_MAX];

  if (!(nrrd && AIR_IN_CL(1, nrrd->dim, NRRD_DIM_MAX)
        && AIR_IN_OP(nrrdAxisInfoUnknown, axInfo, nrrdAxisInfoLast))) {
    return;
  }

  info.P = buffer;
  va_start(ap, axInfo);
  for (ai = 0; ai < nrrd->dim; ai++) {
    switch (axInfo) {
    case nrrdAxisInfoSize:
      info.ST[ai] = va_arg(ap, size_t);
      /*
      printf("!%s: got int[%d] = %d\n", "nrrdAxisInfoSet", d, info.I[ai]);
      */
      break;
    case nrrdAxisInfoSpaceDirection:
      dp = va_arg(ap, double *); /* punting on using info enum */
      /*
      printf("!%s: got dp = %lu\n", "nrrdAxisInfoSet",
             (unsigned long)(dp));
      */
      for (si = 0; si < nrrd->spaceDim; si++) {
        /* nrrd->axis[ai].spaceDirection[si] = dp[si]; */
        svec[ai][si] = dp[si];
      }
      for (si = nrrd->spaceDim; si < NRRD_SPACE_DIM_MAX; si++) {
        /* nrrd->axis[ai].spaceDirection[si] = AIR_NAN; */
        svec[ai][si] = dp[si];
      }
      break;
    case nrrdAxisInfoCenter:
    case nrrdAxisInfoKind:
      info.I[ai] = va_arg(ap, int);
      /*
      printf("!%s: got int[%d] = %d\n",
             "nrrdAxisInfoSet", d, info.I[ai]);
      */
      break;
    case nrrdAxisInfoSpacing:
    case nrrdAxisInfoThickness:
    case nrrdAxisInfoMin:
    case nrrdAxisInfoMax:
      info.D[ai] = va_arg(ap, double);
      /*
      printf("!%s: got double[%d] = %g\n",
             "nrrdAxisInfoSet", d, info.D[ai]);
      */
      break;
    case nrrdAxisInfoLabel:
      /* we DO NOT do the airStrdup() here because this pointer value is
         just going to be handed to nrrdAxisInfoSet_nva(), which WILL do the
         airStrdup(); we're not violating the rules for axis labels */
      info.CP[ai] = va_arg(ap, char *);
      /*
      printf("!%s: got char*[%d] = |%s|\n",
             "nrrdAxisInfoSet", d, info.CP[ai]);
      */
      break;
    case nrrdAxisInfoUnits:
      /* see not above */
      info.CP[ai] = va_arg(ap, char *);
      break;
    }
  }
  va_end(ap);

  if (nrrdAxisInfoSpaceDirection != axInfo) {
    /* now set the quantities which we've gotten from the var args */
    nrrdAxisInfoSet_nva(nrrd, axInfo, info.P);
  } else {
    nrrdAxisInfoSet_nva(nrrd, axInfo, svec);
  }

  return;
}

/*
******** nrrdAxisInfoGet_nva()
**
** get any of the axis fields into an array
**
** Note that getting axes labels involves implicitly allocating space
** for them, due to the action of airStrdup().  The user is
** responsible for free()ing these strings when done with them.
**
** type to pass for third argument:
**           nrrdAxisInfoSize: size_t*
**        nrrdAxisInfoSpacing: double*
**      nrrdAxisInfoThickness: double*
**            nrrdAxisInfoMin: double*
**            nrrdAxisInfoMax: double*
** nrrdAxisInfoSpaceDirection: double (*var)[NRRD_SPACE_DIM_MAX]
**         nrrdAxisInfoCenter: int*
**           nrrdAxisInfoKind: int*
**          nrrdAxisInfoLabel: char**
**          nrrdAxisInfoUnits: char**
*/
void
nrrdAxisInfoGet_nva(const Nrrd *nrrd, int axInfo, void *_info) {
  _nrrdAxisInfoGetPtrs info;
  unsigned int ai, si;

  if (!(nrrd && AIR_IN_CL(1, nrrd->dim, NRRD_DIM_MAX)
        && AIR_IN_OP(nrrdAxisInfoUnknown, axInfo, nrrdAxisInfoLast))) {
    return;
  }

  info.P = _info;
  for (ai = 0; ai < nrrd->dim; ai++) {
    switch (axInfo) {
    case nrrdAxisInfoSize:
      info.ST[ai] = nrrd->axis[ai].size;
      break;
    case nrrdAxisInfoSpacing:
      info.D[ai] = nrrd->axis[ai].spacing;
      break;
    case nrrdAxisInfoThickness:
      info.D[ai] = nrrd->axis[ai].thickness;
      break;
    case nrrdAxisInfoMin:
      info.D[ai] = nrrd->axis[ai].min;
      break;
    case nrrdAxisInfoMax:
      info.D[ai] = nrrd->axis[ai].max;
      break;
    case nrrdAxisInfoSpaceDirection:
      for (si = 0; si < nrrd->spaceDim; si++) {
        info.V[ai][si] = nrrd->axis[ai].spaceDirection[si];
      }
      for (si = nrrd->spaceDim; si < NRRD_SPACE_DIM_MAX; si++) {
        info.V[ai][si] = AIR_NAN;
      }
      break;
    case nrrdAxisInfoCenter:
      info.I[ai] = nrrd->axis[ai].center;
      break;
    case nrrdAxisInfoKind:
      info.I[ai] = nrrd->axis[ai].kind;
      break;
    case nrrdAxisInfoLabel:
      /* note airStrdup()! */
      info.CP[ai] = airStrdup(nrrd->axis[ai].label);
      break;
    case nrrdAxisInfoUnits:
      /* note airStrdup()! */
      info.CP[ai] = airStrdup(nrrd->axis[ai].units);
      break;
    }
  }
  if (nrrdAxisInfoSpaceDirection == axInfo) {
    for (ai = nrrd->dim; ai < NRRD_DIM_MAX; ai++) {
      for (si = 0; si < NRRD_SPACE_DIM_MAX; si++) {
        info.V[ai][si] = AIR_NAN;
      }
    }
  }
  return;
}

/*
** types to pass, one for each dimension:
**           nrrdAxisInfoSize: size_t*
**        nrrdAxisInfoSpacing: double*
**      nrrdAxisInfoThickness: double*
**            nrrdAxisInfoMin: double*
**            nrrdAxisInfoMax: double*
** nrrdAxisInfoSpaceDirection: double*
**         nrrdAxisInfoCenter: int*
**           nrrdAxisInfoKind: int*
**          nrrdAxisInfoLabel: char**
**          nrrdAxisInfoUnits: char**
*/
void
nrrdAxisInfoGet_va(const Nrrd *nrrd, int axInfo, ...) {
  void *buffer[NRRD_DIM_MAX], *ptr;
  _nrrdAxisInfoGetPtrs info;
  unsigned int ai, si;
  va_list ap;
  double svec[NRRD_DIM_MAX][NRRD_SPACE_DIM_MAX];

  if (!(nrrd && AIR_IN_CL(1, nrrd->dim, NRRD_DIM_MAX)
        && AIR_IN_OP(nrrdAxisInfoUnknown, axInfo, nrrdAxisInfoLast))) {
    return;
  }

  if (nrrdAxisInfoSpaceDirection != axInfo) {
    info.P = buffer;
    nrrdAxisInfoGet_nva(nrrd, axInfo, info.P);
  } else {
    nrrdAxisInfoGet_nva(nrrd, axInfo, svec);
  }

  va_start(ap, axInfo);
  for (ai = 0; ai < nrrd->dim; ai++) {
    ptr = va_arg(ap, void *);
    /*
    printf("!%s(%d): ptr = %lu\n",
           "nrrdAxisInfoGet", d, (unsigned long)ptr);
    */
    switch (axInfo) {
    case nrrdAxisInfoSize:
      *((size_t *)ptr) = info.ST[ai];
      break;
    case nrrdAxisInfoSpacing:
    case nrrdAxisInfoThickness:
    case nrrdAxisInfoMin:
    case nrrdAxisInfoMax:
      *((double *)ptr) = info.D[ai];
      /* printf("!%s: got double[%d] = %lg\n", "nrrdAxisInfoGet", d,
       *((double*)ptr)); */
      break;
    case nrrdAxisInfoSpaceDirection:
      for (si = 0; si < nrrd->spaceDim; si++) {
        ((double *)ptr)[si] = svec[ai][si];
      }
      for (si = nrrd->spaceDim; si < NRRD_SPACE_DIM_MAX; si++) {
        ((double *)ptr)[si] = AIR_NAN;
      }
      break;
    case nrrdAxisInfoCenter:
    case nrrdAxisInfoKind:
      *((int *)ptr) = info.I[ai];
      /* printf("!%s: got int[%d] = %d\n",
         "nrrdAxisInfoGet", d, *((int*)ptr)); */
      break;
    case nrrdAxisInfoLabel:
    case nrrdAxisInfoUnits:
      /* we DO NOT do the airStrdup() here because this pointer value just
         came from nrrdAxisInfoGet_nva(), which already did the airStrdup() */
      *((char **)ptr) = info.CP[ai];
      /* printf("!%s: got char*[%d] = |%s|\n", "nrrdAxisInfoSet", d,
       *((char**)ptr)); */
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
** as input.  Converts nrrdCenterUnknown into nrrdDefaultCenter,
** and then clamps to (nrrdCenterUnknown+1, nrrdCenterLast-1).
**
** Thus, this ALWAYS returns nrrdCenterNode or nrrdCenterCell
** (as long as those are the only two centering schemes).
*/
int /* Biff: (private) nope */
_nrrdCenter(int center) {

  center = (nrrdCenterUnknown == center ? nrrdDefaultCenter : center);
  center = AIR_CLAMP(nrrdCenterUnknown + 1, center, nrrdCenterLast - 1);
  return center;
}

int /* Biff: (private) nope */
_nrrdCenter2(int center, int defCenter) {

  center = (nrrdCenterUnknown == center ? defCenter : center);
  center = AIR_CLAMP(nrrdCenterUnknown + 1, center, nrrdCenterLast - 1);
  return center;
}

/*
******** nrrdAxisInfoPos()
**
** given a nrrd, an axis, and a (floating point) index space position,
** return the position implied the axis's min, max, and center
** Does the opposite of nrrdAxisIdx().
*/
double /* Biff: nope */
nrrdAxisInfoPos(const Nrrd *nrrd, unsigned int ax, double idx) {
  int center;
  size_t size;
  double min, max;

  if (!(nrrd && ax <= nrrd->dim - 1)) {
    return AIR_NAN;
  }
  center = _nrrdCenter(nrrd->axis[ax].center);
  min = nrrd->axis[ax].min;
  max = nrrd->axis[ax].max;
  size = nrrd->axis[ax].size;

  return NRRD_POS(center, min, max, size, idx);
}

/*
******** nrrdAxisInfoIdx()
**
** given a nrrd, an axis, and a (floating point) world space position,
** return the index implied the axis's min, max, and center.
** Does the opposite of nrrdAxisPos().
*/
double /* Biff: nope */
nrrdAxisInfoIdx(const Nrrd *nrrd, unsigned int ax, double pos) {
  int center;
  size_t size;
  double min, max;

  if (!(nrrd && ax <= nrrd->dim - 1)) {
    return AIR_NAN;
  }
  center = _nrrdCenter(nrrd->axis[ax].center);
  min = nrrd->axis[ax].min;
  max = nrrd->axis[ax].max;
  size = nrrd->axis[ax].size;

  return NRRD_IDX(center, min, max, size, pos);
}

/*
******** nrrdAxisInfoPosRange()
**
** given a nrrd, an axis, and two (floating point) index space positions,
** return the range of positions implied the axis's min, max, and center
** The opposite of nrrdAxisIdxRange()
*/
void
nrrdAxisInfoPosRange(double *loP, double *hiP, const Nrrd *nrrd, unsigned int ax,
                     double loIdx, double hiIdx) {
  int center, flip = 0;
  size_t size;
  double min, max, tmp;

  if (!(loP && hiP && nrrd && ax <= nrrd->dim - 1)) {
    if (loP) *loP = AIR_NAN;
    if (hiP) *hiP = AIR_NAN;
    return;
  }
  center = _nrrdCenter(nrrd->axis[ax].center);
  min = nrrd->axis[ax].min;
  max = nrrd->axis[ax].max;
  size = nrrd->axis[ax].size;

  if (loIdx > hiIdx) {
    flip = 1;
    tmp = loIdx;
    loIdx = hiIdx;
    hiIdx = tmp;
  }
  if (nrrdCenterCell == center) {
    *loP = AIR_AFFINE(0, loIdx, size, min, max);
    *hiP = AIR_AFFINE(0, hiIdx + 1, size, min, max);
  } else {
    *loP = AIR_AFFINE(0, loIdx, size - 1, min, max);
    *hiP = AIR_AFFINE(0, hiIdx, size - 1, min, max);
  }
  if (flip) {
    tmp = *loP;
    *loP = *hiP;
    *hiP = tmp;
  }

  return;
}

/*
******** nrrdAxisInfoIdxRange()
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
nrrdAxisInfoIdxRange(double *loP, double *hiP, const Nrrd *nrrd, unsigned int ax,
                     double loPos, double hiPos) {
  int center, flip = 0;
  size_t size;
  double min, max, tmp;

  if (!(loP && hiP && nrrd && ax <= nrrd->dim - 1)) {
    *loP = *hiP = AIR_NAN;
    return;
  }
  center = _nrrdCenter(nrrd->axis[ax].center);
  min = nrrd->axis[ax].min;
  max = nrrd->axis[ax].max;
  size = nrrd->axis[ax].size;

  if (loPos > hiPos) {
    flip = 1;
    tmp = loPos;
    loPos = hiPos;
    hiPos = tmp;
  }
  if (nrrdCenterCell == center) {
    if (min < max) {
      *loP = AIR_AFFINE(min, loPos, max, 0, size);
      *hiP = AIR_AFFINE(min, hiPos, max, -1, size - 1);
    } else {
      *loP = AIR_AFFINE(min, loPos, max, -1, size - 1);
      *hiP = AIR_AFFINE(min, hiPos, max, 0, size);
    }
  } else {
    *loP = AIR_AFFINE(min, loPos, max, 0, size - 1);
    *hiP = AIR_AFFINE(min, hiPos, max, 0, size - 1);
  }
  if (flip) {
    tmp = *loP;
    *loP = *hiP;
    *hiP = tmp;
  }

  return;
}

void
nrrdAxisInfoSpacingSet(Nrrd *nrrd, unsigned int ax) {
  int sign;
  double min, max, tmp;

  if (!(nrrd && ax <= nrrd->dim - 1)) {
    return;
  }

  min = nrrd->axis[ax].min;
  max = nrrd->axis[ax].max;
  if (!(AIR_EXISTS(min) && AIR_EXISTS(max))) {
    /* there's no actual basis on which to set the spacing information,
       but we have to set it something, so here goes .. */
    nrrd->axis[ax].spacing = nrrdDefaultSpacing;
    return;
  }

  if (min > max) {
    tmp = min;
    min = max;
    max = tmp;
    sign = -1;
  } else {
    sign = 1;
  }

  /* the skinny */
  nrrd->axis[ax].spacing = NRRD_SPACING(_nrrdCenter(nrrd->axis[ax].center), min, max,
                                        nrrd->axis[ax].size);
  nrrd->axis[ax].spacing *= sign;

  return;
}

void
nrrdAxisInfoMinMaxSet(Nrrd *nrrd, unsigned int ax, int defCenter) {
  int center;
  double spacing;

  if (!(nrrd && ax <= nrrd->dim - 1)) {
    return;
  }

  center = _nrrdCenter2(nrrd->axis[ax].center, defCenter);
  spacing = nrrd->axis[ax].spacing;
  if (!AIR_EXISTS(spacing)) spacing = nrrdDefaultSpacing;
  if (nrrdCenterCell == center) {
    nrrd->axis[ax].min = 0;
    nrrd->axis[ax].max = spacing * AIR_CAST(double, nrrd->axis[ax].size);
  } else {
    nrrd->axis[ax].min = 0;
    nrrd->axis[ax].max = spacing * AIR_CAST(double, nrrd->axis[ax].size - 1);
  }

  return;
}
/* ---- BEGIN non-NrrdIO */

/*
** not using the value comparators in accessors.c because of their
** slightly strange behavior WRT infinity (+inf < -42).  This code
** may eventually warrant wider availability, for now its here but
** accessible to nrrd files via privateNrrd.h
*/
int /* Biff: (private) nope */
_nrrdDblcmp(double aa, double bb) {
  int nna, nnb, ret;

  nna = AIR_EXISTS(aa) || !airIsNaN(aa);
  nnb = AIR_EXISTS(bb) || !airIsNaN(bb);
  if (nna && nnb) {
    /* both either exist or are an infinity */
    ret = (aa < bb ? -1 : (aa > bb ? 1 : 0));
  } else {
    /* one or the other is NaN */
    ret = (nna < nnb ? -1 : (nna > nnb ? 1 : 0));
  }
  return ret;
}

/*
******** nrrdAxisInfoCompare
**
** compares all fields in the NrrdAxisInfoCompare
**
** See comment about logic of return value above nrrdCompare()
**
** NOTE: the structure of this code is very similar to that of
** nrrdCompare, and any improvements here should be reflected there
*/
int /* Biff: 1 */
nrrdAxisInfoCompare(const NrrdAxisInfo *axisA, const NrrdAxisInfo *axisB, int *differ,
                    char explain[AIR_STRLEN_LARGE + 1]) {
  static const char me[] = "nrrdAxisInfoCompare";
  unsigned int saxi;

  if (!(axisA && axisB && differ)) {
    biffAddf(NRRD, "%s: got NULL pointer (%p, %p, or %p)", me, AIR_CVOIDP(axisA),
             AIR_CVOIDP(axisB), AIR_VOIDP(differ));
    return 1;
  }

  if (explain) {
    strcpy(explain, "");
  }
  if (axisA->size != axisB->size) {
    char stmp[2][AIR_STRLEN_SMALL + 1];
    *differ = axisA->size < axisB->size ? -1 : 1;
    if (explain) {
      sprintf(explain, "axisA->size=%s %s axisB->size=%s",
              airSprintSize_t(stmp[0], axisA->size), *differ < 0 ? "<" : ">",
              airSprintSize_t(stmp[1], axisB->size));
    }
    return 0;
  }

#define DOUBLE_COMPARE(VAL, STR)                                                        \
  *differ = _nrrdDblcmp(axisA->VAL, axisB->VAL);                                        \
  if (*differ) {                                                                        \
    if (explain) {                                                                      \
      sprintf(explain, "axisA->%s %.17g %s axisB->%s %.17g", STR, axisA->VAL,           \
              *differ < 0 ? "<" : ">", STR, axisB->VAL);                                \
    }                                                                                   \
    return 0;                                                                           \
  }

  DOUBLE_COMPARE(spacing, "spacing");
  DOUBLE_COMPARE(thickness, "thickness");
  DOUBLE_COMPARE(min, "min");
  DOUBLE_COMPARE(max, "max");
  for (saxi = 0; saxi < NRRD_SPACE_DIM_MAX; saxi++) {
    char stmp[AIR_STRLEN_SMALL + 1];
    sprintf(stmp, "spaceDirection[%u]", saxi);
    DOUBLE_COMPARE(spaceDirection[saxi], stmp);
  }
#undef DOUBLE_COMPARE

  if (axisA->center != axisB->center) {
    *differ = axisA->center < axisB->center ? -1 : 1;
    if (explain) {
      sprintf(explain, "axisA->center %s %s axisB->center %s",
              airEnumStr(nrrdCenter, axisA->center), *differ < 0 ? "<" : ">",
              airEnumStr(nrrdCenter, axisB->center));
    }
    return 0;
  }
  if (axisA->kind != axisB->kind) {
    *differ = axisA->kind < axisB->kind ? -1 : 1;
    if (explain) {
      sprintf(explain, "axisA->kind %s %s axisB->kind %s",
              airEnumStr(nrrdKind, axisA->kind), *differ < 0 ? "<" : ">",
              airEnumStr(nrrdKind, axisB->kind));
    }
    return 0;
  }
  *differ = airStrcmp(axisA->label, axisB->label);
  if (*differ) {
    if (explain) {
      /* can't safely print whole labels because of fixed-size of explain */
      sprintf(explain, "axisA->label %s axisB->label", *differ < 0 ? "<" : ">");
      if (strlen(explain) + airStrlen(axisA->label) + airStrlen(axisB->label)
            + 2 * strlen(" \"\" ") + 1
          < AIR_STRLEN_LARGE + 1) {
        /* ok, we can print them */
        sprintf(explain, "axisA->label \"%s\" %s axisB->label \"%s\"",
                axisA->label ? axisA->label : "", *differ < 0 ? "<" : ">",
                axisB->label ? axisB->label : "");
      }
    }
    return 0;
  }
  *differ = airStrcmp(axisA->units, axisB->units);
  if (*differ) {
    if (explain) {
      /* can't print whole string because of fixed-size of explain */
      sprintf(explain, "axisA->units %s axisB->units", *differ < 0 ? "<" : ">");
    }
    return 0;
  }

  return 0;
}
/* ---- END non-NrrdIO */

/*
******** nrrdDomainAxesGet
**
** Based on the per-axis "kind" field, learns which are the domain
** (resample-able) axes of an image, in other words, the axes which
** correspond to independent variables.  The return value is the
** number of domain axes, and that many values are set in the given
** axisIdx[] array
**
** NOTE: this takes a wild guess that an unset (nrrdKindUnknown) kind
** is a domain axis.
*/
unsigned int /* Biff: nope */
nrrdDomainAxesGet(const Nrrd *nrrd, unsigned int axisIdx[NRRD_DIM_MAX]) {
  unsigned int domAxi, axi;

  if (!(nrrd && axisIdx)) {
    return 0;
  }
  domAxi = 0;
  for (axi = 0; axi < nrrd->dim; axi++) {
    if (nrrdKindUnknown == nrrd->axis[axi].kind
        || nrrdKindIsDomain(nrrd->axis[axi].kind)) {
      axisIdx[domAxi++] = axi;
    }
  }
  return domAxi;
}

static int
_nrrdSpaceVecExists(const Nrrd *nrrd, unsigned int axi) {
  unsigned int sai;
  int ret;

  if (!(nrrd && axi < nrrd->dim && nrrd->spaceDim)) {
    ret = AIR_FALSE;
  } else {
    ret = AIR_TRUE;
    for (sai = 0; sai < nrrd->spaceDim; sai++) {
      ret &= AIR_EXISTS(nrrd->axis[axi].spaceDirection[sai]);
    }
  }
  return ret;
}

unsigned int /* Biff: nope */
nrrdSpatialAxesGet(const Nrrd *nrrd, unsigned int axisIdx[NRRD_DIM_MAX]) {
  unsigned int spcAxi, axi;

  if (!(nrrd && axisIdx && nrrd->spaceDim)) {
    return 0;
  }
  spcAxi = 0;
  for (axi = 0; axi < nrrd->dim; axi++) {
    if (_nrrdSpaceVecExists(nrrd, axi)) {
      axisIdx[spcAxi++] = axi;
    }
  }
  return spcAxi;
}

/*
******** nrrdRangeAxesGet
**
** Based on the per-axis "kind" field, learns which are the range
** (non-resample-able) axes of an image, in other words, the axes
** which correspond to dependent variables.  The return value is the
** number of range axes; that number of values are set in the given
** axisIdx[] array
**
** Note: this really is as simple as returning the complement of the
** axis selected by nrrdDomainAxesGet()
*/
unsigned int /* Biff: nope */
nrrdRangeAxesGet(const Nrrd *nrrd, unsigned int axisIdx[NRRD_DIM_MAX]) {
  unsigned int domNum, domIdx[NRRD_DIM_MAX], rngAxi, axi, ii, isDom;

  if (!(nrrd && axisIdx)) {
    return 0;
  }
  domNum = nrrdDomainAxesGet(nrrd, domIdx);
  rngAxi = 0;
  for (axi = 0; axi < nrrd->dim; axi++) {
    isDom = AIR_FALSE;
    for (ii = 0; ii < domNum; ii++) { /* yes, inefficient */
      isDom |= axi == domIdx[ii];
    }
    if (!isDom) {
      axisIdx[rngAxi++] = axi;
    }
  }
  return rngAxi;
}

unsigned int /* Biff: nope */
nrrdNonSpatialAxesGet(const Nrrd *nrrd, unsigned int axisIdx[NRRD_DIM_MAX]) {
  unsigned int spcNum, spcIdx[NRRD_DIM_MAX], nspAxi, axi, ii, isSpc;

  if (!(nrrd && axisIdx)) {
    return 0;
  }
  /* HEY: copy and paste, should refactor with above */
  spcNum = nrrdSpatialAxesGet(nrrd, spcIdx);
  nspAxi = 0;
  for (axi = 0; axi < nrrd->dim; axi++) {
    isSpc = AIR_FALSE;
    for (ii = 0; ii < spcNum; ii++) { /* yes, inefficient */
      isSpc |= axi == spcIdx[ii];
    }
    if (!isSpc) {
      axisIdx[nspAxi++] = axi;
    }
  }
  return nspAxi;
}

/*
******** nrrdSpacingCalculate
**
** Determine nrrdSpacingStatus, and whatever can be calculated about
** spacing for a given axis.  Takes a nrrd, an axis, a double pointer
** (for returning a scalar), a space vector, and an int pointer for
** returning the known length of the space vector.
**
** The behavior of what has been set by the function is determined by
** the return value, which takes values from the nrrdSpacingStatus*
** enum, as follows:
**
** returned status value:            what it means, and what it set
** ---------------------------------------------------------------------------
** nrrdSpacingStatusUnknown          Something about the given arguments is
**                                   invalid.
**                                   *spacing = NaN,
**                                   vector = all NaNs
**
** nrrdSpacingStatusNone             There is no spacing info at all:
**                                   *spacing = NaN,
**                                   vector = all NaNs
**
** nrrdSpacingStatusScalarNoSpace    There is no surrounding space, but the
**                                   axis's spacing was known.
**                                   *spacing = axis->spacing,
**                                   vector = all NaNs
**
** nrrdSpacingStatusScalarWithSpace  There *is* a surrounding space, but the
**                                   given axis does not live in that space,
**                                   because it has no space direction.  Caller
**                                   may want to think about what's going on.
**                                   *spacing = axis->spacing,
**                                   vector = all NaNs
**
** nrrdSpacingStatusDirection        There is a surrounding space, in which
**                                   this axis has a direction V:
**                                   *spacing = |V| (length of direction),
**                                   vector = V/|V| (normalized direction)
**                                   NOTE: it is still possible for both
**                                   *spacing and vector to be all NaNs!!
*/
int /* Biff: nope */
nrrdSpacingCalculate(const Nrrd *nrrd, unsigned int ax, double *spacing,
                     double vector[NRRD_SPACE_DIM_MAX]) {
  int ret;

  if (!(nrrd && spacing && vector && ax <= nrrd->dim - 1
        && !_nrrdCheck(nrrd, AIR_FALSE, AIR_FALSE))) {
    /* there's a problem with the arguments.  Note: the _nrrdCheck()
       call does not check on non-NULL-ity of nrrd->data */
    ret = nrrdSpacingStatusUnknown;
    if (spacing) {
      *spacing = AIR_NAN;
    }
    if (vector) {
      nrrdSpaceVecSetNaN(vector);
    }
  } else {
    if (AIR_EXISTS(nrrd->axis[ax].spacing)) {
      if (nrrd->spaceDim > 0) {
        ret = nrrdSpacingStatusScalarWithSpace;
      } else {
        ret = nrrdSpacingStatusScalarNoSpace;
      }
      *spacing = nrrd->axis[ax].spacing;
      nrrdSpaceVecSetNaN(vector);
    } else {
      if (nrrd->spaceDim > 0 && _nrrdSpaceVecExists(nrrd, ax)) {
        ret = nrrdSpacingStatusDirection;
        *spacing = nrrdSpaceVecNorm(nrrd->spaceDim, nrrd->axis[ax].spaceDirection);
        nrrdSpaceVecScale(vector, 1.0 / (*spacing), nrrd->axis[ax].spaceDirection);
      } else {
        ret = nrrdSpacingStatusNone;
        *spacing = AIR_NAN;
        nrrdSpaceVecSetNaN(vector);
      }
    }
  }
  return ret;
}

int /* Biff: 1 */
nrrdOrientationReduce(Nrrd *nout, const Nrrd *nin, int setMinsFromOrigin) {
  static const char me[] = "nrrdOrientationReduce";
  unsigned int spatialAxisNum, spatialAxisIdx[NRRD_DIM_MAX], saxii;
  NrrdAxisInfo *axis;

  if (!(nout && nin)) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }

  if (nout != nin) {
    if (nrrdCopy(nout, nin)) {
      biffAddf(NRRD, "%s: trouble doing initial copying", me);
      return 1;
    }
  }
  if (!nout->spaceDim) {
    /* we're done! */
    return 0;
  }
  spatialAxisNum = nrrdSpatialAxesGet(nout, spatialAxisIdx);
  for (saxii = 0; saxii < spatialAxisNum; saxii++) {
    axis = nout->axis + spatialAxisIdx[saxii];
    axis->spacing = nrrdSpaceVecNorm(nout->spaceDim, axis->spaceDirection);
    if (setMinsFromOrigin) {
      axis->min = (saxii < nout->spaceDim ? nout->spaceOrigin[saxii] : AIR_NAN);
    }
  }
  nrrdSpaceSet(nout, nrrdSpaceUnknown);

  return 0;
}

/* ---- BEGIN non-NrrdIO */

/*
******** nrrdMetaData
**
** The brains of "unu dnorm" (for Diderot normalization): put all meta-data
** of a nrrd into some simpler canonical form.
**
** This function probably doesn't belong in this file, but it is kind
** the opposite of nrrdOrientationReduce (above), so here it is
*/
int /* Biff: 1 */
nrrdMetaDataNormalize(Nrrd *nout, const Nrrd *nin, int version, int trivialOrient,
                      int permuteComponentAxisFastest, int recenterGrid,
                      double sampleSpacing, int *lostMeasurementFrame) {
  static const char me[] = "nrrdMetaDataNormalize";
  size_t size[NRRD_DIM_MAX];
  int kindIn, kindOut, haveMM, gotmf;
  unsigned int kindAxis, axi, si, sj;
  Nrrd *ntmp;
  airArray *mop;

  if (!(nout && nin)) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  if (airEnumValCheck(nrrdMetaDataCanonicalVersion, version)) {
    biffAddf(NRRD, "%s: version %d not valid %s", me, version,
             nrrdMetaDataCanonicalVersion->name);
    return 1;
  }
  if (nrrdMetaDataCanonicalVersionAlpha != version) {
    biffAddf(NRRD, "%s: sorry, %s %s not implemented (only %s)", me,
             nrrdMetaDataCanonicalVersion->name,
             airEnumStr(nrrdMetaDataCanonicalVersion, version),
             airEnumStr(nrrdMetaDataCanonicalVersion,
                        nrrdMetaDataCanonicalVersionAlpha));
    return 1;
  }

  if (_nrrdCheck(nin, AIR_FALSE /* checkData */, AIR_TRUE /* useBiff */)) {
    biffAddf(NRRD, "%s: basic check failed", me);
    return 1;
  }
  /* but can't deal with block type */
  if (nrrdTypeBlock == nin->type) {
    biffAddf(NRRD, "%s: can only have scalar types (not %s)", me,
             airEnumStr(nrrdType, nrrdTypeBlock));
    return 1;
  }

  /* look at all per-axis kinds */
  /* see if there's a range kind, verify that there's only one */
  /* set haveMM */
  haveMM = AIR_TRUE;
  kindIn = nrrdKindUnknown;
  kindAxis = 0; /* only means something if kindIn != nrrdKindUnknown */
  for (axi = 0; axi < nin->dim; axi++) {
    if (nrrdKindUnknown == nin->axis[axi].kind
        || nrrdKindIsDomain(nin->axis[axi].kind)) {
      haveMM &= AIR_EXISTS(nin->axis[axi].min);
      haveMM &= AIR_EXISTS(nin->axis[axi].max);
    } else {
      if (nrrdKindUnknown != kindIn) {
        biffAddf(NRRD,
                 "%s: got non-domain kind %s on axis %u, but already "
                 "have kind %s on previous axis %u",
                 me, airEnumStr(nrrdKind, nin->axis[axi].kind), axi,
                 airEnumStr(nrrdKind, kindIn), kindAxis);
        return 1;
      }
      kindIn = nin->axis[axi].kind;
      kindAxis = axi;
    }
  }

  if (nrrdKindUnknown != kindIn && kindAxis) {
    /* have a non-domain axis, and it isn't the fastest */
    if (permuteComponentAxisFastest) {
      if (nout == nin) {
        biffAddf(NRRD,
                 "%s: can't permute non-domain axis %u (kind %s) "
                 "to axis 0 with nout == nin",
                 me, kindAxis, airEnumStr(nrrdKind, kindIn));
        return 1;
      }
      biffAddf(NRRD,
               "%s: sorry, permuting non-domain axis %u (kind %s) "
               "to axis 0 not yet implemented",
               me, kindAxis, airEnumStr(nrrdKind, kindIn));
      return 1;
    } else {
      /* caller thinks its okay for non-domain axis to be on
         something other than fastest axis */
      if (nrrdMetaDataCanonicalVersionAlpha == version) {
        biffAddf(NRRD,
                 "%s: (%s) non-domain axis %u (kind %s) "
                 "must be fastest axis",
                 me, airEnumStr(nrrdMetaDataCanonicalVersion, version), kindAxis,
                 airEnumStr(nrrdKind, kindIn));
        return 1;
      }
      /* maybe with nrrdMetaDataCanonicalVersionAlpha != version
         it is okay to have non-domain axis on non-fastest axis? */
    }
  }

  /* HEY: would be nice to handle a stub "scalar" axis by deleting it */

  /* see if the non-domain kind is something we can interpret as a tensor */
  if (nrrdKindUnknown != kindIn) {
    switch (kindIn) {
      /* ======= THESE are the kinds that we can possibly output ======= */
    case nrrdKind2Vector:
    case nrrdKind3Vector:
    case nrrdKind4Vector:
    case nrrdKind2DSymMatrix:
    case nrrdKind2DMatrix:
    case nrrdKind3DSymMatrix:
    case nrrdKind3DMatrix:
      /* =============================================================== */
      kindOut = kindIn;
      break;
      /* Some other kinds are mapped to those above */
    case nrrdKind3Color:
    case nrrdKindRGBColor:
      kindOut = nrrdKind3Vector;
      break;
    case nrrdKind4Color:
    case nrrdKindRGBAColor:
      kindOut = nrrdKind4Vector;
      break;
    default:
      biffAddf(NRRD, "%s: got non-conforming kind %s on axis %u", me,
               airEnumStr(nrrdKind, kindIn), kindAxis);
      return 1;
    }
  } else {
    /* kindIn is nrrdKindUnknown, so its a simple scalar image,
       and that's what the output will be too; kindOut == nrrdKindUnknown
       is used in the code below to say "its a scalar image" */
    kindOut = nrrdKindUnknown;
  }

  /* initialize output by copying meta-data from nin to ntmp */
  mop = airMopNew();
  ntmp = nrrdNew();
  airMopAdd(mop, ntmp, (airMopper)nrrdNix, airMopAlways);
  /* HEY this is doing the work of a shallow copy, which isn't
     available in the API.  You can pass nrrdCopy() a nin with NULL
     nin->data, which implements a shallow copy, but we can't set
     nin->data=NULL here because of const correctness */
  nrrdAxisInfoGet_nva(nin, nrrdAxisInfoSize, size);
  if (nrrdWrap_nva(ntmp, NULL, nin->type, nin->dim, size)) {
    biffAddf(NRRD, "%s: couldn't wrap buffer nrrd around NULL", me);
    airMopError(mop);
    return 1;
  }
  /* so ntmp->data == NULL */
  nrrdAxisInfoCopy(ntmp, nin, NULL, NRRD_AXIS_INFO_SIZE_BIT);
  if (nrrdBasicInfoCopy(ntmp, nin, NRRD_BASIC_INFO_DATA_BIT)) {
    biffAddf(NRRD, "%s: trouble copying basic info", me);
    airMopError(mop);
    return 1;
  }

  /* no comments */
  nrrdCommentClear(ntmp);

  /* no measurement frame */
  gotmf = AIR_FALSE;
  for (si = 0; si < NRRD_SPACE_DIM_MAX; si++) {
    for (sj = 0; sj < NRRD_SPACE_DIM_MAX; sj++) {
      gotmf |= AIR_EXISTS(ntmp->measurementFrame[si][sj]);
    }
  }
  if (lostMeasurementFrame) {
    *lostMeasurementFrame = gotmf;
  }
  for (si = 0; si < NRRD_SPACE_DIM_MAX; si++) {
    for (sj = 0; sj < NRRD_SPACE_DIM_MAX; sj++) {
      ntmp->measurementFrame[si][sj] = AIR_NAN;
    }
  }

  /* no key/value pairs */
  nrrdKeyValueClear(ntmp);

  /* no content field */
  ntmp->content = (char *)airFree(ntmp->content);

  /* normalize domain kinds to "space" */
  /* HEY: if Diderot supports time-varying fields, this will have to change */
  /* turn off centers (current Diderot semantics don't expose centering) */
  /* turn off thickness */
  /* turn off labels and units */
  for (axi = 0; axi < ntmp->dim; axi++) {
    if (nrrdKindUnknown == kindOut) {
      ntmp->axis[axi].kind = nrrdKindSpace;
    } else {
      ntmp->axis[axi].kind = (kindAxis == axi ? kindOut : nrrdKindSpace);
    }
    ntmp->axis[axi].center = nrrdCenterUnknown;
    ntmp->axis[axi].thickness = AIR_NAN;
    ntmp->axis[axi].label = (char *)airFree(ntmp->axis[axi].label);
    ntmp->axis[axi].units = (char *)airFree(ntmp->axis[axi].units);
    ntmp->axis[axi].min = AIR_NAN;
    ntmp->axis[axi].max = AIR_NAN;
    ntmp->axis[axi].spacing = AIR_NAN;
  }

  /* logic of orientation definition:
     If space dimension is known:
        set origin to zero if not already set
        set space direction to unit vector if not already set
     Else if have per-axis min and max:
        set spae origin and directions to communicate same intent
        as original per-axis min and max and original centering
     Else
        set origin to zero and all space directions to units.
     (It might be nice to use gage's logic for mapping from world to index,
     but we have to accept a greater variety of kinds and dimensions
     than gage ever has to process.)
     The result is that space origin and space directions are set.
     the "space" field is not used, only "spaceDim"
  */
  /* no named space */
  ntmp->space = nrrdSpaceUnknown;
  if (ntmp->spaceDim && !trivialOrient) {
    int saxi = 0;
    if (!nrrdSpaceVecExists(ntmp->spaceDim, ntmp->spaceOrigin)) {
      nrrdSpaceVecSetZero(ntmp->spaceOrigin);
    }
    for (axi = 0; axi < ntmp->dim; axi++) {
      if (nrrdKindUnknown == kindOut || kindAxis != axi) {
        /* its a domain axis of output */
        if (!nrrdSpaceVecExists(ntmp->spaceDim, ntmp->axis[axi].spaceDirection)) {
          nrrdSpaceVecSetZero(ntmp->axis[axi].spaceDirection);
          ntmp->axis[axi].spaceDirection[saxi] = sampleSpacing;
        }
        /* else we leave existing space vector as is */
        saxi++;
      } else {
        /* else its a range (non-domain, component) axis */
        nrrdSpaceVecSetNaN(ntmp->axis[axi].spaceDirection);
      }
    }
  } else if (haveMM && !trivialOrient) {
    int saxi = 0;
    size_t N;
    double rng;
    for (axi = 0; axi < ntmp->dim; axi++) {
      if (nrrdKindUnknown == kindOut || kindAxis != axi) {
        /* its a domain axis of output */
        nrrdSpaceVecSetZero(ntmp->axis[axi].spaceDirection);
        rng = nin->axis[axi].max - nin->axis[axi].min;
        if (nrrdCenterNode == nin->axis[axi].center) {
          ntmp->spaceOrigin[saxi] = nin->axis[axi].min;
          N = nin->axis[axi].size;
          ntmp->axis[axi].spaceDirection[saxi] = rng / (N - 1);
        } else {
          /* unknown centering treated as cell */
          N = nin->axis[axi].size;
          ntmp->spaceOrigin[saxi] = nin->axis[axi].min + (rng / N) / 2;
          ntmp->axis[axi].spaceDirection[saxi] = rng / N;
        }
        saxi++;
      } else {
        /* else its a range axis */
        nrrdSpaceVecSetNaN(ntmp->axis[axi].spaceDirection);
      }
    }
    ntmp->spaceDim = saxi;
  } else {
    /* either trivialOrient, or, not spaceDim and not haveMM */
    int saxi = 0;
    nrrdSpaceVecSetZero(ntmp->spaceOrigin);
    for (axi = 0; axi < ntmp->dim; axi++) {
      if (nrrdKindUnknown == kindOut || kindAxis != axi) {
        /* its a domain axis of output */
        nrrdSpaceVecSetZero(ntmp->axis[axi].spaceDirection);
        ntmp->axis[axi].spaceDirection[saxi] = (AIR_EXISTS(nin->axis[axi].spacing)
                                                  ? nin->axis[axi].spacing
                                                  : sampleSpacing);
        saxi++;
      } else {
        /* else its a range axis */
        nrrdSpaceVecSetNaN(ntmp->axis[axi].spaceDirection);
      }
    }
    ntmp->spaceDim = saxi;
  }

  /* space dimension has to match the number of domain axes */
  if (ntmp->dim != ntmp->spaceDim + !!kindOut) {
    biffAddf(NRRD, "%s: output dim %d != spaceDim %d + %d %s%s%s%s", me, ntmp->dim,
             ntmp->spaceDim, !!kindOut, kindOut ? "for non-scalar (" : "(scalar data)",
             kindOut ? airEnumStr(nrrdKind, kindOut) : "", kindOut ? ") data" : "",
             kindOut ? ""
                     : "; a non-domain axis in the input "
                       "may be missing an informative \"kind\", leading to the "
                       "false assumption of a scalar array");
    airMopError(mop);
    return 1;
  }

  if (recenterGrid) {
    /* sets field's origin so field is centered on the origin. capiche? */
    /* this code was tacked on later than the stuff above, so its
       logic could probably be moved up there, but it seems cleaner to
       have it as a separate post-process */
    double mean[NRRD_SPACE_DIM_MAX];
    nrrdSpaceVecSetZero(mean);
    for (axi = 0; axi < ntmp->dim; axi++) {
      if (nrrdKindUnknown == kindOut || kindAxis != axi) {
        nrrdSpaceVecScaleAdd2(mean, 1.0, mean, 0.5 * (ntmp->axis[axi].size - 1),
                              ntmp->axis[axi].spaceDirection);
      }
    }
    nrrdSpaceVecScaleAdd2(mean, 1.0, mean, 1.0, ntmp->spaceOrigin);
    /* now mean is the center of the field */
    nrrdSpaceVecScaleAdd2(ntmp->spaceOrigin, 1.0, ntmp->spaceOrigin, -1.0, mean);
  }

  /* with that all done, now copy from ntmp to nout */
  if (nout != nin) {
    /* have to copy data */
    ntmp->data = nin->data;
    if (nrrdCopy(nout, ntmp)) {
      biffAddf(NRRD, "%s: problem copying (with data) to output", me);
      airMopError(mop);
      return 1;
    }
  } else {
    /* nout == nin; have to copy only meta-data, leave data as is */
    void *data = nin->data;
    /* ntmp->data == NULL, so this is a shallow copy */
    if (nrrdCopy(nout, ntmp)) {
      biffAddf(NRRD, "%s: problem copying meta-data to output", me);
      airMopError(mop);
      return 1;
    }
    nout->data = data;
  }

  airMopOkay(mop);
  return 0;
}

/* ---- END non-NrrdIO */
