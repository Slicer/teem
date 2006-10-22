/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

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
pushThingNew(unsigned int vertNum) {
  static int ttaagg=0; 
  pushThing *thg;
  unsigned int idx;

  if (!( vertNum >= 1 )) {
    thg = NULL;
  } else {
    thg = (pushThing *)calloc(1, sizeof(pushThing));
    if (thg) {
      thg->ttaagg = ttaagg++;
      thg->point.thing = thg;
      thg->vertNum = vertNum;
      if (1 == vertNum) {
        thg->vert = &(thg->point); /* HEY: is this just inviting confusion? */
      } else {
        thg->vert = (pushPoint *)calloc(vertNum, sizeof(pushPoint));
        for (idx=0; idx<vertNum; idx++) {
          thg->vert[idx].thing = thg;
        }
      }
      thg->seedIdx = 0;
      thg->len = 0;
    }
  }
  return thg;
}

pushThing *
pushThingNix(pushThing *thg) {
  
  if (thg) {
    if (1 < thg->vertNum) {
      thg->vert = (pushPoint *)airFree(thg->vert);
    }
    airFree(thg);
  }
  return NULL;
}

pushContext *
pushContextNew(void) {
  pushContext *pctx;

  pctx = (pushContext *)calloc(1, sizeof(pushContext));
  if (pctx) {
    pctx->nin = NULL;
    pctx->npos = NULL;
    pctx->nstn = NULL;
    pctx->step = 1;
    pctx->scale = 0.2;
    pctx->wall = 0.1;
    pctx->cntScl = 0.0;
    pctx->bigTrace = 0.0;
    pctx->minMeanVel = 0.0;

    pctx->detReject = AIR_FALSE;
    pctx->midPntSmp = AIR_FALSE;
    pctx->verbose = 0;

    pctx->seedRNG = 42;
    pctx->thingNum = 0;
    pctx->threadNum = 1;
    pctx->maxIter = 0;
    pctx->snap = 0;

    pctx->gravItem = tenGageUnknown;
    pctx->gravNotItem[0] = tenGageUnknown;
    pctx->gravNotItem[1] = tenGageUnknown;
    pctx->gravScl = 0.0;

    pctx->seedThreshItem  = tenGageUnknown;
    pctx->seedThreshSign = +1;
    pctx->seedThresh = 0.0;

    pctx->ensp = NULL;

    pctx->tltUse = AIR_FALSE;
    pctx->tltFrenet = AIR_FALSE;
    pctx->tltStepNum = 5;
    pctx->tltThresh = 0.0;
    pctx->tltSoft = 0.0;
    pctx->tltStep = 0.0;

    pctx->binSingle = AIR_FALSE;
    pctx->binIncr = 512;

    pctx->ksp00 = nrrdKernelSpecNew();
    pctx->ksp11 = nrrdKernelSpecNew();
    pctx->ksp22 = nrrdKernelSpecNew();

    pctx->nten = NULL;
    pctx->ninv = NULL;
    pctx->nmask = NULL;
    pctx->gctx = NULL;
    pctx->tpvl = NULL;
    pctx->ipvl = NULL;
    pctx->fctx = NULL;
    pctx->finished = AIR_FALSE;
    pctx->dimIn = 0;
    pctx->sliceAxis = 42;  /* an invalid value */

    pctx->bin = NULL;
    ELL_3V_SET(pctx->binsEdge, 0, 0, 0);
    pctx->binNum = 0;
    pctx->binIdx = 0;
    pctx->binMutex = NULL;

    pctx->maxDist = AIR_NAN;
    pctx->maxEval = AIR_NAN;
    pctx->meanEval = AIR_NAN;
    pctx->maxDet = AIR_NAN;
    pctx->meanVel = 0;

    pctx->task = NULL;

    pctx->iterBarrierA = NULL;
    pctx->iterBarrierB = NULL;
    pctx->timeIteration = 0;
    pctx->timeRun = 0;
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
    pctx->ksp22 = nrrdKernelSpecNix(pctx->ksp22);
    airFree(pctx);
  }
  return NULL;
}
