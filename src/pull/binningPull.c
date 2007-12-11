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
** because the pullContext keeps an array of bins (not pointers to them)
** we have Init and Done functions (not New and Nix)
*/
void
pullBinInit(pullBin *bin, unsigned int incr) {

  bin->pointNum = 0;
  bin->point = NULL;
  bin->pointArr = airArrayNew((void**)&(bin->point), &(bin->pointNum),
                              sizeof(pullPoint *), incr);
  bin->neigh = NULL;
  return;
}

/*
** bins own the points they contain- so this frees them
*/
void
pullBinDone(pullBin *bin) {
  unsigned int idx;

  for (idx=0; idx<bin->pointNum; idx++) {
    bin->point[idx] = pullPointNix(bin->point[idx]);
  }
  bin->pointArr = airArrayNuke(bin->pointArr);
  bin->neigh = (pullBin **)airFree(bin->neigh);
  return;
}

/* 
** bins on boundary now extend to infinity; so the only time this 
** returns NULL (indicating error) is for non-existant positions
*/
pullBin *
_pullBinLocate(pullContext *pctx, double *posWorld) {
  char me[]="_pullBinLocate", err[BIFF_STRLEN];
  unsigned int axi, eidx[3], binIdx;

  if (!ELL_3V_EXISTS(posWorld)) {
    sprintf(err, "%s: non-existant position (%g,%g,%g)", me,
            posWorld[0], posWorld[1], posWorld[2]);
    biffAdd(PULL, err); return NULL;
  }

  if (pctx->binSingle) {
    binIdx = 0;
  } else {
    for (axi=0; axi<3; axi++) {
      eidx[axi] = airIndexClamp(pctx->bboxMin[axi],
                                posWorld[axi],
                                pctx->bboxMax[axi],
                                pctx->binsEdge[axi]);
    }
    binIdx = (eidx[0]
              + pctx->binsEdge[0]*(eidx[1] 
                                   + pctx->binsEdge[1]*eidx[2]));
  }
  /*
  fprintf(stderr, "!%s: bin(%g,%g,%g) = %u\n", me, 
          _posWorld[0], _posWorld[1], _posWorld[2], binIdx);
  */

  return pctx->bin + binIdx;
}

/*
** this makes the bin the owner of the point
*/
void
_pullBinPointAdd(pullContext *pctx, pullBin *bin, pullPoint *point) {
  int pntI;

  AIR_UNUSED(pctx);
  pntI = airArrayLenIncr(bin->pointArr, 1);
  bin->point[pntI] = point;
  return;
}

/*
** the bin loses track of the point, caller responsible for ownership
*/
void
_pullBinPointRemove(pullContext *pctx, pullBin *bin, int loseIdx) {

  AIR_UNUSED(pctx);
  bin->point[loseIdx] = bin->point[bin->pointNum-1];
  airArrayLenIncr(bin->pointArr, -1);
  return;
}

void
_pullBinNeighborSet(pullBin *bin, pullBin **nei, unsigned int num) {
  unsigned int neiI;

  bin->neigh = (pullBin **)airFree(bin->neigh);
  bin->neigh = (pullBin **)calloc(1+num, sizeof(pullBin *));
  for (neiI=0; neiI<num; neiI++) {
    bin->neigh[neiI] = nei[neiI];
  }
  bin->neigh[neiI] = NULL;
  return;
}

void
pullBinAllNeighborSet(pullContext *pctx) {
  /* char me[]="pullBinAllNeighborSet"; */
  pullBin *nei[3*3*3];
  unsigned int neiNum, xi, yi, zi, xx, yy, zz, xmax, ymax, zmax, binIdx;
  int xmin, ymin, zmin;

  if (pctx->binSingle) {
    neiNum = 0;
    nei[neiNum++] = pctx->bin + 0;
    _pullBinNeighborSet(pctx->bin + 0, nei, neiNum);
  } else {
    for (zi=0; zi<pctx->binsEdge[2]; zi++) {
      zmin = AIR_MAX(0, (int)zi-1);
      zmax = AIR_MIN(zi+1, pctx->binsEdge[2]-1);
      for (yi=0; yi<pctx->binsEdge[1]; yi++) {
        ymin = AIR_MAX(0, (int)yi-1);
        ymax = AIR_MIN(yi+1, pctx->binsEdge[1]-1);
        for (xi=0; xi<pctx->binsEdge[0]; xi++) {
          xmin = AIR_MAX(0, (int)xi-1);
          xmax = AIR_MIN(xi+1, pctx->binsEdge[0]-1);
          neiNum = 0;
          for (zz=zmin; zz<=zmax; zz++) {
            for (yy=ymin; yy<=ymax; yy++) {
              for (xx=xmin; xx<=xmax; xx++) {
                binIdx = xx + pctx->binsEdge[0]*(yy + pctx->binsEdge[1]*zz);
                /*
                fprintf(stderr, "!%s: nei[%u](%u,%u,%u) = %u\n", me, 
                        neiNum, xi, yi, zi, binIdx);
                */
                nei[neiNum++] = pctx->bin + binIdx;
              }
            }
          }
          _pullBinNeighborSet(pctx->bin + xi + pctx->binsEdge[0]
                              *(yi + pctx->binsEdge[1]*zi), nei, neiNum);
        }
      }
    }
  }
  return;
}

int
pullBinPointAdd(pullContext *pctx, pullPoint *point) {
  char me[]="pullBinPointAdd", err[BIFF_STRLEN];
  pullBin *bin;
  
  if (!( bin = _pullBinLocate(pctx, point->pos) )) {
    sprintf(err, "%s: can't locate point %p %u",
            me, AIR_CAST(void*, point), point->idtag);
    biffAdd(PULL, err); return 1;
  }
  _pullBinPointAdd(pctx, bin, point);
  return 0;
}

/*
** This function is only called by the master thread, this 
** does *not* have to be thread-safe in any way
*/
int
pullRebin(pullContext *pctx) {
  char me[]="pullRebin", err[BIFF_STRLEN];
  unsigned int oldBinIdx, pointIdx;
  pullBin *oldBin, *newBin;
  pullPoint *point;

  if (!pctx->binSingle) {
    for (oldBinIdx=0; oldBinIdx<pctx->binNum; oldBinIdx++) {
      oldBin = pctx->bin + oldBinIdx;
      
      for (pointIdx=0; pointIdx<oldBin->pointNum; /* nope! */) {
        point = oldBin->point[pointIdx];
        newBin = _pullBinLocate(pctx, point->pos);
        if (!newBin) {
          sprintf(err, "%s: can't locate point %p %u",
                  me, AIR_CAST(void*, point), point->idtag);
          biffAdd(PULL, err); return 1;
        }
        if (oldBin != newBin) {
          _pullBinPointRemove(pctx, oldBin, pointIdx);
          _pullBinPointAdd(pctx, newBin, point);
        } else {
          /* its in the right bin, move on */
          pointIdx++;
        }
      } /* for pointIdx */
    } /* for oldBinIdx */
  }

  return 0;
}

int
_pullBinSetup(pullContext *pctx) {
  char me[]="_pullBinSetup", err[BIFF_STRLEN];
  unsigned ii;
  double volEdge[3];

  gageShapeBoundingBox(pctx->bboxMin, pctx->bboxMax,
                       pctx->vol[0]->gctx->shape);
  for (ii=1; ii<pctx->volNum; ii++) {
    double min[3], max[3];
    gageShapeBoundingBox(min, max, pctx->vol[ii]->gctx->shape);
    ELL_3V_MIN(pctx->bboxMin, pctx->bboxMin, min);
    ELL_3V_MIN(pctx->bboxMax, pctx->bboxMax, max);
  }
  fprintf(stderr, "!%s: bbox min (%g,%g,%g) max (%g,%g,%g)\n", me,
          pctx->bboxMin[0], pctx->bboxMin[1], pctx->bboxMin[2],
          pctx->bboxMax[0], pctx->bboxMax[1], pctx->bboxMax[2]);
  
  pctx->maxDist = (pctx->interScl ? pctx->interScl : 0.2);
  fprintf(stderr, "!%s: interScl = %g --> maxDist = %g\n", me, 
          pctx->interScl, pctx->maxDist);

  if (pctx->binSingle) {
    pctx->binsEdge[0] = 1;
    pctx->binsEdge[1] = 1;
    pctx->binsEdge[2] = 1;
    pctx->binNum = 1;
  } else {
    volEdge[0] = pctx->bboxMax[0] - pctx->bboxMin[0];
    volEdge[1] = pctx->bboxMax[1] - pctx->bboxMin[1];
    volEdge[2] = pctx->bboxMax[2] - pctx->bboxMin[2];
    fprintf(stderr, "!%s: volEdge = %g %g %g\n", me,
            volEdge[0], volEdge[1], volEdge[2]);
    pctx->binsEdge[0] = AIR_CAST(unsigned int,
                                 floor(volEdge[0]/pctx->maxDist));
    pctx->binsEdge[0] = pctx->binsEdge[0] ? pctx->binsEdge[0] : 1;
    pctx->binsEdge[1] = AIR_CAST(unsigned int,
                                 floor(volEdge[1]/pctx->maxDist));
    pctx->binsEdge[1] = pctx->binsEdge[1] ? pctx->binsEdge[1] : 1;
    pctx->binsEdge[2] = AIR_CAST(unsigned int,
                                 floor(volEdge[2]/pctx->maxDist));
    pctx->binsEdge[2] = pctx->binsEdge[2] ? pctx->binsEdge[2] : 1;
    fprintf(stderr, "!%s: binsEdge=(%u,%u,%u)\n", me,
            pctx->binsEdge[0], pctx->binsEdge[1], pctx->binsEdge[2]);
    pctx->binNum = pctx->binsEdge[0]*pctx->binsEdge[1]*pctx->binsEdge[2];
  }
  pctx->bin = (pullBin *)calloc(pctx->binNum, sizeof(pullBin));
  if (!( pctx->bin )) {
    sprintf(err, "%s: trouble allocating bin arrays", me);
    biffAdd(PULL, err); return 1;
  }
  for (ii=0; ii<pctx->binNum; ii++) {
    pullBinInit(pctx->bin + ii, pctx->binIncr);
  }
  pullBinAllNeighborSet(pctx);
  return 0;
}

void
_pullBinFinish(pullContext *pctx) {
  unsigned int ii;

  for (ii=0; ii<pctx->binNum; ii++) {
    pullBinDone(pctx->bin + ii);
  }
  pctx->bin = (pullBin *)airFree(pctx->bin);
  ELL_3V_SET(pctx->binsEdge, 0, 0, 0);
  pctx->binNum = 0;
}

