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
#include "tenPrivate.h"

int tenVerbose = 0;

/*
******** tenTensorCheck()
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
tenTensorCheck(Nrrd *nin, int wantType, int useBiff) {
  char me[]="tenTensorCheck", err[256];
  
  if (!nin) {
    sprintf(err, "%s: got NULL pointer", me);
    if (useBiff) biffAdd(TEN, err); return 1;
  }
  if (wantType) {
    if (nin->type != wantType) {
      sprintf(err, "%s: wanted type %s, got type %s", me,
	      airEnumStr(nrrdType, wantType),
	      airEnumStr(nrrdType, nin->type));
      if (useBiff) biffAdd(TEN, err); return 1;
    }
  }
  else {
    if (!(nin->type == nrrdTypeFloat || nin->type == nrrdTypeShort)) {
      sprintf(err, "%s: need data of type float or short", me);
      if (useBiff) biffAdd(TEN, err); return 1;
    }
  }
  if (!(4 == nin->dim)) {
    sprintf(err, "%s: given dimension is %d, not 4", me, nin->dim);
    if (useBiff) biffAdd(TEN, err); return 1;
  }
  if (!(7 == nin->axis[0].size)) {
    sprintf(err, "%s: axis 0 has size %d, not 7", me, nin->axis[0].size);
    if (useBiff) biffAdd(TEN, err); return 1;
  }
  return 0;
}

int
tenExpand(Nrrd *nout, Nrrd *nin, float thresh) {
  char me[]="tenExpand", err[AIR_STRLEN_MED];
  size_t N, I;
  int sx, sy, sz;
  float *seven, *nine;

  if (!( nout && nin && AIR_EXISTS(thresh) )) {
    sprintf(err, "%s: got NULL pointer or non-existant threshold", me);
    biffAdd(TEN, err); return 1;
  }
  if (tenTensorCheck(nin, nrrdTypeFloat, AIR_TRUE)) {
    sprintf(err, "%s: ", me);
    biffAdd(TEN, err); return 1;
  }

  sx = nin->axis[1].size;
  sy = nin->axis[2].size;
  sz = nin->axis[3].size;
  N = sx*sy*sz;
  if (nrrdMaybeAlloc(nout, nrrdTypeFloat, 4, 9, sx, sy, sz)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  for (I=0; I<=N-1; I++) {
    seven = (float*)(nin->data) + I*7;
    nine = (float*)(nout->data) + I*9;
    if (seven[0] < thresh) {
      ELL_3M_ZERO_SET(nine);
      continue;
    }
    TEN_LIST2MAT(nine, seven);
  }
  if (nrrdAxesCopy(nout, nin, NULL, NRRD_AXESINFO_SIZE_BIT)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, NRRD); return 1;
  }

  return 0;
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
  ret = ell3mEigensolve(eval, evec, m, AIR_TRUE);
  ELL_3V_COPY(_eval, eval);
  ELL_3M_COPY(_evec, evec);
  if (tenVerbose && _eval[2] < 0) {
    fprintf(stderr, "% 15.7f % 15.7f % 15.7f\n", 
	    t[1], t[2], t[3]);
    fprintf(stderr, "% 15.7f % 15.7f % 15.7f\n", 
	    t[2], t[4], t[5]);
    fprintf(stderr, "% 15.7f % 15.7f % 15.7f\n", 
	    t[3], t[5], t[6]);
    fprintf(stderr, " --> % 15.7f % 15.7f % 15.7f\n",
	    _eval[0], _eval[1], _eval[2]);
  }
  return ret;
}
