/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "pull.h"
#include "privatePull.h"

pullTrace * /* Biff: nope */
pullTraceNew(void) {
  pullTrace *ret;

  ret = AIR_CALLOC(1, pullTrace);
  if (ret) {
    ret->seedPos[0] = ret->seedPos[1] = AIR_NAN;
    ret->seedPos[2] = ret->seedPos[3] = AIR_NAN;
    ret->nvert = nrrdNew();
    ret->nstrn = nrrdNew();
    ret->nstab = nrrdNew();
    ret->norin = nrrdNew();
    ret->seedIdx = 0;
    ret->whyStop[0] = ret->whyStop[1] = pullTraceStopUnknown;
    ret->whyStopCFail[0] = ret->whyStopCFail[1] = pullConstraintFailUnknown;
    ret->whyNowhere = pullTraceStopUnknown;
    ret->whyNowhereCFail = pullConstraintFailUnknown;
  }
  return ret;
}

pullTrace * /* Biff: nope */
pullTraceNix(pullTrace *pts) {

  if (pts) {
    nrrdNuke(pts->nvert);
    nrrdNuke(pts->nstrn);
    nrrdNuke(pts->nstab);
    nrrdNuke(pts->norin);
    free(pts);
  }
  return NULL;
}

void
pullTraceStability(double *spcStab,
                   double *oriStab,
                   const double pos0[4],
                   const double pos1[4],
                   const double ori0[3],
                   const double ori1[3],
                   double sigma0,
                   const pullContext *pctx) {
  double sc, stb, dx, ds, diff[4];

  ELL_4V_SUB(diff, pos1, pos0);
  dx = ELL_3V_LEN(diff) / (pctx->voxelSizeSpace);
  sc = (pos0[3] + pos1[3]) / 2;
  if (pctx->flag.scaleIsTau) {
    sc = gageSigOfTau(sc);
  }
  sc += sigma0;
  dx /= sc;
  ds = diff[3];
  stb = atan2(ds, dx) / (AIR_PI / 2); /* [-1,1]: 0 means least stable */
  stb = AIR_ABS(stb);                 /* [0,1]: 0 means least stable */
  *spcStab = AIR_CLAMP(0, stb, 1);
  /*
  static double maxvelo = 0;
  if (1 && vv > maxvelo) {
    static const char me[] = "pullTraceStability";
    printf("\n%s: [%g,%g,%g,%g] - [%g,%g,%g,%g] = [%g,%g,%g,%g]\n", me,
           pos0[0], pos0[1], pos0[2], pos0[3],
           pos1[0], pos1[1], pos1[2], pos1[3],
           diff[0], diff[1], diff[2], diff[3]);
    printf("%s: dx = %g -> %g; ds = %g\n", me, dx0, dx, ds);
    printf("%s: vv = atan2(%g,%g)/pi = %g -> %g > %g\n", me, ds, dx, vv0, vv, maxvelo);
    maxvelo = vv;
  }
  */
  if (ori0 && ori1) {
    /* dori = delta in orientation */
    double dori = ell_3v_angle_d(ori0, ori1);
    /* dori in [0,pi]; 0 and pi mean no change */
    if (dori > AIR_PI / 2) {
      dori -= AIR_PI;
    }
    /* dori in [-pi/2,pi/2]; 0 means no change; +-pi/2 means most change */
    dori = AIR_ABS(dori);
    /* dori in [0,pi/2]; 0 means no change; pi/2 means most change */
    *oriStab = atan2(ds, dori) / (AIR_PI / 2);
    /* *oriStab in [0,1]; 0 means stable, 1 means not stable */
  } else {
    *oriStab = 0;
  }
  return;
}

static int /* Biff: 1 */
_pullConstrTanSlap(pullContext *pctx, pullPoint *point, double tlen,
                   /* input+output */ double toff[3],
                   /* output */ int *constrFailP) {
  static const char me[] = "_pullConstrTanSlap";
  double pos0[4], tt;

  if (!(pctx && point && toff && constrFailP)) {
    biffAddf(PULL, "%s: got NULL pointer", me);
    return 1;
  }
  if (pctx->flag.zeroZ) {
    toff[2] = 0;
  }
  ELL_3V_NORM(toff, toff, tt);
  if (!(tlen > 0 && tt > 0)) {
    biffAddf(PULL, "%s: tlen %g or |toff| %g not positive", me, tlen, tt);
    return 1;
  }
  ELL_4V_COPY(pos0, point->pos); /* pos0[3] should stay put; will test at end */

  ELL_3V_SCALE_ADD2(point->pos, 1, pos0, tlen, toff);
  if (_pullConstraintSatisfy(pctx->task[0], point, 0 /* travmax */, constrFailP)) {
    biffAddf(PULL, "%s: trouble", me);
    ELL_4V_COPY(point->pos, pos0);
    return 1;
  }
  /* save offset to new position */
  ELL_3V_SUB(toff, point->pos, pos0);

  if (pos0[3] != point->pos[3]) {
    biffAddf(PULL, "%s: point->pos[3] %g was changed (from %g)", me, point->pos[3],
             pos0[3]);
    ELL_4V_COPY(point->pos, pos0);
    return 1;
  }
  ELL_4V_COPY(point->pos, pos0);
  return 0;
}

static int /* Biff: 1 */
_pullConstrOrientFind(pullContext *pctx, pullPoint *point,
                      int normalfind, /* find normal two 2D surface,
                                         else find tangent to 1D curve */
                      double tlen,
                      const double tdir0[3], /* if non-NULL, try using this direction */
                      /* output */
                      double dir[3], int *constrFailP) {
  static const char me[] = "_pullConstrOrientFind";
  double tt;

#define SLAP(LEN, DIR)                                                                  \
  /* fprintf(stderr, "!%s: SLAP %g %g %g -->", me, (DIR)[0], (DIR)[1], (DIR)[2]); */    \
  if (_pullConstrTanSlap(pctx, point, (LEN), (DIR), constrFailP)) {                     \
    biffAddf(PULL, "%s: looking for tangent, starting with (%g,%g,%g)", me, (DIR)[0],   \
             (DIR)[1], (DIR)[2]);                                                       \
    return 1;                                                                           \
  }                                                                                     \
  if (*constrFailP) {                                                                   \
    /* unsuccessful in finding tangent, but not a biff error */                         \
    return 0;                                                                           \
  }                                                                                     \
  /* fprintf(stderr, " %g %g %g\n", (DIR)[0], (DIR)[1], (DIR)[2]); */

  if (!(pctx && point && constrFailP)) {
    biffAddf(PULL, "%s: got NULL pointer", me);
    return 1;
  }
  *constrFailP = pullConstraintFailUnknown;
  if (normalfind) {
    double tan0[3], tan1[3];
    /* looking for a surface normal */
    if (tdir0) { /* have something to start with */
      ell_3v_perp_d(tan0, tdir0);
      ELL_3V_CROSS(tan1, tan0, tdir0);
      SLAP(tlen, tan1);
      SLAP(tlen, tan0);
      ELL_3V_CROSS(dir, tan1, tan0);
    } else { /* have to start from scratch */
      double tns[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
      SLAP(tlen, tns + 0);
      SLAP(tlen, tns + 3);
      SLAP(tlen, tns + 6);
      ell_3m_1d_nullspace_d(dir, tns);
    }
  } else {
    /* looking for a curve tangent */
    if (tdir0) { /* have something to start with */
      ELL_3V_COPY(dir, tdir0);
      SLAP(tlen, dir);
      /* SLAP(tlen, dir);   (didn't have much effect, in one test) */
    } else { /* have to start from scratch */
      double tX[3] = {1, 0, 0}, tY[3] = {0, 1, 0}, tZ[3] = {0, 0, 1};
      SLAP(tlen, tX);
      SLAP(tlen, tY);
      if (ELL_3V_DOT(tX, tY) < 0) {
        ELL_3V_SCALE(tY, -1, tY);
      }
      if (pctx->flag.zeroZ) {
        ELL_3V_SET(tZ, 0, 0, 0);
      } else {
        SLAP(tlen, tZ);
        if (ELL_3V_DOT(tX, tZ) < 0) {
          ELL_3V_SCALE(tZ, -1, tZ);
        }
        if (ELL_3V_DOT(tY, tZ) < 0) {
          ELL_3V_SCALE(tY, -1, tZ);
        }
      }
      ELL_3V_ADD3(dir, tX, tY, tZ);
    }
  }
  ELL_3V_NORM(dir, dir, tt);
  if (!(tt > 0)) {
    biffAddf(PULL, "%s: computed direction is zero (%g)?", me, tt);
    return 1;
  }
  return 0;
}

/*
******** pullTraceSet
**
** computes a single trace, according to the given parameters,
** and store it in the pullTrace
int recordStrength: should strength be recorded along trace
double scaleDelta: discrete step along scale
double halfScaleWin: how far, along scale, trace should go in each direction
unsigned int arrIncr: increment for storing position (maybe strength)
const double _seedPos[4]: starting position
*/
int /* Biff: 1 */
pullTraceSet(pullContext *pctx, pullTrace *pts, int recordStrength, double scaleDelta,
             double halfScaleWin, double orientTestLen, unsigned int arrIncr,
             const double _seedPos[4]) {
  static const char me[] = "pullTraceSet";
  pullPoint *point;
  airArray *mop, *trceArr[2], *hstrnArr[2], *horinArr[2];
  double *trce[2], ssrange[2], *vert, *hstrn[2], *horin[2], *strn, *orin, *stab,
    seedPos[4], polen, porin[3];
  int constrFail;
  unsigned int dirIdx, lentmp, tidx, oidx, vertNum;

  if (!(pctx && pts && _seedPos)) {
    biffAddf(PULL, "%s: got NULL pointer", me);
    return 1;
  }
  if (!(AIR_EXISTS(scaleDelta) && scaleDelta > 0.0)) {
    biffAddf(PULL, "%s: need existing scaleDelta > 0 (not %g)", me, scaleDelta);
    return 1;
  }
  if (!(halfScaleWin > 0)) {
    biffAddf(PULL, "%s: need halfScaleWin > 0", me);
    return 1;
  }
  if (!(pctx->constraint)) {
    biffAddf(PULL, "%s: given context doesn't have constraint set", me);
    return 1;
  }
  pts->fdim = _pullConstraintDim(pctx);
  if (0 > pts->fdim) {
    biffAddf(PULL, "%s: couldn't learn dimension of feature", me);
    return 1;
  }
  if (pts->fdim == 2 && pctx->flag.zeroZ) {
    biffAddf(PULL, "%s: can't have feature dim 2 with zeroZ", me);
    return 1;
  }
  if (!(AIR_EXISTS(orientTestLen) && orientTestLen >= 0)) {
    biffAddf(PULL, "%s: need non-negative orientTestLen (not %g)\n", me, orientTestLen);
    return 1;
  }
  if (pts->fdim && !orientTestLen) {
    /* not really an error */
    /*
    fprintf(stderr, "\n\n%s: WARNING: have a %d-D feature, but not "
            "measuring its orientation\n\n\n", me, pts->fdim);
    */
  }
  /* The test for "should I measure orientation"
     is "if (pts->fdim && orientTestLen)" */
  if (recordStrength && !pctx->ispec[pullInfoStrength]) {
    biffAddf(PULL, "%s: want to record strength but %s not set in context", me,
             airEnumStr(pullInfo, pullInfoStrength));
    return 1;
  }
  if (pullConstraintScaleRange(pctx, ssrange)) {
    biffAddf(PULL, "%s: trouble getting scale range", me);
    return 1;
  }

  /* re-initialize termination descriptions (in case of trace re-use) */
  pts->whyStop[0] = pts->whyStop[1] = pullTraceStopUnknown;
  pts->whyStopCFail[0] = pts->whyStopCFail[1] = pullConstraintFailUnknown;
  pts->whyNowhere = pullTraceStopUnknown;
  pts->whyNowhereCFail = pullConstraintFailUnknown;

  /* enforce zeroZ */
  ELL_4V_COPY(seedPos, _seedPos);
  if (pctx->flag.zeroZ) {
    seedPos[2] = 0.0;
  }

  /* save seedPos in any case */
  ELL_4V_COPY(pts->seedPos, seedPos);

  mop = airMopNew();
  point = pullPointNew(pctx); /* we'll want to decrement idtagNext later */
  airMopAdd(mop, point, (airMopper)pullPointNix, airMopAlways);

  ELL_4V_COPY(point->pos, seedPos);
  /*
  if (pctx->verbose) {
    fprintf(stderr, "%s: trying at seed=(%g,%g,%g,%g)\n", me,
            seedPos[0], seedPos[1], seedPos[2], seedPos[3]);
  }
  */
  /* The termination of the trace due to low stablity is handled here
     (or could be), not by _pullConstraintSatisfy, so we set 0 for the
     travelMax arg of _pullConstraintSatisfy (no travel limit) */
  if (_pullConstraintSatisfy(pctx->task[0], point, 0 /* travmax */, &constrFail)) {
    biffAddf(PULL, "%s: constraint sat on seed point", me);
    airMopError(mop);
    return 1;
  }
  /*
  if (pctx->verbose) {
    fprintf(stderr, "%s: seed=(%g,%g,%g,%g) -> %s (%g,%g,%g,%g)\n", me,
            seedPos[0], seedPos[1], seedPos[2], seedPos[3],
            constrFail ? "!NO!" : "(yes)",
            point->pos[0] - seedPos[0], point->pos[1] - seedPos[1],
            point->pos[2] - seedPos[2], point->pos[3] - seedPos[3]);
  }
  */
  if (constrFail) {
    pts->whyNowhere = pullTraceStopConstrFail;
    airMopOkay(mop);
    /* pctx->idtagNext -= 1; / * HACK * / */
    return 0;
  }
  if (point->status & PULL_STATUS_EDGE_BIT) {
    pts->whyNowhere = pullTraceStopVolumeEdge;
    airMopOkay(mop);
    /* pctx->idtagNext -= 1; / * HACK * / */
    return 0;
  }
  if (pctx->flag.zeroZ && point->pos[2] != 0) {
    biffAddf(PULL, "%s: zeroZ violated (a)", me);
    airMopError(mop);
    return 1;
  }

  /* else constraint sat worked at seed point; we have work to do */
  if (pts->fdim && orientTestLen) {
    double pos0[4], dp[4];
    int cf;
    /* learn orientation at seed point */
    polen = (orientTestLen
             *pctx->voxelSizeSpace
             /* if used, the effect of this this last (unprincipled) factor
                is gradually increase the test distance with scale
                *(1 + gageTauOfSig(_pullSigma(pctx, point->pos))) */ );
    ELL_4V_COPY(pos0, point->pos);

    if (_pullConstrOrientFind(pctx, point, pts->fdim == 2, polen,
                              NULL /* no initial guess */, porin, &cf)) {
      biffAddf(PULL, "%s: trying to find orientation at seed", me);
      airMopError(mop);
      return 1;
    }
    ELL_4V_SUB(dp, pos0, point->pos);
    AIR_UNUSED(dp[0]); /* to quiet set-but-not-used warning */
    /*
    fprintf(stderr, "!%s: cf = %d (%s)\n", me, cf, airEnumStr(pullConstraintFail, cf));
    fprintf(stderr, "!%s: (fdim=%u) pos=[%g,%g,%g,%g] polen=%g porin=[%g,%g,%g] |%g|\n",
            me, pts->fdim,
            point->pos[0], point->pos[1], point->pos[2], point->pos[3],
            polen, porin[0], porin[1], porin[2], ELL_4V_LEN(dp));
    */
    if (cf) {
      pts->whyNowhere = pullTraceStopConstrFail;
      pts->whyNowhereCFail = cf;
      airMopOkay(mop);
      /* pctx->idtagNext -= 1; / * HACK * / */
      return 0;
    }
  } else {
    /* either feature is 0D points, or don't care about orientation */
    polen = AIR_NAN;
    ELL_3V_SET(porin, AIR_NAN, AIR_NAN, AIR_NAN);
  }

  for (dirIdx = 0; dirIdx < 2; dirIdx++) {
    trceArr[dirIdx] = airArrayNew((void **)(trce + dirIdx), NULL, 4 * sizeof(double),
                                  arrIncr);
    airMopAdd(mop, trceArr[dirIdx], (airMopper)airArrayNuke, airMopAlways);
    if (recordStrength) {
      hstrnArr[dirIdx] = airArrayNew((void **)(hstrn + dirIdx), NULL, sizeof(double),
                                     arrIncr);
      airMopAdd(mop, hstrnArr[dirIdx], (airMopper)airArrayNuke, airMopAlways);
    } else {
      hstrnArr[dirIdx] = NULL;
      hstrn[dirIdx] = NULL;
    }
    if (pts->fdim && orientTestLen) {
      horinArr[dirIdx] = airArrayNew((void **)(horin + dirIdx), NULL, 3 * sizeof(double),
                                     arrIncr);
      airMopAdd(mop, horinArr[dirIdx], (airMopper)airArrayNuke, airMopAlways);
    } else {
      horinArr[dirIdx] = NULL;
      horin[dirIdx] = NULL;
    }
  }
  for (dirIdx = 0; dirIdx < 2; dirIdx++) {
    unsigned int step;
    double dscl;
    dscl = (!dirIdx ? -1 : +1) * scaleDelta;
    step = 0;
    while (1) {
      if (!step) {
        /* first step in both directions requires special tricks */
        if (0 == dirIdx) {
          /* this is done once, at the very start */
          /* save constraint sat of seed point */
          tidx = airArrayLenIncr(trceArr[0], 1);
          ELL_4V_COPY(trce[0] + 4 * tidx, point->pos);
          if (recordStrength) {
            airArrayLenIncr(hstrnArr[0], 1);
            hstrn[0][0] = pullPointScalar(pctx, point, pullInfoStrength, NULL, NULL);
          }
          if (pts->fdim && orientTestLen) {
            airArrayLenIncr(horinArr[0], 1);
            ELL_3V_COPY(horin[0] + 3 * 0, porin);
          }
        } else {
          /* re-set position from constraint sat of seed pos */
          ELL_4V_COPY(point->pos, trce[0] + 4 * 0);
          if (pts->fdim && orientTestLen) {
            ELL_3V_COPY(porin, horin[0] + 3 * 0);
          }
        }
      }
      /* nudge position along scale */
      point->pos[3] += dscl;
      if (!AIR_IN_OP(ssrange[0], point->pos[3], ssrange[1])) {
        /* if we've stepped outside the range of scale for the volume
           containing the constraint manifold, we're done */
        pts->whyStop[dirIdx] = pullTraceStopBounds;
        break;
      }
      if (AIR_ABS(point->pos[3] - seedPos[3]) > halfScaleWin) {
        /* we've moved along scale as far as allowed */
        pts->whyStop[dirIdx] = pullTraceStopLength;
        break;
      }
      /* re-assert constraint */
      /*
      fprintf(stderr, "%s(%u): pos = %g %g %g %g.... \n", me,
              point->idtag, point->pos[0], point->pos[1],
              point->pos[2], point->pos[3]);
      */
      if (_pullConstraintSatisfy(pctx->task[0], point, 0 /* travmax */, &constrFail)) {
        biffAddf(PULL, "%s: dir %u, step %u", me, dirIdx, step);
        airMopError(mop);
        return 1;
      }
      /*
      if (pctx->verbose) {
        fprintf(stderr, "%s(%u): ... %s(%d); pos = %g %g %g %g\n", me,
                point->idtag,
                constrFail ? "FAIL" : "(ok)",
                constrFail, point->pos[0], point->pos[1],
                point->pos[2], point->pos[3]);
      }
      */
      if (point->status & PULL_STATUS_EDGE_BIT) {
        pts->whyStop[dirIdx] = pullTraceStopVolumeEdge;
        break;
      }
      if (constrFail) {
        /* constraint sat failed; no error, we're just done
           with stepping for this direction */
        pts->whyStop[dirIdx] = pullTraceStopConstrFail;
        pts->whyStopCFail[dirIdx] = constrFail;
        break;
      }
      if (pctx->flag.zeroZ && point->pos[2] != 0) {
        biffAddf(PULL, "%s: zeroZ violated (b)", me);
        airMopError(mop);
        return 1;
      }
      if (pts->fdim && orientTestLen) {
        if (_pullConstrOrientFind(pctx, point, pts->fdim == 2, polen, porin, porin,
                                  &constrFail)) {
          biffAddf(PULL, "%s: at dir %u, step %u", me, dirIdx, step);
          airMopError(mop);
          return 1;
        }
      }
      if (trceArr[dirIdx]->len >= 2) {
        /* see if we're moving too fast, by comparing with previous point */
        /* actually, screw that */
      }
      /* else save new point on trace */
      tidx = airArrayLenIncr(trceArr[dirIdx], 1);
      ELL_4V_COPY(trce[dirIdx] + 4 * tidx, point->pos);
      if (recordStrength) {
        tidx = airArrayLenIncr(hstrnArr[dirIdx], 1);
        hstrn[dirIdx][tidx] = pullPointScalar(pctx, point, pullInfoStrength, NULL, NULL);
      }
      if (pts->fdim && orientTestLen) {
        tidx = airArrayLenIncr(horinArr[dirIdx], 1);
        ELL_3V_COPY(horin[dirIdx] + 3 * tidx, porin);
      }
      step++;
    }
  }

  /* transfer trace halves to pts->nvert */
  vertNum = trceArr[0]->len + trceArr[1]->len;
  if (0 == vertNum || 1 == vertNum || 2 == vertNum) {
    pts->whyNowhere = pullTraceStopStub;
    airMopOkay(mop);
    /* pctx->idtagNext -= 1; / * HACK * / */
    return 0;
  }

  if (nrrdMaybeAlloc_va(pts->nvert, nrrdTypeDouble, 2, AIR_SIZE_T(4),
                        AIR_SIZE_T(vertNum))
      || nrrdMaybeAlloc_va(pts->nstab, nrrdTypeDouble, 2, AIR_SIZE_T(2),
                           AIR_SIZE_T(vertNum))) {
    biffMovef(PULL, NRRD, "%s: allocating output", me);
    airMopError(mop);
    return 1;
  }
  if (recordStrength) {
    /* doing slicing is a simple form of allocation */
    if (nrrdSlice(pts->nstrn, pts->nvert, 0 /* axis */, 0 /* pos */)) {
      biffMovef(PULL, NRRD, "%s: allocating strength output", me);
      airMopError(mop);
      return 1;
    }
  }
  if (pts->fdim && orientTestLen) {
    /* cropping just to allocate */
    size_t cmin[2] = {0, 0}, cmax[2];
    ELL_2V_SET(cmax, 2, pts->nvert->axis[1].size - 1);
    if (nrrdCrop(pts->norin, pts->nvert, cmin, cmax)) {
      biffMovef(PULL, NRRD, "%s: allocating orientation output", me);
      airMopError(mop);
      return 1;
    }
  }
  vert = AIR_CAST(double *, pts->nvert->data);
  if (recordStrength) {
    strn = AIR_CAST(double *, pts->nstrn->data);
  } else {
    strn = NULL;
  }
  if (pts->fdim && orientTestLen) {
    orin = AIR_CAST(double *, pts->norin->data);
  } else {
    orin = NULL;
  }
  stab = AIR_CAST(double *, pts->nstab->data);
  lentmp = trceArr[0]->len;
  oidx = 0;
  for (tidx = 0; tidx < lentmp; tidx++) {
    ELL_4V_COPY(vert + 4 * oidx, trce[0] + 4 * (lentmp - 1 - tidx));
    if (strn) {
      strn[oidx] = hstrn[0][lentmp - 1 - tidx];
    }
    if (orin) {
      ELL_3V_COPY(orin + 3 * oidx, horin[0] + 3 * (lentmp - 1 - tidx));
    }
    oidx++;
  }
  /* the last index written to (before oidx++) was the seed index */
  pts->seedIdx = oidx - 1;
  lentmp = trceArr[1]->len;
  for (tidx = 0; tidx < lentmp; tidx++) {
    ELL_4V_COPY(vert + 4 * oidx, trce[1] + 4 * tidx);
    if (strn) {
      strn[oidx] = hstrn[1][tidx];
    }
    if (orin) {
      ELL_3V_COPY(orin + 3 * oidx, horin[1] + 3 * tidx);
    }
    oidx++;
  }
  lentmp = AIR_UINT(pts->nstab->axis[1].size);
  if (1 == lentmp) {
    stab[0 + 2 * 0] = 0.0;
    stab[1 + 2 * 0] = 0.0;
  } else {
    for (tidx = 0; tidx < lentmp; tidx++) {
      double *pA, *pB, *p0, *p1, *p2, *rA, *rB, *r0 = NULL, *r1 = NULL, *r2 = NULL;
      p0 = vert + 4 * (tidx - 1);
      p1 = vert + 4 * tidx;
      p2 = vert + 4 * (tidx + 1);
      if (orin) {
        r0 = orin + 3 * (tidx - 1);
        r1 = orin + 3 * tidx;
        r2 = orin + 3 * (tidx + 1);
      }
      if (!tidx) {
        /* first */
        pA = p1;
        rA = r1;
        pB = p2;
        rB = r2;
      } else if (tidx < lentmp - 1) {
        /* middle */
        pA = p0;
        rA = r0;
        pB = p2;
        rB = r2;
      } else {
        /* last */
        pA = p0;
        rA = r0;
        pB = p1;
        rB = r1;
      }
      pullTraceStability(stab + 0 + 2 * tidx, stab + 1 + 2 * tidx, pA, pB, rA, rB,
                         0.5 /* sigma0 */, pctx);
    }
  }

  airMopOkay(mop);
  /* pctx->idtagNext -= 1; / * HACK * / */
  return 0;
}

typedef union {
  pullTrace ***trace;
  void **v;
} blahblahUnion;

pullTraceMulti * /* Biff: nope */
pullTraceMultiNew(void) {
  /* static const char me[] = "pullTraceMultiNew"; */
  pullTraceMulti *ret;
  blahblahUnion bbu;

  ret = AIR_CALLOC(1, pullTraceMulti);
  if (ret) {
    ret->trace = NULL;
    ret->traceNum = 0;
    ret->traceArr = airArrayNew((bbu.trace = &(ret->trace), bbu.v), &(ret->traceNum),
                                sizeof(pullTrace *), _PULL_TRACE_MULTI_INCR);
    airArrayPointerCB(ret->traceArr,
                      NULL, /* because we get handed pullTrace structs
                               that have already been allocated
                               (and then we own them) */
                      (void *(*)(void *))pullTraceNix);
  }
  return ret;
}

int /* Biff: 1 */
pullTraceMultiAdd(pullTraceMulti *mtrc, pullTrace *trc, int *addedP) {
  static const char me[] = "pullTraceMultiAdd";
  unsigned int indx;

  if (!(mtrc && trc && addedP)) {
    biffAddf(PULL, "%s: got NULL pointer", me);
    return 1;
  }
  if (!(trc->nvert->data && trc->nvert->axis[1].size >= 3)) {
    /*  for now getting a stub trace is not an error
    biffAddf(PULL, "%s: got stub trace", me);
    return 1; */
    *addedP = AIR_FALSE;
    return 0;
  }
  if (!(trc->nstab->data && trc->nstab->axis[1].size == trc->nvert->axis[1].size)) {
    biffAddf(PULL, "%s: stab data inconsistent", me);
    return 1;
  }
  *addedP = AIR_TRUE;
  indx = airArrayLenIncr(mtrc->traceArr, 1);
  if (!mtrc->trace) {
    biffAddf(PULL, "%s: alloc error", me);
    return 1;
  }
  mtrc->trace[indx] = trc;
  return 0;
}

int /* Biff: 1 */
pullTraceMultiPlotAdd(Nrrd *nplot, const pullTraceMulti *mtrc, const Nrrd *nfilt,
                      int strengthUse, int smooth, int flatWght, unsigned int trcIdxMin,
                      unsigned int trcNum, Nrrd *nmaskedpos, const Nrrd *nmask) {
  static const char me[] = "pullTraceMultiPlotAdd";
  double ssRange[2], vRange[2], *plot;
  unsigned int sizeS, sizeT, trcIdx, trcIdxMax;
  int *filt;
  airArray *mop;
  Nrrd *nsmst; /* smoothed stability */
  airArray *mposArr;
  double *mpos;
  unsigned char *mask;

  if (!(nplot && mtrc)) {
    biffAddf(PULL, "%s: got NULL pointer", me);
    return 1;
  }
  if (nrrdCheck(nplot)) {
    biffMovef(PULL, NRRD, "%s: trouble with nplot", me);
    return 1;
  }
  if (nfilt) {
    if (nrrdCheck(nfilt)) {
      biffMovef(PULL, NRRD, "%s: trouble with nfilt", me);
      return 1;
    }
    if (!(1 == nfilt->dim && nrrdTypeInt == nfilt->type)) {
      biffAddf(PULL, "%s: didn't get 1-D array of %s (got %u-D of %s)", me,
               airEnumStr(nrrdType, nrrdTypeInt), nfilt->dim,
               airEnumStr(nrrdType, nfilt->type));
      return 1;
    }
  }
  if (!(2 == nplot->dim && nrrdTypeDouble == nplot->type)) {
    biffAddf(PULL, "%s: didn't get 2-D array of %s (got %u-D of %s)", me,
             airEnumStr(nrrdType, nrrdTypeDouble), nplot->dim,
             airEnumStr(nrrdType, nplot->type));
    return 1;
  }
  if (!(trcIdxMin < mtrc->traceNum)) {
    biffAddf(PULL, "%s: trcIdxMin %u not < traceNum %u", me, trcIdxMin, mtrc->traceNum);
    return 1;
  }
  if (trcNum) {
    trcIdxMax = trcIdxMin + trcNum - 1;
    if (!(trcIdxMax < mtrc->traceNum)) {
      biffAddf(PULL, "%s: trcIdxMax %u = %u+%u-1 not < traceNum %u", me, trcIdxMax,
               trcIdxMin, trcNum, mtrc->traceNum);
      return 1;
    }
  } else {
    trcIdxMax = mtrc->traceNum - 1;
  }
  if (nmaskedpos || nmask) {
    if (!(nmaskedpos && nmask)) {
      biffAddf(PULL,
               "%s: need either both or neither of nmaskedpos (%p)"
               "and nmask (%p)",
               me, (void *)nmaskedpos, (const void *)nmask);
      return 1;
    }
    if (!(2 == nmask->dim && nrrdTypeUChar == nmask->type
          && nplot->axis[0].size == nmask->axis[0].size
          && nplot->axis[1].size == nmask->axis[1].size)) {
      biffAddf(PULL,
               "%s: got trace mask but wanted "
               "2-D %s %u-by-%u (not %u-D %s %u-by-%u)\n",
               me, airEnumStr(nrrdType, nrrdTypeUChar), AIR_UINT(nplot->axis[0].size),
               AIR_UINT(nplot->axis[1].size), nmask->dim,
               airEnumStr(nrrdType, nmask->type), AIR_UINT(nmask->axis[0].size),
               AIR_UINT(nmask->axis[1].size));
      return 1;
    }
    mask = AIR_CAST(unsigned char *, nmask->data);
  } else {
    mask = NULL;
  }

  ssRange[0] = nplot->axis[0].min;
  ssRange[1] = nplot->axis[0].max;
  vRange[0] = nplot->axis[1].min;
  vRange[1] = nplot->axis[1].max;
  if (!(AIR_EXISTS(ssRange[0]) && AIR_EXISTS(ssRange[1]) && AIR_EXISTS(vRange[0])
        && AIR_EXISTS(vRange[1]))) {
    biffAddf(PULL, "%s: need both axis 0 (%g,%g) and 1 (%g,%g) min,max", me, ssRange[0],
             ssRange[1], vRange[0], vRange[1]);
    return 1;
  }
  if (1 != vRange[0]) {
    biffAddf(PULL, "%s: expected vRange[0] == 1 not %g", me, vRange[0]);
    return 1;
  }
  mop = airMopNew();

  mpos = NULL;
  if (nmaskedpos && nmask) {
    nrrdEmpty(nmaskedpos);
    mposArr = airArrayNew((void **)(&mpos), NULL, 4 * sizeof(double), 512 /* HEY */);
  } else {
    mposArr = NULL;
  }

  nsmst = nrrdNew();
  airMopAdd(mop, nsmst, (airMopper)nrrdNuke, airMopAlways);
  plot = AIR_CAST(double *, nplot->data);
  filt = (nfilt ? AIR_CAST(int *, nfilt->data) : NULL);
  sizeS = AIR_UINT(nplot->axis[0].size);
  sizeT = AIR_UINT(nplot->axis[1].size);
  for (trcIdx = trcIdxMin; trcIdx <= trcIdxMax; trcIdx++) {
    int pntIdx, pntNum;
    const pullTrace *trc;
    const double *vert, *stab, *strn;
    unsigned int maskInCount = 0;       /* to quiet maybe-used-uninit warning */
    double maskInPos[4] = {0, 0, 0, 0}; /* to quiet maybe-used-uninit warning */

    if (filt && !filt[trcIdx]) {
      continue;
    }
    trc = mtrc->trace[trcIdx];
    if (pullTraceStopStub == trc->whyNowhere) {
      continue;
    }
    if (strengthUse && !(trc->nstrn && trc->nstrn->data)) {
      biffAddf(PULL,
               "%s: requesting strength-based weighting, but don't have "
               "strength info in trace %u",
               me, trcIdx);
      airMopError(mop);
      return 1;
    }
    pntNum = AIR_INT(trc->nvert->axis[1].size);
    vert = AIR_CAST(double *, trc->nvert->data);
    stab = AIR_CAST(double *, trc->nstab->data);
    if (smooth > 0) {
      double *smst;
      if (nrrdCopy(nsmst, trc->nstab)) {
        biffMovef(PULL, NRRD, "%s: trouble w/ trace %u", me, trcIdx);
        airMopError(mop);
        return 1;
      }
      smst = AIR_CAST(double *, nsmst->data);
      for (pntIdx = 0; pntIdx < pntNum; pntIdx++) {
        int ii, jj;
        double ss, ww, ws;
        ss = ws = 0;
        for (jj = -smooth; jj <= smooth; jj++) {
          ii = pntIdx + jj;
          ii = AIR_CLAMP(0, ii, pntNum - 1);
          ww = nrrdKernelBSpline3->eval1_d(AIR_AFFINE(-smooth - 1, jj, smooth + 1, -2,
                                                      2),
                                           NULL);
          ws += ww;
          ss += ww * stab[0 + 2 * ii] * stab[1 + 2 * ii];
        }
        smst[pntIdx] = ss / ws;
      }
      /* now redirect stab */
      stab = smst;
    }
    strn = AIR_CAST(double *, (strengthUse && trc->nstrn ? trc->nstrn->data : NULL));
    /* would be nice to get some graphical indication of this */
    fprintf(stderr, "!%s: trace %u in [%u,%u]: %u points; stops = %s(%s) | %s(%s)\n", me,
            trcIdx, trcIdxMin, trcIdxMax, pntNum,
            airEnumStr(pullTraceStop, trc->whyStop[0]),
            (pullTraceStopConstrFail == trc->whyStop[0]
               ? airEnumStr(pullConstraintFail, trc->whyStopCFail[0])
               : ""),
            airEnumStr(pullTraceStop, trc->whyStop[1]),
            (pullTraceStopConstrFail == trc->whyStop[1]
               ? airEnumStr(pullConstraintFail, trc->whyStopCFail[1])
               : ""));
    /* */

    if (mask) {
      maskInCount = 0;
      ELL_4V_SET(maskInPos, 0, 0, 0, 0);
    }
    for (pntIdx = 0; pntIdx < pntNum; pntIdx++) {
      const double *pp;
      double add, ww;
      unsigned int sidx, vidx;
      pp = vert + 4 * pntIdx;
      if (!(AIR_IN_OP(ssRange[0], pp[3], ssRange[1]))) {
        continue;
      }
      if (flatWght > 0) {
        if (!pntIdx || pntIdx == pntNum - 1) {
          continue;
        }
      } else if (flatWght < 0) {
        /* HACK: only show the seed point */
        if (AIR_UINT(pntIdx) != trc->seedIdx) {
          continue;
        }
      }
      sidx = airIndex(ssRange[0], pp[3], ssRange[1], sizeS);
      vidx = airIndexClamp(1, stab[0 + 2 * pntIdx] * stab[1 + 2 * pntIdx], 0, sizeT);
      add = strn ? strn[pntIdx] : 1;
      if (flatWght > 0) {
        double dx = (((vert + 4 * (pntIdx + 1))[3] - (vert + 4 * (pntIdx - 1))[3])
                     / (ssRange[1] - ssRange[0]));
        /*
        double dx = ( ((vert + 4*(pntIdx+1))[3] - pp[3])
                      / (ssRange[1] - ssRange[0]) );
        */
        double dy = (stab[0 + 2 * (pntIdx + 1)] * stab[1 + 2 * (pntIdx + 1)]
                     - stab[0 + 2 * (pntIdx - 1)] * stab[1 + 2 * (pntIdx - 1)]);
        ww = dx / sqrt(dx * dx + dy * dy);
      } else {
        ww = 1;
      }
      plot[sidx + sizeS * vidx] += AIR_MAX(0, ww * add);
      if (mask && mask[sidx + sizeS * vidx] > 200) {
        ELL_4V_ADD2(maskInPos, maskInPos, pp);
        maskInCount++;
      }
    }
    if (mask && maskInCount) {
      unsigned int mpi = airArrayLenIncr(mposArr, 1);
      ELL_4V_SCALE(mpos + 4 * mpi, 1.0 / maskInCount, maskInPos);
    }
  }
  if (mask && mposArr->len) {
    if (nrrdMaybeAlloc_va(nmaskedpos, nrrdTypeDouble, 2, AIR_SIZE_T(4),
                          AIR_SIZE_T(mposArr->len))) {
      biffAddf(PULL, "%s: couldn't allocate masked pos", me);
      airMopError(mop);
      return 1;
    }
    memcpy(nmaskedpos->data, mposArr->data, 4 * (mposArr->len) * sizeof(double));
  }
  airMopOkay(mop);
  return 0;
}

static size_t
nsizeof(const Nrrd *nrrd) {
  return (nrrd ? nrrdElementSize(nrrd) * nrrdElementNumber(nrrd) : 0);
}

size_t /* Biff: nope */
pullTraceMultiSizeof(const pullTraceMulti *mtrc) {
  size_t ret;
  unsigned int ti;

  if (!mtrc) {
    return 0;
  }
  ret = 0;
  for (ti = 0; ti < mtrc->traceNum; ti++) {
    ret += sizeof(pullTrace);
    ret += nsizeof(mtrc->trace[ti]->nvert);
    ret += nsizeof(mtrc->trace[ti]->nstrn);
    ret += nsizeof(mtrc->trace[ti]->nstab);
  }
  ret += sizeof(pullTrace *) * (mtrc->traceArr->size);
  return ret;
}

pullTraceMulti * /* Biff: nope */
pullTraceMultiNix(pullTraceMulti *mtrc) {

  if (mtrc) {
    airArrayNuke(mtrc->traceArr);
    free(mtrc);
  }
  return NULL;
}

#define PULL_MTRC_MAGIC "PULLMTRC0001"
#define DEMARK_STR      "======"

static int /* Biff: 1 */
tracewrite(FILE *file, const pullTrace *trc, unsigned int ti) {
  static const char me[] = "tracewrite";

  /*
  this was used to get ascii coordinates for a trace,
  to help isolate (via emacs) one trace from a saved multi-trace
  NrrdIoState *nio = nrrdIoStateNew();
  nio->encoding = nrrdEncodingAscii;
  */

  fprintf(file, "%s %u\n", DEMARK_STR, ti);
  ell_4v_print_d(file, trc->seedPos);
#define WRITE(FF)                                                                       \
  if (trc->FF && trc->FF->data) {                                                       \
    if (nrrdWrite(file, trc->FF, NULL /* nio */)) {                                     \
      biffMovef(PULL, NRRD, "%s: trouble with " #FF, me);                               \
      return 1;                                                                         \
    }                                                                                   \
  } else {                                                                              \
    fprintf(file, "NULL");                                                              \
  }                                                                                     \
  fprintf(file, "\n")

  fprintf(file, "nrrds: vert strn stab = %d %d %d\n", trc->nvert && trc->nvert->data,
          trc->nstrn && trc->nstrn->data, trc->nstab && trc->nstab->data);
  WRITE(nvert);
  WRITE(nstrn);
  WRITE(nstab);
  fprintf(file, "%u\n", trc->seedIdx);
  fprintf(file, "%s %s %s\n", airEnumStr(pullTraceStop, trc->whyStop[0]),
          airEnumStr(pullTraceStop, trc->whyStop[1]),
          airEnumStr(pullTraceStop, trc->whyNowhere));
#undef WRITE
  return 0;
}

int /* Biff: 1 */
pullTraceMultiWrite(FILE *file, const pullTraceMulti *mtrc) {
  static const char me[] = "pullTraceMultiWrite";
  unsigned int ti;

  if (!(file && mtrc)) {
    biffAddf(PULL, "%s: got NULL pointer", me);
    return 1;
  }
  fprintf(file, "%s\n", PULL_MTRC_MAGIC);
  fprintf(file, "%u traces\n", mtrc->traceNum);

  for (ti = 0; ti < mtrc->traceNum; ti++) {
    if (tracewrite(file, mtrc->trace[ti], ti)) {
      biffAddf(PULL, "%s: trace %u/%u", me, ti, mtrc->traceNum);
      return 1;
    }
  }
  return 0;
}

static int /* Biff: 1 */
traceread(pullTrace *trc, FILE *file, unsigned int _ti) {
  static const char me[] = "traceread";
  char line[AIR_STRLEN_MED + 1], name[AIR_STRLEN_MED + 1];
  unsigned int ti, lineLen;
  int stops[3], hackhack, vertHN, strnHN, stabHN; /* HN == have nrrd */

  sprintf(name, "separator");
  lineLen = airOneLine(file, line, AIR_STRLEN_MED + 1);
  if (!lineLen) {
    biffAddf(PULL, "%s: didn't get %s line", me, name);
    return 1;
  }
  if (1 != sscanf(line, DEMARK_STR " %u", &ti)) {
    biffAddf(PULL, "%s: \"%s\" doesn't look like %s line", me, line, name);
    return 1;
  }
  if (ti != _ti) {
    biffAddf(PULL, "%s: read trace index %u but wanted %u", me, ti, _ti);
    return 1;
  }
  sprintf(name, "seed pos");
  lineLen = airOneLine(file, line, AIR_STRLEN_MED + 1);
  if (!lineLen) {
    biffAddf(PULL, "%s: didn't get %s line", me, name);
    return 1;
  }
  if (4
      != sscanf(line, "%lg %lg %lg %lg", trc->seedPos + 0, trc->seedPos + 1,
                trc->seedPos + 2, trc->seedPos + 3)) {
    biffAddf(PULL, "%s: couldn't parse %s line \"%s\" as 4 doubles", me, name, line);
    return 1;
  }
  sprintf(name, "have nrrds");
  lineLen = airOneLine(file, line, AIR_STRLEN_MED + 1);
  if (!lineLen) {
    biffAddf(PULL, "%s: didn't get %s line", me, name);
    return 1;
  }
  if (3 != sscanf(line, "nrrds: vert strn stab = %d %d %d", &vertHN, &strnHN, &stabHN)) {
    biffAddf(PULL, "%s: couldn't parse %s line", me, name);
    return 1;
  }
#define READ(FF)                                                                        \
  if (FF##HN) {                                                                         \
    if (nrrdRead(trc->n##FF, file, NULL)) {                                             \
      biffMovef(PULL, NRRD, "%s: trouble with " #FF, me);                               \
      return 1;                                                                         \
    }                                                                                   \
    fgetc(file);                                                                        \
  } else {                                                                              \
    airOneLine(file, line, AIR_STRLEN_MED + 1);                                         \
  }
  hackhack = nrrdStateVerboseIO; /* should be fixed in Teem v2 */
  nrrdStateVerboseIO = 0;
  READ(vert);
  READ(strn);
  READ(stab);
  nrrdStateVerboseIO = hackhack;

  sprintf(name, "seed idx");
  lineLen = airOneLine(file, line, AIR_STRLEN_MED + 1);
  if (!lineLen) {
    biffAddf(PULL, "%s: didn't get %s line", me, name);
    return 1;
  }
  if (1 != sscanf(line, "%u", &(trc->seedIdx))) {
    biffAddf(PULL, "%s: didn't parse uint from %s line \"%s\"", me, name, line);
    return 1;
  }
  sprintf(name, "stops");
  lineLen = airOneLine(file, line, AIR_STRLEN_MED + 1);
  if (!lineLen) {
    biffAddf(PULL, "%s: didn't get %s line", me, name);
    return 1;
  }
  if (3 != airParseStrE(stops, line, " ", 3, pullTraceStop)) {
    biffAddf(PULL, "%s: didn't see 3 %s on %s line \"%s\"", me, pullTraceStop->name,
             name, line);
    return 1;
  }

  return 0;
}
int /* Biff: 1 */
pullTraceMultiRead(pullTraceMulti *mtrc, FILE *file) {
  static const char me[] = "pullTraceMultiRead";
  char line[AIR_STRLEN_MED + 1], name[AIR_STRLEN_MED + 1];
  unsigned int lineLen, ti, tnum;
  pullTrace *trc;

  if (!(mtrc && file)) {
    biffAddf(PULL, "%s: got NULL pointer", me);
    return 1;
  }
  airArrayLenSet(mtrc->traceArr, 0);
  sprintf(name, "magic");
  lineLen = airOneLine(file, line, AIR_STRLEN_MED + 1);
  if (!lineLen) {
    biffAddf(PULL, "%s: didn't get %s line", me, name);
    return 1;
  }
  if (strcmp(line, PULL_MTRC_MAGIC)) {
    biffAddf(PULL, "%s: %s line \"%s\" not expected \"%s\"", me, name, line,
             PULL_MTRC_MAGIC);
    return 1;
  }

  sprintf(name, "# of traces");
  lineLen = airOneLine(file, line, AIR_STRLEN_MED + 1);
  if (!lineLen) {
    biffAddf(PULL, "%s: didn't get %s line", me, name);
    return 1;
  }
  if (1 != sscanf(line, "%u traces", &tnum)) {
    biffAddf(PULL, "%s: \"%s\" doesn't look like %s line", me, line, name);
    return 1;
  }
  for (ti = 0; ti < tnum; ti++) {
    int added;
    trc = pullTraceNew();
    if (traceread(trc, file, ti)) {
      biffAddf(PULL, "%s: on trace %u/%u", me, ti, tnum);
      return 1;
    }
    if (pullTraceMultiAdd(mtrc, trc, &added)) {
      biffAddf(PULL, "%s: adding trace %u/%u", me, ti, tnum);
      return 1;
    }
    if (!added) {
      trc = pullTraceNix(trc);
    }
  }

  return 0;
}
