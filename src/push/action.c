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

void
_pushTenInv(pushContext *pctx, push_t *inv, push_t *ten) {
  push_t tmp=0.0, det;

  if (2 == pctx->dimIn) {
    tmp = ten[6];
    ten[6] = 1.0;
  }
  TEN_T_INV(inv, ten, det);
  if (2 == pctx->dimIn) {
    ten[6] = tmp;
    inv[6] = 0.0;
  }
  return;
}

/* returns -1 if position is outside simulation domain */
int
_pushBinFind(pushContext *pctx, push_t *pos) {
  push_t min, max;
  int be, xi, yi, zi, bi;

  if (pctx->singleBin) {
    bi = 0;
  } else {
    min = -1.0 - pctx->margin;
    max = 1.0 + pctx->margin;
    be = pctx->binsEdge;
    if (AIR_IN_CL(min, pos[0], max)
        && AIR_IN_CL(min, pos[1], max)
        && AIR_IN_CL(min, pos[2], max)) {
      AIR_INDEX(min, pos[0], max, be, xi);
      AIR_INDEX(min, pos[1], max, be, yi);
      if (2 == pctx->dimIn) {
        zi = 0;
      } else {
        AIR_INDEX(min, pos[2], max, be, zi);
      }
      bi = xi + be*(yi + be*zi);
    } else {
      bi = -1;
    }
  }

  return bi;
}

void
_pushBinPointAdd(pushContext *pctx, int bi, int pi) {
  int pii;

  pii = airArrayIncrLen(pctx->pidxArr[bi], 1);
  pctx->pidx[bi][pii] = pi;
  return;
}

void
_pushBinPointRemove(pushContext *pctx, int bi, int losePii) {
  airArray *pidxArr;
  int *pidx, npi, pii;

  pidx = pctx->pidx[bi];
  pidxArr = pctx->pidxArr[bi];

  /*
  fprintf(stderr, "______________ bi=%d, losePii=%d\n", bi, losePii);
  for (pii=0; pii<pidxArr->len; pii++) {
    fprintf(stderr, " %d", pidx[pii]);
  }
  fprintf(stderr, "\n");
  */
  
  npi = pidxArr->len;
  for (pii=losePii; pii<npi-1; pii++) {
    pidx[pii] = pidx[pii+1];
  }
  airArrayIncrLen(pidxArr, -1);

  /*
  for (pii=0; pii<pidxArr->len; pii++) {
    fprintf(stderr, " %d", pidx[pii]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "^^^^^^^^^^^^^^\n");
  */

  return;
}

/* does no checking for being out of bounds */
void
_pushBinPointsAllAdd(pushContext *pctx) {
  int bi, pi, np;
  push_t *attr, *pos;

  np = pctx->nPointAttr->axis[1].size;
  attr = (push_t*)pctx->nPointAttr->data;
  for (pi=0; pi<np; pi++) {
    pos = attr + PUSH_POS + PUSH_ATTR_LEN*pi;
    bi = _pushBinFind(pctx, pos);
    _pushBinPointAdd(pctx, bi, pi);
  }
  return;
}

int
_pushBinPointsRebin(pushContext *pctx) {
  char me[]="_pushBinPointsRebin", err[AIR_STRLEN_MED];
  airArray *pidxArr;
  int oldbi, newbi, pi, pii;
  push_t *attr, *pos;

  if (!pctx->singleBin) {
    attr = (push_t*)pctx->nPointAttr->data;
    for (oldbi=0; oldbi<pctx->numBin; oldbi++) {
      pidxArr = pctx->pidxArr[oldbi];
      for (pii=0; pii<pidxArr->len; /* nope! */) {
        pi = pctx->pidx[oldbi][pii];
        pos = attr + PUSH_POS + PUSH_ATTR_LEN*pi;
        newbi = _pushBinFind(pctx, pos);
        if (-1 == newbi) {
          sprintf(err, "%s: point %d pos (%g,%g,%g) outside domain", me,
                  pi, pos[0], pos[1], pos[2]);
          biffAdd(PUSH, err); return 1;
        }
        if (oldbi != newbi) {
          /* fprintf(stderr, "!%s: bingo 0: out of %d\n", me, oldbi); */
          _pushBinPointRemove(pctx, oldbi, pii);
          /* fprintf(stderr, "!%s: bingo 1: into %d\n", me, newbi); */
          _pushBinPointAdd(pctx, newbi, pi);
          /* fprintf(stderr, "!%s: bingo 2\n", me); */
          /* don't increment pii; the next point index is now at pii */
        } else {
          /* this point is already in the right bin, move to next */
          pii++;
        }
        /* fprintf(stderr, "!%s: bingo 3: %d %d %d\n", me,
           pii, pidxArr->len, pii<pidxArr->len); */
      }
      /* fprintf(stderr, "!%s: bingo 4\n", me); */
    }
    /*
      for (oldbi=0; oldbi<pctx->numBin; oldbi++) {
      pidxArr = pctx->pidxArr[oldbi];
      fprintf(stderr, "!%s: bin %d len = %d\n", me, oldbi, pidxArr->len);
      }
    */
  }

  return 0;
}

/*
** sets tenAns, cntAns
*/
int
_pushInputProcess(pushContext *pctx) {
  char me[]="_pushInputProcess", err[AIR_STRLEN_MED];
  Nrrd *seven[7], *two[2];
  Nrrd *ntmp;
  NrrdRange *nrange;
  airArray *mop;
  int E, ii, nn;
  gagePerVolume *tpvl, *mpvl;
  float *tdata, eval[3], maxDist;

  mop = airMopNew();

  /* ------------------------ fill pctx->nten, check mask range */
  ntmp = nrrdNew();
  airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
  E = AIR_FALSE;
  if (3 == pctx->nin->dim) {
    /* input is 2D array of 2D tensors */
    pctx->dimIn = 2;
    for (ii=0; ii<7; ii++) {
      if (ii < 2) {
        two[ii] = nrrdNew();
        airMopAdd(mop, two[ii], (airMopper)nrrdNuke, airMopAlways);
      }
      seven[ii] = nrrdNew();
      airMopAdd(mop, seven[ii], (airMopper)nrrdNuke, airMopAlways);
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
  if (!E) E |= nrrdSlice(pctx->nmask, pctx->nten, 0, 0);
  if (E) {
    sprintf(err, "%s: trouble creating 3D tensor input", me);
    biffMove(PUSH, err, NRRD); airMopError(mop); return 1;
  }
  nrange = nrrdRangeNewSet(pctx->nmask, nrrdBlind8BitRangeFalse);
  airMopAdd(mop, nrange, (airMopper)nrrdRangeNix, airMopAlways);
  if (AIR_ABS(1.0 - nrange->max) > 0.005) {
    sprintf(err, "%s: tensor mask max %g not close 1.0", me, nrange->max);
    biffAdd(PUSH, err); airMopError(mop); return 1;
  }

  /* ------------------------ set up gage and answer pointers */
  pctx->nten->axis[1].spacing = (AIR_EXISTS(pctx->nten->axis[1].spacing)
                                 ? pctx->nten->axis[1].spacing
                                 : 1.0);
  pctx->nten->axis[2].spacing = (AIR_EXISTS(pctx->nten->axis[2].spacing)
                                 ? pctx->nten->axis[2].spacing
                                 : 1.0);
  pctx->nten->axis[3].spacing = (AIR_EXISTS(pctx->nten->axis[3].spacing)
                                 ? pctx->nten->axis[3].spacing
                                 : 1.0);
  pctx->nmask->axis[0].spacing = pctx->nten->axis[1].spacing;
  pctx->nmask->axis[1].spacing = pctx->nten->axis[2].spacing;
  pctx->nmask->axis[2].spacing = pctx->nten->axis[3].spacing;
  /* HEY: we're only doing this because gage has a bug with
     cell-centered volume 1 sample thick- perhaps there should
     be a warning ... */
  pctx->nten->axis[1].center = pctx->nmask->axis[0].center = nrrdCenterNode;
  pctx->nten->axis[2].center = pctx->nmask->axis[1].center = nrrdCenterNode;
  pctx->nten->axis[3].center = pctx->nmask->axis[2].center = nrrdCenterNode;

  pctx->gctx = gageContextNew();
  E = AIR_FALSE;
  /* set up tensor probing */
  if (!E) E |= !(tpvl = gagePerVolumeNew(pctx->gctx,
                                         pctx->nten, tenGageKind));
  if (!E) E |= gagePerVolumeAttach(pctx->gctx, tpvl);
  if (!E) E |= gageKernelSet(pctx->gctx, gageKernel00,
                             pctx->ksp00->kernel, pctx->ksp00->parm);
  if (!E) E |= gageQueryItemOn(pctx->gctx, tpvl, tenGageTensor);
  /* set up mask gradient probing */
  if (!E) E |= !(mpvl = gagePerVolumeNew(pctx->gctx,
                                         pctx->nmask, gageKindScl));
  if (!E) E |= gagePerVolumeAttach(pctx->gctx, mpvl);
  if (!E) E |= gageKernelSet(pctx->gctx, gageKernel11,
                             pctx->ksp11->kernel, pctx->ksp11->parm);
  if (!E) E |= gageQueryItemOn(pctx->gctx, mpvl, gageSclGradVec);
  if (!E) E |= gageUpdate(pctx->gctx);
  if (E) {
    sprintf(err, "%s: trouble setting up gage", me);
    biffMove(PUSH, err, GAGE); airMopError(mop); return 1;
  }
  pctx->tenAns = gageAnswerPointer(pctx->gctx, tpvl, tenGageTensor);
  pctx->cntAns = gageAnswerPointer(pctx->gctx, mpvl, gageSclGradVec);
  gageParmSet(pctx->gctx, gageParmRequireAllSpacings, AIR_TRUE);

  /* ------------------------ find maxEval and set up binning */
  nn = nrrdElementNumber(pctx->nten)/7;
  tdata = (float*)pctx->nten->data;
  pctx->maxEval = 0;
  for (ii=0; ii<nn; ii++) {
    tenEigensolve_f(eval, NULL, tdata);
    if (tdata[0] > 0.5) {
      /* HEY: this limitation may be a bad idea */
      pctx->maxEval = AIR_MAX(pctx->maxEval, eval[0]);
    }
    tdata += 7;
  }
  maxDist = 2*pctx->maxEval*pctx->scale;
  if (pctx->singleBin) {
    pctx->binsEdge = 1;
    pctx->numBin = 1;
  } else {
    pctx->binsEdge = ceil((1 + 2*pctx->margin)/maxDist);
    fprintf(stderr, "!%s: maxEval = %g -> binsEdge = %d\n",
            me, pctx->maxEval, pctx->binsEdge);
    pctx->numBin = pctx->binsEdge*pctx->binsEdge*(2 == pctx->dimIn ? 
                                                  1 : pctx->binsEdge);
  }
  pctx->pidx = (int**)calloc(pctx->numBin, sizeof(int*));
  pctx->pidxArr = (airArray**)calloc(pctx->numBin, sizeof(airArray*));
  if (!( pctx->pidx && pctx->pidxArr )) {
    sprintf(err, "%s: trouble allocating pidx arrays", me);
    biffAdd(PUSH, err); airMopError(mop); return 1;
  }
  for (ii=0; ii<pctx->numBin; ii++) {
    pctx->pidx[ii] = NULL;
    pctx->pidxArr[ii] = airArrayNew((void **)&(pctx->pidx[ii]), NULL,
                                    sizeof(int), PUSH_PIDX_INCR);
  }
  /* we can do binning now- but it will be rebinned post-initialization */
  _pushBinPointsAllAdd(pctx);

  /* ------------------------ other stuff */
  ELL_3V_SCALE(pctx->minPos, -1, pctx->gctx->shape->volHalfLen);
  ELL_3V_SCALE(pctx->maxPos, 1, pctx->gctx->shape->volHalfLen);

  airMopOkay(mop);
  return 0;
}

int
pushOutputGet(Nrrd *nPosOut, Nrrd *nTenOut, pushContext *pctx) {
  char me[]="pushOutputGet", err[AIR_STRLEN_MED];
  int min[2], max[2], numPoint, E;
  Nrrd *ntmp, *four[4];
  airArray *mop;

  mop = airMopNew();
  numPoint = pctx->nPointAttr->axis[1].size;
  if (nPosOut) {
    min[0] = PUSH_POS;
    min[1] = 0;
    max[0] = PUSH_POS + (2 == pctx->dimIn ? 1 : 2);
    max[1] = numPoint - 1;
    if (nrrdCrop(nPosOut, pctx->nPointAttr, min, max)) {
      sprintf(err, "%s: couldn't crop to recover position output", me);
      biffMove(PUSH, err, NRRD); airMopError(mop); return 1;
    }
  }
  if (nTenOut) {
    ntmp = NULL;
    if (2 == pctx->dimIn) {
      ntmp = nrrdNew();
      airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
    }
    min[0] = PUSH_TEN;
    min[1] = 0;
    max[0] = PUSH_TEN + 6;
    max[1] = numPoint - 1;
    if (nrrdCrop((2 == pctx->dimIn
                  ? ntmp
                  : nTenOut), pctx->nPointAttr, min, max)) {
      sprintf(err, "%s: couldn't crop to recover tensor output", me);
      biffMove(PUSH, err, NRRD); airMopError(mop); return 1;
    }
    if (2 == pctx->dimIn) {
      four[0] = nrrdNew();
      airMopAdd(mop, four[0], (airMopper)nrrdNuke, airMopAlways);
      four[1] = nrrdNew();
      airMopAdd(mop, four[1], (airMopper)nrrdNuke, airMopAlways);
      four[2] = nrrdNew();
      airMopAdd(mop, four[2], (airMopper)nrrdNuke, airMopAlways);
      four[3] = nrrdNew();
      airMopAdd(mop, four[3], (airMopper)nrrdNuke, airMopAlways);
      /*    (0)         (0)
       *     1  2  3     1  2
       *        4  5        3
       *           6            */
      E = AIR_FALSE;
      if (!E) E |= nrrdSlice(four[0], ntmp, 0, 0);
      if (!E) E |= nrrdSlice(four[1], ntmp, 0, 1);
      if (!E) E |= nrrdSlice(four[2], ntmp, 0, 2);
      if (!E) E |= nrrdSlice(four[3], ntmp, 0, 4);
      if (!E) E |= nrrdJoin(nTenOut, (const Nrrd *const *)four,
                            4, 0, AIR_TRUE);
      if (E) {
        sprintf(err, "%s: trouble generating 2D tensor output", me);
        biffMove(PUSH, err, NRRD); airMopError(mop); return 1;
      }
    }
  }

  airMopOkay(mop);
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
  int numPoint, pi;
  push_t *attr, *pos, *vel, *ten, *cnt;

  /*
  {
    Nrrd *ntmp;
    double *data;
    int sx, sy, xi, yi;
    double p[3];
    
    sx = 30;
    sy = 30;
    ntmp = nrrdNew();
    nrrdMaybeAlloc(ntmp, nrrdTypeDouble, 3, 4, sx, sy);
    data = (double*)ntmp->data;
    p[2] = 0.0;
    for (yi=0; yi<sy; yi++) {
      p[1] = AIR_AFFINE(0, yi, sy-1, pctx->minPos[1], pctx->maxPos[1]);
      for (xi=0; xi<sx; xi++) {
        p[0] = AIR_AFFINE(0, xi, sx-1, pctx->minPos[0], pctx->maxPos[0]);
        _pushProbe(pctx, pctx->gctx, p[0], p[1], p[2]);
        data[0 + 4*(xi + sx*yi)] = pctx->tenAns[0];
        data[1 + 4*(xi + sx*yi)] = pctx->tenAns[1];
        data[2 + 4*(xi + sx*yi)] = pctx->tenAns[2];
        data[3 + 4*(xi + sx*yi)] = pctx->tenAns[4];
      }
    }
    nrrdSave("pray.nrrd", ntmp, NULL);
  }
  */
  
  numPoint = pctx->nPointAttr->axis[1].size;
  for (pi=0; pi<numPoint; pi++) {
    attr = (push_t *)(pctx->nPointAttr->data) + PUSH_ATTR_LEN*pi;
    pos = attr + PUSH_POS;
    vel = attr + PUSH_VEL;
    ten = attr + PUSH_TEN;
    cnt = attr + PUSH_CNT;
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
      TEN_T_COPY(ten, pctx->tenAns);
    } while (ten[0] < 0.5);
    ELL_3V_SET(vel, 0, 0, 0);
    ELL_3V_COPY(cnt, pctx->cntAns);
  }
  /* do rebinning, now that we have positions */
  _pushBinPointsRebin(pctx);

  /* HEY: this should be done by the user */
  pctx->process[0] = _pushRepel;
  pctx->process[1] = _pushUpdate;

  return;
}

/*
** for now, don't get clever and take point list indices instead
** of point information- may need to probe between arc vertices 
** to get intermediate tensors
*/
void
_pushForceCalc(pushTask *task, push_t force[3], push_t scale, 
               push_t *myPos, push_t *myTen,
               push_t *herPos, push_t *herTen) {
  char me[]="_pushForceIncr";
  push_t ten[7], inv[7];
  float U[3], nU[3], lenU, V[3], nV[3], lenV, ff;

  ELL_3V_SUB(U, myPos, herPos);
  ELL_3V_NORM(nU, U, lenU);
  TEN_T_SCALE_ADD2(ten, 0.5, myTen, 0.5, herTen);
  _pushTenInv(task->pctx, inv, ten);
  TEN_TV_MUL(V, inv, U);
  ELL_3V_NORM(nV, V, lenV);
  if (!lenU) {
    fprintf(stderr, "%s: myPos == herPos == (%g,%g,%g)\n", me,
            myPos[0], myPos[1], myPos[2]);
    return;
  }

  /* distorted world; scale=0.2875 for packing, 0.175 for quart */

  ff = AIR_MAX(0, 2*scale - lenV);
  ff = ff*ff;
  ff /= lenV;
  ELL_3V_SCALE(force, ff, U);


  /* true packing? scale=0.285 for packing, for 0.1735 for quart */
  /*
  ff = AIR_MAX(0, 2*scale - lenV);
  ff = ff*ff;
  ELL_3V_SCALE(force, ff, nV);
  */
  
  if (_pushVerbose) {
    fprintf(stderr, "myPos = %g %g %g\n", myPos[0], myPos[1], myPos[2]);
    fprintf(stderr, "U = %g %g %g\n", U[0], U[1], U[2]);
    fprintf(stderr, "myTen = %g %g %g %g %g %g %g\n", myTen[0],
            myTen[1], myTen[2], myTen[3], myTen[4], myTen[5], myTen[6]);
  }

  return;
}

int
_pushBinNeighborhoodFind(pushContext *pctx, int *nei, int bin, int dimIn) {
  int numNei, be, tmp, xx, yy, zz, xi, yi, zi,
    xmin, xmax, ymin, ymax, zmin, zmax;

  numNei = 0;
  if (pctx->singleBin) {
    nei[numNei++] = 0;
  } else {
    be = pctx->binsEdge;
    tmp = bin;
    xi = tmp % be;
    tmp = (tmp-xi)/be;
    yi = tmp % be;
    xmin = AIR_MAX(0, xi-1);
    xmax = AIR_MIN(xi+1, be-1);
    ymin = AIR_MAX(0, yi-1);
    ymax = AIR_MIN(yi+1, be-1);
    if (2 == pctx->dimIn) {
      zmin = zmax = 0;
    } else {
      zi = (tmp-yi)/be;
      zmin = AIR_MAX(0, zi-1);
      zmax = AIR_MIN(zi+1, be-1);
    }
    for (zz=zmin; zz<=zmax; zz++) {
      for (yy=ymin; yy<=ymax; yy++) {
        for (xx=xmin; xx<=xmax; xx++) {
          nei[numNei++] = xx + be*(yy + be*zz);
        }
      }
    }
  }
  
  return numNei;
}

void
_pushRepel(pushTask *task, int bin, double parm[PUSH_STAGE_PARM_MAX]) {
  push_t *attr, *velAcc, *attrI, *attrJ, force[3], sumForce[3], dist, dir[3];
  int *neiPidx, *myPidx, nei[27], ni, numNei, jj, ii, pidxJ, pidxI,
    myPidxArrLen, neiPidxArrLen;

  attr = (push_t *)task->pctx->nPointAttr->data;
  velAcc = (push_t *)task->pctx->nVelAcc->data;
  myPidx = task->pctx->pidx[bin];
  myPidxArrLen = task->pctx->pidxArr[bin]->len;
  numNei = _pushBinNeighborhoodFind(task->pctx, nei, bin, task->pctx->dimIn);
  for (ii=0; ii<myPidxArrLen; ii++) {
    pidxI = myPidx[ii];
    attrI = attr + PUSH_ATTR_LEN*pidxI;

    /* initialize force accumulator */
    ELL_3V_SET(sumForce, 0, 0, 0);

    /* go through pairs */
    for (ni=0; ni<numNei; ni++) {
      neiPidx = task->pctx->pidx[nei[ni]];
      neiPidxArrLen = task->pctx->pidxArr[nei[ni]]->len;
      for (jj=0; jj<neiPidxArrLen; jj++) {
        pidxJ = neiPidx[jj];
        if (pidxI == pidxJ) {
          continue;
        }
        attrJ = attr + PUSH_ATTR_LEN*pidxJ;
        _pushForceCalc(task, force, task->pctx->scale,
                       attrI + PUSH_POS, attrI + PUSH_TEN, 
                       attrJ + PUSH_POS, attrJ + PUSH_TEN);
        /*
        if (ELL_3V_LEN(force)) {
          fprintf(stderr, "!%s: %d <---> %d : %g\n",
                  "_pushRepel", pidxI, pidxJ, ELL_3V_LEN(force));
        }
        */
        ELL_3V_INCR(sumForce, force);
      }
    }

    /* drag */
    ELL_3V_SCALE_INCR(sumForce, -task->pctx->drag, attrI + PUSH_VEL);

    /* nudging towards image center */
    ELL_3V_NORM(dir, attrI + PUSH_POS, dist);
    if (dist) {
      ELL_3V_SCALE_INCR(sumForce, -task->pctx->nudge*dist, dir);
    }
    /*
    if (ELL_3V_LEN(sumForce)) {
      fprintf(stderr, "!%s: %d(%d) : (%g,%g,%g)\n",
              "_pushRepel", pidxI, bin, 
              sumForce[0], sumForce[1], sumForce[2]);
    }
    */

    /* copy results to tmp world */
    ELL_3V_COPY(velAcc + 3*(0 + 2*pidxI), attrI + PUSH_VEL);
    ELL_3V_SCALE(velAcc + 3*(1 + 2*pidxI), 1.0/task->pctx->mass, sumForce);
  }
  return;
}

void
_pushUpdate(pushTask *task, int bin,
            double parm[PUSH_STAGE_PARM_MAX]) {
  push_t *attrData, *attr, *velAccData, *oldVel, *oldAcc;
  int *pidx, pidxLen, pidxI, pii;
  double step;

  step = task->pctx->step;
  velAccData = (push_t *)task->pctx->nVelAcc->data;
  attrData = (push_t *)task->pctx->nPointAttr->data;
  pidx = task->pctx->pidx[bin];
  pidxLen = task->pctx->pidxArr[bin]->len;
  for (pii=0; pii<pidxLen; pii++) {
    pidxI = pidx[pii];
    attr =  attrData + PUSH_ATTR_LEN*pidxI;
    oldVel = velAccData + 3*(0 + 2*pidxI);
    oldAcc = velAccData + 3*(1 + 2*pidxI);
    ELL_3V_SCALE_INCR(attr + PUSH_POS, step, oldVel);
    ELL_3V_SCALE_INCR(attr + PUSH_VEL, step, oldAcc);
    task->sumVel += ELL_3V_LEN(attr + PUSH_VEL);
    _pushProbe(task->pctx, task->gctx, 
               (attr + PUSH_POS)[0], 
               (attr + PUSH_POS)[1], 
               (attr + PUSH_POS)[2]);
    TEN_T_COPY(attr + PUSH_TEN + PUSH_ATTR_LEN*pidxI, task->tenAns);
    ELL_3V_COPY(attr + PUSH_CNT + PUSH_ATTR_LEN*pidxI, task->cntAns);
  }
  return;
}

int
pushRun(pushContext *pctx) {
  char me[]="pushRun", err[AIR_STRLEN_MED],
    poutS[AIR_STRLEN_MED], toutS[AIR_STRLEN_MED];
  Nrrd *npos, *nten;
  double vel[2], meanVel;

  pctx->iter = 0;
  pctx->time0 = airTime();
  vel[0] = AIR_NAN;
  vel[1] = AIR_NAN;
  do {
    if (pushIterate(pctx)) {
      sprintf(err, "%s: trouble on iter %d", me, pctx->iter);
      biffAdd(PUSH, err); return 1;
    }
    /* this goofiness is because it seems like my stupid Euler 
       integration can lead to real motion only happening on
       every other iteration ... */
    if (0 == pctx->iter) {
      vel[0] = pctx->meanVel;
      meanVel = pctx->meanVel;
    } else if (1 == pctx->iter) {
      vel[1] = pctx->meanVel;
      meanVel = (vel[0] + vel[1])/2;
    } else {
      vel[0] = vel[1];
      vel[1] = pctx->meanVel;
      meanVel = (vel[0] + vel[1])/2;
    }
    if (pctx->snap && !(pctx->iter % pctx->snap)) {
      nten = nrrdNew();
      npos = nrrdNew();
      sprintf(poutS, "snap-%06d-pos.nrrd", pctx->iter);
      sprintf(toutS, "snap-%06d-ten.nrrd", pctx->iter);
      if (pushOutputGet(npos, nten, pctx)) {
        sprintf(err, "%s: couldn't get snapshot for iter %d", me, pctx->iter);
        biffAdd(PUSH, err); return 1;
      }
      fprintf(stderr, "%s: %s (meanVel = %g)\n", me, poutS, meanVel);
      if (nrrdSave(poutS, npos, NULL)
          || nrrdSave(toutS, nten, NULL)) {
        sprintf(err, "%s: couldn't save snapshot for iter %d", me, pctx->iter);
        biffMove(PUSH, err, NRRD); return 1;
      }
      nten = nrrdNuke(nten);
      npos = nrrdNuke(npos);
    }
    pctx->iter++;
  } while ( (pctx->iter < pctx->minIter)
            || (meanVel > pctx->minMeanVel
                && (0 == pctx->maxIter
                    || pctx->iter < pctx->maxIter)) );
  pctx->time1 = airTime();
  pctx->time = pctx->time1 - pctx->time0;

  return 0;
}
