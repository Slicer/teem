/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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
#include "privateTen.h"

tenGlyphParm *
tenGlyphParmNew() {
  tenGlyphParm *parm;

  parm = calloc(1, sizeof(tenGlyphParm));
  if (parm) {
    parm->nmask = NULL;
    parm->anisoType = tenAnisoUnknown;
    parm->onlyPositive = AIR_TRUE;
    parm->confThresh = AIR_NAN;
    parm->anisoThresh = AIR_NAN;
    parm->maskThresh = AIR_NAN;

    parm->glyphType = tenGlyphTypeUnknown;
    parm->facetRes = 10;
    parm->glyphScale = 1.0;
    parm->sqdSharp = 3.0;
    ELL_5V_SET(parm->edgeWidth, 0.0, 0.0, 0.4, 0.2, 0.1);

    parm->colEvec = 0;  /* first */
    parm->colMaxSat = 1; 
    parm->colGamma = 1;
    parm->colIsoGray = 1;
    parm->colAnisoType = tenAnisoUnknown;
    parm->colAnisoModulate = 0;
    ELL_4V_SET(parm->ADSP, 0, 1, 0, 30);

    parm->doSlice = AIR_FALSE;
    parm->sliceAxis = -1;
    parm->slicePos = -1;
    parm->sliceAnisoType = tenAnisoUnknown;
    parm->sliceOffset = 0.0;
    parm->sliceBias = 0.05;
    parm->sliceGamma = 1.0;
  }
  return parm;
}

tenGlyphParm *
tenGlyphParmNix(tenGlyphParm *parm) {

  return airFree(parm);
}

int
tenGlyphParmCheck(tenGlyphParm *parm, Nrrd *nten, Nrrd *npos, Nrrd *nslc) {
  char me[]="tenGlyphParmCheck", err[AIR_STRLEN_MED];
  int duh, tenSize[3];

  if (!(parm && nten)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (airEnumValCheck(tenAniso, parm->anisoType)) {
    sprintf(err, "%s: unset (or invalid) anisoType (%d)", me, parm->anisoType);
    biffAdd(TEN, err); return 1;
  }
  if (airEnumValCheck(tenAniso, parm->colAnisoType)) {
    sprintf(err, "%s: unset (or invalid) colAnisoType (%d)",
	    me, parm->colAnisoType);
    biffAdd(TEN, err); return 1;
  }
  if (!( parm->facetRes >= 3 )) {
    sprintf(err, "%s: facet resolution %d not >= 3", me, parm->facetRes);
    biffAdd(TEN, err); return 1;
  }
  if (!( AIR_IN_OP(tenGlyphTypeUnknown, parm->glyphType,
		   tenGlyphTypeLast) )) {
    sprintf(err, "%s: unset (or invalid) glyphType (%d)", me, parm->glyphType);
    biffAdd(TEN, err); return 1;
  }
  if (parm->nmask) {
    if (npos) {
      sprintf(err, "%s: can't do masking with explicit coordinate list", me);
      biffAdd(TEN, err); return 1;
    }
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
  if (parm->doSlice) {
    if (npos) {
      sprintf(err, "%s: can't do slice with explicit coordinate list", me);
      biffAdd(TEN, err); return 1;
    }
    if (!( AIR_IN_CL(0, parm->sliceAxis, 2) )) {
      sprintf(err, "%s: slice axis %d invalid", me, parm->sliceAxis);
      biffAdd(TEN, err); return 1;
    }
    if (!( AIR_IN_CL(0, parm->slicePos,
		     nten->axis[1+parm->sliceAxis].size-1) )) {
      sprintf(err, "%s: slice pos %d not in valid range [0..%d]", me,
	      parm->slicePos, nten->axis[1+parm->sliceAxis].size-1);
      biffAdd(TEN, err); return 1;
    }
    if (nslc) {
      if (2 != nslc->dim) {
	sprintf(err, "%s: explicit slice must be 2-D (not %d)", me, nslc->dim);
	biffAdd(TEN, err); return 1;
      }
      tenSize[0] = nten->axis[1].size;
      tenSize[1] = nten->axis[2].size;
      tenSize[2] = nten->axis[3].size;
      for (duh=parm->sliceAxis; duh<2; duh++) {
	tenSize[duh] = tenSize[duh+1];
      }
      if (!( tenSize[0] == nslc->axis[0].size
	     && tenSize[1] == nslc->axis[1].size )) {
	sprintf(err, "%s: axis %d slice of %dx%dx%d volume is not %dx%d", me,
		parm->sliceAxis, nten->axis[1].size, nten->axis[2].size,
		nten->axis[3].size, nslc->axis[0].size, nslc->axis[1].size);
	biffAdd(TEN, err); return 1;
      }
    } else {
      if (airEnumValCheck(tenAniso, parm->sliceAnisoType)) {
	sprintf(err, "%s: unset (or invalid) sliceAnisoType (%d)",
		me, parm->sliceAnisoType);
	biffAdd(TEN, err); return 1;
      }
    }
  }
  return 0;
}

int
tenGlyphGen(limnObject *glyphsLimn, echoScene *glyphsEcho,
	    tenGlyphParm *parm, Nrrd *nten, Nrrd *npos, Nrrd *nslc) {
  char me[]="tenGlyphGen", err[AIR_STRLEN_MED];
  gageShape *shape;
  airArray *mop;
  double pI[3], pW[3];
  float cl, cp, *tdata, evec[9], rotEvec[9], eval[3], *cvec,
    aniso[TEN_ANISO_MAX+1], sRot[16], mA[16], mB[16],
    R, G, B, qA, qB, glyphAniso, sliceGray;
  int slcCoord[3], idx, _idx=0, glyphIdx, axis, numGlyphs, duh;
  limnLook *look; int lookIdx;
  echoObject *eglyph, *inst, *list=NULL, *split, *esquare;
  echoPos_t eM[16], originOffset[3], edge0[3], edge1[3];
  /*
  int eret;
  double tmp1[3], tmp2[3];  
  */

  if (!( (glyphsLimn || glyphsEcho) && nten && parm)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  mop = airMopNew();
  shape = gageShapeNew();
  shape->defaultCenter = nrrdCenterCell;
  airMopAdd(mop, shape, (airMopper)gageShapeNix, airMopAlways);
  if (npos) {
    if (!( 2 == nten->dim && 7 == nten->axis[0].size )) {
      sprintf(err, "%s: nten isn't 2-D 7-by-N array", me);
      biffAdd(TEN, err); airMopError(mop); return 1;
    }
    if (!( 2 == npos->dim && 3 == npos->axis[0].size
	   && nten->axis[1].size == npos->axis[1].size )) {
      sprintf(err, "%s: npos isn't 2-D 3-by-%d array", me, nten->axis[1].size);
      biffAdd(TEN, err); airMopError(mop); return 1;
    }
    if (!( nrrdTypeFloat == nten->type && nrrdTypeFloat == npos->type )) {
      sprintf(err, "%s: nten and npos must be %s, not %s and %s", me,
	      airEnumStr(nrrdType, nrrdTypeFloat),
	      airEnumStr(nrrdType, nten->type),
	      airEnumStr(nrrdType, npos->type));
      biffAdd(TEN, err); airMopError(mop); return 1;
    }
  } else {
    if (tenTensorCheck(nten, nrrdTypeFloat, AIR_TRUE, AIR_TRUE)) {
      sprintf(err, "%s: didn't get a valid DT volume", me);
      biffAdd(TEN, err); airMopError(mop); return 1;
    }
  }
  if (tenGlyphParmCheck(parm, nten, npos, nslc)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (!npos) {
    if (gageShapeSet(shape, nten, tenGageKind->baseDim)) {
      sprintf(err, "%s: trouble", me);
      biffMove(TEN, err, GAGE); airMopError(mop); return 1;
    }
  }
  if (parm->doSlice) {
    ELL_3V_COPY(edge0, shape->voxLen);
    ELL_3V_COPY(edge1, shape->voxLen);
    edge0[parm->sliceAxis] = edge1[parm->sliceAxis] = 0.0;
    switch(parm->sliceAxis) {
    case 0:
      edge0[1] = edge1[2] = 0;
      ELL_4M_ROTATE_Y_SET(sRot, AIR_PI/2);
      break;
    case 1:
      edge0[0] = edge1[2] = 0;
      ELL_4M_ROTATE_X_SET(sRot, AIR_PI/2);
      break;
    case 2: default:
      edge0[0] = edge1[1] = 0;
      ELL_4M_IDENTITY_SET(sRot);
      break;
    }
    ELL_3V_COPY(originOffset, shape->voxLen);
    ELL_3V_SCALE(originOffset, -0.5, originOffset);
    originOffset[parm->sliceAxis] *= -2*parm->sliceOffset;
  }
  if (glyphsLimn) {
    /* create limnLooks for diffuse and ambient-only shading */
  }
  if (glyphsEcho) {
    list = echoObjectNew(glyphsEcho, echoTypeList);
  }
  if (npos) {
    numGlyphs = nten->axis[1].size;
  } else {
    numGlyphs = shape->size[0] * shape->size[1] * shape->size[2];
  }
  for (idx=0; idx<numGlyphs; idx++, _idx = idx) {
    tdata = (float*)(nten->data) + 7*idx;
    if (npos) {
      ELL_3V_COPY(pW, (float*)(npos->data) + 3*idx);
    } else {
      NRRD_COORD_GEN(pI, shape->size, 3, _idx);
      gageShapeUnitItoW(shape, pW, pI);
      if (parm->nmask) {
	if (!( nrrdFLookup[parm->nmask->type](parm->nmask->data, idx)
	       >= parm->maskThresh ))
	  continue;
      }
    }
    tenEigensolve_f(eval, evec, tdata);
    ELL_3M_TRANSPOSE(rotEvec, evec);
    tenAnisoCalc_f(aniso, eval);
    if (parm->doSlice
	&& pI[parm->sliceAxis] == parm->slicePos) {
      /* set sliceGray */
      if (nslc) {
	/* we aren't masked by confidence, as anisotropy slice is */
	for (duh=0; duh<parm->sliceAxis; duh++) {
	  slcCoord[duh] = pI[duh];
	}
	for (duh=duh<parm->sliceAxis; duh<2; duh++) {
	  slcCoord[duh] = pI[duh+1];
	}
	ELL_3V_COPY(slcCoord, pI);
	sliceGray = 
	  nrrdFLookup[nslc->type](nslc->data, slcCoord[0] 
				  + nslc->axis[0].size*slcCoord[1]);
      } else {
	if (!( tdata[0] >= parm->confThresh ))
	  continue;
	sliceGray = aniso[parm->sliceAnisoType];
      }
      if (parm->sliceGamma > 0) {
	sliceGray = AIR_AFFINE(0, sliceGray, 1, parm->sliceBias, 1);
	sliceGray = pow(sliceGray, 1.0/parm->sliceGamma);
      } else {
	sliceGray = AIR_AFFINE(0, sliceGray, 1, 0, 1-parm->sliceBias);
	sliceGray = 1.0 - pow(sliceGray, -1.0/parm->sliceGamma);
      }
      /* make slice contribution */
      if (glyphsLimn) {
	lookIdx = limnObjectLookAdd(glyphsLimn);
	look = glyphsLimn->look + lookIdx;
	ELL_4V_SET(look->rgba, sliceGray, sliceGray, sliceGray, 1);
	ELL_3V_SET(look->kads, 1, 0, 0);
	look->spow = 0;
	glyphIdx = limnObjectSquareAdd(glyphsLimn, lookIdx);
	ELL_4M_IDENTITY_SET(mA);
	ell_4m_post_mul_f(mA, sRot);
	if (!npos) {
	  ELL_4M_SCALE_SET(mB,
			   shape->voxLen[0],
			   shape->voxLen[1],
			   shape->voxLen[2]);
	}
	ell_4m_post_mul_f(mA, mB);
	ELL_4M_TRANSLATE_SET(mB, pW[0], pW[1], pW[2]);
	ell_4m_post_mul_f(mA, mB);
	ELL_4M_TRANSLATE_SET(mB,
			     originOffset[0],
			     originOffset[1],
			     originOffset[2]);
	ell_4m_post_mul_f(mA, mB);
	limnObjectPartTransform(glyphsLimn, glyphIdx, mA);
      }
      if (glyphsEcho) {
	esquare = echoObjectNew(glyphsEcho,echoTypeRectangle);
	ELL_3V_ADD2(((echoRectangle*)esquare)->origin, pW, originOffset);
	ELL_3V_COPY(((echoRectangle*)esquare)->edge0, edge0);
	ELL_3V_COPY(((echoRectangle*)esquare)->edge1, edge1);
	echoColorSet(esquare, sliceGray, sliceGray, sliceGray, 1);
	echoMatterPhongSet(glyphsEcho, esquare, 1, 0, 0, 40);
	echoListAdd(list, esquare);
      }
    }
    if (parm->onlyPositive) {
      if (eval[2] < 0) {
	/* didn't have all positive eigenvalues, its outta here */
	continue;
      }
    }
    if (!( tdata[0] >= parm->confThresh ))
      continue;
    if (!( aniso[parm->anisoType] >= parm->anisoThresh ))
      continue;
    glyphAniso = aniso[parm->colAnisoType];
    /*
      fprintf(stderr, "%s: eret = %d; evals = %g %g %g\n", me,
      eret, eval[0], eval[1], eval[2]);
      ELL_3V_CROSS(tmp1, evec+0, evec+3); tmp2[0] = ELL_3V_LEN(tmp1);
      ELL_3V_CROSS(tmp1, evec+0, evec+6); tmp2[1] = ELL_3V_LEN(tmp1);
      ELL_3V_CROSS(tmp1, evec+3, evec+6); tmp2[2] = ELL_3V_LEN(tmp1);
      fprintf(stderr, "%s: crosses = %g %g %g\n", me,
      tmp2[0], tmp2[1], tmp2[2]);
    */
    
    /* set transform (in mA) */
    ELL_4M_IDENTITY_SET(mA);                        /* reset */
    ELL_3V_SCALE(eval, parm->glyphScale, eval);     /* scale by evals */
    ELL_4M_SCALE_SET(mB, eval[0], eval[1], eval[2]);

    ell_4m_post_mul_f(mA, mB);
    ELL_43M_INSET(mB, rotEvec);                     /* rotate by evecs */
    ell_4m_post_mul_f(mA, mB);
    ELL_4M_TRANSLATE_SET(mB, pW[0], pW[1], pW[2]);  /* translate */
    ell_4m_post_mul_f(mA, mB);
    
    /* set color (in R,G,B) */
    cvec = evec + 3*(AIR_CLAMP(0, parm->colEvec, 2));
    R = AIR_ABS(cvec[0]);                           /* standard mapping */
    G = AIR_ABS(cvec[1]);
    B = AIR_ABS(cvec[2]);
    /* desaturate by colMaxSat */
    R = AIR_AFFINE(0.0, parm->colMaxSat, 1.0, parm->colIsoGray, R);
    G = AIR_AFFINE(0.0, parm->colMaxSat, 1.0, parm->colIsoGray, G);
    B = AIR_AFFINE(0.0, parm->colMaxSat, 1.0, parm->colIsoGray, B);
    /* desaturate some by anisotropy */
    R = AIR_AFFINE(0.0, parm->colAnisoModulate, 1.0,
		   R, AIR_AFFINE(0.0, glyphAniso, 1.0, parm->colIsoGray, R));
    G = AIR_AFFINE(0.0, parm->colAnisoModulate, 1.0,
		   G, AIR_AFFINE(0.0, glyphAniso, 1.0, parm->colIsoGray, G));
    B = AIR_AFFINE(0.0, parm->colAnisoModulate, 1.0,
		   B, AIR_AFFINE(0.0, glyphAniso, 1.0, parm->colIsoGray, B));
    /* clamp and do gamma */
    R = AIR_CLAMP(0.0, R, 1.0);
    G = AIR_CLAMP(0.0, G, 1.0);
    B = AIR_CLAMP(0.0, B, 1.0);
    R = pow(R, parm->colGamma);
    G = pow(G, parm->colGamma);
    B = pow(B, parm->colGamma);
    
    /* which is the axis of revolution */
    cl = AIR_MIN(0.99, aniso[tenAniso_Cl1]);
    cp = AIR_MIN(0.99, aniso[tenAniso_Cp1]);
    if (cl > cp) {
      axis = 0;
      qA = pow(1-cp, parm->sqdSharp);
      qB = pow(1-cl, parm->sqdSharp);
    } else {
      axis = 2;
      qA = pow(1-cl, parm->sqdSharp);
      qB = pow(1-cp, parm->sqdSharp);
    }
    
    /* add the glyph */
    if (glyphsLimn) {
      lookIdx = limnObjectLookAdd(glyphsLimn);
      look = glyphsLimn->look + lookIdx;
      ELL_4V_SET(look->rgba, R, G, B, 1);
      ELL_3V_SET(look->kads, parm->ADSP[0], parm->ADSP[1], parm->ADSP[2]);
      look->spow = 0;
      switch(parm->glyphType) {
      case tenGlyphTypeBox:
	glyphIdx = limnObjectCubeAdd(glyphsLimn, lookIdx);
	break;
      case tenGlyphTypeSphere:
	glyphIdx = limnObjectPolarSphereAdd(glyphsLimn, lookIdx, axis,
					    2*parm->facetRes, parm->facetRes);
	break;
      case tenGlyphTypeCylinder:
	glyphIdx = limnObjectCylinderAdd(glyphsLimn, lookIdx, axis,
					 parm->facetRes);
	break;
      case tenGlyphTypeSuperquad:
      default:
	glyphIdx = limnObjectPolarSuperquadAdd(glyphsLimn, lookIdx, axis,
					       qA, qB, 2*parm->facetRes,
					       parm->facetRes);
	break;
      }
      limnObjectPartTransform(glyphsLimn, glyphIdx, mA);
    }
    if (glyphsEcho) {
      switch(parm->glyphType) {
      case tenGlyphTypeBox:
	eglyph = echoObjectNew(glyphsEcho, echoTypeCube);
	/* nothing else to set */
	break;
      case tenGlyphTypeSphere:
	eglyph = echoObjectNew(glyphsEcho, echoTypeSphere);
	echoSphereSet(eglyph, 0, 0, 0, 1);
	break;
      case tenGlyphTypeCylinder:
	eglyph = echoObjectNew(glyphsEcho, echoTypeCylinder);
	echoCylinderSet(eglyph, axis);
	break;
      case tenGlyphTypeSuperquad:
      default:
	eglyph = echoObjectNew(glyphsEcho, echoTypeSuperquad);
	echoSuperquadSet(eglyph, axis, qA, qB);
	break;
      }
      echoColorSet(eglyph, R, G, B, 1);
      echoMatterPhongSet(glyphsEcho, eglyph, 
			 parm->ADSP[0], parm->ADSP[1],
			 parm->ADSP[2], parm->ADSP[3]);
      inst = echoObjectNew(glyphsEcho, echoTypeInstance);
      ELL_4M_COPY(eM, mA);
      echoInstanceSet(inst, eM, eglyph);
      echoListAdd(list, inst);
    }
  }
  if (glyphsEcho) {
    split = echoListSplit3(glyphsEcho, list, 10);
    echoObjectAdd(glyphsEcho, split);
  }
  
  airMopOkay(mop);
  return 0;
}
