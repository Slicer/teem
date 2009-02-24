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

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "pull.h"
#include "privatePull.h"

int _pullPraying = 0;

/*
** issues:
** does everything work on the first iteration
** how to handle the needed extra probe for d strength / d scale
** how are force/energy along scale handled differently than in space?
*/

char
_pullProcessModeStr[PULL_PROCESS_MODE_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_mode)",
  "descent",
  "nlearn",
  "adding",
  "nixing"
};

airEnum
_pullProcessMode = {
  "process mode",
  PULL_PROCESS_MODE_MAX,
  _pullProcessModeStr,  NULL,
  NULL, NULL, NULL,
  AIR_FALSE
};
airEnum *
pullProcessMode = &_pullProcessMode;

/*
** this sets, in task->neighPoint (*NOT* point->neighPoint), all the
** points in neighboring bins with which we might possibly interact,
** and returns the number of such points.  At this phase there is no
** attempt to be clever about ruling out interaction with too-distant
** points
*/
static unsigned int
_neighBinPoints(pullTask *task, pullBin *bin, pullPoint *point) {
  char me[]="_neighBinPoints";
  unsigned int nn, herPointIdx, herBinIdx;
  pullBin *herBin;
  pullPoint *herPoint;

  nn = 0;
  herBinIdx = 0;
  while ((herBin = bin->neighBin[herBinIdx])) {
    for (herPointIdx=0; herPointIdx<herBin->pointNum; herPointIdx++) {
      herPoint = herBin->point[herPointIdx];
      /*
      fprintf(stderr, "!%s(%u): neighbin %u has point %u\n", me,
              point->idtag, herBinIdx, herPoint->idtag);
      */
      /* can't interact with myself, or anything nixed */
      if (point != herPoint
          && !(herPoint->status & PULL_STATUS_NIXME_BIT)) {
        if (nn+1 < _PULL_NEIGH_MAXNUM) {
          task->neighPoint[nn++] = herPoint;
          /*
          fprintf(stderr, "%s(%u): neighPoint[%u] = %u\n", 
                  me, point->idtag, nn-1, herPoint->idtag);
          */
        } else {
          fprintf(stderr, "%s: hit max# (%u) poss. neighbors (from bins)\n",
                  me, _PULL_NEIGH_MAXNUM);
        }
      }
    }
    herBinIdx++;
  }
  /* also have to consider things in the add queue */
  for (herPointIdx=0; herPointIdx<task->addPointNum; herPointIdx++) {
    herPoint = task->addPoint[herPointIdx];
    if (point != herPoint) {
      if (nn+1 < _PULL_NEIGH_MAXNUM) {
        task->neighPoint[nn++] = herPoint;
      } else {
        fprintf(stderr, "%s: hit max# (%u) poss. neighbors (from add queue)\n",
                me, _PULL_NEIGH_MAXNUM);
      }
    }
  }
  return nn;
}

/*
** compute the energy at "me" due to "she", and
** the gradient vector of her energy (probably pointing towards her)
**
** we're passed spaceDist to save us work of recomputing sqrt()
**
** egrad will be NULL if this is being called only to assess
** the energy at this point, rather than for learning how to move it
*/
static double
_energyInterParticle(pullTask *task, pullPoint *me, pullPoint *she, 
                     double spaceDist, double scaleDist,
                     /* output */
                     double egrad[4]) {
  char meme[]="_energyInterParticle";
  double diff[4], scaleSgn, spaceRad, scaleRad,
    rr, ss, enr, denr, *parm;
  double enrTotal=0;

  /* the vector "diff" goes from her, to me, in both space and scale */
  ELL_4V_SUB(diff, me->pos, she->pos);
  /* computed by caller: spaceDist = ELL_3V_LEN(diff); */
  /* computed by caller: scaleDist = AIR_ABS(diff[3]); */ 
  spaceRad = task->pctx->radiusSpace;
  scaleRad = task->pctx->radiusScale;
  rr = spaceDist/spaceRad;
  if (task->pctx->haveScale) {
    ss = scaleDist/scaleRad;
    scaleSgn = airSgn(diff[3]);
  } else {
    ss = 0;
  }

  /*
  fprintf(stderr, "!%s: rr(%u,%u) = %g\n", meme, me->idtag, she->idtag, rr);
  */
  if (rr > 1 || ss > 1) {
    if (egrad) {
      ELL_4V_SET(egrad, 0, 0, 0, 0);
    }
    return 0;
  }
  if (rr == 0 && ss == 0) {
    fprintf(stderr, "%s: pos of pts %u, %u equal: (%g,%g,%g,%g)\n",
            meme, me->idtag, she->idtag, 
            me->pos[0], me->pos[1], me->pos[2], me->pos[3]);
    if (egrad) {
      ELL_4V_SET(egrad, 0, 0, 0, 0);
    }
    return 0;
  }

  if (1 || !task->pctx->haveScale) {
    parm = task->pctx->energySpec->parm;
    enr = task->pctx->energySpec->energy->eval(&denr, rr, parm);
    if (egrad) {
      denr *= 1.0/(spaceRad*spaceDist);
      ELL_3V_SCALE(egrad, denr, diff);
      egrad[3] = 0;
    }
    enrTotal = enr;
  } else {
#if 0
    /*Implementation of Phi_{x-G}(r,s)*/
    double enrs,frcs, enrg, frcg, beta;
    parm = task->pctx->energySpec->parm;
    enr = task->pctx->energySpec->energy->eval(&frc, rr, parm);
    enrs = pullEnergyGauss->eval(&frcs, rrs, NULL);
    enrg = pullEnergyGauss->eval(&frcg, rr, NULL);
    beta = task->pctx->beta;
    frc = -1.0 * (beta * frc - (1-beta) * frcg) * enrs * (1.0/(2*sparad));
    ELL_3V_SCALE(egrad,frc/spadist,diff);
    frcs *= -1.0 * (beta * enr - (1-beta) * enrg) / (2*scalerad);
    /*Compute final gradient*/
    ELL_3V_SCALE(egrad,frc,diff);
    egrad[3] = frcs;
    enrTotal = (beta * enr - (1-beta) * enrg) *enrs;

    if (0) {
      /* Implementation of Phi_x(r,x) */
      double y;
      y = sqrt((spadist * spadist)/(sparad * sparad) + 
               (scaledist * scaledist)/(scalerad * scalerad));
      parm = task->pctx->energySpec->parm;
      enr = task->pctx->energySpec->energy->eval(&frc, y, parm);
      ELL_3V_SCALE(egrad,frc*spadist/(y*sparad*sparad),diff);
      egrad[3] = frc*scaledist/(y*scalerad*scalerad);
      enrTotal = enr;
    }
#endif
  }
  /*
  fprintf(stderr, "%s: %u <-- %u = %g,%g,%g -> egrad = %g,%g,%g, enr = %g\n",
          meme, me->idtag, she->idtag, 
          diff[0], diff[1], diff[2],
          egrad[0], egrad[1], egrad[2], enr);
  */
  return enrTotal;
}

/*
** the "egradSum" argument is where the sum (over neighboring points) of
** the energy gradient goes, but it is also effectively a flag for the
** kind of computation that happens here:
**
** non-NULL egradSum: besides computing current energy, compute energy
** gradient (with possible constraint modifications) so that we can update
** the system.  point->neighInterNum will be computed, and thus so will
** point->neighDistMean and point->neighMode
**
** NULL egradSum: just tell me what the energy is; and do NOT compute:
** point->neighInterNum, point->neighDistMean, point->neighMode
*/
double
_pullEnergyFromPoints(pullTask *task, pullBin *bin, pullPoint *point, 
                      /* output */
                      double egradSum[4]) {
  char me[]="_pullEnergyFromPoints";
  double energySum, spaDistSqMax, modeWghtSum;
  int nopt,     /* optimiziation: we enable the re-use of neighbor lists
                   between interations, or at system start, creation of
                   neighbor lists */
    ntrue;      /* we search all possible neighbors availble in the bins
                   (either because !nopt, or, this iter we learn true
                   subset of interacting neighbors).  This could also
                   be called "dontreuse" or something like that */
  unsigned int nidx,
    nnum;       /* how much of task->neigh[] we use */

  /* set nopt and ntrue */
  if (pullProcessModeNeighLearn == task->processMode) {
    /* we're here to both learn and store the true interacting neighbors */
    nopt = AIR_TRUE;
    ntrue = AIR_TRUE;
  } else if (pullProcessModeAdding == task->processMode
             || pullProcessModeNixing == task->processMode) {
    /* we assume that the work of learning neighbors has already been 
       done, so we can reuse them now */
    nopt = AIR_TRUE;
    ntrue = AIR_FALSE;
  } else if (pullProcessModeDescent == task->processMode) {
    if (task->pctx->neighborTrueProb < 1) {
      nopt = AIR_TRUE;
      if (egradSum) {
        /* We allow the neighbor list optimization only when we're also
           asked to compute the energy gradient, since that's the first
           part of moving the particle. */
        ntrue = (0 == task->pctx->iter
                 || airDrandMT_r(task->rng) < task->pctx->neighborTrueProb);
      } else {
        /* When we're not getting the energy gradient, we're being
           called to test the waters at possible new locations, in which
           case we can't be changing the effective neighborhood */
        ntrue = AIR_TRUE;
      }
    } else {
      /* never trying neighborhood caching */
      nopt = AIR_FALSE;
      ntrue = AIR_TRUE;
    }
  } else {
    fprintf(stderr, "%s: process mode %d unrecognized!\n", me,
            task->processMode);
    return AIR_NAN;
  }

  /* NOTE that you can't have both nopt and ntrue be false, specifically,
     ntrue can be false only when nopt is true */
  /*
  fprintf(stderr, "!%s(%u), nopt = %d, ntrue = %d\n", me, point->idtag,
          nopt, ntrue);
  */
  /* set nnum and task->neigh[] */
  if (ntrue) {
    /* this finds the over-inclusive set of all possible interacting
       points, based on bin membership as well the task's add queue */
    nnum = _neighBinPoints(task, bin, point);
    if (nopt) {
      airArrayLenSet(point->neighPointArr, 0);
    }
  } else {
    /* (nopt true) this iter we re-use this point's existing neighbor
       list, copying it into the the task's neighbor list to simulate
       the action of _neighBinPoints() */
    nnum = point->neighPointNum;
    for (nidx=0; nidx<nnum; nidx++) {
      task->neighPoint[nidx] = point->neighPoint[nidx];
    }
  }

  /* loop through neighbor points */
  spaDistSqMax = task->pctx->radiusSpace*task->pctx->radiusSpace;
  /*
  fprintf(stderr, "%s: radiusSpace = %g -> spaDistSqMax = %g\n", me,
          task->pctx->radiusSpace, spaDistSqMax);
  */
  modeWghtSum = 0;
  energySum = 0;
  point->neighInterNum = 0;
  point->neighDistMean = 0.0;
  point->neighMode = 0.0;
  if (egradSum) {
    ELL_4V_SET(egradSum, 0, 0, 0, 0);
  }
  for (nidx=0; nidx<nnum; nidx++) {
    double diff[4], spaDistSq, spaDist, sclDist, enr, egrad[4];
    pullPoint *herPoint;

    herPoint = task->neighPoint[nidx];
    if (herPoint->status & PULL_STATUS_NIXME_BIT) {
      /* this point is not long for this world, pass over it */
      continue;
    }
    ELL_4V_SUB(diff, point->pos, herPoint->pos); /* me - her */
    spaDistSq = ELL_3V_DOT(diff, diff);
    /*
    fprintf(stderr, "!%s: %u:%g,%g,%g <-- %u:%g,%g,%g = sqd %g %s %g\n", me,
            point->idtag, point->pos[0], point->pos[1], point->pos[2], 
            herPoint->idtag,
            herPoint->pos[0], herPoint->pos[1], herPoint->pos[2],
            spaDistSq, spaDistSq > spaDistSqMax ? ">" : "<=", spaDistSqMax);
    */
    if (spaDistSq > spaDistSqMax) {
      continue;
    }
    sclDist = AIR_ABS(diff[3]);
    if (sclDist > task->pctx->radiusScale) {
      continue;
    }
    spaDist = sqrt(spaDistSq);
    /* we pass spaDist to avoid recomputing sqrt(), and sclDist for
       stupid consistency  */
    enr = _energyInterParticle(task, point, herPoint, spaDist, sclDist,
                               egradSum ? egrad : NULL);
    if (enr) {
      /* there is some non-zero energy due to her; and we assume that
         its not just a fluke zero-crossing of the potential profile */
      double nsclDist, nspaDist, ndist, ww;

      if (nopt && ntrue) {
        unsigned int ii;
        /* we have to record that we had an interaction with this point */
        ii = airArrayLenIncr(point->neighPointArr, 1);
        point->neighPoint[ii] = herPoint;
      }
      energySum += enr;
      point->neighInterNum++;
      nspaDist = spaDist/task->pctx->radiusSpace;
      if (task->pctx->haveScale) {
        nsclDist = sclDist/task->pctx->radiusScale;
        ndist = sqrt(nspaDist*nspaDist + nsclDist*nsclDist);
      } else {
        ndist = nspaDist;
      }
      point->neighDistMean += ndist;
      if (task->pctx->ispec[pullInfoTangentMode]) {
        double mm;
        mm = _pullPointScalar(task->pctx, herPoint, pullInfoTangentMode,
                              NULL, NULL);
        ww = 1.0/spaDistSq;
        point->neighMode += ww*mm;
        modeWghtSum += ww;
      }
      if (egradSum) {
        ELL_4V_INCR(egradSum, egrad);
      }
    }
  }
  
  /* finish computing things averaged over neighbors */
  if (point->neighInterNum) {
    point->neighDistMean /= point->neighInterNum;
    if (task->pctx->ispec[pullInfoTangentMode]) {
      point->neighMode /= modeWghtSum;
    }
  } else {
    /* we had no neighbors at all */
    point->neighDistMean = 0.0; /* shouldn't happen in any normal case */
    if (task->pctx->ispec[pullInfoTangentMode]) {
      point->neighMode = AIR_NAN;
    }
  }

  return energySum;
}

static double
_energyFromImage(pullTask *task, pullPoint *point,
                 /* output */
                 double egradSum[4]) {
  char me[]="_energyFromImage";
  double energy, grad3[3];
  int probed;

  /* not sure I have the logic for this right 
  int ptrue;
  if (task->pctx->probeProb < 1 && allowProbeProb) {
    if (egrad) {
      ptrue = (0 == task->pctx->iter
               || airDrandMT_r(task->rng) < task->pctx->probeProb);
    } else {
      ptrue = AIR_FALSE;
    }
  } else {
    ptrue = AIR_TRUE;
  }
  */
  probed = AIR_FALSE;

#define MAYBEPROBE \
  if (!probed) { \
    if (_pullProbe(task, point)) { \
      fprintf(stderr, "%s: problem probing!!!\n%s\n", me, biffGetDone(PULL)); \
    } \
    probed = AIR_TRUE; \
  }

  energy = 0;
  if (egradSum) {
    ELL_4V_SET(egradSum, 0, 0, 0, 0);
  }
  /* Note that height doesn't contribute to the energy if there is
     a constraint associated with it */
  if (task->pctx->ispec[pullInfoHeight]
      && !task->pctx->ispec[pullInfoHeight]->constraint
      && !(task->pctx->ispec[pullInfoHeightLaplacian]
           && task->pctx->ispec[pullInfoHeightLaplacian]->constraint)) {
    MAYBEPROBE;
    energy += _pullPointScalar(task->pctx, point, pullInfoHeight,
                               grad3, NULL);
    if (egradSum) {
      ELL_3V_INCR(egradSum, grad3);
    }
  }
  if (task->pctx->ispec[pullInfoIsovalue]
      && !task->pctx->ispec[pullInfoIsovalue]->constraint) {
    /* we're only going towards an isosurface, not constrained to it
       ==> set up a parabolic potential well around the isosurface */
    double val;
    MAYBEPROBE;
    val = _pullPointScalar(task->pctx, point, pullInfoIsovalue,
                           grad3, NULL);
    energy += val*val;
    if (egradSum) {
      ELL_3V_SCALE_INCR(egradSum, 2*val, grad3);
    }
  }
  /* HEY what about strength? */
  return energy;
}
#undef MAYBEPROBE

/*
** NOTE that the "egrad" being non-NULL has consequences for what gets
** computed in _energyFromImage and _pullEnergyFromPoints:
**
** NULL "egrad": we're simply learning the energy (and want to know it
** as truthfully as possible) for the sake of inspecting system state
**
** non-NULL "egrad": we're learning the current energy, but the real point
** is to determine how to move the point to lower energy
**
** the ignoreImage flag is a hack, to allow _pullPointProcessAdding to
** do descent on a new point according to other points, but not the
** image.
*/
double
_pullPointEnergyTotal(pullTask *task, pullBin *bin, pullPoint *point,
                      int ignoreImage,
                      /* output */
                      double egrad[4]) {
  char me[]="_pullPointEnergyTotal";
  double enrIm, enrPt, egradIm[4], egradPt[4], energy;
    
  ELL_4V_SET(egradIm, 0, 0, 0, 0);
  ELL_4V_SET(egradPt, 0, 0, 0, 0);
  if (ignoreImage) {
    enrIm = 0;
  } else {
    enrIm = _energyFromImage(task, point, egrad ? egradIm : NULL);
  }
  enrPt = _pullEnergyFromPoints(task, bin, point, egrad ? egradPt : NULL);
  energy = AIR_LERP(task->pctx->alpha, enrIm, enrPt);
  /*
  fprintf(stderr, "!%s(%u): energy = lerp(%g, im %g, pt %g) = %g\n", me,
          point->idtag, task->pctx->alpha, enrIm, enrPt, energy);
  */
  if (egrad) {
    ELL_4V_LERP(egrad, task->pctx->alpha, egradIm, egradPt);
    /*
    fprintf(stderr, "!%s(%u): egradIm = %g %g %g %g\n", me, point->idtag,
            egradIm[0], egradIm[1], egradIm[2], egradIm[3]);
    fprintf(stderr, "!%s(%u): egradPt = %g %g %g %g\n", me, point->idtag,
            egradPt[0], egradPt[1], egradPt[2], egradPt[3]);
    fprintf(stderr, "!%s(%u): ---> force = %g %g %g %g\n", me,
            point->idtag, force[0], force[1], force[2], force[3]);
    */
  }
  if (task->pctx->wall) {
    unsigned int axi;
    double dwe; /* derivative of wall energy */
    for (axi=0; axi<4; axi++) {
      dwe = point->pos[axi] - task->pctx->bboxMin[axi];
      if (dwe > 0) {
        /* pos not below min */
        dwe = point->pos[axi] - task->pctx->bboxMax[axi];
        if (dwe < 0) {
          /* pos not above max */
          dwe = 0;
        }
      } 
      energy += task->pctx->wall*dwe*dwe/2;
      if (egrad) {
        egrad[axi] += task->pctx->wall*dwe;
      }
    }
  }
  if (!AIR_EXISTS(energy)) {
    fprintf(stderr, "!%s(%u): HEY! non-exist energy %g\n", me, point->idtag,
            energy);
  }
  return energy;
}

/*
** distance limit on a particles motion in both r and s,
** in rs-normalized space (sqrt((r/radiusSpace)^2 + (s/radiusScale)^2))
**
** This means that if particles are jammed together in space,
** they aren't allowed to move very far in scale, either, which
** is a little weird, but probably okay.
*/
double
_pullDistLimit(pullTask *task, pullPoint *point) {
  double ret;

  if (point->neighDistMean == 0 /* no known neighbors from last iter */
      || pullEnergyZero == task->pctx->energySpec->energy) {
    ret = 1;
  } else {
    ret = _PULL_DIST_CAP_SCALE*point->neighDistMean;
  }
  /* HEY: task->pctx->constraintVoxelSize might be considered here */
  return ret;
}

/*
** here is where the energy gradient is converted into force
*/
int
_pullPointProcessDescent(pullTask *task, pullBin *bin, pullPoint *point,
                         int ignoreImage) {
  char me[]="pullPointProcessDescent", err[BIFF_STRLEN];
  double energyOld, energyNew, egrad[4], force[4], posOld[4];
  int stepBad, giveUp;

  if (!point->stepEnergy) {
    sprintf(err, "%s: whoa, point %u step is zero!", me, point->idtag);
    biffAdd(PULL, err); return 1;
  }
  
  /* learn the energy at existing location, and the energy gradient */
  energyOld = _pullPointEnergyTotal(task, bin, point, ignoreImage, egrad);
  ELL_4V_SCALE(force, -1, egrad);
  if (!( AIR_EXISTS(energyOld) && ELL_4V_EXISTS(force) )) {
    sprintf(err, "%s: point %u non-exist energy or force", me, point->idtag);
    biffAdd(PULL, err); return 1;
  }
  if (252 == point->idtag) {
    fprintf(stderr, "!%s(%u): old pos = %g %g %g %g\n", me, point->idtag,
            point->pos[0], point->pos[1],
            point->pos[2], point->pos[3]);
    fprintf(stderr, "!%s(%u): energyOld = %g; force = %g %g %g %g\n", me,
            point->idtag, energyOld, force[0], force[1], force[2], force[3]);
  }
  
  if (!ELL_4V_DOT(force, force)) {
    /* this particle has no reason to go anywhere; we're done with it */
    point->energy = energyOld;
    return 0;
  }

  if (task->pctx->constraint) {
    /* we have a constraint, so do something to get the force more
       tangential to the constraint surface (only in the spatial axes) */
    double proj[9], pfrc[3];
    _pullConstraintTangent(task, point, proj);
    ELL_3MV_MUL(pfrc, proj, force);
    ELL_3V_COPY(force, pfrc);
    /* force[3] untouched */
  }

  if (252 == point->idtag) {
    fprintf(stderr, "!%s(%u): post-constraint tan: force = %g %g %g %g\n", me,
            point->idtag, force[0], force[1], force[2], force[3]);
    fprintf(stderr, "   precap stepEnergy = %g\n", point->stepEnergy);
  }

  /* Cap particle motion. The point is only allowed to move at most unit
     distance in rs-normalized space, which may mean that motion in r
     or s is effectively cramped by crowding in the other axis, oh well */
  if (1) {
    double capvec[4], nspcLen, nsclLen, max, distLimit;
    
    distLimit = _pullDistLimit(task, point);
    ELL_4V_SCALE(capvec, point->stepEnergy, force);
    nspcLen = ELL_3V_LEN(capvec)/task->pctx->radiusSpace;
    nsclLen = AIR_ABS(capvec[3])/task->pctx->radiusScale;
    max = AIR_MAX(nspcLen, nsclLen);
    if (max > distLimit) {
      point->stepEnergy *= distLimit/max;
    }
  }

  if (252 == point->idtag) {
    fprintf(stderr, "  postcap stepEnergy = %g\n", point->stepEnergy);
  }
  point->status &= ~PULL_STATUS_STUCK_BIT; /* turn off stuck bit */
  ELL_4V_COPY(posOld, point->pos);
  _pullPointHistInit(point);
  _pullPointHistAdd(point, pullCondOld);
  /* try steps along force until we succcessfully lower energy */
  do {
    int constrFail;
    
    giveUp = AIR_FALSE;
    ELL_4V_SCALE_ADD2(point->pos, 1.0, posOld,
                      point->stepEnergy, force);
    if (252 == point->idtag) {
      fprintf(stderr, "!%s(%u):  try pos   = %g %g %g %g\n", me, point->idtag,
              point->pos[0], point->pos[1],
              point->pos[2], point->pos[3]);
    }
    if (task->pctx->haveScale) {
      point->pos[3] = AIR_CLAMP(task->pctx->bboxMin[3], 
                                point->pos[3],
                                task->pctx->bboxMax[3]);
    }
    _pullPointHistAdd(point, pullCondEnergyTry);
    if (task->pctx->constraint) {
      if (_pullConstraintSatisfy(task, point, &constrFail)) {
        sprintf(err, "%s: trouble", me);
        biffAdd(PULL, err); return 1;
      }
    } else {
      constrFail = AIR_FALSE;
    }
    if (252 == point->idtag) {
      fprintf(stderr, "!%s(%u): post constr = %g %g %g %g (%d)\n", me,
              point->idtag,
              point->pos[0], point->pos[1],
              point->pos[2], point->pos[3], constrFail);
    }
    if (constrFail) {
      energyNew = AIR_NAN;
    } else {
      energyNew = _pullPointEnergyTotal(task, bin, point, ignoreImage, NULL);
    }
    if (252 == point->idtag) {
      fprintf(stderr, "!%s(%u): energyNew = %g \n", me,
              point->idtag, energyNew);
    }
    stepBad = (constrFail 
               || (energyNew > energyOld + task->pctx->energyIncreasePermit));
    if (stepBad) {
      point->stepEnergy *= task->pctx->stepScale;
      if (constrFail) {
        _pullPointHistAdd(point, pullCondConstraintFail);
      } else {
        _pullPointHistAdd(point, pullCondEnergyBad);
      }
      /* you have a problem if you had a non-trivial force, but you can't
         ever seem to take a small enough step to reduce energy */
      if (point->stepEnergy < 0.000000000000001) {
        if (task->pctx->verbose > 1) {
          fprintf(stderr, "%s: %u STUCK; (%g,%g,%g,%g) stepEnr %g\n", me,
                  point->idtag, 
                  point->pos[0], point->pos[1], point->pos[2], point->pos[3],
                  point->stepEnergy);
        }
        /* This point is fuct, may as well reset its step, maybe things
           will go better next time.  Without this resetting, it will stay
           effectively frozen */
        ELL_4V_COPY(point->pos, posOld);
        if (_pullProbe(task, point)) {
          sprintf(err, "%s: problem returning %u to %g %g %g %g", me,
                  point->idtag, point->pos[0], point->pos[1],
                  point->pos[2], point->pos[3]);
          biffAdd(PULL, err); return 1;
        }
        energyNew = energyOld; /* to be copied into point->energy below */
        point->stepEnergy = task->pctx->stepInitial;
        point->status |= PULL_STATUS_STUCK_BIT;
        giveUp = AIR_TRUE;
      }
    }
  } while (stepBad && !giveUp);
  /* now: energy decreased, and, if we have one, constraint has been met */
  if (252 == point->idtag) {
    fprintf(stderr, "!%s(%u): changed (%g,%g,%g,%g) -> (%g,%g,%g,%g)\n",
            me, point->idtag,
            posOld[0], posOld[1], posOld[2], posOld[3],
            point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
  }
  _pullPointHistAdd(point, pullCondNew);
  ELL_4V_COPY(point->force, force);

  /* not recorded for the sake of this function, but for system accounting */
  point->energy = energyNew;
  if (!AIR_EXISTS(energyNew)) {
    sprintf(err, "%s: point %u has non-exist final energy %g\n", 
            me, point->idtag, energyNew);
    biffAdd(PULL, err); return 1;
  }

  return 0;
}

int
_pullPointProcess(pullTask *task, pullBin *bin, pullPoint *point) {
  char me[]="_pullPointProcess", err[BIFF_STRLEN];
  int E;

  E = 0;
  switch (task->processMode) {
  case pullProcessModeDescent:
    E = _pullPointProcessDescent(task, bin, point,
                                 AIR_FALSE /* ignoreImage */);
    break;
  case pullProcessModeNeighLearn:
    E = _pullPointProcessNeighLearn(task, bin, point);
    break;
  case pullProcessModeAdding:
    E = _pullPointProcessAdding(task, bin, point);
    break;
  case pullProcessModeNixing:
    E = _pullPointProcessNixing(task, bin, point);
    break;
  default:
    sprintf(err, "%s: process mode %d unrecognized", me, task->processMode);
    biffAdd(PULL, err); return 1;
    break;
  }
  if (E) {
    sprintf(err, "%s: trouble", me);
    biffAdd(PULL, err); return 1;
  }
  return 0;
}

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
    task->stuckNum += (myBin->point[myPointIdx]->status
                       & PULL_STATUS_STUCK_BIT);
  } /* for myPointIdx */

  return 0;
}
