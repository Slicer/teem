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


#include "push.h"
#include "privatePush.h"

void
_pushInitialize(pushContext *pctx) {
  int npt, pi;
  push_t *posVel;
  
  npt = pctx->nPosVel->axis[1].size;
  posVel = (push_t *)pctx->nPosVel->data;
  for (pi=0; pi<npt; pi++) {
    posVel[0 + 3*(0 + 2*pi)] = airDrand48();
    posVel[1 + 3*(0 + 2*pi)] = airDrand48();
    posVel[2 + 3*(0 + 2*pi)] = 0;
    posVel[0 + 3*(1 + 2*pi)] = 0;
    posVel[1 + 3*(1 + 2*pi)] = 0;
    posVel[2 + 3*(1 + 2*pi)] = 0;
  }

  /* HEY: this should be done by the user */
  pctx->process[0] = _pushRepel;
  pctx->process[1] = _pushUpdate;

  return;
}

void
_pushRepel(pushTask *task, int batch,
           double parm[PUSH_STAGE_PARM_MAX]) {
  push_t *mypos, *posVel, *velAcc, diff[3], force[3], tmp[3], len;
  int pi, pj, npt, ppb;

  npt = task->pctx->nPosVel->axis[1].size;
  ppb = task->pctx->pointsPerBatch;
  posVel = (push_t *)task->pctx->nPosVel->data;
  velAcc = (push_t *)task->pctx->nVelAcc->data;
  for (pi=batch*ppb; pi<(batch+1)*ppb; pi++) {
    mypos = posVel + 3*(0 + 2*pi);
    ELL_3V_SET(force, 0, 0, 0);
    for (pj=0; pj<npt; pj++) {
      if (pi == pj) {
        continue;
      }
      ELL_3V_SUB(diff, mypos, posVel + 3*(0 + 2*pj));
      ELL_3V_NORM(diff, diff, len);
      ELL_3V_SCALE_INCR(force, 1/(len*len), diff);
    }
    ELL_3V_COPY(tmp, mypos);
    tmp[0] = 0;
    ELL_3V_SUB(diff, mypos, tmp);
    ELL_3V_NORM(diff, diff, len);
    ELL_3V_SCALE_INCR(force, 1/(len*len), diff);
    ELL_3V_COPY(tmp, mypos);
    tmp[0] = 1;
    ELL_3V_SUB(diff, mypos, tmp);
    ELL_3V_NORM(diff, diff, len);
    ELL_3V_SCALE_INCR(force, 1/(len*len), diff);
    ELL_3V_COPY(tmp, mypos);
    tmp[1] = 0;
    ELL_3V_SUB(diff, mypos, tmp);
    ELL_3V_NORM(diff, diff, len);
    ELL_3V_SCALE_INCR(force, 1/(len*len), diff);
    ELL_3V_COPY(tmp, mypos);
    tmp[1] = 1;
    ELL_3V_SUB(diff, mypos, tmp);
    ELL_3V_NORM(diff, diff, len);
    ELL_3V_SCALE_INCR(force, 1/(len*len), diff);
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
  double dt, vel;

  ppb = task->pctx->pointsPerBatch;
  dt = task->pctx->step;
  posVel = (push_t *)task->pctx->nPosVel->data;
  velAcc = (push_t *)task->pctx->nVelAcc->data;
  vel = 0;
  for (pi=batch*ppb; pi<(batch+1)*ppb; pi++) {
    ELL_3V_SCALE_INCR(posVel + 3*(0 + 2*pi), dt, velAcc + 3*(0 + 2*pi));
    ELL_3V_SCALE_INCR(posVel + 3*(1 + 2*pi), dt, velAcc + 3*(1 + 2*pi));
    vel += ELL_3V_LEN(posVel + 3*(1 + 2*pi));
  }
  task->meanVel += vel/ppb;
  return;
}

int
pushRun(pushContext *pctx) {
  char me[]="pushRun", err[AIR_STRLEN_MED], outS[AIR_STRLEN_MED];
  Nrrd *ntmp;
  int iter;

  iter = 0;
  pctx->time0 = airTime();
  do {
    if (pushIterate(pctx)) {
      sprintf(err, "%s: trouble on iter %d", me, iter);
      biffAdd(PUSH, err); return 1;
    }
    if (pctx->snap && !(iter % pctx->snap)) {
      ntmp = nrrdNew();
      sprintf(outS, "%06d.nrrd", iter);
      if (pushOutputGet(ntmp, pctx)) {
        sprintf(err, "%s: couldn't get snapshot for iter %d", me, iter);
        biffAdd(PUSH, err); return 1;
      }
      fprintf(stderr, "%s: saving snapshot %s (meanVel = %g)\n",
              me, outS, pctx->meanVel);
      if (nrrdSave(outS, ntmp, NULL)) {
        sprintf(err, "%s: couldn't save snapshot for iter %d", me, iter);
        biffMove(PUSH, err, NRRD); return 1;
      }
      ntmp = nrrdNuke(ntmp);
    }
    iter++;
  } while (pctx->meanVel > pctx->minMeanVel
           && (0 == pctx->maxIter
               || iter < pctx->maxIter));
  pctx->time1 = airTime();
  pctx->time = pctx->time1 - pctx->time0;

  return 0;
}
