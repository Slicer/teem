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
tenExpand(Nrrd *nout, Nrrd *nin, float scale, float thresh) {
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
    ELL_3M_SCALE(nine, scale, nine);
  }
  if (nrrdAxesCopy(nout, nin, NULL, NRRD_AXESINFO_SIZE_BIT)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  AIR_FREE(nout->axis[0].label);
  nout->axis[0].label = airStrdup("matrix");

  return 0;
}

int
tenShrink(Nrrd *tseven, Nrrd *nconf, Nrrd *tnine) {
  char me[]="tenShrink", err[AIR_STRLEN_MED];
  int sx, sy, sz;
  float *seven, *conf, *nine;
  size_t I, N;
  
  if (!(tseven && tnine)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( nrrdTypeFloat == tnine->type &&
	 4 == tnine->dim &&
	 9 == tnine->axis[0].size )) {
    sprintf(err, "%s: type not %s (was %s) or dim not 4 (was %d) "
	    "or first axis size not 9 (was %d)", me,
	    airEnumStr(nrrdType, nrrdTypeFloat),
	    airEnumStr(nrrdType, tnine->type),
	    tnine->dim, tnine->axis[0].size);
    biffAdd(TEN, err); return 1;
  }
  sx = tnine->axis[1].size;
  sy = tnine->axis[2].size;
  sz = tnine->axis[3].size;
  if (nconf) {
    if (!( nrrdTypeFloat == nconf->type &&
	   3 == nconf->dim &&
	   sx == nconf->axis[0].size &&
	   sy == nconf->axis[1].size &&
	   sz == nconf->axis[2].size )) {
      sprintf(err, "%s: confidence type not %s (was %s) or dim not 3 (was %d) "
	      "or dimensions didn't match tensor volume", me,
	      airEnumStr(nrrdType, nrrdTypeFloat),
	      airEnumStr(nrrdType, nconf->type),
	      nconf->dim);
      biffAdd(TEN, err); return 1;
    }
  }
  if (nrrdMaybeAlloc(tseven, nrrdTypeFloat, 4, 7, sx, sy, sz)) {
    sprintf(err, "%s: trouble allocating output", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  seven = tseven->data;
  conf = nconf ? nconf->data : NULL;
  nine = tnine->data;
  N = sx*sy*sz;
  for (I=0; I<N; I++) {
    TEN_MAT2LIST(seven, nine);
    seven[0] = conf ? conf[I] : 1.0;
    seven += 7;
    nine += 9;
  }
  if (nrrdAxesCopy(tseven, tnine, NULL, NRRD_AXESINFO_SIZE_BIT)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  AIR_FREE(tseven->axis[0].label);
  tseven->axis[0].label = airStrdup("tensor");

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

/*
******** tenTensorMake
**
** create a tensor nrrd from nrrds of confidence, eigenvalues, and
** eigenvectors
*/
int
tenTensorMake(Nrrd *nout, Nrrd *nconf, Nrrd *neval, Nrrd *nevec) {
  char me[]="tenTensorMake", err[AIR_STRLEN_MED];
  int sx, sy, sz;
  size_t I, N;
  float *out, *conf, tmpMat1[9], tmpMat2[9], diag[9], *eval, *evec, evecT[9];
  int map[4];

  if (!(nout && nconf && neval && nevec)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (nrrdCheck(nconf) || nrrdCheck(neval) || nrrdCheck(nevec)) {
    sprintf(err, "%s: didn't get three valid nrrds", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  if (!( 3 == nconf->dim && nrrdTypeFloat == nconf->type )) {
    sprintf(err, "%s: first nrrd not a confidence volume "
	    "(dim = %d, not 3; type = %s, not %s)", me,
	    nconf->dim, airEnumStr(nrrdType, nconf->type),
	    airEnumStr(nrrdType, nrrdTypeFloat));
    biffAdd(TEN, err); return 1;
  }
  sx = nconf->axis[0].size;
  sy = nconf->axis[1].size;
  sz = nconf->axis[2].size;
  if (!( 4 == neval->dim && 4 == nevec->dim &&
	 nrrdTypeFloat == neval->type &&
	 nrrdTypeFloat == nevec->type )) {
    sprintf(err, "%s: second and third nrrd aren't both 4-D (%d and %d) "
	    "and type %s (%s and %s)",
	    me, neval->dim, nevec->dim,
	    airEnumStr(nrrdType, nrrdTypeFloat),
	    airEnumStr(nrrdType, neval->type),
	    airEnumStr(nrrdType, nevec->type));
    biffAdd(TEN, err); return 1;
  }
  if (!( 3 == neval->axis[0].size &&
	 sx == neval->axis[1].size &&
	 sy == neval->axis[2].size &&
	 sz == neval->axis[3].size )) {
    sprintf(err, "%s: second nrrd sizes wrong: (%d,%d,%d,%d) not (3,%d,%d,%d)",
	    me, neval->axis[0].size, neval->axis[1].size,
	    neval->axis[2].size, neval->axis[3].size,
	    sx, sy, sz);
    biffAdd(TEN, err); return 1;
  }
  if (!( 9 == nevec->axis[0].size &&
	 sx == nevec->axis[1].size &&
	 sy == nevec->axis[2].size &&
	 sz == nevec->axis[3].size )) {
    sprintf(err, "%s: third nrrd sizes wrong: (%d,%d,%d,%d) not (9,%d,%d,%d)",
	    me, nevec->axis[0].size, nevec->axis[1].size,
	    nevec->axis[2].size, nevec->axis[3].size,
	    sx, sy, sz);
    biffAdd(TEN, err); return 1;
  }

  /* finally */
  if (nrrdMaybeAlloc(nout, nrrdTypeFloat, 4, 7, sx, sy, sz)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  N = sx*sy*sz;
  conf = nconf->data;
  eval = neval->data;
  evec = nevec->data;
  out = nout->data;
  for (I=0; I<N; I++) {
    ELL_3M_ZERO_SET(diag);
    ELL_3M_DIAG_SET(diag, eval[0], eval[1], eval[2]);
    ELL_3M_TRANSPOSE(evecT, evec);
    ELL_3M_MUL(tmpMat1, diag, evecT);
    ELL_3M_MUL(tmpMat2, evec, tmpMat1);
    out[0] = conf[I];
    TEN_MAT2LIST(out, tmpMat2);
    out += 7;
    eval += 3;
    evec += 9;
  }
  ELL_4V_SET(map, -1, 0, 1, 2);
  if (nrrdAxesCopy(nout, nconf, map, NRRD_AXESINFO_SIZE_BIT)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  AIR_FREE(nout->axis[0].label);
  nout->axis[0].label = airStrdup("tensor");

  return 0;
}
