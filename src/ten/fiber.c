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

#include "ten.h"
#include "privateTen.h"

#define TEN_FIBER_INCR 512

/*
** _tenFiberProbe
**
** The job here is to probe at (world space) "wPos" and then set:
**   tfx->fiberTen
**   tfx->fiberEval (all 3 evals)
**   tfx->fiberEvec (all 3 eigenvectors)
**   if (tfx->stop & (1 << tenFiberStopAniso): tfx->fiberAnisoStop
**
** In the case of non-single-tensor tractography, we do so based on
** ten2Which (when at the seedpoint) or
**
** Note that for performance reasons, a non-zero return value
** (indicating error) and the associated use of biff, is only possible
** if seedProbe is non-zero, the reason being that problems can be
** detected at the seedpoint, and won't arise after the seedpoint.
**
** Errors from gage are indicated by *gageRet, which includes leaving
** the domain of the volume, which is used to terminate fibers.
**
** Our use of tfx->seedEvec (shared with _tenFiberAlign), as well as that
** of tfx->lastDir and tfx->lastDirSet, could stand to have further
** debugging and documentation ...
*/
int
_tenFiberProbe(tenFiberContext *tfx, int *gageRet,
               double wPos[3], int seedProbe) {
  char me[]="_tenFiberProbe", err[BIFF_STRLEN];
  double iPos[3];
  int ret = 0;

  gageShapeWtoI(tfx->gtx->shape, iPos, wPos);
  *gageRet = gageProbe(tfx->gtx, iPos[0], iPos[1], iPos[2]);
  
  if (tfx->verbose > 2) {
    fprintf(stderr, "%s(%g,%g,%g, %s): hi ----- %s\n", me,
            iPos[0], iPos[1], iPos[2], seedProbe ? "***TRUE***" : "false",
            tfx->useDwi ? "using DWIs" : "");
  }

  if (!tfx->useDwi) {
    /* normal single-tensor tracking */
    TEN_T_COPY(tfx->fiberTen, tfx->gageTen);
    ELL_3V_COPY(tfx->fiberEval, tfx->gageEval);
    ELL_3M_COPY(tfx->fiberEvec, tfx->gageEvec);
    if (tfx->stop & (1 << tenFiberStopAniso)) {
      tfx->fiberAnisoStop = tfx->gageAnisoStop[0];
    }
    if (seedProbe) {
      ELL_3V_COPY(tfx->seedEvec, tfx->fiberEvec);
    }
  } else { /* tracking in DWIs */
    if (tfx->verbose > 2 && seedProbe) {
      fprintf(stderr, "%s:   fiber type = %s\n", me,
              airEnumStr(tenDwiFiberType, tfx->fiberType));
    }
    switch (tfx->fiberType) {
      double evec[2][9], eval[2][3], len;
    case tenDwiFiberType1Evec0:
      if (tfx->mframeUse) {
        double matTmpA[9], matTmpB[9];
        TEN_T2M(matTmpA, tfx->gageTen);
        ELL_3M_MUL(matTmpB, tfx->mframe, matTmpA);
        ELL_3M_MUL(matTmpA, matTmpB, tfx->mframeT);
        TEN_M2T(tfx->fiberTen, matTmpA);
        tfx->fiberTen[0] = tfx->gageTen[0];
      } else {
        TEN_T_COPY(tfx->fiberTen, tfx->gageTen);
      }
      tenEigensolve_d(tfx->fiberEval, tfx->fiberEvec, tfx->fiberTen);
      if (tfx->stop & (1 << tenFiberStopAniso)) {
        double tmp;
        tmp = tenAnisoTen_d(tfx->fiberTen, tfx->anisoStopType);
        tfx->fiberAnisoStop = AIR_CLAMP(0, tmp, 1);
      }
      if (seedProbe) {
        ELL_3V_COPY(tfx->seedEvec, tfx->fiberEvec);
      }
      break;
    case tenDwiFiberType2Evec0:
      /* Estimate principal diffusion direction of each tensor */
      tenEigensolve_d(eval[0], evec[0], tfx->gageTen2 + 0);
      tenEigensolve_d(eval[1], evec[1], tfx->gageTen2 + 7);

      if (tfx->mframeUse) {
        double vectmp[3];
        /* NOTE: only fixing first (of three) eigenvector, for both tensors */
        ELL_3MV_MUL(vectmp, tfx->mframe, evec[0]);
        ELL_3V_COPY(evec[0], vectmp);
        ELL_3MV_MUL(vectmp, tfx->mframe, evec[1]);
        ELL_3V_COPY(evec[1], vectmp);
      }
      
      /* set ten2Use */
      if (seedProbe) {  /* we're on the *very* 1st probe per tract, 
                           at the seed pt */
        ELL_3V_COPY(tfx->seedEvec, evec[tfx->ten2Which]);
        tfx->ten2Use = tfx->ten2Which;
        if (tfx->verbose > 2) {
          fprintf(stderr, "%s: ** ten2Use == ten2Which == %d\n", me, 
                  tfx->ten2Use);
        }
      } else {
        double *lastVec, dot[2];
        
        if (!tfx->lastDirSet) {
          /* we're on some probe of the first step */
          lastVec = tfx->seedEvec;
        } else {
          /* we're past the first step */
          ELL_3V_NORM(tfx->lastDir, tfx->lastDir, len);
          lastVec = tfx->lastDir;
        }
        dot[0] = ELL_3V_DOT(lastVec, evec[0]);
        dot[1] = ELL_3V_DOT(lastVec, evec[1]);
        if (dot[0] < 0) {
          dot[0] *= -1;
          ELL_3M_SCALE(evec[0], -1, evec[0]);
        }
        if (dot[1] < 0) {
          dot[1] *= -1;
          ELL_3M_SCALE(evec[1], -1, evec[1]);
        }
        tfx->ten2Use = (dot[0] > dot[1]) ? 0 : 1;
        if (tfx->verbose > 2) {
          fprintf(stderr, "%s(%g,%g,%g): dot[0] = %f dot[1] = %f, "
                  "\t using tensor %u\n",
                  me, wPos[0], wPos[1], wPos[2], dot[0], dot[1], tfx->ten2Use );
        }
      }

      /* based on ten2Use, set the rest of the needed fields */
      if (tfx->mframeUse) {
        double matTmpA[9], matTmpB[9];
        TEN_T2M(matTmpA, tfx->gageTen2 + 7*(tfx->ten2Use));
        ELL_3M_MUL(matTmpB, tfx->mframe, matTmpA);
        ELL_3M_MUL(matTmpA, matTmpB, tfx->mframeT);
        TEN_M2T(tfx->fiberTen, matTmpA);
      } else {
        TEN_T_COPY(tfx->fiberTen, tfx->gageTen2 + 7*(tfx->ten2Use));
      }
      tfx->fiberTen[0] = tfx->gageTen2[0];   /* copy confidence */
      ELL_3V_COPY(tfx->fiberEval, eval[tfx->ten2Use]);
      ELL_3M_COPY(tfx->fiberEvec, evec[tfx->ten2Use]);
      if (tfx->stop & (1 << tenFiberStopAniso)) {
        double tmp;
        tmp = tenAnisoEval_d(tfx->fiberEval, tfx->anisoStopType);
        tfx->fiberAnisoStop = AIR_CLAMP(0, tmp, 1);
        /* HEY: what about speed? */
      } else {
        tfx->fiberAnisoStop = AIR_NAN;
      }
      break;
    default:
      sprintf(err, "%s: %s %s (%d) unimplemented!", me, 
              tenDwiFiberType->name,
              airEnumStr(tenDwiFiberType, tfx->fiberType), tfx->fiberType);
      biffAdd(TEN, err); ret = 1;
    } /* switch (tfx->fiberType) */
  }
  if (tfx->verbose > 2) {
    fprintf(stderr, "%s: fiberEvec = %g %g %g\n", me, 
            tfx->fiberEvec[0], tfx->fiberEvec[1], tfx->fiberEvec[2]);
  }

  return ret;
}

int
_tenFiberStopCheck(tenFiberContext *tfx) {
  char me[]="_tenFiberStopCheck";
  
  if (tfx->numSteps[tfx->dir] >= TEN_FIBER_NUM_STEPS_MAX) {
    fprintf(stderr, "%s: numSteps[%d] exceeded sanity check value of %d!!\n",
            me, tfx->dir, TEN_FIBER_NUM_STEPS_MAX);
    fprintf(stderr, "%s: Check fiber termination conditions, or recompile "
            "with a larger value for TEN_FIBER_NUM_STEPS_MAX\n", me);
    return tenFiberStopNumSteps;
  }
  if (tfx->stop & (1 << tenFiberStopConfidence)) {
    if (tfx->fiberTen[0] < tfx->confThresh) {
      return tenFiberStopConfidence;
    }
  }
  if (tfx->stop & (1 << tenFiberStopRadius)) {
    if (tfx->radius < tfx->minRadius) {
      return tenFiberStopRadius;
    }
  }
  if (tfx->stop & (1 << tenFiberStopAniso)) {
    if (tfx->fiberAnisoStop  < tfx->anisoThresh) {
      return tenFiberStopAniso;
    }
  }
  if (tfx->stop & (1 << tenFiberStopNumSteps)) {
    if (tfx->numSteps[tfx->dir] > tfx->maxNumSteps) {
      return tenFiberStopNumSteps;
    }
  }
  if (tfx->stop & (1 << tenFiberStopLength)) {
    if (tfx->halfLen[tfx->dir] >= tfx->maxHalfLen) {
      return tenFiberStopLength;
    }
  }
  if (tfx->useDwi
      && tfx->stop & (1 << tenFiberStopFraction)
      && tfx->gageTen2) { /* not all DWI fiber types use gageTen2 */
    double fracUse;
    fracUse = (0 == tfx->ten2Use
               ? tfx->gageTen2[7]
               : 1 - tfx->gageTen2[7]);
    if (fracUse < tfx->minFraction) {
      return tenFiberStopFraction;
    }
  }
  return 0;
}

void
_tenFiberAlign(tenFiberContext *tfx, double vec[3]) {
  char me[]="_tenFiberAlign";
  double scale, dot;
  
  if (tfx->verbose > 2) {
    fprintf(stderr, "%s: hi %s (lds %d):\t%g %g %g\n", me,
            !tfx->lastDirSet ? "**" : "  ",
            tfx->lastDirSet, vec[0], vec[1], vec[2]);
  }
  if (!(tfx->lastDirSet)) {
    dot = ELL_3V_DOT(tfx->seedEvec, vec);
    /* this is the first step (or one of the intermediate steps
       for RK4) in this fiber half; 1st half follows the
       eigenvector determined at seed point, 2nd goes opposite */
    if (tfx->verbose > 2) {
      fprintf(stderr, "!%s: dir=%d, dot=%g\n", me, tfx->dir, dot);
    }
    if (!tfx->dir) {
      /* 1st half */
      scale = dot < 0 ? -1 : 1;
    } else {
      /* 2nd half */
      scale = dot > 0 ? -1 : 1;
    }
  } else {
    dot = ELL_3V_DOT(tfx->lastDir, vec);
    /* we have some history in this fiber half */
    scale = dot < 0 ? -1 : 1;
  }
  ELL_3V_SCALE(vec, scale, vec);
  if (tfx->verbose > 2) {
    fprintf(stderr, "!%s: scl = %g -> \t%g %g %g\n",
            me, scale, vec[0], vec[1], vec[2]);
  }
  return;
}

/*
** parm[0]: lerp between 1 and the stuff below
** parm[1]: "t": (parm[1],0) is control point between (0,0) and (1,1)
** parm[2]: "d": parabolic blend between parm[1]-parm[2] and parm[1]+parm[2]
*/
void
_tenFiberAnisoSpeed(double *step, double xx, double parm[3]) {
  double aa, dd, tt, yy;

  tt = parm[1];
  dd = parm[2];
  aa = 1.0/(DBL_EPSILON + 4*dd*(1.0-tt));
  yy = xx - tt + dd;
  xx = (xx < tt - dd
        ? 0
        : (xx < tt + dd
           ? aa*yy*yy
           : (xx - tt)/(1 - tt)));
  xx = AIR_LERP(parm[0], 1, xx);
  ELL_3V_SCALE(step, xx, step);
}

/*
** -------------------------------------------------------------------
** -------------------------------------------------------------------
** The _tenFiberStep_* routines are responsible for putting a step into
** the given step[] vector.  Without anisoStepSize, this should be
** UNIT LENGTH, with anisoStepSize, its scaled by that anisotropy measure
*/
void
_tenFiberStep_Evec(tenFiberContext *tfx, double step[3]) {

  /* fiberEvec points to the correct gage answer based on fiberType */
  ELL_3V_COPY(step, tfx->fiberEvec + 3*0);
  _tenFiberAlign(tfx, step);
  if (tfx->anisoSpeedType) {
    _tenFiberAnisoSpeed(step, tfx->fiberAnisoSpeed,
                        tfx->anisoSpeedFunc);
  }
}

void
_tenFiberStep_TensorLine(tenFiberContext *tfx, double step[3]) {
  double cl, evec0[3], vout[3], vin[3], len;

  ELL_3V_COPY(evec0, tfx->fiberEvec + 3*0);
  _tenFiberAlign(tfx, evec0);

  if (tfx->lastDirSet) {
    ELL_3V_COPY(vin, tfx->lastDir);
    TEN_T3V_MUL(vout, tfx->fiberTen, tfx->lastDir);
    ELL_3V_NORM(vout, vout, len);
    _tenFiberAlign(tfx, vout);  /* HEY: is this needed? */
  } else {
    ELL_3V_COPY(vin, evec0);
    ELL_3V_COPY(vout, evec0);
  }

  /* HEY: should be using one of the tenAnisoEval[] functions */
  cl = (tfx->fiberEval[0] - tfx->fiberEval[1])/(tfx->fiberEval[0] + 0.00001);

  ELL_3V_SCALE_ADD3(step,
                    cl, evec0,
                    (1-cl)*(1-tfx->wPunct), vin,
                    (1-cl)*tfx->wPunct, vout);
  /* _tenFiberAlign(tfx, step); */
  ELL_3V_NORM(step, step, len);
  if (tfx->anisoSpeedType) {
    _tenFiberAnisoSpeed(step, tfx->fiberAnisoSpeed,
                        tfx->anisoSpeedFunc);
  }
}

void
_tenFiberStep_PureLine(tenFiberContext *tfx, double step[3]) {
  char me[]="_tenFiberStep_PureLine";

  AIR_UNUSED(tfx);
  AIR_UNUSED(step);
  fprintf(stderr, "%s: sorry, unimplemented!\n", me);
}

void
_tenFiberStep_Zhukov(tenFiberContext *tfx, double step[3]) {
  char me[]="_tenFiberStep_Zhukov";

  AIR_UNUSED(tfx);
  AIR_UNUSED(step);
  fprintf(stderr, "%s: sorry, unimplemented!\n", me);
}

void (*
_tenFiberStep[TEN_FIBER_TYPE_MAX+1])(tenFiberContext *, double *) = {
  NULL,
  _tenFiberStep_Evec,
  _tenFiberStep_Evec,
  _tenFiberStep_Evec,
  _tenFiberStep_TensorLine,
  _tenFiberStep_PureLine,
  _tenFiberStep_Zhukov
};

/*
** -------------------------------------------------------------------
** -------------------------------------------------------------------
** The _tenFiberIntegrate_* routines must assume that
** _tenFiberProbe(tfx, tfx->wPos, AIR_FALSE) has just been called
*/

int
_tenFiberIntegrate_Euler(tenFiberContext *tfx, double forwDir[3]) {

  _tenFiberStep[tfx->fiberType](tfx, forwDir);
  ELL_3V_SCALE(forwDir, tfx->stepSize, forwDir);
  return 0;
}

int
_tenFiberIntegrate_Midpoint(tenFiberContext *tfx, double forwDir[3]) {
  double loc[3], half[3];
  int gret;

  _tenFiberStep[tfx->fiberType](tfx, half);
  ELL_3V_SCALE_ADD2(loc, 1, tfx->wPos, 0.5*tfx->stepSize, half);
  _tenFiberProbe(tfx, &gret, loc, AIR_FALSE); if (gret) return 1;
  _tenFiberStep[tfx->fiberType](tfx, forwDir);
  ELL_3V_SCALE(forwDir, tfx->stepSize, forwDir);
  return 0;
}

int
_tenFiberIntegrate_RK4(tenFiberContext *tfx, double forwDir[3]) {
  double loc[3], k1[3], k2[3], k3[3], k4[3], c1, c2, c3, c4, h;
  int gret;

  h = tfx->stepSize;
  c1 = h/6.0; c2 = h/3.0; c3 = h/3.0; c4 = h/6.0;

  _tenFiberStep[tfx->fiberType](tfx, k1);
  ELL_3V_SCALE_ADD2(loc, 1, tfx->wPos, 0.5*h, k1);
  _tenFiberProbe(tfx, &gret, loc, AIR_FALSE); if (gret) return 1;
  _tenFiberStep[tfx->fiberType](tfx, k2);
  ELL_3V_SCALE_ADD2(loc, 1, tfx->wPos, 0.5*h, k2);
  _tenFiberProbe(tfx, &gret, loc, AIR_FALSE); if (gret) return 1;
  _tenFiberStep[tfx->fiberType](tfx, k3);
  ELL_3V_SCALE_ADD2(loc, 1, tfx->wPos, h, k3);
  _tenFiberProbe(tfx, &gret, loc, AIR_FALSE); if (gret) return 1;
  _tenFiberStep[tfx->fiberType](tfx, k4);

  ELL_3V_SET(forwDir,
             c1*k1[0] + c2*k2[0] + c3*k3[0] + c4*k4[0],
             c1*k1[1] + c2*k2[1] + c3*k3[1] + c4*k4[1],
             c1*k1[2] + c2*k2[2] + c3*k3[2] + c4*k4[2]);

  return 0;
}

int (*
_tenFiberIntegrate[TEN_FIBER_INTG_MAX+1])(tenFiberContext *tfx, double *) = {
  NULL,
  _tenFiberIntegrate_Euler,
  _tenFiberIntegrate_Midpoint,
  _tenFiberIntegrate_RK4
};

/*
******** tenFiberTraceSet
**
** slightly more flexible API for fiber tracking than tenFiberTrace
**
** EITHER: pass a non-NULL nfiber, and NULL, 0, NULL, NULL for
** the following arguments, and things are the same as with tenFiberTrace:
** data inside the nfiber is allocated, and the tract vertices are copied
** into it, having been stored in dynamically allocated airArrays
**
** OR: pass a NULL nfiber, and a buff allocated for 3*(2*halfBuffLen + 1)
** (note the "+ 1" !!!) doubles.  The fiber tracking on each half will stop
** at halfBuffLen points. The given seedpoint will be stored in
** buff[0,1,2 + 3*halfBuffLen].  The linear (1-D) indices for the end of
** the first tract half, and the end of the second tract half, will be set in
** *startIdxP and *endIdxP respectively (this does not include a multiply
** by 3)
*/
int
tenFiberTraceSet(tenFiberContext *tfx, Nrrd *nfiber,
                 double *buff, unsigned int halfBuffLen,
                 unsigned int *startIdxP, unsigned int *endIdxP,
                 double seed[3]) {
  char me[]="tenFiberTraceSet", err[BIFF_STRLEN];
  airArray *fptsArr[2];      /* airArrays of backward (0) and forward (1)
                                fiber points */
  double *fpts[2];           /* arrays storing forward and backward
                                fiber points */
  double
    tmp[3],
    iPos[3],
    currPoint[3],
    forwDir[3],
    *fiber;                  /* array of both forward and backward points,
                                when finished */
  int gret, whyStop, buffIdx, fptsIdx, outIdx, oldStop;
  unsigned int i;
  airArray *mop;
  
  if (!(tfx)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  /* HEY: a hack to preserve the state inside tenFiberContext so that
     we have fewer side effects (tfx->maxNumSteps may still be set) */
  oldStop = tfx->stop;
  if (!nfiber) {
    if (!( buff && halfBuffLen > 0 && startIdxP && startIdxP )) {
      sprintf(err, "%s: need either non-NULL nfiber or fpts buffer info", me);
      biffAdd(TEN, err); return 1;
    }
    if (tenFiberStopSet(tfx, tenFiberStopNumSteps, halfBuffLen)) {
      sprintf(err, "%s: error setting new fiber stop", me);
      biffAdd(TEN, err); return 1;
    }
  }

  /* initialize the quantities which describe the fiber halves */
  tfx->halfLen[0] = tfx->halfLen[1] = 0.0;
  tfx->numSteps[0] = tfx->numSteps[1] = 0;
  tfx->whyStop[0] = tfx->whyStop[1] = tenFiberStopUnknown;
  /*
  fprintf(stderr, "!%s: try probing once, at seed %g %g %g\n", me,
          seed[0], seed[1], seed[2]);
  */
  /* try probing once, at seed point */
  if (tfx->useIndexSpace) {
    gageShapeItoW(tfx->gtx->shape, tmp, seed);
  } else {
    ELL_3V_COPY(tmp, seed);
  }
  if (_tenFiberProbe(tfx, &gret, tmp, AIR_TRUE)) {
    sprintf(err, "%s: first _tenFiberProbe failed", me);
    biffAdd(TEN, err); return 1;
  }
  if (gret) {
    sprintf(err, "%s: gage problem on first _tenFiberProbe: %s (%d)",
            me, tfx->gtx->errStr, tfx->gtx->errNum);
    biffAdd(TEN, err); return 1;
  }

  /* see if we're doomed (tract dies before it gets anywhere)  */
  /* have to fake out the possible radius check, since at this point
     there is no radius of curvature; this will always pass */
  tfx->radius = DBL_MAX;
  if ((whyStop = _tenFiberStopCheck(tfx))) {
    /* stopped immediately at seed point, but that's not an error */
    tfx->whyNowhere = whyStop;
    if (nfiber) {
      nrrdEmpty(nfiber);
    } else {
      *startIdxP = *endIdxP = 0;
    }
    return 0;
  } else {
    /* did not immediately halt */
    tfx->whyNowhere = tenFiberStopUnknown;
  }

  /* airMop{Error,Okay}() can safely be called on NULL */
  mop = nfiber ? airMopNew() : NULL;

  for (tfx->dir=0; tfx->dir<=1; tfx->dir++) {
    if (nfiber) {
      fptsArr[tfx->dir] = airArrayNew((void**)&(fpts[tfx->dir]), NULL,
                                      3*sizeof(double), TEN_FIBER_INCR);
      airMopAdd(mop, fptsArr[tfx->dir], (airMopper)airArrayNuke, airMopAlways);
      fptsIdx = -1;  /* will be over-written with 1st airArrayLenIncr */
      buffIdx = -1;
    } else {
      fptsArr[tfx->dir] = NULL;
      fpts[tfx->dir] = NULL;
      fptsIdx = -1;
      buffIdx = halfBuffLen;
    }
    tfx->halfLen[tfx->dir] = 0;
    if (tfx->useIndexSpace) {
      ELL_3V_COPY(iPos, seed);
      gageShapeItoW(tfx->gtx->shape, tfx->wPos, iPos);
    } else {
      /*
      fprintf(stderr, "!%s(A): %p %p %p\n", me,
              tfx->gtx->shape, iPos, seed);
      */
      gageShapeWtoI(tfx->gtx->shape, iPos, seed);
      ELL_3V_COPY(tfx->wPos, seed);
    }
    /* have to initially pass the possible radius check in
       _tenFiberStopCheck(); this will always pass */
    tfx->radius = DBL_MAX;
    ELL_3V_SET(tfx->lastDir, 0, 0, 0);
    tfx->lastDirSet = AIR_FALSE;
    for (tfx->numSteps[tfx->dir] = 0; AIR_TRUE; tfx->numSteps[tfx->dir]++) {
      _tenFiberProbe(tfx, &gret, tfx->wPos, AIR_FALSE);
      if (gret) {
        /* even if gageProbe had an error OTHER than going out of bounds,
           we're not going to report it any differently here, alas */
        tfx->whyStop[tfx->dir] = tenFiberStopBounds;
	/*
	fprintf(stderr, "!%s: A tfx->whyStop[%d] = %s\n", me, tfx->dir,
		airEnumStr(tenFiberStop, tfx->whyStop[tfx->dir]));
	*/
        break;
      }
      if ((whyStop = _tenFiberStopCheck(tfx))) {
        if (tenFiberStopNumSteps == whyStop) {
          /* we stopped along this direction because tfx->numSteps[tfx->dir]
             exceeded tfx->maxNumSteps.  Okay.  But tfx->numSteps[tfx->dir]
             is supposed to be a record of how steps were (successfully)
             taken.  So we need to decrementing before moving on ... */
          tfx->numSteps[tfx->dir]--;
        }
        tfx->whyStop[tfx->dir] = whyStop;
	/*
	fprintf(stderr, "!%s: B tfx->whyStop[%d] = %s\n", me, tfx->dir,
		airEnumStr(tenFiberStop, tfx->whyStop[tfx->dir]));
	*/
        break;
      }
      if (tfx->useIndexSpace) {
        /*
        fprintf(stderr, "!%s(B): %p %p %p\n", me,
                tfx->gtx->shape, iPos, tfx->wPos);
        */
        gageShapeWtoI(tfx->gtx->shape, iPos, tfx->wPos);
        ELL_3V_COPY(currPoint, iPos);
      } else {
        ELL_3V_COPY(currPoint, tfx->wPos);
      }
      if (nfiber) {
        fptsIdx = airArrayLenIncr(fptsArr[tfx->dir], 1);
        ELL_3V_COPY(fpts[tfx->dir] + 3*fptsIdx, currPoint);
      } else {
        ELL_3V_COPY(buff + 3*buffIdx, currPoint);
        /*
        fprintf(stderr, "!%s: (dir %d) saving to %d pnt %g %g %g\n", me,
                tfx->dir, buffIdx,
                currPoint[0], currPoint[1], currPoint[2]);
        */
        buffIdx += !tfx->dir ? -1 : 1;
      }
      /* forwDir is set by this to point to the next fiber point */
      if (_tenFiberIntegrate[tfx->intg](tfx, forwDir)) {
        tfx->whyStop[tfx->dir] = tenFiberStopBounds;
	/*
	fprintf(stderr, "!%s: C tfx->whyStop[%d] = %s\n", me, tfx->dir,
		airEnumStr(tenFiberStop, tfx->whyStop[tfx->dir]));
	*/
        break;
      }
      /*
      fprintf(stderr, "!%s: forwDir = %g %g %g\n", me,
              forwDir[0], forwDir[1], forwDir[2]);
      */
      if (tfx->stop & (1 << tenFiberStopRadius)) {
        /* some more work required to compute radius of curvature */
        double svec[3], dvec[3], SS, DD; /* sum,diff length squared */
        if (tfx->lastDirSet) {
          ELL_3V_ADD2(svec, tfx->lastDir, forwDir);
          ELL_3V_SUB(dvec, tfx->lastDir, forwDir);
          SS = ELL_3V_DOT(svec, svec);
          DD = ELL_3V_DOT(dvec, dvec);
          tfx->radius = sqrt(SS*(SS+DD)/DD)/4;
        } else {
          tfx->radius = DBL_MAX;
        }
      }
      /*
      if (!tfx->lastDirSet) {
        fprintf(stderr, "!%s: now setting lastDirSet to (%g,%g,%g)\n", me,
                forwDir[0], forwDir[1], forwDir[2]);
      }
      */
      ELL_3V_COPY(tfx->lastDir, forwDir);
      tfx->lastDirSet = AIR_TRUE;
      ELL_3V_ADD2(tfx->wPos, tfx->wPos, forwDir);
      tfx->halfLen[tfx->dir] += ELL_3V_LEN(forwDir);
    }
  }

  if ((tfx->stop & (1 << tenFiberStopStub))
      && (2 == fptsArr[0]->len + fptsArr[1]->len)) {
    /* seed point was actually valid, but neither half got anywhere,
       and the user has set tenFiberStopStub, so we report this as
       a non-starter, via tfx->whyNowhere. */
    tfx->whyNowhere = tenFiberStopStub;
    /* for the curious, tfx->whyStop[0,1] remain set, from above */
    if (nfiber) {
      nrrdEmpty(nfiber);
    } else {
      *startIdxP = *endIdxP = 0;
    }
  } else {
    if (nfiber) {
      if (nrrdMaybeAlloc_va(nfiber, nrrdTypeDouble, 2,
                            AIR_CAST(size_t, 3),
                            AIR_CAST(size_t, (fptsArr[0]->len
                                              + fptsArr[1]->len - 1)))) {
        sprintf(err, "%s: couldn't allocate fiber nrrd", me);
        biffMove(TEN, err, NRRD); airMopError(mop); return 1;
      }
      fiber = (double*)(nfiber->data);
      outIdx = 0;
      for (i=fptsArr[0]->len-1; i>=1; i--) {
        ELL_3V_COPY(fiber + 3*outIdx, fpts[0] + 3*i);
        outIdx++;
      }
      for (i=0; i<=fptsArr[1]->len-1; i++) {
        ELL_3V_COPY(fiber + 3*outIdx, fpts[1] + 3*i);
        outIdx++;
      }
    } else {
      *startIdxP = halfBuffLen - tfx->numSteps[0];
      *endIdxP = halfBuffLen + tfx->numSteps[1];
    }
  }

  tfx->stop = oldStop;
  airMopOkay(mop);
  return 0;
}

/*
******** tenFiberTrace
**
** takes a starting position in index or world space, depending on the
** value of tfx->useIndexSpace
*/
int
tenFiberTrace(tenFiberContext *tfx, Nrrd *nfiber, double seed[3]) {
  char me[]="tenFiberTrace", err[BIFF_STRLEN];
  
  if (tenFiberTraceSet(tfx, nfiber, NULL, 0, NULL, NULL, seed)) {
    sprintf(err, "%s: problem computing tract", me);
    biffAdd(TEN, err); return 1;
  }

  return 0;
}

/*
******** tenFiberDirectionNumber
**
** NOTE: for the time being, a return of zero indicates an error, not
** that we're being clever and detect that the seedpoint is in such 
** isotropy that no directions are possible (though such cleverness
** will hopefully be implemented soon)
*/
unsigned int
tenFiberDirectionNumber(tenFiberContext *tfx, double seed[3]) {
  char me[]="tenFiberDirectionNumber", err[BIFF_STRLEN];
  unsigned int ret;

  if (!(tfx && seed)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 0;
  }

  /* HEY: eventually this stuff will be specific to the seedpoint ... */

  if (tfx->useDwi) {
    switch (tfx->fiberType) {
    case tenDwiFiberType1Evec0:
      ret = 1;
      break;
    case tenDwiFiberType2Evec0:
      ret = 2;
      break;
    case tenDwiFiberType12BlendEvec0:
      sprintf(err, "%s: sorry, type %s not yet implemented", me,
              airEnumStr(tenDwiFiberType, tenDwiFiberType12BlendEvec0));
      biffAdd(TEN, err); ret = 0;
      break;
    default: 
      sprintf(err, "%s: type %d unknown!", me, tfx->fiberType);
      biffAdd(TEN, err); ret = 0;
      break;
    }
  } else {
    /* not using DWIs */
    ret = 1;
  }

  return ret;
}

/*
******** tenFiberSingleTrace
**
** fiber tracing API that uses new tenFiberSingle, as well as being 
** aware of multi-direction tractography
**
** NOTE: this will not try any cleverness in setting "num"
** according to whether the seedpoint is a non-starter
*/
int
tenFiberSingleTrace(tenFiberContext *tfx, tenFiberSingle *tfbs,
                    double seed[3], unsigned int which) {
  char me[]="tenFiberSingleTrace", err[BIFF_STRLEN];

  if (!(tfx && tfbs && seed)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }

  /* set input fields in tfbs */
  ELL_3V_COPY(tfbs->seedPos, seed);
  tfbs->dirIdx = which;
  /* not our job to set tfbx->dirNum ... */

  /* set tfbs->nvert */
  /* no harm in setting this even when there are no multiple fibers */
  tfx->ten2Which = which;
  if (tenFiberTraceSet(tfx, tfbs->nvert, NULL, 0, NULL, NULL, seed)) {
    sprintf(err, "%s: problem computing tract", me);
    biffAdd(TEN, err); return 1;
  }

  /* set other fields based on tfx output */
  tfbs->halfLen[0] = tfx->halfLen[0];
  tfbs->halfLen[1] = tfx->halfLen[1];
  tfbs->seedIdx = tfx->numSteps[0];
  tfbs->stepNum[0] = tfx->numSteps[0];
  tfbs->stepNum[1] = tfx->numSteps[1];
  tfbs->whyStop[0] = tfx->whyStop[0];
  tfbs->whyStop[1] = tfx->whyStop[1];
  tfbs->whyNowhere = tfx->whyNowhere;
  /* no need to touch tfbs->nval or tfbs->measr */

  return 0;
}

int
tenFiberMultiCheck(airArray *arr) {
  char me[]="tenFiberMultiCheck", err[BIFF_STRLEN];

  if (!arr) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (sizeof(tenFiberSingle) != arr->unit) {
    sprintf(err, "%s: given airArray cannot be for fibers", me);
    biffAdd(TEN, err); return 1;
  }
  if (!(AIR_CAST(void (*)(void *), tenFiberSingleInit) == arr->initCB
        && AIR_CAST(void (*)(void *), tenFiberSingleDone) == arr->doneCB)) {
    sprintf(err, "%s: given airArray not set up with fiber callbacks", me);
    biffAdd(TEN, err); return 1;
  }
  return 0;
}

/*
******** tenFiberMultiTrace
**
** does tractography for a list of seedpoints 
**
** "tfbsArr" is an airArray that the caller has already set up to be
** an array of tenFiberSingle pointers, with length set to represent
** the number of tenFiberSingle's that have already been allocated
** (and pointed to).
**
** This is probably the first time that an airArray is being forced on
** Teem API users in this way, since other container structs/classes
** could probably be better here.  Alas.
*/
int
tenFiberMultiTrace(tenFiberContext *tfx, airArray *tfbsArr,
                   const Nrrd *_nseed) {
  char me[]="tenFiberMultiTrace", err[BIFF_STRLEN];
  airArray *mop;
  const double *seedData;
  double seed[3];
  unsigned int seedNum, seedIdx, fibrNum, dirNum, dirIdx;
  tenFiberSingle *tfbs;
  Nrrd *nseed;

  if (!(tfx && tfbsArr && _nseed)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (tenFiberMultiCheck(tfbsArr)) {
    sprintf(err, "%s: problem with fiber array", me);
    biffAdd(TEN, err); return 1;
  }
  if (!(2 == _nseed->dim && 3 == _nseed->axis[0].size)) {
    sprintf(err, "%s: seed list should be a 2-D (not %u-D) "
            "3-by-X (not %u-by-X) array", me, _nseed->dim,
            AIR_CAST(unsigned int, _nseed->axis[0].size));
    biffAdd(TEN, err); return 1;
  }

  mop = airMopNew();

  seedNum = _nseed->axis[1].size;
  if (nrrdTypeDouble == _nseed->type) {
    seedData = AIR_CAST(const double *, _nseed->data);
  } else {
    nseed = nrrdNew();
    airMopAdd(mop, nseed, AIR_CAST(airMopper, nrrdNuke), airMopAlways);
    if (nrrdConvert(nseed, _nseed, nrrdTypeDouble)) {
      sprintf(err, "%s: couldn't convert seed list", me);
      biffMove(TEN, err, NRRD); return 1;
    }
    seedData = AIR_CAST(const double *, nseed->data);
  }

  /* HEY: the correctness of the use of the airArray here is quite subtle */
  fibrNum = 0;
  tfbs = AIR_CAST(tenFiberSingle *, tfbsArr->data);
  for (seedIdx=0; seedIdx<seedNum; seedIdx++) {
    dirNum = tenFiberDirectionNumber(tfx, seed);
    if (!dirNum) {
      sprintf(err, "%s: couldn't learn dirNum at seed (%g,%g,%g)", me,
              seed[0], seed[1], seed[2]);
      biffAdd(TEN, err); return 1;
    }
    for (dirIdx=0; dirIdx<dirNum; dirIdx++) {
      if (tfx->verbose > 1) {
        fprintf(stderr, "%s: dir %u/%u on seed %u/%u; len %u; # %u\n",
                me, dirIdx, dirNum, seedIdx, seedNum, tfbsArr->len, fibrNum);
      }
      /* tfbsArr->len can never be < fibrNum */
      if (tfbsArr->len == fibrNum) {
        airArrayLenIncr(tfbsArr, 1);
        tfbs = AIR_CAST(tenFiberSingle *, tfbsArr->data);
      }
      ELL_3V_COPY(tfbs[fibrNum].seedPos, seedData + 3*seedIdx);
      tfbs[fibrNum].dirIdx = dirIdx;
      tfbs[fibrNum].dirNum = dirNum;
      ELL_3V_COPY(seed, seedData + 3*seedIdx);
      if (tenFiberSingleTrace(tfx, &(tfbs[fibrNum]), seed, dirIdx)) {
        sprintf(err, "%s: trouble on seed (%g,%g,%g) %u/%u, dir %u/%u", me, 
                seed[0], seed[1], seed[2], seedIdx, seedNum, dirIdx, dirNum);
        biffAdd(TEN, err); return 1;
      }
      if (tfx->verbose) {
        if (tenFiberStopUnknown == tfbs[fibrNum].whyNowhere) {
          fprintf(stderr, "%s: (%g,%g,%g) -> steps = %u,%u, stop = %s,%s\n",
                  me, seed[0], seed[1], seed[2],
                  tfbs[fibrNum].stepNum[0], tfbs[fibrNum].stepNum[1],
                  airEnumStr(tenFiberStop, tfbs[fibrNum].whyStop[0]),
                  airEnumStr(tenFiberStop, tfbs[fibrNum].whyStop[1]));
        } else {
          fprintf(stderr, "%s: (%g,%g,%g) -> nowhere: %s\n",
                  me, seed[0], seed[1], seed[2],
                  airEnumStr(tenFiberStop, tfbs[fibrNum].whyNowhere));
        }
      }
      fibrNum++;
    }
  }
  /* if the airArray got to be its length only because of the work above,
     then the following will be a no-op.  Otherwise, via the callbacks,
     it will clear out the tenFiberSingle's that we didn't create here */
  airArrayLenSet(tfbsArr, fibrNum);

  airMopOkay(mop);
  return 0;
}

int
tenFiberMultiPolyData(tenFiberContext *tfx, 
                      limnPolyData *lpld, airArray *fiberArr) {
  char me[]="tenFiberMultiPolyData", err[BIFF_STRLEN];
  tenFiberSingle *fiber;
  unsigned int seedIdx, vertTotalNum, fiberNum, fiberIdx, vertTotalIdx;

  if (!(tfx && lpld && fiberArr)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (tenFiberMultiCheck(fiberArr)) {
    sprintf(err, "%s: problem with fiber array", me);
    biffAdd(TEN, err); return 1;
  }
  fiber = AIR_CAST(tenFiberSingle *, fiberArr->data);

  vertTotalNum = 0;
  fiberNum = 0;
  for (seedIdx=0; seedIdx<fiberArr->len; seedIdx++) {
    if (!(tenFiberStopUnknown == fiber[seedIdx].whyNowhere)) {
      continue;
    }
    vertTotalNum += fiber[seedIdx].nvert->axis[1].size;
    fiberNum++;
  }

  if (limnPolyDataAlloc(lpld, 0, /* no extra per-vertex info */
                        vertTotalNum, vertTotalNum, fiberNum)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffMove(TEN, err, LIMN); return 1;
  }
    
  fiberIdx = 0;
  vertTotalIdx = 0;
  for (seedIdx=0; seedIdx<fiberArr->len; seedIdx++) {
    double *vert;
    unsigned int vertIdx, vertNum;
    if (!(tenFiberStopUnknown == fiber[seedIdx].whyNowhere)) {
      continue;
    }
    vertNum = fiber[seedIdx].nvert->axis[1].size;
    vert = AIR_CAST(double*, fiber[seedIdx].nvert->data);
    for (vertIdx=0; vertIdx<vertNum; vertIdx++) {
      ELL_3V_COPY_TT(lpld->xyzw + 4*vertTotalIdx, float, vert + 3*vertIdx);
      (lpld->xyzw + 4*vertTotalIdx)[3] = 1.0;
      lpld->indx[vertTotalIdx] = vertTotalIdx;
      vertTotalIdx++;
    }
    lpld->type[fiberIdx] = limnPrimitiveLineStrip;
    lpld->icnt[fiberIdx] = vertNum;
    fiberIdx++;
  }

  return 0;
}

