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


#include <air.h>
#include <hest.h>
#include <nrrd.h>
#include <gage.h>
#include <bane.h>

#define QBERT "qbert"
#define QBERT_HIST_BINS 1024     /* histogram size for v, g, and h */

char info[]="Generates volume datasets friendly to hardware-based "
"volume renderers. "
"Each voxel in the output volume contains 8-bit quantized values of the data "
"value, gradient magnitude, and 2nd directional derivative.";

int qbertSaveAll = AIR_TRUE;  /* can be used to save output of every stage */

/*
** This padding/resampling is to get axis[i]'s size >= sz[i], which
** is only needed if the input volume is smaller along any of the axes
** than the desired output volume.
*/
int
qbertSizeUp(Nrrd *nout, Nrrd *nin, int *sz,
	    NrrdKernelSpec *uk) {
  char me[]="qbertSizeUp", err[AIR_STRLEN_MED];
  int i, anyneed, need, padMin[3], padMax[3];
  NrrdResampleInfo *rsi;
  airArray *mop;

  mop = airMopNew();
  rsi=nrrdResampleInfoNew();
  airMopAdd(mop, rsi, (airMopper)nrrdResampleInfoNix, airMopAlways);
  anyneed = 0;
  if (uk) {
    for (i=0; i<=2; i++) {
      anyneed |= need = sz[i] - nin->axis[i].size;
      fprintf(stderr, "%s: sz[%d] = %d -> need = %d --> ", 
	      me, i, nin->axis[i].size, need);
      need = AIR_MAX(0, need);
      fprintf(stderr, "%d --> %s resample\n", need, need ? "WILL" : "won't");
      if (need) {
	rsi->kernel[i] = uk->kernel;
	memcpy(rsi->parm[i], uk->parm, uk->kernel->numParm*sizeof(double));
	if (!AIR_EXISTS(nin->axis[i].min)) {
	  nin->axis[i].min = 0.0;
	}
	if (!AIR_EXISTS(nin->axis[i].max)) {
	  nin->axis[i].max = nin->axis[i].size-1;
	}
	rsi->min[i] = nin->axis[i].min;
	rsi->max[i] = nin->axis[i].max;
	rsi->samples[i] = sz[i];
	nin->axis[i].center = nrrdCenterNode;
      } else {
	rsi->kernel[i] = NULL;
      }
    }
    if (anyneed) {
      rsi->boundary = nrrdBoundaryBleed;
      rsi->type = nrrdTypeUnknown;
      rsi->renormalize = AIR_TRUE;
      rsi->clamp = AIR_TRUE;
      fprintf(stderr, "%s: resampling ... ", me); fflush(stderr);
      if (nrrdSpatialResample(nout, nin, rsi)) {
	sprintf(err, "%s: trouble upsampling", me);
	biffMove(QBERT, err, NRRD); airMopError(mop); return 1;
      }
    }
  } else {
    for (i=0; i<=2; i++) {
      anyneed |= need = sz[i] - nin->axis[i].size;
      fprintf(stderr, "%s: sz[%d] = %d -> need = %d --> ", 
	      me, i, nin->axis[i].size, need);
      need = AIR_MAX(0, need);
      fprintf(stderr, "%d --> ", need);
      padMin[i] = 0 - (int)floor(need/2.0);
      padMax[i] = nin->axis[i].size - 1 + (int)ceil(need/2.0);
      fprintf(stderr, "pad indices: [%d..%d]\n", padMin[i], padMax[i]);
    }
    if (anyneed) {
      fprintf(stderr, "%s: padding ... ", me); fflush(stderr);
      if (nrrdPad(nout, nin, padMin, padMax, nrrdBoundaryPad, 0.0)) {
	sprintf(err, "%s: trouble padding", me);
	biffMove(QBERT, err, NRRD); airMopError(mop); return 1;
      }
      fprintf(stderr, "done\n");
    }
  }
  if (!anyneed) {
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s: trouble copying", me);
      biffMove(QBERT, err, NRRD); airMopError(mop); return 1;
    }
  }
  if (qbertSaveAll) {
    fprintf(stderr, "%s: saving up.nrrd\n", me);
    nrrdSave("pad1.nrrd", nout, NULL);
  }
  airMopOkay(mop);
  return 0;
}

/*
** resampling to get axis[i]'s size down to exactly sz[i]
*/
int
qbertSizeDown(Nrrd *nout, Nrrd *nin, int *sz,
	      NrrdKernelSpec *dk) {
  char me[]="qbertSizeDown", err[AIR_STRLEN_MED];
  NrrdResampleInfo *rsi;
  int i, need;
  airArray *mop;

  mop = airMopNew();
  rsi = nrrdResampleInfoNew();
  airMopAdd(mop, rsi, (airMopper)nrrdResampleInfoNix, airMopAlways);
  rsi->boundary = nrrdBoundaryBleed;
  rsi->type = nrrdTypeFloat;
  rsi->renormalize = AIR_TRUE;
  need = 0;
  for (i=0; i<=2; i++) {
    if (nin->axis[i].size > sz[i]) {
      need = 1;
      rsi->kernel[i] = dk->kernel;
      memcpy(rsi->parm[i], dk->parm, dk->kernel->numParm*sizeof(double));
      rsi->samples[i] = sz[i];
      if (!AIR_EXISTS(nin->axis[i].min)) {
	nin->axis[i].min = 0.0;
      }
      if (!AIR_EXISTS(nin->axis[i].max)) {
	nin->axis[i].max = nin->axis[i].size-1;
      }
      rsi->min[i] = nin->axis[i].min;
      rsi->max[i] = nin->axis[i].max;
      nin->axis[i].center = nrrdCenterNode;
      fprintf(stderr, "%s: downsampling axis %d from %d to %d samples\n", 
	      me, i, nin->axis[i].size, rsi->samples[i]);
    }
    else {
      rsi->kernel[i] = NULL;
    }
  }
  if (need) {
    fprintf(stderr, "%s: resampling ... ", me); fflush(stderr);
    if (nrrdSpatialResample(nout, nin, rsi)) {
      sprintf(err, "%s: trouble resampling", me);
      biffMove(QBERT, err, NRRD); airMopError(mop); return 1;
    }
    fprintf(stderr, "done\n");
  }
  else {
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s: trouble copying", me);
      biffMove(QBERT, err, NRRD); airMopError(mop); return 1;
    }
  }
  rsi = nrrdResampleInfoNix(rsi);
  if (qbertSaveAll) {
    fprintf(stderr, "%s: saving rsmp.nrrd\n", me);
    nrrdSave("rsmp.nrrd", nout, NULL);
  }

  airMopOkay(mop); 
  return 0;
}

/*
** probing to getting floating point V, G, and maybe H values
*/
int
qbertProbe(Nrrd *nout, Nrrd *nin, int doH, int *sz) {
  char me[]="qbertProbe", err[AIR_STRLEN_MED], prog[AIR_STRLEN_SMALL];
  gageContext *ctx;
  gagePerVolume *pvl;
  gage_t *val, *gmag, *scnd;
  double kparm[3];
  float *vghF;
  int E, i, j, k, query;
  airArray *mop;

  doH = !!doH;
  mop = airMopNew();
  ctx = gageContextNew();
  pvl = gagePerVolumeNew(ctx, nin, gageKindScl);
  airMopAdd(mop, ctx, (airMopper)gageContextNix, airMopAlways);
  airMopAdd(mop, pvl, (airMopper)gagePerVolumeNix, airMopAlways);
  gageSet(ctx, gageParmVerbose, 0);
  gageSet(ctx, gageParmRenormalize, AIR_TRUE);
  gageSet(ctx, gageParmCheckIntegrals, AIR_TRUE);
  E = 0;
  if (!E) E |= gagePerVolumeAttach(ctx, pvl);
  /* about kernel setting for probing: currently, the way that probing is
     done is ONLY on grid locations, and never in between voxels.  That 
     means that the kernels set below are really only used for creating
     discrete convolution masks at unit locations */
  kparm[0] = 1.0;
  if (!E) E |= gageKernelSet(ctx, gageKernel00, nrrdKernelTent, kparm);
  /* We'll just use cendif to get central differencing */
  kparm[0] = 1.0;
  if (!E) E |= gageKernelSet(ctx, gageKernel11, nrrdKernelCentDiff, kparm);
  /* 2nd derivative of B-spline generates second central differences */
  kparm[0] = 1.0; kparm[1] = 1.0; kparm[2] = 0.0;
  if (!E) E |= gageKernelSet(ctx, gageKernel22, nrrdKernelBCCubicDD, kparm);
  query = (1 << gageSclValue) | (1 << gageSclGradMag);
  if (doH) {
    query |= (1 << gageScl2ndDD);
  }
  if (!E) E |= gageQuerySet(ctx, pvl, query);
  if (!E) E |= gageUpdate(ctx);
  if (E) {
    sprintf(err, "%s: gage trouble", me);
    biffMove(QBERT, err, GAGE); airMopError(mop); return 1;
  }
  gageSet(ctx, gageParmVerbose, 0);
  val = gageAnswerPointer(ctx, pvl, gageSclValue);
  gmag = gageAnswerPointer(ctx, pvl, gageSclGradMag);
  scnd = gageAnswerPointer(ctx, pvl, gageScl2ndDD);
  if (nrrdMaybeAlloc(nout, nrrdTypeFloat, 4, 2+doH, sz[0], sz[1], sz[2])) {
    sprintf(err, "%s: couldn't allocate floating point VG%s volume",
	    me, doH ? "H" : "");
    biffMove(QBERT, err, NRRD); airMopError(mop); return 1;
  }
  vghF = nout->data;
  fprintf(stderr, "%s: probing ...       ", me); fflush(stderr);
  for (k=0; k<sz[2]; k++) {
    for (j=0; j<sz[1]; j++) {
      if (!((j + sz[1]*k)%100)) {
	fprintf(stderr, "%s", airDoneStr(0, j + sz[1]*k, sz[1]*sz[2], prog));
	fflush(stderr);
      }
      for (i=0; i<sz[0]; i++) {
	gageProbe(ctx, i, j, k);
	vghF[0] = *val;
	vghF[1] = *gmag;
	if (doH) {
	  vghF[2] = *scnd;
	}
	vghF += 2+doH;
      }
    }
  }
  fprintf(stderr, "%s\n", airDoneStr(0, 2, 1, prog));
  ctx = gageContextNix(ctx);
  if (qbertSaveAll) {
    fprintf(stderr, "%s: saving vghF.nrrd\n", me);
    nrrdSave("vghF.nrrd", nout, NULL);
  }

  return 0;
}

/*
** make histograms of v,g,h values as first step in determining the
** inclusions for the later quantization
*/
int
qbertMakeVghHists(Nrrd *nvhist, Nrrd *nghist, Nrrd *nhhist,
		  int *sz, int bins,
		  Nrrd *nvghF, Nrrd *nin) {
  char me[]="qbertMakeVghHists", err[AIR_STRLEN_MED];
  double minv, maxv, ming, maxg, minh=0, maxh=0;
  float *vghF;
  int nval, doH, i, E, *vhist, *ghist, *hhist=NULL, vi, gi, hi;

  nval = nvghF->axis[0].size;
  doH = !!(nval == 3);
  vghF = (float *)nvghF->data;
  minv = maxv = vghF[0 + nval*0];
  ming = maxg = vghF[1 + nval*0];
  if (doH) {
    minh = maxh = vghF[2 + nval*0];
  }
  for (i=0; i<sz[0]*sz[1]*sz[2]; i++) {
    minv = AIR_MIN(minv, vghF[0 + nval*i]);
    maxv = AIR_MAX(maxv, vghF[0 + nval*i]);
    ming = AIR_MIN(ming, vghF[1 + nval*i]);
    maxg = AIR_MAX(maxg, vghF[1 + nval*i]);
    if (doH) {
      minh = AIR_MIN(minh, vghF[2 + nval*i]);
      maxh = AIR_MAX(maxh, vghF[2 + nval*i]);
    }
  }
  fprintf(stderr, "%s: values: [%g .. %g] -> ", me, minv, maxv);
  /* just because we're bastards, we're going to enforce minv >= 0 for
     types that started as unsigned integral types.  Downsampling with a
     ringing kernel can have produced negative values, so this change to
     minv can actually restrict the range, in contrast to to the changes
     to ming, minh, and maxh below */
  if (nrrdTypeIsUnsigned[nin->type]) {
    minv = AIR_MAX(minv, 0.0);
  }
  fprintf(stderr, "[%g .. %g]\n", minv, maxv);
  fprintf(stderr, "%s:  grads: [%g .. %g] -> ", me, ming, maxg);
  ming = 0;
  fprintf(stderr, "[%g .. %g]\n", ming, maxg);
  if (doH) {
    fprintf(stderr, "%s: 2ndDDs: [%g .. %g] -> ", me, minh, maxh);
    if (maxh > -minh) 
      minh = -maxh;
    else
      maxh = -minh;
    fprintf(stderr, "[%g .. %g]\n", minh, maxh);
  }
  fprintf(stderr, "%s: using %d-bin histograms\n", me, bins);
  E = 0;
  if (!E) E |= nrrdMaybeAlloc(nvhist, nrrdTypeInt, 1, bins);
  if (!E) E |= nrrdMaybeAlloc(nghist, nrrdTypeInt, 1, bins);
  if (doH) {
    if (!E) E |= nrrdMaybeAlloc(nhhist, nrrdTypeInt, 1, bins);
  }
  if (E) {
    sprintf(err, "%s: couldn't allocate %d %d-bin histograms",
	    me, nval, bins);
    biffMove(QBERT, err, NRRD); return 1;
  }
  nvhist->axis[0].min = minv;   nvhist->axis[0].max = maxv;
  nghist->axis[0].min = ming;   nghist->axis[0].max = maxg;
  vhist = (int *)nvhist->data;
  ghist = (int *)nghist->data;
  memset(vhist, 0, bins*sizeof(int));
  memset(ghist, 0, bins*sizeof(int));
  if (doH) {
    nhhist->axis[0].min = minh;   nhhist->axis[0].max = maxh; 
    hhist = (int *)nhhist->data;
    memset(hhist, 0, bins*sizeof(int));
  }
  vghF = (float *)nvghF->data;
  for (i=0; i<sz[0]*sz[1]*sz[2]; i++) {
    AIR_INDEX(minv, vghF[0], maxv, bins, vi); vi = AIR_CLAMP(0, vi, bins-1);
    AIR_INDEX(ming, vghF[1], maxg, bins, gi);
    vhist[vi]++;
    ghist[gi]++;
    if (doH) {
      AIR_INDEX(minh, vghF[2], maxh, bins, hi);
      hhist[hi]++;
    }
    vghF += nval;
  }
  if (qbertSaveAll) {
    fprintf(stderr, "%s: saving {v,g,h}hist.nrrd\n", me);
    nrrdSave("vhist.nrrd", nvhist, NULL);
    nrrdSave("ghist.nrrd", nghist, NULL);
    if (doH) {
      nrrdSave("hhist.nrrd", nhhist, NULL);
    }
  }

  return 0;
}

/*
** determine inclusion from histograms and create 8-bit VGH volume
*/
int
qbertMakeVgh(Nrrd *nvgh, Nrrd *nvhist, Nrrd *nghist, Nrrd *nhhist,
	     int *sz, float *perc,
	     Nrrd *nvghF) {
  char me[]="qbertMakeVgh", err[AIR_STRLEN_MED], cmt[AIR_STRLEN_SMALL];
  double minv, maxv, ming, maxg, minh=0, maxh=0;
  int lose, i, *vhist, *ghist, *hhist=NULL, bins, vi, gi, hi, nval, doH;
  unsigned char *vgh;
  float *vghF;

  nval = nvghF->axis[0].size;
  doH = !!(nval == 3);
  minv = nvhist->axis[0].min;   maxv = nvhist->axis[0].max; 
  ming = nghist->axis[0].min;   maxg = nghist->axis[0].max; 
  vhist = (int *)nvhist->data;
  ghist = (int *)nghist->data;
  if (doH) {
    minh = nhhist->axis[0].min;   maxh = nhhist->axis[0].max; 
    hhist = (int *)nhhist->data;
  }

  lose = perc[0]*sz[0]*sz[1]*sz[2]/100;
  bins = nvhist->axis[0].size;
  i = bins-1;
  while (lose > 0) {
    /* HEY: we're nibbling only from top, even though for signed
       value types, there could be a tail at low negative values (had
       this problem with some CT data) */
    lose -= vhist[i--];
  }
  maxv = AIR_AFFINE(0, i, bins-1, minv, maxv);

  lose = perc[1]*sz[0]*sz[1]*sz[2]/100;
  bins = nghist->axis[0].size;
  i = bins-1;
  while (lose > 0) {
    /* nibble from top */
    lose -= ghist[i--];
  }
  maxg = AIR_AFFINE(0, i, bins-1, ming, maxg);

  lose = perc[2]*sz[0]*sz[1]*sz[2]/100;
  bins = nhhist->axis[0].size;
  i = 0;
  while (lose > 0) {
    /* nibble from top and bottom at equal rates */
    lose -= hhist[i] + hhist[bins-1-i];
    i++;
  }
  minh = AIR_AFFINE(0, i, bins-1, minh, maxh);
  maxh = -minh;

  fprintf(stderr, "%s: new values (ignored %5d): [%g .. %g]\n",
	  me, (int)(perc[0]*sz[0]*sz[1]*sz[2]/100), minv, maxv);
  fprintf(stderr, "%s: new  grads (ignored %5d): [%g .. %g]\n",
	  me, (int)(perc[1]*sz[0]*sz[1]*sz[2]/100), ming, maxg);
  if (doH) {
    fprintf(stderr, "%s: new 2ndDDs (ignored %5d): [%g .. %g]\n",
	    me, (int)(perc[2]*sz[0]*sz[1]*sz[2]/100), minh, maxh);
    fprintf(stderr, "%s: putting 2ndDD in range 1 to 169 (0.0 -> 85)\n", me);
  }
  
  if (nrrdMaybeAlloc(nvgh, nrrdTypeUChar, 4, nval, sz[0], sz[1], sz[2])) {
    sprintf(err, "%s: couldn't allocate 8-bit VG%s volume",
	    me, doH ? "H" : "");
    biffMove(QBERT, err, NRRD); return 1;
  }
  vgh = (unsigned char*)nvgh->data;
  vghF = (float*)nvghF->data;
  for (i=0; i<sz[0]*sz[1]*sz[2]; i++) {
    AIR_INDEX(minv, vghF[0], maxv, 254, vi);
    vgh[0] = AIR_CLAMP(1, vi+1, 254);
    AIR_INDEX(ming, vghF[1], maxg, 254, gi);
    vgh[1] = AIR_CLAMP(1, gi+1, 254);
    if (doH) {
      AIR_INDEX(minh, vghF[2], maxh, 168, hi);
      vgh[2] = AIR_CLAMP(1, hi+1, 169);
    }
    vgh += nval;
    vghF += nval;
  }
  if (doH) {
    sprintf(cmt, "exclusions (v g h): %g %g %g", perc[0], perc[1], perc[2]);
  } else {
    sprintf(cmt, "exclusions (v g): %g %g", perc[0], perc[1]);
  }
  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "minv: %g", minv);  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "maxv: %g", maxv);  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "ming: %g", ming);  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "maxg: %g", maxg);  nrrdCommentAdd(nvgh, cmt);
  if (doH) {
    sprintf(cmt, "minh: %g", minh);  nrrdCommentAdd(nvgh, cmt);
    sprintf(cmt, "maxh: %g", maxh);  nrrdCommentAdd(nvgh, cmt);
  }
  nrrdAxesSet(nvgh, nrrdAxesInfoCenter, nrrdCenterUnknown,
	      nrrdCenterNode, nrrdCenterNode, nrrdCenterNode);

  return 0;
}
  
int
main(int argc, char *argv[]) {
  char *me, *outS, *errS;
  Nrrd *nin, *npad, *nrsmp, *nvghF, *nvhist, *nghist, *nhhist, *nvgh;
  int E, i, sz[3], ups, doH, useFloat;
  double amin[4], amax[4], spacing[4];
  float vperc, gperc, hperc, perc[3];
  NrrdKernelSpec *dk, *uk;
  hestParm *hparm;
  hestOpt *hopt = NULL;

  me = argv[0];
  hparm = hestParmNew();
  hparm->elideSingleOtherType = AIR_TRUE;
  hparm->elideSingleNonExistFloatDefault = AIR_TRUE;
  hparm->elideMultipleNonExistFloatDefault = AIR_TRUE;

  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nin, NULL,
	     "input volume, in nrrd format",
	     NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "h", NULL, airTypeInt, 0, 0, &doH, NULL,
	     "Make a 3-channel VGH volume, instead of the usual (default) "
	     "2-channel VG volume.");
  hestOptAdd(&hopt, "f", NULL, airTypeInt, 0, 0, &useFloat, NULL,
	     "Keep the output volume in floating point, instead of "
	     "(by default) quantizing down to 8-bits.  The "
	     "\"-vp\", \"-gp\", and \"-hp\" options become moot.");
  hestOptAdd(&hopt, "d", "dimX dimY dimZ", airTypeInt, 3, 3, sz, NULL,
	     "dimensions of output volume");
  hestOptAdd(&hopt, "up", NULL, airTypeInt, 0, 0, &ups, NULL,
	     "Instead of just padding axes up to dimensions given "
	     "with \"-d\" when original dimensions are smaller, do filtered "
	     "upsampling.");
  hestOptAdd(&hopt, "uk", "upsample k", airTypeOther, 1, 1, &uk,
	     "cubic:0,0.5",
	     "kernel to use when doing the upsampling enabled by \"-up\"",
	     NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "dk", "downsample k", airTypeOther, 1, 1, &dk, "tent",
	     "kernel to use when DOWNsampling volume to fit with specified "
	     "dimensions. NOTE: ringing can be problematic here.",
	     NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "vp", "V excl perc", airTypeFloat, 1, 1, &vperc,
	     "0.000",
	     "Percent of voxels to through away in quantization (if doing "
	     "quantization) based their data value being too or too low. ");
  hestOptAdd(&hopt, "gp", "G perc", airTypeFloat, 1, 1, &gperc, "0.002",
	     "Like \"-vp\", but for gradient magnitudes. ");
  hestOptAdd(&hopt, "hp", "H perc", airTypeFloat, 1, 1, &hperc, "0.004",
	     "Like \"-vp\", but for Hessian-based 2nd derivatives. ");
  hestOptAdd(&hopt, "o", "output", airTypeString, 1, 1, &outS, NULL,
	     "output volume in nrrd format");
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
		 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);

  if (3 != nin->dim) {
    fprintf(stderr, "%s: input nrrd is %-dimensional, not 3\n", me, nin->dim);
    exit(1);
  }

  npad = nrrdNew();
  if (qbertSizeUp(npad, nin, sz, ups ? uk : NULL)) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, errS = biffGetDone(QBERT));
    free(errS); exit(1);
  }  

  nrsmp = nrrdNew();
  if (qbertSizeDown(nrsmp, npad, sz, dk)) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, errS = biffGetDone(QBERT));
    free(errS); exit(1);
  }
  npad = nrrdNuke(npad);
  
  /* this axis info is being saved so that it can be re-enstated at the end */
  spacing[0] = amin[0] = amax[0] = AIR_NAN;
  nrrdAxesGet_nva(nrsmp, nrrdAxesInfoSpacing, spacing+1);
  nrrdAxesGet_nva(nrsmp, nrrdAxesInfoMin, amin+1);
  nrrdAxesGet_nva(nrsmp, nrrdAxesInfoMax, amax+1);
  /* if we had to downsample, we may have enstated axis mins and maxs where
     they didn't exist before, and those shouldn't be saved in output.  But
     we can't just copy axis mins and maxs from the original input because
     padding could have changed them.  If no axis mins and maxs existed on
     the input nrrd, these will all be nan, so they won't be saved out. */
  for (i=0; i<=2; i++) {
    if (!AIR_EXISTS(nin->axis[i].min))
      amin[1+i] = AIR_NAN;
    if (!AIR_EXISTS(nin->axis[i].max))
      amax[1+i] = AIR_NAN;
  }
  
  nvghF = nrrdNew();
  if (qbertProbe(nvghF, nrsmp, doH, sz)) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, errS = biffGetDone(QBERT));
    free(errS); exit(1);
  }
  nrsmp = nrrdNuke(nrsmp);

  if (useFloat) {
    /* we're done! */
    E = nrrdSave(outS, nvghF, NULL);
  } else {
    nvhist = nrrdNew();
    nghist = nrrdNew();
    nhhist = nrrdNew();
    if (qbertMakeVghHists(nvhist, nghist, nhhist,
			  sz, QBERT_HIST_BINS,
			  nvghF, nin)) {
      fprintf(stderr, "%s: trouble:\n%s\n", me, errS = biffGetDone(QBERT));
      free(errS); exit(1);
    }
    
    nvgh = nrrdNew();
    ELL_3V_SET(perc, vperc, gperc, hperc);
    if (qbertMakeVgh(nvgh, nvhist, nghist, nhhist, sz, perc, nvghF)) {
      fprintf(stderr, "%s: trouble:\n%s\n", me, errS = biffGetDone(QBERT));
      free(errS); exit(1);
    }
    nvghF = nrrdNuke(nvghF);
    nvhist = nrrdNuke(nvhist);
    nghist = nrrdNuke(nghist);
    nhhist = nrrdNuke(nhhist);
    
    /* do final decoration of axes */
    nrrdAxesSet(nvgh, nrrdAxesInfoLabel, "vgh", "x", "y", "z");
    nrrdAxesSet_nva(nvgh, nrrdAxesInfoMin, amin);
    nrrdAxesSet_nva(nvgh, nrrdAxesInfoMax, amax);
    nrrdAxesSet_nva(nvgh, nrrdAxesInfoSpacing, spacing);
    nrrdContentSet(nvgh, "qbert", nin, "");
    nin = nrrdNuke(nin);
    
    E = nrrdSave(outS, nvgh, NULL);
    nvgh = nrrdNuke(nvgh);
  }
  if (E) {
    fprintf(stderr, "%s: trouble saving output:\n%s\n", me,
	    errS = biffGetDone(NRRD));
    free(errS); exit(1);
  }
  hparm = hestParmFree(hparm);
  hopt = hestOptFree(hopt);

  /* HEY: why am I getting memory-in-use with purify? */
  exit(0);
}
