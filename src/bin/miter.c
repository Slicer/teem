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

#include <teem/air.h>
#include <teem/biff.h>
#include <teem/ell.h>
#include <teem/nrrd.h>
#include <teem/limn.h>
#include <teem/hoover.h>
#include <teem/mite.h>

char *miteInfo = ("A simple but effective little volume renderer.");

int
main(int argc, char *argv[]) {
  airArray *mop;
  hestOpt *hopt=NULL;
  hestParm *hparm=NULL;
  miteUser *muu;
  char *me, *errS, *outS, *shadeStr, *normalStr;
  int renorm, baseDim, verbPix[2];
  int E, Ecode;
  float ads[3];
  double gmc;
  Nrrd *nin;

  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  muu = miteUserNew();
  airMopAdd(mop, muu, (airMopper)miteUserNix, airMopAlways);
  
  hparm->respFileEnable = AIR_TRUE;
  hestOptAdd(&hopt, "i", "nsin", airTypeOther, 1, 1, &(muu->nsin), "",
	     "input scalar volume to render", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "vi", "nvin", airTypeOther, 1, 1, &(muu->nvin), "",
	     "input vector volume to render", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "ti", "ntin", airTypeOther, 1, 1, &(muu->ntin), "",
	     "input tensor volume to render", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "txf", "nin", airTypeOther, 1, -1, &(muu->ntxf), NULL,
	     "one or more transfer functions",
	     &(muu->ntxfNum), NULL, nrrdHestNrrd);
  limnHestCameraOptAdd(&hopt, muu->hctx->cam,
		       NULL, "0 0 0", "0 0 1",
		       NULL, NULL, NULL,
		       "-1 1", "-1 1");
  hestOptAdd(&hopt, "am", "ambient", airTypeFloat, 3, 3, muu->lit->amb,
	     "1 1 1", "ambient light color");
  hestOptAdd(&hopt, "ld", "light pos", airTypeFloat, 3, 3, muu->lit->_dir[0],
	     "0 0 -1", "view space light position (extended to infinity)");
  hestOptAdd(&hopt, "is", "image size", airTypeInt, 2, 2, muu->hctx->imgSize,
	     "256 256", "image dimensions");
  hestOptAdd(&hopt, "ads", "ka kd ks", airTypeFloat, 3, 3, ads,
	     "0.1 0.6 0.3", "phong components");
  hestOptAdd(&hopt, "sp", "spec pow", mite_at, 1, 1, 
	     &(muu->rangeInit[miteRangeSP]), "30", "phong specular power");
  hestOptAdd(&hopt, "k00", "kernel", airTypeOther, 1, 1,
	     &(muu->ksp[gageKernel00]),
	     "tent", "value reconstruction kernel",
	     NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k11", "kernel", airTypeOther, 1, 1,
	     &(muu->ksp[gageKernel11]),
	     "cubicd:1,0", "first derivative kernel",
	     NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k22", "kernel", airTypeOther, 1, 1,
	     &(muu->ksp[gageKernel22]),
	     "cubicdd:1,0",  "second derivative kernel",
	     NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "ss", "shading spec", airTypeString, 1, 1, &shadeStr,
	     "phong:gage(scalar:n)", "how to do shading");
  hestOptAdd(&hopt, "ns", "normal spec", airTypeString, 1, 1, &normalStr,
	     "", "\"normal\" to use for those miteVal's that need one");
  hestOptAdd(&hopt, "side", "normal side", airTypeInt, 1, 1,
	     &(muu->normalSide),
	     "1", "how to interpret gradients as normals:\n "
	     "\b\bo \"1\": normal points to lower values (higher == "
	     "more \"inside\")\n "
	     "\b\bo \"0\": \"two-sided\": dot-products are abs()'d\n "
	     "\b\bo \"-1\": normal points to higher values (lower == "
	     "more \"inside\")");
  hestOptAdd(&hopt, "rn", NULL, airTypeBool, 0, 0, &renorm, NULL,
	     "renormalize kernel weights at each new sample location. "
	     "\"Accurate\" kernels don't need this; doing it always "
	     "makes things go slower");
  hestOptAdd(&hopt, "gmc", "min gradmag", airTypeDouble, 1, 1, &gmc, "0.0",
	     "For curvature-based transfer functions, set curvature to "
	     "zero when gradient magnitude is below this");
  hestOptAdd(&hopt, "step", "size", airTypeDouble, 1, 1, &(muu->rayStep),
	     "0.01", "step size along ray in world space");
  hestOptAdd(&hopt, "ref", "size", airTypeDouble, 1, 1, &(muu->refStep),
	     "0.01", "\"reference\" step size (world space) for doing "
	     "opacity correction in compositing");
  hestOptAdd(&hopt, "vp", "verbose pixel", airTypeInt, 2, 2, verbPix,
	     "-1 -1", "pixel for which to turn on verbose messages");
  hestOptAdd(&hopt, "n1", "near1", airTypeDouble, 1, 1, &(muu->opacNear1),
	     "0.99", "opacity close enough to 1.0 to terminate ray");
  hestOptAdd(&hopt, "nt", "# threads", airTypeInt, 1, 1,
	     &(muu->hctx->numThreads), "1", 
	     (airThreadCapable
	      ? "number of threads hoover should use"
	      : "if pthreads where enabled in this teem build, this is how "
	      "you would control the number of threads hoover should use"));
  hestOptAdd(&hopt, "o", "filename", airTypeString, 1, 1, &outS,
	     NULL, "file to write output nrrd to");
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
		 me, miteInfo, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (muu->nsin) {
    nin = muu->nsin;
    baseDim = 0;
  } else if (muu->nvin) {
    nin = muu->nvin;
    baseDim = 1;
  } else if (muu->ntin) {
    nin = muu->ntin;
    baseDim = 1;
  } else {
    fprintf(stderr, "%s: didn't get any volumes to render!\n", me);
    airMopError(mop);
    return 1;
  }

  /* finish processing command-line args */
  muu->rangeInit[miteRangeKa] = ads[0];
  muu->rangeInit[miteRangeKd] = ads[1];
  muu->rangeInit[miteRangeKs] = ads[2];
  gageParmSet(muu->gctx0, gageParmGradMagCurvMin, gmc);
  gageParmSet(muu->gctx0, gageParmRenormalize, renorm);
  muu->verbUi = verbPix[0];
  muu->verbVi = verbPix[1];

  muu->nout = nrrdNew();  
  airMopAdd(mop, muu->nout, (airMopper)nrrdNuke, airMopAlways);
  ELL_3V_SET(muu->lit->col[0], 1, 1, 1);
  muu->lit->on[0] = AIR_TRUE;
  muu->lit->vsp[0] = AIR_TRUE;
  if (limnCameraUpdate(muu->hctx->cam)
      || limnLightUpdate(muu->lit, muu->hctx->cam)) {
    airMopAdd(mop, errS = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble setting camera:\n%s\n", me, errS);
    airMopError(mop);
    return 1;
  }
  strncpy(muu->shadeStr, shadeStr, AIR_STRLEN_MED-1);
  strncpy(muu->normalStr, normalStr, AIR_STRLEN_MED-1);
  muu->shadeStr[AIR_STRLEN_MED-1] = 0;
  muu->hctx->volSize[0] = nin->axis[baseDim+0].size;
  muu->hctx->volSize[1] = nin->axis[baseDim+1].size;
  muu->hctx->volSize[2] = nin->axis[baseDim+2].size;
  muu->hctx->volSpacing[0] = nin->axis[baseDim+0].spacing;
  muu->hctx->volSpacing[1] = nin->axis[baseDim+1].spacing;
  muu->hctx->volSpacing[2] = nin->axis[baseDim+2].spacing;
  muu->hctx->user = muu;
  muu->hctx->renderBegin = (hooverRenderBegin_t *)miteRenderBegin;
  muu->hctx->threadBegin = (hooverThreadBegin_t *)miteThreadBegin;
  muu->hctx->rayBegin = (hooverRayBegin_t *)miteRayBegin;
  muu->hctx->sample = (hooverSample_t *)miteSample;
  muu->hctx->rayEnd = (hooverRayEnd_t *)miteRayEnd;
  muu->hctx->threadEnd = (hooverThreadEnd_t *)miteThreadEnd;
  muu->hctx->renderEnd = (hooverRenderEnd_t *)miteRenderEnd;

  if (!airThreadCapable && 1 != muu->hctx->numThreads) {
    fprintf(stderr, "%s: This teem not compiled with "
	    "multi-threading support.\n", me);
    fprintf(stderr, "%s: ==> can't use %d threads; only using 1\n",
	    me, muu->hctx->numThreads);
    muu->hctx->numThreads = 1;
  }

  fprintf(stderr, "%s: rendering ... ", me); fflush(stderr);

  E = hooverRender(muu->hctx, &Ecode, NULL);
  if (E) {
    if (hooverErrInit == E) {
      airMopAdd(mop, errS = biffGetDone(HOOVER), airFree, airMopAlways);
      fprintf(stderr, "%s: ERROR:\n%s\n", me, errS);
    } else {
      airMopAdd(mop, errS = biffGetDone(MITE), airFree, airMopAlways);
      fprintf(stderr, "%s: ERROR:\n%s\n", me, errS);
    }
    airMopError(mop);
    return 1;
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "%s: rendering time = %g secs\n", me, muu->rendTime);
  fprintf(stderr, "%s: sampling rate = %g Khz\n", me, muu->sampRate);

  if (nrrdSave(outS, muu->nout, NULL)) {
    airMopAdd(mop, errS = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble saving image:\n%s\n", me, errS);
    airMopError(mop);
    return 1;
  }
  
  airMopOkay(mop);
  return 0;
}
