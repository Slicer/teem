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
#if TEEM_PTHREAD
#include <pthread.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#if TEEM_PTHREAD || defined(_WIN32)
const int hooverMyPthread = 1;
#else
const int hooverMyPthread = 0;
#endif

/*
** learned: if you're going to "simplify" code which computes some
** floating point value within a loop using AFFINE() on the loop
** control variable, by simply incrementing that value with the
** correct amount iteration, BE SURE THAT THE INCREMENTING IS DONE in
** every possible control path of the loop (wasn't incrementing ray
** sample position if first sample wasn't inside the volume)
*/

/*
** _hooverLearnLengths()
**
** This is where we enforce the constraint that the volume always fit
** inside a cube with edge length 2, centered at the origin.
**
** volHLen[i] is the HALF the length of the volume along axis i
*/
void
_hooverLearnLengths(double volHLen[3], double voxLen[3], hooverContext *ctx) {
  double maxLen;
  int numSamples[3], numElements[3];

  ELL_3V_COPY(numSamples, ctx->volSize);
  if (nrrdCenterNode == ctx->volCentering) {
    numElements[0] = numSamples[0]-1;
    numElements[1] = numSamples[1]-1;
    numElements[2] = numSamples[2]-1;
  } else {
    numElements[0] = numSamples[0];
    numElements[1] = numSamples[1];
    numElements[2] = numSamples[2];
  }
  volHLen[0] = numElements[0]*ctx->volSpacing[0];
  volHLen[1] = numElements[1]*ctx->volSpacing[1];
  volHLen[2] = numElements[2]*ctx->volSpacing[2];
  maxLen = AIR_MAX(volHLen[0], volHLen[1]);
  maxLen = AIR_MAX(volHLen[2], maxLen);
  volHLen[0] /= maxLen;
  volHLen[1] /= maxLen;
  volHLen[2] /= maxLen;
  voxLen[0] = 2*volHLen[0]/numElements[0];
  voxLen[1] = 2*volHLen[1]/numElements[1];
  voxLen[2] = 2*volHLen[2]/numElements[2];
}

/*
** _hooverExtraContext struct
**
** Like hooverContext, this is READ-ONLY information which is not specific
** to any thread.
** Unlike hooverContext, it is solely for the benefit of the calculations
** done in _hooverThreadBody.
**
** No one outside hoover should need to know about this.
*/
typedef struct {
  double volHLen[3],     /* length of x,y,z edges of volume bounding box */
    voxLen[3],           /* length of x,y,z edges of voxels */
    uBase, uCap,         /* uMin and uMax as seen on the near cutting plane */
    vBase, vCap,         /* analogous to uBase and uCap */
    rayZero[3];          /* location of near plane, line of sight interxion */
} _hooverExtraContext;

_hooverExtraContext *
_hooverExtraContextNew(hooverContext *ctx) {
  _hooverExtraContext *ec;
  
  ec = calloc(1, sizeof(_hooverExtraContext));
  if (ec) {
    _hooverLearnLengths(ec->volHLen, ec->voxLen, ctx);
    ELL_3V_SCALEADD(ec->rayZero,
		    1.0, ctx->cam->from,
		    ctx->cam->vspNeer, ctx->cam->N);
  }
  return ec;
}

_hooverExtraContext *
_hooverExtraContextNix(_hooverExtraContext *ec) {
  
  if (ec) {
    free(ec);
  }
  return NULL;
}

/*
** _hooverThreadArg struct
**
** A pointer to this is passed to _hooverThreadBody.  It contains all the
** information which is not thread-specific, and all the thread-specific
** information known at the level of hooverRender.
**
** For simplicity sake, a pointer to a struct of this type is also
** returned from _hooverThreadBody, so this is where we store an
** error-signaling return value (errCode), and what function had
** trouble (whichErr).
*/
typedef struct {
  /* ----------------------- intput */
  hooverContext *ctx;
  _hooverExtraContext *ec;
  void *render;
  int whichThread;
  /* ----------------------- output */
  int whichErr;
  int errCode;
} _hooverThreadArg;

#if !TEEM_PTHREAD && defined(_WIN32)
int WINAPI
#else
void *
#endif
_hooverThreadBody(void *_arg) {
  _hooverThreadArg *arg;
  void *thread;
  int ret,               /* to catch return values from callbacks */
    sampleI,             /* which sample we're on */
    inside,              /* we're inside the volume */
    vI, uI;              /* integral coords in image */
  double tmp,
    mm,                  /* lowest position in index space, for all axes */
    Mx, My, Mz,          /* highest position in index space on each axis */
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

  arg = (_hooverThreadArg *)_arg;
  if ( (ret = (arg->ctx->threadBegin)(&thread, 
				      arg->render, 
				      arg->ctx->user,
				      arg->whichThread)) ) {
    arg->errCode = ret;
    arg->whichErr = hooverErrThreadBegin;
#if !TEEM_PTHREAD && defined(_WIN32)
    return ret;
#else
    return arg;
#endif
  }
  lx = arg->ec->volHLen[0];
  ly = arg->ec->volHLen[1];
  lz = arg->ec->volHLen[2];
  if (nrrdCenterNode == arg->ctx->volCentering) {
    mm = 0;
    Mx = arg->ctx->volSize[0]-1;
    My = arg->ctx->volSize[1]-1;
    Mz = arg->ctx->volSize[2]-1;
  } else {
    mm = -0.5;
    Mx = arg->ctx->volSize[0]-0.5;
    My = arg->ctx->volSize[1]-0.5;
    Mz = arg->ctx->volSize[2]-0.5;
  }
  
  if (arg->ctx->cam->ortho) {
    ELL_3V_COPY(rayDirW, arg->ctx->cam->N);
    rayDirI[0] = AIR_DELTA(-lx, rayDirW[0], lx, mm, Mx);
    rayDirI[1] = AIR_DELTA(-ly, rayDirW[1], ly, mm, My);
    rayDirI[2] = AIR_DELTA(-lz, rayDirW[2], lz, mm, Mz);
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
    if (nrrdCenterCell == arg->ctx->imgCentering) {
      v = uvScale*AIR_AFFINE(-0.5, vI, arg->ctx->imgSize[1]-0.5,
			     arg->ctx->cam->vRange[0],
			     arg->ctx->cam->vRange[1]);
    } else {
      v = uvScale*AIR_AFFINE(0.0, vI, arg->ctx->imgSize[1]-1.0,
			     arg->ctx->cam->vRange[0],
			     arg->ctx->cam->vRange[1]);
    }
    ELL_3V_SCALE(vOff, v, arg->ctx->cam->V);
    for (uI=0; uI<arg->ctx->imgSize[0]; uI++) {
      if (nrrdCenterCell == arg->ctx->imgCentering) {
	u = uvScale*AIR_AFFINE(-0.5, uI, arg->ctx->imgSize[0]-0.5,
			       arg->ctx->cam->uRange[0],
			       arg->ctx->cam->uRange[1]);
      } else {
	u = uvScale*AIR_AFFINE(0.0, uI, arg->ctx->imgSize[0]-1.0,
			       arg->ctx->cam->uRange[0],
			       arg->ctx->cam->uRange[1]);
      }
      ELL_3V_SCALE(uOff, u, arg->ctx->cam->U);
      ELL_3V_ADD3(rayStartW, uOff, vOff, arg->ec->rayZero);
      rayStartI[0] = AIR_AFFINE(-lx, rayStartW[0], lx, mm, Mx);
      rayStartI[1] = AIR_AFFINE(-ly, rayStartW[1], ly, mm, My);
      rayStartI[2] = AIR_AFFINE(-lz, rayStartW[2], lz, mm, Mz);
      if (!arg->ctx->cam->ortho) {
	ELL_3V_SUB(rayDirW, rayStartW, arg->ctx->cam->from);
	ELL_3V_NORM(rayDirW, rayDirW, tmp);
	rayDirI[0] = AIR_DELTA(-lx, rayDirW[0], lx, mm, Mx);
	rayDirI[1] = AIR_DELTA(-ly, rayDirW[1], ly, mm, My);
	rayDirI[2] = AIR_DELTA(-lz, rayDirW[2], lz, mm, Mz);
	rayLen = ((arg->ctx->cam->vspFaar - arg->ctx->cam->vspNeer)/
		  ELL_3V_DOT(rayDirW, arg->ctx->cam->N));
      }
      if ( (ret = (arg->ctx->rayBegin)(thread,
				       arg->render,
				       arg->ctx->user,
				       uI, vI, rayLen,
				       rayStartW, rayStartI,
				       rayDirW, rayDirI)) ) {
	arg->errCode = ret;
	arg->whichErr = hooverErrRayBegin;
#if !TEEM_PTHREAD && defined(_WIN32)
	return ret;
#else
	return arg;
#endif
      }
      
      sampleI = 0;
      rayT = 0;
      while (1) {
	ELL_3V_SCALEADD(rayPosW, 1.0, rayStartW, rayT, rayDirW);
	ELL_3V_SCALEADD(rayPosI, 1.0, rayStartI, rayT, rayDirI);
	inside = (AIR_IN_CL(mm, rayPosI[0], Mx) &&
		  AIR_IN_CL(mm, rayPosI[1], My) &&
		  AIR_IN_CL(mm, rayPosI[2], Mz));
	rayStep = (arg->ctx->sample)(thread,
				     arg->render,
				     arg->ctx->user,
				     sampleI, rayT,
				     inside,
				     rayPosW, rayPosI);
	if (!AIR_EXISTS(rayStep)) {
	  /* sampling failed */
	  arg->errCode = 0;
	  arg->whichErr = hooverErrSample;
#if !TEEM_PTHREAD && defined(_WIN32)
	  return ret;
#else
	  return arg;
#endif
	}
	if (!rayStep) {
	  /* ray decided to finish itself */
	  break;
	} 
	/* else we moved to a new location along the ray */
	rayT += rayStep;
	if (!AIR_IN_CL(0, rayT, rayLen)) {
	  /* ray stepped outside near-far clipping region, its done. */
	  break;
	}
	sampleI++;
      }
      
      if ( (ret = (arg->ctx->rayEnd)(thread,
				     arg->render,
				     arg->ctx->user)) ) {
	arg->errCode = ret;
	arg->whichErr = hooverErrRayEnd;
#if !TEEM_PTHREAD && defined(_WIN32)
	return ret;
#else
	return arg;
#endif
      }
    }  /* end this scanline */
    vI += arg->ctx->numThreads;
  } /* end skipping through scanlines */

  if ( (ret = (arg->ctx->threadEnd)(thread,
				    arg->render,
				    arg->ctx->user)) ) {
    arg->errCode = ret;
    arg->whichErr = hooverErrThreadEnd;
#if !TEEM_PTHREAD && defined(_WIN32)
    return ret;
#else
    return arg;
#endif
  }
  
  /* returning NULL actually indicates that there was NOT an error */
#if !TEEM_PTHREAD && defined(_WIN32)
    return 0;
#else
    return NULL;
#endif
}

/*
******** hooverRender()
**
** because of the biff usage(), only one thread can call hooverRender(),
** and no promises if the threads themselves call biff...
*/
int
hooverRender(hooverContext *ctx, int *errCodeP, int *errThreadP) {
  char me[]="hooverRender", err[AIR_STRLEN_MED];
  _hooverExtraContext *ec;
  _hooverThreadArg args[HOOVER_THREAD_MAX];
  _hooverThreadArg *errArg;
#if TEEM_PTHREAD
  pthread_t thread[HOOVER_THREAD_MAX];
  pthread_attr_t attr;
#elif defined(_WIN32)
  HANDLE thread[HOOVER_THREAD_MAX];
  int errCode;
#endif

  void *render;
  int ret;
  airArray *mop;
  int threadIdx;
  
  /* this calls limnCamUpdate() */
  if (hooverContextCheck(ctx)) {
    sprintf(err, "%s: problem detected in given context", me);
    biffAdd(HOOVER, err);
    return hooverErrInit;
  }

  if (!(ec = _hooverExtraContextNew(ctx))) {
    sprintf(err, "%s: problem creating thread context", me);
    biffAdd(HOOVER, err);
    return hooverErrInit;
  }
  mop = airMopNew();
  airMopAdd(mop, ec, (airMopper)_hooverExtraContextNix, airMopAlways);
  if ( (ret = (ctx->renderBegin)(&render, ctx->user)) ) {
    *errCodeP = ret;
    airMopError(mop);
    return hooverErrRenderBegin;
  }

  for (threadIdx=0; threadIdx<ctx->numThreads; threadIdx++) {
    args[threadIdx].ctx = ctx;
    args[threadIdx].ec = ec;
    args[threadIdx].render = render;
    args[threadIdx].whichThread = threadIdx;
    args[threadIdx].whichErr = hooverErrNone;
    args[threadIdx].errCode = 0;
  }

  /* (done): call pthread_create() once per thread, passing the
     address of a distinct (and appropriately intialized)
     _hooverThreadArg to each.  If return of pthread_create() is
     non-zero, put its return in *errCodeP, the number of the
     problematic in *errThreadP, and return hooverErrThreadCreate.
     Then call pthread_join() on all the threads, passing &errArg as
     "retval". On non-zero return, set *errCodeP and *errThreadP,
     and return hooverErrThreadJoin. If return of pthread_join() is
     zero, but the errArg is non-NULL, then assume that this errArg
     is actually just the passed _hooverThreadArg returned to us, and
     from this copy errArg->errCode into *errCodeP, and return
     errArg->whichErr */

#ifdef TEEM_PTHREAD
  pthread_attr_init(&attr);
#  ifdef __sgi
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_BOUND_NP);
#  endif
  for (threadIdx=0; threadIdx<ctx->numThreads; threadIdx++) {
    if ((ret = pthread_create(&thread[threadIdx], &attr, _hooverThreadBody, 
			      (void *) &args[threadIdx]))) {
      *errCodeP = ret;
      *errThreadP = threadIdx;
      airMopError(mop);
      return hooverErrThreadCreate;
    }
  }
  for (threadIdx=0; threadIdx<ctx->numThreads; threadIdx++) {
    if ((ret = pthread_join(thread[threadIdx], (void **) (&errArg)))) {
      *errCodeP = ret;
      *errThreadP = threadIdx;
      airMopError(mop);
      return hooverErrThreadJoin;
    }
    if (errArg != NULL) {
      *errCodeP = errArg->errCode;
      *errThreadP = threadIdx;
      return errArg->whichErr;
    }
  }
#elif defined(_WIN32)
  if (1 == ctx->numThreads) {
    errCode = _hooverThreadBody(&(args[0]));
    if (errCode) {
      *errCodeP = errCode;
      airMopError(mop);
      return args[0].whichErr;
    }
  } else {
    for (threadIdx=0; threadIdx<ctx->numThreads; threadIdx++) {
      thread[threadIdx] = CreateThread(0, 0, _hooverThreadBody,
				       (void*)&args[threadIdx], 0, 0);
      if (NULL == thread[threadIdx]) {
	*errCodeP = GetLastError();
	*errThreadP = threadIdx;
	airMopError(mop);
	return hooverErrThreadCreate;
      }
    }
    for (threadIdx=0; threadIdx<ctx->numThreads; threadIdx++) {
      if (WAIT_FAILED == WaitForSingleObject(thread[threadIdx], INFINITE)) {
	*errCodeP = GetLastError();
	*errThreadP = threadIdx;
	airMopError(mop);
	return hooverErrThreadJoin;
      }
      if (0 == GetExitCodeThread(thread[threadIdx], &errCode)) {
	*errCodeP = GetLastError();
	*errThreadP = threadIdx;
	airMopError(mop);
	return hooverErrThreadJoin;
      }
      if (0 != errCode) {
	*errCodeP = errCode;
	*errThreadP = threadIdx;
	return args[threadIdx].whichErr;
      }
    }
  }
#else
  /* old code -- before multi-threading */
  if (1 == ctx->numThreads) {
    errArg = _hooverThreadBody(&(args[0]));
    if (errArg) {
      *errCodeP = errArg->errCode;
      airMopError(mop);
      return errArg->whichErr;
    }
  } else {
    sprintf(err, "%s: sorry, not compiled with TEEM_PTHREAD enabled", me);
    biffAdd(HOOVER, err); return hooverErrInit; 
  }
#endif

  if ( (ret = (ctx->renderEnd)(render, ctx->user)) ) {
    *errCodeP = ret;
    return hooverErrRenderEnd;
  }
  render = NULL;
  airMopOkay(mop);

  return hooverErrNone;
}
