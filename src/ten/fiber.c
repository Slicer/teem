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
tenFiberTrace(tenFiberContext *tfx, Nrrd *nfiber,
	      double startX, double startY, double startZ) {
  char me[]="tenFiberTrace", err[AIR_STRLEN_MED];
  airArray *fptsArr[2];      /* airArrays of backward (0) and forward (1)
				fiber points */
  double *fpts[2];           /* arrays storing forward and backward
				fiber points */
  double wPos[3],            /* world position of current point */
    wLastPos[3],             /* last world position */
    iPos[3],                 /* index space position of current point */
    len[2],                  /* length of fiber */
    forwdir[3],
    lastdir[3],
    *fiber,                  /* array of both forward and backward points, 
				when finished */
    tmp;
  int i, idx, dir;

  if (!(tfx && nfiber)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (gageProbe(tfx->gtx, startX, startY, startZ)) {
    sprintf(err, "%s: first gageProbe failed: %s (%d)", 
	    me, gageErrStr, gageErrNum);
    biffAdd(TEN, err); return 1;
  }

  for (dir=0; dir<=1; dir++) {
    fptsArr[dir] = airArrayNew((void**)&(fpts[dir]), NULL, 
			       3*sizeof(double), TEN_FIBER_INCR);
    len[dir] = 0;
    iPos[0] = startX;
    iPos[1] = startY;
    iPos[2] = startZ;
    gageShapeUnitItoW(tfx->gtx->shape, wPos, iPos);
    while (1) {
      /* record old pos */
      idx = airArrayIncrLen(fptsArr[dir], 1);
      ELL_3V_COPY(fpts[dir] + 3*idx, wPos);
      ELL_3V_COPY(wLastPos, wPos);
      
      /* calculate new position */
      gageShapeUnitWtoI(tfx->gtx->shape, iPos, wPos);
      if (gageProbe(tfx->gtx, iPos[0], iPos[1], iPos[2])) {
	/* even if gageProbe had an error OTHER than going out of bounds,
	   we're not going to report it any differently here */
	tfx->stop[dir] = tenFiberStopBounds;
	break;
      }
      ELL_3V_COPY(forwdir, tfx->evec);
      if (0 == idx) {
	if (0 == dir) {
	  /* we use the eigenvector as is */
	} else {
	  /* we go opposite of the eigenvector */
	  ELL_3V_SCALE(forwdir, -1, forwdir);
	}
	fprintf(stderr, "%s: !idx, dir = %d, forwdir = (%g,%g,%g)\n", me, dir,
		forwdir[0], forwdir[1], forwdir[2]);
      } else {
	if (ELL_3V_DOT(lastdir, forwdir) < 0) {
	  ELL_3V_SCALE(forwdir, -1, forwdir);
	}
      }
      ELL_3V_SCALEADD(wPos, 1.0, wPos, tfx->step, forwdir);
      
      /* check termination conditions */
      ELL_3V_SUB(lastdir, wPos, wLastPos);
      tmp = ELL_3V_LEN(lastdir);
      len[dir] += tmp;
      if (len[dir] >= tfx->maxHalfLen) {
	tfx->stop[dir] = tenFiberStopLen;
	break;
      }
      ELL_3V_SCALE(lastdir, 1/tmp, lastdir);
    }
  }

  if (nrrdMaybeAlloc(nfiber, nrrdTypeDouble, 2,
		     3, fptsArr[1]->len + fptsArr[0]->len - 1)) {
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
