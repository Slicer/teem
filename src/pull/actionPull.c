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
** meanNeighDist is allowed to be NULL
**
** meanNeighDist must be set to something; < 0 if there are no neighbors
*/
static double
_energyPoints(pullTask *task, pullBin *bin, pullPoint *point, 
              /* output */
              double egrad[4], double *meanNeighDist) {
  double energy;

  energy = 0;
  if (meanNeighDist) {
    *meanNeighDist = -1;
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
  if (distLimit < 0 /* no neighbors! */
      || pullEnergyZero == task->pctx->energySpec->energy) {
    distLimit = task->pctx->radiusSpace;
  }
  /* hey, maybe using voxel size should also be used with distLimit */

  ELL_4V_COPY(posOld, point->pos);
  point->stepEnergy = AIR_MIN(distLimit, point->stepEnergy);
  do {
    int constrFail;

    ELL_4V_SCALE_ADD2(point->pos, 1.0, posOld, point->stepEnergy, force);
    if (_pullProbe(task->pctx->task[0], point, point->pos)) {
      sprintf(err, "%s: probing initial newpos (step=%g)", me,
              point->stepEnergy);
      biffAdd(PULL, err); return 1;
    }
    if (task->pctx->haveConstraint) {
      /* posNew = satisfy constraint */
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
      /* the idea is that if you had a non-trivial force, but you can't
         ever seem to take a small enough step to reduce energy, then
         you have a serious problem */
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
