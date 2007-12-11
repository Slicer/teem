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

#include "pull.h"
#include "privatePull.h"

int
pullBinProcess(pullTask *task, unsigned int myBinIdx) {
#if 0
  char me[]="pushBinProcess", err[BIFF_STRLEN];
  pushBin *myBin, *herBin, **neighbor;
  unsigned int myPointIdx, herPointIdx;
  pushPoint *myPoint, *herPoint;
  double enr, frc[3], delta[3], deltaLen, deltaNorm[3], warp[3], limit,
    maxDiffLenSqrd, iscl, diff[3], diffLenSqrd, dscale, ssss;

  if (task->pctx->verbose > 2) {
    fprintf(stderr, "%s(%u): doing bin %u\n", me, task->threadIdx, myBinIdx);
  }
  maxDiffLenSqrd = (task->pctx->maxDist)*(task->pctx->maxDist);
  myBin = task->pctx->bin + myBinIdx;
  iscl = 1.0/(2*task->pctx->scale);
  for (myPointIdx=0; myPointIdx<myBin->pointNum; myPointIdx++) {
    myPoint = myBin->point[myPointIdx];
    myPoint->enr = 0;
    ELL_3V_SET(myPoint->frc, 0, 0, 0);
    ssss = AIR_AFFINE(0, myPoint->posSS, task->pctx->numSS-1,
                      task->pctx->minSS, task->pctx->maxSS);

    if (1.0 <= task->pctx->neighborTrueProb
        || airDrandMT_r(task->rng) <= task->pctx->neighborTrueProb
        || !myPoint->neighArr->len) {
      neighbor = myBin->neighbor;
      if (1.0 > task->pctx->neighborTrueProb) {
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
          if (_pushPairwiseEnergy(task, &enr, frc, task->pctx->ensp,
                                  myPoint, herPoint, diff, iscl/ssss)) {
            sprintf(err, "%s: between points %u and %u, A", me,
                    myPoint->ttaagg, herPoint->ttaagg);
            biffAdd(PULL, err); return 1;
          }
          myPoint->enr += enr/2;
          if (ELL_3V_DOT(frc, frc)) {
            ELL_3V_INCR(myPoint->frc, frc);
            if (1.0 > task->pctx->neighborTrueProb) {
              unsigned int idx;
              idx = airArrayLenIncr(myPoint->neighArr, 1);
              myPoint->neigh[idx] = herPoint;
            }
          }
          if (!ELL_3V_EXISTS(myPoint->frc)) {
            sprintf(err, "%s: bad myPoint->frc (%g,%g,%g) @ bin %p end", me,
                    myPoint->frc[0], myPoint->frc[1], myPoint->frc[2],
                    herBin);
            biffAdd(PULL, err); return 1;
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
        if (_pushPairwiseEnergy(task, &enr, frc, task->pctx->ensp,
                                myPoint, herPoint, diff, iscl/ssss)) {
          sprintf(err, "%s: between points %u and %u, B", me,
                  myPoint->ttaagg, herPoint->ttaagg);
          biffAdd(PULL, err); return 1;
        }
        myPoint->enr += enr/2;
        ELL_3V_INCR(myPoint->frc, frc);
      }
    }
    if (!ELL_3V_EXISTS(myPoint->frc)) {
      sprintf(err, "%s: post-nei myPoint->frc (%g,%g,%g) doesn't exist", me,
              myPoint->frc[0], myPoint->frc[1], myPoint->frc[2]);
      biffAdd(PULL, err); return 1;
    }

    if (!task->pctx->numSS) {
      /* containment forces + gravity do not currently
         apply in the case of SS behavior */

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
    }      
    if (!ELL_3V_EXISTS(myPoint->frc)) {
      sprintf(err, "%s: post-grav myPoint->frc (%g,%g,%g) doesn't exist", me,
              myPoint->frc[0], myPoint->frc[1], myPoint->frc[2]);
      biffAdd(PULL, err); return 1;
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
    } /* wall */
    if (!ELL_3V_EXISTS(myPoint->frc)) {
      sprintf(err, "%s: post-wall myPoint->frc (%g,%g,%g) doesn't exist", me,
              myPoint->frc[0], myPoint->frc[1], myPoint->frc[2]);
      biffAdd(PULL, err); return 1;
    }

    task->energySum += myPoint->enr;

    /* -------------------------------------------- */
    /* force calculation done, now update positions */
    /* -------------------------------------------- */

    ELL_3V_SCALE(delta, task->pctx->step, myPoint->frc);
    ELL_3V_NORM(deltaNorm, delta, deltaLen);
    if (!(AIR_EXISTS(deltaLen) && ELL_3V_EXISTS(deltaNorm))) {
      sprintf(err, "%s: deltaLen %g or deltaNorm (%g,%g,%g) doesn't exist", me,
              deltaLen, deltaNorm[0], deltaNorm[1], deltaNorm[2]);
      biffAdd(PULL, err); return 1;
    }
    if (deltaLen) {
      double newDelta;
      TEN_TV_MUL(warp, myPoint->inv, delta);
      /* limit is some fraction of glyph radius along direction of delta */
      limit = (task->pctx->deltaLimit
               *task->pctx->scale*deltaLen/(FLT_MIN + ELL_3V_LEN(warp)));
      newDelta = limit*deltaLen/(limit + deltaLen);
      /* by definition newDelta <= deltaLen */
      task->deltaFracSum += newDelta/deltaLen;
      ELL_3V_SCALE_INCR(myPoint->pos, newDelta, deltaNorm);
      if (!ELL_3V_EXISTS(myPoint->pos)) {
        sprintf(err, "%s: myPoint->pos %g*(%g,%g,%g) --> (%g,%g,%g) "
                "doesn't exist", me,
                newDelta, deltaNorm[0], deltaNorm[1], deltaNorm[2],
                myPoint->pos[0], myPoint->pos[1], myPoint->pos[2]);
        biffAdd(PULL, err); return 1;
      }
    }
    if (2 == task->pctx->dimIn) {
      double posIdx[4], posWorld[4], posOrig[4];
      ELL_3V_COPY(posOrig, myPoint->pos); posOrig[3] = 1.0;
      ELL_4MV_MUL(posIdx, task->pctx->gctx->shape->WtoI, posOrig);
      ELL_4V_HOMOG(posIdx, posIdx);
      posIdx[task->pctx->sliceAxis] = 0.0;
      ELL_4MV_MUL(posWorld, task->pctx->gctx->shape->ItoW, posIdx);
      ELL_34V_HOMOG(myPoint->pos, posWorld);
      if (!ELL_3V_EXISTS(myPoint->pos)) {
        sprintf(err, "%s: myPoint->pos (%g,%g,%g) -> (%g,%g,%g) "
                "doesn't exist", me,
                posOrig[0], posOrig[1], posOrig[2], 
                myPoint->pos[0], myPoint->pos[1], myPoint->pos[2]);
        biffAdd(PULL, err); return 1;
      }
    }
    if (1.0 <= task->pctx->probeProb
        || airDrandMT_r(task->rng) <= task->pctx->probeProb) {
      if (_pushProbe(task, myPoint)) {
        sprintf(err, "%s: probing at new field pos", me);
        biffAdd(PULL, err); return 1;
      }
    }
    
    /* the point lived, count it */
    task->pointNum += 1;
  } /* for myPointIdx */
#endif
  return 0;
}
