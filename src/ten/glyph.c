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

tenGlyphParm *
tenGlyphParmNew() {
  tenGlyphParm *parm;

  parm = calloc(1, sizeof(tenGlyphParm));
  if (parm) {
    parm->anisoType = tenAnisoUnknown;
    parm->colEvec = 0;  /* first */
    parm->colSat = 1; 
    parm->colGamma = 1;
    parm->siloWidth = 0.8;
    parm->edgeWidth = 0.4;
    parm->anisoThresh = AIR_NAN;
    parm->confThresh = parm->useColor = AIR_NAN;
    parm->maskThresh = AIR_NAN;
    parm->glyphScale = 1.0;
  }
  return parm;
}

tenGlyphParm *
tenGlyphParmNix(tenGlyphParm *parm) {

  return airFree(parm);
}

int
tenGlyphParmCheck(tenGlyphParm *parm, Nrrd *nten) {
  char me[]="tenGlyphParmCheck", err[AIR_STRLEN_MED];

  if (!(parm && nten)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( AIR_IN_OP(tenAnisoUnknown, parm->anisoType, tenAnisoLast) )) {
    sprintf(err, "%s: invalid anisoType (%d)", me, parm->anisoType);
    biffAdd(TEN, err); return 1;
  }
  if (parm->nmask) {
    if (!( 3 == parm->nmask->dim
	   && parm->nmask->axis[0].size == nten->axis[1].size
	   && parm->nmask->axis[1].size == nten->axis[2].size
	   && parm->nmask->axis[2].size == nten->axis[3].size )) {
      sprintf(err, "%s: mask isn't 3-D or doesn't have sizes (%d,%d,%d)", me,
	      nten->axis[1].size, nten->axis[2].size, nten->axis[3].size);
      biffAdd(TEN, err); return 1;
    }
    if (!(AIR_EXISTS(parm->maskThresh))) {
      sprintf(err, "%s: maskThresh hasn't been set", me);
      biffAdd(TEN, err); return 1;
    }
  }
  if (!( AIR_EXISTS(parm->anisoThresh)
	 && AIR_EXISTS(parm->confThresh) )) {
    sprintf(err, "%s: anisoThresh and confThresh haven't both been set", me);
    biffAdd(TEN, err); return 1;
  }
  return 0;
}

int
tenGlyphGen(limnObj *obj, Nrrd *nten, tenGlyphParm *parm) {
  char me[]="tenGlyphGen", err[AIR_STRLEN_MED];
  gageShape *shape;
  airArray *mop;
  double I[3], W[3];
  float *tdata, evec[9], eval[3], *cvec, aniso[TEN_ANISO_MAX+1],
    mA[16], mB[16], R, G, B;
  int idx, ri;
  limnPart *cube;

  if (!(obj && nten && parm)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  mop = airMopNew();
  shape = gageShapeNew();
  airMopAdd(mop, shape, (airMopper)gageShapeNix, airMopAlways);
  if (tenTensorCheck(nten, nrrdTypeFloat, AIR_TRUE)) {
    sprintf(err, "%s: didn't get a valid DT volume", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (tenGlyphParmCheck(parm, nten)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (gageShapeSet(shape, nten, tenGageKind->baseDim)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, GAGE); airMopError(mop); return 1;
  }
  idx = 0;
  for (I[2]=0; I[2]<shape->size[2]; I[2]++) {
    for (I[1]=0; I[1]<shape->size[1]; I[1]++) {
      for (I[0]=0; I[0]<shape->size[0]; idx++, I[0]++) {
	tdata = (float*)(nten->data) + 7*idx;
	if (!( tdata[0] >= parm->confThresh ))
	  continue;
	if (parm->nmask) {
	  if (!( nrrdFLookup[parm->nmask->type](parm->nmask->data, idx)
		 >= parm->maskThresh ))
	    continue;
	}
	tenEigensolve(eval, evec, tdata);
	tenAnisoCalc(aniso, eval);
	if (!( aniso[parm->anisoType] >= parm->anisoThresh ))
	  continue;
	gageShapeUnitItoW(shape, W, I);
	
	/* reset transform */
	ELL_4M_IDENTITY_SET(mA);

	/* scale by eigenvalues */
	ELL_3V_SCALE(eval, parm->glyphScale, eval);
	ELL_4M_SCALE_SET(mB, eval[0], eval[1], eval[2]);
	ell4mPostMul_f(mA, mB);
	
	/* rotate by eigenvectors */
	ELL_43M_INSET(mB, evec);
	ell4mPostMul_f(mA, mB);

	/* translate to sample location */
	ELL_4M_TRANSLATE_SET(mB, W[0], W[1], W[2]);
	ell4mPostMul_f(mA, mB);

	ri = limnObjCubeAdd(obj, 0);
	cube = obj->r + ri;
	cvec = evec + 3*(AIR_CLAMP(0, parm->colEvec, 2));
	R = AIR_ABS(cvec[0]);
	G = AIR_ABS(cvec[1]);
	B = AIR_ABS(cvec[2]);
	R = AIR_AFFINE(0.0, parm->colSat, 1.0, 1.0, R);
	G = AIR_AFFINE(0.0, parm->colSat, 1.0, 1.0, G);
	B = AIR_AFFINE(0.0, parm->colSat, 1.0, 1.0, B);
	R = pow(R, parm->colGamma);
	G = pow(G, parm->colGamma);
	B = pow(B, parm->colGamma);
	ELL_4V_SET(cube->rgba, R, G, B, 1);
	limnObjPartTransform(obj, ri, mA);
      }
    }
  }
  
  airMopOkay(mop);
  return 0;
}
