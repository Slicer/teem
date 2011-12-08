/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2011, 2010, 2009  University of Chicago
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

#define PRAYING 0

/*
typedef struct {
  double val, absval, grad[3];
} stateIso;

static int
probeIso(pullTask *task, pullPoint *point, unsigned int iter, int cond,
         double pos[3],
         stateIso *state) {
  static const char me[]="probeIso";
  
  ELL_3V_COPY(point->pos, pos);  / * NB: not touching point->pos[3] * /
  _pullPointHistAdd(point, cond);
  if (_pullProbe(task, point)) {
    biffAddf(PULL, "%s: on iter %u", me, iter);
    return 1;
  }
  state->val = _pullPointScalar(task->pctx, point,
                                pullInfoIsovalue,
                                state->grad, NULL);
  state->absval = AIR_ABS(state->val);
  return 0;
}
*/

/* NOTE: this assumes variables "iter" (uint) and "me" (char*) */
#define NORMALIZE_ERR(dir, grad, len)                                    \
  ELL_3V_NORM((dir), (grad), (len));                                     \
  if (!(len)) {                                                          \
    biffAddf(PULL, "%s: got zero grad at (%g,%g,%g,%g) on iter %u\n", me,\
             point->pos[0], point->pos[1], point->pos[2],                \
             point->pos[3], iter);                                       \
    return 1;                                                            \
  }

#define NORMALIZE(dir, grad, len)                                        \
  ELL_3V_NORM((dir), (grad), (len));                                     \
  if (!(len)) {                                                          \
    ELL_3V_SET((dir), 0, 0, 0) ;                                         \
  }


/* ------------------------------- isosurface */



#define PROBE(v, av, g)  if (_pullProbe(task, point)) {        \
      biffAddf(PULL, "%s: on iter %u", me, iter);              \
      return 1;                                                \
    }                                                          \
    (v) = _pullPointScalar(task->pctx, point,                  \
                           pullInfoIsovalue, (g), NULL);       \
    (av) = AIR_ABS(v)
#define SAVE(state, aval, val, grad, pos)      \
  state[0] = aval;                             \
  state[1] = val;                              \
  ELL_3V_COPY(state + 1 + 1, grad);            \
  ELL_3V_COPY(state + 1 + 1 + 3, pos)
#define RESTORE(aval, val, grad, pos, state)   \
  aval = state[0];                             \
  val = state[1];                              \
  ELL_3V_COPY(grad, state + 1 + 1);            \
  ELL_3V_COPY(pos, state + 1 + 1 + 3)

static int
constraintSatIso(pullTask *task, pullPoint *point,
                 double stepMax, unsigned int iterMax,
                 /* output */
                 int *constrFailP) {
  static const char me[]="constraintSatIso";
  double 
    step,         /* current step size */
    val, aval,    /* last and current function values */
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
    if (!len) {
      /* no gradient; back off */
      hack *= task->pctx->sysParm.backStepScale;
      RESTORE(aval, val, grad, point->pos, state);
      continue;
    }
    step = -val/len; /* the newton-raphson step */
    step = step > 0 ? AIR_MIN(stepMax, step) : AIR_MAX(-stepMax, step);
    ELL_3V_SCALE_INCR(point->pos, hack*step, dir);
    _pullPointHistAdd(point, pullCondConstraintSatA);
    PROBE(val, aval, grad);
    if (aval <= state[0]) {  /* we're no further from the root */
      if (AIR_ABS(step) < stepMax*task->pctx->sysParm.constraintStepMin) {
        /* we have converged! */
        break;
      }
      SAVE(state, aval, val, grad, point->pos);
      hack = 1;
    } else { /* oops, try again, don't update dir or len, reset val */
      hack *= task->pctx->sysParm.backStepScale;
      RESTORE(aval, val, grad, point->pos, state);
    }
  }
  if (iter > iterMax) {
    *constrFailP = pullConstraintFailIterMaxed;
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
      biffAddf(PULL, "%s: on iter %u", me, iter);                  \
      return 1;                                                    \
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
  static const char me[]="constraintSatLapl";
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
    /* printf("!%s: a lapl == 0!\n", me); */
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
    *constrFailP = pullConstraintFailIterMaxed;
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
      printf("!%s: b lapl == 0!\n", me);
      break;
    }
    /* "Illinois" false-position. Dumb, but it works. */
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
    if (AIR_ABS(diff) < stepMax*task->pctx->sysParm.constraintStepMin) {
      /* converged! */
      break;
    }
  }
  if (iter > iterMax) {
    *constrFailP = pullConstraintFailIterMaxed;
  } else {
    *constrFailP = AIR_FALSE;
  }
  return 0;
}
#undef PROBE
#undef PROBEG


/* ------------------------------------------- height (line xor surf) */

static int
probeHeight(pullTask *task, pullPoint *point, 
            /* output */
            double *heightP, double grad[3], double hess[9]) {
  static const char me[]="probeHeight";

  if (_pullProbe(task, point)) {
    biffAddf(PULL, "%s: trouble", me);
    return 1;
  }                             
  *heightP = _pullPointScalar(task->pctx, point, pullInfoHeight, grad, hess);
  return 0;
}

/*
** creaseProj
**
** eigenvectors (with non-zero eigenvalues) of output proj are
** tangents to the directions along which particle is allowed to move
** *downward* (in height) for constraint satisfaction (according to
** tangent 1 and tangent2)
**
** negproj is the same, but for points moving upwards (according to
** negativetangent1 and negativetangent2)
*/
static void
creaseProj(pullTask *task, pullPoint *point,
           int tang1Use, int tang2Use,
           int negtang1Use, int negtang2Use,
           /* output */
           double posproj[9], double negproj[9]) {
#if PRAYING
  static const char me[]="creaseProj";
#endif
  double pp[9];
  double *tng;

  ELL_3M_ZERO_SET(posproj);
  if (tang1Use) {
    tng = point->info + task->pctx->infoIdx[pullInfoTangent1];
#if PRAYING
    fprintf(stderr, "!%s: tng1 = %g %g %g\n", me, tng[0], tng[1], tng[2]);
#endif
    ELL_3MV_OUTER(pp, tng, tng);
    ELL_3M_ADD2(posproj, posproj, pp);
  }
  if (tang2Use) {
    tng = point->info + task->pctx->infoIdx[pullInfoTangent2];
    ELL_3MV_OUTER(pp, tng, tng);
    ELL_3M_ADD2(posproj, posproj, pp);
  }

  ELL_3M_ZERO_SET(negproj);
  if (negtang1Use) {
    tng = point->info + task->pctx->infoIdx[pullInfoNegativeTangent1];
    ELL_3MV_OUTER(pp, tng, tng);
    ELL_3M_ADD2(negproj, negproj, pp);
  }
  if (negtang2Use) {
    tng = point->info + task->pctx->infoIdx[pullInfoNegativeTangent2];
    ELL_3MV_OUTER(pp, tng, tng);
    ELL_3M_ADD2(negproj, negproj, pp);
  }

  return;
}

/* HEY: body of probeHeight could really be expanded in here */
#define PROBE(height, grad, hess, posproj, negproj)             \
  if (probeHeight(task, point,                                  \
                  &(height), (grad), (hess))) {                 \
    biffAddf(PULL, "%s: trouble on iter %u", me, iter);         \
    return 1;                                                   \
  }                                                             \
  creaseProj(task, point, tang1Use, tang2Use,                   \
             negtang1Use, negtang2Use, posproj, negproj)
#define SAVE(state, height, grad, hess, posproj, negproj, pos)   \
  state[0] = height;                                             \
  ELL_3V_COPY(state + 1, grad);                                  \
  ELL_3M_COPY(state + 1 + 3, hess);                              \
  ELL_3M_COPY(state + 1 + 3 + 9, posproj);                       \
  ELL_3M_COPY(state + 1 + 3 + 9 + 9, negproj);                   \
  ELL_3V_COPY(state + 1 + 3 + 9 + 9 + 9, pos)
#define RESTORE(height, grad, hess, posproj, negproj, pos, state)   \
  height = state[0];                                                \
  ELL_3V_COPY(grad,    state + 1);                                  \
  ELL_3M_COPY(hess,    state + 1 + 3);                              \
  ELL_3M_COPY(posproj, state + 1 + 3 + 9);                          \
  ELL_3M_COPY(negproj, state + 1 + 3 + 9 + 9);                      \
  ELL_3V_COPY(pos,     state + 1 + 3 + 9 + 9 + 9)
#define POSNORM(d1, d2, pdir, plen, pgrad, grad, hess, posproj)       \
  ELL_3MV_MUL(pgrad, posproj, grad);                                  \
  ELL_3V_NORM(pdir, pgrad, plen);                                     \
  d1 = ELL_3V_DOT(grad, pdir);                                        \
  d2 = ELL_3MV_CONTR(hess, pdir)
#define NEGNORM(d1, d2, pdir, plen, pgrad, grad, hess, negproj)       \
  ELL_3MV_MUL(pgrad, negproj, grad);                                  \
  ELL_3V_NORM(pdir, pgrad, plen);                                     \
  d1 = -ELL_3V_DOT(grad, pdir);                                       \
  d2 = -ELL_3MV_CONTR(hess, pdir)
#define PRINT(prefix)                                                   \
  fprintf(stderr, "-------------- probe results %s:\n-- val = %g\n",    \
          prefix, val);                                                 \
  fprintf(stderr, "-- grad = %g %g %g\n", grad[0], grad[1], grad[2]);   \
  fprintf(stderr,"-- hess = %g %g %g;  %g %g %g;  %g %g %g\n",          \
          hess[0], hess[1], hess[2],                                    \
          hess[3], hess[4], hess[5],                                    \
          hess[6], hess[7], hess[8]);                                   \
  fprintf(stderr, "-- posproj = %g %g %g;  %g %g %g;  %g %g %g\n",      \
          posproj[0], posproj[1], posproj[2],                           \
          posproj[3], posproj[4], posproj[5],                           \
          posproj[6], posproj[7], posproj[8]);                          \
  fprintf(stderr, "-- negproj = %g %g %g;  %g %g %g;  %g %g %g\n",      \
          negproj[0], negproj[1], negproj[2],                           \
          negproj[3], negproj[4], negproj[5],                           \
          negproj[6], negproj[7], negproj[8])

static int
constraintSatHght(pullTask *task, pullPoint *point,
                  int tang1Use, int tang2Use,
                  int negtang1Use, int negtang2Use,
                  double stepMax, unsigned int iterMax,
                  int *constrFailP) {
  static const char me[]="constraintSatHght";
  double val, grad[3], hess[9], posproj[9], negproj[9],
    state[1+3+9+9+9+3], hack, step,
    d1, d2, pdir[3], plen, pgrad[3], stpmin;
#if PRAYING
  double _tmpv[3]={0,0,0};
#endif
  int havePos, haveNeg;
  unsigned int iter = 0;  /* 0: initial probe, 1..iterMax: probes in loop */
  /* http://en.wikipedia.org/wiki/Newton%27s_method_in_optimization */

  havePos = tang1Use || tang2Use;
  haveNeg = negtang1Use || negtang2Use;
  stpmin = task->pctx->voxelSizeSpace*task->pctx->sysParm.constraintStepMin;
#if PRAYING
  fprintf(stderr, "!%s(%u): starting at %g %g %g %g\n", me, point->idtag,
          point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
  fprintf(stderr, "!%s: pt %d %d nt %d %d stepMax %g, iterMax %u\n", me,
          tang1Use, tang2Use, negtang1Use, negtang2Use,
          stepMax, iterMax);
  fprintf(stderr, "!%s: stpmin = %g = voxsize %g * parm.stepmin %g\n", me,
          stpmin, task->pctx->voxelSizeSpace,
          task->pctx->sysParm.constraintStepMin);
#endif
  _pullPointHistAdd(point, pullCondOld);
  PROBE(val, grad, hess, posproj, negproj);
#if PRAYING
  PRINT("initial probe");
#endif
  SAVE(state, val, grad, hess, posproj, negproj, point->pos);
  hack = 1;
  for (iter=1; iter<=iterMax; iter++) {
#if PRAYING
    fprintf(stderr, "!%s: =============== begin iter %u\n", me, iter);
#endif
    /* HEY: no opportunistic increase of hack? */
    if (havePos) {
      POSNORM(d1, d2, pdir, plen, pgrad, grad, hess, posproj);
      if (!plen) {
        /* this use to be a biff error, which got to be annoying */
        *constrFailP = pullConstraintFailProjGradZeroA;
        return 0;
      }
      step = (d2 <= 0 ? -plen : -d1/d2);
#if PRAYING
      fprintf(stderr, "!%s: (+) iter %u step = (%g <= 0 ? %g : %g) --> %g\n",
              me, iter, d2, -plen, -d1/d2, step);
#endif
      step = step > 0 ? AIR_MIN(stepMax, step) : AIR_MAX(-stepMax, step);
      if (AIR_ABS(step) < stepMax*task->pctx->sysParm.constraintStepMin) {
        /* no further iteration needed; we're converged */
#if PRAYING
        fprintf(stderr, "     |step| %g < %g*%g = %g ==> converged!\n",
                AIR_ABS(step),
                stepMax, task->pctx->sysParm.constraintStepMin,
                stepMax*task->pctx->sysParm.constraintStepMin);
#endif
        if (!haveNeg) {
          break;
        } else {
          goto nextstep;
        }
      }
      /* else we have to take a significant step */
#if PRAYING
      fprintf(stderr, "       -> step %g, |pdir| = %g\n",
              step, ELL_3V_LEN(pdir));
      ELL_3V_COPY(_tmpv, point->pos);
      fprintf(stderr, "       ->  pos (%g,%g,%g,%g) += %g * %g * (%g,%g,%g)\n",
              point->pos[0], point->pos[1], point->pos[2], point->pos[3],
              hack, step, pdir[0], pdir[1], pdir[2]);
#endif
      ELL_3V_SCALE_INCR(point->pos, hack*step, pdir);
#if PRAYING
      ELL_3V_SUB(_tmpv, _tmpv, point->pos);
      fprintf(stderr, "       -> moved to %g %g %g %g\n", 
              point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
      fprintf(stderr, "       (moved %g)\n", ELL_3V_LEN(_tmpv));
#endif
      _pullPointHistAdd(point, pullCondConstraintSatA);
      PROBE(val, grad, hess, posproj, negproj);
#if PRAYING
      fprintf(stderr, "  (+) probed at (%g,%g,%g,%g)\n", 
              point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
      PRINT("after move");
      fprintf(stderr, "  val(%g,%g,%g,%g)=%g %s state[0]=%g\n", 
              point->pos[0], point->pos[1], point->pos[2], point->pos[3],
              val, val <= state[0] ? "<=" : ">", state[0]);
#endif
      if (val <= state[0]) {
        /* we made progress */
#if PRAYING
        fprintf(stderr, "  (+) progress!\n");
#endif
        SAVE(state, val, grad, hess, posproj, negproj, point->pos);
        hack = 1;
      } else { 
        /* oops, we went uphill instead of down; try again */
#if PRAYING
        fprintf(stderr, "  val *increased*; backing hack from %g to %g\n",
                hack, hack*task->pctx->sysParm.backStepScale);
#endif
        hack *= task->pctx->sysParm.backStepScale;
        RESTORE(val, grad, hess, posproj, negproj, point->pos, state);
#if PRAYING
        fprintf(stderr, "  restored to pos (%g,%g,%g,%g)\n", 
                point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
#endif
      }
    }
  nextstep:
    if (haveNeg) {
      /* HEY: copy and paste from above, minus fluff */
      NEGNORM(d1, d2, pdir, plen, pgrad, grad, hess, negproj);
      if (!plen && !haveNeg) {
        /* this use to be a biff error, which got to be annoying */
        *constrFailP = pullConstraintFailProjGradZeroA;
        return 0;
      }
      step = (d2 <= 0 ? -plen : -d1/d2);
#if PRAYING
      fprintf(stderr, "!%s: -+) iter %u step = (%g <= 0 ? %g : %g) --> %g\n",
              me, iter, d2, -plen, -d1/d2, step);
#endif
      step = step > 0 ? AIR_MIN(stepMax, step) : AIR_MAX(-stepMax, step);
      if (AIR_ABS(step) < stepMax*task->pctx->sysParm.constraintStepMin) {
#if PRAYING
        fprintf(stderr, "     |step| %g < %g*%g = %g ==> converged!\n",
                AIR_ABS(step),
                stepMax, task->pctx->sysParm.constraintStepMin,
                stepMax*task->pctx->sysParm.constraintStepMin);
#endif
        /* no further iteration needed; we're converged */
        break;
      }
      /* else we have to take a significant step */
#if PRAYING
      fprintf(stderr, "       -> step %g, |pdir| = %g\n",
              step, ELL_3V_LEN(pdir));
      ELL_3V_COPY(_tmpv, point->pos);
      fprintf(stderr, "       ->  pos (%g,%g,%g,%g) += %g * %g * (%g,%g,%g)\n",
              point->pos[0], point->pos[1], point->pos[2], point->pos[3],
              hack, step, pdir[0], pdir[1], pdir[2]);
#endif
      ELL_3V_SCALE_INCR(point->pos, hack*step, pdir);
#if PRAYING
      ELL_3V_SUB(_tmpv, _tmpv, point->pos);
      fprintf(stderr, "       -> moved to %g %g %g %g\n", 
              point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
      fprintf(stderr, "       (moved %g)\n", ELL_3V_LEN(_tmpv));
#endif
      _pullPointHistAdd(point, pullCondConstraintSatA);
      PROBE(val, grad, hess, posproj, negproj);
#if PRAYING
      fprintf(stderr, "  (-) probed at (%g,%g,%g,%g)\n", 
              point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
      PRINT("after move");
      fprintf(stderr, "  val(%g,%g,%g,%g)=%g %s state[0]=%g\n", 
              point->pos[0], point->pos[1], point->pos[2], point->pos[3],
              val, val >= state[0] ? ">=" : "<", state[0]);
#endif
      if (val >= state[0]) {
        /* we made progress */
#if PRAYING
        fprintf(stderr, "  (-) progress!\n");
#endif
        SAVE(state, val, grad, hess, posproj, negproj, point->pos);
        hack = 1;
      } else { 
        /* oops, we went uphill instead of down; try again */
#if PRAYING
        fprintf(stderr, "  val *increased*; backing hack from %g to %g\n",
                hack, hack*task->pctx->sysParm.backStepScale);
#endif
        hack *= task->pctx->sysParm.backStepScale;
        RESTORE(val, grad, hess, posproj, negproj, point->pos, state);
#if PRAYING
        fprintf(stderr, "  restored to pos (%g,%g,%g,%g)\n", 
                point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
#endif
      }
    }
  }
  if (iter > iterMax) {
    *constrFailP = pullConstraintFailIterMaxed;
  } else {
    *constrFailP = AIR_FALSE;
  }
  /*
  printf("!%s: %d %s\n", me, *constrFailP, 
         *constrFailP ? "FAILED!" : "ok");
          */
  return 0;
}
#undef PROBE
#undef POSNORM
#undef NEGNORM
#undef SAVE
#undef RESTORE

/* ------------------------------------------- */

/* HEY: have to make sure that scale position point->pos[3] 
** is not modified anywhere in here: constraints are ONLY spatial
**
** This uses biff, but only for showstopper problems
*/
int
_pullConstraintSatisfy(pullTask *task, pullPoint *point,
                       double travelMax,
                       /* output */
                       int *constrFailP) {
  static const char me[]="_pullConstraintSatisfy";
  double stepMax;
  unsigned int iterMax;
  double pos3Orig[3], pos3Diff[3], travel;
  
  ELL_3V_COPY(pos3Orig, point->pos);
  stepMax = task->pctx->voxelSizeSpace;
  iterMax = task->pctx->iterParm.constraintMax;
  /*
  dlim = _pullDistLimit(task, point);
  if (iterMax*stepMax > dlim) {
    stepMax = dlim/iterMax;
  }
  */
  /*
  fprintf(stderr, "!%s(%d): hi ==== %g %g %g, stepMax = %g, iterMax = %u\n",
          me, point->idtag, point->pos[0], point->pos[1], point->pos[2],
          stepMax, iterMax);
  */
  task->pctx->count[pullCountConstraintSatisfy] += 1;
  switch (task->pctx->constraint) {
  case pullInfoHeightLaplacian: /* zero-crossing edges */
    if (constraintSatLapl(task, point, stepMax/4, 4*iterMax, constrFailP)) {
      biffAddf(PULL, "%s: trouble", me);
      return 1;
    }
    break;
  case pullInfoIsovalue:
    if (constraintSatIso(task, point, stepMax, iterMax, constrFailP)) {
      biffAddf(PULL, "%s: trouble", me);
      return 1;
    }
    break;
  case pullInfoHeight:
    if (constraintSatHght(task, point,
                          !!task->pctx->ispec[pullInfoTangent1],
                          !!task->pctx->ispec[pullInfoTangent2],
                          !!task->pctx->ispec[pullInfoNegativeTangent1],
                          !!task->pctx->ispec[pullInfoNegativeTangent2],
                          stepMax, iterMax, constrFailP)) {
      biffAddf(PULL, "%s: trouble", me);
      return 1;
    }
    break;
  default:
    fprintf(stderr, "%s: constraint on %s (%d) unimplemented!!\n", me,
            airEnumStr(pullInfo, task->pctx->constraint),
            task->pctx->constraint);
  }
  ELL_3V_SUB(pos3Diff, pos3Orig, point->pos);
  travel = ELL_3V_LEN(pos3Diff)/task->pctx->voxelSizeSpace;
  if (travel > travelMax) {
    *constrFailP = pullConstraintFailTravel;
  }
  /*
  fprintf(stderr, "!%s(%u) %s @ (%g,%g,%g) = (%g,%g,%g) + (%g,%g,%g)\n", me,
          point->idtag,
          (*constrFailP
           ? airEnumStr(pullConstraintFail, *constrFailP)
           : "#GOOD#"),
          point->pos[0], point->pos[1], point->pos[2],
          pos3Diff[0], pos3Diff[1], pos3Diff[2],
          pos3Orig[0], pos3Orig[1], pos3Orig[2]);
  */
  return 0;
}

#undef NORMALIZE

/*
** _pullConstraintTangent
**
** eigenvectors (with non-zero eigenvalues) of output proj are
** (hopefully) approximate tangents to the manifold to which particles
** are constrained.  It is *not* the local tangent of the directions
** along which particles are allowed to move during constraint
** satisfaction (that is given by creaseProj for creases)
** 
** this can assume that probe() has just been called 
*/
void
_pullConstraintTangent(pullTask *task, pullPoint *point, 
                       /* output */
                       double proj[9]) {
  double vec[4], nvec[3], outer[9], len, posproj[9], negproj[9];

  ELL_3M_IDENTITY_SET(proj); /* NOTE: we are starting with identity . . . */
  switch (task->pctx->constraint) {
  case pullInfoHeight:
    creaseProj(task, point,
               !!task->pctx->ispec[pullInfoTangent1],
               !!task->pctx->ispec[pullInfoTangent2],
               !!task->pctx->ispec[pullInfoNegativeTangent1],
               !!task->pctx->ispec[pullInfoNegativeTangent2],
               posproj, negproj);
    /* .. and subracting out output from creaseProj */
    ELL_3M_SUB(proj, proj, posproj);
    ELL_3M_SUB(proj, proj, negproj);
    break;
  case pullInfoHeightLaplacian:
  case pullInfoIsovalue:
    if (pullInfoHeightLaplacian == task->pctx->constraint) {
      /* using gradient of height as approx normal to laplacian 0-crossing */
      _pullPointScalar(task->pctx, point, pullInfoHeight, vec, NULL);
    } else {
      _pullPointScalar(task->pctx, point, pullInfoIsovalue, vec, NULL);
    }
    ELL_3V_NORM(nvec, vec, len);
    if (len) {
      /* .. or and subracting out tensor product of normal with itself */
      ELL_3MV_OUTER(outer, nvec, nvec);
      ELL_3M_SUB(proj, proj, outer);
    }
    break;
  }
  return;
}

/*
** returns the *dimension* (not codimension) of the constraint manifold:
** 0 for points
** 1 for lines
** 2 for surfaces
**
** a -1 return value represents a biff-able error
*/
int
_pullConstraintDim(const pullContext *pctx) {
  static const char me[]="_pullConstraintDim";
  int ret, t1, t2, nt1, nt2;
  
  switch (pctx->constraint) {
  case pullInfoHeightLaplacian: /* zero-crossing edges */
    ret = 2;
    break;
  case pullInfoIsovalue:
    ret = 2;
    break;
  case pullInfoHeight:
    t1 = !!pctx->ispec[pullInfoTangent1];
    t2 = !!pctx->ispec[pullInfoTangent2];
    nt1 = !!pctx->ispec[pullInfoNegativeTangent1];
    nt2 = !!pctx->ispec[pullInfoNegativeTangent2];
    switch (t1 + t2 + nt1 + nt2) {
    case 0:
    case 3:
      ret = 0;
      break;
    case 1:
      ret = 2;
      break;
    case 2:
      ret = 1;
      break;
    default:
      biffAddf(PULL, "%s: can't simultaneously use all tangents "
               "(%s,%s,%s,%s) as this implies co-dimension of -1", me,
               airEnumStr(pullInfo, pullInfoTangent1),
               airEnumStr(pullInfo, pullInfoTangent2),
               airEnumStr(pullInfo, pullInfoNegativeTangent1),
               airEnumStr(pullInfo, pullInfoNegativeTangent2));
      return -1;
    }
    break;
  default:
    biffAddf(PULL, "%s: constraint on %s (%d) unimplemented", me,
             airEnumStr(pullInfo, pctx->constraint), pctx->constraint);
    return -1;
  }
  return ret;
}

/* --------------------------------------------- */

pullTraceSingle *
pullTraceSingleNew(void) {
  pullTraceSingle *ret;

  ret = AIR_CALLOC(1, pullTraceSingle);
  if (ret) {
    ret->seedPos[0] = ret->seedPos[1] = AIR_NAN;
    ret->seedPos[2] = ret->seedPos[3] = AIR_NAN;
    ret->nvert = nrrdNew();
    ret->nstrn = nrrdNew();
    ret->nvelo = nrrdNew();
    ret->seedIdx = ret->stepNum[0] = ret->stepNum[1] = 0;
    ret->speeding = AIR_FALSE;
    ret->nonstarter = AIR_FALSE;
    ret->calstop = AIR_FALSE;
  }
  return ret;
}

pullTraceSingle *
pullTraceSingleNix(pullTraceSingle *pts) {

  if (pts) {
    nrrdNuke(pts->nvert);
    nrrdNuke(pts->nstrn);
    nrrdNuke(pts->nvelo);
  }
  return NULL;
}


int
pullScaleTrace(pullContext *pctx, pullTraceSingle *pts,
               int useStrength,
               double scaleDelta, double halfScaleWin,
               double velocityMax,
               const double seedPos[4]) {
  static const char me[]="pullScaleTrace";
  pullPoint *point;
  airArray *mop, *trceArr[2], *hstrnArr[2];
  double *trce[2], ssrange[2], *vert, *hstrn[2], *strn, *velo, travmax;
  int constrFail;
  unsigned int dirIdx, lentmp, tidx, oidx, vertNum, win;

#define INCR 100

  if (!( pctx && pts && seedPos )) {
    biffAddf(PULL, "%s: got NULL pointer", me);
    return 1;
  }
  if (!( AIR_EXISTS(scaleDelta) && scaleDelta > 0.0 )) {
    biffAddf(PULL, "%s: need existing scaleDelta > 0 (not %g)",
             me, scaleDelta);
    return 1;
  }
  if (!( halfScaleWin > 0 )) {
    biffAddf(PULL, "%s: need halfScaleWin > 0", me);
    return 1;
  }
  if (!(pctx->constraint)) {
    biffAddf(PULL, "%s: given context doesn't have constraint set", me);
    return 1;
  }
  if (pullConstraintScaleRange(pctx, ssrange)) {
    biffAddf(PULL, "%s: trouble getting scale range", me);
    return 1;
  }

  /* save seedPos in any case */
  ELL_4V_COPY(pts->seedPos, seedPos);

  mop = airMopNew();
  point = pullPointNew(pctx);
  airMopAdd(mop, point, (airMopper)pullPointNix, airMopAlways);

  ELL_4V_COPY(point->pos, seedPos);
  if (_pullConstraintSatisfy(pctx->task[0], point, 2, &constrFail)) {
    biffAddf(PULL, "%s: constraint sat on seed point", me);
    airMopError(mop);
    return 1;
  }
  /*
  fprintf(stderr, "!%s: seed=(%g,%g,%g,%g) -> %s (%g,%g,%g,%g)\n", me,
          seedPos[0], seedPos[1], seedPos[2], seedPos[3], 
          constrFail ? "!NO!" : "(yes)",
          point->pos[0] - seedPos[0], point->pos[1] - seedPos[1],
          point->pos[2] - seedPos[2], point->pos[3] - seedPos[3]);
  */
  if (constrFail) {
    pts->speeding = AIR_FALSE;
    pts->nonstarter = AIR_TRUE;
    airMopOkay(mop);
    return 0;
  }

  /* else constraint sat worked at seed point; we have work to do */
  pts->speeding = AIR_FALSE;
  pts->nonstarter = AIR_FALSE;
  travmax = 10.0*scaleDelta*velocityMax/pctx->voxelSizeSpace;

  for (dirIdx=0; dirIdx<2; dirIdx++) {
    trceArr[dirIdx] = airArrayNew((void**)(trce + dirIdx), NULL,
                                  4*sizeof(double), INCR);
    airMopAdd(mop, trceArr[dirIdx], (airMopper)airArrayNuke, airMopAlways);
    if (useStrength && pctx->ispec[pullInfoStrength]) {
      hstrnArr[dirIdx] = airArrayNew((void**)(hstrn + dirIdx), NULL,
                                     sizeof(double), INCR);
      airMopAdd(mop, hstrnArr[dirIdx], (airMopper)airArrayNuke, airMopAlways);
    } else {
      hstrnArr[dirIdx] = NULL;
      hstrn[dirIdx] = NULL;
    }
  }
  for (dirIdx=0; dirIdx<2; dirIdx++) {
    unsigned int step;
    double dscl;
    dscl = (!dirIdx ? -1 : +1)*scaleDelta;
    step = 0;
    while (1) {
      if (!step) {
        /* first step in both directions requires special tricks */
        if (0 == dirIdx) {
          /* save constraint sat of seed point */
          tidx = airArrayLenIncr(trceArr[0], 1);
          ELL_4V_COPY(trce[0] + 4*tidx, point->pos);
          if (useStrength && pctx->ispec[pullInfoStrength]) {
            tidx = airArrayLenIncr(hstrnArr[0], 1);
            hstrn[0][0] = _pullPointScalar(pctx, point, pullInfoStrength,
                                           NULL, NULL);
          }
        } else {
          /* re-set position from constraint sat of seed pos */
          ELL_4V_COPY(point->pos, trce[0] + 4*0);
        }
      }
      /* nudge position along scale */
      point->pos[3] += dscl;
      if (!AIR_IN_OP(ssrange[0], point->pos[3], ssrange[1])) {
        /* if we've stepped outside the range of scale for the volume
           containing the constraint manifold, we're done */
        break;
      }
      if (AIR_ABS(point->pos[3] - seedPos[3]) > halfScaleWin) {
        /* we've moved along scale as far as allowed */
        break;
      }
      /* re-assert constraint */
      /*
      fprintf(stderr, "%s(%u): pos = %g %g %g %g.... \n", me,
              point->idtag, point->pos[0], point->pos[1],
              point->pos[2], point->pos[3]);
      */
      if (_pullConstraintSatisfy(pctx->task[0], point,
                                 travmax, &constrFail)) {
        biffAddf(PULL, "%s: dir %u, step %u", me, dirIdx, step);
        airMopError(mop);
        return 1;
      }
      /*
      fprintf(stderr, "%s(%u): ... %s(%d); pos = %g %g %g %g\n", me,
              point->idtag,
              constrFail ? "FAIL" : "(ok)",
              constrFail, point->pos[0], point->pos[1],
              point->pos[2], point->pos[3]);
      */
      if (constrFail) {
        /* constraint sat failed; no error, we're just done
           with stepping for this direction */
        break;
      }
      if (trceArr[dirIdx]->len >= 2) {
        /* see if we're moving too fast, by comparing with previous point */
        double pos0[3], pos1[3], diff[3], velo;
        unsigned int ii;
        
        ii = trceArr[dirIdx]->len-2;
        ELL_3V_COPY(pos0, trce[dirIdx] + 4*(ii+0));
        ELL_3V_COPY(pos1, trce[dirIdx] + 4*(ii+1));
        ELL_3V_SUB(diff, pos1, pos0);
        velo = ELL_3V_LEN(diff)/scaleDelta;
        /*
        fprintf(stderr, "%s(%u): velo %g %s velocityMax %g => %s\n", me, 
                point->idtag, velo, 
                velo > velocityMax ? ">" : "<=",
                velocityMax, 
                velo > velocityMax ? "FAIL" : "(ok)");
        */
        if (velo > velocityMax) {
          pts->speeding = AIR_TRUE;
          break;
        }
      }
      /* else save new point on trace */
      tidx = airArrayLenIncr(trceArr[dirIdx], 1);
      ELL_4V_COPY(trce[dirIdx] + 4*tidx, point->pos);
      if (useStrength && pctx->ispec[pullInfoStrength]) {
        tidx = airArrayLenIncr(hstrnArr[dirIdx], 1);
        hstrn[dirIdx][tidx] = _pullPointScalar(pctx, point, pullInfoStrength,
                                               NULL, NULL);
      }
      step++;
    }
  }
  
  /* transfer trace halves to pts->nvert */
  vertNum = trceArr[0]->len + trceArr[1]->len;
  if (nrrdMaybeAlloc_va(pts->nvert, nrrdTypeDouble, 2,
                        AIR_CAST(size_t, 4),
                        AIR_CAST(size_t, vertNum))
      || nrrdMaybeAlloc_va(pts->nvelo, nrrdTypeDouble, 1,
                           AIR_CAST(size_t, vertNum))) {
    biffMovef(PULL, NRRD, "%s: allocating output", me);
    airMopError(mop);
    return 1;
  }
  if (useStrength && pctx->ispec[pullInfoStrength]) {
    if (nrrdSlice(pts->nstrn, pts->nvert, 0 /* axis */, 0 /* pos */)) {
      biffMovef(PULL, NRRD, "%s: allocating output", me);
      airMopError(mop);
      return 1;
    }
  }
  vert = AIR_CAST(double *, pts->nvert->data);
  if (useStrength && pctx->ispec[pullInfoStrength]) {
    strn = AIR_CAST(double *, pts->nstrn->data);
  } else {
    strn = NULL;
  }
  velo = AIR_CAST(double *, pts->nvelo->data);
  lentmp = trceArr[0]->len;
  oidx = 0;
  for (tidx=0; tidx<lentmp; tidx++) {
    ELL_4V_COPY(vert + 4*oidx, trce[0] + 4*(lentmp - 1 - tidx));
    if (strn) {
      strn[oidx] = hstrn[0][lentmp - 1 - tidx];
    }
    oidx++;
  }
  lentmp = trceArr[1]->len;
  for (tidx=0; tidx<lentmp; tidx++) {
    ELL_4V_COPY(vert + 4*oidx, trce[1] + 4*tidx);
    if (strn) {
      strn[oidx] = hstrn[1][tidx];
    }
    oidx++;
  }
  lentmp = pts->nvelo->axis[0].size;
  for (tidx=0; tidx<lentmp; tidx++) {
    double *p0, *p1, *p2, diff[3];
    if (!tidx) {
      /* first */
      p1 = vert + 4*tidx;
      p2 = vert + 4*(tidx+1);
      ELL_3V_SUB(diff, p2, p1);
      velo[tidx] = ELL_3V_LEN(diff)/(p2[3]-p1[3]);
    } else if (tidx < lentmp-1) {
      /* middle */
      p0 = vert + 4*(tidx-1);
      p2 = vert + 4*(tidx+1);
      ELL_3V_SUB(diff, p2, p0);
      velo[tidx] = ELL_3V_LEN(diff)/(p2[3]-p0[3]);
    } else {
      /* last */
      p0 = vert + 4*(tidx-1);
      p1 = vert + 4*tidx;
      ELL_3V_SUB(diff, p1, p0);
      velo[tidx] = ELL_3V_LEN(diff)/(p1[3]-p0[3]);
    }
  }
  win = lentmp/20; /* HEY magic constant */
  if (win >= 4) {
    unsigned int schange;
    double dv, dv0, rdv, dv1;
    schange = 0;
    rdv = 0.0;
    for (tidx=0; tidx<lentmp-1; tidx++) {
      dv = (velo[tidx+1] - velo[tidx])/scaleDelta;
      if (tidx < win) {
        rdv += dv;
      } else {
        double tmp;
        if (tidx == win) {
          dv0 = rdv;
        }
        tmp = rdv;
        rdv += dv;
        rdv -= (velo[tidx-win+1] - velo[tidx-win])/scaleDelta;
        schange += (rdv*tmp < 0);
      }
    }
    dv1 = rdv;
    pts->calstop = (1 == schange) && dv0 <= 0.0 && dv1 >= 0.0;
  } else {
    pts->calstop = AIR_FALSE;
  }


#undef INCR
  airMopOkay(mop);
  return 0;
}

