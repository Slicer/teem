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
  if (!(7 == nin->axis[0].size)) {
    sprintf(err, "%s: axis 0 has size %d, not 7", me, nin->axis[0].size);
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
  ELL_3M_COPY(_evec, evec);
  return ret;
}

