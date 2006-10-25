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
_pushPointTotal(pushContext *pctx) {
  unsigned int binIdx, pointNum;
  pushBin *bin;

  pointNum = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    pointNum += bin->pointNum;
  }
  return pointNum;
}

void
_pushProbe(pushTask *task, pushPoint *point) {
  /* char me[]="_pushProbe"; */
  double posWorld[4], posIdx[4];

  ELL_3V_COPY(posWorld, point->pos); posWorld[3] = 1.0;
  ELL_4MV_MUL(posIdx, task->gctx->shape->WtoI, posWorld);
  ELL_4V_HOMOG(posIdx, posIdx);
  posIdx[0] = AIR_CLAMP(-0.5, posIdx[0], task->gctx->shape->size[0]-0.5);
  posIdx[1] = AIR_CLAMP(-0.5, posIdx[1], task->gctx->shape->size[1]-0.5);
  posIdx[2] = AIR_CLAMP(-0.5, posIdx[2], task->gctx->shape->size[2]-0.5);
  gageProbe(task->gctx, posIdx[0], posIdx[1], posIdx[2]);

  TEN_T_COPY(point->ten, task->tenAns);
  TEN_T_COPY(point->inv, task->invAns);
  ELL_3V_COPY(point->cnt, task->cntAns);
  if (tenGageUnknown != task->pctx->gravItem) {
    point->grav = task->gravAns[0];
    ELL_3V_COPY(point->gravGrad, task->gravGradAns);
  }
  if (tenGageUnknown != task->pctx->seedThreshItem) {
    point->seedThresh = task->seedThreshAns[0];
  }
  return;
}

int
pushOutputGet(Nrrd *nPosOut, Nrrd *nTenOut, Nrrd *nEnrOut,
              pushContext *pctx) {
  char me[]="pushOutputGet", err[BIFF_STRLEN];
  unsigned int binIdx, pointRun, pointNum, pointIdx;
  int E;
  float *posOut, *tenOut, *enrOut;
  pushBin *bin;
  pushPoint *point;

  pointNum = _pushPointTotal(pctx);
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
  if (nEnrOut) {
    E |= nrrdMaybeAlloc_va(nEnrOut, nrrdTypeFloat, 1, 
                           AIR_CAST(size_t, pointNum));
  }
  if (E) {
    sprintf(err, "%s: trouble allocating outputs", me);
    biffMove(PUSH, err, NRRD); return 1;
  }
  posOut = nPosOut ? (float*)(nPosOut->data) : NULL;
  tenOut = nTenOut ? (float*)(nTenOut->data) : NULL;
  enrOut = nEnrOut ? (float*)(nEnrOut->data) : NULL;

  pointRun = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      if (posOut) {
        ELL_3V_SET(posOut + 3*pointRun,
                   point->pos[0], point->pos[1], point->pos[2]);
      }
      if (tenOut) {
        TEN_T_COPY(tenOut + 7*pointRun, point->ten);
      }
      if (enrOut) {
        enrOut[pointRun] = point->enr;
      }
      pointRun++;
    }
  }

  return 0;
}

double
_pushPairwiseEnergy(pushTask *task,
                    double frc[3],
                    pushEnergySpec *ensp,
                    pushPoint *myPoint, pushPoint *herPoint,
                    double YY[3], double iscl) {
  /* char me[]="_pushPairwiseEnergy", err[BIFF_STRLEN]; */
  double inv[7], XX[3], nXX[3], rr, mag, WW[3], enr;

  if (task->pctx->midPntSmp) {
    pushPoint _tmpPoint;
    double det;
    ELL_3V_SCALE_ADD2(_tmpPoint.pos,
                      0.5, myPoint->pos,
                      0.5, herPoint->pos);
    _pushProbe(task, &_tmpPoint);
    TEN_T_INV(inv, _tmpPoint.ten, det);
  } else {
    TEN_T_SCALE_ADD2(inv,
                     0.5, myPoint->inv,
                     0.5, herPoint->inv);
  }
  TEN_TV_MUL(XX, inv, YY);
  ELL_3V_NORM(nXX, XX, rr);

  ensp->energy->eval(&enr, &mag, rr*iscl, ensp->parm);
  if (mag) {
    mag *= iscl;
    TEN_TV_MUL(WW, inv, nXX);
    ELL_3V_SCALE(frc, mag, WW);
  } else {
    ELL_3V_SET(frc, 0, 0, 0);
  }

  return enr;
}

int
pushBinProcess(pushTask *task, unsigned int myBinIdx) {
  char me[]="pushBinProcess";
  pushBin *myBin, *herBin, **neighbor;
  unsigned int myPointIdx, herPointIdx;
  pushPoint *myPoint, *herPoint;
  double enr, frc[3], delta[3], deltaLen, deltaNorm[3], warp[3], limit,
    maxDiffLenSqrd, iscl, diff[3], diffLenSqrd;

  if (task->pctx->verbose > 2) {
    fprintf(stderr, "%s(%u): doing bin %u\n", me, task->threadIdx, myBinIdx);
  }
  maxDiffLenSqrd = (task->pctx->maxDist)*(task->pctx->maxDist);
  iscl = 1.0/(2*task->pctx->scale);
  myBin = task->pctx->bin + myBinIdx;
  for (myPointIdx=0; myPointIdx<myBin->pointNum; myPointIdx++) {
    myPoint = myBin->point[myPointIdx];
    myPoint->enr = 0;
    ELL_3V_SET(myPoint->frc, 0, 0, 0);

    if (!task->pctx->iterNeighbor
        || 0 == task->pctx->iter % task->pctx->iterNeighbor) {
      neighbor = myBin->neighbor;
      if (task->pctx->iterNeighbor) {
        airArrayLenSet(myPoint->neighArr, 0);
      }
      while ((herBin = *neighbor)) {
        for (herPointIdx=0; herPointIdx<herBin->pointNum; herPointIdx++) {
          herPoint = herBin->point[herPointIdx];
          if (myPoint == herPoint) {
            /* can't interact with myself */
            continue;
          }
          ELL_3V_SUB(diff, herPoint->pos, myPoint->pos);
          diffLenSqrd = ELL_3V_DOT(diff, diff);
          if (diffLenSqrd > maxDiffLenSqrd) {
            /* too far away to interact */
            continue;
          }
          enr = _pushPairwiseEnergy(task, frc, task->pctx->ensp,
                                    myPoint, herPoint, diff, iscl);
          myPoint->enr += enr/2;
          if (ELL_3V_DOT(frc, frc)) {
            ELL_3V_INCR(myPoint->frc, frc);
            if (task->pctx->iterNeighbor) {
              unsigned int idx;
              idx = airArrayLenIncr(myPoint->neighArr, 1);
              myPoint->neigh[idx] = herPoint;
            }
          }
        }
        neighbor++;
      }
    } else {
      /* we are doing neighborhood list optimization, and this is an
         iteration where we use the list.  So the body of this loop
         has to be the same as the meat of the above loop */
      unsigned int neighIdx;
      for (neighIdx=0; neighIdx<myPoint->neighArr->len; neighIdx++) {
        herPoint = myPoint->neigh[neighIdx];
        ELL_3V_SUB(diff, herPoint->pos, myPoint->pos);
        enr = _pushPairwiseEnergy(task, frc, task->pctx->ensp,
                                  myPoint, herPoint, diff, iscl);
        myPoint->enr += enr/2;
        ELL_3V_INCR(myPoint->frc, frc);
      }
    }

    /* each point sees containment forces */
    ELL_3V_SCALE(frc, task->pctx->cntScl, myPoint->cnt);
    ELL_3V_INCR(myPoint->frc, frc);
    myPoint->enr += task->pctx->cntScl*(1 - myPoint->ten[0]);

    /* each point also maybe experiences gravity */
    if (tenGageUnknown != task->pctx->gravItem) {
      ELL_3V_SCALE(frc, -task->pctx->gravScl, myPoint->gravGrad);
      myPoint->enr += 
        task->pctx->gravScl*(myPoint->grav - task->pctx->gravZero);
      ELL_3V_INCR(myPoint->frc, frc);
    }
  
    /* each point in this thing also maybe experiences wall forces */
    if (task->pctx->wall) {
      /* there's an effort here to get the forces and energies, which
         are actually computed in index space, to be correctly scaled
         into world space, but no promises that its right ... */
      double enrIdx[4], enrWorld[4];
      unsigned int ci;
      double posWorld[4], posIdx[4], len, frcIdx[4], frcWorld[4];
      ELL_3V_COPY(posWorld, myPoint->pos); posWorld[3] = 1.0;
      ELL_4MV_MUL(posIdx, task->pctx->gctx->shape->WtoI, posWorld);
      ELL_4V_HOMOG(posIdx, posIdx);
      for (ci=0; ci<3; ci++) {
        if (1 == task->pctx->gctx->shape->size[ci]) {
          frcIdx[ci] = 0;          
        } else {
          len = posIdx[ci] - -0.5;
          if (len < 0) {
            len *= -1;
            frcIdx[ci] = task->pctx->wall*len;
            enrIdx[ci] = task->pctx->wall*len*len/2;
          } else {
            len = posIdx[ci] - (task->pctx->gctx->shape->size[ci] - 0.5);
            if (len > 0) {
              frcIdx[ci] = -task->pctx->wall*len;
              enrIdx[ci] = task->pctx->wall*len*len/2;
            } else {
              frcIdx[ci] = 0;
              enrIdx[ci] = 0;
            }
          }
        }
      }
      frcIdx[3] = 0.0;
      enrIdx[3] = 0.0;
      ELL_4MV_MUL(frcWorld, task->pctx->gctx->shape->ItoW, frcIdx);
      ELL_4MV_MUL(enrWorld, task->pctx->gctx->shape->ItoW, enrIdx);
      ELL_3V_INCR(myPoint->frc, frcWorld);
      myPoint->enr += ELL_3V_LEN(enrWorld);
    }

    task->energySum += myPoint->enr;

    /* -------------------------------------------- */
    /* force calculation done, now update positions */
    /* -------------------------------------------- */

    ELL_3V_SCALE(delta, task->pctx->step, myPoint->frc);
    ELL_3V_NORM(deltaNorm, delta, deltaLen);
    if (deltaLen) {
      TEN_TV_MUL(warp, myPoint->inv, delta);
      /* limit is some fraction glyph radius along direction of delta */
      limit = 0.3*task->pctx->scale*deltaLen/(FLT_MIN + ELL_3V_LEN(warp));
      deltaLen = limit*deltaLen/(limit + deltaLen);
      ELL_3V_SCALE_INCR(myPoint->pos, deltaLen, deltaNorm);
    }
    if (2 == task->pctx->dimIn) {
      double posIdx[4], posWorld[4];
      ELL_3V_COPY(posWorld, myPoint->pos); posWorld[3] = 1.0;
      ELL_4MV_MUL(posIdx, task->pctx->gctx->shape->WtoI, posWorld);
      ELL_4V_HOMOG(posIdx, posIdx);
      posIdx[task->pctx->sliceAxis] = 0.0;
      ELL_4MV_MUL(posWorld, task->pctx->gctx->shape->ItoW, posIdx);
      ELL_34V_HOMOG(myPoint->pos, posWorld);
    }
    if (!task->pctx->iterProbe
        || 0 == task->pctx->iter % task->pctx->iterProbe) {
      _pushProbe(task, myPoint);
    }
  }
  
  return 0;
}
