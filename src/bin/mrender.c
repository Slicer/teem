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
#include <biff.h>
#include <nrrd.h>
#include <gage.h>
#include <limn.h>
#include <hoover.h>


#define MREND "mrender"

char *info = ("A demonstration of nrrd measures, with hoover and gage. "
	      "Uses hoover to cast rays through a scalar volume, gage to "
	      "reconstruct values along the rays, and a specified "
	      "nrrd measure to reduce all the values along a ray down "
	      "to one scalar, which is saved in the output image.");

/* -------------------------------------------------------------- */

typedef struct {
  Nrrd *nin;
  double rayStep;
  int whatq, measr, renorm;
  NrrdKernelSpec *ksp00, *ksp11, *ksp22;
  hoovContext *ctx;
  char *outS;

  airArray *rmop;
} mrendUserInfo;

mrendUserInfo *
mrendUserInfoNew(hoovContext *ctx) {
  mrendUserInfo *uu;

  uu = (mrendUserInfo *)calloc(1, sizeof(mrendUserInfo));
  uu->nin = NULL;
  uu->rayStep = 0.0;
  uu->whatq = gageSclUnknown;
  uu->measr = nrrdMeasureUnknown;
  uu->renorm = AIR_TRUE;
  uu->ksp00 = uu->ksp11 = uu->ksp22 = NULL;
  uu->ctx = ctx;
  uu->rmop = NULL;
  return uu;
}

/* -------------------------------------------------------------- */

/* HEY: this is obviously a short-term solution: nothing about
   the gage context belongs in render info- that is purely 
   thread-specific
*/

typedef struct {
  gageContext *gtx;
  gage_t *answer;
  double time0, time1;
  Nrrd *nout;
  float *imgData;
  int sx, sy,     /* image dimensions */
    numSamples;   /* HEY: totally un-thread-safe counter
		     of all samples per rendering */
} mrendRenderInfo;

int
mrendRenderBegin(mrendRenderInfo **rrP, mrendUserInfo *uu) {
  char me[]="mrendRenderBegin", err[AIR_STRLEN_MED];
  gageContext *gtx;
  gagePerVolume *pvl;
  int E;

  *rrP = (mrendRenderInfo *)calloc(1, sizeof(mrendRenderInfo));
  (*rrP)->gtx = gtx = gageContextNew();
  pvl = gagePerVolumeNew(gageKindScl);

  uu->rmop = airMopInit();
  airMopAdd(uu->rmop, *rrP, airFree, airMopAlways);
  airMopAdd(uu->rmop, gtx, (airMopper)gageContextNix, airMopAlways);

  (*rrP)->time0 = airTime();
  gageSet(gtx, gageRenormalize, uu->renorm);
  gageSet(gtx, gageCheckIntegrals, AIR_TRUE);
  E = 0;
  if (!E) E |= gagePerVolumeAttach(gtx, pvl);
  if (!E) E |= gageKernelSet(gtx, gageKernel00,
			     uu->ksp00->kernel, uu->ksp00->parm);
  if (!E) E |= gageKernelSet(gtx, gageKernel11,
			     uu->ksp11->kernel, uu->ksp11->parm);
  if (!E) E |= gageKernelSet(gtx, gageKernel22,
			     uu->ksp22->kernel, uu->ksp22->parm);
  if (!E) E |= gageVolumeSet(gtx, pvl, uu->nin);
  if (!E) E |= gageQuerySet(gtx, pvl, 1 << uu->whatq);
  if (!E) E |= gageUpdate(gtx);
  if (E) {
    sprintf(err, "%s: gage trouble", me);
    biffMove(MREND, err, GAGE);
    return 1;
  }
  fprintf(stderr, "%s: kernel support = %d samples\n", me, gtx->fd);
  (*rrP)->answer = gageAnswerPointer(pvl, uu->whatq);

  if (nrrdAlloc((*rrP)->nout=nrrdNew(), nrrdTypeFloat, 2,
		uu->ctx->imgSize[0], uu->ctx->imgSize[1])) {
    sprintf(err, "%s: nrrd trouble", me);
    biffMove(MREND, err, NRRD);
    return 1;
  }
  (*rrP)->nout->axis[0].center = nrrdCenterCell;
  (*rrP)->nout->axis[0].min = uu->ctx->cam->uRange[0];
  (*rrP)->nout->axis[0].max = uu->ctx->cam->uRange[1];
  (*rrP)->nout->axis[1].center = nrrdCenterCell;
  (*rrP)->nout->axis[1].min = uu->ctx->cam->vRange[0];
  (*rrP)->nout->axis[1].max = uu->ctx->cam->vRange[1];
  airMopAdd(uu->rmop, (*rrP)->nout, (airMopper)nrrdNuke, airMopAlways);
  (*rrP)->imgData = (*rrP)->nout->data;
  (*rrP)->sx = uu->ctx->imgSize[0];
  (*rrP)->sy = uu->ctx->imgSize[1];
  (*rrP)->numSamples = 0;
  
  return 0;
}

int
mrendRenderEnd(mrendRenderInfo *rr, mrendUserInfo *uu) {
  char me[]="mrendRenderEnd", err[AIR_STRLEN_MED];

  rr->time1 = airTime();
  fprintf(stderr, "\n");
  fprintf(stderr, "%s: rendering time = %g secs\n", me,
	  rr->time1 - rr->time0);
  fprintf(stderr, "%s: sampling rate = %g KHz\n", me,
	  rr->numSamples/(1000.0*(rr->time1 - rr->time0)));
  if (nrrdSave(uu->outS, rr->nout, NULL)) {
    sprintf(err, "%s: trouble saving image", me);
    biffMove(MREND, err, NRRD);
    return 1;
  }

  airMopOkay(uu->rmop);
  return 0;
}

/* -------------------------------------------------------------- */

typedef struct {
  float *val, rayLen;
  int thrid, /* thread ID */
    len,     /* number of samples allocated */
    num,     /* number of samples stored */
    ui, vi,  /* image coords */
    totNum;  /* total number of samples this thread has done */
} mrendThreadInfo;

int
mrendThreadBegin(mrendThreadInfo **ttP,
		 mrendRenderInfo *rr, mrendUserInfo *uu, int whichThread) {
  
  (*ttP) = (mrendThreadInfo *)calloc(1, sizeof(mrendThreadInfo));
  airMopAdd(uu->rmop, *ttP, airFree, airMopAlways);

  (*ttP)->val = NULL;
  (*ttP)->rayLen = 0;
  (*ttP)->thrid = whichThread;
  (*ttP)->len = 0;
  (*ttP)->num = 0;
  (*ttP)->totNum = 0;
  return 0;
}

int
mrendThreadEnd(mrendThreadInfo *tt, mrendRenderInfo *rr, mrendUserInfo *uu) {
  
  tt->val = airFree(tt->val);

  /* HEY: totally not thread-save */
  rr->numSamples += tt->totNum;
  return 0;
}

/* -------------------------------------------------------------- */

int
mrendRayBegin(mrendThreadInfo *tt, mrendRenderInfo *rr, mrendUserInfo *uu,
	      int uIndex,
	      int vIndex,
	      double rayLen,
	      double rayStartWorld[3],
	      double rayStartIndex[3],
	      double rayDirWorld[3],
	      double rayDirIndex[3]) {
  int newLen;

  tt->ui = uIndex;
  tt->vi = vIndex;
  tt->rayLen = rayLen;
  newLen = AIR_ROUNDUP(rayLen/uu->rayStep);
  if (!tt->val || newLen > tt->len) {
    airFree(tt->val);
    tt->len = newLen;
    tt->val = (float*)calloc(newLen, sizeof(float));
  }
  tt->num = 0;
  if (!uIndex) {
    fprintf(stderr, "%d/%d ", vIndex, uu->ctx->imgSize[1]);
    fflush(stderr);
  }
  
  return 0;
}

int
mrendRayEnd(mrendThreadInfo *tt, mrendRenderInfo *rr, mrendUserInfo *uu) {
  float answer;

  if (tt->num) {
    nrrdMeasureLine[uu->measr](&answer,
			       nrrdTypeFloat,
			       tt->val, nrrdTypeFloat,
			       tt->num,
			       0, tt->rayLen);
    /* this silliness is because when using a histo-based
       nrrdMeasure, and if the values where all zero,
       then the answer will probably be NaN */
    answer = AIR_EXISTS(answer) ? answer : 0.0;
    rr->imgData[(tt->ui) + (rr->sx)*(tt->vi)] = answer;
  } else {
    rr->imgData[(tt->ui) + (rr->sx)*(tt->vi)] = 0.0;
  }

  return 0;
}

/* -------------------------------------------------------------- */

double
mrendSample(mrendThreadInfo *tt, mrendRenderInfo *rr, mrendUserInfo *uu,
	    int num, double rayT,
	    int inside,
	    double samplePosWorld[3],
	    double samplePosIndex[3]) {
  char me[]="mrendSample", err[AIR_STRLEN_MED];

  if (inside) {
    if (gageProbe(rr->gtx, samplePosIndex[0], samplePosIndex[1],
		  samplePosIndex[2])) {
      sprintf(err, "%s: gage trouble: %s (%d)", me, gageErrStr, gageErrNum);
      biffAdd(MREND, err);
      return AIR_NAN;
    }
    tt->val[tt->num++] = *(rr->answer);
    tt->totNum++;
  }
  
  return uu->rayStep;
}

/* -------------------------------------------------------------- */

int
main(int argc, char *argv[]) {
  hestOpt *hopt=NULL;
  hestParm *hparm;
  hoovContext *ctx;
  int E, Ecode;
  char *me, *errS;
  mrendUserInfo *uu;
  airArray *mop;
  
  me = argv[0];
  mop = airMopInit();
  ctx = hoovContextNew();
  hparm = hestParmNew();
  hparm->respFileEnable = AIR_TRUE;
  uu = mrendUserInfoNew(ctx);

  airMopAdd(mop, ctx, (airMopper)hoovContextNix, airMopAlways);
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  airMopAdd(mop, uu, airFree, airMopAlways);

  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &(uu->nin), NULL,
	     "input nrrd to render", NULL, NULL, nrrdHestNrrd);
  limnHestCamOptAdd(&hopt, ctx->cam,
		    NULL, "0 0 0", "0 0 1",
		    NULL, NULL, NULL,
		    NULL, NULL);
  hestOptAdd(&hopt, "is", "image size", airTypeInt, 2, 2, ctx->imgSize,
	     "256 256", "image dimensions");
  hestOptAdd(&hopt, "k00", "kernel", airTypeOther, 1, 1, &(uu->ksp00),
	     "tent", "value reconstruction kernel",
	     NULL, NULL, nrrdHestNrrdKernelSpec);
  hestOptAdd(&hopt, "k11", "kernel", airTypeOther, 1, 1, &(uu->ksp11),
	     "fordif", "first derivative kernel",
	     NULL, NULL, nrrdHestNrrdKernelSpec);
  hestOptAdd(&hopt, "k22", "kernel", airTypeOther, 1, 1, &(uu->ksp22),
	     "fordif",
	     "second derivative kernel (if needed, DON'T use default)",
	     NULL, NULL, nrrdHestNrrdKernelSpec);
  hestOptAdd(&hopt, "rn", NULL, airTypeBool, 0, 0, &(uu->renorm), NULL,
	     "renormalize kernel weights at each new sample location. "
	     "\"Accurate\" kernels don't need this; doing it always "
	     "makes things go slower");
  hestOptAdd(&hopt, "q", "quantity", airTypeEnum, 1, 1, &(uu->whatq), NULL,
	     "the quantity to measure at sample points along rays",
	     NULL, gageKindScl->enm);
  hestOptAdd(&hopt, "m", "measure", airTypeEnum, 1, 1, &(uu->measr), NULL,
	     "how to collapse ray samples into one scalar",
	     NULL, nrrdMeasure);
  hestOptAdd(&hopt, "step", "size", airTypeDouble, 1, 1, &(uu->rayStep),
	     "0.01", "step size along ray in world space");
  hestOptAdd(&hopt, "o", "filename", airTypeString, 1, 1, &(uu->outS),
	     NULL, "file to write output nrrd to");
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
		 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  /* other command-line option actions and checks */
  if (3 != uu->nin->dim) {
    fprintf(stderr, "%s: input nrrd needs 3 dimensions, not %d\n", 
	    me, uu->nin->dim);
    airMopError(mop);
    return 1;
  }
  if (1 != gageKindScl->ansLength[uu->whatq]) {
    fprintf(stderr, "%s: quantity %s isn't a scalar; can't render it\n",
	    me, airEnumStr(gageKindScl->enm, uu->whatq));
    airMopError(mop);
    return 1;
  }
  
  fprintf(stderr, "%s: will render %s of %s\n", me,
	  airEnumStr(nrrdMeasure, uu->measr),
	  airEnumStr(gageKindScl->enm, uu->whatq));
  
  /* set remaining fields of hoover context */
  nrrdAxesGet_nva(uu->nin, nrrdAxesInfoSize, ctx->volSize);
  nrrdAxesGet_nva(uu->nin, nrrdAxesInfoSpacing, ctx->volSpacing);
  ctx->numThreads = 1;
  /* HEY: until gageContextCopy is implemented, we can only do 
     a single thread, since multiple threads would each have their
     own padded copy of the volume */
  ctx->userInfo = uu;
  ctx->renderBegin = (hoovRenderBegin_t *)mrendRenderBegin;
  ctx->threadBegin = (hoovThreadBegin_t *)mrendThreadBegin;
  ctx->rayBegin = (hoovRayBegin_t *)mrendRayBegin;
  ctx->sample = (hoovSample_t *)mrendSample;
  ctx->rayEnd = (hoovRayEnd_t *)mrendRayEnd;
  ctx->threadEnd = (hoovThreadEnd_t *)mrendThreadEnd;
  ctx->renderEnd = (hoovRenderEnd_t *)mrendRenderEnd;

  E = hoovRender(ctx, &Ecode, NULL);
  if (E) {
    if (hoovErrInit == E) {
      fprintf(stderr, "%s: ERROR:\n%s\n",
	      me, errS = biffGetDone(HOOVER)); free(errS);
    } else {
      fprintf(stderr, "%s: ERROR:\n%s\n",
	      me, errS = biffGetDone(MREND)); free(errS);
    }
    airMopError(mop);
    return 1;
  }
  
  airMopOkay(mop);
  return 0;
}

