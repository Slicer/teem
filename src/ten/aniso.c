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

/*
******** tenAnistropy
**
** given an array of three SORTED (descending) eigenvalues "e",
** calculates the anisotropy coefficients of Westin et al.,
** and stores them in the "c" array.
** 
** c[tenAnisoC_l]: c_l; linear 
** c[tenAnisoC_p]: c_p; planar
** c[tenAnisoC_a]: c_a: c_l + c_p
** c[tenAnisoC_s]: c_s: 1 - c_a
** c[tenAnisoC_t]: c_theta: Gordon's anisotropy type: 0:linear <-> 1:planar
**
** For a time, this function did not clamp the cl and cp values
** between 0.0 and 1.0.  Some sort of bone-headed purist argument.
** Caused nothing but trouble, really. The problem was that in
** measured tensor data there were voxels for which the "threshold"
** (based on sum of diffusion-weighted images) was 1.0, but the voxel
** actually had physically impossible eigenvalues, so no amount of
** threshold-based trickery could reign the crazy values back into line.
** Naive clamping of the anisotropy into a physically plausible range
** is the next best thing, as far as I can tell.
**
** This does NOT use biff.  
*/
void
tenAnisotropy(float c[TEN_MAX_ANISO+1], float e[3]) {
  float sum, cl, cp, ca;
  
  sum = e[0] + e[1] + e[2];
  
  if (sum) {
    cl = (e[0] - e[1])/sum;   cl = AIR_CLAMP(0.0, cl, 1.0);
    cp = 2*(e[1] - e[2])/sum; cp = AIR_CLAMP(0.0, cp, 1.0);
    c[tenAnisoC_l] = cl;
    c[tenAnisoC_p] = cp;
    ca = cl + cp;             ca = AIR_CLAMP(0.0, ca, 1.0);
    c[tenAnisoC_a] = ca;
    c[tenAnisoC_s] = 1 - ca;
    c[tenAnisoC_t] = ca ? cp/ca : 0;
  }
  else {
    c[tenAnisoC_l] = 0.0;
    c[tenAnisoC_p] = 0.0;
    c[tenAnisoC_a] = 0.0;
    c[tenAnisoC_s] = 0.0;
    c[tenAnisoC_t] = 0.0;
  }
  return;
}

int
tenAnisoVolume(Nrrd *nout, Nrrd *nin, float anisoType) {
  char me[]="tenAnisoVolume", err[128];
  nrrdBigInt N, I;
  float *out, *in, *tensor, eval[3], evec[9], c[TEN_MAX_ANISO+1], cl, cp;
  int d, map[NRRD_DIM_MAX];

  if (!tenValidTensor(nin, nrrdTypeFloat, AIR_TRUE)) {
    sprintf(err, "%s: didn't get a tensor nrrd", me);
    biffAdd(TEN, err); return 1;
  }
  N = nin->axis[1].size*nin->axis[2].size*nin->axis[3].size;
  if (nrrdMaybeAlloc_va(nout, nrrdTypeFloat, 3,
			nin->axis[1].size, 
			nin->axis[2].size, 
			nin->axis[3].size)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  for (d=0; d<=2; d++) {
    map[d] = d+1;
  }
  out = nout->data;
  in = nin->data;
  for (I=0; I<=N-1; I++) {
    tensor = &(in[I*7]);
    if (tensor[0] < 0.5) {
      out[I] = 0.0;
      continue;
    }
    tenEigensolve(eval, evec, tensor);
    tenAnisotropy(c, eval);
    cl = c[tenAnisoC_l];
    cp = c[tenAnisoC_p];
    out[I] = AIR_LERP(anisoType, cl, cp);
  }
  if (nrrdAxesCopy(nout, nin, map, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  return 0;
}


