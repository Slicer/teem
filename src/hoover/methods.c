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

hoovContext *
hoovContextNew() {
  hoovContext *ctx;

  ctx = (hoovContext *)calloc(1, sizeof(hoovContext));
  if (ctx) {
    ctx->cam = limnCamNew();
    ELL_3V_SET(ctx->volSize, 0, 0, 0);
    ELL_3V_SET(ctx->volSpacing, AIR_NAN, AIR_NAN, AIR_NAN);
    ctx->imgUSize = ctx->imgVSize = 0;
    ctx->userInfo = NULL;
    ctx->numThreads = 1;
    ctx->renderBegin = hoovStubRenderBegin;
    ctx->threadBegin = hoovStubThreadBegin;
    ctx->rayBegin = hoovStubRayBegin;
    ctx->sample = hoovStubSample;
    ctx->rayEnd = hoovStubRayEnd;
    ctx->threadEnd = hoovStubThreadEnd;
    ctx->renderEnd = hoovStubRenderEnd;
  }
  return(ctx);
}

int
hoovContentCheck(hoovContext *ctx) {
  char me[] = "hoovContentCheck", err[AIR_STRLEN_MED];

  if (!ctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(HOOVER, err); return 1;
  }
  if (limnCamUpdate(ctx->cam)) {
    sprintf(err, "%s: trouble learning view transform matrix", me);
    biffMove(HOOVER, err, LIMN); return 1;
  }
  if (!(ctx->volSize[0] > 1 
	&& ctx->volSize[1] > 1 
	&& ctx->volSize[2] > 1)) {
    sprintf(err, "%s: volume dimensions (%dx%dx%d) invalid", me,
	    ctx->volSize[0], ctx->volSize[1], ctx->volSize[2]);
    biffAdd(HOOVER, err); return 1;
  }
  if (!(ctx->volSpacing[0] > 0.0
	&& ctx->volSpacing[1] > 0.0
	&& ctx->volSpacing[2] > 0.0)) {
    sprintf(err, "%s: volume spacing (%gx%gx%g) invalid", me,
	    ctx->volSpacing[0], ctx->volSpacing[1], ctx->volSpacing[2]);
    biffAdd(HOOVER, err); return 1;
  }
  if (!(ctx->imgUSize > 1 && ctx->imgVSize > 1)) {
    sprintf(err, "%s: image dimensions (%dx%d) invalid", me,
	    ctx->imgUSize, ctx->imgVSize);
    biffAdd(HOOVER, err); return 1;
  }
  if (!(ctx->numThreads >= 1)) {
    sprintf(err, "%s: number threads (%d) invalid", me, ctx->numThreads);
    biffAdd(HOOVER, err); return 1;
  }
  if (!(ctx->numThreads <= HOOV_THREAD_MAX)) {
    sprintf(err, "%s: sorry, number threads (%d) > max (%d)", me, 
	    ctx->numThreads, HOOV_THREAD_MAX);
    biffAdd(HOOVER, err); return 1;
  }
  if (!ctx->renderBegin) {
    sprintf(err, "%s: need a non-NULL begin rendering callback", me);
    biffAdd(HOOVER, err); return 1;
  }
  if (!ctx->rayBegin) {
    sprintf(err, "%s: need a non-NULL begin ray callback", me);
    biffAdd(HOOVER, err); return 1;
  }
  if (!ctx->threadBegin) {
    sprintf(err, "%s: need a non-NULL begin thread callback", me);
    biffAdd(HOOVER, err); return 1;
  }
  if (!ctx->sample) {
    sprintf(err, "%s: need a non-NULL sampler callback function", me);
    biffAdd(HOOVER, err); return 1;
  }
  if (!ctx->rayEnd) {
    sprintf(err, "%s: need a non-NULL end ray callback", me);
    biffAdd(HOOVER, err); return 1;
  }
  if (!ctx->threadEnd) {
    sprintf(err, "%s: need a non-NULL end thread callback", me);
    biffAdd(HOOVER, err); return 1;
  }
  if (!ctx->renderEnd) {
    sprintf(err, "%s: need a non-NULL end render callback", me);
    biffAdd(HOOVER, err); return 1;
  }

  return 0;
}

void
hoovContextNix(hoovContext *ctx) {

  if (ctx) {
    limnCamNix(ctx->cam);
    free(ctx);
  }
}

