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

int
tenGlyphGen(limnObj *obj, Nrrd *nin, tenGlyphParm *parm) {
  char me[]="tenGlyphGen", err[128];
  int idx, sx, sy, sz, xi, yi, zi, ri, Ri, Gi, Bi;
  float sum, *data, *ten, x, y, z, tt[16], eval[3], evec[9],
    sc[3], cl, cp, c[TEN_MAX_ANISO+1], R, G, B, H, S, V, *vThreshVol;
  limnPart *r;
  
  if (!(obj && nin && parm)) {
    sprintf(err, BIFF_NULL, me); biffAdd(TEN, err); return 1;
  }
  if (!tenValidTensor(nin, nrrdTypeFloat, AIR_TRUE)) {
    sprintf(err, "%s: didn't get a tensor volume", me);
    biffAdd(TEN, err); return 1;
  }
  data = nin->data;
  sx = nin->axis[1].size;
  sy = nin->axis[2].size;
  sz = nin->axis[3].size;
  if (parm->vThreshVol) {
    if (!(nrrdTypeFloat == parm->vThreshVol->type &&
	  sx == parm->vThreshVol->axis[0].size &&
	  sy == parm->vThreshVol->axis[1].size &&
	  sz == parm->vThreshVol->axis[2].size)) {
      sprintf(err, "%s: thresh volume not %dx%dx%d floats (%dx%dx%d)", 
	      me, sx, sy, sz, 
	      parm->vThreshVol->axis[0].size,
	      parm->vThreshVol->axis[1].size,
	      parm->vThreshVol->axis[2].size);
      biffAdd(TEN, err); return 1;
    }
    vThreshVol = parm->vThreshVol->data;
  }
  else {
    vThreshVol = NULL;
  }

  idx = 0;
  for (zi=0; zi<sz; zi++) {
    z = AIR_AFFINE(-0.5, zi, sz-0.5, -(sz-1)/2.0, (sz-1)/2.0);
    for (yi=0; yi<sy; yi++) {
      y = AIR_AFFINE(-0.5, yi, sy-0.5, -(sy-1)/2.0, (sy-1)/2.0);
      for (xi=0; xi<sx; xi++) {
	x = AIR_AFFINE(-0.5, xi, sx-0.5, -(sx-1)/2.0, (sx-1)/2.0);
	
	ten = data + 7*idx;
	if (ten[0] < 0.5) {
	  goto nextiter;
	}

	tenEigensolve(eval, evec, ten);
	sum = eval[0] + eval[1] + eval[2];
	if (sum < parm->sumFloor || sum > parm->sumCeil)
	  goto nextiter;

	/*
	if (AIR_ABS((evec+0)[0]) < 0.8)
	  goto nextiter;
	*/

	tenAnisotropy(c, eval);
	if (vThreshVol) {
	  if (vThreshVol[idx] < parm->vThresh) {
	    goto nextiter;
	  }
	}
	else {
	  cl = c[tenAniso_Cl];
	  cp = c[tenAniso_Cp];
	  if (AIR_LERP(parm->anisoType, cl, cp) < parm->anisoThresh) {
	    goto nextiter;
	  }
	}
	if (1 || ten[0] > parm->dwiThresh) {
	  ELL_3V_SCALE(sc, eval, parm->cscale);
	  
	  switch (parm->dim) {
	  case 3:
	    ri = limnObjCubeAdd(obj, 2);
	    break;
	  case 2:
	    ri = limnObjSquareAdd(obj, 2);
	    break;
	  case 1:
	    ri = limnObjLoneEdgeAdd(obj, 2);
	    break;
	  }
	  ELL_4M_SET_SCALE(tt,sc[0],sc[1],sc[2]);
	  limnObjPartTransform(obj, ri, tt);
	  ELL_43M_INSET(tt, evec);
	  limnObjPartTransform(obj, ri, tt);
	  ELL_4M_SET_TRANSLATE(tt, x, y, z);
	  limnObjPartTransform(obj, ri, tt);

	  r = obj->r + ri;
	  R = parm->fakeSat*AIR_ABS(evec[0+3*0]);
	  G = parm->fakeSat*AIR_ABS(evec[1+3*0]);
	  B = parm->fakeSat*AIR_ABS(evec[2+3*0]);
	  R = AIR_CLAMP(0, R, 1);
	  G = AIR_CLAMP(0, G, 1);
	  B = AIR_CLAMP(0, B, 1);
	  dyeRGBtoHSV(&H, &S, &V, R, G, B);
	  S *= pow(c[tenAniso_Cl], 0.7);
	  dyeHSVtoRGB(&R, &G, &B, H, S, V);
	  R = AIR_AFFINE(0, parm->useColor, 1, 1, R);
	  G = AIR_AFFINE(0, parm->useColor, 1, 1, G);
	  B = AIR_AFFINE(0, parm->useColor, 1, 1, B);
	  AIR_INDEX(0, R, 1, 256, Ri);
	  AIR_INDEX(0, G, 1, 256, Gi);
	  AIR_INDEX(0, B, 1, 256, Bi);
	  
	  ELL_3V_SET(r->rgba, Ri, Gi, Bi);
	}

      nextiter:
	idx++;
      }
    }
  }

  return 0;
}
