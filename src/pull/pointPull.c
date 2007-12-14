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

pullPoint *
pullPointNew(pullContext *pctx) {
  char me[]="pullPointNew", err[BIFF_STRLEN];
  pullPoint *pnt;
  unsigned int ii;
  
  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PULL, err); return NULL;
  }
  if (!pctx->infoTotalLen) {
    sprintf(err, "%s: can't allocate points w/out infoTotalLen set\n", me);
    biffAdd(PULL, err); return NULL;
  }
  /* Allocate the pullPoint so that it has pctx->infoTotalLen doubles.
     The pullPoint declaration has info[1], hence the "- 1" below */
  pnt = AIR_CAST(pullPoint *,
                 calloc(1, sizeof(pullPoint)
                        + sizeof(double)*(pctx->infoTotalLen - 1)));
  if (!pnt) {
    sprintf(err, "%s: couldn't allocate point (info len %u)\n", me, 
            pctx->infoTotalLen - 1);
    biffAdd(PULL, err); return NULL;
  }

  pnt->idtag = pctx->idtagNext++;
  pnt->neighArr = airArrayNew((void**)&(pnt->neigh), &(pnt->neighNum),
                              sizeof(pullPoint *), PULL_POINT_NEIGH_INCR);
  ELL_4V_SET(pnt->pos, AIR_NAN, AIR_NAN, AIR_NAN, AIR_NAN);
  /* the first thing that pullBinProcess does (per point) is:
     myPoint->energyLast = myPoint->energy;
     so we pre-load energy with DBL_MAX, which is higher than whatever
     finite energy might be computed, which ensures that none of the
     adaptive time step stuff kicks in */
  pnt->energy = DBL_MAX;
  pnt->energyLast = AIR_NAN;
  ELL_4V_SET(pnt->move, AIR_NAN, AIR_NAN, AIR_NAN, AIR_NAN);
  pnt->stepInter = pctx->stepInitial;
  pnt->stepConstr = pctx->stepInitial;
  for (ii=0; ii<pctx->infoTotalLen; ii++) {
    pnt->info[ii] = AIR_NAN;
  }
  return pnt;
}

pullPoint *
pullPointNix(pullPoint *pnt) {

  pnt->neighArr = airArrayNix(pnt->neighArr);
  airFree(pnt);
  return NULL;
}

unsigned int
_pullPointNumber(const pullContext *pctx) {
  unsigned int binIdx, pointNum;
  const pullBin *bin;

  pointNum = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    pointNum += bin->pointNum;
  }
  return pointNum;
}

double
_pullEnergyAverage(const pullContext *pctx) {
  unsigned int binIdx, pointIdx, pointNum;
  const pullBin *bin;
  const pullPoint *point;
  double sum, avg;

  sum = 0;
  pointNum = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    pointNum += bin->pointNum;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      sum += point->energy;
    }
  }
  avg = (!pointNum ? AIR_NAN : sum/pointNum);
  return avg;
}

double
_pullStepInterAverage(const pullContext *pctx) {
  unsigned int binIdx, pointIdx, pointNum;
  const pullBin *bin;
  const pullPoint *point;
  double sum, avg;

  sum = 0;
  pointNum = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    pointNum += bin->pointNum;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      sum += point->stepInter;
    }
  }
  avg = (!pointNum ? AIR_NAN : sum/pointNum);
  return avg;
}
/* ^^^  vvv HEY HEY HEY: COPY + PASTE COPY + PASTE COPY + PASTE */
double
_pullStepConstrAverage(const pullContext *pctx) {
  unsigned int binIdx, pointIdx, pointNum;
  const pullBin *bin;
  const pullPoint *point;
  double sum, avg;

  sum = 0;
  pointNum = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    pointNum += bin->pointNum;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      sum += point->stepConstr;
    }
  }
  avg = (!pointNum ? AIR_NAN : sum/pointNum);
  return avg;
}

void
_pullPointStepScale(const pullContext *pctx, double scale) {
  unsigned int binIdx, pointIdx;
  const pullBin *bin;
  pullPoint *point;

  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      point->stepInter *= scale;
      point->stepConstr *= scale;
    }
  }
  return;
}

int
_pullProbe(pullTask *task, pullPoint *point) {
  char me[]="_pullProbe", err[BIFF_STRLEN];
  unsigned int ii, gret=0;
  
  for (ii=0; ii<task->pctx->volNum; ii++) {
    if (task->pctx->iter && task->vol[ii]->seedOnly) {
      /* its after the 1st iteration (#0), and this vol is only for seeding */
      continue;
    }
    if (task->vol[ii]->ninSingle) {
      gret = gageProbeSpace(task->vol[ii]->gctx,
                            point->pos[0], point->pos[1], point->pos[2],
                            AIR_FALSE, AIR_TRUE);
    } else {
      gret = gageStackProbeSpace(task->vol[ii]->gctx,
                                 point->pos[0], point->pos[1],
                                 point->pos[2], point->pos[3],
                                 AIR_FALSE, AIR_TRUE);
    }
    if (gret) {
      break;
    }
  }
  if (gret) {
    sprintf(err, "%s: probe failed on vol %u/%u: (%d) %s\n", me,
            ii, task->pctx->volNum,
            task->vol[ii]->gctx->errNum, task->vol[ii]->gctx->errStr);
    biffAdd(PULL, err); return 1;
  }

  /* maybe is a little stupid to have the infos indexed this way, 
     since it means that we always have to loop through all indices,
     but at least the compiler can unroll it... */
  for (ii=0; ii<=PULL_INFO_MAX; ii++) {
    unsigned int alen, aidx;
    if (task->ans[ii]) {
      alen = _pullInfoAnswerLen[ii];
      aidx = task->pctx->infoIdx[ii];
      _pullInfoAnswerCopy[alen](point->info + aidx, task->ans[ii]);
    }
  }
  return 0;
}

/*
** _pullPointSetup sets:
**** pctx->pointNumInitial (in case pctx->npos)
**
** This is only called by the master thread
** 
** this should set stuff to be like after an update stage and
** just before the rebinning
*/
int
_pullPointSetup(pullContext *pctx) {
  char me[]="_pullPointSetup", err[BIFF_STRLEN], doneStr[AIR_STRLEN_SMALL];
  unsigned int pointIdx, tick;
  pullPoint *point;
  double *posData;
  airRandMTState *rng;
  int reject;

  pctx->pointNumInitial = (pctx->npos
                           ? pctx->npos->axis[1].size
                           : pctx->pointNumInitial);
  posData = (pctx->npos
             ? AIR_CAST(double *, pctx->npos->data)
             : NULL);
  fprintf(stderr, "!%s: initilizing/seeding ...       ", me);
  fflush(stderr);
  rng = pctx->task[0]->rng;
  tick = pctx->pointNumInitial/1000;
  for (pointIdx=0; pointIdx<pctx->pointNumInitial; pointIdx++) {
    if (tick < 100 || 0 == pointIdx % tick) {
      fprintf(stderr, "%s", airDoneStr(0, pointIdx, pctx->pointNumInitial,
                                       doneStr));
      fflush(stderr);
    }
    if (pctx->verbose > 5) {
      fprintf(stderr, "%s: setting up point = %u/%u\n", me,
              pointIdx, pctx->pointNumInitial);
    }
    point = pullPointNew(pctx);
    if (pctx->npos) {
      ELL_4V_COPY(point->pos, posData + 4*pointIdx);
      /* even though we are dictating the point locations, we still have
         to do the initial probe */
      if (_pullProbe(pctx->task[0], point)) {
        sprintf(err, "%s: probing pointIdx %u of npos", me, pointIdx);
        biffAdd(PULL, err); return 1;
      }
    } else {
      do {
        ELL_3V_SET(point->pos,
                   AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0,
                              pctx->bboxMin[0], pctx->bboxMax[0]),
                   AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0,
                              pctx->bboxMin[1], pctx->bboxMax[1]),
                   AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0,
                              pctx->bboxMin[2], pctx->bboxMax[2]));
        if (pctx->haveScale) {
          point->pos[3] = AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0,
                                     pctx->bboxMin[3], pctx->bboxMax[3]);
        } else {
          point->pos[3] = AIR_NAN;
        }
        if (_pullProbe(pctx->task[0], point)) {
          sprintf(err, "%s: probing pointIdx %u of world", me, pointIdx);
          biffAdd(PULL, err); return 1;
        }
        reject = AIR_FALSE;
        if (pctx->ispec[pullInfoSeedThresh]) {
          pullInfoSpec *ispec;
          double val;
          ispec = pctx->ispec[pullInfoSeedThresh];
          val = point->info[pctx->infoIdx[pullInfoSeedThresh]];
          val = (val - ispec->zero)*ispec->scale;
          reject |= val < 0;
        }
      } while (reject);
    }
    if (pullBinPointAdd(pctx, point)) {
      sprintf(err, "%s: trouble binning point %u", me, point->idtag);
      biffAdd(PULL, err); return 1;
    }
  }
  fprintf(stderr, "%s\n", airDoneStr(0, pointIdx, pctx->pointNumInitial,
                                     doneStr));
  return 0;
}

int 
pullHeightDerivativesTest(pullContext *pctx,
                          double xx, double yy, double zz, double ss, 
                          double eps) {
  char me[]="pullHeightDerivativesTest", err[BIFF_STRLEN];
  pullPoint *point;
  airArray *mop;
  unsigned int ai;
  double xyz[3], *val, *grad, *hess, agrad[3], dvec[3];

  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PULL, err); return 1;
  }
  if (!( pctx->ispec[pullInfoHeight]
         && pctx->ispec[pullInfoHeightGradient]
         && pctx->ispec[pullInfoHeightHessian] )) {
    sprintf(err, "%s: don't have %s, %s, and %s ispecs set", me,
            airEnumStr(pullInfo, pullInfoHeight),
            airEnumStr(pullInfo, pullInfoHeightGradient),
            airEnumStr(pullInfo, pullInfoHeightHessian));
    biffAdd(PULL, err); return 1;
  }

  fprintf(stderr, "%s: hello\n", me);
  mop = airMopNew();
  point = pullPointNew(pctx);
  airMopAdd(mop, point, (airMopper)pullPointNix, airMopAlways);
  val = point->info + pctx->infoIdx[pullInfoHeight];
  grad = point->info + pctx->infoIdx[pullInfoHeightGradient];
  hess = point->info + pctx->infoIdx[pullInfoHeightHessian];
  
  ELL_4V_SET(point->pos, xx, yy, zz, ss);
  if (_pullProbe(pctx->task[0], point)) {
    sprintf(err, "%s: initial probe", me);
    biffAdd(PULL, err); return 1;
  }
  fprintf(stderr, "%s: val(%g,%g,%g) = %g\n", me, xx, yy, zz, val[0]);

  point->pos[3] = ss;
  ELL_3V_SET(xyz, xx, yy, zz);
  for (ai=0; ai<3; ai++) {
    ELL_3V_COPY(point->pos, xyz); point->pos[ai] += eps;
    _pullProbe(pctx->task[0], point);
    agrad[ai] = val[0];
    ELL_3V_COPY(point->pos, xyz); point->pos[ai] -= eps;
    _pullProbe(pctx->task[0], point);
    agrad[ai] = (agrad[ai] - val[0])/(2*eps);
  }
  ELL_3V_COPY(point->pos, xyz);
  _pullProbe(pctx->task[0], point);
  ELL_3V_SUB(dvec, grad, agrad);
  fprintf(stderr, "%s: grad = (%g,%g,%g), agrad = (%g,%g,%g); err = %g\n", me,
          grad[0], grad[1], grad[2],
          agrad[0], agrad[1], agrad[2],
          ELL_3V_LEN(dvec));

  airMopOkay(mop);
  return 0;
}
