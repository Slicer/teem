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


#include "ten.h"

int tenVerbose = 0;

/*
******** tenValidTensor()
**
** describes if the given nrrd could be a diffusion tensor dataset,
** either the measured DWI data or the calculated tensor data.
**
** We've been using 7 floats for BOTH kinds of tensor data- both the
** measured DWI and the calculated tensor matrices.  The measured data
** comes as one anatomical image and 6 DWIs.  For the calculated tensors,
** in addition to the 6 matrix components, we keep a "threshold" value
** which is based on the sum of all the DWIs, which describes if the
** calculated tensor means anything or not.
** 
** useBiff controls if biff is used to describe the problem
*/
int
tenValidTensor(Nrrd *nin, int wantType, int useBiff) {
  char me[]="tenValidTensor", err[256];
  
  if (!nin) {
    sprintf(err, "%s: got NULL pointer", me);
    if (useBiff) biffAdd(TEN, err); return 0;
  }
  if (wantType) {
    if (nin->type != wantType) {
      sprintf(err, "%s: wanted type %d, got type %d", me, wantType, nin->type);
      if (useBiff) biffAdd(TEN, err); return 0;
    }
  }
  else {
    if (!(nin->type == nrrdTypeFloat || nin->type == nrrdTypeShort)) {
      sprintf(err, "%s: need data of type float or short", me);
      if (useBiff) biffAdd(TEN, err); return 0;
    }
  }
  if (!(4 == nin->dim)) {
    sprintf(err, "%s: given dimension is %d, not 4", me, nin->dim);
    if (useBiff) biffAdd(TEN, err); return 0;
  }
  if (!(7 == nin->size[0])) {
    sprintf(err, "%s: axis 0 has size %d, not 7", me, nin->size[0]);
    if (useBiff) biffAdd(TEN, err); return 0;
  }
  return 1;
}

/*
******** tenEigensolve
**
** uses ell3mEigensolve to get the eigensystem of a single tensor
**
** return is same as ell3mEigensolve, which is same as ellCubic
**
** This does NOT use biff
*/
int
tenEigensolve(float _eval[3], float _evec[9], float t[7]) {
  double m[9], eval[3], evec[9];
  int ret;
  
  TEN_LIST2MAT(m, t);
  ret = ell3mEigensolve(eval, evec, m, AIR_FALSE);
  ELL_3V_COPY(_eval, eval);
  ELL_3V_COPY(_evec+0, evec+0);
  ELL_3V_COPY(_evec+3, evec+3);
  ELL_3V_COPY(_evec+6, evec+6);
  return ret;
}
