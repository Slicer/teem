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
** does everything work on the first iteration
** how to handle the needed extra probe for d strength / d scale
** does it eventually catch non-existant energy or force
** how are force/energy along scale handled differently than in space?
*/

int praying = 0;
double prayCorner[2][2][3];
size_t prayRes[2] = {400,400};

unsigned int
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
      /* can't interact with myself */
      if (point != herPoint) {
        if (nn < _PULL_NEIGH_MAXNUM) {
          task->neighPoint[nn++] = herPoint;
          /*
          fprintf(stderr, "%s(%u): neighPoint[%u] = %u\n", 
                  me, point->idtag, nn-1, herPoint->idtag);
          */
        } else {
          fprintf(stderr, "%s: maxed out at %u points!\n", me, 
                  _PULL_NEIGH_MAXNUM);
        }
      }
    }
    herBinIdx++;
  }
  return nn;
}


double
_energyInterParticle(pullTask *task, pullPoint *me, pullPoint *she, 
                     /* output */
                     double egrad[4]) {
  char meme[]="_energyInterParticle";
  double spadist, sparad, diff[4], rr, enr, frc, *parm;

  ELL_4V_SUB(diff, she->pos, me->pos);
  spadist = ELL_3V_LEN(diff);
  sparad = task->pctx->radiusSpace;
  
  rr = spadist/(2*sparad);
  /*
  fprintf(stderr, "!%s: rr(%u,%u) = %g\n", meme, me->idtag, she->idtag, rr);
  */
  if (rr > 1) {
    ELL_4V_SET(egrad, 0, 0, 0, 0);
    return 0;
  }
  if (rr == 0) {
    fprintf(stderr, "%s: pos of pts %u, %u equal: (%g,%g,%g,%g)\n",
            meme, me->idtag, she->idtag, 
            me->pos[0], me->pos[1], me->pos[2], me->pos[3]);
    ELL_4V_SET(egrad, 0, 0, 0, 0);
    return 0;
  }

  parm = task->pctx->energySpec->parm;
  enr = task->pctx->energySpec->energy->eval(&frc, rr, parm);
  frc *= -1.0/(2*sparad*spadist);
  ELL_3V_SCALE(egrad, frc, diff);
  egrad[3] = 0;
  /*
  fprintf(stderr, "%s: %u <-- %u = %g,%g,%g -> egrad = %g,%g,%g, enr = %g\n",
          meme, me->idtag, she->idtag, 
          diff[0], diff[1], diff[2],
          egrad[0], egrad[1], egrad[2], enr);
  */
  return enr;
}

/*
** meanNeighDist is allowed to be NULL. If it is non-NULL, 
** it must be set, and must be < 0 if there are no neighbors
*/
double
_energyFromPoints(pullTask *task, pullBin *bin, pullPoint *point, 
                  /* output */
                  double egradSum[4], double *meanNeighDist) {
  /* char me[]="_energyFromPoints"; */
  double energySum, distSqSum, spaDistSqMax;
  int nopt,     /* optimiziation: we sometimes re-use neighbor lists */
    ntrue;      /* we search all possible neighbors, stored in the bins
                   (either because !nopt, or, this iter we learn true
                   subset of interacting neighbors).  This could also
                   be called "dontreuse" or something like that */
  unsigned int nidx, neighCount,
    nnum;       /* how much of task->neigh[] we use */

  if (meanNeighDist && !egradSum) {
    /* shouldn't happen */
    return AIR_NAN;
  }
  
  /* set nopt and ntrue */
  if (task->pctx->neighborTrueProb < 1) {
    nopt = AIR_TRUE;
    if (egradSum) {
      /* We allow the neighbor list optimization only when we're also asked
         to compute the energy gradient.  When we're not getting the energy
         gradient, we're being called to test the waters at possible new
         locations, in which case we can't be changing the effective particle 
         neighborhood */
      ntrue = (0 == task->pctx->iter
               || airDrandMT_r(task->rng) < task->pctx->neighborTrueProb);
    } else {
      ntrue = AIR_FALSE;
    }
  } else {
    nopt = AIR_FALSE;
    ntrue = AIR_TRUE;
  }
  /*
  fprintf(stderr, "!%s(%u), nopt = %d, ntrue = %d\n", me, point->idtag,
          nopt, ntrue);
  */
  /* set nnum and task->neigh[] */
  if (ntrue) {
    nnum = _neighBinPoints(task, bin, point);
    if (nopt) {
      airArrayLenSet(point->neighArr, 0);
    }
  } else {
    /* (nopt true) this iter we re-use existing neighbor list */
    nnum = point->neighNum;
    for (nidx=0; nidx<nnum; nidx++) {
      task->neighPoint[nidx] = point->neighPoint[nidx];
    }
  }

  /* loop through neighbor points */
  spaDistSqMax = 4*task->pctx->radiusSpace*task->pctx->radiusSpace;
  /*
  fprintf(stderr, "%s: radiusSpace = %g -> spaDistSqMax = %g\n", me,
          task->pctx->radiusSpace, spaDistSqMax);
  */
  energySum = 0;
  distSqSum = 0;
  neighCount = 0;
  if (egradSum) {
    ELL_4V_SET(egradSum, 0, 0, 0, 0);
  }
  for (nidx=0; nidx<nnum; nidx++) {
    double diff[4], spaDistSq, enr, egrad[4];
    pullPoint *herPoint;
    herPoint = task->neighPoint[nidx];
    ELL_4V_SUB(diff, herPoint->pos, point->pos);
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
    if (AIR_ABS(diff[3] > task->pctx->radiusScale)) {
      continue;
    }
    enr = _energyInterParticle(task, point, herPoint, egrad);
    /*
    fprintf(stderr, "!%s: energySum = %g + %g = %g\n", me, 
            energySum, enr, energySum + enr);
    */
    energySum += enr;
    if (egradSum) {
      ELL_4V_INCR(egradSum, egrad);
      if (ELL_4V_DOT(egrad, egrad)) {
        if (meanNeighDist) {
          neighCount++;
          distSqSum += spaDistSq;
        }
        if (nopt && ntrue) {
          unsigned int ii;
          ii = airArrayLenIncr(point->neighArr, 1);
          point->neighPoint[ii] = herPoint;
        }
      }
    }
  }
  
  /* finish computing mean distance to neighbors */
  if (meanNeighDist) {
    if (neighCount) {
      *meanNeighDist = sqrt(distSqSum/neighCount);
    } else {
      *meanNeighDist = -1;
    }
  }

  return energySum;
}

double
_energyFromImage(pullTask *task, pullPoint *point,
                 /* output */
                 double egradSum[4]) {
  char me[]="_energyFromImage";
  double energy, grad3[3];

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
  if (_pullProbe(task, point)) {
    fprintf(stderr, "%s: problem probing!!!\n%s\n", me, biffGetDone(PULL));
  }
  energy = 0;
  if (egradSum) {
    ELL_4V_SET(egradSum, 0, 0, 0, 0);
  }
  if (task->pctx->ispec[pullInfoHeight]
      && !task->pctx->ispec[pullInfoHeight]->constraint
      && !task->pctx->ispec[pullInfoHeightLaplacian]->constraint) {
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
    val = _pullPointScalar(task->pctx, point, pullInfoIsovalue,
                           grad3, NULL);
    energy += val*val;
    if (egradSum) {
      ELL_3V_SCALE_INCR(egradSum, 2*val, grad3);
    }
  }
  return energy;
}

/*
** its in here that we scale from "energy gradient" to "force"
*/
double
_pullPointEnergyTotal(pullTask *task, pullBin *bin, pullPoint *point,
                      /* output */
                      double force[4], double *neighDist) {
  /* char me[]="_pullPointEnergyTotal"; */
  double enrIm, enrPt, egradIm[4], egradPt[4], energy;
    
  ELL_4V_SET(egradIm, 0, 0, 0, 0); /* sssh */
  ELL_4V_SET(egradPt, 0, 0, 0, 0); /* sssh */
  enrIm = _energyFromImage(task, point, 
                           force ? egradIm : NULL);
  enrPt = _energyFromPoints(task, bin, point,
                            force ? egradPt : NULL, neighDist);
  energy = AIR_LERP(task->pctx->alpha, enrIm, enrPt);
  /*
  fprintf(stderr, "!%s(%u): energy = lerp(%g, im %g, pt %g) = %g\n", me,
          point->idtag, task->pctx->alpha, enrIm, enrPt, energy);
  */
  if (force) {
    ELL_4V_LERP(force, task->pctx->alpha, egradIm, egradPt);
    ELL_4V_SCALE(force, -1, force);
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
    double frc;
    for (axi=0; axi<4; axi++) {
      frc = task->pctx->bboxMin[axi] - point->pos[axi];
      if (frc < 0) {
        /* not below min */
        frc = task->pctx->bboxMax[axi] - point->pos[axi];
        if (frc > 0) {
          /* not above max */
          frc = 0;
        }
      } 
      energy += task->pctx->wall*frc*frc/2;
      if (force) {
        force[axi] += task->pctx->wall*frc;
      }
    }
  }
  return energy;
}

int
_pullPointProcess(pullTask *task, pullBin *bin, pullPoint *point) {
  char me[]="pullPointProcess", err[BIFF_STRLEN];
  double energyOld, energyNew, force[4], distLimit, posOld[4],
    capvec[3], caplen, capscl;  /* related to capping distance traveled
                                   in a per-iteration way */
  int stepBad, giveUp;

  if (!point->stepEnergy) {
    sprintf(err, "%s: whoa, point %u step is zero!", me, point->idtag);
    biffAdd(PULL, err); return 1;
  }

  if (0 && 541 == point->idtag && 10 == task->pctx->iter) {
    fprintf(stderr, "!%s: !!!!!!!!!!!! praying ON!\n", me);
    praying = AIR_TRUE;
    ELL_3V_COPY(prayCorner[0][0], point->pos);
  } else {
    praying = AIR_FALSE;
  }

  if (praying) {
    fprintf(stderr, "%s: =============================== (%u) hi @ %g %g %g\n",
            me, point->idtag, point->pos[0], point->pos[1], point->pos[2]);
  }
  energyOld = _pullPointEnergyTotal(task, bin, point, force, &distLimit);
  if (praying) {
    fprintf(stderr, "!%s: =================== point %u has:\n "
            "     energy = %g ; ndist = %g, force %g %g %g %g\n", me,
            point->idtag, energyOld, distLimit, 
            force[0], force[1], force[2], force[3]);
  }
  if (!( AIR_EXISTS(energyOld) && ELL_4V_EXISTS(force) )) {
    sprintf(err, "%s: point %u non-exist energy or force", me, point->idtag);
    biffAdd(PULL, err); return 1;
  }

  if (task->pctx->constraint) {
    /* we have a constraint, so do something to get the force more
       tangential to the constriant surface */
    double vec[4], proj[9], nvec[3], outer[9], pfrc[3], len;

    switch (task->pctx->constraint) {
    case pullInfoHeight:
      /* HEY: need an approximate constraint surface tangent */
      ELL_3V_SET(vec, 0, 0, 0);
      break;
    case pullInfoHeightLaplacian:
      _pullPointScalar(task->pctx, point, pullInfoHeight, vec, NULL);
      break;
    case pullInfoIsovalue:
      _pullPointScalar(task->pctx, point, pullInfoIsovalue, vec, NULL);
      break;
    }
    ELL_3V_NORM(nvec, vec, len);
    if (len) {
      ELL_3MV_OUTER(outer, nvec, nvec);
      ELL_3M_IDENTITY_SET(proj);
      ELL_3M_SUB(proj, proj, outer);
      ELL_3MV_MUL(pfrc, proj, force);
      ELL_3V_COPY(force, pfrc);
    }
    /* if !len, gradient is zero, and no change is made */
  }

  point->status = 0;
  ELL_4V_COPY(posOld, point->pos);
  _pullPointHistInit(point);
  _pullPointHistAdd(point, pullCondOld);
  
  if (!ELL_4V_LEN(force)) {
    /* this particle has no reason to go anywhere; we're done with it */
    point->energy = energyOld;
    return 0;
  }
  if (distLimit < 0 /* no neighbors! */
      || pullEnergyZero == task->pctx->energySpec->energy) {
    distLimit = task->pctx->radiusSpace;
  }
  /* maybe voxel size should also be considered for finding distLimit */

  /* find capscl */
  ELL_3V_SCALE(capvec, point->stepEnergy, force);
  caplen = ELL_3V_LEN(capvec);
  if (caplen > distLimit) {
    capscl = distLimit/caplen;
  } else {
    capscl = 1;
  }
  if (praying) {
    fprintf(stderr, "%s: ======= (%u) capscl = %g\n", me, point->idtag, capscl);
  }

  do {
    int constrFail;
    
    giveUp = AIR_FALSE;
    ELL_4V_SCALE_ADD2(point->pos, 1.0, posOld,
                      capscl*point->stepEnergy, force);

    _pullPointHistAdd(point, pullCondEnergyTry);
    if (praying) {
      fprintf(stderr, "!%s: ======= (%u) try step %g to pos %g %g %g %g\n", me,
              point->idtag, capscl*point->stepEnergy, 
              point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
    }
    if (task->pctx->constraint) {
      if (_pullConstraintSatisfy(task, point, &constrFail)) {
        sprintf(err, "%s: trouble", me);
        biffAdd(PULL, err); return 1;
      }
    } else {
      constrFail = AIR_FALSE;
    }
    if (constrFail) {
      energyNew = AIR_NAN;
    } else {
      energyNew = _pullPointEnergyTotal(task, bin, point,  NULL, NULL);
      if (praying) {
        fprintf(stderr, "!%s: ======= e new = %g %s old %g %s\n", me, energyNew,
                energyNew > energyOld ? ">" : "<=", energyOld,
                energyNew > energyOld ? "!! BADSTEP !!" : "ok");
      }
    }
    stepBad = constrFail || (energyNew > energyOld);
    if (stepBad) {
      point->stepEnergy *= task->pctx->stepScale;
      _pullPointHistAdd(point, pullCondEnergyBad);
      /* you have a problem if you had a non-trivial force, but you can't
         ever seem to take a small enough step to reduce energy */
      if (point->stepEnergy < 0.000000000000001) {
        fprintf(stderr, "\n%s: %u (%g,%g,%g,%g) stepEnr %g stuck; "
                "capscl %g <<<<\n\n\n", 
                me, point->idtag, 
                point->pos[0], point->pos[1], point->pos[2], point->pos[3],
                point->stepEnergy, capscl);
        /* This point is fuct, may as well reset its step, maybe things
           will go better next time.  Without this resetting, it will stay
           effectively frozen */
        ELL_4V_COPY(point->pos, posOld);
        point->stepEnergy = task->pctx->stepInitial;
        point->status = 1;
        giveUp = AIR_TRUE;
      }
    }  
  } while (stepBad && !giveUp);
  /* now: energy decreased, and, if we have one, constraint has been met */

  _pullPointHistAdd(point, pullCondNew);
  ELL_4V_COPY(point->force, force);

  /* not recorded for the sake of this function, but for system accounting */
  point->energy = energyNew;

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
    task->stuckNum += myBin->point[myPointIdx]->status;
  } /* for myPointIdx */

  return 0;
}
