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

/*
** we go into this assuming that all the points we'll look at
** have just had _pullProbe() called on them
*/
int
pullBinProcess(pullTask *task, unsigned int myBinIdx) {
  char me[]="pushBinProcess", err[BIFF_STRLEN];
  pullBin *myBin, *herBin, **neighbor;
  unsigned int myPointIdx, herPointIdx;
  pullPoint *myPoint, *herPoint;

  double maxDistSqrd, move[3], moveNorm[3], moveLen;

  if (task->pctx->verbose > 2) {
    fprintf(stderr, "%s(%u): doing bin %u\n", me, task->threadIdx, myBinIdx);
  }
  maxDistSqrd = (task->pctx->maxDist)*(task->pctx->maxDist);
  myBin = task->pctx->bin + myBinIdx;
  for (myPointIdx=0; myPointIdx<myBin->pointNum; myPointIdx++) {
    myPoint = myBin->point[myPointIdx];

    myPoint->energyLast = myPoint->energy;
    myPoint->energy = 0;
    ELL_4V_SET(myPoint->move, 0, 0, 0, 0);

#if 0
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
                                  myPoint, herPoint, diff, iscl)) {
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
                                myPoint, herPoint, diff, iscl)) {
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
#endif

    if (task->pctx->ispec[pullInfoHeight]) {
      const pullInfoSpec *ispec;
      const unsigned int *infoIdx;
      double val, grad[3], hess[9];
      ispec = task->pctx->ispec[pullInfoHeight];
      infoIdx = task->pctx->infoIdx;
      val = myPoint->info[infoIdx[pullInfoHeight]];
      ELL_3V_COPY(grad, myPoint->info + infoIdx[pullInfoHeightGradient]);
      ELL_3M_COPY(hess, myPoint->info + infoIdx[pullInfoHeightHessian]);
      val = (val - ispec->zero)*ispec->scale;
      ELL_3V_SCALE(grad, ispec->scale, grad);
      ELL_3M_SCALE(hess, ispec->scale, hess);
      ELL_3V_SCALE_INCR(myPoint->move, -1, grad);
      myPoint->energy += val;
    }

    /* ------------------------------------------------ */
    /* all increments to move[] done, now actually move */
    /* ------------------------------------------------ */

    if (myPoint->energy > myPoint->energyLast) {
      /* alas, we didn't go downhill in energy with the last move.
         The current/next move (below) will be smaller */
      myPoint->step *= task->pctx->energyStepScale;
    }
    myPoint->energyLast = myPoint->energy;
    ELL_3V_SCALE(move, myPoint->step, myPoint->move);
    moveLen = ELL_3V_LEN(move);
    if (moveLen) {
      ELL_3V_SCALE(moveNorm, 1.0/moveLen, move);
    } else {
      ELL_3V_SET(moveNorm, 0, 0, 0);
    }
    if (!(AIR_EXISTS(moveLen) && ELL_3V_EXISTS(moveNorm))) {
      sprintf(err, "%s: moveLen %g or moveNorm (%g,%g,%g) doesn't exist", me,
              moveLen, moveNorm[0], moveNorm[1], moveNorm[2]);
      biffAdd(PULL, err); return 1;
    }
    if (moveLen) {
      double newMoveLen, limit, moveFrac;
      /* limit is some fraction of glyph radius along direction of delta */
      limit = task->pctx->moveLimit*task->pctx->interScale;
      newMoveLen = limit*moveLen/(limit + moveLen);
      ELL_3V_SCALE_INCR(myPoint->pos, newMoveLen, moveNorm);
      if (!ELL_3V_EXISTS(myPoint->pos)) {
        sprintf(err, "%s: myPoint->pos %g*(%g,%g,%g) --> (%g,%g,%g) "
                "doesn't exist", me,
                newMoveLen, moveNorm[0], moveNorm[1], moveNorm[2],
                myPoint->pos[0], myPoint->pos[1], myPoint->pos[2]);
        biffAdd(PULL, err); return 1;
      }
      /* by definition newMoveLen <= moveLen */
      moveFrac = newMoveLen/moveLen;
      if (moveFrac < task->pctx->moveFracMin) {
        myPoint->step *= task->pctx->moveFracStepScale;
      }
    }

    /* (possibly) probe at new location */
    if (1.0 <= task->pctx->probeProb
        || airDrandMT_r(task->rng) <= task->pctx->probeProb) {
      if (_pullProbe(task, myPoint)) {
        sprintf(err, "%s: probing at new field pos", me);
        biffAdd(PULL, err); return 1;
      }
    }
  } /* for myPointIdx */

  return 0;
}

#if 0
    if (enrImprov < 0 || pctx->deltaFrac < pctx->deltaFracMin) {
      /* either energy went up instead of down,
         or particles were hitting their speed limit too much */
      double tmp;
      tmp = pctx->step;
      if (enrImprov < 0) {
        pctx->step *= pctx->energyStepFrac;
        fprintf(stderr, "%s: ***** iter %u e improv = %g; step = %g --> %g\n",
                me, pctx->iter, enrImprov, tmp, pctx->step);
      } else {
        pctx->step *= pctx->deltaFracStepFrac;
        fprintf(stderr, "%s: ##### iter %u deltaf = %g; step = %g --> %g\n",
                me, pctx->iter, pctx->deltaFrac, tmp, pctx->step);
      }
      /* this forces another iteration */
      enrImprovAvg = AIR_NAN; 
    } else {
      /* there was some improvement; energy went down */
      if (!AIR_EXISTS(enrImprovAvg)) {
        /* either enrImprovAvg has initial NaN setting, or was set to NaN
           because we had to decrease step size; either way we now
           re-initialize it to a large-ish value, to delay convergence */
        enrImprovAvg = 3*enrImprov;
      } else {
        /* we had improvement this iteration and last, do weighted average
           of the two, so that we are measuring the trend, rather than being
           sensitive to two iterations that just happen to have the same
           energy.  Thus, when enrImprovAvg gets near user-defined threshold,
           we really must have converged */
        enrImprovAvg = (enrImprovAvg + enrImprov)/2;
      }
    }
#endif
