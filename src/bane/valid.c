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


#include "bane.h"

int
baneValidHVol(Nrrd *hvol) {
  char me[]="baneValidHVol", err[AIR_STRLEN_MED];

  if (3 != hvol->dim) {
    sprintf(err, "%s: need dimension to be 3 (not %d)", me, hvol->dim);
    biffAdd(BANE, err); return 0;
  }
  if (nrrdTypeUChar != hvol->type) {
    sprintf(err, "%s: need type to be unsigned char (not %s)", 
	    me, airEnumStr(nrrdType, hvol->type));
    biffAdd(BANE, err); return 0;
  }
  if (!( AIR_EXISTS(hvol->axis[0].min) && AIR_EXISTS(hvol->axis[0].max) && 
	 AIR_EXISTS(hvol->axis[1].min) && AIR_EXISTS(hvol->axis[1].max) && 
	 AIR_EXISTS(hvol->axis[2].min) && AIR_EXISTS(hvol->axis[2].max) )) {
    sprintf(err, "%s: axisMin and axisMax must be set for all axes", me);
    biffAdd(BANE, err); return 0;
  }
  if (strcmp(hvol->axis[0].label, "gradient-mag_cd")) {
    sprintf(err, "%s: expected \"gradient-mag_cd\" on axis 0", me);
    biffAdd(BANE, err); return 0;
  }
  if (strcmp(hvol->axis[1].label, "Laplacian_cd") &&
      strcmp(hvol->axis[1].label, "Hessian-2dd_cd") &&
      strcmp(hvol->axis[1].label, "grad-mag-grad_cd")) {
    sprintf(err, "%s: expected a second derivative measure on axis 1", me);
    biffAdd(BANE, err); return 0;    
  }
  if (strcmp(hvol->axis[2].label, "value")) {
    sprintf(err, "%s: expected \"value\" on axis 2", me);
    biffAdd(BANE, err); return 0;
  }
  return 1;
}

int
baneValidInfo(Nrrd *info, int wantDim) {
  char me[]="baneValidInfo", err[AIR_STRLEN_MED];
  int gotDim;

  if (!info) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(BANE, err); return 0;
  }
  gotDim = info->dim;
  if (wantDim) {
    if (!(1 == wantDim || 2 == wantDim)) {
      sprintf(err, "%s: wantDim should be 1 or 2, not %d", me, wantDim);
      biffAdd(BANE, err); return 0;
    }
    if (wantDim+1 != gotDim) {
      sprintf(err, "%s: dim is %d, not %d", me, gotDim, wantDim+1);
      biffAdd(BANE, err); return 0;
    }
  }
  else {
    if (!(2 == gotDim || 3 == gotDim)) {
      sprintf(err, "%s: dim is %d, not 2 or 3", me, gotDim);
      biffAdd(BANE, err); return 0;
    }
  }
  if (nrrdTypeFloat != info->type) {
    sprintf(err, "%s: need data of type float", me);
    biffAdd(BANE, err); return 0;
  }
  if (2 != info->axis[0].size) {
    sprintf(err, "%s: 1st axis needs size 2 (not %d)", me, info->axis[0].size);
    biffAdd(BANE, err); return 0;
  }
  return gotDim-1;
}

int
baneValidPos(Nrrd *pos, int wantDim) {
  char me[]="baneValidPos", err[AIR_STRLEN_MED];
  int gotDim;

  if (!pos) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(BANE, err); return 0;
  }
  gotDim = pos->dim;
  if (wantDim) {
    if (!(1 == wantDim || 2 == wantDim)) {
      sprintf(err, "%s: wantDim should be 1 or 2, not %d", me, wantDim);
      biffAdd(BANE, err); return 0;
    }
    if (wantDim != gotDim) {
      sprintf(err, "%s: dim is %d, not %d", me, gotDim, wantDim);
      biffAdd(BANE, err); return 0;
    }
  }
  else {
    if (!(1 == gotDim || 2 == gotDim)) {
      sprintf(err, "%s: dim is %d, not 1 or 2", me, gotDim);
      biffAdd(BANE, err); return 0;
    }
  }
  if (nrrdTypeFloat != pos->type) {
    sprintf(err, "%s: need data of type float", me);
    biffAdd(BANE, err); return 0;
  }
  /* HEY? check for values in axisMin[0] and axisMax[0] ? */
  /* HEY? check for values in axisMin[0] and axisMax[0] ? */
  /* HEY? check for values in axisMin[1] and axisMax[1] ? */
  return 1;
}

int
baneValidBcpts(Nrrd *Bcpts) {
  char me[]="baneValidBcpts", err[AIR_STRLEN_MED];
  int i, len;
  float *data;

  if (2 != Bcpts->dim) {
    sprintf(err, "%s: need 2-dimensional (not %d)", me, Bcpts->dim);
    biffAdd(BANE, err); return 0;
  }
  if (2 != Bcpts->axis[0].size) {
    sprintf(err, "%s: axis#0 needs size 2 (not %d)", me, Bcpts->axis[0].size);
    biffAdd(BANE, err); return 0;
  }
  if (nrrdTypeFloat != Bcpts->type) {
    sprintf(err, "%s: need data of type float", me);
    biffAdd(BANE, err); return 0;
  }
  len = Bcpts->axis[1].size;
  data = Bcpts->data;
  for (i=0; i<=len-2; i++) {
    if (!(data[0 + 2*i] <= data[0 + 2*(i+1)])) {
      sprintf(err, "%s: value coord %d (%g) not <= coord %d (%g)", me,
	      i, data[0 + 2*i], i+1, data[0 + 2*(i+1)]);
      biffAdd(BANE, err); return 0;
    }
  }
  return 1;
}

