/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
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

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "pull.h"
#include "privatePull.h"

/*
** HEY: this messes with the points' idtag, and pctx->idtagNext, 
** even though it really shouldn't have to 
*/
int
pullCCFind(pullContext *pctx) {
  char me[]="pullCCFind", err[BIFF_STRLEN];
  airArray *mop, *eqvArr;
  unsigned int passIdx, binIdx, pointIdx, neighIdx, eqvNum,
    pointNum, *idmap;
  pullBin *bin;
  pullPoint *point, *her;
  
  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PULL, err); return 1;
  }
  if (_pullIterate(pctx, pullProcessModeNeighLearn)) {
    sprintf(err, "%s: trouble with %s for CC", me,
            airEnumStr(pullProcessMode, pullProcessModeNeighLearn));
    biffAdd(PULL, err); return 1;
  }

  mop = airMopNew();
  pointNum = pullPointNumber(pctx);
  eqvArr = airArrayNew(NULL, NULL, 2*sizeof(unsigned int), pointNum);
  airMopAdd(mop, eqvArr, (airMopper)airArrayNuke, airMopAlways);
  idmap = AIR_CAST(unsigned int *, calloc(pointNum, sizeof(unsigned int)));
  airMopAdd(mop, idmap, airFree, airMopAlways);

  /* to be safe, renumber all points, so that we know that the
     idtags are contiguous, starting at 0. HEY: this should handled
     by having a map from real point->idtag to a point number assigned 
     just for the sake of doing CCs */
  pctx->idtagNext = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      point->idtag = pctx->idtagNext++;
    }
  }
  
  /* same stupidity copied from limn/polymod.c:limnPolyDataCCFind */
  eqvNum = 0;
  for (passIdx=0; passIdx<2; passIdx++) {
    if (passIdx) {
      airArrayLenPreSet(eqvArr, eqvNum);
    }
    for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
      bin = pctx->bin + binIdx;
      for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
        point = bin->point[pointIdx];
        for (neighIdx=0; neighIdx<point->neighPointNum; neighIdx++) {
          if (0 == passIdx) {
            ++eqvNum;
          } else {
            her = point->neighPoint[neighIdx];
            airEqvAdd(eqvArr, point->idtag, her->idtag);
          }
        }
      }
    }
  }

  /* do the CC analysis */
  pctx->CCNum = airEqvMap(eqvArr, idmap, pointNum);

  /* assign idcc's */
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      point->idCC = idmap[point->idtag];
    }
  }
  
  airMopOkay(mop);
  return 0;
}

/*
** measure something about connected componts already found 
**
** measrInfo can be 0 to say "measure # particles in CC", or
** it can be a scalar pullInfo
*/
int
pullCCMeasure(pullContext *pctx, Nrrd *nsize, Nrrd *nmeas, int measrInfo) {
  char me[]="pullCCMeasure", err[BIFF_STRLEN];
  airArray *mop;
  unsigned int *size, binIdx, pointIdx, ii;
  double *meas;
  pullBin *bin;
  pullPoint *point;
  
  if (!( pctx && nmeas )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PULL, err); return 1;
  }
  if (!pctx->CCNum) {
    sprintf(err, "%s: CCNum == 0: haven't yet learned CCs?", me);
    biffAdd(PULL, err); return 1;
  }
  if (measrInfo) {
    if (airEnumValCheck(pullInfo, measrInfo)) {
      sprintf(err, "%s: measrInfo %d not a valid %s", me,
              measrInfo, pullInfo->name);
      biffAdd(PULL, err); return 1;
    }
    if (1 != pullInfoAnswerLen(measrInfo)) {
      sprintf(err, "%s: measrInfo %s (%d) isn't a scalar (len %u)", me,
              airEnumStr(pullInfo, measrInfo), measrInfo,
              pullInfoAnswerLen(measrInfo));
      biffAdd(PULL, err); return 1;
    }
  } /* else measrInfo is zero, they want to know # points */
  if (nrrdMaybeAlloc_va(nmeas, nrrdTypeDouble, 1, 
                        AIR_CAST(size_t, pctx->CCNum))) {
    sprintf(err, "%s: couldn't alloc nmeas", me);
    biffMove(PULL, err, NRRD); return 1;
  }
  meas = AIR_CAST(double *, nmeas->data);

  mop = airMopNew();
  if (nsize) {
    if (nrrdMaybeAlloc_va(nsize, nrrdTypeUInt, 1, 
                          AIR_CAST(size_t, pctx->CCNum))) {
      sprintf(err, "%s: couldn't alloc nsize", me);
      biffMove(PULL, err, NRRD); airMopError(mop); return 1;
    }
    size = AIR_CAST(unsigned int *, nsize->data);
  } else {
    /* HEY: don't actually need to allocate and set size[],
       if measrInfo == 0 */
    if (!(size = AIR_CAST(unsigned int *,
                          calloc(pctx->CCNum, sizeof(unsigned int))))) {
      sprintf(err, "%s: couldn't alloc size", me);
      biffAdd(PULL, err); airMopError(mop); return 1;
    }
    airMopAdd(mop, size, airFree, airMopAlways);
  }

  /* finally, do measurement */
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      size[point->idCC]++;
      meas[point->idCC] += (measrInfo
                            ? _pullPointScalar(pctx, point, measrInfo,
                                               NULL, NULL)
                            : 1);
    }
  }
  if (measrInfo) {
    for (ii=0; ii<pctx->CCNum; ii++) {
      meas[ii] /= size[ii];
    }
  }

  airMopOkay(mop);
  return 0;
}

#if 0
typedef struct {
  unsigned int i;
  double d;
} ccpair;

int
pullCCSort(pullContext *pctx, int measrInfo, double ) {
  char me[]="pullCCSort", err[BIFF_STRLEN];
  ccpair *pair;
  airArray *mop;
  unsigned int ii;
  
  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PULL, err); return 1;
  }
  if (!pctx->CCNum) {
    sprintf(err, "%s: haven't yet learned CCs?", me);
    biffAdd(PULL, err); return 1;
  }

  mop = airMopNew();
  pair = AIR_CAST(ccpair *, calloc(2*pctx->CCNum, sizeof(ccpair)));

  measr = AIR_CAST(double *, calloc(2*pctx->CCNum, sizeof(double)));
  size  = AIR_CAST(double *, calloc(2*pctx->CCNum, sizeof(double)));
  blend = AIR_CAST(double *, calloc(2*pctx->CCNum, sizeof(double)));
  if (!( ccmap = AIR_CAST(double *, ) )) {
    sprintf(err, "%s: couldn't alloc buff", me);
    biffAdd(PULL, err); return 1;
  }
  airMopAdd(mop, ccmap, airFree, airMopAlways);
  
  airMopOkay(mop);
  return 0;
}
#endif
