/*
  Teem: Gordon Kindlmann's research software
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* this file is where the interesting stuff is */

#include "push.h"
#include "privatePush.h"

/*
** creates nten and gctx
** sets tenAns
*/
int
_pushInputProcess(pushContext *pctx) {
  char me[]="_pushInputProcess", err[AIR_STRLEN_MED];
  Nrrd *seven[7], *two[2];
  Nrrd *ntmp;
  NrrdRange *nrange;
  airArray *mop;
  int E, ni;
  gagePerVolume *pvl;

  pctx->nten = nrrdNew();
  fprintf(stderr, "!%s: bingo 0\n", me);
  mop = airMopNew();
  ntmp = nrrdNew();
  airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
  E = AIR_FALSE;
  if (3 == pctx->nin->dim) {
    /* input is 2D array of 2D tensors */
    pctx->dimIn = 2;
    for (ni=0; ni<7; ni++) {
      if (ni < 2) {
        two[ni] = nrrdNew();
        airMopAdd(mop, two[ni], (airMopper)nrrdNuke, airMopAlways);
      }
      seven[ni] = nrrdNew();
      airMopAdd(mop, seven[ni], (airMopper)nrrdNuke, airMopAlways);
    }
    /*    (0)         (0)
     *     1  2  3     1  2
     *        4  5        3
     *           6            */
    if (!E) E |= nrrdSlice(seven[0], pctx->nin, 0, 0);
    if (!E) E |= nrrdSlice(seven[1], pctx->nin, 0, 1);
    if (!E) E |= nrrdSlice(seven[2], pctx->nin, 0, 2);
    if (!E) E |= nrrdArithUnaryOp(seven[3], nrrdUnaryOpZero, seven[0]);
    if (!E) E |= nrrdSlice(seven[4], pctx->nin, 0, 3);
    if (!E) E |= nrrdArithUnaryOp(seven[5], nrrdUnaryOpZero, seven[0]);
    if (!E) E |= nrrdArithUnaryOp(seven[6], nrrdUnaryOpZero, seven[0]);
    if (!E) E |= nrrdJoin(two[0], (const Nrrd *const *)seven, 7, 0, AIR_TRUE);
    if (!E) E |= nrrdCopy(two[1], two[0]);
    if (!E) E |= nrrdJoin(ntmp, (const Nrrd *const *)two, 2, 3, AIR_TRUE);
    if (!E) E |= nrrdConvert(pctx->nten, ntmp, nrrdTypeFloat);
  } else {
    /* input was already 3D */
    pctx->dimIn = 3;
    E = nrrdConvert(pctx->nten, pctx->nin, nrrdTypeFloat);
  }
  if (!E) E |= nrrdSlice(ntmp, pctx->nten, 0, 0);
  if (E) {
    sprintf(err, "%s: trouble creating 3D tensor input", me);
    biffMove(PUSH, err, NRRD); airMopError(mop); return 1;
  }
  nrange = nrrdRangeNewSet(pctx->nten, nrrdBlind8BitRangeFalse);
  airMopAdd(mop, nrange, (airMopper)nrrdRangeNix, airMopAlways);
  if (AIR_ABS(1.0 - nrange->max) > 0.01) {
    sprintf(err, "%s: max value in tensor mask is %g, not 1.0", me,
            nrange->max);
    biffAdd(PUSH, err); airMopError(mop); return 1;
  }
  pctx->nten->axis[1].spacing = (AIR_EXISTS(pctx->nten->axis[1].spacing)
                                 ? pctx->nten->axis[1].spacing
                                 : 1.0);
  pctx->nten->axis[2].spacing = (AIR_EXISTS(pctx->nten->axis[2].spacing)
                                 ? pctx->nten->axis[2].spacing
                                 : 1.0);
  pctx->nten->axis[3].spacing = (AIR_EXISTS(pctx->nten->axis[3].spacing)
                                 ? pctx->nten->axis[3].spacing
                                 : 1.0);
  /* HEY: we're only doing this because gage has a bug with
     cell-centered volume 1 sample thick- perhaps there should
     be a warning ... */
  pctx->nten->axis[1].center = nrrdCenterNode;
  pctx->nten->axis[2].center = nrrdCenterNode;
  pctx->nten->axis[3].center = nrrdCenterNode;

  pctx->gctx = gageContextNew();
  E = AIR_FALSE;
  if (!E) E |= !(pvl = gagePerVolumeNew(pctx->gctx, pctx->nten, tenGageKind));
  if (!E) E |= gagePerVolumeAttach(pctx->gctx, pvl);
  if (!E) E |= gageKernelSet(pctx->gctx, gageKernel00,
                             pctx->kernel, pctx->kparm);
  if (!E) E |= gageQueryItemOn(pctx->gctx, pvl, tenGageTensor);
  if (!E) E |= gageUpdate(pctx->gctx);
  if (E) {
    sprintf(err, "%s: trouble setting up gage", me);
    biffMove(PUSH, err, GAGE); airMopError(mop); return 1;
  }
  pctx->tenAns = gageAnswerPointer(pctx->gctx, pvl, tenGageTensor);
  gageParmSet(pctx->gctx, gageParmRequireAllSpacings, AIR_FALSE);

  ELL_3V_SCALE(pctx->minPos, -1, pctx->gctx->shape->volHalfLen);
  ELL_3V_SCALE(pctx->maxPos, 1, pctx->gctx->shape->volHalfLen);

  airMopOkay(mop);
  fprintf(stderr, "!%s: bingo 100 \n%d %d %d %d\n%g %g %g\n", me,
          pctx->nten->axis[0].size,
          pctx->nten->axis[1].size,
          pctx->nten->axis[2].size,
          pctx->nten->axis[3].size,
          pctx->maxPos[0], pctx->maxPos[1], pctx->maxPos[2]);
  return 0;
}

int
pushOutputGet(Nrrd *nPosOut, Nrrd *nTenOut, pushContext *pctx) {
  char me[]="pushOutputGet", err[AIR_STRLEN_MED];
  int min[2], max[2], npt, pi;
  float *tdata;
  push_t *posVel;

  npt = pctx->nPosVel->axis[1].size;
  if (nPosOut) {
    min[0] = 0;
    min[1] = 0;
    max[0] = (2 == pctx->dimIn ? 1 : 2);
    max[1] = npt - 1;
    if (nrrdCrop(nPosOut, pctx->nPosVel, min, max)) {
      sprintf(err, "%s: couldn't crop to recover position output", me);
      biffMove(PUSH, err, NRRD); return 1;
    }
  }
  if (nTenOut) {
    /*    (0)         (0)
     *     1  2  3     1  2
     *        4  5        3
     *           6            */
    if (nrrdMaybeAlloc(nTenOut, nrrdTypeFloat, 2,
                       (2 == pctx->dimIn ? 4 : 7), npt)) {
      sprintf(err, "%s: couldn't allocate tensor output", me);
      biffMove(PUSH, err, NRRD); return 1;
    }
    tdata = (float*)nTenOut->data;
    posVel = (push_t*)pctx->nPosVel->data;
    for (pi=0; pi<npt; pi++) {
      _pushProbe(pctx, pctx->gctx, posVel[0], posVel[1], posVel[2]);
      if (2 == pctx->dimIn) {
        tdata[0] = pctx->tenAns[0];
        tdata[1] = pctx->tenAns[1];
        tdata[2] = pctx->tenAns[2];
        tdata[3] = pctx->tenAns[4];
        tdata += 4;
      } else {
        TEN_T_COPY(tdata, pctx->tenAns);
        tdata += 7;
      }
      posVel += 6;
    }
  }

  return 0;
}

void
_pushProbe(pushContext *pctx, gageContext *gctx,
           double x, double y, double z) {
  double xi, yi, zi;
  int max0, max1, max2;

  /* HEY: this assumes node centering */
  max0 = pctx->nten->axis[1].size - 1;
  max1 = pctx->nten->axis[2].size - 1;
  max2 = pctx->nten->axis[3].size - 1;
  xi = AIR_AFFINE(pctx->minPos[0], x, pctx->maxPos[0], 0, max0);
  yi = AIR_AFFINE(pctx->minPos[1], y, pctx->maxPos[1], 0, max1);
  zi = AIR_AFFINE(pctx->minPos[2], z, pctx->maxPos[2], 0, max2);
  xi = AIR_CLAMP(0, xi, max0);
  yi = AIR_CLAMP(0, yi, max1);
  zi = AIR_CLAMP(0, zi, max2);
  gageProbe(gctx, xi, yi, zi);
  return;
}

void
_pushInitialize(pushContext *pctx) {
  int npt, pi;
  push_t *pos, *vel;

  /*
  {
    Nrrd *ntmp;
    double *data;
    int sx, sy, xi, yi;
    double p[3];
    
    sx = 300;
    sy = 300;
    ntmp = nrrdNew();
    nrrdMaybeAlloc(ntmp, nrrdTypeDouble, 2, sx, sy);
    data = (double*)ntmp->data;
    p[2] = 0.0;
    for (yi=0; yi<sy; yi++) {
      p[1] = AIR_AFFINE(0, yi, sy-1, pctx->minPos[1], pctx->maxPos[1]);
      for (xi=0; xi<sx; xi++) {
        p[0] = AIR_AFFINE(0, xi, sx-1, pctx->minPos[0], pctx->maxPos[0]);
        _pushProbe(pctx, pctx->gctx, p[0], p[1], p[2]);
        data[xi + sx*yi] = pctx->tenAns[0];
      }
    }
    nrrdSave("pray.nrrd", ntmp, NULL);
  }
  */
  
  npt = pctx->nPosVel->axis[1].size;
  for (pi=0; pi<npt; pi++) {
    pos = (push_t *)(pctx->nPosVel->data) + 2*3*pi;
    vel = pos + 3;
    ELL_3V_SET(vel, 0, 0, 0);
    do {
      pos[0] = AIR_AFFINE(0.0, airDrand48(), 1.0,
                          pctx->minPos[0], pctx->maxPos[0]);
      pos[1] = AIR_AFFINE(0.0, airDrand48(), 1.0,
                          pctx->minPos[1], pctx->maxPos[1]);
      if (2 == pctx->dimIn) {
        pos[2] = 0;
      } else {
        pos[2] = AIR_AFFINE(0.0, airDrand48(), 1.0,
                            pctx->minPos[2], pctx->maxPos[2]);
      }
      _pushProbe(pctx, pctx->gctx, pos[0], pos[1], pos[2]);
    } while (pctx->tenAns[0] < 0.5);
  }

  /* HEY: this should be done by the user */
  pctx->process[0] = _pushRepel;
  pctx->process[1] = _pushUpdate;

  return;
}

void
_pushForceIncr(push_t force[3],
               push_t myten[7], push_t mypos[3], push_t *there) {
  push_t diff[3], tmp[3], ctr, len;
  
  ELL_3V_SUB(diff, mypos, there);
  ELL_3V_NORM(diff, diff, len);
  /*  (0)
   *   1  2  3
   *   2  4  5
   *   3  5  6 
   */
  tmp[0] = myten[1]*diff[0] + myten[2]*diff[1] + myten[3]*diff[2];
  tmp[1] = myten[2]*diff[0] + myten[4]*diff[1] + myten[5]*diff[2];
  tmp[2] = myten[3]*diff[0] + myten[5]*diff[1] + myten[6]*diff[2];
  ctr = ELL_3V_DOT(tmp, diff);
  ELL_3V_SCALE_INCR(force, ctr/(len*len), diff);
  return;
}

void
_pushRepel(pushTask *task, int batch,
           double parm[PUSH_STAGE_PARM_MAX]) {
  push_t *mypos, myten[7], *posVel, *velAcc, force[3], tmp[3];
  int pi, pj, npt, ppb;

  npt = task->pctx->nPosVel->axis[1].size;
  ppb = task->pctx->pointsPerBatch;
  posVel = (push_t *)task->pctx->nPosVel->data;
  velAcc = (push_t *)task->pctx->nVelAcc->data;
  for (pi=batch*ppb; pi<(batch+1)*ppb; pi++) {
    mypos = posVel + 3*(0 + 2*pi);
    _pushProbe(task->pctx, task->gctx, mypos[0], mypos[1], mypos[2]);
    TEN_T_COPY(myten, task->tenAns);
    ELL_3V_SET(force, 0, 0, 0);
    for (pj=0; pj<npt; pj++) {
      if (pi == pj) {
        continue;
      }
      _pushForceIncr(force, myten, mypos, posVel + 3*(0 + 2*pj));
    }
    ELL_3V_COPY(tmp, mypos);
    tmp[0] = 2*task->pctx->minPos[0] - tmp[0];
    _pushForceIncr(force, myten, mypos, tmp);
    ELL_3V_COPY(tmp, mypos);
    tmp[0] = 2*task->pctx->maxPos[0] - tmp[0];
    _pushForceIncr(force, myten, mypos, tmp);
    ELL_3V_COPY(tmp, mypos);
    tmp[1] = 2*task->pctx->minPos[1] - tmp[1];
    _pushForceIncr(force, myten, mypos, tmp);
    ELL_3V_COPY(tmp, mypos);
    tmp[1] = 2*task->pctx->maxPos[1] - tmp[1];
    _pushForceIncr(force, myten, mypos, tmp);
    ELL_3V_SCALE_INCR(force, -task->pctx->drag, posVel + 3*(1 + 2*pi));
    ELL_3V_COPY(velAcc + 3*(0 + 2*pi), posVel + 3*(1 + 2*pi));
    ELL_3V_COPY(velAcc + 3*(1 + 2*pi), force);
  }
  return;
}

void
_pushUpdate(pushTask *task, int batch,
            double parm[PUSH_STAGE_PARM_MAX]) {
  push_t *posVel, *velAcc;
  int pi, ppb;
  double dt, move;

  ppb = task->pctx->pointsPerBatch;
  dt = task->pctx->step;
  posVel = (push_t *)task->pctx->nPosVel->data;
  velAcc = (push_t *)task->pctx->nVelAcc->data;
  for (pi=batch*ppb; pi<(batch+1)*ppb; pi++) {
    _pushProbe(task->pctx, task->gctx,
               (posVel + 3*(0 + 2*pi))[0],
               (posVel + 3*(0 + 2*pi))[1],
               (posVel + 3*(0 + 2*pi))[2]);
    move = task->tenAns[0] > 0.5;
    ELL_3V_SCALE_INCR(posVel + 3*(0 + 2*pi), move*dt, velAcc + 3*(0 + 2*pi));
    ELL_3V_SCALE_INCR(posVel + 3*(1 + 2*pi), move*dt, velAcc + 3*(1 + 2*pi));
    ELL_3V_SCALE(posVel + 3*(1 + 2*pi), move, posVel + 3*(1 + 2*pi));
    task->sumVel += ELL_3V_LEN(posVel + 3*(1 + 2*pi));
  }
  return;
}

int
pushRun(pushContext *pctx) {
  char me[]="pushRun", err[AIR_STRLEN_MED],
    poutS[AIR_STRLEN_MED], toutS[AIR_STRLEN_MED];
  Nrrd *npos, *nten;
  int iter;

  iter = 0;
  pctx->time0 = airTime();
  do {
    if (pushIterate(pctx)) {
      sprintf(err, "%s: trouble on iter %d", me, iter);
      biffAdd(PUSH, err); return 1;
    }
    if (pctx->snap && !(iter % pctx->snap)) {
      nten = nrrdNew();
      npos = nrrdNew();
      sprintf(poutS, "snap-%06d-pos.nrrd", iter);
      sprintf(toutS, "snap-%06d-ten.nrrd", iter);
      if (pushOutputGet(npos, nten, pctx)) {
        sprintf(err, "%s: couldn't get snapshot for iter %d", me, iter);
        biffAdd(PUSH, err); return 1;
      }
      fprintf(stderr, "%s: saving snapshot %s (meanVel = %g)\n",
              me, poutS, pctx->meanVel);
      if (nrrdSave(poutS, npos, NULL)
          || nrrdSave(toutS, nten, NULL)) {
        sprintf(err, "%s: couldn't save snapshot for iter %d", me, iter);
        biffMove(PUSH, err, NRRD); return 1;
      }
      nten = nrrdNuke(nten);
      npos = nrrdNuke(npos);
    }
    iter++;
  } while (pctx->meanVel > pctx->minMeanVel
           && (0 == pctx->maxIter
               || iter < pctx->maxIter));
  pctx->time1 = airTime();
  pctx->time = pctx->time1 - pctx->time0;

  return 0;
}
