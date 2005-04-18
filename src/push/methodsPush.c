/*
  Teem: Gordon Kindlmann's research software
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

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


#include "push.h"
#include "privatePush.h"

pushThing *
pushThingNew(int numVert) {
  /* static int ttaagg=0; */
  pushThing *thg;

  if (!( numVert >= 1 )) {
    thg = NULL;
  } else {
    thg = (pushThing *)calloc(1, sizeof(pushThing));
    if (thg) {
      /* thg->ttaagg = ttaagg++; */
      thg->numVert = numVert;
      if (1 == numVert) {
        thg->vert = &(thg->point);
      } else {
        thg->vert = (pushPoint *)calloc(numVert, sizeof(pushPoint));
      }
      thg->len = 0;
      thg->seedIdx = -1;
    }
  }
  return thg;
}

pushThing *
pushThingNix(pushThing *thg) {
  
  if (thg) {
    if (thg->vert != &(thg->point)) {
      thg->vert = airFree(thg->vert);
    }
    airFree(thg);
  }
  return NULL;
}

pushBin *
pushBinNew() {
  pushBin *bin;

  bin = (pushBin *)calloc(1, sizeof(pushBin));
  if (bin) {
    bin->numThing = 0;
    bin->thing = NULL;
    bin->thingArr = airArrayNew((void**)&(bin->thing), &(bin->numThing),
                                sizeof(pushThing *), PUSH_ARRAY_INCR);
    /* airArray callbacks are tempting but super confusing .... */
    bin->neighbor = NULL;
  }
  return bin;
}

/*
** bins own their contents: when you nix a bin, you nix its contents
*/
pushBin *
pushBinNix(pushBin *bin) {
  int thingI;

  if (bin) {
    for (thingI=0; thingI<bin->numThing; thingI++) {
      bin->thing[thingI] = pushThingNix(bin->thing[thingI]);
    }
    bin->thingArr = airArrayNuke(bin->thingArr);
    bin->neighbor = airFree(bin->neighbor);
    airFree(bin);
  }
  return NULL;
}


pushContext *
pushContextNew(void) {
  pushContext *pctx;
  int si, pi;

  pctx = (pushContext *)calloc(1, sizeof(pushContext));
  if (pctx) {
    pctx->nin = NULL;
    pctx->npos = NULL;
    pctx->nstn = NULL;
    pctx->drag = 0.1;
    pctx->preDrag = 1.0;
    pctx->step = 0.01;
    pctx->mass = 1.0;
    pctx->scale = 0.2;
    pctx->nudge = 0.0;
    pctx->wall = 0.1;
    pctx->margin = 0.3;
    pctx->tlThresh = 0.0;
    pctx->tlSoft = 0.0;
    pctx->minMeanVel = 0.0;
    pctx->seed = 42;
    pctx->numThing = 0;
    pctx->numThread = 1;
    pctx->numStage = 0;
    pctx->minIter = 0;
    pctx->maxIter = 0;
    pctx->snap = 0;
    pctx->singleBin = AIR_FALSE;
    pctx->driftCorrect = AIR_TRUE;
    pctx->verbose = 0;
    pctx->force = NULL;
    pctx->ksp00 = nrrdKernelSpecNew();
    pctx->ksp11 = nrrdKernelSpecNew();
    for (si=0; si<PUSH_STAGE_MAXNUM; si++) {
      for (pi=0; pi<PUSH_STAGE_PARM_MAXNUM; pi++) {
        pctx->stageParm[si][pi] = AIR_NAN;
      }
      pctx->process[si] = _pushProcessDummy;
    }
    pctx->nten = NULL;
    pctx->nmask = NULL;
    pctx->gctx = NULL;
    pctx->fctx = NULL;
    pctx->dimIn = 0;
    /* binsEdge and numBin are found later */
    pctx->binsEdge = pctx->numBin = 0;
    pctx->finished = AIR_FALSE;
    pctx->stageIdx = pctx->binIdx = 0;
    pctx->bin = NULL;
    pctx->maxDist = AIR_NAN;
    ELL_3V_SET(pctx->minPos, AIR_NAN, AIR_NAN, AIR_NAN);
    ELL_3V_SET(pctx->maxPos, AIR_NAN, AIR_NAN, AIR_NAN);
    pctx->meanVel = 0;
    pctx->time0 = pctx->time1 = 0;
    pctx->task = NULL;
    pctx->binMutex = NULL;
    pctx->stageBarrierA = NULL;
    pctx->stageBarrierB = NULL;
    pctx->time = 0;
    pctx->iter = 0;
    pctx->noutPos = NULL;
    pctx->noutTen = NULL;
  }
  return pctx;
}

pushContext *
pushContextNix(pushContext *pctx) {
  
  if (pctx) {
    /* weirdness: we don't manage the pushForce- caller (perhaps hest) does */
    pctx->ksp00 = nrrdKernelSpecNix(pctx->ksp00);
    pctx->ksp11 = nrrdKernelSpecNix(pctx->ksp11);
    airFree(pctx);
  }
  return NULL;
}

