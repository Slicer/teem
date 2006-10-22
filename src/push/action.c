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

unsigned int
_pushThingTotal(pushContext *pctx) {
  unsigned int binIdx, thingNum;

  thingNum = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    thingNum += pctx->bin[binIdx].thingNum;
  }
  return thingNum;
}

unsigned int
_pushPointTotal(pushContext *pctx) {
  unsigned int binIdx, thingIdx, pointNum;
  pushBin *bin;

  pointNum = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    for (thingIdx=0; thingIdx<bin->thingNum; thingIdx++) {
      pointNum += bin->thing[thingIdx]->vertNum;
    }
  }
  return pointNum;
}

int
_pushProbe(pushTask *task, pushPoint *point) {
  /* char me[]="_pushProbe"; */
  double eval[3], sum, posWorld[4], posIdx[4];
  int inside, ret;

  ELL_3V_COPY(posWorld, point->pos); posWorld[3] = 1.0;
  ELL_4MV_MUL(posIdx, task->gctx->shape->WtoI, posWorld);
  ELL_4V_HOMOG(posIdx, posIdx);
  inside = (AIR_IN_OP(-0.5, posIdx[0], task->gctx->shape->size[0]-0.5) &&
            AIR_IN_OP(-0.5, posIdx[1], task->gctx->shape->size[1]-0.5) &&
            AIR_IN_OP(-0.5, posIdx[2], task->gctx->shape->size[2]-0.5));
  if (!inside) {
    posIdx[0] = AIR_CLAMP(-0.5, posIdx[0], task->gctx->shape->size[0]-0.5);
    posIdx[1] = AIR_CLAMP(-0.5, posIdx[1], task->gctx->shape->size[1]-0.5);
    posIdx[2] = AIR_CLAMP(-0.5, posIdx[2], task->gctx->shape->size[2]-0.5);
  }
  ret = gageProbe(task->gctx, posIdx[0], posIdx[1], posIdx[2]);

  TEN_T_COPY(point->ten, task->tenAns);
  TEN_T_COPY(point->inv, task->invAns);
  if (task->pctx->tltUse) {
    tenEigensolve_d(eval, NULL, point->ten);
    /* sadly, the fact that tenAnisoCalc_f exists only for floats is part
       of the motivation for hard-wiring the aniso measure to Cl1 */
    /* HEY: with _tenAnisoEval_f[](), that's no longer true!!! */
    sum = eval[0] + eval[1] + eval[2];
    point->aniso = (eval[0] - eval[1])/(sum + FLT_EPSILON);
  } else {
    point->aniso = 0.0;
  }
  ELL_3V_COPY(point->cnt, task->cntAns);
  if (tenGageUnknown != task->pctx->gravItem) {
    ELL_3V_COPY(point->grav, task->gravAns);
  }
  /*
  if (tenGageUnknown != task->pctx->gravNotItem[0]) {
    ELL_3V_COPY(point->gravNot[0], task->gravNotAns[0]);
  }
  if (tenGageUnknown != task->pctx->gravNotItem[1]) {
    ELL_3V_COPY(point->gravNot[1], task->gravNotAns[1]);
  }
  */
  if (tenGageUnknown != task->pctx->seedThreshItem) {
    point->seedThresh = task->seedThreshAns[0];
  }
  return inside;
}

int
pushOutputGet(Nrrd *nPosOut, Nrrd *nTenOut, Nrrd *nStnOut,
              pushContext *pctx) {
  char me[]="pushOutputGet", err[BIFF_STRLEN];
  unsigned int binIdx, pointRun, pointNum, thingRun, thingNum,
    pointIdx, thingIdx, *stnOut;
  int E;
  float *posOut, *tenOut;
  pushBin *bin;
  pushThing *thing;
  pushPoint *point;

  pointNum = _pushPointTotal(pctx);
  thingNum = _pushThingTotal(pctx);
  E = AIR_FALSE;
  if (nPosOut) {
    E |= nrrdMaybeAlloc_va(nPosOut, nrrdTypeFloat, 2,
                           AIR_CAST(size_t, 3),
                           AIR_CAST(size_t, pointNum));
  }
  if (nTenOut) {
    E |= nrrdMaybeAlloc_va(nTenOut, nrrdTypeFloat, 2, 
                           AIR_CAST(size_t, 7),
                           AIR_CAST(size_t, pointNum));
  }
  if (nStnOut) {
    E |= nrrdMaybeAlloc_va(nStnOut, nrrdTypeUInt, 2,
                           AIR_CAST(size_t, 3),
                           AIR_CAST(size_t, thingNum));
  }
  if (E) {
    sprintf(err, "%s: trouble allocating outputs", me);
    biffMove(PUSH, err, NRRD); return 1;
  }
  posOut = nPosOut ? (float*)(nPosOut->data) : NULL;
  tenOut = nTenOut ? (float*)(nTenOut->data) : NULL;
  stnOut = nStnOut ? (unsigned int*)(nStnOut->data) : NULL;

  thingRun = 0;
  pointRun = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    for (thingIdx=0; thingIdx<bin->thingNum; thingIdx++) {
      thing = bin->thing[thingIdx];
      if (stnOut) {
        ELL_3V_SET(stnOut + 3*thingRun,
                   pointRun, thing->vertNum, thing->seedIdx);
      }
      for (pointIdx=0; pointIdx<thing->vertNum; pointIdx++) {
        point = thing->vert + pointIdx;
        if (posOut) {
          ELL_3V_SET(posOut + 3*pointRun,
                     point->pos[0], point->pos[1], point->pos[2]);
        }
        if (tenOut) {
          TEN_T_COPY(tenOut + 7*pointRun, point->ten);
        }
        pointRun++;
      }
      thingRun++;
    }
  }

  return 0;
}

int
pushBinProcess(pushTask *task, unsigned int myBinIdx) {
  char me[]="pushBinProcess";

  fprintf(stderr, "!%s(%u): doing bin %u\n", me, task->threadIdx, myBinIdx);
  return 0;
}


