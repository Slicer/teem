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


#include "bane.h"

int
baneValidHVol(Nrrd *hvol) {
  char me[]="baneValidHVol", err[128];

  if (3 != hvol->dim) {
    sprintf(err, "%s: need dimension to be 3 (not %d)", me, hvol->dim);
    biffSet(BANE, err); return 0;
  }
  if (nrrdTypeUChar != hvol->type) {
    sprintf(err, "%s: need type to be unsigned char (not %s)", 
	    me, nrrdEnumValToStr(nrrdEnumType, hvol->type));
    biffSet(BANE, err); return 0;
  }
  if (!( AIR_EXISTS(hvol->axis[0].min) && AIR_EXISTS(hvol->axis[0].max) && 
	 AIR_EXISTS(hvol->axis[1].min) && AIR_EXISTS(hvol->axis[1].max) && 
	 AIR_EXISTS(hvol->axis[2].min) && AIR_EXISTS(hvol->axis[2].max) )) {
    sprintf(err, "%s: axisMin and axisMax must be set for all axes", me);
    biffSet(BANE, err); return 0;
  }
  if (strcmp(hvol->axis[0].label, "gradient-mag_cd")) {
    sprintf(err, "%s: expected \"gradient-mag_cd\" on axis 0", me);
    biffSet(BANE, err); return 0;
  }
  if (strcmp(hvol->axis[1].label, "Laplacian_cd") &&
      strcmp(hvol->axis[1].label, "Hessian-2dd_cd") &&
      strcmp(hvol->axis[1].label, "grad-mag-grad_cd")) {
    sprintf(err, "%s: expected a second derivative measure on axis 1", me);
    biffSet(BANE, err); return 0;    
  }
  if (strcmp(hvol->axis[2].label, "value")) {
    sprintf(err, "%s: expected \"value\" on axis 2", me);
    biffSet(BANE, err); return 0;
  }
  return 1;
}

int
baneValidInfo(Nrrd *info, int wantDim) {
  char me[]="baneValidInfo", err[128];
  int gotDim;

  if (!info) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(BANE, err); return 0;
  }
  gotDim = info->dim;
  if (wantDim) {
    if (!(1 == wantDim || 2 == wantDim)) {
      sprintf(err, "%s: wantDim should be 1 or 2, not %d", me, wantDim);
      biffSet(BANE, err); return 0;
    }
    if (wantDim+1 != gotDim) {
      sprintf(err, "%s: dim is %d, not %d", me, gotDim, wantDim+1);
      biffSet(BANE, err); return 0;
    }
  }
  else {
    if (!(2 == gotDim || 3 == gotDim)) {
      sprintf(err, "%s: dim is %d, not 2 or 3", me, gotDim);
      biffSet(BANE, err); return 0;
    }
  }
  if (nrrdTypeFloat != info->type) {
    sprintf(err, "%s: need data of type float", me);
    biffSet(BANE, err); return 0;
  }
  if (2 != info->axis[0].size) {
    sprintf(err, "%s: 1st axis needs size 2 (not %d)", me, info->axis[0].size);
    biffSet(BANE, err); return 0;
  }
  return gotDim-1;
}

int
baneValidPos(Nrrd *pos, int wantDim) {
  char me[]="baneValidPos", err[128];
  int gotDim;

  if (!pos) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(BANE, err); return 0;
  }
  gotDim = pos->dim;
  if (wantDim) {
    if (!(1 == wantDim || 2 == wantDim)) {
      sprintf(err, "%s: wantDim should be 1 or 2, not %d", me, wantDim);
      biffSet(BANE, err); return 0;
    }
    if (wantDim != gotDim) {
      sprintf(err, "%s: dim is %d, not %d", me, gotDim, wantDim);
      biffSet(BANE, err); return 0;
    }
  }
  else {
    if (!(1 == gotDim || 2 == gotDim)) {
      sprintf(err, "%s: dim is %d, not 1 or 2", me, gotDim);
      biffSet(BANE, err); return 0;
    }
  }
  if (nrrdTypeFloat != pos->type) {
    sprintf(err, "%s: need data of type float", me);
    biffSet(BANE, err); return 0;
  }
  /* HEY? check for values in axisMin[0] and axisMax[0] ? */
  /* HEY? check for values in axisMin[0] and axisMax[0] ? */
  /* HEY? check for values in axisMin[1] and axisMax[1] ? */
  return 1;
}

int
baneValidBcpts(Nrrd *Bcpts) {
  char me[]="baneValidBcpts", err[128];
  int i, len;
  float *data;

  if (2 != Bcpts->dim) {
    sprintf(err, "%s: need 2-dimensional (not %d)", me, Bcpts->dim);
    biffSet(BANE, err); return 0;
  }
  if (2 != Bcpts->axis[0].size) {
    sprintf(err, "%s: axis#0 needs size 2 (not %d)", me, Bcpts->axis[0].size);
    biffSet(BANE, err); return 0;
  }
  if (nrrdTypeFloat != Bcpts->type) {
    sprintf(err, "%s: need data of type float", me);
    biffSet(BANE, err); return 0;
  }
  len = Bcpts->axis[1].size;
  data = Bcpts->data;
  for (i=0; i<=len-2; i++) {
    if (!(data[0 + 2*i] <= data[0 + 2*(i+1)])) {
      sprintf(err, "%s: value coord %d (%g) not <= coord %d (%g)", me,
	      i, data[0 + 2*i], i+1, data[0 + 2*(i+1)]);
      biffSet(BANE, err); return 0;
    }
  }
  return 1;
}

