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

/* 
** _pushTensorFieldSetup sets:
**** pctx->dimIn
**** pctx->nten
**** pctx->ninv
**** pctx->nmask
** and checks mask range
*/
int
_pushTensorFieldSetup(pushContext *pctx) {
  char me[]="_pushTensorFieldSetup", err[BIFF_STRLEN];
  NrrdRange *nrange;
  airArray *mop;
  Nrrd *ntmp;
  int E;
  float *_ten, *_inv;
  double ten[7], inv[7];
  unsigned int numSingle;
  size_t ii, NN;

  mop = airMopNew();
  ntmp = nrrdNew();
  airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
  pctx->nten = nrrdNew();
  pctx->ninv = nrrdNew();
  pctx->nmask = nrrdNew();
  numSingle = 0;
  numSingle += (1 == pctx->nin->axis[1].size);
  numSingle += (1 == pctx->nin->axis[2].size);
  numSingle += (1 == pctx->nin->axis[3].size);
  if (1 == numSingle) {
    pctx->dimIn = 2;
    pctx->sliceAxis = (1 == pctx->nin->axis[1].size
                       ? 0
                       : (1 == pctx->nin->axis[2].size
                          ? 1
                          : 2));
    fprintf(stderr, "!%s: got 2-D input with sliceAxis %u\n",
            me, pctx->sliceAxis);
  } else {
    pctx->dimIn = 3;
    pctx->sliceAxis = 5280;
    fprintf(stderr, "!%s: got 3-D input\n", me);
  }
  E = AIR_FALSE;
  E = nrrdConvert(pctx->nten, pctx->nin, nrrdTypeFloat);
  
  /* set up ninv from nten */
  if (!E) E |= nrrdCopy(pctx->ninv, pctx->nten);
  if (E) {
    sprintf(err, "%s: trouble creating 3D tensor input", me);
    biffMove(PUSH, err, NRRD); airMopError(mop); return 1;
  }
  _ten = (float*)pctx->nten->data;
  _inv = (float*)pctx->ninv->data;
  NN = nrrdElementNumber(pctx->nten)/7;
  for (ii=0; ii<NN; ii++) {
    double det;
    TEN_T_COPY(ten, _ten);
    TEN_T_INV(inv, ten, det);
    TEN_T_COPY(_inv, inv);
    _ten += 7;
    _inv += 7;
  }

  if (!E) E |= nrrdSlice(pctx->nmask, pctx->nten, 0, 0);
  if (E) {
    sprintf(err, "%s: trouble creating mask", me);
    biffMove(PUSH, err, NRRD); airMopError(mop); return 1;
  }
  nrange = nrrdRangeNewSet(pctx->nmask, nrrdBlind8BitRangeFalse);
  airMopAdd(mop, nrange, (airMopper)nrrdRangeNix, airMopAlways);
  if (AIR_ABS(1.0 - nrange->max) > 0.005) {
    sprintf(err, "%s: tensor mask max %g not close 1.0", me, nrange->max);
    biffAdd(PUSH, err); airMopError(mop); return 1;
  }

  /*
  pctx->nten->axis[1].spacing = (AIR_EXISTS(pctx->nten->axis[1].spacing)
                                 ? pctx->nten->axis[1].spacing
                                 : 1.0);
  pctx->nten->axis[2].spacing = (AIR_EXISTS(pctx->nten->axis[2].spacing)
                                 ? pctx->nten->axis[2].spacing
                                 : 1.0);
  pctx->nten->axis[3].spacing = (AIR_EXISTS(pctx->nten->axis[3].spacing)
                                 ? pctx->nten->axis[3].spacing
                                 : 1.0);
  pctx->ninv->axis[1].spacing = pctx->nten->axis[1].spacing;
  pctx->ninv->axis[2].spacing = pctx->nten->axis[2].spacing;
  pctx->ninv->axis[3].spacing = pctx->nten->axis[3].spacing;
  pctx->nmask->axis[0].spacing = pctx->nten->axis[1].spacing;
  pctx->nmask->axis[1].spacing = pctx->nten->axis[2].spacing;
  pctx->nmask->axis[2].spacing = pctx->nten->axis[3].spacing;
  */
  pctx->nten->axis[1].center = nrrdCenterCell;
  pctx->nten->axis[2].center = nrrdCenterCell;
  pctx->nten->axis[3].center = nrrdCenterCell;
  pctx->ninv->axis[1].center = nrrdCenterCell;
  pctx->ninv->axis[2].center = nrrdCenterCell;
  pctx->ninv->axis[3].center = nrrdCenterCell;
  pctx->nmask->axis[0].center = nrrdCenterCell;
  pctx->nmask->axis[1].center = nrrdCenterCell;
  pctx->nmask->axis[2].center = nrrdCenterCell;

  airMopOkay(mop); 
  return 0;
}

/*
** _pushGageSetup sets:
**** pctx->gctx
*/
int
_pushGageSetup(pushContext *pctx) {
  char me[]="_pushGageSetup", err[BIFF_STRLEN];
  gagePerVolume *mpvl;
  int E;

  pctx->gctx = gageContextNew();
  E = AIR_FALSE;
  /* set up tensor probing */
  if (!E) E |= !(pctx->tpvl = gagePerVolumeNew(pctx->gctx,
                                               pctx->nten, tenGageKind));
  if (!E) E |= gagePerVolumeAttach(pctx->gctx, pctx->tpvl);
  if (!E) E |= gageKernelSet(pctx->gctx, gageKernel00,
                             pctx->ksp00->kernel, pctx->ksp00->parm);
  if (!E) E |= gageQueryItemOn(pctx->gctx, pctx->tpvl, tenGageTensor);
  if (tenGageUnknown != pctx->gravItem) {
    if (!E) E |= gageQueryItemOn(pctx->gctx, pctx->tpvl, pctx->gravItem);
  }
  if (tenGageUnknown != pctx->gravNotItem[0]) {
    if (!E) E |= gageQueryItemOn(pctx->gctx, pctx->tpvl, pctx->gravNotItem[0]);
  }
  if (tenGageUnknown != pctx->gravNotItem[1]) {
    if (!E) E |= gageQueryItemOn(pctx->gctx, pctx->tpvl, pctx->gravNotItem[1]);
  }
  /* set up tensor inverse probing */
  if (!E) E |= !(pctx->ipvl = gagePerVolumeNew(pctx->gctx,
                                               pctx->ninv, tenGageKind));
  if (!E) E |= gagePerVolumeAttach(pctx->gctx, pctx->ipvl);
  if (!E) E |= gageQueryItemOn(pctx->gctx, pctx->ipvl, tenGageTensor);
  /* set up mask gradient probing */
  if (!E) E |= !(mpvl = gagePerVolumeNew(pctx->gctx,
                                         pctx->nmask, gageKindScl));
  if (!E) E |= gagePerVolumeAttach(pctx->gctx, mpvl);
  if (!E) E |= gageKernelSet(pctx->gctx, gageKernel11,
                             pctx->ksp11->kernel, pctx->ksp11->parm);
  if (!E) E |= gageKernelSet(pctx->gctx, gageKernel22,
                             pctx->ksp22->kernel, pctx->ksp22->parm);
  if (!E) E |= gageQueryItemOn(pctx->gctx, mpvl, gageSclGradVec);
  /* (maybe) turn on seed thresholding */
  if (tenGageUnknown != pctx->seedThreshItem) {
    if (!E) E |= gageQueryItemOn(pctx->gctx, pctx->tpvl, pctx->seedThreshItem);
  }
  /* HEY: seed threshold item should possibly be turned off later! */
  if (!E) E |= gageUpdate(pctx->gctx);
  if (E) {
    sprintf(err, "%s: trouble setting up gage", me);
    biffMove(PUSH, err, GAGE); return 1;
  }
  gageParmSet(pctx->gctx, gageParmRequireAllSpacings, AIR_TRUE);

  return 0;
}

/*
** _pushFiberSetup sets:
**** pctx->fctx
*/
int
_pushFiberSetup(pushContext *pctx) {
  char me[]="_pushFiberSetup", err[BIFF_STRLEN];
  int E;

  pctx->fctx = tenFiberContextNew(pctx->nten);
  if (!pctx->fctx) { 
    sprintf(err, "%s: couldn't create fiber context", me);
    biffMove(PUSH, err, TEN); return 1;
  }
  E = AIR_FALSE;
  if (!E) E |= tenFiberStopSet(pctx->fctx, tenFiberStopNumSteps,
                               pctx->tlStepNum);
  if (!E) E |= tenFiberStopSet(pctx->fctx, tenFiberStopAniso,
                               tenAniso_Cl1,
                               pctx->tlThresh - pctx->tlSoft);
  if (!E) E |= tenFiberTypeSet(pctx->fctx, tenFiberTypeEvec1);
  if (!E) E |= tenFiberKernelSet(pctx->fctx,
                                 pctx->ksp00->kernel, pctx->ksp00->parm);
  /* if (!E) E |= tenFiberIntgSet(pctx->fctx, tenFiberIntgRK4); */
  if (!E) E |= tenFiberIntgSet(pctx->fctx, tenFiberIntgMidpoint);
  /* if (!E) E |= tenFiberIntgSet(pctx->fctx, tenFiberIntgEuler); */
  if (!E) E |= tenFiberParmSet(pctx->fctx, tenFiberParmStepSize,
                               pctx->tlStep/pctx->tlStepNum);
  if (!E) E |= tenFiberAnisoSpeedSet(pctx->fctx, tenAniso_Cl1,
                                     1 /* lerp */ ,
                                     pctx->tlThresh /* thresh */,
                                     pctx->tlSoft);
  if (!E) E |= tenFiberUpdate(pctx->fctx);
  if (E) {
    sprintf(err, "%s: trouble setting up fiber context", me);
    biffMove(PUSH, err, TEN); return 1;
  }
  return 0;
}

pushTask *
_pushTaskNew(pushContext *pctx, int threadIdx) {
  pushTask *task;

  task = (pushTask *)calloc(1, sizeof(pushTask));
  if (task) {
    task->pctx = pctx;
    task->gctx = gageContextCopy(pctx->gctx);
    task->fctx = tenFiberContextCopy(pctx->fctx);
    /* 
    ** HEY: its a limitation in gage that we have to know a priori
    ** the ordering of per-volumes in the context ...
    */
    task->tenAns = gageAnswerPointer(task->gctx, task->gctx->pvl[0],
                                     tenGageTensor);
    task->invAns = gageAnswerPointer(task->gctx, task->gctx->pvl[1],
                                     tenGageTensor);
    task->cntAns = gageAnswerPointer(task->gctx, task->gctx->pvl[2],
                                     gageSclGradVec);
    task->gravAns = (tenGageUnknown != task->pctx->gravItem
                     ? gageAnswerPointer(task->gctx, task->gctx->pvl[0],
                                         task->pctx->gravItem)
                     : NULL);
    task->gravNotAns[0] = (tenGageUnknown != task->pctx->gravNotItem[0]
                           ? gageAnswerPointer(task->gctx, task->gctx->pvl[0],
                                               task->pctx->gravNotItem[0])
                           : NULL);
    task->gravNotAns[1] = (tenGageUnknown != task->pctx->gravNotItem[1]
                           ? gageAnswerPointer(task->gctx, task->gctx->pvl[0],
                                               task->pctx->gravNotItem[1])
                           : NULL);
    task->seedThreshAns = (tenGageUnknown != task->pctx->seedThreshItem
                           ? gageAnswerPointer(task->gctx, task->gctx->pvl[0],
                                               task->pctx->seedThreshItem)
                           : NULL);
    if (threadIdx) {
      task->thread = airThreadNew();
    }
    task->threadIdx = threadIdx;
    task->thingNum = 0;
    task->sumVel = 0;
    task->vertBuff = (double*)calloc(3*(1 + 2*pctx->tlStepNum),
                                     sizeof(double));
    task->returnPtr = NULL;
  }
  return task;
}

pushTask *
_pushTaskNix(pushTask *task) {

  if (task) {
    task->gctx = gageContextNix(task->gctx);
    task->fctx = tenFiberContextNix(task->fctx);
    if (task->threadIdx) {
      task->thread = airThreadNix(task->thread);
    }
    task->vertBuff = (double *)airFree(task->vertBuff);
    airFree(task);
  }
  return NULL;
}

/*
** _pushTaskSetup sets:
**** pctx->task
**** pctx->task[]
*/
int
_pushTaskSetup(pushContext *pctx) {
  char me[]="_pushTaskSetup", err[BIFF_STRLEN];
  unsigned int tidx;

  pctx->task = (pushTask **)calloc(pctx->threadNum, sizeof(pushTask *));
  if (!(pctx->task)) {
    sprintf(err, "%s: couldn't allocate array of tasks", me);
    biffAdd(PUSH, err); return 1;
  }
  for (tidx=0; tidx<pctx->threadNum; tidx++) {
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
**** pctx->maxDist, pctx->minEval, pctx->maxEval, pctx->maxDet
**** pctx->binsEdge[], pctx->binNum
**** pctx->bin
**** pctx->bin[]
*/
int
_pushBinSetup(pushContext *pctx) {
  char me[]="_pushBinSetup", err[BIFF_STRLEN];
  float eval[3], *tdata;
  unsigned int ii, nn, count;
  double col[3][4], volEdge[3];

  /* ------------------------ find maxEval, maxDet, and set up binning */
  nn = nrrdElementNumber(pctx->nten)/7;
  tdata = (float*)pctx->nten->data;
  pctx->maxEval = 0;
  pctx->maxDet = 0;
  pctx->meanEval = 0;
  count = 0;
  for (ii=0; ii<nn; ii++) {
    tenEigensolve_f(eval, NULL, tdata);
    if (tdata[0] > 0.5) {
      /* HEY: this limitation may be a bad idea */
      count++;
      pctx->meanEval += eval[0];
      pctx->maxEval = AIR_MAX(pctx->maxEval, eval[0]);
      pctx->maxDet = AIR_MAX(pctx->maxDet, eval[0] + eval[1] + eval[2]);
    }
    tdata += 7;
  }
  pctx->meanEval /= count;
  pctx->maxDist = (2*pctx->scale*pctx->maxEval
                   *pctx->force->maxDist(pctx->force->parm));
  if (pctx->singleBin) {
    pctx->binsEdge[0] = 1;
    pctx->binsEdge[1] = 1;
    pctx->binsEdge[2] = 1;
    pctx->binNum = 1;
  } else {
    ELL_4MV_COL0_GET(col[0], pctx->gctx->shape->ItoW); col[0][3] = 0.0;
    ELL_4MV_COL1_GET(col[1], pctx->gctx->shape->ItoW); col[1][3] = 0.0;
    ELL_4MV_COL2_GET(col[2], pctx->gctx->shape->ItoW); col[2][3] = 0.0;
    volEdge[0] = ELL_3V_LEN(col[0])*(pctx->gctx->shape->size[0]-1);
    volEdge[1] = ELL_3V_LEN(col[1])*(pctx->gctx->shape->size[1]-1);
    volEdge[2] = ELL_3V_LEN(col[2])*(pctx->gctx->shape->size[2]-1);
    fprintf(stderr, "!%s: volEdge = %g %g %g\n", me,
            volEdge[0], volEdge[1], volEdge[2]);
    pctx->binsEdge[0] = (int)floor(volEdge[0]/pctx->maxDist);
    pctx->binsEdge[0] = pctx->binsEdge[0] ? pctx->binsEdge[0] : 1;
    pctx->binsEdge[1] = (int)floor(volEdge[1]/pctx->maxDist);
    pctx->binsEdge[1] = pctx->binsEdge[1] ? pctx->binsEdge[1] : 1;
    pctx->binsEdge[2] = (int)floor(volEdge[2]/pctx->maxDist);
    pctx->binsEdge[2] = pctx->binsEdge[2] ? pctx->binsEdge[2] : 1;
    if (2 == pctx->dimIn) {
      pctx->binsEdge[pctx->sliceAxis] = 1;
    }
    fprintf(stderr, "!%s: maxEval=%g -> maxDist=%g -> binsEdge=%u,%u,%u\n",
            me, pctx->maxEval, pctx->maxDist,
            pctx->binsEdge[0], pctx->binsEdge[1], pctx->binsEdge[2]);
    pctx->binNum = pctx->binsEdge[0]*pctx->binsEdge[1]*pctx->binsEdge[2];
  }
  pctx->bin = (pushBin *)calloc(pctx->binNum, sizeof(pushBin));
  if (!( pctx->bin )) {
    sprintf(err, "%s: trouble allocating bin arrays", me);
    biffAdd(PUSH, err); return 1;
  }
  for (ii=0; ii<pctx->binNum; ii++) {
    pushBinInit(pctx->bin + ii, pctx->binIncr);
  }
  pushBinAllNeighborSet(pctx);

  return 0;
}

/*
** THIS IS A COMPLETE HACK!!!
*/
int
pushTaskFiberReSetup(pushContext *pctx,
                     double tlThresh, double tlSoft, double tlStep,
                     unsigned int tlStepNum) {
  char me[]="pushTaskFiberReSetup", err[BIFF_STRLEN];
  tenFiberContext *fctx;
  unsigned int taskIdx;
  int E = 0;

  fprintf(stderr, "!%s: %d\n", me, __LINE__);
  if (!pctx->task) {
    fprintf(stderr, "!%s: %d bailing\n", me, __LINE__);
    return 0;
  }
  pctx->tlStepNum = tlStepNum;
  pctx->tlThresh = tlThresh;
  pctx->tlSoft = tlSoft;
  pctx->tlStep = tlStep;

  fprintf(stderr, "!%s: %d\n", me, __LINE__);
  for (taskIdx=0; pctx->threadNum; taskIdx++) {
    fctx = pctx->task[taskIdx]->fctx;
    fprintf(stderr, "!%s: %d %p\n", me, __LINE__, fctx);
    if (!E) E |= tenFiberStopSet(fctx, tenFiberStopNumSteps,
                                 pctx->tlStepNum);
    fprintf(stderr, "!%s: %d %p\n", me, __LINE__, fctx);
    if (!E) E |= tenFiberStopSet(fctx, tenFiberStopAniso,
                                 tenAniso_Cl1,
                                 pctx->tlThresh - pctx->tlSoft);
    fprintf(stderr, "!%s: %d %p\n", me, __LINE__, fctx);
    if (!E) E |= tenFiberParmSet(fctx, tenFiberParmStepSize,
                                 pctx->tlStep/pctx->tlStepNum);
    fprintf(stderr, "!%s: %d %p\n", me, __LINE__, fctx);
    if (!E) E |= tenFiberAnisoSpeedSet(fctx, tenAniso_Cl1,
                                       1 /* lerp */ ,
                                       pctx->tlThresh /* thresh */,
                                       pctx->tlSoft);
    fprintf(stderr, "!%s: %d %p\n", me, __LINE__, fctx);
    if (!E) E |= tenFiberUpdate(fctx);
    fprintf(stderr, "!%s: %d %p\n", me, __LINE__, fctx);
    if (E) {
      sprintf(err, "%s: trouble resetting task %u fiber context", me, taskIdx);
      biffMove(PUSH, err, TEN); return 1;
    }
    fprintf(stderr, "!%s: %d %p\n", me, __LINE__, fctx);
  }
  fprintf(stderr, "!%s: %d\n", me, __LINE__);
  return 1;
}


/*
** _pushThingSetup sets:
**** pctx->thingNum (in case pctx->nstn and/or pctx->npos)
**
** This is only called by the master thread
** 
** this should set stuff to be like after an update stage and
** just before the rebinning
*/
int
_pushThingSetup(pushContext *pctx) {
  char me[]="_pushThingSetup", err[BIFF_STRLEN];
  double (*lup)(const void *v, size_t I);
  unsigned int *stn, pointIdx, baseIdx, thingIdx;
  pushThing *thing;

  pctx->thingNum = (pctx->nstn
                    ? pctx->nstn->axis[1].size
                    : (pctx->npos
                       ? pctx->npos->axis[1].size
                       : pctx->thingNum));
  lup = pctx->npos ? nrrdDLookup[pctx->npos->type] : NULL;
  stn = pctx->nstn ? (unsigned int*)pctx->nstn->data : NULL;
  for (thingIdx=0; thingIdx<pctx->thingNum; thingIdx++) {
    double detProbe;
    /*
    fprintf(stderr, "!%s: thingIdx = %u/%u\n", me, thingIdx, pctx->thingNum);
    */
    if (pctx->nstn) {
      baseIdx = stn[0 + 3*thingIdx];
      thing = pushThingNew(stn[1 + 3*thingIdx]);
      for (pointIdx=0; pointIdx<thing->vertNum; pointIdx++) {
        ELL_3V_SET(thing->vert[pointIdx].pos,
                   lup(pctx->npos->data, 0 + 3*(pointIdx + baseIdx)),
                   lup(pctx->npos->data, 1 + 3*(pointIdx + baseIdx)),
                   lup(pctx->npos->data, 2 + 3*(pointIdx + baseIdx)));
        _pushProbe(pctx->task[0], thing->vert + pointIdx);
        thing->vert[pointIdx].charge = _pushThingPointCharge(pctx, thing);
      }
      thing->seedIdx = stn[2 + 3*thingIdx];
      if (1 < thing->vertNum) {
        /* info about seedpoint has to be set separately */
        ELL_3V_SET(thing->point.pos,
                   lup(pctx->npos->data, 0 + 3*(thing->seedIdx + baseIdx)),
                   lup(pctx->npos->data, 1 + 3*(thing->seedIdx + baseIdx)),
                   lup(pctx->npos->data, 2 + 3*(thing->seedIdx + baseIdx)));
        _pushProbe(pctx->task[0], &(thing->point));
      }
      /*
      fprintf(stderr, "!%s: thingNum(%d) = %d\n", "_pushThingSetup",
              thingIdx, thing->vertNum);
      */
    } else if (pctx->npos) {
      thing = pushThingNew(1);
      ELL_3V_SET(thing->vert[0].pos,
                 lup(pctx->npos->data, 0 + 3*thingIdx),
                 lup(pctx->npos->data, 1 + 3*thingIdx),
                 lup(pctx->npos->data, 2 + 3*thingIdx));
      _pushProbe(pctx->task[0], thing->vert + 0);
      thing->vert[0].charge = _pushThingPointCharge(pctx, thing);
    } else {
      thing = pushThingNew(1);
      do {
        double posIdx[4], posWorld[4];
        posIdx[0] = AIR_AFFINE(0.0, airDrandMT(), 1.0,
                               -0.5, pctx->gctx->shape->size[0]-0.5);
        posIdx[1] = AIR_AFFINE(0.0, airDrandMT(), 1.0,
                               -0.5, pctx->gctx->shape->size[1]-0.5);
        posIdx[2] = AIR_AFFINE(0.0, airDrandMT(), 1.0,
                               -0.5, pctx->gctx->shape->size[2]-0.5);
        posIdx[3] = 1.0;
        if (2 == pctx->dimIn) {
          posIdx[pctx->sliceAxis] = 0.0;
        }
        ELL_4MV_MUL(posWorld, pctx->gctx->shape->ItoW, posIdx);
        ELL_34V_HOMOG(thing->vert[0].pos, posWorld);
        /*
        fprintf(stderr, "%s: posIdx = %g %g %g --> posWorld = %g %g %g "
                "--> %g %g %g\n", me,
                posIdx[0], posIdx[1], posIdx[2],
                posWorld[0], posWorld[1], posWorld[2],
                thing->vert[0].pos[0], thing->vert[0].pos[1],
                thing->vert[0].pos[2]);
        */
        _pushProbe(pctx->task[0], thing->vert + 0);
        detProbe = TEN_T_TRACE(thing->vert[0].ten);
        /* assuming that we're not using some very blurring kernel,
           this will eventually succeed, because we previously checked
           the range of values in the mask */
        /* HEY: can't ensure that this will eventually succeed with
           seedThresh enabled! */
        /*
        fprintf(stderr, "!%s: ten[0] = %g\n", me, thing->vert[0].ten[0]);
        */
        /* we OR together all the things that would
           make us REJECT this last sample */
      } while (thing->vert[0].ten[0] < 0.5
               || (tenGageUnknown != pctx->seedThreshItem
                   && ((pctx->seedThresh - thing->vert[0].seedThresh)
                       *pctx->seedThreshSign > 0)
                   )
               || (pctx->detReject && (airDrandMT() < detProbe/pctx->maxDet))
               );
    }
    for (pointIdx=0; pointIdx<thing->vertNum; pointIdx++) {
      if (pushBinPointAdd(pctx, thing->vert + pointIdx)) {
        sprintf(err, "%s: trouble binning vert %d of thing %d", me,
                pointIdx, thingIdx);
        biffAdd(PUSH, err); return 1;
      }
      ELL_3V_SET(thing->vert[pointIdx].vel, 0, 0, 0);
      thing->vert[pointIdx].charge = _pushThingPointCharge(pctx, thing);
    }
    if (pushBinThingAdd(pctx, thing)) {
      sprintf(err, "%s: trouble thing %d", me, thingIdx);
      biffAdd(PUSH, err); return 1;
    }
  }
  /*
  {
    Nrrd *nten, *npos, *nstn;
    char me[]="dammit", err[BIFF_STRLEN], poutS[AIR_STRLEN_MED],
      toutS[AIR_STRLEN_MED], soutS[AIR_STRLEN_MED];
      nten = nrrdNew();
      npos = nrrdNew();
      nstn = nrrdNew();
      sprintf(poutS, "snap-pre.%06d.pos.nrrd", -1);
      sprintf(toutS, "snap-pre.%06d.ten.nrrd", -1);
      sprintf(soutS, "snap-pre.%06d.stn.nrrd", -1);
      if (pushOutputGet(npos, nten, nstn, pctx)) {
        sprintf(err, "%s: couldn't get snapshot for iter %d", me, -1);
        biffAdd(PUSH, err); return 1;
      }
      if (nrrdSave(poutS, npos, NULL)
          || nrrdSave(toutS, nten, NULL)
          || nrrdSave(soutS, nstn, NULL)) {
        sprintf(err, "%s: couldn't save snapshot for iter %d", me, -1);
        biffMove(PUSH, err, NRRD); return 1;
      }
      nten = nrrdNuke(nten);
      npos = nrrdNuke(npos);
      nstn = nrrdNuke(nstn);
  }
  */
  return 0;
}

