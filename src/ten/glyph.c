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
tenGlyphGen(limnObj *obj, Nrrd *nin, float useColor,
	    float anisoThresh, float anisoType,
	    float dwiThresh, float cscale) {
  char me[]="tenGlyphGen", err[128];
  int idx, sx, sy, sz, xi, yi, zi, ri, Ri, Gi, Bi;
  float aniso, *data, *ten, x, y, z, tt[16], eval[3], evec[9],
    sc[3], c[5], R, G, B, H, S, V;
  limnPart *r;
  
  if (!(obj && nin)) {
    sprintf(err, BIFF_NULL, me); biffAdd(TEN, err); return 1;
  }
  if (!tenValidTensor(nin, nrrdTypeFloat, AIR_TRUE)) {
    sprintf(err, "%s: didn't get a tensor volume", me);
    biffAdd(TEN, err); return 1;
  }

  data = nin->data;
  sx = nin->size[1];
  sy = nin->size[2];
  sz = nin->size[3];
  idx = 0;
  for (zi=0; zi<sz; zi++) {
    z = AIR_AFFINE(-0.5, zi, sz-0.5, -(sz-1)/2.0, (sz-1)/2.0);
    for (yi=0; yi<sy; yi++) {
      y = AIR_AFFINE(-0.5, yi, sy-0.5, -(sy-1)/2.0, (sy-1)/2.0);
      for (xi=0; xi<sx; xi++) {
	x = AIR_AFFINE(-0.5, xi, sx-0.5, -(sx-1)/2.0, (sx-1)/2.0);
	
	ten = data+7*idx;
	tenEigensolve(eval, evec, ten);
	tenAnisotropy(c, eval);
	aniso = (c[tenAnisoC_l]*(1-anisoType) +
		 c[tenAnisoC_p]*anisoType);

	if (ten[0] > dwiThresh && aniso > anisoThresh) {
	  ELL_3V_SCALE(sc, eval, cscale);
	  ri = limnObjCubeAdd(obj, 2);
	  limnObjPartTransform(obj, ri, 
			       ELL_4M_SET_SCALE(tt,sc[0],sc[1],sc[2]));
	  limnObjPartTransform(obj, ri, ELL_43M_INSET(tt, evec));
	  limnObjPartTransform(obj, ri, ELL_4M_SET_TRANSLATE(tt, x, y, z));
	  
	  r = obj->r + ri;
	  R = 1.5*AIR_ABS(evec[0+3*0]);
	  G = 1.5*AIR_ABS(evec[1+3*0]);
	  B = 1.5*AIR_ABS(evec[2+3*0]);
	  R = AIR_CLAMP(0, R, 1);
	  G = AIR_CLAMP(0, G, 1);
	  B = AIR_CLAMP(0, B, 1);
	  dyeRGBtoHSV(&H, &S, &V, R, G, B);
	  S *= pow(c[tenAnisoC_l], 0.7);
	  dyeHSVtoRGB(&R, &G, &B, H, S, V);
	  R = AIR_AFFINE(0, useColor, 1, 1, R);
	  G = AIR_AFFINE(0, useColor, 1, 1, G);
	  B = AIR_AFFINE(0, useColor, 1, 1, B);
	  AIR_INDEX(0, R, 1, 256, Ri);
	  AIR_INDEX(0, G, 1, 256, Gi);
	  AIR_INDEX(0, B, 1, 256, Bi);
	  ELL_3V_SET(r->rgba, Ri, Gi, Bi);
	}

	idx++;
      }
    }
  }

  return 0;
}
