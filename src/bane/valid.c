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
	    me, nrrdType2Str[hvol->type]);
    biffSet(BANE, err); return 0;
  }
  if (!( AIR_EXISTS(hvol->axisMin[0]) && AIR_EXISTS(hvol->axisMax[0]) && 
	 AIR_EXISTS(hvol->axisMin[1]) && AIR_EXISTS(hvol->axisMax[1]) && 
	 AIR_EXISTS(hvol->axisMin[2]) && AIR_EXISTS(hvol->axisMax[2]) )) {
    sprintf(err, "%s: axisMin and axisMax must be set for all axes", me);
    biffSet(BANE, err); return 0;
  }
  if (strcmp(hvol->label[0], "gradient-mag_cd")) {
    sprintf(err, "%s: expected \"gradient-mag_cd\" on axis 0", me);
    biffSet(BANE, err); return 0;
  }
  if (strcmp(hvol->label[1], "Laplacian_cd") &&
      strcmp(hvol->label[1], "Hessian-2dd_cd") &&
      strcmp(hvol->label[1], "grad-mag-grad_cd")) {
    sprintf(err, "%s: expected a second derivative measure on axis 1", me);
    biffSet(BANE, err); return 0;    
  }
  if (strcmp(hvol->label[2], "value")) {
    sprintf(err, "%s: expected \"value\" on axis 2", me);
    biffSet(BANE, err); return 0;
  }
  return 1;
}

int
baneValidInfo2D(Nrrd *info2D) {
  char me[]="baneValidInfo2D", err[128];

  if (3 != info2D->dim) {
    sprintf(err, "%s: need 3 dimensions, not %d", me, info2D->dim);
    biffSet(BANE, err); return 0;
  }
  if (nrrdTypeFloat != info2D->type) {
    sprintf(err, "%s: need data of type float", me);
    biffSet(BANE, err); return 0;
  }
  if (2 != info2D->size[0]) {
    sprintf(err, "%s: 1st axis needs size 2 (not %d)", me, info2D->size[0]);
    biffSet(BANE, err); return 0;
  }
  return 1;
}

int
baneValidInfo1D(Nrrd *info1D) {
  char me[]="baneValidInfo1D", err[128];
  
  if (2 != info1D->dim) {
    sprintf(err, "%s: need 2 dimensions, not %d", me, info1D->dim);
    biffSet(BANE, err); return 0;
  }
  if (nrrdTypeFloat != info1D->type) {
    sprintf(err, "%s: need data of type float", me);
    biffSet(BANE, err); return 0;
  }
  if (2 != info1D->size[0]) {
    sprintf(err, "%s: 1st axis needs size 2 (not %d)", me, info1D->size[0]);
    biffSet(BANE, err); return 0;
  }
  return 1;
}

int
baneValidPos1D(Nrrd *pos1D) {
  char me[]="baneValidPos1D", err[128];

  if (1 != pos1D->dim) {
    sprintf(err, "%s: need 1-dimensional (not %d)", me, pos1D->dim);
    biffSet(BANE, err); return 0;
  }
  if (nrrdTypeFloat != pos1D->type) {
    sprintf(err, "%s: need data of type float", me);
    biffSet(BANE, err); return 0;
  }
  /* HEY? check for values in axisMin[0] and axisMax[0] ? */
  return 1;
}

int
baneValidPos2D(Nrrd *pos2D) {
  char me[]="baneValidPos2D", err[128];

  if (2 != pos2D->dim) {
    sprintf(err, "%s: need 2-dimensional (not %d)", me, pos2D->dim);
    biffSet(BANE, err); return 0;
  }
  if (nrrdTypeFloat != pos2D->type) {
    sprintf(err, "%s: need data of type float", me);
    biffSet(BANE, err); return 0;
  }
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
  if (2 != Bcpts->size[0]) {
    sprintf(err, "%s: axis#0 needs size 2 (not %d)", me, Bcpts->size[0]);
    biffSet(BANE, err); return 0;
  }
  if (nrrdTypeFloat != Bcpts->type) {
    sprintf(err, "%s: need data of type float", me);
    biffSet(BANE, err); return 0;
  }
  len = Bcpts->size[1];
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

