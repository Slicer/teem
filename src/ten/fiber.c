/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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

#include "ten.h"
#include "tenPrivate.h"

#define TEN_FIBER_INCR 512

int
_tenFiberCheckStop(tenFiberContext *tfx) {
  
  if (tfx->stop >> tenFiberStopAniso) {
    if (tfx->aniso[tfx->anisoType] < tfx->anisoThresh) {
      return tfx->whyStop[tfx->dir] = tenFiberStopAniso;
    }
  }
  if (tfx->stop >> tenFiberStopLen) {
    if (tfx->halfLen[tfx->dir] >= tfx->maxHalfLen) {
      return tfx->whyStop[tfx->dir] = tenFiberStopLen;
    }
  }
  return 0;
}

/*
** _tenFiberForward
**
** this is responsible for setting:
** tfx->lastDir
** tfx->wPos
** tfx->halfLen[tfx->dir]
*/
void
_tenFiberForward(tenFiberContext *tfx) {
  char me[]="_tenFiberForward";
  double forwDir[3];

  switch(tfx->fiberType) {
  case tenFiberTypeEvec1:
    ELL_3V_COPY(forwDir, tfx->evec + 3*0);
    if (!(ELL_3V_LEN(tfx->lastDir))) {
      /* this is the first step in this fiber half; first half follows
	 calculated eigenvector, second half goes opposite it */
      if (tfx->dir) {
	ELL_3V_SCALE(forwDir, -1, forwDir);
      }
    } else {
      /* we have some history in this fiber half */
      if (ELL_3V_DOT(tfx->lastDir, forwDir) < 0) {
	ELL_3V_SCALE(forwDir, -1, forwDir);
      }
    }
    ELL_3V_SCALE(forwDir, tfx->stepSize, forwDir);
    break;
  case tenFiberTypeTensorLine:
  case tenFiberTypePureLine:
  case tenFiberTypeZhukov:
    fprintf(stderr, "%s: %s fibers currently not implemented!\n", me,
	    airEnumStr(tenFiberType, tfx->fiberType));
    break;
  default:
    /* what? */
    break;
  }
  ELL_3V_COPY(tfx->lastDir, forwDir);
  ELL_3V_ADD(tfx->wPos, tfx->wPos, forwDir);
  tfx->halfLen[tfx->dir] += ELL_3V_LEN(forwDir);
  return;
}

/*
******** tenFiberTrace
**
** takes a starting position (same as gage) in *index* space
*/
int
tenFiberTrace(tenFiberContext *tfx, Nrrd *nfiber,
	      double startX, double startY, double startZ) {
  char me[]="tenFiberTrace", err[AIR_STRLEN_MED];
  airArray *fptsArr[2];      /* airArrays of backward (0) and forward (1)
				fiber points */
  double *fpts[2];           /* arrays storing forward and backward
				fiber points */
  double 
    wLastPos[3],             /* last world position */
    *fiber;                  /* array of both forward and backward points, 
				when finished */
  int i, idx;

  if (!(tfx && nfiber)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (gageProbe(tfx->gtx, startX, startY, startZ)) {
    sprintf(err, "%s: first gageProbe failed: %s (%d)", 
	    me, gageErrStr, gageErrNum);
    biffAdd(TEN, err); return 1;
  }

  for (tfx->dir=0; tfx->dir<=1; tfx->dir++) {
    fptsArr[tfx->dir] = airArrayNew((void**)&(fpts[tfx->dir]), NULL, 
				    3*sizeof(double), TEN_FIBER_INCR);
    tfx->halfLen[tfx->dir] = 0;
    ELL_3V_SET(tfx->iPos, startX, startY, startZ);
    gageShapeUnitItoW(tfx->gtx->shape, tfx->wPos, tfx->iPos);
    ELL_3V_COPY(wLastPos, tfx->wPos);
    ELL_3V_SET(tfx->lastDir, 0, 0, 0);
    while (1) {
      if (gageProbe(tfx->gtx, tfx->iPos[0], tfx->iPos[1], tfx->iPos[2])) {
	/* even if gageProbe had an error OTHER than going out of bounds,
	   we're not going to report it any differently here, alas */
	tfx->whyStop[tfx->dir] = tenFiberStopBounds;
	break;
      }

      if (_tenFiberCheckStop(tfx)) {
	break;
      }

      idx = airArrayIncrLen(fptsArr[tfx->dir], 1);
      ELL_3V_COPY(fpts[tfx->dir] + 3*idx, tfx->wPos);
      ELL_3V_COPY(wLastPos, tfx->wPos);

      _tenFiberForward(tfx);
      
      gageShapeUnitWtoI(tfx->gtx->shape, tfx->iPos, tfx->wPos);
    }
  }

  if (nrrdMaybeAlloc(nfiber, nrrdTypeDouble, 2,
		     3, fptsArr[0]->len + fptsArr[1]->len - 1)) {
    sprintf(err, "%s: couldn't allocate fiber nrrd", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  fiber = (double*)(nfiber->data);
  idx = 0;
  for (i=fptsArr[0]->len-1; i>=1; i--) {
    ELL_3V_COPY(fiber + 3*idx, fpts[0] + 3*i);
    idx++;
  }
  for (i=0; i<=fptsArr[1]->len-1; i++) {
    ELL_3V_COPY(fiber + 3*idx, fpts[1] + 3*i);
    idx++;
  }
  return 0;
}
