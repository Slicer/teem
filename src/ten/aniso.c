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

float tenAnisoSigma = 0.000001;

/*
******** tenAnisoCalc
**
** given an array of three SORTED (descending) eigenvalues "e",
** calculates the anisotropy coefficients of Westin et al.,
** as well as various others.
**
** This does NOT use biff.  
*/
void
tenAnisoCalc(float c[TEN_ANISO_MAX+1], float e[3]) {
  float e0, e1, e2, stdv, mean, sum, cl, cp, ca, ra, fa, vf, denom;

  if (!( e[0] >= e[1] && e[1] >= e[2] )) {
    fprintf(stderr, "tenAnisoCalc: eigen values not sorted: "
	    "%g %g %g (%d %d)\n",
	    e[0], e[1], e[2], e[0] >= e[1], e[1] >= e[2]);
  }
  if (!( e[0] >= 0 && e[1] >= 0 && e[2] >= 0 )) {
    fprintf(stderr, "tenAnisoCalc: eigen values not all >= 0: %g %g %g\n",
	    e[0], e[1], e[2]);
  }
  e0 = AIR_MAX(e[0], 0);
  e1 = AIR_MAX(e[1], 0);
  e2 = AIR_MAX(e[2], 0);
  sum = e0 + e1 + e2;
  
  /* first version of cl, cp, cs */
  cl = (e0 - e1)/(tenAnisoSigma + sum);
  c[tenAniso_Cl1] = cl;
  cp = 2*(e1 - e2)/(tenAnisoSigma + sum);
  c[tenAniso_Cp1] = cp;
  ca = cl + cp;
  c[tenAniso_Ca1] = ca;
  c[tenAniso_Cs1] = 1 - ca;
  c[tenAniso_Ct1] = ca ? cp/ca : 0;
  /* second version of cl, cp, cs */
  cl = (e0 - e1)/(tenAnisoSigma + e0);
  c[tenAniso_Cl2] = cl;
  cp = (e1 - e2)/(tenAnisoSigma + e0);
  c[tenAniso_Cp2] = cp;
  ca = cl + cp;
  c[tenAniso_Ca2] = ca;
  c[tenAniso_Cs2] = 1 - ca;
  c[tenAniso_Ct2] = ca ? cp/ca : 0;
  /* non-westin anisos */
  mean = sum/3.0;
  stdv = sqrt((mean-e0)*(mean-e0)   /* okay, not exactly standard dev */
	      + (mean-e1)*(mean-e1) 
	      + (mean-e2)*(mean-e2));
  ra = stdv/(mean*sqrt(6.0));  ra = AIR_CLAMP(0.0, ra, 1.0);
  c[tenAniso_RA] = ra;
  denom = 2.0*(e0*e0 + e1*e1 + e2*e2);
  if (denom) {
    fa = stdv*sqrt(3.0/denom);
    fa = AIR_CLAMP(0.0, fa, 1.0);
  } else {
    fa = 0.0;
  }
  c[tenAniso_FA] = fa;
  vf = 1 - e0*e1*e2/(mean*mean*mean);
  vf = AIR_CLAMP(0.0, vf, 1.0);
  c[tenAniso_VF] = vf;
  return;
}

int
tenAnisoPlot(Nrrd *nout, int aniso, int res) {
  char me[]="tenAnisoMap", err[AIR_STRLEN_MED];
  float *out, c[TEN_ANISO_MAX+1];
  int x, y;
  float m0[3], m1[3], m2[3], c0, c1, c2, e[3];
  float S = 1/3.0, L = 1, P = 1/2.0;  /* these make Westin's original
					 (cl,cp,cs) align with the 
					 barycentric coordinates */

  if (!airEnumValidVal(tenAniso, aniso)) {
    sprintf(err, "%s: invalid aniso (%d)", me, aniso);
    biffAdd(TEN, err); return 1;
  }
  if (!(res > 2)) {
    sprintf(err, "%s: resolution (%d) invalid", me, res);
    biffAdd(TEN, err); return 1;
  }
  if (nrrdMaybeAlloc(nout, nrrdTypeFloat, 2, res, res)) {
    sprintf(err, "%s: ", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  out = nout->data;
  ELL_3V_SET(m0, S, S, S);
  ELL_3V_SET(m1, L, 0, 0);
  ELL_3V_SET(m2, P, P, 0);
  for (y=0; y<res; y++) {
    for (x=0; x<=y; x++) {
      /* (c0,c1,c2) are the barycentric coordinates */
      c0 = 1 - AIR_AFFINE(-0.5, y, res-0.5, 0.0, 1.0);
      c2 = AIR_AFFINE(-0.5, x, res-0.5, 0.0, 1.0);
      c1 = 1 - c0 - c2;
      e[0] = c0*m0[0] + c1*m1[0] + c2*m2[0];
      e[1] = c0*m0[1] + c1*m1[1] + c2*m2[1];
      e[2] = c0*m0[2] + c1*m1[2] + c2*m2[2];
      tenAnisoCalc(c, e);
      out[x + res*y] = c[aniso];
    }
  }

  return 0;
}

int
tenAnisoVolume(Nrrd *nout, Nrrd *nin, int aniso, float thresh) {
  char me[]="tenAnisoVolume", err[AIR_STRLEN_MED];
  size_t N, I;
  float *out, *in, *tensor, eval[3], evec[9], c[TEN_ANISO_MAX+1];
  int d, sx, sy, sz, map[NRRD_DIM_MAX];

  if (!tenValidTensor(nin, nrrdTypeFloat, AIR_TRUE)) {
    sprintf(err, "%s: didn't get a tensor nrrd", me);
    biffAdd(TEN, err); return 1;
  }
  if (!airEnumValidVal(tenAniso, aniso)) {
    sprintf(err, "%s: invalid aniso (%d)", me, aniso);
    biffAdd(TEN, err); return 1;
  }
  thresh = AIR_CLAMP(0.0, thresh, 1.0);

  sx = nin->axis[1].size;
  sy = nin->axis[2].size;
  sz = nin->axis[3].size;
  N = sx*sy*sz;
  if (nrrdMaybeAlloc(nout, nrrdTypeFloat, 3, sx, sy, sz)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  out = nout->data;
  in = nin->data;
  for (I=0; I<=N-1; I++) {
    tensor = in + I*7;
    if (tensor[0] < thresh) {
      out[I] = 0.0;
      continue;
    }
    tenEigensolve(eval, evec, tensor);
    tenAnisoCalc(c, eval);
    out[I] = c[aniso];
  }
  for (d=0; d<=2; d++) {
    map[d] = d+1;
  }
  if (nrrdAxesCopy(nout, nin, map, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, NRRD); return 1;
  }

  return 0;
}


