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
** as well as various others, 
** 
** c[tenAniso_Cl]: c_l; linear 
** c[tenAniso_Cp]: c_p; planar
** c[tenAniso_Ca]: c_a: c_l + c_p
** c[tenAniso_Cs]: c_s: 1 - c_a
** c[tenAniso_Ct]: c_theta: Gordon's anisotropy type: 0:linear <-> 1:planar
** c[tenAniso_RA]: Bass+Pier's relative anisotropy
** c[tenAniso_FA]: (Bass+Pier's fractional anisotropy)/sqrt(2)
** c[tenAniso_VR]: 1 - (Bass+Pier's volume ratio)
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
  float stdv, mean, sum, cl, cp, ca, ra, fa, vf, denom;
  
  sum = e[0] + e[1] + e[2];
  
  if (sum) {
    cl = (e[0] - e[1])/sum;   cl = AIR_CLAMP(0.0, cl, 1.0);
    c[tenAniso_Cl] = cl;
    cp = 2*(e[1] - e[2])/sum; cp = AIR_CLAMP(0.0, cp, 1.0);
    c[tenAniso_Cp] = cp;
    ca = cl + cp;             ca = AIR_CLAMP(0.0, ca, 1.0);
    c[tenAniso_Ca] = ca;
    c[tenAniso_Cs] = 1 - ca;
    c[tenAniso_Ct] = ca ? cp/ca : 0;
    mean = sum/3.0;
    stdv = sqrt((mean-e[0])*(mean-e[0])   /* okay, not exactly standard dev */
		+ (mean-e[1])*(mean-e[1]) 
		+ (mean-e[2])*(mean-e[2]));
    ra = stdv/(mean*sqrt(6.0));  ra = AIR_CLAMP(0.0, ra, 1.0);
    c[tenAniso_RA] = ra;
    denom = 2.0*(e[0]*e[0] + e[1]*e[1] + e[2]*e[2]);
    if (denom) {
      fa = stdv*sqrt(3.0/denom);
      fa = AIR_CLAMP(0.0, fa, 1.0);
    }
    else {
      fa = 0.0;
    }
    c[tenAniso_FA] = fa;
    vf = 1 - e[0]*e[1]*e[2]/(mean*mean*mean);
    vf = AIR_CLAMP(0.0, vf, 1.0);
    c[tenAniso_VF] = vf;
  }
  else {
    c[tenAniso_Cl] = 0.0;
    c[tenAniso_Cp] = 0.0;
    c[tenAniso_Ca] = 0.0;
    c[tenAniso_Cs] = 0.0;
    c[tenAniso_Ct] = 0.0;
    c[tenAniso_RA] = 0.0;
    c[tenAniso_FA] = 0.0;
    c[tenAniso_VF] = 0.0;
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
    cl = c[tenAniso_Cl];
    cp = c[tenAniso_Cp];
    out[I] = AIR_LERP(anisoType, cl, cp);
  }
  if (nrrdAxesCopy(nout, nin, map, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  return 0;
}


