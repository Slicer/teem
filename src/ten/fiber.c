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

int
_tenFiberProbe(tenFiberContext *tfx, double wPos[3], int seedProbe) {
  /* char me[]="_tenFiberProbe"; */
  double iPos[3];
  int ret;

  gageShapeWtoI(tfx->gtx->shape, iPos, wPos);
  ret = gageProbe(tfx->gtx, iPos[0], iPos[1], iPos[2]);
  /*
  fprintf(stderr, "!%s(%g,%g,%g): hi\n", me,
          iPos[0], iPos[1], iPos[2]);
  */
  if (!tfx->ten2Tracking) {
    /* normal single-tensor tracking */
    TEN_T_COPY(tfx->fiberTen, tfx->gageTen);
    ELL_3V_COPY(tfx->fiberEval, tfx->gageEval);
    ELL_3M_COPY(tfx->fiberEvec, tfx->gageEvec);
    if (tfx->stop & (1 << tenFiberStopAniso)) {
      tfx->fiberAnisoStop = tfx->gageAnisoStop[0];
    }
  } else {
    /* two-tensor tracking */
    double evec[2][9], eval[2][3], len;
    int useTensor=-1;

    /* Estimate principal diffusion direction of each tensor */
    tenEigensolve_d(eval[0], evec[0], tfx->gageTen2 + 0);
    tenEigensolve_d(eval[1], evec[1], tfx->gageTen2 + 7);

    /* set useTensor */
    if (seedProbe) {
      /* we're on the *very* first probe per tract, at the seed point */
      ELL_3V_COPY(tfx->seedEvec, evec[tfx->ten2Which]);
      useTensor = tfx->ten2Which;
      /*
      fprintf(stderr, "!%s: ** useTensor == ten2Which == %d\n", me, 
              useTensor);
      */
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
      useTensor = (dot[0] > dot[1]) ? 0 : 1;
      /*
      fprintf(stderr, "!%s(%g,%g,%g): dot[0] = %f dot[1] = %f, "
              "\t using tensor %d\n",
              me, wPos[0], wPos[1], wPos[2], dot[0], dot[1], useTensor );
      */
    }
    TEN_T_COPY(tfx->fiberTen, tfx->gageTen2 + 7*useTensor);
    tfx->fiberTen[0] = tfx->gageTen2[0];   /* copy confidence */
    ELL_3V_COPY(tfx->fiberEval, eval[useTensor]);
    ELL_3M_COPY(tfx->fiberEvec, evec[useTensor]);
    if (tfx->stop & (1 << tenFiberStopAniso)) {
      double tmp;
      tmp = tenAnisoEval_d(tfx->fiberEval, tfx->anisoStopType);
      tfx->fiberAnisoStop = AIR_CLAMP(0, tmp, 1);
      /* HEY: what about speed? */
    }
  }
  /*
  fprintf(stderr, "!%s: fiberEvec = %g %g %g\n", me, 
          tfx->fiberEvec[0], tfx->fiberEvec[1], tfx->fiberEvec[2]);
  */
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
  return 0;
}

void
_tenFiberAlign(tenFiberContext *tfx, double vec[3]) {
  /* char me[]="_tenFiberAlign"; */
  double scale, dot;
  
  /*
  fprintf(stderr, "!%s: hi %s (lds %d):\t%g %g %g\n", me,
          !tfx->lastDirSet ? "**" : "  ",
          tfx->lastDirSet, vec[0], vec[1], vec[2]);
  */
  if (!(tfx->lastDirSet)) {
    dot = ELL_3V_DOT(tfx->seedEvec, vec);
    /* this is the first step (or one of the intermediate steps
       for RK4) in this fiber half; 1st half follows the
       eigenvector determined at seed point, 2nd goes opposite */
    /*
    fprintf(stderr, "!%s: dir=%d, dot=%g\n", me, tfx->dir, dot);
    */
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
  /*
  fprintf(stderr, "!%s: scl = %g -> \t%g %g %g\n",
          me, scale, vec[0], vec[1], vec[2]);
  */
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

  _tenFiberStep[tfx->fiberType](tfx, half);
  ELL_3V_SCALE_ADD2(loc, 1, tfx->wPos, 0.5*tfx->stepSize, half);
  if (_tenFiberProbe(tfx, loc, AIR_FALSE)) return 1;
  _tenFiberStep[tfx->fiberType](tfx, forwDir);
  ELL_3V_SCALE(forwDir, tfx->stepSize, forwDir);
  return 0;
}

int
_tenFiberIntegrate_RK4(tenFiberContext *tfx, double forwDir[3]) {
  double loc[3], k1[3], k2[3], k3[3], k4[3], c1, c2, c3, c4, h;

  h = tfx->stepSize;
  c1 = h/6.0; c2 = h/3.0; c3 = h/3.0; c4 = h/6.0;

  _tenFiberStep[tfx->fiberType](tfx, k1);
  ELL_3V_SCALE_ADD2(loc, 1, tfx->wPos, 0.5*h, k1);
  if (_tenFiberProbe(tfx, loc, AIR_FALSE)) return 1;
  _tenFiberStep[tfx->fiberType](tfx, k2);
  ELL_3V_SCALE_ADD2(loc, 1, tfx->wPos, 0.5*h, k2);
  if (_tenFiberProbe(tfx, loc, AIR_FALSE)) return 1;
  _tenFiberStep[tfx->fiberType](tfx, k3);
  ELL_3V_SCALE_ADD2(loc, 1, tfx->wPos, h, k3);
  if (_tenFiberProbe(tfx, loc, AIR_FALSE)) return 1;
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
  int ret, whyStop, buffIdx, fptsIdx, outIdx, oldStop;
  unsigned int i;
  airArray *mop;
  
  fprintf(stderr, "!%s: type = %s (%d)\n", me, 
          airEnumStr(tenFiberType, tfx->fiberType), tfx->fiberType);
          

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
    ret = _tenFiberProbe(tfx, tmp, AIR_TRUE);
  } else {
    ret = _tenFiberProbe(tfx, seed, AIR_TRUE);
  }
  if (ret) {
    sprintf(err, "%s: first _tenFiberProbe failed: %s (%d)",
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

  /* record the principal eigenvector at the seed point, so that we know
     which direction to go at the beginning of each half */
  ELL_3V_COPY(tfx->seedEvec, tfx->fiberEvec + 3*0);
  /*
  fprintf(stderr, "!%s: **** seedEvec = %g %g %g\n", me,
          tfx->seedEvec[0], tfx->seedEvec[1], tfx->seedEvec[2]);
  */

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
      gageShapeWtoI(tfx->gtx->shape, iPos, seed);
      ELL_3V_COPY(tfx->wPos, seed);
    }
    /* have to initially pass the possible radius check in
       _tenFiberStopCheck(); this will always pass */
    tfx->radius = DBL_MAX;
    ELL_3V_SET(tfx->lastDir, 0, 0, 0);
    tfx->lastDirSet = AIR_FALSE;
    TEN_T_SET(tfx->lastTen, 0,   0, 0, 0,   0, 0,  0);
    tfx->lastTenSet = AIR_FALSE;
    /*
    fprintf(stderr, "!%s: (again) seedEvec = %g %g %g\n", me,
            tfx->seedEvec[0], tfx->seedEvec[1], tfx->seedEvec[2]);
    */
    for (tfx->numSteps[tfx->dir] = 0; AIR_TRUE; tfx->numSteps[tfx->dir]++) {
      if (_tenFiberProbe(tfx, tfx->wPos, AIR_FALSE)) {
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
