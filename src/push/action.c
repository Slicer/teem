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

int
_pushProbe(pushTask *task, pushPoint *point) {
  char me[]="_pushProbe", err[BIFF_STRLEN];
  double posWorld[4], posIdx[4];

  ELL_3V_COPY(posWorld, point->pos); posWorld[3] = 1.0;
  ELL_4MV_MUL(posIdx, task->gctx->shape->WtoI, posWorld);
  ELL_4V_HOMOG(posIdx, posIdx);
  posIdx[0] = AIR_CLAMP(-0.5, posIdx[0], task->gctx->shape->size[0]-0.5);
  posIdx[1] = AIR_CLAMP(-0.5, posIdx[1], task->gctx->shape->size[1]-0.5);
  posIdx[2] = AIR_CLAMP(-0.5, posIdx[2], task->gctx->shape->size[2]-0.5);
  if (task->pctx->numSS) {
    if (gageStackProbe(task->gctxSS, point->posSS)
        || gageProbe(task->gctxSS, posIdx[0], posIdx[1], posIdx[2])) {
      sprintf(err, "%s: gageProbe(SS,%p) failed:\n (%d) %s\n", me,
              task->gctxSS, task->gctxSS->errNum, task->gctxSS->errStr);
      biffAdd(PUSH, err); return 1;
    }
    point->zcSS = task->zcSSAns[0];
    ELL_3V_COPY(point->gvSS, task->gvSSAns);
  }

  if (gageProbe(task->gctx, posIdx[0], posIdx[1], posIdx[2])) {
    sprintf(err, "%s: gageProbe failed:\n (%d) %s\n", me,
            task->gctx->errNum, task->gctx->errStr);
    biffAdd(PUSH, err); return 1;
  }
    
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
  return 0;
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
  double sclmin, sclmax, sclmean;

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
  sclmean = 0;
  sclmin = sclmax = AIR_NAN;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      if (posOut) {
        ELL_3V_SET(posOut + 3*pointRun,
                   point->pos[0], point->pos[1], point->pos[2]);
      }
      if (tenOut) {
        if (pctx->numSS) {
          double matOut[9], matTmp[9], norm[3], perp1[3], perp2[3], len, scl;
          ELL_3V_NORM(norm, point->gvSS, len);
          ELL_3MV_OUTER(matTmp, norm, norm);
          ELL_3M_ZERO_SET(matOut);
          ELL_3M_SCALE_INCR(matOut, 0.01, matTmp);
          ell_3v_perp_d(perp1, norm);
          ELL_3V_NORM(perp1, perp1, len);
          ELL_3MV_OUTER(matTmp, perp1, perp1);
          scl = AIR_AFFINE(0, point->posSS, pctx->numSS-1,
                           pctx->minSS, pctx->maxSS);
          sclmin = AIR_EXISTS(sclmin) ? AIR_MIN(sclmin, scl) : scl;
          sclmax = AIR_EXISTS(sclmax) ? AIR_MAX(sclmax, scl) : scl;
          ELL_3M_SCALE_INCR(matOut, scl, matTmp);
          ELL_3V_CROSS(perp2, norm, perp1);
          ELL_3MV_OUTER(matTmp, perp2, perp2);
          ELL_3M_SCALE_INCR(matOut, scl, matTmp);
          TEN_M2T(tenOut + 7*pointRun, matOut);
          sclmean += scl;
        } else {
          TEN_T_COPY(tenOut + 7*pointRun, point->ten);
        }
      }
      if (enrOut) {
        enrOut[pointRun] = point->enr;
      }
      pointRun++;
    }
  }
  if (pctx->numSS) {
    sclmean /= pointRun;
    fprintf(stderr, "!%s: scale min, mean, max = %g %g %g (%g)\n", me,
            sclmin, sclmean, sclmax, sclmax - sclmin);
  }

  return 0;
}

int
_pushPairwiseEnergy(pushTask *task,
                    double *enrP,
                    double frc[3],
                    pushEnergySpec *ensp,
                    pushPoint *myPoint, pushPoint *herPoint,
                    double YY[3], double iscl) {
  char me[]="_pushPairwiseEnergy", err[BIFF_STRLEN];
  double inv[7], XX[3], nXX[3], nYY[3], rr, mag, WW[3];

  if (task->pctx->numSS) {
    ELL_3V_NORM(nYY, YY, rr);
    ensp->energy->eval(enrP, &mag, rr*iscl, ensp->parm);
    ELL_3V_SCALE(frc, mag, nYY);
  } else {
    if (task->pctx->midPntSmp) {
      pushPoint _tmpPoint;
      double det;
      ELL_3V_SCALE_ADD2(_tmpPoint.pos,
                        0.5, myPoint->pos,
                        0.5, herPoint->pos);
      if (_pushProbe(task, &_tmpPoint)) {
        sprintf(err, "%s: at midpoint of %u and %u", me,
                myPoint->ttaagg, herPoint->ttaagg);
        biffAdd(PUSH, err); *enrP = AIR_NAN; return 1;
      }
      TEN_T_INV(inv, _tmpPoint.ten, det);
    } else {
      TEN_T_SCALE_ADD2(inv,
                       0.5, myPoint->inv,
                       0.5, herPoint->inv);
    }
    TEN_TV_MUL(XX, inv, YY);
    ELL_3V_NORM(nXX, XX, rr);
    
    ensp->energy->eval(enrP, &mag, rr*iscl, ensp->parm);
    if (mag) {
      mag *= iscl;
      TEN_TV_MUL(WW, inv, nXX);
      ELL_3V_SCALE(frc, mag, WW);
    } else {
      ELL_3V_SET(frc, 0, 0, 0);
    }
  }

  return 0;
}

#define EPS_PER_MAX_DIST 200
#define SEEK_MAX_ITER 30

int
_pushSurfaceSeek(pushTask *task, pushPoint *myPoint, double step) {
  char me[]="_pushSurfaceSeek", err[BIFF_STRLEN];
  double stepDir[3], gm, zcLast, endA[3], endB[3], aa, bb, cc, za, zb, eps,
    posOrig[3];
  unsigned int iter;

  ELL_3V_COPY(posOrig, myPoint->pos);

  eps = task->pctx->maxDist/EPS_PER_MAX_DIST;
  step = AIR_ABS(step)*0.618;
  /*
  fprintf(stderr, "!%s: eps = %g; step = %g\n", me, eps, step);
  */
  if (_pushProbe(task, myPoint)) {
    sprintf(err, "%s: initial probe", me);
    biffAdd(PUSH, err); return 1;
  }
  if (myPoint->zcSS < 0) {
    step *= -1;
  }
  iter = 0;
  do {
    zcLast = myPoint->zcSS;
    ELL_3V_NORM(stepDir, myPoint->gvSS, gm);
    ELL_3V_SCALE_INCR(myPoint->pos, step, stepDir);
    if (_pushProbe(task, myPoint)) {
      sprintf(err, "%s: on iter %u of zero-cross seeking", me, iter);
      biffAdd(PUSH, err); return 1;
    }
    iter++;
  } while (zcLast*myPoint->zcSS > 0
           && iter < SEEK_MAX_ITER);

  if (SEEK_MAX_ITER == iter) {
    /* ELL_3V_COPY(myPoint->pos, posOrig); */
    sprintf(err, "%s: never saw ZC after %u iters (step=%g)", me, iter, step);
    biffAdd(PUSH, err); return 1;
  }

  /* position of last probe led to a change in zc.
     last probe at myPoint->pos,
     previous probe at myPoint->pos - step*stepDir */
  ELL_3V_COPY(endA, myPoint->pos);
  za = myPoint->zcSS;
  aa = 0;
  ELL_3V_SCALE_ADD2(endB, 1, myPoint->pos, -step, stepDir);
  zb = zcLast;
  bb = 1;
  iter = 0;
  do {
    cc = (zb*aa - za*bb)/(zb - za);
    ELL_3V_LERP(myPoint->pos, cc, endA, endB);
    if (_pushProbe(task, myPoint)) {
      sprintf(err, "%s: on iter %u of zero-cross converge", me, iter);
      biffAdd(PUSH, err); return 1;
    }
    if (za * myPoint->zcSS > 0) {
      aa = cc;
    } else { 
      bb = cc;
    }
    iter ++;
  } while ((bb - aa)*step > eps
           && iter < SEEK_MAX_ITER);

  if (SEEK_MAX_ITER == iter) {
    /* ELL_3V_COPY(myPoint->pos, posOrig); */
    sprintf(err, "%s: failed ZC converge after %u iters (step=%g)", me, 
            iter, step);
    biffAdd(PUSH, err); return 1;
  }

  return 0;
}

#define SCALE_TEST_STEP 0.1
#define SCALE_STEP_SIZE 0.2

int
_pushDeltaScale(pushTask *task, double *dscaleP, pushPoint *myPoint) {
  char me[]="_pushDeltaScale", err[BIFF_STRLEN];
  pushPoint *herPoint;
  unsigned int neighIdx;
  double wght, wghtSum, diff[3], oldScaleIdx, oldScale,
    oldStrength, testScale, testScaleIdx, testStrength, strengthGrad,
    newScale, newScaleIdx,
    avgScaleIdx, wantScaleIdx, limit;
  int signRnd;

  oldScaleIdx = myPoint->posSS;

  avgScaleIdx = 0;
  wghtSum = 0;
  for (neighIdx=0; neighIdx<myPoint->neighArr->len; neighIdx++) {
    herPoint = myPoint->neigh[neighIdx];
    ELL_3V_SUB(diff, herPoint->pos, myPoint->pos);
    wght = 1.0/(FLT_MIN + ELL_3V_LEN(diff));
    avgScaleIdx += wght*herPoint->posSS;
    wghtSum += wght;
  }
  if (wghtSum) {
    avgScaleIdx /= wghtSum;
  } else {
    /* we had no neighbors! */
    avgScaleIdx = oldScaleIdx;
  }
  
  oldScale = AIR_AFFINE(0, oldScaleIdx, task->pctx->numSS-1,
                        task->pctx->minSS, task->pctx->maxSS);
  oldStrength = ELL_3V_LEN(myPoint->gvSS)*oldScale;
  signRnd = 2*airRandInt_r(task->rng, 2) - 1;
  testScaleIdx = oldScaleIdx + SCALE_TEST_STEP*signRnd*task->pctx->numSS;
  myPoint->posSS = testScaleIdx;
  if (_pushSurfaceSeek(task, myPoint,
                       task->pctx->deltaLimit*task->pctx->scale)) {
    sprintf(err, "%s: seeking at testScaleIdx %g=%g", me,
            myPoint->posSS, testScaleIdx);
    biffAdd(PUSH, err); return 1;
  }
  testScale = AIR_AFFINE(0, testScaleIdx, task->pctx->numSS-1,
                         task->pctx->minSS, task->pctx->maxSS);
  testStrength = ELL_3V_LEN(myPoint->gvSS)*testScale;
  strengthGrad = (testStrength - oldStrength)/(testScale - oldScale);
  newScale = oldScale + SCALE_STEP_SIZE*strengthGrad;
  newScaleIdx= AIR_AFFINE(task->pctx->minSS, newScale, task->pctx->maxSS,
                          0, task->pctx->numSS-1);
  
  wantScaleIdx = AIR_LERP(0.5, avgScaleIdx, newScaleIdx);
  /*
  fprintf(stderr, "!%s: {avg,new,want}ScaleIdx = %g %g %g\n", me,
          avgScaleIdx, newScaleIdx, wantScaleIdx);
  */

  /* this is not how deltaLimit is supposed to be used, but its similar */
  *dscaleP = wantScaleIdx - oldScaleIdx;
  limit = task->pctx->deltaLimit;
  *dscaleP = (*dscaleP > 0
              ? limit*(*dscaleP)/(limit + (*dscaleP))
              : limit*(*dscaleP)/(limit - (*dscaleP)));

  /*
  fprintf(stderr, "!%s: oldScaleIdx %g -> dscale = %g\n", me,
          oldScaleIdx, *dscaleP);
  fprintf(stderr, "!%s: limit=%g, strengthGrad = (%g - %g)/(%g - %g) = %g \n"
          "  -> dscale = %g\n", me, limit,
          testStrength, oldStrength, testScale, oldScale,
          strengthGrad, *dscaleP);
  */
  *dscaleP = AIR_EXISTS(*dscaleP) ? *dscaleP : 0;
  
  /* return it the way we got it */
  myPoint->posSS = oldScaleIdx;
  return 0;
}

int
pushBinProcess(pushTask *task, unsigned int myBinIdx) {
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
            biffAdd(PUSH, err); return 1;
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
          biffAdd(PUSH, err); return 1;
        }
        myPoint->enr += enr/2;
        ELL_3V_INCR(myPoint->frc, frc);
      }
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

    task->energySum += myPoint->enr;

    /* -------------------------------------------- */
    /* force calculation done, now update positions */
    /* -------------------------------------------- */

    ELL_3V_SCALE(delta, task->pctx->step, myPoint->frc);
    ELL_3V_NORM(deltaNorm, delta, deltaLen);
    if (task->pctx->numSS) {
      double newDelta = 0;

      /* learn change in scale at old position */
      if (_pushDeltaScale(task, &dscale, myPoint)) {
        sprintf(err, "%s: pre-update on %u", me, myPoint->ttaagg);
        biffAdd(PUSH, err); return 1;
      }
      /* update spatial position */
      if (deltaLen) {
        limit = task->pctx->deltaLimit*task->pctx->scale;
        newDelta = limit*deltaLen/(limit + deltaLen);
        /* by definition newDelta <= deltaLen */
        task->deltaFracSum += newDelta/deltaLen;
        ELL_3V_SCALE_INCR(myPoint->pos, newDelta, deltaNorm);
      }
      /* update scale-space position */
      myPoint->posSS += dscale;
      /* clamp to node-centered range (though gage can take cell-centered) */
      myPoint->posSS = AIR_CLAMP(0, myPoint->posSS, task->pctx->numSS-1);
      /* if anything changed, re-seek to surface */
      if (newDelta || dscale) {
        if (_pushSurfaceSeek(task, myPoint, 
                             newDelta ? newDelta : 0.05)) {
          sprintf(err, "%s: seeking post-update on %u", me, myPoint->ttaagg);
          biffAdd(PUSH, err); return 1;
        }
      }
    } else {
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
      if (1.0 <= task->pctx->probeProb
          || airDrandMT_r(task->rng) <= task->pctx->probeProb) {
        if (_pushProbe(task, myPoint)) {
          sprintf(err, "%s: probing at new field pos", me);
          biffAdd(PUSH, err); return 1;
        }
      }
    }
    
    /* the point lived, count it */
    task->pointNum += 1;
  } /* for myPointIdx */
  
  return 0;
}
