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

char info[]="Generates volume datasets for the Simian volume renderer. "
"Each voxel in the output volume contains 8-bit quantized values of the data "
"value, gradient magnitude, and 2nd directional derivative.";

int saveall = AIR_TRUE;  /* can be used to save output of every stage */

/*
** This padding/resampling is to get axis[i]'s size >= sz[i], which
** is only needed if the input volume is smaller along any of the axes
** than the desired output volume.
*/
int
qbertSizeUp(Nrrd *nout, Nrrd *nin, int *sz,
	    NrrdKernelSpec *uk) {
  char me[]="qbertPadStage1", err[AIR_STRLEN_MED];
  int i, need, padMin[3], padMax[3];
  /* NrrdResampleInfo *rsmpInfo; */

  need = 0;
  for (i=0; i<=2; i++) {
    need = sz[i] - nin->axis[i].size;
    fprintf(stderr, "%s: sz[%d] = %d -> need = %d --> ", 
	    me, i, nin->axis[i].size, need);
    need = AIR_MAX(0, need);
    fprintf(stderr, "%d --> ", need);
    padMin[i] = 0 - (int)floor(need/2.0);
    padMax[i] = nin->axis[i].size - 1 + (int)ceil(need/2.0);
    fprintf(stderr, "pad indices: [%d..%d]\n", padMin[i], padMax[i]);
  }
  fprintf(stderr, "%s: padding ... ", me); fflush(stderr);
  if (nrrdPad(nout, nin, padMin, padMax, nrrdBoundaryPad, 0.0)) {
    sprintf(err, "%s: trouble padding", me);
    biffMove(QBERT, err, NRRD); return 1;
  }
  fprintf(stderr, "done\n");
  if (saveall) {
    fprintf(stderr, "%s: saving pad1.nrrd\n", me);
    nrrdSave("pad1.nrrd", nout, NULL);
  }
  return 0;
}

/*
** resampling to get axis[i]'s size down to exactly sz[i]
*/
int
qbertSizeDown(Nrrd *nout, Nrrd *nin, int *sz,
	      NrrdKernelSpec *dk) {
  char me[]="qbertResample", err[AIR_STRLEN_MED];
  NrrdResampleInfo *rsmpInfo;
  int i, need;

  rsmpInfo = nrrdResampleInfoNew();
  rsmpInfo->boundary = nrrdBoundaryBleed;
  rsmpInfo->type = nrrdTypeFloat;
  rsmpInfo->renormalize = AIR_TRUE;
  need = 0;
  for (i=0; i<=2; i++) {
    if (nin->axis[i].size > sz[i]) {
      need = 1;
      rsmpInfo->kernel[i] = dk->kernel;
      memcpy(rsmpInfo->parm[i], dk->parm, dk->kernel->numParm*sizeof(double));
      rsmpInfo->samples[i] = sz[i];
      if (!AIR_EXISTS(nin->axis[i].min)) {
	nin->axis[i].min = 0.0;
      }
      if (!AIR_EXISTS(nin->axis[i].max)) {
	nin->axis[i].max = nin->axis[i].size-1;
      }
      rsmpInfo->min[i] = nin->axis[i].min;
      rsmpInfo->max[i] = nin->axis[i].max;
      nin->axis[i].center = nrrdCenterNode;
      fprintf(stderr, "%s: resampling axis %d from %d to %d samples\n", 
	      me, i, nin->axis[i].size, rsmpInfo->samples[i]);
    }
    else {
      rsmpInfo->kernel[i] = NULL;
    }
  }
  fprintf(stderr, "%s: resampling ... ", me); fflush(stderr);
  if (need) {
    if (nrrdSpatialResample(nout, nin, rsmpInfo)) {
      sprintf(err, "%s: trouble resampling", me);
      biffMove(QBERT, err, NRRD); return 1;
    }
  }
  else {
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s: trouble copying", me);
      biffMove(QBERT, err, NRRD); return 1;
    }
  }
  fprintf(stderr, "done\n");
  rsmpInfo = nrrdResampleInfoNix(rsmpInfo);
  if (saveall) {
    fprintf(stderr, "%s: saving rsmp.nrrd\n", me);
    nrrdSave("rsmp.nrrd", nout, NULL);
  }

  return 0;
}

/*
** probing to getting floating point V, G, H values
*/
int
qbertProbe(Nrrd *nout, Nrrd *nin, int *sz) {
  char me[]="qbertProbe", err[AIR_STRLEN_MED], prog[AIR_STRLEN_SMALL];
  gageContext *ctx;
  gagePerVolume *pvl;
  gage_t *val, *gmag, *scnd;
  double kparm[3];
  float *vghF;
  double t0, t1;
  int E, i, j, k;

  ctx = gageContextNew();
  pvl = gagePerVolumeNew(nin, gageKindScl);
  gageSet(ctx, gageParmVerbose, 0);
  gageSet(ctx, gageParmRenormalize, AIR_TRUE);
  gageSet(ctx, gageParmCheckIntegrals, AIR_TRUE);
  kparm[0] = 1.0; kparm[1] = 1.0; kparm[2] = 0.0;
  E = 0;
  if (!E) E |= gagePerVolumeAttach(ctx, pvl);
  /* about kernel setting for probing: currently, the way that probing is
     done is ONLY on grid locations, and never in between voxels.  That 
     means that the kernels set below are really only use for creating
     discrete convolution masks at unit locations */
  kparm[0] = 1.0;
  if (!E) E |= gageKernelSet(ctx, gageKernel00, nrrdKernelTent, kparm);
  /* We'll just use cendif to get central differencing */
  kparm[0] = 1.0;
  if (!E) E |= gageKernelSet(ctx, gageKernel11, nrrdKernelCentDiff, kparm);
  /* 2nd derivative of B-spline generates second central differences */
  kparm[0] = 1.0; kparm[1] = 1.0; kparm[2] = 0.0;
  if (!E) E |= gageKernelSet(ctx, gageKernel22, nrrdKernelBCCubicDD, kparm);
  if (!E) E |= gageQuerySet(pvl,
			    (1 << gageSclValue) | 
			    (1 << gageSclGradMag) |
			    (1 << gageScl2ndDD));
  if (!E) E |= gageUpdate(ctx);
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(GAGE));
    exit(1);
  }
  gageSet(ctx, gageParmVerbose, 0);
  val = gageAnswerPointer(pvl, gageSclValue);
  gmag = gageAnswerPointer(pvl, gageSclGradMag);
  scnd = gageAnswerPointer(pvl, gageScl2ndDD);
  if (nrrdMaybeAlloc(nout, nrrdTypeFloat, 4, 3, sz[0], sz[1], sz[2])) {
    sprintf(err, "%s: couldn't allocate floating point VGH volume", me);
    biffMove(QBERT, err, NRRD); return 1;
  }
  vghF = nout->data;
  fprintf(stderr, "%s: probing ...       ", me); fflush(stderr);
  t0 = airTime();
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
	vghF[2] = *scnd;
	vghF += 3;
      }
    }
  }
  t1 = airTime();
  fprintf(stderr, "%s; probe rate = %g KHz\n",
	  airDoneStr(0, 2, 1, prog), sz[0]*sz[1]*sz[2]/(1000.0*(t1-t0)));
  ctx = gageContextNix(ctx);
  if (saveall) {
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
  double minv, maxv, ming, maxg, minh, maxh;
  float *vghF;
  int i, E, *vhist, *ghist, *hhist, vi, gi, hi;

  vghF = (float *)nvghF->data;
  minv = maxv = vghF[0 + 3*0];
  ming = maxg = vghF[1 + 3*0];
  minh = maxh = vghF[2 + 3*0];
  for (i=0; i<sz[0]*sz[1]*sz[2]; i++) {
    minv = AIR_MIN(minv, vghF[0 + 3*i]);
    maxv = AIR_MAX(maxv, vghF[0 + 3*i]);
    ming = AIR_MIN(ming, vghF[1 + 3*i]);
    maxg = AIR_MAX(maxg, vghF[1 + 3*i]);
    minh = AIR_MIN(minh, vghF[2 + 3*i]);
    maxh = AIR_MAX(maxh, vghF[2 + 3*i]);
  }
  fprintf(stderr, "%s: values: [%g .. %g] -> ", me, minv, maxv);
  /* just because we're bastards, we're going to enforce minv >= 0 for
     types that started as unsigned integral types.  Downsampling with a
     ringing kernel can have produced negative values, so this change to
     minv can actually restrict the range, in contrast to to the changes
     to ming, minh, and maxh below */
  if (nrrdTypeUnsigned[nin->type]) {
    minv = AIR_MAX(minv, 0.0);
  }
  fprintf(stderr, "[%g .. %g]\n", minv, maxv);
  fprintf(stderr, "%s:  grads: [%g .. %g] -> ", me, ming, maxg);
  ming = 0;
  fprintf(stderr, "[%g .. %g]\n", ming, maxg);
  fprintf(stderr, "%s: 2ndDDs: [%g .. %g] -> ", me, minh, maxh);
  if (maxh > -minh) 
    minh = -maxh;
  else
    maxh = -minh;
  fprintf(stderr, "[%g .. %g]\n", minh, maxh);
  fprintf(stderr, "%s: using %d-bin histograms\n", me, bins);
  E = 0;
  if (!E) E |= nrrdMaybeAlloc(nvhist, nrrdTypeInt, 1, bins);
  if (!E) E |= nrrdMaybeAlloc(nghist, nrrdTypeInt, 1, bins);
  if (!E) E |= nrrdMaybeAlloc(nhhist, nrrdTypeInt, 1, bins);
  if (E) {
    sprintf(err, "%s: couldn't allocate 3 %d-bin histograms", me, bins);
    biffMove(QBERT, err, NRRD); return 1;
  }
  nvhist->axis[0].min = minv;   nvhist->axis[0].max = maxv; 
  nghist->axis[0].min = ming;   nghist->axis[0].max = maxg; 
  nhhist->axis[0].min = minh;   nhhist->axis[0].max = maxh; 
  vhist = (int *)nvhist->data;
  ghist = (int *)nghist->data;
  hhist = (int *)nhhist->data;
  memset(vhist, 0, bins*sizeof(int));
  memset(ghist, 0, bins*sizeof(int));
  memset(hhist, 0, bins*sizeof(int));
  vghF = (float *)nvghF->data;
  for (i=0; i<sz[0]*sz[1]*sz[2]; i++) {
    AIR_INDEX(minv, vghF[0], maxv, bins, vi); vi = AIR_CLAMP(0, vi, bins-1);
    AIR_INDEX(ming, vghF[1], maxg, bins, gi);
    AIR_INDEX(minh, vghF[2], maxh, bins, hi);
    vhist[vi]++;
    ghist[gi]++;
    hhist[hi]++;
    vghF += 3;
  }
  if (saveall) {
    fprintf(stderr, "%s: saving {v,g,h}hist.nrrd\n", me);
    nrrdSave("vhist.nrrd", nvhist, NULL);
    nrrdSave("ghist.nrrd", nghist, NULL);
    nrrdSave("hhist.nrrd", nhhist, NULL);
  }

  return 0;
}

/*
** determine inclusion from histograms and create 8-bit VGH volume
*/
int
qbertMakeVgh(Nrrd *nvgh, Nrrd *nvhist, Nrrd *nghist, Nrrd *nhhist,
	     int *sz, double *perc,
	     Nrrd *nvghF) {
  char me[]="qbertMakeVgh", err[AIR_STRLEN_MED], cmt[AIR_STRLEN_SMALL];
  double minv, maxv, ming, maxg, minh, maxh;
  int lose, i, *vhist, *ghist, *hhist, bins, vi, gi, hi;
  unsigned char *vgh;
  float *vghF;

  minv = nvhist->axis[0].min;   maxv = nvhist->axis[0].max; 
  ming = nghist->axis[0].min;   maxg = nghist->axis[0].max; 
  minh = nhhist->axis[0].min;   maxh = nhhist->axis[0].max; 
  vhist = (int *)nvhist->data;
  ghist = (int *)nghist->data;
  hhist = (int *)nhhist->data;

  lose = perc[0]*sz[0]*sz[1]*sz[2]/100;
  bins = nvhist->axis[0].size;
  i = bins-1;
  while (lose > 0) {
    /* HEY: we're nibbling only from top, even though there for signed
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
  fprintf(stderr, "%s: new 2ndDDs (ignored %5d): [%g .. %g]\n",
	  me, (int)(perc[2]*sz[0]*sz[1]*sz[2]/100), minh, maxh);
  fprintf(stderr, "%s: putting 2ndDD in range 1 to 169 (0.0 -> 85)\n", me);
  
  if (nrrdMaybeAlloc(nvgh, nrrdTypeUChar, 4, 3, sz[0], sz[1], sz[2])) {
    sprintf(err, "%s: couldn't allocate 8-bit VGH volume", me);
    biffMove(QBERT, err, NRRD); return 1;
  }
  vgh = (unsigned char*)nvgh->data;
  vghF = (float*)nvghF->data;
  for (i=0; i<sz[0]*sz[1]*sz[2]; i++) {
    AIR_INDEX(minv, vghF[0], maxv, 254, vi);
    vgh[0] = AIR_CLAMP(1, vi+1, 254);
    AIR_INDEX(ming, vghF[1], maxg, 254, gi);
    vgh[1] = AIR_CLAMP(1, gi+1, 254);
    AIR_INDEX(minh, vghF[2], maxh, 168, hi);
    vgh[2] = AIR_CLAMP(1, hi+1, 169);
    vgh += 3;
    vghF += 3;
  }
  sprintf(cmt, "exclusions (v g h): %g %g %g", perc[0], perc[1], perc[2]);
  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "minv: %g", minv);  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "maxv: %g", maxv);  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "ming: %g", ming);  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "maxg: %g", maxg);  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "minh: %g", minh);  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "maxh: %g", maxh);  nrrdCommentAdd(nvgh, cmt);
  nrrdAxesSet(nvgh, nrrdAxesInfoCenter, nrrdCenterUnknown,
	      nrrdCenterNode, nrrdCenterNode, nrrdCenterNode);

  return 0;
}
  
int
main(int argc, char *argv[]) {
  char *me, *outS, *errS;
  Nrrd *nin, *npad, *nrsmp, *nvghF, *nvhist, *nghist, *nhhist, *nvgh;
  int i, sz[3], ups;
  double amin[4], amax[4], spacing[4];
  NrrdKernelSpec *dk, *uk;
  hestParm *hparm;
  hestOpt *hopt = NULL;
  double inc[3*(1+BANE_INC_PARM_NUM)];

  me = argv[0];
  hparm = hestParmNew();
  hparm->elideSingleOtherType = AIR_TRUE;
  hparm->elideSingleNonExistFloatDefault = AIR_TRUE;
  hparm->elideMultipleNonExistFloatDefault = AIR_TRUE;

  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nin, NULL,
	     "input volume, in nrrd format",
	     NULL, NULL, nrrdHestNrrd);
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
	     "kernel to use when downsampling volume to fit with specified "
	     "dimensions; ringing can be problematic here.",
	     NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "s", "incV incG incH", airTypeOther, 3, 3, inc, 
	     "f:1.0 p:0.01 p:0.03",
	     "Strategies for determining how much of the range "
	     "of a quantity (V, G, or H) should be included and quantized "
	     "in the output VGH volume.  Possibilities include:\n "
	     "\b\bo \"f:<F>\": included range is some fraction of the "
	     "total range, as scaled by F\n "
	     "\b\bo \"p:<P>\": exclude the extremal P percent of "
	     "the values\n "
	     "\b\bo \"s:<S>\": included range is S times the standard "
	     "deviation of the values\n "
	     "\b\bo \"a:<min>,<max>\": range is from <min> to <max>",
	     NULL, NULL, baneGkmsHestIncStrategy);
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
  if (qbertProbe(nvghF, nrsmp, sz)) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, errS = biffGetDone(QBERT));
    free(errS); exit(1);
  }
  nrsmp = nrrdNuke(nrsmp);

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
  if (qbertMakeVgh(nvgh, nvhist, nghist, nhhist, sz, inc, nvghF)) {
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
  
  nrrdSave(outS, nvgh, NULL);
  nvgh = nrrdNuke(nvgh);
  hparm = hestParmFree(hparm);
  hopt = hestOptFree(hopt);

  /* HEY: why am I getting memory-in-use with purify? */
  exit(0);
}
