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
_pullPairwiseEnergy(pullTask *task,
                    double *enrP,
                    double frc[3],
                    pullEnergySpec *ensp,
                    pullPoint *myPoint, pullPoint *herPoint,
                    double XX[3], double iscl) {
  double nXX[3], rr, mag;
  
  ELL_3V_NORM(nXX, XX, rr);
  ensp->energy->eval(enrP, &mag, rr*iscl, ensp->parm);
  if (mag) {
    mag *= iscl;
    ELL_3V_SCALE(frc, mag, nXX);
  } else {
    ELL_3V_SET(frc, 0, 0, 0);
  }

  return 0;
}

/*
** we go into this assuming that all the points we'll look at
** have just had _pullProbe() called on them
*/
int
pullBinProcess(pullTask *task, unsigned int myBinIdx) {
  char me[]="pullBinProcess", err[BIFF_STRLEN];
  pullBin *myBin, *herBin, **neighbor;
  unsigned int myPointIdx, herPointIdx;
  pullPoint *myPoint, *herPoint;

  double maxDistSqrd, iscl, move[3], moveNorm[3], moveLen, enrImprov;

  if (task->pctx->verbose > 2) {
    fprintf(stderr, "%s(%u): doing bin %u\n", me, task->threadIdx, myBinIdx);
  }
  maxDistSqrd = (task->pctx->maxDist)*(task->pctx->maxDist);
  iscl = 1.0/(2*task->pctx->interScale);
  if (task->pctx->verbose > 2) {
    fprintf(stderr, "%s: maxDist = %g, interScale = %g -> iscl = %g\n", me, 
            task->pctx->maxDist, task->pctx->interScale, iscl);
  }
  myBin = task->pctx->bin + myBinIdx;
  for (myPointIdx=0; myPointIdx<myBin->pointNum; myPointIdx++) {
    myPoint = myBin->point[myPointIdx];

    myPoint->energyLast = myPoint->energy;
    myPoint->energy = 0;
    ELL_4V_SET(myPoint->move, 0, 0, 0, 0);

    if (task->pctx->ispec[pullInfoHeight]) {
      const pullInfoSpec *ispec;
      const unsigned int *infoIdx;
      double val, grad[3], hess[9], tmp[3], contr, tt;
      ispec = task->pctx->ispec[pullInfoHeight];
      infoIdx = task->pctx->infoIdx;
      val = myPoint->info[infoIdx[pullInfoHeight]];
      ELL_3V_COPY(grad, myPoint->info + infoIdx[pullInfoHeightGradient]);
      ELL_3M_COPY(hess, myPoint->info + infoIdx[pullInfoHeightHessian]);
      val = (val - ispec->zero)*ispec->scale;
      ELL_3V_SCALE(grad, ispec->scale, grad);
      ELL_3M_SCALE(hess, ispec->scale, hess);
      myPoint->energy += val;

      ELL_3MV_MUL(tmp, hess, grad);
      contr = ELL_3V_DOT(grad, tmp);
      if (contr <= 0) {
        /* if the contraction of the hessian along the gradient is
           negative then we seem to be near a local maxima of height,
           which is bad, so we do simple gradient descent. This also
           catches the case when the second derivative is zero. */
        tt = 1;
      } else {
        tt = ELL_3V_DOT(grad, grad)/contr;
        /* hack to make sure we don't try to move too far */
        tt = AIR_MIN(3, tt);
      }
      ELL_3V_SCALE_INCR(myPoint->move, -tt, grad);
    }
    /*
      if we have both tang1 and tang2: move only within their span
      if we have tang1: move within its span
      with mode: some lerp between the two
    */
    if (task->pctx->ispec[pullInfoTangent2]) {
      const pullInfoSpec *ispec1, *ispec2;
      const unsigned int *infoIdx;
      const double *tang1, *tang2;
      double tmp[3], out1[9], out2[9], proj[9];
      ispec1 = task->pctx->ispec[pullInfoTangent1];
      ispec2 = task->pctx->ispec[pullInfoTangent2];
      infoIdx = task->pctx->infoIdx;
      tang1 = myPoint->info + infoIdx[pullInfoTangent1];
      tang2 = myPoint->info + infoIdx[pullInfoTangent2];
      ELL_3MV_OUTER(out1, tang1, tang1);
      ELL_3MV_OUTER(out2, tang2, tang2);
      ELL_3M_ADD2(proj, out1, out2);
      ELL_3MV_MUL(tmp, proj, myPoint->move);
      ELL_3V_COPY(myPoint->move, tmp);
    }
    
    if (pullEnergyZero != task->pctx->energySpec->energy) {
      if (1.0 <= task->pctx->neighborLearnProb
          || airDrandMT_r(task->rng) <= task->pctx->neighborLearnProb
          || !myPoint->neighArr->len) {
        neighbor = myBin->neigh;
        if (1.0 > task->pctx->neighborLearnProb) {
          airArrayLenSet(myPoint->neighArr, 0);
        }
        while ((herBin = *neighbor)) {
          for (herPointIdx=0; herPointIdx<herBin->pointNum; herPointIdx++) {
            double distSqrd, enr, diff[3], frc[3];
            herPoint = herBin->point[herPointIdx];
            if (myPoint == herPoint) {
              /* can't interact with myself */
              continue;
            }
            ELL_3V_SUB(diff, herPoint->pos, myPoint->pos);
            distSqrd = ELL_3V_DOT(diff, diff);
            /*
            fprintf(stderr, "!%s: dist(%u,%u)^2 = %g %s %g\n", me,
                    myPoint->idtag, herPoint->idtag, 
                    distSqrd, (distSqrd > maxDistSqrd ? ">" : "<="), 
                    maxDistSqrd);
            */
            if (distSqrd > maxDistSqrd) {
              /* too far away to interact */
              continue;
            }
            if (_pullPairwiseEnergy(task, &enr, frc, task->pctx->energySpec,
                                    myPoint, herPoint, diff, iscl)) {
              sprintf(err, "%s: between points %u and %u, A", me,
                      myPoint->idtag, herPoint->idtag);
              biffAdd(PULL, err); return 1;
            }
            myPoint->energy += enr/2;
            if (ELL_3V_DOT(frc, frc)) {
              ELL_3V_INCR(myPoint->move, frc);
              if (1.0 > task->pctx->neighborLearnProb) {
                unsigned int idx;
                idx = airArrayLenIncr(myPoint->neighArr, 1);
                myPoint->neigh[idx] = herPoint;
              }
            }
            if (!ELL_3V_EXISTS(myPoint->move)) {
              sprintf(err, "%s: bad myPoint->frc (%g,%g,%g) @ bin %p end", me,
                      myPoint->move[0], myPoint->move[1], myPoint->move[2],
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
          double diff[3], enr, frc[3];
          herPoint = myPoint->neigh[neighIdx];
          ELL_3V_SUB(diff, herPoint->pos, myPoint->pos);
          if (_pullPairwiseEnergy(task, &enr, frc, task->pctx->energySpec,
                                  myPoint, herPoint, diff, iscl)) {
            sprintf(err, "%s: between points %u and %u, B", me,
                    myPoint->idtag, herPoint->idtag);
            biffAdd(PULL, err); return 1;
          }
          myPoint->energy += enr/2;
          ELL_3V_INCR(myPoint->move, frc);
        }
      }
      if (!ELL_3V_EXISTS(myPoint->move)) {
        sprintf(err, "%s: post-nei myPoint->move (%g,%g,%g) doesn't exist", me,
                myPoint->move[0], myPoint->move[1], myPoint->move[2]);
        biffAdd(PULL, err); return 1;
      }
    }

    /* ------------------------------------------------ */
    /* all increments to move[] done, now actually move */
    /* ------------------------------------------------ */

    enrImprov = 2*(myPoint->energyLast - myPoint->energy)
      / (AIR_ABS(myPoint->energyLast) + AIR_ABS(myPoint->energy));
    if (enrImprov < task->pctx->energyImprovFloor) {
      /* alas, we didn't go downhill in energy with the last move.
         The current/next move (below) will be smaller */
      myPoint->step *= task->pctx->energyStepScale;
      if (task->pctx->verbose > 4) {
        fprintf(stderr, "!%s: point %u enr %g %g step --> %g\n", me,
                myPoint->idtag, myPoint->energyLast,
                myPoint->energy, myPoint->step);
      }
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
