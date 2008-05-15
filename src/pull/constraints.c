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

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "pull.h"
#include "privatePull.h"

/* NOTE: this assumes variables "iter" (uint) and "me" (char*) */
#define NORMALIZE(dir, grad, len)                                        \
  ELL_3V_NORM((dir), (grad), (len));                                     \
  if (!(len)) {                                                          \
    sprintf(err, "%s: got zero grad at (%g,%g,%g,%g) on iter %u\n", me,  \
            point->pos[0], point->pos[1], point->pos[2],                 \
            point->pos[3], iter);                                        \
    biffAdd(PULL, err); return 1;                                        \
  }



/* ------------------------------- isosurface */



#define PROBE(v, av, g)  if (_pullProbe(task, point)) {        \
      sprintf(err, "%s: on iter %u", me, iter);                \
      biffAdd(PULL, err); return 1;                            \
    }                                                          \
    (v) = _pullPointScalar(task->pctx, point,                  \
                           pullInfoIsovalue, (g), NULL);       \
    (av) = AIR_ABS(v)
#define SAVE(state, aval, val, grad, pos)     \
  state[0] = aval;                             \
  state[1] = val;                              \
  ELL_3V_COPY(state + 1 + 1, grad);            \
  ELL_3V_COPY(state + 1 + 1 + 3, pos)
#define RESTORE(aval, val, grad, pos, state)      \
  aval = state[0];                                \
  val = state[1];                              \
  ELL_3V_COPY(grad, state + 1 + 1);            \
  ELL_3V_COPY(pos, state + 1 + 1 + 3)

static int
constraintSatIso(pullTask *task, pullPoint *point,
                 double stepMax, unsigned int iterMax,
                 /* output */
                 int *constrFailP) {
  char me[]="constraintSatIso", err[BIFF_STRLEN];
  double 
    step,         /* current step size */
    val, aval, /* last and current function values */
    hack,         /* how to control re-tries in the context of a single
                     for-loop, instead of a nested do-while loop */
    grad[4], dir[3], len, state[1 + 1 + 3 + 3];
  unsigned int iter = 0;  /* 0: initial probe, 1..iterMax: probes in loop */

  PROBE(val, aval, grad);
  SAVE(state, aval, val, grad, point->pos);
  hack = 1;
  for (iter=1; iter<=iterMax; iter++) {
    /* consider? http://en.wikipedia.org/wiki/Halley%27s_method */
    NORMALIZE(dir, grad, len);
    step = -val/len; /* the newton-raphson step */
    step = step > 0 ? AIR_MIN(stepMax, step) : AIR_MAX(-stepMax, step);
    ELL_3V_SCALE_INCR(point->pos, hack*step, dir);
    _pullPointHistAdd(point, pullCondConstraintSatA);
    PROBE(val, aval, grad);
    if (aval <= state[0]) {  /* we're no further from the root */
      if (AIR_ABS(step) < stepMax*task->pctx->constraintStepMin) {
        /* we have converged! */
        break;
      }
      SAVE(state, aval, val, grad, point->pos);
      hack = 1;
    } else { /* oops, try again, don't update dir or len, reset val */
      hack *= task->pctx->stepScale;
      RESTORE(aval, val, grad, point->pos, state);
    }
  }
  if (iter > iterMax) {
    *constrFailP = AIR_TRUE;
  } else {
    *constrFailP = AIR_FALSE;
  }
  return 0;
}

#undef PROBE
#undef SAVE
#undef RESTORE



/* ------------------------------- laplacian */



#define PROBE(l)  if (_pullProbe(task, point)) {                   \
      sprintf(err, "%s: on iter %u", me, iter);                    \
      biffAdd(PULL, err); return 1;                                \
    }                                                              \
    (l) = _pullPointScalar(task->pctx, point,                      \
                           pullInfoHeightLaplacian, NULL, NULL);
#define PROBEG(l, g) \
    PROBE(l);                                                      \
    _pullPointScalar(task->pctx, point, pullInfoHeight, (g), NULL);
  
static int
constraintSatLapl(pullTask *task, pullPoint *point,
                  double stepMax, unsigned int iterMax,
                  /* output */
                  int *constrFailP) {
  char me[]="constraintSatLapl", err[BIFF_STRLEN];
  double 
    step,         /* current step size */
    valLast, val, /* last and current function values */
    grad[4], dir[3], len, 
    posOld[3], posNew[3], tmpv[3];
  double a=0, b=1, s, fa, fb, fs, tmp, diff;
  int side = 0;
  unsigned int iter = 0;  /* 0: initial probe, 1..iterMax: probes in loop */

  step = stepMax/2;
  PROBEG(val, grad);
  if (0 == val) {
    /* already exactly at the zero, we're done. This actually happens! */
    fprintf(stderr, "!%s: a lapl == 0!\n", me);
    return 0;
  }
  valLast = val;
  NORMALIZE(dir, grad, len);
  /* first phase: follow normalized gradient until laplacian sign change */
  for (iter=1; iter<=iterMax; iter++) {
    double sgn;
    ELL_3V_COPY(posOld, point->pos);
    sgn = airSgn(val); /* lapl < 0 => downhill; lapl > 0 => uphill */
    ELL_3V_SCALE_INCR(point->pos, sgn*step, dir);
    _pullPointHistAdd(point, pullCondConstraintSatA);
    PROBEG(val, grad);
    if (val*valLast < 0) {
      /* laplacian has changed sign; stop looking */
      break;
    }
    valLast = val;
    NORMALIZE(dir, grad, len);
  }
  if (iter > iterMax) {
    *constrFailP = AIR_TRUE;
    return 0;
  }
  /* second phase: find the zero-crossing, looking between
     f(posOld)=valLast and f(posNew)=val */
  ELL_3V_COPY(posNew, point->pos);
  ELL_3V_SUB(tmpv, posNew, posOld);
  len = ELL_3V_LEN(tmpv);
  fa = valLast;
  fb = val;
  if (AIR_ABS(fa) < AIR_ABS(fb)) {
    ELL_SWAP2(a, b, tmp); ELL_SWAP2(fa, fb, tmp);
  }
  for (iter=1; iter<=iterMax; iter++) {
    s = AIR_AFFINE(fa, 0, fb, a, b);
    ELL_3V_LERP(point->pos, s, posOld, posNew);
    _pullPointHistAdd(point, pullCondConstraintSatB);
    PROBE(fs);
    if (0 == fs) {
      /* exactly nailed the zero, we're done. This actually happens! */
      fprintf(stderr, "!%s: b lapl == 0!\n", me);
      break;
    }
    /* "Illinois" false-position.  Look, it works. */
    if (fs*fb > 0) { /* not between s and b */
      b = s;
      fb = fs;
      if (+1 == side) {
        fa /= 2;
      }
      side = +1;
    } else { /* not between a and s */
      a = s;
      fa = fs;
      if (-1 == side) {
        fb /= 2;
      }
      side = -1;
    }
    diff = (b - a)*len;
    if (AIR_ABS(diff) < stepMax*task->pctx->constraintStepMin) {
      /* converged! */
      break;
    }
  }
  if (iter > iterMax) {
    *constrFailP = AIR_TRUE;
  } else {
    *constrFailP = AIR_FALSE;
  }
  return 0;
}
#undef PROBE
#undef PROBEG


/* ------------------------------------------- height (line xor surf) */

#define MODEWARP(m, p)                          \
  (m > 0                                        \
   ? 1 - airIntPow(1-m, p)                      \
   : airIntPow(m+1, p) - 1)

static int
probeHeight(pullTask *task, pullPoint *point, int tang2Use, int useMode,
            /* output */
            double *heightP, double grad[3], double hess[9], double proj[9]) {
  char me[]="probeHeight", err[BIFF_STRLEN];
  double *tng;

  if (_pullProbe(task, point)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(PULL, err); return 1;
  }                             
  *heightP = _pullPointScalar(task->pctx, point, pullInfoHeight, grad, hess);
  tng = point->info + task->pctx->infoIdx[pullInfoTangent1];
  ELL_3MV_OUTER(proj, tng, tng);
  if (tang2Use) {
    double proj2[9];
    tng = point->info + task->pctx->infoIdx[pullInfoTangent2];
    ELL_3MV_OUTER(proj2, tng, tng);
    if (useMode) {
      double mode;
      ELL_3M_ADD2(proj2, proj, proj2);
      mode = _pullPointScalar(task->pctx, point, pullInfoTangentMode,
                              NULL, NULL);
      mode = MODEWARP(mode, 8);
      mode = AIR_AFFINE(-1, mode, 1, 0, 1);
      ELL_3M_LERP(proj, mode, proj, proj2);
    } else {
      ELL_3M_ADD2(proj, proj, proj2);
    }
  }
  return 0;
}

#define PROBE(height, grad, hess, proj)                 \
  if (probeHeight(task, point, tang2Use, modeUse,       \
                  &(height), (grad), (hess), (proj))) { \
    sprintf(err, "%s: trouble on iter %u", me, iter);   \
    biffAdd(PULL, err); return 1;                       \
  }
#define SAVE(state, height, grad, hess, proj, pos) \
  state[0] = height;                               \
  ELL_3V_COPY(state + 1, grad);                    \
  ELL_3M_COPY(state + 1 + 3, hess);                \
  ELL_3M_COPY(state + 1 + 3 + 9, proj);            \
  ELL_3V_COPY(state + 1 + 3 + 9 + 9, pos)
#define RESTORE(height, grad, hess, proj, pos, state)   \
  height = state[0];                                    \
  ELL_3V_COPY(grad, state + 1);                         \
  ELL_3M_COPY(hess, state + 1 + 3);                     \
  ELL_3M_COPY(proj, state + 1 + 3 + 9);                 \
  ELL_3V_COPY(pos, state + 1 + 3 + 9 + 9)
#define NORM(d1, d2, pdir, plen, pgrad, grad, hess, proj)             \
  ELL_3MV_MUL(pgrad, proj, grad);                                     \
  ELL_3V_NORM(pdir, pgrad, plen);                                     \
  if (!(plen)) {                                                      \
    sprintf(err, "%s: got 0 pgrad at (%g,%g,%g,%g) on iter %u\n", me, \
            point->pos[0], point->pos[1], point->pos[2],              \
            point->pos[3], iter);                                     \
    biffAdd(PULL, err); return 1;                                     \
  }                                                                   \
  d1 = ELL_3V_DOT(grad, pdir);                                        \
  d2 = ELL_3MV_CONTR(hess, pdir)

double _tmpv[3];

static int
constraintSatHght(pullTask *task, pullPoint *point, int tang2Use, int modeUse,
                  double stepMax, unsigned int iterMax,
                  int *constrFailP) {
  char me[]="constraintSatHght", err[BIFF_STRLEN];
  double val, grad[3], hess[9], proj[9],
    state[1+3+9+9+3], hack, step,
    d1, d2, pdir[3], plen, pgrad[3];
  unsigned int iter = 0;  /* 0: initial probe, 1..iterMax: probes in loop */
  /* http://en.wikipedia.org/wiki/Newton%27s_method_in_optimization */

  _pullPointHistAdd(point, pullCondOld);
  PROBE(val, grad, hess, proj);
  SAVE(state, val, grad, hess, proj, point->pos);
  hack = 1;
  for (iter=1; iter<=iterMax; iter++) {
    NORM(d1, d2, pdir, plen, pgrad, grad, hess, proj);
    step = (d2 <= 0 ? -plen : -d1/d2);
    /*
    fprintf(stderr, "!%s: iter %u step = (%g <= 0 ? %g : %g) --> %g\n", me,
            iter, d2, -plen, -d1/d2, step);
    */
    step = step > 0 ? AIR_MIN(stepMax, step) : AIR_MAX(-stepMax, step);
    /*
    fprintf(stderr, "       -> %g, |pdir| = %g\n", step, ELL_3V_LEN(pdir));
    */
    ELL_3V_COPY(_tmpv, point->pos);
    ELL_3V_SCALE_INCR(point->pos, hack*step, pdir);
    ELL_3V_SUB(_tmpv, _tmpv, point->pos);
    /*
    fprintf(stderr, "        -> moved %g\n", ELL_3V_LEN(_tmpv));
    */
    _pullPointHistAdd(point, pullCondConstraintSatA);
    PROBE(val, grad, hess, proj);
    if (val <= state[0]) {
      if (AIR_ABS(step) < stepMax*task->pctx->constraintStepMin) {
        /* we have converged! */
        break;
      }
      SAVE(state, val, grad, hess, proj, point->pos);
      hack = 1;
    } else { /* oops, try again */
      hack *= task->pctx->stepScale;
      RESTORE(val, grad, hess, proj, point->pos, state);
    }
  }
  if (iter > iterMax) {
    *constrFailP = AIR_TRUE;
  } else {
    *constrFailP = AIR_FALSE;
  }
  /*
  fprintf(stderr, "!%s: %d %s\n", me, *constrFailP, 
          *constrFailP ? "FAILED!" : "ok");
          */
  return 0;
}
#undef PROBE
#undef NORM
#undef SAVE
#undef RESTORE

/* ------------------------------------------- */


/* HEY: have to make sure that scale position point->pos[3] 
** is not modified anywhere in here: constraints are ONLY spatial
*/
int
_pullConstraintSatisfy(pullTask *task, pullPoint *point, 
                       /* output */
                       int *constrFailP) {
  char me[]="_pullConstraintSatisfy", err[BIFF_STRLEN];
  double stepMax;
  unsigned int iterMax;
  
  stepMax = task->pctx->constraintVoxelSize;
  iterMax = task->pctx->constraintIterMax;
  /*
  fprintf(stderr, "!%s(%d): hi %g %g %g, stepMax = %g, iterMax = %u\n",
          me, point->idtag, point->pos[0], point->pos[1], point->pos[2],
          stepMax, iterMax);
  */
  switch (task->pctx->constraint) {
  case pullInfoHeightLaplacian: /* zero-crossing edges */
    if (constraintSatLapl(task, point, stepMax/4, 4*iterMax, constrFailP)) {
      sprintf(err, "%s: trouble", me);
      biffAdd(PULL, err); return 1;
    }
    break;
  case pullInfoIsovalue:
    if (constraintSatIso(task, point, stepMax, iterMax, constrFailP)) {
      sprintf(err, "%s: trouble", me);
      biffAdd(PULL, err); return 1;
    }
    break;
  case pullInfoHeight:
    if (constraintSatHght(task, point,
                          (task->pctx->ispec[pullInfoTangent2]
                           ? AIR_TRUE
                           : AIR_FALSE),
                          (task->pctx->ispec[pullInfoTangentMode]
                           ? AIR_TRUE
                           : AIR_FALSE),
                          stepMax, iterMax, constrFailP)) {
      sprintf(err, "%s: trouble", me);
      biffAdd(PULL, err); return 1;
    }
    break;
  default:
    fprintf(stderr, "%s: constraint on %s (%d) unimplemented!!\n", me,
            airEnumStr(pullInfo, task->pctx->constraint),
            task->pctx->constraint);
  }
  if (*constrFailP) {
    fprintf(stderr, "!%s(%u) bye, fail = %d, %g %g %g\n", me,
            point->idtag, *constrFailP,
            point->pos[0], point->pos[1], point->pos[2]);
  }
  return 0;
}

#undef NORMALIZE
