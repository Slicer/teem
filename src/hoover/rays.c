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

#include "hoover.h"
/* #include "hoovThreads.h" */

/*
** learned: if you're going to "simplify" code which computes some
** floating point value within a loop using AFFINE() on the loop
** control variable, by simply incrementing that value with the
** correct amount iteration, BE SURE THAT THE INCREMENTING IS DONE in
** every possible control path of the loop (wasn't incrementing ray
** sample position if first sample wasn't inside the volume)
*/

/*
** _hoovLearnLengths()
**
** This is where we enforce the constraint that the volume always fit
** inside a cube with edge length 2, centered at the origin.
**
** volHLen[i] is the HALF the length of the volume along axis i
*/
void
_hoovLearnLengths(double volHLen[3], double voxLen[3], hoovContext *ctx) {
  double maxLen;
  int numSamples[3];

  ELL_3V_COPY(numSamples, ctx->volSize);
  volHLen[0] = (numSamples[0]-1)*ctx->volSpacing[0];
  volHLen[1] = (numSamples[1]-1)*ctx->volSpacing[1];
  volHLen[2] = (numSamples[2]-1)*ctx->volSpacing[2];
  maxLen = AIR_MAX(volHLen[0], volHLen[1]);
  maxLen = AIR_MAX(volHLen[2], maxLen);
  volHLen[0] /= maxLen;
  volHLen[1] /= maxLen;
  volHLen[2] /= maxLen;
  voxLen[0] = 2*volHLen[0]/(numSamples[0]-1);
  voxLen[1] = 2*volHLen[1]/(numSamples[1]-1);
  voxLen[2] = 2*volHLen[2]/(numSamples[2]-1);
}

/*
** _hoovExtraContext struct
**
** Like hoovContext, this is READ-ONLY information which is not specific
** to any thread.
** Unlike hoovContext, it is solely for the benefit of the calculations
** done in _hoovThreadBody.
**
** No one outside hoover should need to know about this.
*/
typedef struct {
  double volHLen[3],     /* length of x,y,z edges of volume bounding box */
    voxLen[3],           /* length of x,y,z edges of voxels */
    uBase, uCap,         /* uMin and uMax as seen on the near cutting plane */
    vBase, vCap,         /* analogous to uBase and uCap */
    rayZero[3];          /* location of near plane, line of sight interxion */
} _hoovExtraContext;

_hoovExtraContext *
_hoovExtraContextNew(hoovContext *ctx) {
  _hoovExtraContext *ec;
  
  ec = calloc(1, sizeof(_hoovExtraContext));
  if (ec) {
    _hoovLearnLengths(ec->volHLen, ec->voxLen, ctx);
    ELL_3V_SCALEADD(ec->rayZero,
		    1.0, ctx->cam->from,
		    ctx->cam->vspNeer, ctx->cam->N);
  }
  return ec;
}

_hoovExtraContext *
_hoovExtraContextNix(_hoovExtraContext *ec) {
  
  if (ec) {
    free(ec);
  }
  return NULL;
}

/*
** _hoovThreadArg struct
**
** A pointer to this is passed to _hoovThreadBody.  It contains all the
** information which is not thread-specific, and all the thread-specific
** information known at the level of hoovRender.
**
** For simplicity sake, a pointer to a struct of this type is also
** returned from _hoovThreadBody, so this is where we store an
** error-signaling return value (errCode), and what function had
** trouble (whichErr).
*/
typedef struct {
  /* ----------------------- intput */
  hoovContext *ctx;
  _hoovExtraContext *ec;
  void *renderInfo;
  int whichThread;
  /* ----------------------- output */
  int whichErr;
  int errCode;
} _hoovThreadArg;

void *
_hoovThreadBody(void *_arg) {
  _hoovThreadArg *arg;
  void *threadInfo;
  int ret,               /* to catch return values from callbacks */
    sampleI,             /* which sample we're on */
    inside,              /* we're inside the volume */
    mx, my, mz,          /* highest index on each axis */
    vI, uI;              /* integral coords in image */
  double tmp,
    u, v,                /* floating-point coords in image */
    uvScale,             /* how to scale (u,v) to go from image to 
			    near plane, according to ortho or perspective */
    lx, ly, lz,          /* half edge-lengths of volume */
    rayLen=0,            /* length of segment formed by ray line intersecting
			    the near and far clipping planes */
    rayT,                /* current position along ray (world-space) */
    rayDirW[3],          /* unit-length ray direction (world-space) */
    rayDirI[3],          /* rayDirW transformed into index space;
			    not unit length, but a unit change in
			    world space along rayDirW translates to
			    this change in index space along rayDirI */
    rayPosW[3],          /* current ray location (world-space) */
    rayPosI[3],          /* current ray location (index-space) */
    rayStartW[3],        /* ray start on near plane (world-space) */
    rayStartI[3],        /* ray start on near plane (index-space) */
    rayStep,             /* distance between samples (world-space) */
    vOff[3], uOff[3];    /* offsets in arg->ec->wU and arg->ec->wV
			    directions towards start of ray */

  arg = (_hoovThreadArg *)_arg;
  if ( (ret = (arg->ctx->threadBegin)(&threadInfo, 
				      arg->renderInfo, 
				      arg->ctx->userInfo,
				      arg->whichThread)) ) {
    arg->errCode = ret;
    arg->whichErr = hoovErrThreadBegin;
    return arg;
  }
  lx = arg->ec->volHLen[0];
  ly = arg->ec->volHLen[1];
  lz = arg->ec->volHLen[2];
  mx = arg->ctx->volSize[0]-1;
  my = arg->ctx->volSize[1]-1;
  mz = arg->ctx->volSize[2]-1;
  
  if (arg->ctx->cam->ortho) {
    ELL_3V_COPY(rayDirW, arg->ctx->cam->N);
    rayDirI[0] = AIR_DELTA(-lx, rayDirW[0], lx, 0, mx);
    rayDirI[1] = AIR_DELTA(-ly, rayDirW[1], ly, 0, my);
    rayDirI[2] = AIR_DELTA(-lz, rayDirW[2], lz, 0, mz);
    rayLen = arg->ctx->cam->vspFaar - arg->ctx->cam->vspNeer;
    uvScale = 1.0;
  } else {
    uvScale = arg->ctx->cam->vspNeer/arg->ctx->cam->vspDist;
  }

  /* for now, the load-balancing among P processors is simplistic: the
     Nth thread (0-based numbering) gets scanlines N, N+P, N+2P, N+3P,
     etc., until it goes beyond the last scanline */
  vI = arg->whichThread;
  while (vI < arg->ctx->imgSize[1]) {
    v = uvScale*AIR_AFFINE(-0.5, vI, arg->ctx->imgSize[1]-0.5,
			   arg->ctx->cam->vRange[0],
			   arg->ctx->cam->vRange[1]);
    ELL_3V_SCALE(vOff, v, arg->ctx->cam->V);
    for (uI=0; uI<arg->ctx->imgSize[0]; uI++) {
      u = uvScale*AIR_AFFINE(-0.5, uI, arg->ctx->imgSize[0]-0.5,
			     arg->ctx->cam->uRange[0],
			     arg->ctx->cam->uRange[1]);
      ELL_3V_SCALE(uOff, u, arg->ctx->cam->U);
      ELL_3V_ADD3(rayStartW, uOff, vOff, arg->ec->rayZero);
      rayStartI[0] = AIR_AFFINE(-lx, rayStartW[0], lx, 0, mx);
      rayStartI[1] = AIR_AFFINE(-ly, rayStartW[1], ly, 0, my);
      rayStartI[2] = AIR_AFFINE(-lz, rayStartW[2], lz, 0, mz);
      if (!arg->ctx->cam->ortho) {
	ELL_3V_SUB(rayDirW, rayStartW, arg->ctx->cam->from);
	ELL_3V_NORM(rayDirW, rayDirW, tmp);
	rayDirI[0] = AIR_DELTA(-lx, rayDirW[0], lx, 0, mx);
	rayDirI[1] = AIR_DELTA(-ly, rayDirW[1], ly, 0, my);
	rayDirI[2] = AIR_DELTA(-lz, rayDirW[2], lz, 0, mz);
	rayLen = ((arg->ctx->cam->vspFaar - arg->ctx->cam->vspNeer)/
		  ELL_3V_DOT(rayDirW, arg->ctx->cam->N));
      }
      if ( (ret = (arg->ctx->rayBegin)(threadInfo,
				       arg->renderInfo,
				       arg->ctx->userInfo,
				       uI, vI, rayLen,
				       rayStartW, rayStartI,
				       rayDirW, rayDirI)) ) {
	arg->errCode = ret;
	arg->whichErr = hoovErrRayBegin;
	return arg;
      }
      
      sampleI = 0;
      rayT = 0;
      while (1) {
	ELL_3V_SCALEADD(rayPosW, 1.0, rayStartW, rayT, rayDirW);
	ELL_3V_SCALEADD(rayPosI, 1.0, rayStartI, rayT, rayDirI);
	inside = (AIR_INSIDE(0, rayPosI[0], mx) &&
		  AIR_INSIDE(0, rayPosI[1], my) &&
		  AIR_INSIDE(0, rayPosI[2], mz));
	rayStep = (arg->ctx->sample)(threadInfo,
				     arg->renderInfo,
				     arg->ctx->userInfo,
				     sampleI, rayT,
				     inside,
				     rayPosW, rayPosI);
	if (!AIR_EXISTS(rayStep)) {
	  /* sampling failed */
	  arg->errCode = 0;
	  arg->whichErr = hoovErrSample;
	  return arg;
	}
	if (!rayStep) {
	  /* ray decided to finish itself */
	  break;
	} 
	/* else we moved to a new location along the ray */
	rayT += rayStep;
	if (!AIR_INSIDE(0, rayT, rayLen)) {
	  /* ray stepped outside near-far clipping region, its done. */
	  break;
	}
	sampleI++;
      }
      
      if ( (ret = (arg->ctx->rayEnd)(threadInfo,
				     arg->renderInfo,
				     arg->ctx->userInfo)) ) {
	arg->errCode = ret;
	arg->whichErr = hoovErrRayEnd;
	return arg;
      }
    }  /* end this scanline */
    vI += arg->ctx->numThreads;
  } /* end skipping through scanlines */

  if ( (ret = (arg->ctx->threadEnd)(threadInfo,
				    arg->renderInfo,
				    arg->ctx->userInfo)) ) {
    arg->errCode = ret;
    arg->whichErr = hoovErrThreadEnd;
    return arg;
  }
  
  /* returning NULL actually indicates that there was NOT an error */
  return NULL;
}

/*
******** hoovRender()
**
** because of the biff usage(), only one thread can call hoovRender(),
** and no promises if the threads themselves call biff...
*/
int
hoovRender(hoovContext *ctx, int *errCodeP, int *errThreadP) {
  char me[]="hoovRender", err[AIR_STRLEN_MED];
  _hoovExtraContext *ec;
  _hoovThreadArg args[HOOV_THREAD_MAX];
  _hoovThreadArg *errArg;
  void *renderInfo;
  int ret;
  airArray *mop;

  /* this calls limnCamUpdate() */
  if (hoovContextCheck(ctx)) {
    sprintf(err, "%s: problem detected in given context", me);
    biffAdd(HOOVER, err);
    return hoovErrInit;
  }

  if (!(ec = _hoovExtraContextNew(ctx))) {
    sprintf(err, "%s: problem creating thread context", me);
    biffAdd(HOOVER, err);
    return hoovErrInit;
  }
  mop = airMopInit();
  airMopAdd(mop, ec, (airMopper)_hoovExtraContextNix, airMopAlways);
  if ( (ret = (ctx->renderBegin)(&renderInfo, ctx->userInfo)) ) {
    *errCodeP = ret;
    airMopError(mop);
    return hoovErrRenderBegin;
  }

  if (1 == ctx->numThreads) {
    args[0].ctx = ctx;
    args[0].ec = ec;
    args[0].renderInfo = renderInfo;
    args[0].whichThread = 0;
    args[0].whichErr = hoovErrNone;
    args[0].errCode = 0;
    errArg = _hoovThreadBody(&(args[0]));
    if (errArg) {
      *errCodeP = errArg->errCode;
      airMopError(mop);
      return errArg->whichErr;
    }
  }
  else {
    sprintf(err, "%s: sorry, multi-threading under construction", me);
    biffAdd(HOOVER, err); return hoovErrInit;
    /* TODO: call pthread_create() once per thread, passing the
       address of a distinct (and appropriately intialized)
       _hoovThreadArg to each.  If return of pthread_create() is
       non-zero, put its return in *errCodeP, the number of the
       problematic in *errThreadP, and return hoovErrCreateThread.
       Then call pthread_join() on all the threads, passing &errArg as
       "retval". On non-zero return, set *errCodeP and *errThreadP,
       and return hoovErrJoinThread. If return of pthread_join() is
       zero, but the errArg is non-NULL, then assume that this errArg
       is actually just the passed _hoovThreadArg returned to us, and
       from this copy errArg->errCode into *errCodeP, and return
       errArg->whichErr */
  }

  if ( (ret = (ctx->renderEnd)(renderInfo, ctx->userInfo)) ) {
    *errCodeP = ret;
    return hoovErrRenderEnd;
  }
  renderInfo = NULL;
  airMopOkay(mop);

  return hoovErrNone;
}
