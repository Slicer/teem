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
** _hoovLearnScales()
**
** this assumes that limnCamUpdate(ctx->cam) has been called
*/
void
_hoovLearnScales(double *uBaseP, double *uCapP,
		 double *vBaseP, double *vCapP,
		 double imgOrigin[3],
		 hoovContext *ctx) {
  double toNear[4],       /* from eye to (u,v) origin of near plane */
    near, dist,           /* distances to near and image planes rel. to eye */
    scale,                /* how to scale (u,v) coordinates on image plane
			     to (u,v)-ish coordinates on near plane */
    delta;                /* for calculating pixel locations */

  near = ctx->cam->vspNear;
  dist = ctx->cam->vspDist;
  ELL_4M_GET_ROW2(toNear, ctx->cam);
  ELL_3V_SCALE(toNear, toNear, near);
  ELL_3V_ADD(imgOrigin, ctx->cam->from, toNear);
  if (ctx->cam->ortho) {
    scale = 1.0;
  }
  else {
    scale = near/dist;
  }
  *uBaseP = scale*AIR_AFFINE(-0.5, 0, ctx->imgURes-0.5, 
			     ctx->cam->uMin, ctx->cam->uMax);
  *uCapP = scale*AIR_AFFINE(-0.5, ctx->imgURes-1, ctx->imgURes-0.5, 
			    ctx->cam->uMin, ctx->cam->uMax);
  *vBaseP = scale*AIR_AFFINE(-0.5, 0, ctx->imgVRes-0.5, 
			     ctx->cam->vMin, ctx->cam->vMax);
  *vCapP = scale*AIR_AFFINE(-0.5, ctx->imgVRes-1, ctx->imgVRes-0.5, 
			    ctx->cam->vMin, ctx->cam->vMax);
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
  float volHLen[3],       /* length of x,y,z edges of volume bounding box */
    voxLen[3],           /* length of x,y,z edges of voxels */
    uBase, uCap,         /* uMin and uMax as seen on the near cutting plane,
			    and taking into account ctx->imgPixInside */
    vBase, vCap,         /* analogous to uBase and uCap */
    rayZero[3],          /* location of near plane, line of sight interxion */
    wU[3], wV[3], wN[3], /* orthornormal basis of view space, in world
			    space coordinates */
    minIdx, maxXIdx,     /* bounds in index space of volume, which is */
    maxYIdx, maxZIdx;    /*    affected by ctx->volVoxels */
} _hoovExtraContext;

_hoovExtraContext *
_hoovExtraContextNew(hoovContext *ctx) {
  float uvn[9];
  _hoovExtraContext *ec;
  
  ec = calloc(1, sizeof(_hoovExtraContext));
  if (ec) {
    _hoovLearnLengths(ec->volHLen, ec->voxLen, ctx);
    _hoovLearnScales(&(ec->uBase), &(ec->uCap), &(ec->vBase), &(ec->vCap), 
		     ec->rayZero, ctx);
    if (ctx->volVoxels) {
      ec->minIdx = 0;
      ec->maxXIdx = ctx->volSize[0]-1;
      ec->maxYIdx = ctx->volSize[1]-1;
      ec->maxZIdx = ctx->volSize[2]-1;
    }
    else {
      ec->minIdx = -0.5;
      ec->maxXIdx = ctx->volSize[0]-0.5;
      ec->maxYIdx = ctx->volSize[1]-0.5;
      ec->maxZIdx = ctx->volSize[2]-0.5;
    }
    ELL_34M_EXTRACT(uvn, ctx->cam->W2V);
    ELL_3MV_GET_ROW0(ec->wU, uvn);
    ELL_3MV_GET_ROW1(ec->wV, uvn);
    ELL_3MV_GET_ROW2(ec->wN, uvn);
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
** information which is not thread-specific, as well as some which is.
** It is also intended to be read-only.
*/
typedef struct {
  hoovContext *ctx;
  _hoovExtraContext *ec;
  void *rendInfo;
  int whichThread;
  int whichErr;
  int errCode;
} _hoovThreadArg;

void *
_hoovThreadBody(void *_arg) {
  _hoovThreadArg *arg;
  void *threadInfo;
  int ret,               /* to catch return values from callbacks */
    s,                   /* which sample we're on */
    numSmpls,            /* # of samples for this ray */
    vI, uI,              /* integral coords in image */
    volI[3];             /* integral coords in volume */
  float u, v,            /* floating-point coords in image */
    rayStep,             /* distance between samples (world-space) */
    rayDir[3],           /* unit-length ray direction (world-space) */
    rayStart[3],         /* beginning of current ray (world-space) */
    vOff[3], uOff[3],    /* offsets in arg->ec->wU and arg->ec->wV
			    directions towards start of ray */
    hyp, near, far,      /* comes in handy for calculating steps */
    volF[3],             /* position along ray (index space) */
    rayX, rayY, rayZ,    /* where we are currently along ray (index space) */
    rayXd, rayYd, rayZd, /* increment between samples (index space) */
    norm;                /* for normalizing vectors */

  int debug;

  arg = _arg;
  ret = (arg->ctx->cbBeginThread)(&threadInfo, 
				  arg->rendInfo, 
				  arg->ctx->userInfo,
				  arg->whichThread);
  if (ret) {
    arg->errCode = ret;
    arg->whichErr = hoovErrCbBeginThread;
    return arg;
  }

  /* in case its not perspective projection, then set rayDir (once) */
  if (arg->ctx->cam->ortho) {
    ELL_3V_COPY(rayDir, arg->ec->wN);
  }

  /* for now, the load-balancing among P processors is stupid: the
     Nth thread gets scanline N, and scanlines N+P, N+2P, N+3P, etc */
  vI = arg->whichThread;
  near = arg->ctx->cam->vspNear;
  far = arg->ctx->cam->vspFar;
  while (1) {
    v = AIR_AFFINE(0, vI, arg->ctx->imgVRes-1,
		   arg->ec->vBase, arg->ec->vCap);
    ELL_3V_SCALE(vOff, arg->ec->wV, v);
    for (uI=0; uI<=arg->ctx->imgURes-1; uI++) {
      u = AIR_AFFINE(0, uI, arg->ctx->imgURes-1,
		     arg->ec->uBase, arg->ec->uCap);
      /*
      printf("%s: image coords (%d,%d) -> location (%g,%g) (%x)\n", me,
	     uIdx, vIdx, u, v, arg->ctx->cbBeginRay);
      */
      ELL_3V_SCALE(uOff, arg->ec->wU, u);

      ELL_3V_ADD3(rayStart, uOff, vOff, arg->ec->rayZero);
      if (!arg->ctx->cam->ortho) {
	ELL_3V_SUB(rayDir, rayStart, arg->ctx->cam->from);
	ELL_3V_NORM(rayDir, rayDir, norm);
	hyp = sqrt(near*near + u*u + v*v);
	if (arg->ctx->raySheetStep) {
	  rayStep = (hyp/near)*(arg->ctx->rayStep);
	  numSmpls = 1 + (far - near)/rayStep;
	}
	else {
	  rayStep = arg->ctx->rayStep;
	  numSmpls = 1 + (hyp/near)*(far - near)/rayStep;
	}
      }

      /* the samples along the ray position are always calculated
	 and maintained in the index space of the volume */
      rayX = AIR_AFFINE(-arg->ec->volHLen[0],
			rayStart[0],
			arg->ec->volHLen[0], 
			arg->ec->minIdx, arg->ec->maxXIdx);
      rayY = AIR_AFFINE(-arg->ec->volHLen[1],
			rayStart[1],
			arg->ec->volHLen[1], 
			arg->ec->minIdx, arg->ec->maxYIdx);
      rayZ = AIR_AFFINE(-arg->ec->volHLen[2],
			rayStart[2],
			arg->ec->volHLen[2], 
			arg->ec->minIdx, arg->ec->maxZIdx);
      rayXd = AIR_DELTA(-arg->ec->volHLen[0], 
			rayStep*rayDir[0], 
			arg->ec->volHLen[0], 
			arg->ec->minIdx, arg->ec->maxXIdx);
      rayYd = AIR_DELTA(-arg->ec->volHLen[1],
			rayStep*rayDir[1],
			arg->ec->volHLen[1], 
			arg->ec->minIdx, arg->ec->maxYIdx);
      rayZd = AIR_DELTA(-arg->ec->volHLen[2],
			rayStep*rayDir[2],
			arg->ec->volHLen[2], 
			arg->ec->minIdx, arg->ec->maxZIdx);
      ret = (arg->ctx->cbBeginRay)(threadInfo,
				   arg->rendInfo, arg->ctx->userInfo,
				   uI, vI, rayDir,
				   rayStep, numSmpls);
      if (ret) {
	if (1 == ret) {
	  break;
	}
	else {
	  arg->errCode = ret;
	  arg->whichErr = hoovErrCbBeginRay;
	  return arg;
	}
      }
      
      for (s=0; s<=numSmpls-1; s++) {
	/* this checks for being inside the _closed_ intervals */
	if (AIR_INSIDE(arg->ec->minIdx, rayX, arg->ec->maxXIdx) &&
	    AIR_INSIDE(arg->ec->minIdx, rayY, arg->ec->maxYIdx) &&
	    AIR_INSIDE(arg->ec->minIdx, rayZ, arg->ec->maxZIdx)) {
	  /* we're inside the volume, let's sample */
	  if (arg->ctx->volVoxels) {
	    volI[0] = rayX;
	    volI[1] = rayY;
	    volI[2] = rayZ;
	  }
	  else {
	    volI[0] = rayX + 0.5;
	    volI[1] = rayY + 0.5;
	    volI[2] = rayZ + 0.5;
	  }
	  volI[0] -= volI[0] == arg->ec->maxXIdx;
	  volI[1] -= volI[1] == arg->ec->maxYIdx;
	  volI[2] -= volI[2] == arg->ec->maxZIdx;
	  volF[0] = rayX - volI[0];
	  volF[1] = rayY - volI[1];
	  volF[2] = rayZ - volI[2];
	
	  ret = (arg->ctx->cbSample)(threadInfo,
				     arg->rendInfo, arg->ctx->userInfo,
				     volI, volF);
	  if (ret) {
	    if (1 == ret) {
	      break;
	    }
	    else {
	      arg->errCode = ret;
	      arg->whichErr = hoovErrCbSample;
	      return arg;
	    }
	  }
	}
	/* else the sample was outside the volume */
	rayX += rayXd;
	rayY += rayYd;
	rayZ += rayZd;
      }
      
      ret = (arg->ctx->cbEndRay)(threadInfo,
				 arg->rendInfo, arg->ctx->userInfo);
      if (ret) {
	if (1 == ret) {
	  break;
	}
	else {
	  arg->errCode = ret;
	  arg->whichErr = hoovErrCbEndRay;
	  return arg;
	}
      }
    }
  }

  ret = (arg->ctx->cbEndThread)(threadInfo,
				arg->rendInfo, arg->ctx->userInfo);
  if (ret) {
    arg->errCode = ret;
    arg->whichErr = hoovErrCbEndThread;
    return arg;
  }
  
  /* returning NULL actually indicates that there was NOT an error */
  return NULL;
}

/*
******** hoovRender()
**
** because of the biff usage(), only one thread can call hoovRender()
*/
int
hoovRender(hoovContext *ctx, int *errCodeP, int *errThreadP) {
  char me[]="hoovRender", err[AIR_STRLEN_MED];
  _hoovExtraContext *ec;
  _hoovThreadArg args[HOOV_THREAD_MAX];
  _hoovThreadArg *errArg;
  void *rendInfo;
  int ret;

  /* this calls limnCamUpdate() */
  if (hoovCheck(ctx)) {
    sprintf(err, "%s: problem detected in given context", me);
    biffAdd(HOOVER, err);
    return hoovErrInit;
  }

  if (!(ec = _hoovExtraContextNew(ctx))) {
    sprintf(err, "%s: problem creating thread context", me);
    biffAdd(HOOVER, err);
    return hoovErrInit;
  }
  
  ret = (ctx->cbBeginRender)(&rendInfo, ctx->userInfo);
  if (ret) {
    *errCodeP = ret;
    return hoovErrCbBeginRender;
  }

  if (1 == ctx->numThreads) {
    args[0].ctx = ctx;
    args[0].ec = ec;
    args[0].rendInfo = rendInfo;
    args[0].whichThread = 0;
    args[0].whichErr = hoovErrNone;
    args[0].errCode = 0;
    errArg = _hoovThreadBody(&(args[0]));
    if (errArg) {
      *errCodeP = errArg->errCode;
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

  _hoovExtraContextNix(ec);
  ret = (ctx->cbEndRender)(rendInfo, ctx->userInfo);
  if (ret) {
    *errCodeP = ret;
    return hoovErrCbEndRender;
  }
  rendInfo = NULL;

  return hoovErrNone;
}
