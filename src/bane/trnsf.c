#include "bane.h"

int
_baneValidHVol(Nrrd *hvol) {
  char me[]="_baneValidHVol", err[128];

  if (3 != hvol->dim) {
    sprintf(err, "%s: need dimension to be 3 (not %d)", me, hvol->dim);
    biffSet(BANE, err); return 0;
  }
  if (nrrdTypeUChar != hvol->type) {
    sprintf(err, "%s: need type to be unsigned char (not %s)", 
	    me, nrrdType2Str[hvol->type]);
    biffSet(BANE, err); return 0;
  }
  if (!( AIR_EXISTS(hvol->axisMin[0]) && AIR_EXISTS(hvol->axisMax[0]) && 
	 AIR_EXISTS(hvol->axisMin[1]) && AIR_EXISTS(hvol->axisMax[1]) && 
	 AIR_EXISTS(hvol->axisMin[2]) && AIR_EXISTS(hvol->axisMax[2]) )) {
    sprintf(err, "%s: axisMin and axisMax must be set for all axes", me);
    biffSet(BANE, err); return 0;
  }
  if (strcmp(hvol->label[0], "gradient-mag_cd")) {
    sprintf(err, "%s: expected \"gradient-mag_cd\" on axis 0", me);
    biffSet(BANE, err); return 0;
  }
  if (strcmp(hvol->label[1], "Laplacian_cd") &&
      strcmp(hvol->label[1], "Hessian-2dd_cd") &&
      strcmp(hvol->label[1], "grad-mag-grad_cd")) {
    sprintf(err, "%s: expected a second derivative measure on axis 1", me);
    biffSet(BANE, err); return 0;    
  }
  if (strcmp(hvol->label[2], "value")) {
    sprintf(err, "%s: expected \"value\" on axis 2", me);
    biffSet(BANE, err); return 0;
  }
  return 1;
}

int
bane2DOpacInfo(Nrrd *info2D, Nrrd *hvol) {
  char me[]="bane2DOpacInfo", err[128];
  Nrrd *proj2, *projT;
  float *data2D;
  int i, sv, sg, axes[2];

  if (!(info2D && hvol)) {
    sprintf(err, BIFF_NULL, me);
    biffSet(BANE, err); return 1;
  }

  if (!_baneValidHVol(hvol)) {
    sprintf(err, "%s: given nrrd doesn't seem to be a histogram volume", me);
    biffAdd(BANE, err); return 1;
  }

  /* hvol axes: 0: grad, 1: 2nd deriv: 2: data value */
  sv = hvol->size[2];
  sg = hvol->size[0];
  if (!info2D->data) {
    if (nrrdAlloc(info2D, 2*sv*sg, nrrdTypeFloat, 3)) {
      sprintf(err, BIFF_NRRDALLOC, me);
      biffMove(BANE, err, NRRD); return 1;
    }
  }
  info2D->size[0] = 2;
  info2D->size[1] = sv;
  info2D->size[2] = sg;
  info2D->axisMin[1] = hvol->axisMin[2];
  info2D->axisMax[1] = hvol->axisMax[2];
  info2D->axisMin[2] = hvol->axisMin[0];
  info2D->axisMax[2] = hvol->axisMax[0];
  data2D = info2D->data;
  
  axes[0] = 1;
  axes[1] = 0;

  /* first create h(v,g) */
  proj2 = nrrdNewMeasureAxis(hvol, 1, nrrdMeasrHistoMean);
  if (!(projT = nrrdNewPermuteAxes(proj2, axes))) {
    sprintf(err, "%s: trouble projecting to create h(v,g)", me);
    biffMove(BANE, err, NRRD); return 1;
  }
  for (i=0; i<=sv*sg-1; i++) {
    data2D[0 + 2*i] = nrrdFLookup[projT->type](projT->data, i);
  }
  nrrdNuke(proj2);
  nrrdNuke(projT);

  /* then create #hits(v,g) */
  proj2 = nrrdNewMeasureAxis(hvol, 1, nrrdMeasrSum);
  if (!(projT = nrrdNewPermuteAxes(proj2, axes))) {
    sprintf(err, "%s: trouble projecting to create #(v,g)", me);
    biffMove(BANE, err, NRRD); return 1;
  }
  for (i=0; i<=sv*sg-1; i++) {
    data2D[1 + 2*i] = nrrdFLookup[projT->type](projT->data, i);
  }
  nrrdNuke(proj2);
  nrrdNuke(projT);
  
  return 0;
}

Nrrd *
baneNew2DOpacInfo(Nrrd *hvol) {
  char me[]="baneNew2DOpacInfo", err[128];
  Nrrd *info2D;

  if (!(info2D = nrrdNew())) {
    sprintf(err, BIFF_NRRDNEW, me);
    biffSet(BANE, err); return NULL;
  }

  if (bane2DOpacInfo(info2D, hvol)) {
    sprintf(err, "%s: trouble creating 2D opacity function info", me);
    biffAdd(BANE, err); return NULL;
  }
  
  return info2D;
}

int
bane1DOpacInfo(Nrrd *info1D, Nrrd *hvol) {
  char me[]="bane1DOpacInfo", err[128];
  Nrrd *proj2, *proj1;
  float *data1D;
  int i, len;

  if (!(info1D && hvol)) {
    sprintf(err, BIFF_NULL, me);
    biffSet(BANE, err); return 1;
  }

  if (!_baneValidHVol(hvol)) {
    sprintf(err, "%s: given nrrd doesn't seem to be a histogram volume", me);
    biffAdd(BANE, err); return 1;
  }

  /* hvol axes: 0: grad, 1: 2nd deriv, 2: value */
  len = hvol->size[2];
  if (!info1D->data) {
    if (nrrdAlloc(info1D, 2*len, nrrdTypeFloat, 2)) {
      sprintf(err, BIFF_NRRDALLOC, me);
      biffMove(BANE, err, NRRD); return 1;
    }
  }

  info1D->size[0] = 2;
  info1D->size[1] = len;
  info1D->axisMin[1] = hvol->axisMin[2];
  info1D->axisMax[1] = hvol->axisMax[2];
  data1D = info1D->data;

  /* sum up along 2nd deriv for each data value, grad mag */
  if (!(proj2 = nrrdNewMeasureAxis(hvol, 1, nrrdMeasrSum))) {
    sprintf(err, "%s: trouble projecting out 2nd deriv. for g(v)", me);
    biffMove(BANE, err, NRRD); return 1;
  }
  /* now determine average gradient at each value (0: grad, 1: value) */
  if (!(proj1 = nrrdNewMeasureAxis(proj2, 0, nrrdMeasrHistoMean))) {
    sprintf(err, "%s: trouble projecting along gradient for g(v)", me);
    biffMove(BANE, err, NRRD); return 1;
  }
  for (i=0; i<=len-1; i++) {
    data1D[0 + 2*i] = nrrdFLookup[proj1->type](proj1->data, i);
  }
  nrrdNuke(proj1);
  nrrdNuke(proj2);

  /* sum up along gradient for each data value, 2nd deriv */
  if (!(proj2 = nrrdNewMeasureAxis(hvol, 0, nrrdMeasrSum))) {
    sprintf(err, "%s: trouble projecting out gradient for h(v)", me);
    biffMove(BANE, err, NRRD); return 1;
  }
  /* now determine average gradient at each value (0: 2nd deriv, 1: value) */
  if (!(proj1 = nrrdNewMeasureAxis(proj2, 0, nrrdMeasrHistoMean))) {
    sprintf(err, "%s: trouble projecting along 2nd deriv. for h(v)", me);
    biffMove(BANE, err, NRRD); return 1;
  }
  for (i=0; i<=len-1; i++) {
    data1D[1 + 2*i] = nrrdFLookup[proj1->type](proj1->data, i);
  }
  nrrdNuke(proj1);
  nrrdNuke(proj2);
  
  return 0;
}

Nrrd *
baneNew1DOpacInfo(Nrrd *hvol) {
  char me[]="baneNew1DOpacInfo", err[128];
  Nrrd *info1D;

  if (!(info1D = nrrdNew())) {
    sprintf(err, BIFF_NRRDNEW, me);
    biffSet(BANE, err); return NULL;
  }
  if (bane1DOpacInfo(info1D, hvol)) {
    sprintf(err, "%s: trouble creating 1D opacity function information", me);
    biffSet(BANE, err); return NULL;
  }
  return info1D;
}

int
_baneValidInfo2D(Nrrd *info2D) {
  char me[]="_baneValidInfo2D", err[128];

  if (3 != info2D->dim) {
    sprintf(err, "%s: need 3 dimensions, not %d", me, info2D->dim);
    biffSet(BANE, err); return 0;
  }
  if (nrrdTypeFloat != info2D->type) {
    sprintf(err, "%s: need data of type float", me);
    biffSet(BANE, err); return 0;
  }
  if (2 != info2D->size[0]) {
    sprintf(err, "%s: 1st axis needs size 2 (not %d)", me, info2D->size[0]);
    biffSet(BANE, err); return 0;
  }
  return 1;
}

int
_baneValidInfo1D(Nrrd *info1D) {
  char me[]="_baneValidInfo1D", err[128];
  
  if (2 != info1D->dim) {
    sprintf(err, "%s: need 2 dimensions, not %d", me, info1D->dim);
    biffSet(BANE, err); return 0;
  }
  if (nrrdTypeFloat != info1D->type) {
    sprintf(err, "%s: need data of type float", me);
    biffSet(BANE, err); return 0;
  }
  if (2 != info1D->size[0]) {
    sprintf(err, "%s: 1st axis needs size 2 (not %d)", me, info1D->size[0]);
    biffSet(BANE, err); return 0;
  }
  return 1;
}

int
bane1DOpacInfoFrom2D(Nrrd *info1D, Nrrd *info2D) {
  char me[]="bane1DOpacInfoFrom2D", err[128];
  Nrrd *projH2, *projH1, *projN, *projG1;
  float *data1D;
  int i, len;
  
  if (!(info1D && info2D)) {
    sprintf(err, BIFF_NULL, me); biffSet(BANE, err); return 1;
  }
  if (!_baneValidInfo2D(info2D)) {
    sprintf(err, "%s: didn't get valid 2D info", me);
    biffAdd(BANE, err); return 1;
  }
  
  len = info2D->size[1];
  projH2 = nrrdNewMeasureAxis(info2D, 0, nrrdMeasrProduct);
  /* nrrdWrite(fopen("projH2","w"), projH2); */
  projH1 = nrrdNewMeasureAxis(projH2, 1, nrrdMeasrSum);
  /* nrrdWrite(fopen("projH1","w"), projH1); */
  projN = nrrdNewMeasureAxis(info2D, 2, nrrdMeasrSum);
  /* nrrdWrite(fopen("projN","w"), projN); */
  projG1 = nrrdNewMeasureAxis(info2D, 2, nrrdMeasrHistoMean);
  /* nrrdWrite(fopen("projG1","w"), projG1); */
  if (!(projH2 && projH1 && projN && projG1)) {
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

Nrrd *
baneNew1DOpacInfoFrom2D(Nrrd *info2D) {
  char me[]="baneNew1DOpacInfoFrom2D", err[128];
  Nrrd *info1D;

  if (!(info1D = nrrdNew())) {
    sprintf(err, BIFF_NRRDNEW, me); biffSet(BANE, err); return NULL;
  }
  if (bane1DOpacInfoFrom2D(info1D, info2D)) {
    sprintf(err, "%s: trouble deriving 1D info from 2D info", me);
    biffAdd(BANE, err); return NULL;
  }
  return info1D;
}

int
baneSigmaCalc1D(float *sP, Nrrd *info1D) {
  char me[]="baneSigmaCalc", err[128];
  int i, len;
  float maxg, maxh, minh, *data;
  
  if (!(sP && info1D)) { 
    sprintf(err, BIFF_NULL, me); biffSet(BANE, err); return 1;
  }
  if (!_baneValidInfo1D(info1D)) {
    sprintf(err, "%s: didn't get a valid 1D info", me);
    biffAdd(BANE, err); return 1;
  }

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
baneSigmaCalc2D(float *sP, Nrrd *info2D) {
  char me[]="baneSigmaCalc2D", err[128];
  Nrrd *info1D;

  if (!(info1D = baneNew1DOpacInfoFrom2D(info2D))) {
    sprintf(err, "%s: couldn't create 1D opac info from 2d", me);
    biffAdd(BANE, err); return 1;
  }
  if (baneSigmaCalc1D(sP, info1D)) {
    sprintf(err, "%s: trouble calculating sigma", me);
    biffAdd(BANE, err); return 1;
  }
  nrrdNuke(info1D);
  return 0;
}

int
banePosCalc1D(Nrrd *pos1D, float sigma, float gthresh, Nrrd *info1D) {
  char me[]="banePosCalc1D", err[128];
  int i, len;
  float *posData, *infoData, h, g, p;

  if (!(pos1D && info1D)) {
    sprintf(err, BIFF_NULL, me); biffSet(BANE, err); return 1;
  }
  if (!_baneValidInfo1D(info1D)) {
    sprintf(err, "%s: didn't get a valid 1D info", me);
    biffAdd(BANE, err); return 1;
  }
  len = info1D->size[1];
  if (!pos1D->data) {
    if (nrrdAlloc(pos1D, len, nrrdTypeFloat, 1)) {
      sprintf(err, BIFF_NRRDALLOC, me); 
      biffMove(BANE, err, NRRD); return 1;
    }
  }
  pos1D->size[0] = len;
  pos1D->axisMin[0] = info1D->axisMin[1];
  pos1D->axisMax[0] = info1D->axisMax[1];
  posData = pos1D->data;
  infoData = info1D->data;
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
  return 0;
}

Nrrd *
baneNewPosCalc1D(float sigma, float gthresh, Nrrd *info1D) {
  char me[]="baneNewPosCalc1D", err[128];
  Nrrd *pos1D;

  if (!(pos1D = nrrdNew())) {
    sprintf(err, BIFF_NULL, me); biffSet(BANE, err); return NULL;
  }
  if (banePosCalc1D(pos1D, sigma, gthresh, info1D)) {
    sprintf(err, "%s: trouble calculating position", me);
    biffAdd(BANE, err); return NULL;
  }
  return pos1D;
}

int
banePosCalc2D(Nrrd *pos2D, float sigma, float gthresh, Nrrd *info2D) {
  char me[]="banePosCalc2D", err[128];
  int vi, gi, sv, sg;
  float *posData, g, h, p;

  if (!(pos2D && info2D)) {
    sprintf(err, BIFF_NULL, me); biffSet(BANE, err); return 1;
  }
  if (!_baneValidInfo2D(info2D)) {
    sprintf(err, "%s: didn't get valid 2D info", me);
    biffAdd(BANE, err); return 1;
  }
  sv = info2D->size[1];
  sg = info2D->size[2];
  if (!pos2D->data) {
    if (nrrdAlloc(pos2D, sv*sg, nrrdTypeFloat, 2)) {
      sprintf(err, BIFF_NRRDALLOC, me); biffMove(BANE, err, NRRD); return 1;
    }
  }
  pos2D->size[0] = sv;
  pos2D->size[1] = sg;
  pos2D->axisMin[0] = info2D->axisMin[1];
  pos2D->axisMax[0] = info2D->axisMax[1];
  pos2D->axisMin[1] = info2D->axisMin[2];
  pos2D->axisMax[1] = info2D->axisMax[2];
  printf("%s: g range: %g - %g\n", me, 
	 info2D->axisMin[2], info2D->axisMax[2]);
  posData = pos2D->data;
  for (gi=0; gi<=sg-1; gi++) {
    g = AIR_AFFINE(0, gi, sg-1, info2D->axisMin[2], info2D->axisMax[2]);
    /* printf("gi=%d --> g = %g\n", gi, g); */
    for (vi=0; vi<=sv-1; vi++) {
      h = nrrdFLookup[info2D->type](info2D->data, 0 + 2*(vi + sv*gi));
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
  return 0;
}

Nrrd *
baneNewPosCalc2D(float sigma, float gthresh, Nrrd *info2D) {
  char me[]="baneNewPosCalc2D", err[128];
  Nrrd *pos2D;

  if (!(pos2D = nrrdNew())) {
    sprintf(err, BIFF_NULL, me); biffSet(BANE, err); return NULL;
  }
  if (banePosCalc2D(pos2D, sigma, gthresh, info2D)) {
    sprintf(err, "%s: trouble calculating position", me);
    biffAdd(BANE, err); return NULL;
  }
  return pos2D;
}

int
_baneValidPos1D(Nrrd *pos1D) {
  char me[]="_baneValidPos1D", err[128];

  if (1 != pos1D->dim) {
    sprintf(err, "%s: need 1-dimensional (not %d)", me, pos1D->dim);
    biffSet(BANE, err); return 0;
  }
  if (nrrdTypeFloat != pos1D->type) {
    sprintf(err, "%s: need data of type float", me);
    biffSet(BANE, err); return 0;
  }
  /* HEY? check for values in axisMin[0] and axisMax[0] ? */
  return 1;
}

int
_baneValidPos2D(Nrrd *pos2D) {
  char me[]="_baneValidPos2D", err[128];

  if (2 != pos2D->dim) {
    sprintf(err, "%s: need 2-dimensional (not %d)", me, pos2D->dim);
    biffSet(BANE, err); return 0;
  }
  if (nrrdTypeFloat != pos2D->type) {
    sprintf(err, "%s: need data of type float", me);
    biffSet(BANE, err); return 0;
  }
  /* HEY? check for values in axisMin[0] and axisMax[0] ? */
  /* HEY? check for values in axisMin[1] and axisMax[1] ? */
  return 1;
}

int
_baneValidBcpts(Nrrd *Bcpts) {
  char me[]="_baneValidBcpts", err[128];
  int i, len;
  float *data;

  if (2 != Bcpts->dim) {
    sprintf(err, "%s: need 2-dimensional (not %d)", me, Bcpts->dim);
    biffSet(BANE, err); return 0;
  }
  if (2 != Bcpts->size[0]) {
    sprintf(err, "%s: axis#0 needs size 2 (not %d)", me, Bcpts->size[0]);
    biffSet(BANE, err); return 0;
  }
  if (nrrdTypeFloat != Bcpts->type) {
    sprintf(err, "%s: need data of type float", me);
    biffSet(BANE, err); return 0;
  }
  len = Bcpts->size[1];
  data = Bcpts->data;
  for (i=0; i<=len-2; i++) {
    if (!(data[0 + 2*i] <= data[0 + 2*(i+1)])) {
      sprintf(err, "%s: value coord %d (%g) not <= coord %d (%g)", me,
	      i, data[0 + 2*i], i+1, data[0 + 2*(i+1)]);
      biffSet(BANE, err); return 0;
    }
  }
  return 1;
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
  char me[]="_baneOpacCalcB";
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
baneOpacCalc1Dcpts(Nrrd *opac, Nrrd *Bcpts, Nrrd *pos1D) {
  char me[]="baneOpacCalc1Dcpts", err[128];
  int len, npts;
  float *bdata, *odata, *pdata;

  if (!(opac && Bcpts && pos1D)) {
    sprintf(err, BIFF_NULL, me); biffSet(BANE, err); return 1;
  }
  if (!_baneValidBcpts(Bcpts)) {
    sprintf(err, "%s: didn't get valid control points for b(x)", me);
    biffAdd(BANE, err); return 1;
  }
  if (!_baneValidPos1D(pos1D)) {
    sprintf(err, "%s: didn't get valid p(v)", me);
    biffAdd(BANE, err); return 1;
  }
  len = pos1D->size[0];
  if (!opac->data) {
    if (nrrdAlloc(opac, len, nrrdTypeFloat, 1)) {
      sprintf(err, BIFF_NRRDALLOC, me); biffMove(BANE, err, NRRD); return 1;
    }
  }
  opac->size[0] = len;
  opac->axisMin[0] = pos1D->axisMin[0];
  opac->axisMax[0] = pos1D->axisMax[0];
  odata = opac->data;
  bdata = Bcpts->data;
  pdata = pos1D->data;
  npts = Bcpts->size[1];
  _baneOpacCalcA(len, odata, npts, bdata, pdata);
  return 0;
}

Nrrd *
baneNewOpacCalc1Dcpts(Nrrd *Bcpts, Nrrd *pos1D) {
  char me[]="baneNewOpacCalc1Dcpts", err[128];
  Nrrd *opac;

  opac = nrrdNew();
  if (!opac) {
    sprintf(err, BIFF_NRRDNEW, me); biffSet(BANE, err); return NULL;
  }
  if (baneOpacCalc1Dcpts(opac, Bcpts, pos1D)) {
    sprintf(err, "%s: trouble calculating opacity", me);
    biffAdd(BANE, err); nrrdNuke(opac); return NULL;
  }
  return opac;
}

int
baneOpacCalc2Dcpts(Nrrd *opac, Nrrd *Bcpts, Nrrd *pos2D) {
  char me[]="baneOpacCalc2Dcpts", err[128];
  int sv, sg, npts;
  float *bdata, *odata, *pdata;

  if (!(opac && Bcpts && pos2D)) {
    sprintf(err, BIFF_NULL, me); biffSet(BANE, err); return 1;
  }
  if (!_baneValidBcpts(Bcpts)) {
    sprintf(err, "%s: didn't get valid control points for b(x)", me);
    biffAdd(BANE, err); return 1;
  }
  if (!_baneValidPos2D(pos2D)) {
    sprintf(err, "%s: didn't get valid p(v,g)", me);
    biffAdd(BANE, err); return 1;
  }
  sv = pos2D->size[0];
  sg = pos2D->size[1];
  if (!opac->data) {
    if (nrrdAlloc(opac, sv*sg, nrrdTypeFloat, 2)) {
      sprintf(err, BIFF_NRRDALLOC, me); biffMove(BANE, err, NRRD); return 1;
    }
  }
  opac->size[0] = sv;
  opac->size[1] = sg;
  opac->axisMin[0] = pos2D->axisMin[0];
  opac->axisMax[0] = pos2D->axisMax[0];
  opac->axisMin[1] = pos2D->axisMin[1];
  opac->axisMax[1] = pos2D->axisMax[1];
  odata = opac->data;
  bdata = Bcpts->data;
  pdata = pos2D->data;
  npts = Bcpts->size[1];
  _baneOpacCalcA(sv*sg, odata, npts, bdata, pdata);
  return 0;
}

Nrrd *
baneNewOpacCalc2Dcpts(Nrrd *Bcpts, Nrrd *pos2D) {
  char me[]="baneNewOpacCalc2Dcpts", err[128];
  Nrrd *opac;

  opac = nrrdNew();
  if (!opac) {
    sprintf(err, BIFF_NRRDNEW, me); biffSet(BANE, err); return NULL;
  }
  if (baneOpacCalc2Dcpts(opac, Bcpts, pos2D)) {
    sprintf(err, "%s: trouble calculating opacity", me);
    biffAdd(BANE, err); nrrdNuke(opac); return NULL;
  }
  return opac;
}

