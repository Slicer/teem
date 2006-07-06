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

/* this is needed to make sure that tiny tractlets that are really just
   a bunch of vertices piled up on each other, and are not subjected
   to any kind of frenet frame calculation */
#define MIN_FRENET_LEN 0.05

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
  /* now we're probing the inverse from the pre-computed ninv
  TEN_T_INV(point->inv, point->ten, det);
  */
  TEN_T_COPY(point->inv, task->invAns);
  if (task->pctx->tlUse) {
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
_pushPairwiseForce(pushContext *pctx, double fvec[3], pushForce *force,
                   pushPoint *myPoint, pushPoint *herPoint) {
  /* char me[]="_pushPairwiseForce", err[BIFF_STRLEN]; */
  double inv[7], dist, mag,
    U[3], lenUsqr,
    V[3], nV[3], W[3], lenV;

#if 0
  if (force->noop) {
    /* no actual force */
    ELL_3V_SET(fvec, 0, 0, 0);
    return 0;
  }
#endif

  ELL_3V_SUB(U, herPoint->pos, myPoint->pos);
  lenUsqr = ELL_3V_DOT(U, U);
#if 0
  if (lenUsqr < FLT_EPSILON) {
    /* myPoint and herPoint are overlapping */
    /*
    fprintf(stderr, "%s: myPos == herPos == (%g,%g,%g)\n", me,
            myPoint->pos[0], myPoint->pos[1], myPoint->pos[2]);
    */
    ELL_3V_SET(fvec, 0, 0, 0);
    return 0;
  }
#endif
  if (lenUsqr >= (pctx->maxDist)*(pctx->maxDist)) {
    /* too far away to influence each other */
    ELL_3V_SET(fvec, 0, 0, 0);
    return 0;
  }

  TEN_T_SCALE_ADD2(inv,
                   0.5, myPoint->inv,
                   0.5, herPoint->inv);
  TEN_TV_MUL(V, inv, U);
  ELL_3V_NORM(nV, V, lenV);
  dist = lenV/(2*pctx->scale);

  mag = force->func(dist, force->parm);
  TEN_TV_MUL(W, inv, nV);
  ELL_3V_SCALE(fvec, mag, W);

#if 0
  if (!( AIR_EXISTS(mag) )) {
    fprintf(stderr, "!%s: myPos = %g %g %g\n", me,
            myPoint->pos[0], myPoint->pos[1], myPoint->pos[2]);
    fprintf(stderr, "!%s: mag=%g, dist=%g=(|V=%g,%g,%g|=%g)/%g\n",
            me, mag, dist,
            V[0], V[1], V[2], lenV,
            pctx->scale);
    fprintf(stderr, "!%s: U=%g,%g,%g\n", me, U[0], U[1], U[2]);
    ELL_3V_SET(fvec, 0, 0, 0);
  } else {
    TEN_TV_MUL(W, inv, nV);
    ELL_3V_SCALE(fvec, mag, W);
  }
#endif
  
  return 0;
}

#define THING_SIZE(pctx, thg) \
  (1 + (thg)->len/(2*(pctx)->meanEval*(pctx)->scale))

double
_pushThingMass(pushContext *pctx, pushThing *thg) {

  return pctx->mass*THING_SIZE(pctx, thg);
}

double
_pushThingPointCharge(pushContext *pctx, pushThing *thg) {

  return THING_SIZE(pctx, thg)/thg->vertNum;
}

#if 0
int
_pushForceSample(pushContext *pctx, unsigned int sx, unsigned int sy) {
  Nrrd *ntmp;
  double *data;
  unsigned int xi, yi, hi;
  double fsum[3], fvec[3];
  pushPoint _probe, *probe, *her;
  pushBin *bin, **neigh;

  probe = &_probe;
  ntmp = nrrdNew();
  nrrdMaybeAlloc_va(ntmp, nrrdTypeDouble, 3,
                    AIR_CAST(size_t, 3),
                    AIR_CAST(size_t, sx),
                    AIR_CAST(size_t, sy));
  data = (double*)ntmp->data;
  probe->pos[2] = 0.0;
  fprintf(stderr, "sampling force field"); fflush(stderr);
  for (yi=0; yi<sy; yi++) {
    fprintf(stderr, " %d/%d", yi, sy);
    probe->pos[1] = AIR_CAST(double,
                             AIR_AFFINE(0, yi, sy-1,
                                        pctx->minPos[1], pctx->maxPos[1]));
    for (xi=0; xi<sx; xi++) {
      probe->pos[0] = AIR_CAST(double,
                               AIR_AFFINE(0, xi, sx-1,
                                          pctx->minPos[0], pctx->maxPos[0]));
      _pushProbe(pctx->task[0], probe);
      bin = _pushBinLocate(pctx, probe->pos);
      neigh = bin->neighbor;
      ELL_3V_SET(fsum, 0, 0, 0);
      do {
        for (hi=0; hi<(*neigh)->pointNum; hi++) {
          her = (*neigh)->point[hi];
          _pushPairwiseForce(pctx, fvec, pctx->force, probe, her);
          ELL_3V_INCR(fsum, fvec);
        }
        neigh++;
      } while (*neigh);
      ELL_3V_COPY(data+ 3*(xi + sx*yi), fsum);
    }
  }
  fprintf(stderr, " done.\n");
  ntmp->axis[1].min = pctx->minPos[0];
  ntmp->axis[1].max = pctx->maxPos[0];
  ntmp->axis[2].min = pctx->minPos[1];
  ntmp->axis[2].max = pctx->maxPos[1];
  nrrdSave("pray.nrrd", ntmp, NULL);
  
  return 0;
}
#endif

int
_pushForce(pushTask *task, int myBinIdx,
           const double parm[PUSH_STAGE_PARM_MAXNUM]) {
  char me[]="_pushForce", err[BIFF_STRLEN];
  pushBin *myBin, *herBin, **neighbor;
  pushPoint *myPoint, *herPoint;
  pushThing *myThing;
  unsigned int myThingIdx, myPointIdx, herPointIdx, ci;
  double myCharge, herCharge, dir[3], drag, fvec[3];

  AIR_UNUSED(parm);

  myBin = task->pctx->bin + myBinIdx;

  /* initialize forces for things in this bin.  This is necessary
     for tractlets in this bin, and redundant (with following
     loop) for points in this bin */
  for (myThingIdx=0; myThingIdx<myBin->thingNum; myThingIdx++) {
    myThing = myBin->thing[myThingIdx];
    ELL_3V_SET(myThing->point.frc, 0, 0, 0);
  }

  /* compute pair-wise forces between all points in this bin,
     and all points in all neighboring bins */
  for (myPointIdx=0; myPointIdx<myBin->pointNum; myPointIdx++) {
    myPoint = myBin->point[myPointIdx];
    myCharge = myPoint->charge;
    ELL_3V_SET(myPoint->frc, 0, 0, 0);

    if (strcmp("none", task->pctx->force->name)) {
      neighbor = myBin->neighbor;
      while ((herBin = *neighbor)) {
        double theCharge;
        for (herPointIdx=0; herPointIdx<herBin->pointNum; herPointIdx++) {
          herPoint = herBin->point[herPointIdx];
          if (myPoint->thing == herPoint->thing) {
            /* there are no intra-thing forces */
            continue;
          }
          herCharge = herPoint->charge;
          /*
            task->pctx->verbose = (myPoint->thing->ttaagg == 398);
          */
          if (_pushPairwiseForce(task->pctx, fvec, task->pctx->force,
                                 myPoint, herPoint)) {
            sprintf(err, "%s: myPoint (thing %d) vs herPoint (thing %d)", me,
                    myPoint->thing->ttaagg, herPoint->thing->ttaagg);
            biffAdd(PUSH, err); return 1;
          }
          theCharge = task->pctx->forceScl*(myCharge + herCharge)/2;  
          ELL_3V_SCALE_INCR(myPoint->frc, theCharge, fvec);
          if (!ELL_3V_EXISTS(myPoint->frc)) {
            sprintf(err, "%s: point (thing %d) frc -> "
                    "NaN from point (thing %d) w/ fvec (%g,%g,%g)",
                    me, myPoint->thing->ttaagg,
                    herPoint->thing->ttaagg, fvec[0], fvec[1], fvec[2]);
            biffAdd(PUSH, err); return 1;
          }
          if (task->pctx->verbose) {
            fprintf(stderr, "   ... myPoint->frc = %g %g %g\n", 
                    myPoint->frc[0], myPoint->frc[1], myPoint->frc[2]);
          }
        }
        neighbor++;
      }
    }
    if (task->pctx->bigTrace) {
      double scl, trc;
      trc = TEN_T_TRACE(myPoint->ten);
      scl = task->pctx->bigTrace*trc/(task->pctx->bigTrace + trc);
      ELL_3V_SCALE(myPoint->frc, scl, myPoint->frc);
    }

    /* each point sees containment forces */
    ELL_3V_SCALE(fvec, task->pctx->cntScl, myPoint->cnt);
    ELL_3V_INCR(myPoint->frc, fvec);

    /* each point in this thing also potentially experiences gravity */
    if (tenGageUnknown != task->pctx->gravItem) {
      double tdot;
      ELL_3V_SCALE(fvec, task->pctx->gravScl, myPoint->grav);
      ELL_3V_INCR(myPoint->frc, fvec);
      if (task->gravNotAns[0]) {
        tdot = ELL_3V_DOT(myPoint->frc, task->gravNotAns[0]);
        ELL_3V_SCALE_INCR(myPoint->frc, -tdot, task->gravNotAns[0]);
      }
      if (task->gravNotAns[1]) {
        tdot = ELL_3V_DOT(myPoint->frc, task->gravNotAns[1]);
        ELL_3V_SCALE_INCR(myPoint->frc, -tdot, task->gravNotAns[1]);
      }
    }
  
    /* each point in this thing also potentially experiences wall forces */
    if (task->pctx->wall) {
      /* these wall forces are NOT the same as springs; the scheme
         ensures that the forces are C1, not just C0, which seems to
         help with stability */
      double posWorld[4], posIdx[4], len, frcIdx[4], frcWorld[4];
      ELL_3V_COPY(posWorld, myPoint->pos); posWorld[3] = 1.0;
      ELL_4MV_MUL(posIdx, task->pctx->gctx->shape->WtoI, posWorld);
      ELL_4V_HOMOG(posIdx, posIdx);
      for (ci=0; ci<3; ci++) {
        if (1 == task->pctx->gctx->shape->size[ci]) {
          frcIdx[ci] = 0;          
        } else {
          len = posIdx[ci] - 0.5;
          if (len < 0) {
            len *= -1;
            frcIdx[ci] = task->pctx->wall*len*len;
          } else {
            len = posIdx[ci] - (task->pctx->gctx->shape->size[ci] - 0.5);
            if (len > 0) {
              frcIdx[ci] = -task->pctx->wall*len*len;
            } else {
              frcIdx[ci] = 0;    
            }
          }
        }
      }
      frcIdx[3] = 0.0;
      ELL_4MV_MUL(frcWorld, task->pctx->gctx->shape->ItoW, frcIdx);
      ELL_3V_INCR(myPoint->frc, frcWorld);
    }
  } /* for myPointIdx ... */

  if (task->pctx->preDrag
      || task->pctx->drag
      || task->pctx->nudge) {
    /* drag and nudging are computed per-thing, not per-point */
    for (myThingIdx=0; myThingIdx<myBin->thingNum; myThingIdx++) {
      double len;
      myThing = myBin->thing[myThingIdx];
      if (task->pctx->minIter
          && task->pctx->iter < task->pctx->minIter) {
        drag = AIR_CAST(double,
                        AIR_AFFINE(0, task->pctx->iter, task->pctx->minIter,
                                   task->pctx->preDrag, task->pctx->drag));
      } else {
        drag = AIR_CAST(double, task->pctx->drag);
      }
      ELL_3V_SCALE_INCR(myThing->point.frc, -drag, myThing->point.vel);
      if (task->pctx->nudge) {
        ELL_3V_NORM(dir, myThing->point.pos, len);
        if (len) {
          ELL_3V_SCALE_INCR(myThing->point.frc, -task->pctx->nudge*len, dir);
        }
      }
    }
  }

  return 0;
}

int
_pushThingPointBe(pushTask *task, pushThing *thing, pushBin *oldBin) {
  char me[]="_pushThingPointBe", err[BIFF_STRLEN];
  unsigned int vertIdx;

  if (1 == thing->vertNum) {
    /* its already a point, so no points have to be nullified
       in the bins.  The point may be rebinned later, but so what. */
  } else {
    /* have to nullify, but not remove (because it wouldn't be
       thread-safe) any outstanding pointers to this point in bins */
    for (vertIdx=0; vertIdx<thing->vertNum; vertIdx++) {
      if (_pushBinPointNullify(task->pctx, NULL, thing->vert + vertIdx)) {
        sprintf(err, "%s(%d): couldn't nullify vertex %d of thing %p",
                me, task->threadIdx,
                vertIdx, AIR_CAST(void*, thing));
        biffAdd(PUSH, err); return 1;
      }
    }
    /* the now-single point does have to be binned */
    _pushBinPointAdd(task->pctx, oldBin, &(thing->point));
    thing->point.charge = (task->pctx->tlUse 
                           ? _pushThingPointCharge(task->pctx, thing)
                           : 1.0);
    /* free vertex info */
    airFree(thing->vert);
    thing->vert = &(thing->point);
    thing->vertNum = 1;
    thing->len = 0;
    thing->seedIdx = 0;
  }
  return 0;
}

int
_pushThingTractletBe(pushTask *task, pushThing *thing, pushBin *oldBin) {
  char me[]="_pushThingTractletBe", err[BIFF_STRLEN];
  unsigned int vertIdx, startIdx, endIdx, vertNum;
  int tret;
  double seed[3], tmp;

  /* NOTE: the seed point velocity remains as the tractlet velocity */

  ELL_3V_COPY(seed, thing->point.pos);
  tret = tenFiberTraceSet(task->fctx, NULL, 
                          task->vertBuff, task->pctx->tlStepNum,
                          &startIdx, &endIdx, seed);
  if (tret) {
    sprintf(err, "%s(%d): fiber tracing failed", me, task->threadIdx);
    biffMove(PUSH, err, TEN); return 1;
  }
  if (task->fctx->whyNowhere) {
    sprintf(err, "%s(%d): fiber tracing got nowhere: %d == %s\n", me,
            task->threadIdx,
            task->fctx->whyNowhere,
            airEnumDesc(tenFiberStop, task->fctx->whyNowhere));
    biffAdd(PUSH, err); return 1;
  }
  vertNum = endIdx - startIdx + 1;
  if (!( vertNum >= 3 )) {
    sprintf(err, "%s(%d): vertNum only %d < 3", me, task->threadIdx, vertNum);
    biffAdd(PUSH, err); return 1;
  }

  /* remember the length */
  thing->len = 
    AIR_CAST(double, task->fctx->halfLen[0] + task->fctx->halfLen[1]);

  /* allocate tractlet vertices as needed */
  if (vertNum != thing->vertNum) {
    if (1 == thing->vertNum) {
      /* it used to be a point, nullify old bin's pointer to it */
      if (_pushBinPointNullify(task->pctx, oldBin, &(thing->point))) {
        sprintf(err, "%s(%d): couldn't nullify former point %p of thing %p",
                me, task->threadIdx,
                AIR_CAST(void*, &(thing->point)), AIR_CAST(void*, thing));
        biffAdd(PUSH, err); return 1;
      }
    } else {
      /* it used to be a tractlet (but w/ different vertNum); verts still
         have the position information so that we can recover the old bins */
      for (vertIdx=0; vertIdx<thing->vertNum; vertIdx++) {
        if (_pushBinPointNullify(task->pctx, NULL, thing->vert + vertIdx)) {
          sprintf(err, "%s(%d): couldn't nullify old vert %d %p of thing %p",
                  me, task->threadIdx, vertIdx,
                  AIR_CAST(void*, thing->vert + vertIdx),
                  AIR_CAST(void*, thing));
          biffAdd(PUSH, err); return 1;
        }
      }
      airFree(thing->vert);
    }
    thing->vert = (pushPoint*)calloc(vertNum, sizeof(pushPoint));
    thing->vertNum = vertNum;
    /* put tractlet points into last bin we were, since we can do so in a 
       thread-safe way; later they will be re-binned */
    for (vertIdx=0; vertIdx<thing->vertNum; vertIdx++) {
      _pushBinPointAdd(task->pctx, oldBin, thing->vert + vertIdx);
    }
  }

  /* copy from fiber tract vertex buffer */
  for (vertIdx=0; vertIdx<vertNum; vertIdx++) {
    thing->vert[vertIdx].thing = thing;
    ELL_3V_COPY(thing->vert[vertIdx].pos,
                task->vertBuff + 3*(startIdx + vertIdx));
    _pushProbe(task, thing->vert + vertIdx);
    thing->vert[vertIdx].charge = _pushThingPointCharge(task->pctx, thing);
  }
  thing->seedIdx = task->pctx->tlStepNum - startIdx;

  /* compute tangent at all vertices */
  if (task->pctx->tlFrenet && thing->len > MIN_FRENET_LEN) {
    ELL_3V_SUB(thing->vert[0].tan, thing->vert[1].pos, thing->vert[0].pos);
    ELL_3V_NORM(thing->vert[0].tan,
                thing->vert[0].tan, tmp);
    for (vertIdx=1; vertIdx<vertNum-1; vertIdx++) {
      ELL_3V_SUB(thing->vert[vertIdx].tan,
                 thing->vert[vertIdx+1].pos,
                 thing->vert[vertIdx-1].pos);
      ELL_3V_NORM(thing->vert[vertIdx].tan,
                  thing->vert[vertIdx].tan, tmp);
    }
    ELL_3V_SUB(thing->vert[vertNum-1].tan,
             thing->vert[vertNum-1].pos,
               thing->vert[vertNum-2].pos);
    ELL_3V_NORM(thing->vert[vertNum-1].tan,
                thing->vert[vertNum-1].tan, tmp);
    
    /* compute "normal" at all vertices */
    for (vertIdx=1; vertIdx<vertNum-1; vertIdx++) {
      ELL_3V_CROSS(thing->vert[vertIdx].nor,
                   thing->vert[vertIdx+1].tan,
                   thing->vert[vertIdx-1].tan);
      ELL_3V_NORM(thing->vert[vertIdx].nor,
                  thing->vert[vertIdx].nor, tmp);
      tmp = ELL_3V_LEN(thing->vert[vertIdx].nor);
      if (!AIR_EXISTS(tmp)) {
        fprintf(stderr, "(%d) (%g,%g,%g) X (%g,%g,%g) = "
                "%g %g %g --> %g\n", vertIdx,
                (thing->vert[vertIdx+1].tan)[0],
                (thing->vert[vertIdx+1].tan)[1],
                (thing->vert[vertIdx+1].tan)[2],
                (thing->vert[vertIdx-1].tan)[0],
                (thing->vert[vertIdx-1].tan)[1],
                (thing->vert[vertIdx-1].tan)[2],
                thing->vert[vertIdx].nor[0],
                thing->vert[vertIdx].nor[1],
                thing->vert[vertIdx].nor[2],
                ELL_3V_LEN(thing->vert[vertIdx].nor));
        exit(1);
      }
    }
    ELL_3V_COPY(thing->vert[0].nor, thing->vert[1].nor);
    ELL_3V_COPY(thing->vert[vertNum-1].nor, thing->vert[vertNum-2].nor);
  }

  return 0;
}

void
_pushPrintForce(pushContext *pctx, pushThing *thing) {
  int posI[3], frcI[3];
  double pos[3], frc[3];
  unsigned int vi;

#define TMP_3V_SCALE(v2, a, v1) \
  ((v2)[0] = (int)((a)*(v1)[0]),       \
   (v2)[1] = (int)((a)*(v1)[1]),       \
   (v2)[2] = (int)((a)*(v1)[2]))

  AIR_UNUSED(pctx);
  TMP_3V_SCALE(posI, 1000000, thing->point.pos);
  ELL_3V_SCALE(pos, 1.0/1000000, posI);
  TMP_3V_SCALE(frcI, 1000000, thing->point.frc);
  ELL_3V_SCALE(frc, 1.0/1000000, frcI);
  fprintf(stderr, "% 4d@(% 6.6f,% 6.6f)(% 6.6f,% 6.6f)",
          thing->ttaagg, pos[0], pos[1], frc[0], frc[1]);
  for (vi=0; vi<thing->vertNum; vi++) {
    TMP_3V_SCALE(frcI, 1000000, thing->vert[vi].frc);
    ELL_3V_SCALE(frc, 1.0/1000000, frcI);
    fprintf(stderr, "--(% 6.6f,% 6.6f)", frc[0], frc[1]);
  }
  fprintf(stderr, "\n");

#undef TMP_3V_SCALE
}

int
_pushUpdate(pushTask *task, int binIdx,
            const double parm[PUSH_STAGE_PARM_MAXNUM]) {
  char me[]="_pushUpdate", err[BIFF_STRLEN];
  int ret, inside;
  unsigned int thingIdx, vertIdx;
  double step, mass;
  double fTNB[3], binorm[3], fvec[3];
  pushBin *bin;
  pushThing *thing;
  pushPoint *point, *seedPoint;

  AIR_UNUSED(parm);
  step = task->pctx->step;
  bin = task->pctx->bin + binIdx;
  for (thingIdx=0; thingIdx<bin->thingNum; thingIdx++) {
    thing = bin->thing[thingIdx];
    /*
    task->pctx->verbose = (thing->ttaagg == 29);
    */
    /* convert per-vertex forces on tractlet to total force */
    if (thing->vertNum > 1) {
      ELL_3V_SET(fvec, 0, 0, 0);
      if (task->pctx->tlFrenet && thing->len > MIN_FRENET_LEN) {
        ELL_3V_SET(fTNB, 0, 0, 0);
        for (vertIdx=0; vertIdx<thing->vertNum; vertIdx++) {
          point = thing->vert + vertIdx;
          ELL_3V_CROSS(binorm, point->tan, point->nor);
          fTNB[0] += ELL_3V_DOT(point->frc, point->tan);
          fTNB[1] += ELL_3V_DOT(point->frc, point->nor);
          fTNB[2] += ELL_3V_DOT(point->frc, binorm);
        }
        seedPoint = thing->vert + thing->seedIdx;
        ELL_3V_CROSS(binorm, seedPoint->tan, seedPoint->nor);
        ELL_3V_SCALE_INCR(fvec, fTNB[0], seedPoint->tan);
        ELL_3V_SCALE_INCR(fvec, fTNB[1], seedPoint->nor);
        ELL_3V_SCALE_INCR(fvec, fTNB[2], binorm);
      } else {
        for (vertIdx=0; vertIdx<thing->vertNum; vertIdx++) {
          point = thing->vert + vertIdx;
          ELL_3V_INCR(fvec, point->frc);
        }
      }
      /* we have to add this on ("INCR") and not just set it,
         because the drag and nudge forces were already stored
         here during the force stage */
      /* ELL_3V_SCALE_INCR(thing->point.frc, 1.0/thing->vertNum, fvec); */
      ELL_3V_INCR(thing->point.frc, fvec);
    }

    /* POST */
    if (task->pctx->verbose) {
      fprintf(stderr, "final: %f,%f\n", 
              thing->point.frc[0], thing->point.frc[1]);
    }

    /* update dynamics: applies equally to points and tractlets */
    mass = task->pctx->tlUse ? _pushThingMass(task->pctx, thing) : 1.0;
    if (task->pctx->verbose) {
      fprintf(stderr, "vel(%f,%f) * step(%f) -+-> pos(%f,%f)\n", 
              thing->point.vel[0], thing->point.vel[0], step,
              thing->point.pos[0], thing->point.pos[1]);
      fprintf(stderr, "frc(%f,%f)*step(%f)/mass(%f) (%f) -+-> vel(%f,%f)\n", 
              thing->point.frc[0], thing->point.frc[0],
              step, mass, step/mass,
              thing->point.vel[0], thing->point.vel[1]);
    }
    if (!ELL_3V_EXISTS(thing->point.vel)) {
      ELL_3V_SET(thing->point.vel, 0, 0, 0);
    }
    if (2 == task->pctx->dimIn) {
      double posIdx[4], posWorld[4];
      ELL_3V_COPY(posWorld, thing->point.pos); posWorld[3] = 1.0;
      ELL_4MV_MUL(posIdx, task->pctx->gctx->shape->WtoI, posWorld);
      ELL_4V_HOMOG(posIdx, posIdx);
      posIdx[task->pctx->sliceAxis] = 0.0;
      ELL_4MV_MUL(posWorld, task->pctx->gctx->shape->ItoW, posIdx);
      ELL_34V_HOMOG(thing->point.pos, posWorld);
    }
    if (!ELL_3V_EXISTS(thing->point.frc)) {
      ELL_3V_SET(thing->point.frc, 0, 0, 0);
    }
    if (task->pctx->velWarp) {
      /* HEY: try to optimize out some sqrt()s */
      double dpos[3], ndpos[3], dposLen, tmp[3], rad, speed, wspeed;
      /* dpos = step that we would take, without velocity warping */
      ELL_3V_SCALE(dpos, step*step/mass, thing->point.frc);
      dposLen = ELL_3V_LEN(dpos);
      if (dposLen) {
        ELL_3V_SCALE(ndpos, 1.0/dposLen, dpos);
      } else {
        ELL_3V_SET(ndpos, 1, 0, 0);
      }
      TEN_T3V_MUL(tmp, thing->point.inv, ndpos);
      /* speed = magnitude of velocity in glyphs per time step */
      rad = task->pctx->scale/ELL_3V_LEN(tmp);
      speed = dposLen / rad;
      wspeed = task->pctx->velWarp*speed/(task->pctx->velWarp + speed);
      /* fix thing->point.vel accordingly */
      ELL_3V_SCALE_INCR(thing->point.vel,
                        task->pctx->scale*wspeed/step, ndpos);
    } else {
      ELL_3V_SCALE_INCR(thing->point.vel, step/mass, thing->point.frc);
    }
    if (task->pctx->verbose) {
      fprintf(stderr, "thing %d: pos(%f,%f); vel(%f,%f)\n",
              thing->ttaagg,
              thing->point.pos[0], thing->point.pos[1],
              thing->point.vel[0], thing->point.vel[0]);
      fprintf(stderr, "sumVel = %f ---> ", task->sumVel);
    }
    task->sumVel += ELL_3V_LEN(thing->point.vel);
    if (task->pctx->verbose) {
      fprintf(stderr, "%f (exists %d)\n", task->sumVel,
              AIR_EXISTS(task->sumVel));
    }
    if (!AIR_EXISTS(task->sumVel)) {
      sprintf(err, "%s(%d): sumVel went NaN (from vel (%g,%g,%g), force "
              "(%g,%g,%g)) on thing %d (%d verts) %p of bin %d", 
              me, task->threadIdx,
              thing->point.vel[0],
              thing->point.vel[1],
              thing->point.vel[2],
              thing->point.frc[0],
              thing->point.frc[1],
              thing->point.frc[2],
              thing->ttaagg, thing->vertNum, AIR_CAST(void*, thing), binIdx);
      biffAdd(PUSH, err); return 1;
    }
    ELL_3V_SCALE_INCR(thing->point.pos, step, thing->point.vel);
    
    task->thingNum += 1;
    /* while _pushProbe clamps positions to inside domain before
       calling gageProbe, we can exert no such control over the gageProbe
       called within tenFiberTraceSet.  So for now, things turn to points
       as soon as they leave the domain.  Tough luck. */
    /* sample field at new point location */
    inside = _pushProbe(task, &(thing->point));
    /* be a point or tractlet, depending on anisotropy (and location) */
    if (inside && (thing->point.aniso 
                   >= (task->pctx->tlThresh - task->pctx->tlSoft))) {
      ret = _pushThingTractletBe(task, thing, bin);
    } else {
      ret = _pushThingPointBe(task, thing, bin);
    }
    if (ret) {
      sprintf(err, "%s(%d): trouble updating thing %d %p of bin %d",
              me, task->threadIdx,
              thing->ttaagg, AIR_CAST(void*, thing), binIdx);
      biffAdd(PUSH, err); return 1;
    }
  } /* for thingIdx */
  return 0;
}

