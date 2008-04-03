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

int
_pullPairwiseEnergy(pullTask *task,
                    double *enrP,
                    double frc[4],
                    pullPoint *myPoint, pullPoint *herPoint) {
  /* XX is the vector: me ----> her */
  double XX[4], nXX[4], rr, mag, radspace, radscale;
  pullEnergySpec *ensp;

  ensp = task->pctx->energySpec;

  ELL_4V_SUB(XX, herPoint->pos, myPoint->pos);
  ELL_3V_NORM(nXX, XX, rr);  /* computes rr */
  radspace = 2*task->pctx->radiusSpace;
  if (task->pctx->haveScale) {
    double ss, nss;
    radscale = 2*task->pctx->radiusScale;
    ss = XX[3]; nss = ss > 0 ? 1 : -1;
    if (task->pctx->radiusSingle) {
      ensp->energy->eval(enrP, &mag, nss*ss/radscale, ensp->parm);
      if (mag) {
        frc[3] = mag*nss/radscale;
      } else {
        frc[3] = 0;
      }
    }
  } else {
    frc[3] = 0;
  }
  ensp->energy->eval(enrP, &mag, rr/radspace, ensp->parm);
  if (mag) {
    ELL_3V_SCALE(frc, mag/radspace, nXX);
  } else {
    ELL_3V_SET(frc, 0, 0, 0);
  }

  return 0;
}

/*
** this assumes that _pullProbe() has just been called on the point,
** and the point is used only as a record of the info set there
**
** is only for "height" in 3D, not anythign along scale
*/
void
_pullPointDescent(double move[3], const pullTask *task,
                  const pullPoint *point) {
  /* char me[]="_pullPointHeightStep"; */
  const pullInfoSpec *ispec;
  const unsigned int *infoIdx;
  double val, grad[3], hess[9], tmp[3], contr, tt, gmag, norm[3];
  double moveLine[3], moveSurf[3];
  int wantLine, wantSurf;

  ispec = task->pctx->ispec[pullInfoHeight];
  infoIdx = task->pctx->infoIdx;
  val = point->info[infoIdx[pullInfoHeight]];
  ELL_3V_COPY(grad, point->info + infoIdx[pullInfoHeightGradient]);
  ELL_3M_COPY(hess, point->info + infoIdx[pullInfoHeightHessian]);
  val = (val - ispec->zero)*ispec->scale;
  ELL_3V_SCALE(grad, ispec->scale, grad);
  ELL_3M_SCALE(hess, ispec->scale, hess);

  gmag = ELL_3V_LEN(grad);
  if (gmag) {
    ELL_3V_SCALE(norm, 1.0/gmag, grad);
  } else {
    ELL_3V_COPY(norm, grad);
  }
  ELL_3MV_MUL(tmp, hess, norm);
  contr = ELL_3V_DOT(norm, tmp);
  if (contr <= 0) {
    /* if the contraction of the hessian along the gradient is
       negative then we seem to be near a local maxima of height,
       which is bad, so we do simple gradient descent. This also
       catches the case when the second derivative is zero. */
    tt = 1;
  } else {
    tt = gmag*gmag/contr;
    /* to be safe, we limit ourselves to the distance (or some scaling
       of it) that could have been gone via gradient descent */
    tt = tt/(3 + tt);
  }
  ELL_3V_SCALE(move, -tt, grad);

  wantSurf = wantLine = AIR_FALSE;
  if (task->pctx->ispec[pullInfoTangentMode]) {
    wantSurf = wantLine = AIR_TRUE;
  } else {
    if (task->pctx->ispec[pullInfoTangent2]) {
      wantLine = AIR_TRUE;
    } else if (task->pctx->ispec[pullInfoTangent1]) {
      wantSurf = AIR_TRUE;
    }
  }
  if (wantLine || wantSurf) {
    ELL_3V_SET(moveLine, AIR_NAN, AIR_NAN, AIR_NAN);
    ELL_3V_SET(moveSurf, AIR_NAN, AIR_NAN, AIR_NAN);
    if (wantLine) {
      /* with both tang1 and tang2: move within their span towards line */
      const double *tang1, *tang2;
      double tmp[3], out1[9], out2[9], proj[9];
      
      tang1 = point->info + infoIdx[pullInfoTangent1];
      tang2 = point->info + infoIdx[pullInfoTangent2];
      ELL_3MV_OUTER(out1, tang1, tang1);
      ELL_3MV_OUTER(out2, tang2, tang2);
      ELL_3M_ADD2(proj, out1, out2);
      ELL_3MV_MUL(tmp, proj, move);
      ELL_3V_COPY(moveLine, tmp);
    }
    if (wantSurf) {
      /* with tang1 only: move within its span towards surface */
      const double *tang1;
      double tmp[3], proj[9];
      
      tang1 = point->info + infoIdx[pullInfoTangent1];
      ELL_3MV_OUTER(proj, tang1, tang1);
      ELL_3MV_MUL(tmp, proj, move);
      ELL_3V_COPY(moveSurf, tmp);
    }
    if (wantLine && wantSurf) {
      /* with mode: some lerp between the two */
      double mode;
      ispec = task->pctx->ispec[pullInfoTangentMode];
      mode = point->info[infoIdx[pullInfoTangentMode]];
      mode = (mode - ispec->zero)*ispec->scale;
      mode = (1 + mode)/2;
      ELL_3V_LERP(move, mode, moveSurf, moveLine);
    } else if (wantLine) {
      ELL_3V_COPY(move, moveLine);
    } else if (wantSurf) {
      ELL_3V_COPY(move, moveSurf);
    }
  }

  return;
}

static double
_energyImage(pullTask *task, pullPoint *point,
             /* output */
             double force[4]) {

  return 0;
}

static double
_energyPoints(pullTask *task, pullPoint *point,
              /* output */
              double force[4], double *meanNeighDist) {

  return 0;
}

int
_pullPointProcess(pullTask *task, pullBin *myBin, pullPoint *myPoint) {
  char me[]="pullPointProcess";
  double enrIm, enrPt, frcIm[4], frcPt[4], enr, frc[4], meanND;

  enrIm = _energyImage(task, point, &enIm, frcIm, &meanND);
  enrPt = _energyPoints(task, point, $enPt, frcPt);

  return 0;
}

/*
** we go into this assuming that all the points we'll look at
** have just had _pullProbe() called on them
*/
int
pullBinProcess(pullTask *task, unsigned int myBinIdx) {
  char me[]="pullBinProcess", err[BIFF_STRLEN];
  pullBin *myBin;
  unsigned int myPointIdx;

  if (task->pctx->verbose > 2) {
    fprintf(stderr, "%s(%u): doing bin %u\n", me, task->threadIdx, myBinIdx);
  }
  myBin = task->pctx->bin + myBinIdx;
  for (myPointIdx=0; myPointIdx<myBin->pointNum; myPointIdx++) {
    
    if (_pullPointProcess(task, myBin, myBin->point[myPointIdx])) {
      sprintf(err, "%s: on point %u of bin %u\n", me, 
              myPointIdx, myBinIdx);
      biffAdd(PULL, err); return 1;
    }


  } /* for myPointIdx */

  return 0;
}
