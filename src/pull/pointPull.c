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
  /* HEY: old code used DBL_MAX:
     "any finite quantity will be less than this" ... why??? */
  pnt->energy = 0.0;
  ELL_4V_SET(pnt->force, AIR_NAN, AIR_NAN, AIR_NAN, AIR_NAN);
  for (ii=0; ii<pctx->infoTotalLen; ii++) {
    pnt->info[ii] = AIR_NAN;
  }
  return pnt;
}

pullPoint *
pullPointNix(pullPoint *pnt) {

  airFree(pnt);
  return NULL;
}

unsigned int
_pullPointTotal(pullContext *pctx) {
  unsigned int binIdx, pointNum;
  pullBin *bin;

  pointNum = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    pointNum += bin->pointNum;
  }
  return pointNum;
}

int
_pullProbe(pullTask *task, pullPoint *point) {
  char me[]="_pullProbe", err[BIFF_STRLEN];
  unsigned int ii, gret=0;
  
  for (ii=0; ii<task->pctx->volNum; ii++) {
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
     since it means that we always have to loop through all indices */
  for (ii=0; ii<=PULL_INFO_MAX; ii++) {
    if (task->ans[ii]) {
      _pullInfoAnswerCopy[_pullInfoAnswerLen[ii]](point->info 
                                                  + task->infoOffset[ii],
                                                  task->ans[ii]);
    }
  }
  return 0;
}

/*
** _pullPointSetup sets:
**** pctx->pointNum (in case pctx->npos)
**
** This is only called by the master thread
** 
** this should set stuff to be like after an update stage and
** just before the rebinning
*/
int
_pullPointSetup(pullContext *pctx) {
  char me[]="_pullPointSetup", err[BIFF_STRLEN];
  unsigned int pointIdx;
  pullPoint *point;
  double *posData;
  int reject;

  pctx->pointNum = (pctx->npos
                    ? pctx->npos->axis[1].size
                    : pctx->pointNum);
  posData = (pctx->npos
             ? AIR_CAST(double *, pctx->npos->data)
             : NULL);
  fprintf(stderr, "!%s: initilizing/seeding ... \n", me);
  for (pointIdx=0; pointIdx<pctx->pointNum; pointIdx++) {
    /*
    fprintf(stderr, "!%s: pointIdx = %u/%u\n", me, pointIdx, pctx->pointNum);
    */
    point = pullPointNew(pctx);
    if (pctx->npos) {
      ELL_4V_COPY(point->pos, posData + 4*pointIdx);
      if (_pullProbe(pctx->task[0], point)) {
        sprintf(err, "%s: probing pointIdx %u of npos", me, pointIdx);
        biffAdd(PULL, err); return 1;
      }
    } else {
      reject = AIR_FALSE;
      do {
        ELL_3V_SET(point->pos,
                   AIR_AFFINE(0.0, airDrandMT(), 1.0,
                              pctx->bboxMin[0], pctx->bboxMin[0]),
                   AIR_AFFINE(0.0, airDrandMT(), 1.0,
                              pctx->bboxMin[1], pctx->bboxMin[1]),
                   AIR_AFFINE(0.0, airDrandMT(), 1.0,
                              pctx->bboxMin[2], pctx->bboxMin[2]));
        if (pctx->haveScale) {
          point->pos[3] = AIR_AFFINE(0.0, airDrandMT(), 1.0,
                                     pctx->bboxMin[3], pctx->bboxMin[3]);
        } else {
          point->pos[3] = AIR_NAN;
        }
        if (_pullProbe(pctx->task[0], point)) {
          sprintf(err, "%s: probing pointIdx %u of world", me, pointIdx);
          biffAdd(PULL, err); return 1;
        }
      } while (reject);
    }
    if (pullBinPointAdd(pctx, point)) {
      sprintf(err, "%s: trouble binning point %u", me, point->idtag);
      biffAdd(PULL, err); return 1;
    }
  }
  fprintf(stderr, "!%s: ... seeding DONE\n", me);
  return 0;
}

