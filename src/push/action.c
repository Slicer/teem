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

/*
** sets tenAns, cntAns
*/
int
_pushInputProcess(pushContext *pctx) {
  char me[]="_pushInputProcess", err[AIR_STRLEN_MED];
  Nrrd *seven[7], *two[2];
  Nrrd *ntmp;
  NrrdRange *nrange;
  airArray *mop;
  int E, ii, nn;
  gagePerVolume *tpvl, *mpvl;
  float *tdata, maxEval, eval[3];

  mop = airMopNew();

  /* ------------------------ fill pctx->nten, check mask range */
  ntmp = nrrdNew();
  airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
  E = AIR_FALSE;
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

  /* ------------------------ set up gage and answer pointers */
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
    biffMove(PUSH, err, GAGE); airMopError(mop); return 1;
  }
  pctx->tenAns = gageAnswerPointer(pctx->gctx, tpvl, tenGageTensor);
  pctx->cntAns = gageAnswerPointer(pctx->gctx, mpvl, gageSclGradVec);
  gageParmSet(pctx->gctx, gageParmRequireAllSpacings, AIR_TRUE);

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
  pctx->pidx = (int**)calloc(pctx->numBin, sizeof(int*));
  pctx->pidxArr = (airArray**)calloc(pctx->numBin, sizeof(airArray*));
  if (!( pctx->pidx && pctx->pidxArr )) {
    sprintf(err, "%s: trouble allocating pidx arrays", me);
    biffAdd(PUSH, err); airMopError(mop); return 1;
  }
  for (ii=0; ii<pctx->numBin; ii++) {
    pctx->pidx[ii] = NULL;
    pctx->pidxArr[ii] = airArrayNew((void **)&(pctx->pidx[ii]), NULL,
                                    sizeof(int), PUSH_PIDX_INCR);
  }
  /* we can do binning now- but it will be rebinned post-initialization */
  _pushBinPointsAllAdd(pctx);

  /* ------------------------ other stuff */
  ELL_3V_SCALE(pctx->minPos, -1, pctx->gctx->shape->volHalfLen);
  ELL_3V_SCALE(pctx->maxPos, 1, pctx->gctx->shape->volHalfLen);

  airMopOkay(mop);
  return 0;
}

int
pushOutputGet(Nrrd *nPosOut, Nrrd *nTenOut, pushContext *pctx) {
  char me[]="pushOutputGet", err[AIR_STRLEN_MED];
  int min[2], max[2], numPoint, E;
  Nrrd *ntmp, *four[4];
  airArray *mop;

  mop = airMopNew();
  numPoint = pctx->nPointAttr->axis[1].size;
  if (nPosOut) {
    min[0] = PUSH_POS;
    min[1] = 0;
    max[0] = PUSH_POS + (2 == pctx->dimIn ? 1 : 2);
    max[1] = numPoint - 1;
    if (nrrdCrop(nPosOut, pctx->nPointAttr, min, max)) {
      sprintf(err, "%s: couldn't crop to recover position output", me);
      biffMove(PUSH, err, NRRD); airMopError(mop); return 1;
    }
  }
  if (nTenOut) {
    ntmp = NULL;
    if (2 == pctx->dimIn) {
      ntmp = nrrdNew();
      airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
    }
    min[0] = PUSH_TEN;
    min[1] = 0;
    max[0] = PUSH_TEN + 6;
    max[1] = numPoint - 1;
    if (nrrdCrop((2 == pctx->dimIn
                  ? ntmp
                  : nTenOut), pctx->nPointAttr, min, max)) {
      sprintf(err, "%s: couldn't crop to recover tensor output", me);
      biffMove(PUSH, err, NRRD); airMopError(mop); return 1;
    }
    if (2 == pctx->dimIn) {
      four[0] = nrrdNew();
      airMopAdd(mop, four[0], (airMopper)nrrdNuke, airMopAlways);
      four[1] = nrrdNew();
      airMopAdd(mop, four[1], (airMopper)nrrdNuke, airMopAlways);
      four[2] = nrrdNew();
      airMopAdd(mop, four[2], (airMopper)nrrdNuke, airMopAlways);
      four[3] = nrrdNew();
      airMopAdd(mop, four[3], (airMopper)nrrdNuke, airMopAlways);
      /*    (0)         (0)
       *     1  2  3     1  2
       *        4  5        3
       *           6            */
      E = AIR_FALSE;
      if (!E) E |= nrrdSlice(four[0], ntmp, 0, 0);
      if (!E) E |= nrrdSlice(four[1], ntmp, 0, 1);
      if (!E) E |= nrrdSlice(four[2], ntmp, 0, 2);
      if (!E) E |= nrrdSlice(four[3], ntmp, 0, 4);
      if (!E) E |= nrrdJoin(nTenOut, (const Nrrd *const *)four,
                            4, 0, AIR_TRUE);
      if (E) {
        sprintf(err, "%s: trouble generating 2D tensor output", me);
        biffMove(PUSH, err, NRRD); airMopError(mop); return 1;
      }
    }
  }

  airMopOkay(mop);
  return 0;
}

void
_pushProbe(pushContext *pctx, gageContext *gctx, push_t pos[3]) {
  double xi, yi, zi;
  int max0, max1, max2;

  /* HEY: this assumes node centering */
  max0 = pctx->nten->axis[1].size - 1;
  max1 = pctx->nten->axis[2].size - 1;
  max2 = pctx->nten->axis[3].size - 1;
  xi = AIR_AFFINE(pctx->minPos[0], pos[0], pctx->maxPos[0], 0, max0);
  yi = AIR_AFFINE(pctx->minPos[1], pos[1], pctx->maxPos[1], 0, max1);
  zi = AIR_AFFINE(pctx->minPos[2], pos[2], pctx->maxPos[2], 0, max2);
  xi = AIR_CLAMP(0, xi, max0);
  yi = AIR_CLAMP(0, yi, max1);
  zi = AIR_CLAMP(0, zi, max2);
  gageProbe(gctx, xi, yi, zi);
  return;
}

/*
** for now, don't get clever and take point list indices instead
** of point information- may need to probe between arc vertices 
** to get intermediate tensors
*/
void
_pushForceCalc(pushContext *pctx, push_t fvec[3], pushForce *force,
               push_t *myAttr, push_t *herAttr) {
  char me[]="_pushForceIncr";
  push_t ten[7], inv[7], dot;
  float haveDist, restDist, mm, fix, mag,
    D[3], nD[3], lenD, 
    U[3], nU[3], lenU, 
    V[3], lenV;

  /* in case lenD > maxDist */
  ELL_3V_SET(fvec, 0, 0, 0);

  ELL_3V_SUB(D, myAttr + PUSH_POS, herAttr + PUSH_POS);
  ELL_3V_NORM(nD, D, lenD);
  if (!lenD) {
    fprintf(stderr, "%s: myPos == herPos == (%g,%g,%g)\n", me,
            (myAttr + PUSH_POS)[0],
            (myAttr + PUSH_POS)[1], 
            (myAttr + PUSH_POS)[2]);
    return;
  }
  if (lenD <= pctx->maxDist) {
    TEN_T_SCALE_ADD2(ten,
                     0.5, myAttr + PUSH_TEN,
                     0.5, herAttr + PUSH_TEN);
    pushTenInv(pctx, inv, ten);
    TEN_TV_MUL(U, inv, D);
    ELL_3V_NORM(nU, U, lenU);
    dot = ELL_3V_DOT(nU, nD);
    haveDist = dot*lenD;
    restDist = dot*2*pctx->scale*lenD/lenU;
    mag = force->func(haveDist, restDist, pctx->scale, force->parm);
    ELL_3V_SCALE(fvec, mag, nU);

    if (pctx->driftCorrect) {
      TEN_TV_MUL(V, myAttr + PUSH_INV, D);
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
_pushRepel(pushTask *task, int bin,
           const push_t parm[PUSH_STAGE_PARM_MAXNUM]) {
  push_t *attr, *velAcc, *attrI, *attrJ, fvec[3], sumFvec[3],
    dist, dir[3], drag;
  int *neiPidx, *myPidx, nei[27], ni, numNei, jj, ii, ci, pidxJ, pidxI,
    myPidxArrLen, neiPidxArrLen;
  /* push_t mid[3]; */

  attr = (push_t *)task->pctx->nPointAttr->data;
  velAcc = (push_t *)task->pctx->nVelAcc->data;
  myPidx = task->pctx->pidx[bin];
  myPidxArrLen = task->pctx->pidxArr[bin]->len;
  numNei = _pushBinNeighborhoodFind(task->pctx, nei, bin, task->pctx->dimIn);
  for (ii=0; ii<myPidxArrLen; ii++) {
    pidxI = myPidx[ii];
    attrI = attr + PUSH_ATTR_LEN*pidxI;

    /* initialize force accumulator */
    ELL_3V_SET(sumFvec, 0, 0, 0);

    /* go through pairs */
    for (ni=0; ni<numNei; ni++) {
      neiPidx = task->pctx->pidx[nei[ni]];
      neiPidxArrLen = task->pctx->pidxArr[nei[ni]]->len;
      for (jj=0; jj<neiPidxArrLen; jj++) {
        pidxJ = neiPidx[jj];
        if (pidxI == pidxJ) {
          continue;
        }
        attrJ = attr + PUSH_ATTR_LEN*pidxJ;
        _pushForceCalc(task->pctx, fvec, task->pctx->force, attrI, attrJ);
        /*
        ELL_3V_SCALE_ADD2(mid, 0.5, attrI + PUSH_POS, 0.5, attrJ + PUSH_POS);
        if (ELL_3V_LEN(fvec)
            && AIR_IN_OP(-0.26, mid[0], -0.24)
            && AIR_IN_OP(0.08, mid[1], 0.10)) {
          fprintf(stderr, "!%s: @(%g,%g) : %d <--- %d : (%f,%f) %30.15f\n",
                  "_pushRepel",
                  mid[0], mid[1],
                  pidxI, pidxJ, fvec[0], fvec[1],
                  ELL_3V_LEN(fvec));
        }
        */
        /*
          if (ELL_3V_LEN(fvec)) {
          fprintf(stderr, "!%s: %d <---> %d : %g\n",
                  "_pushRepel", pidxI, pidxJ, ELL_3V_LEN(fvec));
        }
        */
        ELL_3V_INCR(sumFvec, fvec);
      }
    }

    /* drag */
    if (task->pctx->minIter
        && task->pctx->iter < task->pctx->minIter) {
      drag = AIR_AFFINE(0, task->pctx->iter, task->pctx->minIter,
                        task->pctx->preDrag, task->pctx->drag);
    } else {
      drag = task->pctx->drag;
    }
    ELL_3V_SCALE_INCR(sumFvec, -drag, attrI + PUSH_VEL);

    /* nudging towards image center */
    if (task->pctx->nudge) {
      ELL_3V_NORM(dir, attrI + PUSH_POS, dist);
      if (dist) {
        ELL_3V_SCALE_INCR(sumFvec, -task->pctx->nudge*dist, dir);
      }
    }
    if (task->pctx->wall) {
      for (ci=0; ci<=2; ci++) {
        dist = (attrI + PUSH_POS)[ci] - task->pctx->minPos[ci];
        if (dist < 0) {
          sumFvec[ci] += -task->pctx->wall*dist;
        } else {
          dist = task->pctx->maxPos[ci] - (attrI + PUSH_POS)[ci];
          if (dist < 0) {
            sumFvec[ci] += task->pctx->wall*dist;
          }
        }
      }
    }
    /*
    if (ELL_3V_LEN(sumFvec)) {
      fprintf(stderr, "!%s: %d(%d) : (%g,%g,%g)\n",
              "_pushRepel", pidxI, bin, 
              sumFvec[0], sumFvec[1], sumFvec[2]);
    }
    */

    /* copy results to tmp world */
    ELL_3V_COPY(velAcc + 3*(0 + 2*pidxI), attrI + PUSH_VEL);
    ELL_3V_SCALE(velAcc + 3*(1 + 2*pidxI), 1.0/task->pctx->mass, sumFvec);
  }
  return;
}

void
_pushUpdate(pushTask *task, int bin,
            const push_t parm[PUSH_STAGE_PARM_MAXNUM]) {
  push_t *attrData, *attr, *velAccData, *oldVel, *oldAcc;
  int *pidx, pidxLen, pidxI, pii;
  double step;

  step = task->pctx->step;
  velAccData = (push_t *)task->pctx->nVelAcc->data;
  attrData = (push_t *)task->pctx->nPointAttr->data;
  pidx = task->pctx->pidx[bin];
  pidxLen = task->pctx->pidxArr[bin]->len;
  for (pii=0; pii<pidxLen; pii++) {
    pidxI = pidx[pii];
    attr =  attrData + PUSH_ATTR_LEN*pidxI;
    oldVel = velAccData + 3*(0 + 2*pidxI);
    oldAcc = velAccData + 3*(1 + 2*pidxI);
    ELL_3V_SCALE_INCR(attr + PUSH_POS, step, oldVel);
    ELL_3V_SCALE_INCR(attr + PUSH_VEL, step, oldAcc);
    task->sumVel += ELL_3V_LEN(attr + PUSH_VEL);
    _pushProbe(task->pctx, task->gctx, attr + PUSH_POS);
    TEN_T_COPY(attr + PUSH_TEN, task->tenAns);
    pushTenInv(task->pctx, attr + PUSH_INV, attr + PUSH_TEN);
    ELL_3V_COPY(attr + PUSH_CNT, task->cntAns);
  }
  return;
}

void
_pushInitialize(pushContext *pctx) {
  int numPoint, pi;
  push_t *attr;
  double (*lup)(const void *v, size_t I);

  /*
  {
    Nrrd *ntmp;
    double *data;
    int sx, sy, xi, yi;
    push_t pos[3];
    
    sx = 30;
    sy = 30;
    ntmp = nrrdNew();
    nrrdMaybeAlloc(ntmp, nrrdTypeDouble, 3, 4, sx, sy);
    data = (double*)ntmp->data;
    pos[2] = 0.0;
    for (yi=0; yi<sy; yi++) {
      pos[1] = AIR_AFFINE(0, yi, sy-1, pctx->minPos[1], pctx->maxPos[1]);
      for (xi=0; xi<sx; xi++) {
        pos[0] = AIR_AFFINE(0, xi, sx-1, pctx->minPos[0], pctx->maxPos[0]);
        _pushProbe(pctx, pctx->gctx, pos);
        data[0 + 4*(xi + sx*yi)] = pctx->tenAns[0];
        data[1 + 4*(xi + sx*yi)] = pctx->tenAns[1];
        data[2 + 4*(xi + sx*yi)] = pctx->tenAns[2];
        data[3 + 4*(xi + sx*yi)] = pctx->tenAns[4];
      }
    }
    nrrdSave("pray.nrrd", ntmp, NULL);
  }
  */
  
  numPoint = pctx->nPointAttr->axis[1].size;
  lup = pctx->npos ? nrrdDLookup[pctx->npos->type] : NULL;
  for (pi=0; pi<numPoint; pi++) {
    attr = (push_t *)(pctx->nPointAttr->data) + PUSH_ATTR_LEN*pi;
    if (pctx->npos) {
      ELL_3V_SET(attr + PUSH_POS,
                 lup(pctx->npos->data, 0 + 3*pi),
                 lup(pctx->npos->data, 1 + 3*pi),
                 lup(pctx->npos->data, 2 + 3*pi));
      _pushProbe(pctx, pctx->gctx, attr + PUSH_POS);
      TEN_T_COPY(attr + PUSH_TEN, pctx->tenAns);
    } else {
      do {
        (attr + PUSH_POS)[0] = AIR_AFFINE(0.0, airDrand48(), 1.0,
                                          pctx->minPos[0], pctx->maxPos[0]);
        (attr + PUSH_POS)[1] = AIR_AFFINE(0.0, airDrand48(), 1.0,
                                          pctx->minPos[1], pctx->maxPos[1]);
        if (2 == pctx->dimIn) {
          (attr + PUSH_POS)[2] = 0;
        } else {
          (attr + PUSH_POS)[2] = AIR_AFFINE(0.0, airDrand48(), 1.0,
                                            pctx->minPos[2], pctx->maxPos[2]);
        }
        _pushProbe(pctx, pctx->gctx, attr + PUSH_POS);
        TEN_T_COPY(attr + PUSH_TEN, pctx->tenAns);
      } while ((attr + PUSH_TEN)[0] < 0.5);
    }
    ELL_3V_SET(attr + PUSH_VEL, 0, 0, 0);
    pushTenInv(pctx, attr + PUSH_INV, attr + PUSH_TEN);
    ELL_3V_COPY(attr + PUSH_CNT, pctx->cntAns);
  }
  /* do rebinning, now that we have positions */
  _pushBinPointsRebin(pctx);


  {
    Nrrd *ntmp;
    double *data;
    int sx, sy, np, xi, yi, pi;
    push_t fsum[3], fvec[3], attrTmp[PUSH_ATTR_LEN];


    sx = 300;
    sy = 300;
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
          _pushForceCalc(pctx, fvec, pctx->force, attrTmp, attr);
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
  }


  /* HEY: this should be done by the user */
  pctx->process[0] = _pushRepel;
  pctx->process[1] = _pushUpdate;

  return;
}

int
pushRun(pushContext *pctx) {
  char me[]="pushRun", err[AIR_STRLEN_MED],
    poutS[AIR_STRLEN_MED], toutS[AIR_STRLEN_MED];
  Nrrd *npos, *nten;
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
      sprintf(poutS, "snap.%06d.pos.nrrd", pctx->iter);
      sprintf(toutS, "snap.%06d.ten.nrrd", pctx->iter);
      if (pushOutputGet(npos, nten, pctx)) {
        sprintf(err, "%s: couldn't get snapshot for iter %d", me, pctx->iter);
        biffAdd(PUSH, err); return 1;
      }
      fprintf(stderr, "%s: %s, meanVel=%g, %g iter/sec\n", me,
              poutS, meanVel, pctx->iter/(airTime()-pctx->time0));
      if (nrrdSave(poutS, npos, NULL)
          || nrrdSave(toutS, nten, NULL)) {
        sprintf(err, "%s: couldn't save snapshot for iter %d", me, pctx->iter);
        biffMove(PUSH, err, NRRD); return 1;
      }
      nten = nrrdNuke(nten);
      npos = nrrdNuke(npos);
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
