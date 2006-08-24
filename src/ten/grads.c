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

tenGradientParm *
tenGradientParmNew(void) {
  tenGradientParm *ret;

  ret = (tenGradientParm *)calloc(1, sizeof(tenGradientParm));
  if (ret) {
    ret->step = 1;
    ret->jitter = 0.2;
    ret->minVelocity = 0.000000001;
    ret->minPotentialChange = 0.000000001;
    ret->minMean = 0.0001;
    ret->minMeanImprovement = 0.00001;
    ret->single = AIR_FALSE;
    ret->snap = 0;
    ret->expo = 1;
    ret->seed = 42;
    ret->maxIteration = 1000000;
    ret->step = 0;
    ret->idealEdge = 0;
    ret->nudge = 0;
    ret->itersUsed = 0;
  }
  return ret;
}

tenGradientParm *
tenGradientParmNix(tenGradientParm *tgparm) {
  
  airFree(tgparm);
  return NULL;
}

int
tenGradientCheck(const Nrrd *ngrad, int type, unsigned int minnum) {
  char me[]="tenGradientCheck", err[BIFF_STRLEN];
  
  if (nrrdCheck(ngrad)) {
    sprintf(err, "%s: basic validity check failed", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  if (!( 3 == ngrad->axis[0].size && 2 == ngrad->dim )) {
    sprintf(err, "%s: need a 3xN 2-D array (not a " _AIR_SIZE_T_CNV 
            "x? %u-D array)",
            me, ngrad->axis[0].size, ngrad->dim);
    biffAdd(TEN, err); return 1;
  }
  if (nrrdTypeDefault != type && type != ngrad->type) {
    sprintf(err, "%s: requested type %s but got type %s", me,
            airEnumStr(nrrdType, type), airEnumStr(nrrdType, ngrad->type));
    biffAdd(TEN, err); return 1;
  }
  if (nrrdTypeBlock == ngrad->type) {
    sprintf(err, "%s: sorry, can't use %s type", me, 
            airEnumStr(nrrdType, nrrdTypeBlock));
    biffAdd(TEN, err); return 1;
  }
  if (!( minnum <= ngrad->axis[1].size )) {
    sprintf(err, "%s: have only " _AIR_SIZE_T_CNV " gradients, "
            "need at least %d",
            me, ngrad->axis[1].size, minnum);
    biffAdd(TEN, err); return 1;
  }

  return 0;
}

/*
******** tenGradientRandom
**
** generates num random unit vectors of type double
*/
int
tenGradientRandom(Nrrd *ngrad, unsigned int num, unsigned int seed) {
  char me[]="tenGradientRandom", err[BIFF_STRLEN];
  double *grad, len;
  unsigned int gi;
  
  if (nrrdMaybeAlloc_va(ngrad, nrrdTypeDouble, 2,
                        AIR_CAST(size_t, 3), AIR_CAST(size_t, num))) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  airSrandMT(seed);
  grad = AIR_CAST(double*, ngrad->data);
  for (gi=0; gi<num; gi++) {
    do {
      grad[0] = AIR_AFFINE(0, airDrandMT(), 1, -1, 1);
      grad[1] = AIR_AFFINE(0, airDrandMT(), 1, -1, 1);
      grad[2] = AIR_AFFINE(0, airDrandMT(), 1, -1, 1);
      len = ELL_3V_LEN(grad);
    } while (len > 1 || !len);
    ELL_3V_SCALE(grad, 1.0/len, grad);
    grad += 3;
  }
  return 0;
}

/*
******** tenGradientIdealEdge
**
** edge length of delauney triangulation of idealized distribution of
** N gradients (2*N points), but also allowing a boolean "single" flag
** saying that we actually care about N points
*/
double
tenGradientIdealEdge(unsigned int N, int single) {

  return sqrt((!single ? 4 : 8)*AIR_PI/(sqrt(3)*N));
}

/*
******** tenGradientJitter
**
** moves all gradients by amount dist on tangent plane, in a random
** direction, and then renormalizes. The distance is a fraction
** of the ideal edge length (via tenGradientIdealEdge)
*/
int
tenGradientJitter(Nrrd *nout, const Nrrd *nin, double dist) {
  char me[]="tenGradientJitter", err[BIFF_STRLEN];
  double *grad, perp0[3], perp1[3], len, theta, cc, ss, edge;
  unsigned int gi, num;

  if (nrrdConvert(nout, nin, nrrdTypeDouble)) {
    sprintf(err, "%s: trouble converting input to double", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  if (tenGradientCheck(nout, nrrdTypeDouble, 3)) {
    sprintf(err, "%s: didn't get valid gradients", me);
    biffAdd(TEN, err); return 1;
  }
  grad = AIR_CAST(double*, nout->data);
  num = nout->axis[1].size;
  /* HEY: possible confusion between single and not */
  edge = tenGradientIdealEdge(num, AIR_FALSE);
  for (gi=0; gi<num; gi++) {
    ELL_3V_NORM(grad, grad, len);
    ell_3v_perp_d(perp0, grad);
    ELL_3V_CROSS(perp1, perp0, grad);
    theta = AIR_AFFINE(0, airDrandMT(), 1, 0, 2*AIR_PI);
    cc = dist*edge*cos(theta);
    ss = dist*edge*sin(theta);
    ELL_3V_SCALE_ADD3(grad, 1.0, grad, cc, perp0, ss, perp1);
    ELL_3V_NORM(grad, grad, len);
    grad += 3;
  }
  
  return 0;
}

void
tenGradientMeasure(double *pot, double *minAngle, 
                   const Nrrd *npos, tenGradientParm *tgparm,
                   int edgeNormalize) {
  /* char me[]="tenGradientMeasure"; */
  double diff[3], *pos, edge, atmp=0, ptmp;
  int ii, jj, num;

  /* char tmpStr[128]; */

  /* allow minAngle NULL */
  if (!(pot && npos && tgparm )) {
    return;
  }

  num = npos->axis[1].size;
  pos = AIR_CAST(double *, npos->data);

  edge = (edgeNormalize 
          ? tenGradientIdealEdge(num, tgparm->single)
          : 1.0);
  *pot = 0;
  if (minAngle) {
    *minAngle = AIR_PI;
  }
  for (ii=0; ii<num; ii++) {
    for (jj=0; jj<ii; jj++) {
      ELL_3V_SUB(diff, pos + 3*ii, pos + 3*jj);
      ptmp = airIntPow(edge/ELL_3V_LEN(diff), tgparm->expo);
      *pot += ptmp;
      if (minAngle) {
        atmp = ell_3v_angle_d(pos + 3*ii, pos + 3*jj);
        *minAngle = AIR_MIN(atmp, *minAngle);
      }
      if (!tgparm->single) {
        *pot += ptmp;
        ELL_3V_ADD2(diff, pos + 3*ii, pos + 3*jj);
        *pot += 2*airIntPow(edge/ELL_3V_LEN(diff), tgparm->expo);
        if (minAngle) {
          *minAngle = AIR_MIN(AIR_PI-atmp, *minAngle);
        }
      }
    }
  }
  /*
  sprintf(tmpStr, "tmp-pos-%g.nrrd", *minAngle);
  fprintf(stderr, "!%s: saving %s\n", me, tmpStr);
  nrrdSave(tmpStr, npos, NULL);
  */
  return;
}

/*
** returns the mean velocity 
*/
static double
update(Nrrd *npos, tenGradientParm *tgparm) {
  /* char me[]="update"; */
  double *pos, meanvel, newpos[3], grad[3], edge, dir[3], len, rep, step;
  int num, ii, jj;
  
  pos = AIR_CAST(double *, npos->data);
  num = npos->axis[1].size;
  edge = tenGradientIdealEdge(num, tgparm->single);
  meanvel = 0;
  for (ii=0; ii<num; ii++) {
    ELL_3V_SET(grad, 0, 0, 0);
    for (jj=0; jj<num; jj++) {
      if (ii == jj) {
        continue;
      }
      ELL_3V_SUB(dir, pos + 3*ii, pos + 3*jj);
      ELL_3V_NORM(dir, dir, len);
      rep = airIntPow(edge/len, tgparm->expo+1);
      ELL_3V_SCALE_INCR(grad, rep, dir);
      if (!tgparm->single) {
        ELL_3V_ADD2(dir, pos + 3*ii, pos + 3*jj);
        ELL_3V_NORM(dir, dir, len);
        rep = airIntPow(edge/len, tgparm->expo+1);
        ELL_3V_SCALE_INCR(grad, rep, dir);
      }
    }
    ELL_3V_NORM(grad, grad, len);
    if (!( ELL_3V_EXISTS(grad) && AIR_EXISTS(len) )) {
      ELL_3V_SET(grad, 0, 0, 0);
      len = 0;
    }
    step = AIR_MIN(len*tgparm->step, edge/2);
    ELL_3V_SCALE_ADD2(newpos,
                      1.0, pos + 3*ii,
                      step, grad);
    ELL_3V_NORM(newpos, newpos, len);
    ELL_3V_COPY(pos + 3*ii, newpos);
    meanvel += len;
  }
  meanvel /= num;

  return meanvel;
}

static double
party(Nrrd *npos, airRandMTState *rstate) {
  double *pos, mean[3];
  unsigned int ii, num, rnd, rndBit;

  pos = (double *)(npos->data);
  num = npos->axis[1].size;
  rnd = airUIrandMT_r(rstate);
  rndBit = 0;
  ELL_3V_SET(mean, 0, 0, 0);
  for (ii=0; ii<num; ii++) {
    if (32 == rndBit) {
      rnd = airUIrandMT_r(rstate);
      rndBit = 0;
    }
    if (rnd & (1 << rndBit++)) {
      ELL_3V_SCALE(pos + 3*ii, -1, pos + 3*ii);
    }
    ELL_3V_INCR(mean, pos + 3*ii);
  }
  ELL_3V_SCALE(mean, 1.0/num, mean);
  return ELL_3V_LEN(mean);
}

int
tenGradientBalance(Nrrd *nout, const Nrrd *nin,
                   tenGradientParm *tgparm) {
  char me[]="tenGradientBalance", err[BIFF_STRLEN];
  double len, lastLen, improv;
  airRandMTState *rstate;

  if (!nout || tenGradientCheck(nin, nrrdTypeUnknown, 2)) {
    sprintf(err, "%s: got NULL pointer or invalid input", me);
    biffAdd(TEN, err); return 1;
  }
  if (nrrdConvert(nout, nin, nrrdTypeDouble)) {
    sprintf(err, "%s: can't initialize output with input", me);
    biffMove(TEN, err, NRRD); return 1;
  }

  rstate = airRandMTStateNew(tgparm->seed);
  lastLen = 1.0;
  do {
    do {
      len = party(nout, rstate);
    } while (len > lastLen);
    improv = lastLen - len;
    lastLen = len;
    fprintf(stderr, "%s: improvement: %g  (mean length = %g)\n",
            me, improv, len);
  } while (improv > tgparm->minMeanImprovement
           && len > tgparm->minMean);
  airRandMTStateNix(rstate);
  
  return 0;
}

/*
******** tenGradientDistribute
**
** takes the given list of gradients, normalizes their lengths,
** optionally jitters their positions, does point repulsion, and then
** selects a combination of directions with minimum vector sum.
*/
int
tenGradientDistribute(Nrrd *nout, const Nrrd *nin,
                      tenGradientParm *tgparm) {
  char me[]="tenGradientDistribute", err[BIFF_STRLEN],
    filename[AIR_STRLEN_SMALL];
  unsigned int ii, num, iter, oldIdx, newIdx;
  airArray *mop;
  Nrrd *npos[2];
  double *pos, len, meanVelocity, pot, newpot, potchange;

  if (!nout || tenGradientCheck(nin, nrrdTypeUnknown, 2) || !tgparm) {
    sprintf(err, "%s: got NULL pointer or invalid input", me);
    biffAdd(TEN, err); return 1;
  }

  num = nin->axis[1].size;
  mop = airMopNew();
  npos[0] = nrrdNew();
  npos[1] = nrrdNew();
  airMopAdd(mop, npos[0], (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, npos[1], (airMopper)nrrdNuke, airMopAlways);
  if (nrrdConvert(npos[0], nin, nrrdTypeDouble)
      || nrrdConvert(npos[1], nin, nrrdTypeDouble)) {
    sprintf(err, "%s: trouble allocating temp buffers", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }

  pos = (double*)(npos[0]->data);
  for (ii=0; ii<num; ii++) {
    ELL_3V_NORM(pos, pos, len);
    pos += 3;
  }
  if (tgparm->jitter) {
    if (tenGradientJitter(npos[0], npos[0], tgparm->jitter)) {
      sprintf(err, "%s: problem jittering input", me);
      biffAdd(TEN, err); return 1;
    }
  }

  tgparm->step = tgparm->initStep;
  tgparm->nudge = 0.1;
  tenGradientMeasure(&pot, NULL, npos[0], tgparm, AIR_TRUE);
  tgparm->idealEdge = tenGradientIdealEdge(num, tgparm->single);
  potchange = 1.0; /* maximum possible value */
  meanVelocity = 0.0;
  oldIdx = 0;
  newIdx = 1;
  iter = 0;
  do {
    memcpy(npos[newIdx]->data, npos[oldIdx]->data, 3*num*sizeof(double));
    meanVelocity = update(npos[newIdx], tgparm);
    tenGradientMeasure(&newpot, NULL, npos[newIdx], tgparm, AIR_TRUE);
    potchange = (newpot - pot)/(pot*tgparm->step);
    if (potchange < 0) {
      /* potential has decreased, good */
      pot = newpot;
      if (tgparm->snap) {
        if (!(iter % tgparm->snap)) {
          sprintf(filename, "%05d.nrrd", iter/tgparm->snap);
          fprintf(stderr, "%s(%d):\n velo = %g, phi = %g ~ %g; saving %s\n",
                  me, iter, meanVelocity, pot, potchange, filename);
          if (nrrdSave(filename, npos[newIdx], NULL)) {
            char *serr;
            serr = biffGetDone(NRRD);
            fprintf(stderr, "%s: iter=%d, couldn't save snapshot:\n%s"
                    "continuing ...\n", me, iter, serr);
            free(serr);
          }
        }
      } else {
        if (!(iter % 500)) {
          fprintf(stderr, "%s(%d):\n velo = %g, phi = %g ~ %g\n",
                  me, iter, meanVelocity, pot, potchange);
        }
      }
      /* nudge up step size */
      tgparm->step *= 1 + tgparm->nudge;
      tgparm->step = AIR_MIN(tgparm->initStep, tgparm->step);
      /* swap buffers, or pretend to */
      newIdx = 1 - newIdx;
      oldIdx = 1 - oldIdx;
      iter++;
    } else {
      /* potential has increased, so back off and try again */
      fprintf(stderr, "%s(%d): step %g --> %g\n", me, iter,
              tgparm->step, tgparm->step/2);
      tgparm->step /= 2;
      tgparm->nudge /= 10;
    }
  } while (iter < tgparm->maxIteration
           && potchange > tgparm->minPotentialChange
           && meanVelocity > tgparm->minVelocity);

  fprintf(stderr, "%s: done distributing:\n", me);
  fprintf(stderr, "    iter=%d, vel = %g, phi = %g, delta phi = %g\n",
          iter, meanVelocity, pot, potchange);
  tgparm->itersUsed = iter;

  if (tgparm->minMeanImprovement || tgparm->minMean) {
    fprintf(stderr, "%s: optimizing balance ... \n", me);
    if (tenGradientBalance(nout, npos[oldIdx], tgparm)) {
      sprintf(err, "%s: failed to minimize vector sum of gradients", me);
      biffAdd(TEN, err); return 1;
    }
  } else {
    fprintf(stderr, "%s: leaving balance as is\n", me);
    if (nrrdConvert(nout, npos[oldIdx], nrrdTypeDouble)) {
      sprintf(err, "%s: couldn't set output", me);
      biffMove(TEN, err, NRRD); return 1;
    }
  }

  airMopOkay(mop); 
  return 0;
}

int
tenGradientGenerate(Nrrd *nout, unsigned int num, tenGradientParm *tgparm) {
  char me[]="tenGradientGenerate", err[BIFF_STRLEN];
  Nrrd *nin;
  airArray *mop;

  if (!(nout && tgparm)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( num >= 3 )) {
    sprintf(err, "%s: can generate minimum of 3 gradient directions "
            "(not %d)", me, num);
    biffAdd(TEN, err); return 1;
  }
  mop = airMopNew();
  nin = nrrdNew();
  airMopAdd(mop, nin, (airMopper)nrrdNuke, airMopAlways);

  if (tenGradientRandom(nin, num, tgparm->seed)
      || tenGradientDistribute(nout, nin, tgparm)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
