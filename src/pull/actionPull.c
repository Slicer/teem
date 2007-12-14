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
  
  AIR_UNUSED(task);
  AIR_UNUSED(myPoint);
  AIR_UNUSED(herPoint);

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

double
_pullPointHeight(const pullTask *task, const pullPoint *point) {
  const pullInfoSpec *ispec;
  const unsigned int *infoIdx;
  double val;

  ispec = task->pctx->ispec[pullInfoHeight];
  infoIdx = task->pctx->infoIdx;
  val = point->info[infoIdx[pullInfoHeight]];
  val = (val - ispec->zero)*ispec->scale;
  return val;
}

/*
** this assumes that _pullProbe() has just been called on the point,
** and the point is used only as a record of the info set there
*/
void
_pullPointDescent(double move[3], const pullTask *task, const pullPoint *point) {
  /* char me[]="_pullPointHeightStep"; */
  const pullInfoSpec *ispec;
  const unsigned int *infoIdx;
  double val, grad[3], hess[9], tmp[3], contr, tt;

  ispec = task->pctx->ispec[pullInfoHeight];
  infoIdx = task->pctx->infoIdx;
  val = point->info[infoIdx[pullInfoHeight]];
  ELL_3V_COPY(grad, point->info + infoIdx[pullInfoHeightGradient]);
  ELL_3M_COPY(hess, point->info + infoIdx[pullInfoHeightHessian]);
  val = (val - ispec->zero)*ispec->scale;
  ELL_3V_SCALE(grad, ispec->scale, grad);
  ELL_3M_SCALE(hess, ispec->scale, hess);

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
    /*
    fprintf(stderr, "!%s(%u): tt = %g/%g = %g --> %g\n", me, 
            point->idtag, ELL_3V_DOT(grad, grad), contr,
            tt, AIR_MIN(3, tt));
    */
    /* to be safe, we limit ourselves to the distance that could
       have been gone via gradient descent */
    tt = tt/(1 + tt);
  }
  ELL_3V_SCALE(move, -tt, grad);
  /*
  fprintf(stderr, "!%s(%u): grad=(%g,%g,%g), tt=%g, move=(%g,%g,%g) len %g\n",
          me, point->idtag, grad[0], grad[1], grad[2], tt,
          move[0], move[1], move[2], ELL_3V_LEN(move));
  */
  /* with both tang1 and tang2: move only within their span
     with tang1 only: move within its span
     with mode: some lerp between the two */
  if (task->pctx->ispec[pullInfoTangent2]) {
    const double *tang1, *tang2;
    double tmp[3], out1[9], out2[9], proj[9];

    tang1 = point->info + infoIdx[pullInfoTangent1];
    tang2 = point->info + infoIdx[pullInfoTangent2];
    ELL_3MV_OUTER(out1, tang1, tang1);
    ELL_3MV_OUTER(out2, tang2, tang2);
    ELL_3M_ADD2(proj, out1, out2);
    ELL_3MV_MUL(tmp, proj, move);
    ELL_3V_COPY(move, tmp);
    /*
    fprintf(stderr, "!%s(%u):   --> move = (len %g) %g %g %g\n", me,
            point->idtag, ELL_3V_LEN(move), move[0], move[1], move[2]);
    */
  }

  return;
}


/*
** sets in point:
**  stepInter or stepConstr (based on enrImprov and moveFrac)
**  pos
**  and then (maybe) probes 
*/
int
_pullPointMove(pullTask *task, pullPoint *point,
               const double enrLast, const double enrNew,
               const double moveWant[3],
               const int forConstraint) {
  char me[]="_pullPointMove", err[BIFF_STRLEN];
  double enrImprov, move[3], moveNorm[3], moveLen;

  enrImprov = _PULL_IMPROV(enrLast, enrNew);
  if (enrImprov < task->pctx->energyImprovTest) {
    /* alas, we didn't go (sufficiently) downhill in energy with the
       last move. The current/next move will be smaller */
    if (forConstraint) {
      point->stepConstr *= task->pctx->energyStepScale;
    } else {
      point->stepInter *= task->pctx->energyStepScale;
    }
  }
    
  if (forConstraint) {
    ELL_3V_SCALE(move, point->stepConstr, moveWant);
  } else {
    ELL_3V_SCALE(move, point->stepInter, moveWant);
  }
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
    /* limit is some fraction of radius along direction of move */
    limit = task->pctx->moveLimit*task->pctx->interScale;
    newMoveLen = limit*moveLen/(limit + moveLen);
    ELL_3V_SCALE_INCR(point->pos, newMoveLen, moveNorm);
    if (!ELL_3V_EXISTS(point->pos)) {
      sprintf(err, "%s: point->pos %g*(%g,%g,%g) --> (%g,%g,%g) "
              "doesn't exist", me,
              newMoveLen, moveNorm[0], moveNorm[1], moveNorm[2],
              point->pos[0], point->pos[1], point->pos[2]);
      biffAdd(PULL, err); return 1;
    }
    /* by definition newMoveLen <= moveLen */
    moveFrac = newMoveLen/moveLen;
    if (moveFrac < task->pctx->moveFracMin) {
      if (forConstraint) {
        point->stepConstr *= task->pctx->moveFracStepScale;
      } else {
        point->stepInter *= task->pctx->moveFracStepScale;
      }
    }
  }
  
  /* (possibly) probe at new location */
  if (forConstraint
      || 1.0 <= task->pctx->probeProb
      || airDrandMT_r(task->rng) <= task->pctx->probeProb) {
    if (_pullProbe(task, point)) {
      sprintf(err, "%s: probing at new field pos", me);
      biffAdd(PULL, err); return 1;
    }
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

  double maxDistSqrd, iscl;

  /*
  double pos0[3] = {0.048773, 0.592845, 0.00134082};
  double pos1[3] = {0.1103, 0.58499, 0.00117511};
  double pdiff[3];
  unsigned int dbgIdx;
  */

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
    /* the purpose of the loop body is to accumulate energy and "force"
       into myPoint->energy and myPoint->move */

    /* if (0 == task->pctx->iter % 2) { */
    if (task->pctx->ispec[pullInfoHeight]) {
      double move[3];

      if (!task->pctx->ispec[pullInfoHeight]->constraint) {
        myPoint->energy += _pullPointHeight(task, myPoint);
        _pullPointDescent(move, task, myPoint);
        ELL_3V_INCR(myPoint->move, move);
      } else {
        /* more involved; height is a constraint */
        unsigned int citer;
        double ceNew, ceLast, ceImprov, ceImprovAvg=AIR_NAN;

        ceLast = _pullPointHeight(task, myPoint);
        _pullPointDescent(move, task, myPoint);
        _pullPointMove(task, myPoint, 1, 0, move, AIR_TRUE);
        for (citer=0; citer<task->pctx->maxConstraintIter; citer++) {
          ceNew = _pullPointHeight(task, myPoint);
          ceImprov = _PULL_IMPROV(ceLast, ceNew);
          ceImprovAvg = _PULL_IMPROV_AVG(!citer, ceImprovAvg, ceImprov);
          if (ceImprovAvg < task->pctx->energyImprovMin) {
            break;
          }
          _pullPointDescent(move, task, myPoint);
          _pullPointMove(task, myPoint, ceLast, ceNew, move, AIR_TRUE);
          ceLast = ceNew;
        }
      }
    }
    /* } */
    
    /* if (1 == task->pctx->iter % 2) { */
    if ((pullEnergyZero != task->pctx->energySpec->energy)
        && (task->pctx->interScale > 0)) {
      if (1.0 <= task->pctx->neighborLearnProb
          || airDrandMT_r(task->rng) <= task->pctx->neighborLearnProb
          || !myPoint->neighArr->len) {
        /* either we are not using neighbor caching,
           or, we are using it and this iteration we rebuild the list, 
           or, we are using it and we haven't built the list yet */
        neighbor = myBin->neigh;
        if (1.0 > task->pctx->neighborLearnProb) {
          airArrayLenSet(myPoint->neighArr, 0);
        }
        while ((herBin = *neighbor)) {
          for (herPointIdx=0; herPointIdx<herBin->pointNum; herPointIdx++) {
            double distSqrd, enr, diff[3], frc[3];
            herPoint = herBin->point[herPointIdx];
            if (myPoint == herPoint) {
              continue; /* can't interact with myself */
            }
            ELL_3V_SUB(diff, herPoint->pos, myPoint->pos);
            distSqrd = ELL_3V_DOT(diff, diff);
            if (distSqrd > maxDistSqrd) {
              continue; /* too far away to interact */
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
        /* we are using neighbor list caching, and this is an
           iteration where we re-use the list.  So the body of this
           loop has to be the same as the meat of the above loop */
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
    /* } */

    if (task->pctx->wallScale > 0) {
      /*
      unsigned int ci;
      double wenr[3], wmve[3], len;

      for (ci=0; ci<3; ci++) {
        len = myPoint->pos[ci] - task->pctx->bboxMin[ci];
        if (len < 0) {
          len *= -1;
          wmve[ci] = task->pctx->wallScale*len;
          wenr[ci] = task->pctx->wallScale*len*len/2;
        } else {
          len = myPoint->pos[ci] - task->pctx->bboxMax[ci];
          if (len > 0) {
            wmve[ci] = -task->pctx->wallScale*len;
            wenr[ci] = task->pctx->wallScale*len*len/2;
          } else {
            wmve[ci] = 0;
            wenr[ci] = 0;
          }
        }
      }
      ELL_3V_INCR(myPoint->move, wmve);
      myPoint->energy += ELL_3V_LEN(wenr);
      */
      ELL_3V_MAX(myPoint->pos, myPoint->pos, task->pctx->bboxMin);
      ELL_3V_MIN(myPoint->pos, myPoint->pos, task->pctx->bboxMax);
    }


    /* ------------------------------------------------ */
    /* all increments to move[] done, now actually move */
    /* ------------------------------------------------ */

    if (_pullPointMove(task, myPoint, myPoint->energyLast, myPoint->energy, 
                       myPoint->move, AIR_FALSE)) {
      sprintf(err, "%s: moving %u", me, myPoint->idtag);
      biffAdd(PULL, err); return 1;
    }

    myPoint->energyLast = myPoint->energy;

  } /* for myPointIdx */

  return 0;
}
