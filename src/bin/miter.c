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
#include <biff.h>
#include <ell.h>
#include <nrrd.h>
#include <limn.h>
#include <hoover.h>
#include <mite.h>

char *miteInfo = ("A simple but effective little volume renderer.");

int
main(int argc, char *argv[]) {
  airArray *mop;
  hestOpt *hopt=NULL;
  hestParm *hparm=NULL;
  hooverContext *ctx;
  miteUserInfo *muu;
  char *me, *errS;
  int E, Ecode;

  me = argv[0];
  mop = airMopInit();
  ctx = hooverContextNew();
  fprintf(stderr, "%s: hoover ctx = %p\n", me, ctx);
  airMopAdd(mop, ctx, (airMopper)hooverContextNix, airMopAlways);
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  fprintf(stderr, "%s: hoover ctx = %p\n", me, ctx);
  muu = miteUserInfoNew(ctx);
  fprintf(stderr, "%s: hoover muu->ctx = %p\n", me, muu->ctx);
  fprintf(stderr, "%s: muu = %p, muu->ctx = %p\n", me, muu, muu->ctx);
  airMopAdd(mop, muu, (airMopper)miteUserInfoNix, airMopAlways);
  
  hparm->respFileEnable = AIR_TRUE;
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &(muu->nin), NULL,
	     "input nrrd to render", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "tf", "nin", airTypeOther, 1, 1, &(muu->ntf), NULL,
	     "nrrd containing transfer function", NULL, NULL, nrrdHestNrrd);
  limnHestCamOptAdd(&hopt, ctx->cam,
		    NULL, "0 0 0", "0 0 1",
		    NULL, NULL, NULL,
		    NULL, NULL);
  hestOptAdd(&hopt, "am", "ambient", airTypeFloat, 3, 3, muu->lit->amb,
	     "0 0 0", "ambient light");
  hestOptAdd(&hopt, "ld", "light pos", airTypeFloat, 3, 3, muu->lit->_dir[0],
	     "0 0 1", "light position (extended to infinity)");
  hestOptAdd(&hopt, "is", "image size", airTypeInt, 2, 2, ctx->imgSize,
	     "256 256", "image dimensions");
  hestOptAdd(&hopt, "k00", "kernel", airTypeOther, 1, 1, &(muu->ksp00),
	     "tent", "value reconstruction kernel",
	     NULL, NULL, nrrdHestNrrdKernelSpec);
  hestOptAdd(&hopt, "k11", "kernel", airTypeOther, 1, 1, &(muu->ksp11),
	     "fordif", "first derivative kernel",
	     NULL, NULL, nrrdHestNrrdKernelSpec);
  hestOptAdd(&hopt, "k22", "kernel", airTypeOther, 1, 1, &(muu->ksp22),
	     "fordif",
	     "second derivative kernel (if needed, DON'T use default)",
	     NULL, NULL, nrrdHestNrrdKernelSpec);
  hestOptAdd(&hopt, "rn", NULL, airTypeBool, 0, 0, &(muu->renorm), NULL,
	     "renormalize kernel weights at each new sample location. "
	     "\"Accurate\" kernels don't need this; doing it always "
	     "makes things go slower");
  hestOptAdd(&hopt, "sum", NULL, airTypeBool, 0, 0, &(muu->sum), NULL,
	     "Ignore opacity and composite simply by summing.");
  hestOptAdd(&hopt, "step", "size", airTypeDouble, 1, 1, &(muu->rayStep),
	     "0.01", "step size along ray in world space");
  hestOptAdd(&hopt, "n1", "near1", airTypeDouble, 1, 1, &(muu->near1),
	     "0.99", "close enough to 1.0 to terminate ray");
  hestOptAdd(&hopt, "o", "filename", airTypeString, 1, 1, &(muu->outS),
	     NULL, "file to write output nrrd to");
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
		 me, miteInfo, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  /* other command-line option actions and checks */
  if (3 != muu->nin->dim) {
    fprintf(stderr, "%s: input nrrd needs 3 dimensions, not %d\n", 
	    me, muu->nin->dim);
    airMopError(mop);
    return 1;
  }
  ELL_3V_SET(muu->lit->col[0], 1, 1, 1);
  muu->lit->on[0] = AIR_TRUE;
  muu->lit->vsp[0] = AIR_TRUE;
  fprintf(stderr, "%s: muu = %p, muu->ctx->cam = %p\n",
	  me, muu, muu->ctx->cam);
  limnCamUpdate(muu->ctx->cam);
  limnLightUpdate(muu->lit, muu->ctx->cam);
  fprintf(stderr, "%s: light dir: %g %g %g\n", me,
	  muu->lit->dir[0][0], muu->lit->dir[0][1], muu->lit->dir[0][2]);

  nrrdAxesGet_nva(muu->nin, nrrdAxesInfoSize, ctx->volSize);
  nrrdAxesGet_nva(muu->nin, nrrdAxesInfoSpacing, ctx->volSpacing);
  ctx->numThreads = 1;
  /* HEY: until gageSimpleCopy is implemented, we can only do 
     a single thread, since multiple threads would each have their
     own padded copy of the volume */
  ctx->userInfo = muu;
  ctx->renderBegin = (hooverRenderBegin_t *)miteRenderBegin;
  ctx->threadBegin = (hooverThreadBegin_t *)miteThreadBegin;
  ctx->rayBegin = (hooverRayBegin_t *)miteRayBegin;
  ctx->sample = (hooverSample_t *)miteSample;
  ctx->rayEnd = (hooverRayEnd_t *)miteRayEnd;
  ctx->threadEnd = (hooverThreadEnd_t *)miteThreadEnd;
  ctx->renderEnd = (hooverRenderEnd_t *)miteRenderEnd;

  fprintf(stderr, "%s: rendering ... ", me); fflush(stderr);
  E = hooverRender(ctx, &Ecode, NULL);
  if (E) {
    if (hooverErrInit == E) {
      fprintf(stderr, "%s: ERROR:\n%s\n",
	      me, errS = biffGetDone(HOOVER)); free(errS);
    } else {
      fprintf(stderr, "%s: ERROR:\n%s\n",
	      me, errS = biffGetDone(MITE)); free(errS);
    }
    airMopError(mop);
    return 1;
  }
  
  airMopOkay(mop);

  return 0;
}
