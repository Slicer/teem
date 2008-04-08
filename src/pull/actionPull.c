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

/*
** issues:
** does everythign work on the first iteration
** has pullProbe() been called when image info is needed
** how to handle the needed extra probe for d strength / d scale
** does it eventually catch non-existant energy or force
** how are force/energy along scale handled differently than in space?
*/

static unsigned int
_neighBinPoints(pullPoint **neigh, pullTask *task, pullBin *bin, 
                pullPoint *point) {

  AIR_UNUSED(neigh);
  AIR_UNUSED(task);
  AIR_UNUSED(bin);
  AIR_UNUSED(point);

  return 0;
}


static double
_energyInterParticle(pullPoint *me, pullPoint *she, double egrad[4],
                     double *spatialDistP) {
  double vec[4];

  ELL_4V_SUB(vec, she->pos, me->pos);
  *spatialDistP = ELL_3V_LEN(vec);
  ELL_4V_SET(egrad, 0, 0, 0, 0);

  return 0;
}

/*
** meanNeighDist is allowed to be NULL. If it is non-NULL, 
** it must be set, and must be < 0 if there are no neighbors
*/
static double
_energyPoints(pullTask *task, pullBin *bin, pullPoint *point, 
              /* output */
              double egradSum[4], double *meanNeighDist) {
  double energySum, distSum, egrad[4];
  int nopt,     /* optimiziation: we sometimes re-use neighbor lists */
    ntrue;      /* we search all possible neighbors, stored in the bins
                   (either because !nopt, or, this iter we learn true
                   subset of interacting neighbors */
  unsigned int nidx, neighCount,
    nnum;       /* how much of task->neigh[] we use */

  if (meanNeighDist && !egradSum) {
    /* shouldn't happen */
    return AIR_NAN;
  }
  
  /* set nopt and ntrue */
  if (task->pctx->neighborTrueProb < 1 && egradSum) {
    /* We do the neighbor list optimization only when we're also asked
       to compute the energy gradient.  When we're not getting the energy
       gradient, we're being called to test the waters at possible new
       locations, in which case we can't be changing the effective particle 
       neighborhood */
    nopt = AIR_TRUE;
    ntrue = (0 == point->neighNum /* as in the first iteration */
             || airDrandMT_r(task->rng) < task->pctx->neighborTrueProb);
  } else {
    nopt = AIR_FALSE;
    ntrue = AIR_TRUE;
  }

  /* set nnum and task->neigh[] */
  if (ntrue) {
    nnum = _neighBinPoints(task->neigh, task, bin, point);
    if (nopt) {
      airArrayLenSet(point->neighArr, 0);
    }
  } else {
    /* (nopt true) this iter we re-use existing neighbor list */
    nnum = point->neighNum;
    for (nidx=0; nidx<point->neighNum; nidx++) {
      task->neigh[nidx] = point->neigh[nidx];
    }
  }

  /* loop through neighbor points */
  energySum = 0;
  distSum = 0;
  neighCount = 0;
  if (egradSum) {
    ELL_4V_SET(egradSum, 0, 0, 0, 0);
  }
  for (nidx=0; nidx<nnum; nidx++) {
    double dist; 
    energySum += _energyInterParticle(point, task->neigh[nidx],
                                      egradSum ? egrad : NULL, &dist);
    if (egradSum) {
      ELL_4V_INCR(egradSum, egrad);
      if (ELL_4V_DOT(egrad, egrad)) {
        if (meanNeighDist) {
          neighCount++;
          distSum += dist;
        }
        if (nopt) {
          unsigned int ii;
          ii = airArrayLenIncr(point->neighArr, 1);
          point->neigh[ii] = task->neigh[nidx];
        }
      }
    }
  }

  /* finish computing mean distance to neighbors */
  if (meanNeighDist) {
    if (neighCount) {
      *meanNeighDist = distSum/neighCount;
    } else {
      *meanNeighDist = -1;
    }
  }

  return energySum;
}

/*
** this requires that "point" has just been the benefit of _pullProbe(),
*/
static double
_energyImage(pullTask *task, pullPoint *point,
             /* output */
             double egrad[4]) {
  double energy;

  if (task->pctx->ispec[pullInfoHeight]
      && !task->pctx->ispec[pullInfoHeight]->constraint) {
    energy = _pullPointHeight(task->pctx, point, egrad, NULL);
  } else {
    energy = 0;
  }

  return energy;
}

/*
** this requires that "point" has just been the benefit of _pullProbe(),
** because thats what _energyImage() needs
**
** its in here that we scale from "energy gradient" to "force"
*/
static double
_energyTotal(pullTask *task, pullBin *bin, pullPoint *point,
             /* output */
             double force[4], double *neighDist) {
  double enrIm, enrPt, egradIm[4], egradPt[4], energy;

  ELL_4V_SET(egradIm, 0, 0, 0, 0); /* sssh */
  ELL_4V_SET(egradPt, 0, 0, 0, 0); /* sssh */
  enrIm = _energyImage(task, point,
                       force ? egradIm : NULL);
  enrPt = _energyPoints(task, bin, point,
                        force ? egradPt : NULL, neighDist);
  energy = AIR_LERP(task->pctx->alpha, enrIm, enrPt);
  if (force) {
    ELL_4V_LERP(force, task->pctx->alpha, egradIm, egradPt);
    ELL_4V_SCALE(force, -1, force);
  }
  return energy;
}

int
_pullPointProcess(pullTask *task, pullBin *bin, pullPoint *point) {
  char me[]="pullPointProcess", err[BIFF_STRLEN];
  double energyOld, energyNew, force[4], distLimit, posOld[4];
  int stepBad;

  energyOld = _energyTotal(task, bin, point, force, &distLimit);
  if (!( AIR_EXISTS(energyOld) && ELL_4V_EXISTS(force) )) {
    sprintf(err, "%s: point %u non-exist energy or force", me, point->idtag);
    biffAdd(PULL, err); return 1;
  }
  if (distLimit < 0 /* no neighbors! */
      || pullEnergyZero == task->pctx->energySpec->energy) {
    distLimit = task->pctx->radiusSpace;
  }
  /* maybe voxel size should also be considered for finding distLimit */

  ELL_4V_COPY(posOld, point->pos);
  point->stepEnergy = AIR_MIN(distLimit, point->stepEnergy);
  do {
    int constrFail;

    ELL_4V_SCALE_ADD2(point->pos, 1.0, posOld, point->stepEnergy, force);
    if (_pullProbe(task, point, point->pos)) {
      sprintf(err, "%s: probing initial newpos (step=%g)", me,
              point->stepEnergy);
      biffAdd(PULL, err); return 1;
    }
    if (task->pctx->haveConstraint) {
      /* point->pos = satisfy constraint */
      /* constrFail = couldn't satisfy constraint */
      constrFail = AIR_FALSE;
    } else {
      constrFail = AIR_FALSE;
    }
    if (constrFail) {
      energyNew = AIR_NAN;
    } else {
      energyNew = _energyTotal(task, bin, point, NULL, NULL);
    }
    stepBad = constrFail || (energyNew > energyOld);
    if (stepBad) {
      point->stepEnergy *= task->pctx->energyStepScale;
      /* you have a problem if you had a non-trivial force, but you can't
         ever seem to take a small enough step to reduce energy */
      if (point->stepEnergy < 0.000000000001
          && ELL_4V_LEN(force) > 0.000001) {
        sprintf(err, "%s: point %u stepEnergy=%g: where can it go?", me,
                point->idtag, point->stepEnergy);
        biffAdd(PULL, err); return 1;
      }
    }
  } while (stepBad);
  /* now: energy decreased, and, if we have one, constraint has been met */

  /* not recorded for the sake of this function, but for system accounting */
  point->energy = energyNew;

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
