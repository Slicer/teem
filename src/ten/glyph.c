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

int
tenGlyphGen(limnObj *obj, Nrrd *nin, tenGlyphParm *parm) {
  char me[]="tenGlyphGen", err[128];
  int idx, sx, sy, sz, xi, yi, zi, ri, Ri, Gi, Bi;
  float sum, *data, *ten, x, y, z, tt[16], eval[3], evec[9],
    sc[3], cl, cp, c[TEN_ANISO_MAX+1], R, G, B, H, S, V, *vThreshVol;
  limnPart *r;
  
  if (!(obj && nin && parm)) {
    sprintf(err, "%s: got NULL pointer", me); biffAdd(TEN, err); return 1;
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

	tenAnisoCalc(c, eval);
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
	  ELL_3V_SCALE(sc, parm->cscale, eval);
	  
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
