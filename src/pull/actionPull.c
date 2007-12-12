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
  double enr, frc[3], delta[3], deltaLen, deltaNorm[3], limit,
    diff[3], diffLenSqrd, dscale;
  /*
  double maxDiffLenSqrd, iscl;
  */

  if (task->pctx->verbose > 2) {
    fprintf(stderr, "%s(%u): doing bin %u\n", me, task->threadIdx, myBinIdx);
  }
  /* maxDiffLenSqrd = (task->pctx->maxDist)*(task->pctx->maxDist); */
  myBin = task->pctx->bin + myBinIdx;
  /* iscl = 1.0/(2*task->pctx->scale); */
  for (myPointIdx=0; myPointIdx<myBin->pointNum; myPointIdx++) {
    myPoint = myBin->point[myPointIdx];
    myPoint->energy = 0;
    ELL_3V_SET(myPoint->force, 0, 0, 0);

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
      
    }
    task->energySum += myPoint->energy;

    /* -------------------------------------------- */
    /* force calculation done, now update positions */
    /* -------------------------------------------- */

    ELL_3V_SCALE(delta, task->pctx->step, myPoint->force);
    deltaLen = ELL_3V_LEN(delta);
    if (deltaLen) {
      ELL_3V_SCALE(deltaNorm, 1.0/deltaLen, delta);
    } else {
      ELL_3V_SET(deltaNorm, 0, 0, 0);
    }
    if (!(AIR_EXISTS(deltaLen) && ELL_3V_EXISTS(deltaNorm))) {
      sprintf(err, "%s: deltaLen %g or deltaNorm (%g,%g,%g) doesn't exist", me,
              deltaLen, deltaNorm[0], deltaNorm[1], deltaNorm[2]);
      biffAdd(PULL, err); return 1;
    }
    if (deltaLen) {
      double newDeltaLen;
      /* limit is some fraction of glyph radius along direction of delta */
      limit = task->pctx->deltaLimit*task->pctx->interScale;
      newDeltaLen = limit*deltaLen/(limit + deltaLen);
      /* by definition newDeltaLen <= deltaLen */
      task->deltaFracSum += newDeltaLen/deltaLen;
      ELL_3V_SCALE_INCR(myPoint->pos, newDeltaLen, deltaNorm);
      if (!ELL_3V_EXISTS(myPoint->pos)) {
        sprintf(err, "%s: myPoint->pos %g*(%g,%g,%g) --> (%g,%g,%g) "
                "doesn't exist", me,
                newDeltaLen, deltaNorm[0], deltaNorm[1], deltaNorm[2],
                myPoint->pos[0], myPoint->pos[1], myPoint->pos[2]);
        biffAdd(PULL, err); return 1;
      }
    }

    if (1.0 <= task->pctx->probeProb
        || airDrandMT_r(task->rng) <= task->pctx->probeProb) {
      if (_pullProbe(task, myPoint)) {
        sprintf(err, "%s: probing at new field pos", me);
        biffAdd(PULL, err); return 1;
      }
    }
    
    /* the point lived, count it */
    task->pointNum += 1;
  } /* for myPointIdx */

  return 0;
}
