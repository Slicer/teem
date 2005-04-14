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

void
_pushProbe(pushTask *task, pushPoint *point) {
  double xi, yi, zi, *minPos, *maxPos;
  int max0, max1, max2;

  /* HEY: this assumes node centering */
  max0 = task->pctx->nten->axis[1].size - 1;
  max1 = task->pctx->nten->axis[2].size - 1;
  max2 = task->pctx->nten->axis[3].size - 1;
  minPos = task->pctx->minPos;
  maxPos = task->pctx->maxPos;
  xi = AIR_AFFINE(minPos[0], point->pos[0], maxPos[0], 0, max0);
  yi = AIR_AFFINE(minPos[1], point->pos[1], maxPos[1], 0, max1);
  zi = AIR_AFFINE(minPos[2], point->pos[2], maxPos[2], 0, max2);
  xi = AIR_CLAMP(0, xi, max0);
  yi = AIR_CLAMP(0, yi, max1);
  zi = AIR_CLAMP(0, zi, max2);
  gageProbe(task->gctx, xi, yi, zi);
  TEN_T_COPY(point->ten, task->tenAns);
  pushTenInv(task->pctx, point->inv, point->ten);
  ELL_3V_COPY(point->cnt, task->cntAns);
  return;
}

/* 
** _pushTensorFieldSetup sets:
**   pctx->dimIn
**   pctx->nten
**   pctx->nmask
** and checks mask range
*/
int
_pushTensorFieldSetup(pushContext *pctx) {
  char me[]="_pushTensorFieldSetup", err[AIR_STRLEN_MED];
  Nrrd *seven[7], *two[2];
  NrrdRange *nrange;
  airArray *mop;
  Nrrd *ntmp;
  int ii, E;

  mop = airMopNew();
  ntmp = nrrdNew();
  airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
  E = AIR_FALSE;
  pctx->nten = nrrdNew();
  pctx->nmask = nrrdNew();
  if (3 == pctx->nin->dim) {
    /* input is 2D array of 2D tensors */
    pctx->dimIn = 2;
    for (ii=0; ii<7; ii++) {
      if (ii < 2) {
        two[ii] = nrrdNew();
        airMopAdd(mop, two[ii], (airMopper)nrrdNuke, airMopAlways);
      }
      seven[ii] = nrrdNew();
      airMopAdd(mop, seven[ii], (airMopper)nrrdNuke, airMopAlways);
    }
    /*    (0)         (0)
     *     1  2  3     1  2
     *        4  5        3
     *           6            */
    if (!E) E |= nrrdSlice(seven[0], pctx->nin, 0, 0);
    if (!E) E |= nrrdSlice(seven[1], pctx->nin, 0, 1);
    if (!E) E |= nrrdSlice(seven[2], pctx->nin, 0, 2);
    if (!E) E |= nrrdArithUnaryOp(seven[3], nrrdUnaryOpZero, seven[0]);
    if (!E) E |= nrrdSlice(seven[4], pctx->nin, 0, 3);
    if (!E) E |= nrrdArithUnaryOp(seven[5], nrrdUnaryOpZero, seven[0]);
    if (!E) E |= nrrdArithUnaryOp(seven[6], nrrdUnaryOpZero, seven[0]);
    if (!E) E |= nrrdJoin(two[0], (const Nrrd *const *)seven, 7, 0, AIR_TRUE);
    if (!E) E |= nrrdCopy(two[1], two[0]);
    if (!E) E |= nrrdJoin(ntmp, (const Nrrd *const *)two, 2, 3, AIR_TRUE);
    if (!E) E |= nrrdConvert(pctx->nten, ntmp, nrrdTypeFloat);
  } else {
    /* input was already 3D */
    pctx->dimIn = 3;
    E = nrrdConvert(pctx->nten, pctx->nin, nrrdTypeFloat);
  }
  if (!E) E |= nrrdSlice(pctx->nmask, pctx->nten, 0, 0);
  if (E) {
    sprintf(err, "%s: trouble creating 3D tensor input", me);
    biffMove(PUSH, err, NRRD); airMopError(mop); return 1;
  }
  nrange = nrrdRangeNewSet(pctx->nmask, nrrdBlind8BitRangeFalse);
  airMopAdd(mop, nrange, (airMopper)nrrdRangeNix, airMopAlways);
  if (AIR_ABS(1.0 - nrange->max) > 0.005) {
    sprintf(err, "%s: tensor mask max %g not close 1.0", me, nrange->max);
    biffAdd(PUSH, err); airMopError(mop); return 1;
  }

  pctx->nten->axis[1].spacing = (AIR_EXISTS(pctx->nten->axis[1].spacing)
                                 ? pctx->nten->axis[1].spacing
                                 : 1.0);
  pctx->nten->axis[2].spacing = (AIR_EXISTS(pctx->nten->axis[2].spacing)
                                 ? pctx->nten->axis[2].spacing
                                 : 1.0);
  pctx->nten->axis[3].spacing = (AIR_EXISTS(pctx->nten->axis[3].spacing)
                                 ? pctx->nten->axis[3].spacing
                                 : 1.0);
  pctx->nmask->axis[0].spacing = pctx->nten->axis[1].spacing;
  pctx->nmask->axis[1].spacing = pctx->nten->axis[2].spacing;
  pctx->nmask->axis[2].spacing = pctx->nten->axis[3].spacing;
  /* HEY: we're only doing this because gage has a bug with
     cell-centered volume 1 sample thick- perhaps there should
     be a warning ... */
  pctx->nten->axis[1].center = pctx->nmask->axis[0].center = nrrdCenterNode;
  pctx->nten->axis[2].center = pctx->nmask->axis[1].center = nrrdCenterNode;
  pctx->nten->axis[3].center = pctx->nmask->axis[2].center = nrrdCenterNode;

  airMopOkay(mop); 
  return 0;
}

/*
** _pushGageSetup sets:
**   pctx->gctx
**   pctx->minPos
**   pctx->maxPos
*/
int
_pushGageSetup(pushContext *pctx) {
  char me[]="_pushGageSetup", err[AIR_STRLEN_MED];
  gagePerVolume *tpvl, *mpvl;
  int E;

  pctx->gctx = gageContextNew();
  E = AIR_FALSE;
  /* set up tensor probing */
  if (!E) E |= !(tpvl = gagePerVolumeNew(pctx->gctx,
                                         pctx->nten, tenGageKind));
  if (!E) E |= gagePerVolumeAttach(pctx->gctx, tpvl);
  if (!E) E |= gageKernelSet(pctx->gctx, gageKernel00,
                             pctx->ksp00->kernel, pctx->ksp00->parm);
  if (!E) E |= gageQueryItemOn(pctx->gctx, tpvl, tenGageTensor);
  /* set up mask gradient probing */
  if (!E) E |= !(mpvl = gagePerVolumeNew(pctx->gctx,
                                         pctx->nmask, gageKindScl));
  if (!E) E |= gagePerVolumeAttach(pctx->gctx, mpvl);
  if (!E) E |= gageKernelSet(pctx->gctx, gageKernel11,
                             pctx->ksp11->kernel, pctx->ksp11->parm);
  if (!E) E |= gageQueryItemOn(pctx->gctx, mpvl, gageSclGradVec);
  if (!E) E |= gageUpdate(pctx->gctx);
  if (E) {
    sprintf(err, "%s: trouble setting up gage", me);
    biffMove(PUSH, err, GAGE); return 1;
  }
  gageParmSet(pctx->gctx, gageParmRequireAllSpacings, AIR_TRUE);
  ELL_3V_SCALE(pctx->minPos, -1, pctx->gctx->shape->volHalfLen);
  ELL_3V_SCALE(pctx->maxPos, 1, pctx->gctx->shape->volHalfLen);

  return 0;
}

/*
** _pushTaskSetup sets:
**   pctx->task
**   pctx->task[]
*/
int
_pushTaskSetup(pushContext *pctx) {
  char me[]="_pushTaskSetup", err[AIR_STRLEN_MED];
  int tidx;

  pctx->task = (pushTask **)calloc(pctx->numThread, sizeof(pushTask *));
  if (!(pctx->task)) {
    sprintf(err, "%s: couldn't allocate array of tasks", me);
    biffAdd(PUSH, err); return 1;
  }
  for (tidx=0; tidx<pctx->numThread; tidx++) {
    pctx->task[tidx] = _pushTaskNew(pctx, tidx);
    if (!(pctx->task[tidx])) {
      sprintf(err, "%s: couldn't allocate task %d", me, tidx);
      biffAdd(PUSH, err); return 1;
    }
  }
  return 0;
}

/*
** _pushBinSetup sets:
**   pctx->maxDist
**   pctx->binsEdge
**   pctx->numBin
**   pctx->bin
**   pctx->bin[]
*/
int
_pushBinSetup(pushContext *pctx) {
  char me[]="_pushBinSetup", err[AIR_STRLEN_MED];
  float eval[3], *tdata, maxEval;
  int ii, nn;

  /* ------------------------ find maxEval and set up binning */
  nn = nrrdElementNumber(pctx->nten)/7;
  tdata = (float*)pctx->nten->data;
  maxEval = 0;
  for (ii=0; ii<nn; ii++) {
    tenEigensolve_f(eval, NULL, tdata);
    if (tdata[0] > 0.5) {
      /* HEY: this limitation may be a bad idea */
      maxEval = AIR_MAX(maxEval, eval[0]);
    }
    tdata += 7;
  }
  pctx->maxDist = pctx->force->maxDist(pctx->scale, maxEval,
                                       pctx->force->parm);
  if (pctx->singleBin) {
    pctx->binsEdge = 1;
    pctx->numBin = 1;
  } else {
    pctx->binsEdge = floor((2.0 + 2*pctx->margin)/pctx->maxDist);
    fprintf(stderr, "!%s: maxEval=%g -> maxDist=%g -> binsEdge=%d\n",
            me, maxEval, pctx->maxDist, pctx->binsEdge);
    if (!(pctx->binsEdge >= 1)) {
      fprintf(stderr, "!%s: fixing binsEdge %d to 1\n", me, pctx->binsEdge);
      pctx->binsEdge = 1;
    }
    pctx->numBin = pctx->binsEdge*pctx->binsEdge*(2 == pctx->dimIn ? 
                                                  1 : pctx->binsEdge);
  }
  pctx->bin = (pushBin **)calloc(pctx->numBin, sizeof(pushBin*));
  if (!( pctx->bin )) {
    sprintf(err, "%s: trouble allocating bin arrays", me);
    biffAdd(PUSH, err); return 1;
  }
  for (ii=0; ii<pctx->numBin; ii++) {
    pctx->bin[ii] = pushBinNew();
  }

  return 0;
}

/*
** _pushThingSetup sets:
**   pctx->numThing (in case pctx->nstn and/or pctx->npos)
*/
int
_pushThingSetup(pushContext *pctx) {
  double (*lup)(const void *v, size_t I);
  int *stn, pointIdx, baseIdx, numPoint, thingIdx;
  pushThing *thing;

  numPoint = (pctx->npos
              ? pctx->npos->axis[1].size
              : pctx->numThing);
  pctx->numThing = (pctx->nstn
                    ? pctx->nstn->axis[1].size
                    : numPoint);
  lup = pctx->npos ? nrrdDLookup[pctx->npos->type] : NULL;
  stn = pctx->nstn ? (int*)pctx->nstn->data : NULL;
  for (thingIdx=0; thingIdx<pctx->numThing; thingIdx++) {
    if (pctx->nstn) {
      thing = pushThingNew(stn[1 + 2*thingIdx]);
      baseIdx = stn[0 + 2*thingIdx];
      for (pointIdx=0; pointIdx<stn[1 + 2*thingIdx]; pointIdx++) {
        ELL_3V_SET(thing->point[pointIdx].pos,
                   lup(pctx->npos->data, 0 + 3*(pointIdx + baseIdx)),
                   lup(pctx->npos->data, 1 + 3*(pointIdx + baseIdx)),
                   lup(pctx->npos->data, 2 + 3*(pointIdx + baseIdx)));
        _pushProbe(pctx->task[0], thing->point + pointIdx);
      }
    } else if (pctx->npos) {
      thing = pushThingNew(1);
      ELL_3V_SET(thing->point[0].pos,
                 lup(pctx->npos->data, 0 + 3*thingIdx),
                 lup(pctx->npos->data, 1 + 3*thingIdx),
                 lup(pctx->npos->data, 2 + 3*thingIdx));
      _pushProbe(pctx->task[0], thing->point + 0);
    } else {
      thing = pushThingNew(1);
      do {
        thing->point[0].pos[0] = AIR_AFFINE(0.0, airDrand48(), 1.0,
                                            pctx->minPos[0],
                                            pctx->maxPos[0]);
        thing->point[0].pos[1] = AIR_AFFINE(0.0, airDrand48(), 1.0,
                                            pctx->minPos[1],
                                            pctx->maxPos[1]);
        if (2 == pctx->dimIn) {
          thing->point[0].pos[2] = 0;
        } else {
          thing->point[0].pos[2] = AIR_AFFINE(0.0, airDrand48(), 1.0,
                                              pctx->minPos[2],
                                              pctx->maxPos[2]);
        }
        _pushProbe(pctx->task[0], thing->point + 0);
      } while (thing->point[0].ten[0] < 0.5);
    }
    pushBinAdd(pctx, thing);
  }

  return 0;
}

#if 0
int
_pushForceSample(pushContext *pctx, int sx, int sy) {
  char me[]="_pushForceSample", err[AIR_STRLEN_MED];
  Nrrd *ntmp;
  double *data;
  int sx, sy, np, xi, yi, pi;
  push_t fsum[3], fvec[3], attrTmp[PUSH_ATTR_LEN];
  

  np = pctx->nPointAttr->axis[1].size;
  ntmp = nrrdNew();
  nrrdMaybeAlloc(ntmp, nrrdTypeDouble, 3, 3, sx, sy);
  data = (double*)ntmp->data;
  (attrTmp + PUSH_POS)[2] = 0.0;
  fprintf(stderr, "sampling force field"); fflush(stderr);
  for (yi=0; yi<sy; yi++) {
    fprintf(stderr, " %d/%d", yi, sy);
    (attrTmp + PUSH_POS)[1] = AIR_AFFINE(0, yi, sy-1,
                                         pctx->minPos[1], pctx->maxPos[1]);
    for (xi=0; xi<sx; xi++) {
      (attrTmp + PUSH_POS)[0] = AIR_AFFINE(0, xi, sx-1,
                                           pctx->minPos[0], pctx->maxPos[0]);
      _pushProbe(pctx, pctx->gctx, attrTmp + PUSH_POS);
      TEN_T_COPY(attrTmp + PUSH_TEN, pctx->tenAns);
      pushTenInv(pctx, attrTmp + PUSH_INV, attrTmp + PUSH_TEN);
      ELL_3V_SET(fsum, 0, 0, 0);
      for (pi=0; pi<np; pi+=30) {
        attr = (push_t *)(pctx->nPointAttr->data) + PUSH_ATTR_LEN*pi;
        _pushPairwiseForce(pctx, fvec, pctx->force, attrTmp, attr);
        ELL_3V_INCR(fsum, fvec);
      }
      ELL_3V_COPY(data+ 3*(xi + sx*yi), fsum);
    }
  }
  fprintf(stderr, " done.\n");
  ntmp->axis[1].min = pctx->minPos[0];
  ntmp->axis[1].max = pctx->maxPos[0];
  ntmp->axis[2].min = pctx->minPos[1];
  ntmp->axis[2].max = pctx->maxPos[1];
  nrrdSave("pray.nrrd", ntmp, NULL);
  
  return 0;
}

#endif 

int
_pushThingTotal(pushContext *pctx) {
  int binIdx, numThing;

  numThing = 0;
  for (binIdx=0; binIdx<pctx->numBin; binIdx++) {
    numThing += pctx->bin[binIdx]->numThing;
  }
  return numThing;
}

int
_pushPointTotal(pushContext *pctx) {
  int binIdx, thingIdx, numPoint;
  pushBin *bin;

  numPoint = 0;
  for (binIdx=0; binIdx<pctx->numBin; binIdx++) {
    bin = pctx->bin[binIdx];
    for (thingIdx=0; thingIdx<bin->numThing; thingIdx++) {
      numPoint += bin->thing[thingIdx]->numPoint;
    }
  }
  return numPoint;
}

int
pushOutputGet(Nrrd *nPosOut, Nrrd *nTenOut, Nrrd *nStnOut,
              pushContext *pctx) {
  char me[]="pushOutputGet", err[AIR_STRLEN_MED];
  int binIdx, pointRun, numPoint, numThing,
    pointIdx, thingIdx, *stnOut, E;
  push_t *posOut, *tenOut;
  pushBin *bin;
  pushThing *thing;
  pushPoint *point;

  numPoint = _pushPointTotal(pctx);
  numThing = _pushThingTotal(pctx);
  E = AIR_FALSE;
  if (nPosOut) {
    E |= nrrdMaybeAlloc(nPosOut, push_nrrdType, 2,
                        2 == pctx->dimIn ? 2 : 3, numPoint);
  }
  if (nTenOut) {
    E |= nrrdMaybeAlloc(nTenOut, push_nrrdType, 2, 
                        2 == pctx->dimIn ? 4 : 7, numPoint);
  }
  if (nStnOut) {
    E |= nrrdMaybeAlloc(nStnOut, nrrdTypeInt, 2, 2, numThing);
  }
  if (E) {
    sprintf(err, "%s: trouble allocating outputs", me);
    biffMove(PUSH, err, NRRD); return 1;
  }
  posOut = nPosOut ? (push_t*)(nPosOut->data) : NULL;
  tenOut = nTenOut ? (push_t*)(nTenOut->data) : NULL;
  stnOut = nStnOut ? (int*)(nStnOut->data) : NULL;

  pointRun = 0;
  for (binIdx=0; binIdx<pctx->numBin; binIdx++) {
    bin = pctx->bin[binIdx];
    for (thingIdx=0; thingIdx<bin->numThing; thingIdx++) {
      thing = bin->thing[thingIdx];
      if (stnOut) {
        ELL_2V_SET(stnOut + thingIdx*2,
                   pointRun, thing->numPoint);
      }
      for (pointIdx=0; pointIdx<thing->numPoint; pointIdx++) {
        point = thing->point + pointIdx;
        if (2 == pctx->dimIn) {
          if (posOut) {
            ELL_2V_SET(posOut + pointRun*2,
                       point->pos[0], point->pos[1]);
          }
          /*    (0)         (0)
           *     1  2  3     1  2
           *        4  5        3
           *           6            */
          if (tenOut) {
            ELL_4V_SET(tenOut + pointRun*4,
                       point->ten[0], point->ten[1],
                       point->ten[2], point->ten[3]);
          }
        } else {
          if (posOut) {
            ELL_3V_SET(posOut + pointRun*3,
                       point->pos[0], point->pos[1], point->pos[2]);
          }
          if (tenOut) {
            TEN_T_COPY(tenOut + pointRun*7, point->ten);
          }
        }
        pointRun++;
      }
    }
  }

  return 0;
}

void
_pushPairwiseForce(pushContext *pctx, push_t fvec[3], pushForce *force,
                   pushPoint *myPoint, pushPoint *herPoint) {
  char me[]="_pushPairwiseForce";
  push_t ten[7], inv[7], dot;
  float haveDist, restDist, mm, fix, mag,
    D[3], nD[3], lenD, 
    U[3], nU[3], lenU, 
    V[3], lenV;

  /* in case lenD > maxDist */
  ELL_3V_SET(fvec, 0, 0, 0);

  ELL_3V_SUB(D, myPoint->pos, herPoint->pos);
  ELL_3V_NORM(nD, D, lenD);
  if (!lenD) {
    fprintf(stderr, "%s: myPos == herPos == (%g,%g,%g)\n", me,
            myPoint->pos[0], myPoint->pos[1], myPoint->pos[2]);
    return;
  }
  if (lenD <= pctx->maxDist) {
    TEN_T_SCALE_ADD2(ten,
                     0.5, myPoint->ten,
                     0.5, herPoint->ten);
    pushTenInv(pctx, inv, ten);
    TEN_TV_MUL(U, inv, D);
    ELL_3V_NORM(nU, U, lenU);
    dot = ELL_3V_DOT(nU, nD);
    haveDist = dot*lenD;
    restDist = dot*2*pctx->scale*lenD/lenU;
    mag = force->func(haveDist, restDist, pctx->scale, force->parm);
    ELL_3V_SCALE(fvec, mag, nU);

    if (pctx->driftCorrect) {
      TEN_TV_MUL(V, myPoint->inv, D);
      lenV = ELL_3V_LEN(V);
      /* dc-0: mm = 2*dot*pctx->scale*(1.0/lenV - 1.0/lenU);
         fix = (1 - mm)/(1 + mm); */
      /* dc-1: mm = 2*dot*pctx->scale*(1.0/lenV - 1.0/lenU);
         fix = (1 + mm)/(1 - mm); */
      /* dc-2: seems to work for gaussian; still drifting w/ charge;
       but *reverse* drift for cotan!! 
       mm = 4*dot*pctx->scale*(1.0/lenV - 1.0/lenU);
       fix = (1 + mm)/(1 - mm); */
      /* 
      ** ----- this is probably correct, based on
      ** ----- tests with the one-ramp.nrrd dataset
      */
      mm = 2*dot*pctx->scale*(1.0/lenV - 1.0/lenU);
      fix = sqrt((1 + mm)/(1 - mm));
      ELL_3V_SCALE(fvec, fix, fvec);
    }
  }
  return;
}

void
_pushForce(pushTask *task, int myBinIdx,
           const push_t parm[PUSH_STAGE_PARM_MAXNUM]) {
  pushBin *myBin, *herBin, **neighbor;
  pushThing *myThing, *herThing;
  pushPoint *myPoint, *herPoint;
  int myThingIdx, herThingIdx, ci;
  push_t dist, dir[3], drag, fvec[3], sumFvec[3];

  myBin = task->pctx->bin[myBinIdx];
  for (myThingIdx=0; myThingIdx<myBin->numThing; myThingIdx++) {
    myThing = myBin->thing[myThingIdx];
    myPoint = myThing->point + 0;

    ELL_3V_SET(sumFvec, 0, 0, 0);

    /* go through all things in all neighboring bins */
    neighbor = myBin->neighbor;
    while ((herBin = *neighbor)) {
      for (herThingIdx=0; herThingIdx<herBin->numThing; herThingIdx++) {
        herThing = herBin->thing[herThingIdx];
        if (myThing == herThing) {
          continue;
        }
        herPoint = herThing->point + 0;
        _pushPairwiseForce(task->pctx, fvec, task->pctx->force,
                           myPoint, herPoint);
        ELL_3V_INCR(sumFvec, fvec);
      }
      neighbor++;
    }
  
    /* drag */
    if (task->pctx->minIter
        && task->pctx->iter < task->pctx->minIter) {
      drag = AIR_AFFINE(0, task->pctx->iter, task->pctx->minIter,
                        task->pctx->preDrag, task->pctx->drag);
    } else {
      drag = task->pctx->drag;
    }
    ELL_3V_SCALE_INCR(sumFvec, -drag, myPoint->vel);
    
    /* nudging towards image center */
    if (task->pctx->nudge) {
      ELL_3V_NORM(dir, myPoint->pos, dist);
      if (dist) {
        ELL_3V_SCALE_INCR(sumFvec, -task->pctx->nudge*dist, dir);
      }
    }
    
    /* walls around simulation domain */
    if (task->pctx->wall) {
      for (ci=0; ci<=2; ci++) {
        dist = myPoint->pos[ci] - task->pctx->minPos[ci];
        if (dist < 0) {
          sumFvec[ci] += -task->pctx->wall*dist;
        } else {
          dist = task->pctx->maxPos[ci] - myPoint->pos[ci];
          if (dist < 0) {
            sumFvec[ci] += task->pctx->wall*dist;
          }
        }
      }
    }   /* if (task->pctx->wall) */

    ELL_3V_COPY(myPoint->frc, sumFvec);
  }
  return;
}

void
_pushUpdate(pushTask *task, int binIdx,
            const push_t parm[PUSH_STAGE_PARM_MAXNUM]) {
  int thingIdx;
  double step, mass;
  pushBin *bin;
  pushThing *thing;
  pushPoint *point;

  step = task->pctx->step;
  mass = task->pctx->mass;
  bin = task->pctx->bin[binIdx];
  for (thingIdx=0; thingIdx<bin->numThing; thingIdx++) {
    thing = bin->thing[thingIdx];
    task->numThing++;
    if (1 == thing->numPoint) {
      point = thing->point + 0;
      ELL_3V_SCALE_INCR(point->pos, step, point->vel);
      ELL_3V_SCALE_INCR(point->vel, step/mass, point->frc);
      task->sumVel += ELL_3V_LEN(point->vel);
      _pushProbe(task, point);
    }
  }
  return;
}

int
pushRun(pushContext *pctx) {
  char me[]="pushRun", err[AIR_STRLEN_MED],
    poutS[AIR_STRLEN_MED], toutS[AIR_STRLEN_MED], soutS[AIR_STRLEN_MED];
  Nrrd *npos, *nten, *nstn;
  double vel[2], meanVel;

  pctx->iter = 0;
  pctx->time0 = airTime();
  vel[0] = AIR_NAN;
  vel[1] = AIR_NAN;
  do {
    if (pushIterate(pctx)) {
      sprintf(err, "%s: trouble on iter %d", me, pctx->iter);
      biffAdd(PUSH, err); return 1;
    }
    /* this goofiness is because it seems like my stupid Euler 
       integration can lead to real motion only happening on
       every other iteration ... */
    if (0 == pctx->iter) {
      vel[0] = pctx->meanVel;
      meanVel = pctx->meanVel;
    } else if (1 == pctx->iter) {
      vel[1] = pctx->meanVel;
      meanVel = (vel[0] + vel[1])/2;
    } else {
      vel[0] = vel[1];
      vel[1] = pctx->meanVel;
      meanVel = (vel[0] + vel[1])/2;
    }
    if (pctx->snap && !(pctx->iter % pctx->snap)) {
      nten = nrrdNew();
      npos = nrrdNew();
      nstn = nrrdNew();
      sprintf(poutS, "snap.%06d.pos.nrrd", pctx->iter);
      sprintf(toutS, "snap.%06d.ten.nrrd", pctx->iter);
      sprintf(soutS, "snap.%06d.stn.nrrd", pctx->iter);
      if (pushOutputGet(npos, nten, nstn, pctx)) {
        sprintf(err, "%s: couldn't get snapshot for iter %d", me, pctx->iter);
        biffAdd(PUSH, err); return 1;
      }
      fprintf(stderr, "%s: %s, meanVel=%g, %g iter/sec\n", me,
              poutS, meanVel, pctx->iter/(airTime()-pctx->time0));
      if (nrrdSave(poutS, npos, NULL)
          || nrrdSave(toutS, nten, NULL)
          || nrrdSave(soutS, nstn, NULL)) {
        sprintf(err, "%s: couldn't save snapshot for iter %d", me, pctx->iter);
        biffMove(PUSH, err, NRRD); return 1;
      }
      nten = nrrdNuke(nten);
      npos = nrrdNuke(npos);
      nstn = nrrdNuke(nstn);
    }
    pctx->iter++;
  } while ( (pctx->iter < pctx->minIter)
            || (meanVel > pctx->minMeanVel
                && (0 == pctx->maxIter
                    || pctx->iter < pctx->maxIter)) );
  pctx->time1 = airTime();
  pctx->time = pctx->time1 - pctx->time0;

  return 0;
}
