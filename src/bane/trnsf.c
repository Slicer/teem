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


#include "bane.h"

int
baneOpacInfo(Nrrd *info, Nrrd *hvol, int dim) {
  char me[]="baneOpacInfo", err[128];
  Nrrd *proj2, *proj1, *projT;
  float *data2D, *data1D;
  int i, len, sv, sg;

  if (!(info && hvol)) {
    sprintf(err, BIFF_NULL, me); biffSet(BANE, err); return 1;
  }
  if (!(1 == dim || 2 == dim)) {
    sprintf(err, "%s: got dimension %d, not 1 or 2", me, dim);
    biffSet(BANE, err); return 1;
  }
  if (!baneValidHVol(hvol)) {
    sprintf(err, "%s: given nrrd doesn't seem to be a histogram volume", me);
    biffAdd(BANE, err); return 1;
  }
  if (1 == dim) {
    len = hvol->size[2];
    if (!info->data) {
      if (nrrdAlloc(info, 2*len, nrrdTypeFloat, 2)) {
	sprintf(err, BIFF_NRRDALLOC, me);
	biffMove(BANE, err, NRRD); return 1;
      }
    }

    info->size[0] = 2;
    info->size[1] = len;
    info->axisMin[1] = hvol->axisMin[2];
    info->axisMax[1] = hvol->axisMax[2];
    data1D = info->data;

    /* sum up along 2nd deriv for each data value, grad mag */
    if (nrrdMeasureAxis(proj2 = nrrdNew(), hvol, 1, nrrdMeasrSum)) {
      sprintf(err, "%s: trouble projecting out 2nd deriv. for g(v)", me);
      biffMove(BANE, err, NRRD); return 1;
    }
    /* now determine average gradient at each value (0: grad, 1: value) */
    if (nrrdMeasureAxis(proj1 = nrrdNew(), proj2, 0, nrrdMeasrHistoMean)) {
      sprintf(err, "%s: trouble projecting along gradient for g(v)", me);
      biffMove(BANE, err, NRRD); return 1;
    }
    for (i=0; i<=len-1; i++) {
      data1D[0 + 2*i] = nrrdFLookup[proj1->type](proj1->data, i);
    }
    nrrdNuke(proj1);
    nrrdNuke(proj2);

    /* sum up along gradient for each data value, 2nd deriv */
    if (nrrdMeasureAxis(proj2 = nrrdNew(), hvol, 0, nrrdMeasrSum)) {
      sprintf(err, "%s: trouble projecting out gradient for h(v)", me);
      biffMove(BANE, err, NRRD); return 1;
    }
    /* now determine average gradient at each value (0: 2nd deriv, 1: value) */
    if (nrrdMeasureAxis(proj1 = nrrdNew(), proj2, 0, nrrdMeasrHistoMean)) {
      sprintf(err, "%s: trouble projecting along 2nd deriv. for h(v)", me);
      biffMove(BANE, err, NRRD); return 1;
    }
    for (i=0; i<=len-1; i++) {
      data1D[1 + 2*i] = nrrdFLookup[proj1->type](proj1->data, i);
    }
    nrrdNuke(proj1);
    nrrdNuke(proj2);
  }
  else {
    /* 2 == dim */
    /* hvol axes: 0: grad, 1: 2nd deriv: 2: data value */
    sv = hvol->size[2];
    sg = hvol->size[0];
    if (!info->data) {
      if (nrrdAlloc(info, 2*sv*sg, nrrdTypeFloat, 3)) {
	sprintf(err, BIFF_NRRDALLOC, me);
	biffMove(BANE, err, NRRD); return 1;
      }
    }
    info->size[0] = 2;
    info->size[1] = sv;
    info->size[2] = sg;
    info->axisMin[1] = hvol->axisMin[2];
    info->axisMax[1] = hvol->axisMax[2];
    info->axisMin[2] = hvol->axisMin[0];
    info->axisMax[2] = hvol->axisMax[0];
    data2D = info->data;

    /* first create h(v,g) */
    if (nrrdMeasureAxis(proj2 = nrrdNew(), hvol, 1, nrrdMeasrHistoMean)) {
      sprintf(err, "%s: trouble projecting (step 1) to create h(v,g)", me);
      biffMove(BANE, err, NRRD); return 1;
    }
    if (nrrdSwapAxes(projT = nrrdNew(), proj2, 0, 1)) {
      sprintf(err, "%s: trouble projecting (step 2) to create h(v,g)", me);
      biffMove(BANE, err, NRRD); return 1;
    }
    for (i=0; i<=sv*sg-1; i++) {
      data2D[0 + 2*i] = nrrdFLookup[projT->type](projT->data, i);
    }
    nrrdNuke(proj2);
    nrrdNuke(projT);

    /* then create #hits(v,g) */
    if (nrrdMeasureAxis(proj2 = nrrdNew(), hvol, 1, nrrdMeasrSum)) {
      sprintf(err, "%s: trouble projecting (step 1) to create #(v,g)", me);
      biffMove(BANE, err, NRRD); return 1;
    }
    if (nrrdSwapAxes(projT = nrrdNew(), proj2, 0, 1)) {
      sprintf(err, "%s: trouble projecting (step 2) to create #(v,g)", me);
      biffMove(BANE, err, NRRD); return 1;
    }
    for (i=0; i<=sv*sg-1; i++) {
      data2D[1 + 2*i] = nrrdFLookup[projT->type](projT->data, i);
    }
    nrrdNuke(proj2);
    nrrdNuke(projT);
  }
  return 0;
}

int
bane1DOpacInfoFrom2D(Nrrd *info1D, Nrrd *info2D) {
  char me[]="bane1DOpacInfoFrom2D", err[128];
  Nrrd *projH2, *projH1, *projN, *projG1;
  float *data1D;
  int E, i, len;
  
  if (!(info1D && info2D)) {
    sprintf(err, BIFF_NULL, me); biffSet(BANE, err); return 1;
  }
  if (!baneValidInfo(info2D, 2)) {
    sprintf(err, "%s: didn't get valid 2D info", me);
    biffAdd(BANE, err); return 1;
  }
  
  len = info2D->size[1];
  E = 0;
  if (!E) E |= nrrdMeasureAxis(projH2=nrrdNew(), info2D, 0, nrrdMeasrProduct);
  if (!E) E |= nrrdMeasureAxis(projH1=nrrdNew(), projH2, 1, nrrdMeasrSum);
  if (!E) E |= nrrdMeasureAxis(projN=nrrdNew(), info2D, 2, nrrdMeasrSum);
  if (!E) E |= nrrdMeasureAxis(projG1=nrrdNew(), info2D, 2,nrrdMeasrHistoMean);
  if (E) {
    sprintf(err, "%s: trouble creating need projections", me);
    biffAdd(BANE, err); return 1;
  }
  
  if (!info1D->data) {
    if (nrrdAlloc(info1D, 2*len, nrrdTypeFloat, 2)) {
      sprintf(err, BIFF_NRRDALLOC, me);
      biffMove(BANE, err, NRRD); return 1;
    }
  }
  info1D->size[0] = 2;
  info1D->size[1] = len;
  info1D->axisMin[1] = info2D->axisMin[1];
  info1D->axisMax[1] = info2D->axisMax[1];
  data1D = info1D->data;

  for (i=0; i<=len-1; i++) {
    data1D[0 + 2*i] = nrrdFLookup[projG1->type](projG1->data, 1 + 2*i);
    data1D[1 + 2*i] = (nrrdFLookup[projH1->type](projH1->data, i) / 
		       nrrdFLookup[projN->type](projN->data, 1 + 2*i));
  }
  nrrdNuke(projH2);
  nrrdNuke(projH1);
  nrrdNuke(projN);
  nrrdNuke(projG1);
  return 0;
}

int
_baneSigmaCalc1D(float *sP, Nrrd *info1D) {
  char me[]="_baneSigmaCalc1D", err[128];
  int i, len;
  float maxg, maxh, minh, *data;
  
  len = info1D->size[1];
  data = info1D->data;
  maxg = -1;
  maxh = -1;
  minh = 1;
  for (i=0; i<=len-1; i++) {
    if (AIR_EXISTS(data[0 + 2*i]))
      maxg = AIR_MAX(maxg, data[0 + 2*i]);
    if (AIR_EXISTS(data[1 + 2*i])) {
      minh = AIR_MIN(minh, data[1 + 2*i]);
      maxh = AIR_MAX(maxh, data[1 + 2*i]);
    }
  }
  if (maxg == -1 || maxh == -1) {
    sprintf(err, "%s: saw only NaNs in 1D info!", me);
    biffSet(BANE, err); return 1;
  }

  /* here's the actual calculation: from page 54 of GK's MS */
  *sP = 2*sqrt(M_E)*maxg/(maxh - minh);

  return 0;
}

int
baneSigmaCalc(float *sP, Nrrd *_info) {
  char me[]="baneSigmaCalc", err[128];
  Nrrd *info;

  if (!(sP && _info)) { 
    sprintf(err, BIFF_NULL, me); biffSet(BANE, err); return 1;
  }
  if (!baneValidInfo(_info, 0)) {
    sprintf(err, "%s: didn't get a valid info", me);
    biffAdd(BANE, err); return 1;
  }
  if (3 == _info->dim) {
    if (bane1DOpacInfoFrom2D(info = nrrdNew(), _info)) {
      sprintf(err, "%s: couldn't create 1D opac info from 2D", me);
      biffAdd(BANE, err); return 1;
    }
  }
  else {
    info = _info;
  }
  if (_baneSigmaCalc1D(sP, info)) {
    sprintf(err, "%s: trouble calculating sigma", me);
    biffAdd(BANE, err); return 1;
  }
  if (_info != info) {
    nrrdNuke(info);
  }
  return 0;
}

int
banePosCalc(Nrrd *pos, float sigma, float gthresh, Nrrd *info) {
  char me[]="banePosCalc", err[128];
  int d, i, len, vi, gi, sv, sg;
  float *posData, *infoData, h, g, p;

  if (!(pos && info)) {
    sprintf(err, BIFF_NULL, me); biffSet(BANE, err); return 1;
  }
  if (!baneValidInfo(info, 0)) {
    sprintf(err, "%s: didn't get a valid info", me);
    biffAdd(BANE, err); return 1;
  }
  d = info->dim-1;
  if (1 == d) {
    len = info->size[1];
    if (!pos->data) {
      if (nrrdAlloc(pos, len, nrrdTypeFloat, 1)) {
	sprintf(err, BIFF_NRRDALLOC, me); 
	biffMove(BANE, err, NRRD); return 1;
      }
    }
    pos->size[0] = len;
    pos->axisMin[0] = info->axisMin[1];
    pos->axisMax[0] = info->axisMax[1];
    posData = pos->data;
    infoData = info->data;
    for (i=0; i<=len-1; i++) {
      /* from pg. 55 of GK's MS */
      g = infoData[0+2*i];
      h = infoData[1+2*i];
      if (AIR_EXISTS(g) && AIR_EXISTS(h))
	p = -sigma*sigma*h/AIR_MAX(0, g-gthresh);
      else
	p = airNanf();
      p = airIsInff(p) ? 10000 : p;
      posData[i] = p;
    }
  }
  else {
    sv = info->size[1];
    sg = info->size[2];
    if (!pos->data) {
      if (nrrdAlloc(pos, sv*sg, nrrdTypeFloat, 2)) {
	sprintf(err, BIFF_NRRDALLOC, me); biffMove(BANE, err, NRRD); return 1;
      }
    }
    pos->size[0] = sv;
    pos->size[1] = sg;
    pos->axisMin[0] = info->axisMin[1];
    pos->axisMax[0] = info->axisMax[1];
    pos->axisMin[1] = info->axisMin[2];
    pos->axisMax[1] = info->axisMax[2];
    posData = pos->data;
    for (gi=0; gi<=sg-1; gi++) {
      g = AIR_AFFINE(0, gi, sg-1, info->axisMin[2], info->axisMax[2]);
      for (vi=0; vi<=sv-1; vi++) {
	h = nrrdFLookup[info->type](info->data, 0 + 2*(vi + sv*gi));
	/* from pg. 61 of GK's MS */
	if (AIR_EXISTS(h)) {
	  p = -sigma*sigma*h/AIR_MAX(0, g-gthresh);
	}
	else {
	  p = airNanf();
	}
	p = airIsInff(p) ? airNanf() : p;
	posData[vi + sv*gi] = p;
      }
    }
  }
  return 0;
}

void
_baneOpacCalcA(int lutLen, float *opacLut, 
	       int numCpts, float *xo,
	       float *pos) {
  int i, j;
  float p;

  for (i=0; i<=lutLen-1; i++) {
    p = pos[i];
    if (!AIR_EXISTS(p)) {
      opacLut[i] = 0;
      continue;
    }
    if (p <= xo[0 + 2*0]) {
      opacLut[i] = xo[1 + 2*0];
      continue;
    }
    if (p >= xo[0 + 2*(numCpts-1)]) {
      opacLut[i] = xo[1 + 2*(numCpts-1)];
      continue;
    }
    for (j=1; j<=numCpts-1; j++)
      if (p < xo[0 + 2*j])
	break;
    opacLut[i] = AIR_AFFINE(xo[0 + 2*(j-1)], p, xo[0 + 2*j], 
			    xo[1 + 2*(j-1)], xo[1 + 2*j]);
  }
  /*
  for (i=0; i<=numCpts-1; i++)
    printf("b(%g) = %g\n", xo[0+2*i], xo[1+2*i]);
  for (i=0; i<=lutLen-1; i++)
    printf("p[%d] = %g -> o = %g\n", i, pos[i], opacLut[i]);
  */
}

void
_baneOpacCalcB(int lutLen, float *opacLut, 
	       int numCpts, float *x, float *o,
	       float *pos) {
  /* char me[]="_baneOpacCalcB"; */
  int i, j;
  float p, op;

  /*
  printf("%s(%d, %lu, %d, %lu, %lu, %lu): hello\n", me, lutLen,
	 (unsigned long)opacLut, numCpts, 
	 (unsigned long)x, (unsigned long)o, 
	 (unsigned long)pos);
  */
  /*
  for (i=0; i<=numCpts-1; i++) {
    printf("%s: opac(%g) = %g\n", me, x[i], o[i]);
  }
  printf("----------\n");
  */
  for (i=0; i<=lutLen-1; i++) {
    p = pos[i];
    /*
    printf("%s: pos[%d] = %g -->", me, i, p); fflush(stdout);
    */
    if (!AIR_EXISTS(p)) {
      op = 0;
      goto endloop;
    }
    if (p <= x[0]) {
      op = o[0];
      goto endloop;
    }
    if (p >= x[numCpts-1]) {
      op = o[numCpts-1];
      goto endloop;
    }
    for (j=1; j<=numCpts-1; j++)
      if (p < x[j])
	break;
    op = AIR_AFFINE(x[j-1], p, x[j], o[j-1], o[j]);
  endloop:
    opacLut[i] = op;
    /* 
    printf("opac[%d] = %g\n", i, op);
    */
  }
  /*
  printf("^^^^^^^^^\n");
  */
}

int
baneOpacCalc(Nrrd *opac, Nrrd *Bcpts, Nrrd *pos) {
  char me[]="baneOpacCalc", err[128];
  int dim, sv, sg, len, npts;
  float *bdata, *odata, *pdata;

  if (!(opac && Bcpts && pos)) {
    sprintf(err, BIFF_NULL, me); biffSet(BANE, err); return 1;
  }
  if (!baneValidBcpts(Bcpts)) {
    sprintf(err, "%s: didn't get valid control points for b(x)", me);
    biffAdd(BANE, err); return 1;
  }
  if (!baneValidPos(pos, 0)) {
    sprintf(err, "%s: didn't get valid position data", me);
    biffAdd(BANE, err); return 1;
  }
  dim = pos->dim;
  if (1 == dim) {
    len = pos->size[0];
    if (!opac->data) {
      if (nrrdAlloc(opac, len, nrrdTypeFloat, 1)) {
	sprintf(err, BIFF_NRRDALLOC, me); biffMove(BANE, err, NRRD); return 1;
      }
    }
    opac->size[0] = len;
    opac->axisMin[0] = pos->axisMin[0];
    opac->axisMax[0] = pos->axisMax[0];
    odata = opac->data;
    bdata = Bcpts->data;
    pdata = pos->data;
    npts = Bcpts->size[1];
    _baneOpacCalcA(len, odata, npts, bdata, pdata);
  }
  else {
    sv = pos->size[0];
    sg = pos->size[1];
    if (!opac->data) {
      if (nrrdAlloc(opac, sv*sg, nrrdTypeFloat, 2)) {
	sprintf(err, BIFF_NRRDALLOC, me); biffMove(BANE, err, NRRD); return 1;
      }
    }
    opac->size[0] = sv;
    opac->size[1] = sg;
    opac->axisMin[0] = pos->axisMin[0];
    opac->axisMax[0] = pos->axisMax[0];
    opac->axisMin[1] = pos->axisMin[1];
    opac->axisMax[1] = pos->axisMax[1];
    odata = opac->data;
    bdata = Bcpts->data;
    pdata = pos->data;
    npts = Bcpts->size[1];
    _baneOpacCalcA(sv*sg, odata, npts, bdata, pdata);
  }
  return 0;
}
