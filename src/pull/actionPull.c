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

/*
** issues:
** does everything work on the first iteration
** how to handle the needed extra probe for d strength / d scale
** how are force/energy along scale handled differently than in space?
*/

#ifdef PRAY
int _pullPraying = 0;
double _pullPrayCorner[2][2][3];
size_t _pullPrayRes[2] = {60,20};
#endif

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
                     double *spadistP, double egrad[4]) {
  char meme[]="_energyInterParticle";
  double spadist,scaledist, sparad, scalerad, diff[4], rr, rrs, enr, frc, *parm;
  double enrTotal;
  ELL_4V_SUB(diff, she->pos, me->pos);
  spadist = ELL_3V_LEN(diff);
  if (spadistP) {
    *spadistP = spadist;
  }
  sparad = task->pctx->radiusSpace;
  rr = spadist/(2*sparad);
  scaledist = AIR_ABS(diff[3]);
  scalerad = task->pctx->radiusScale;
  rrs = scaledist/(2*scalerad);  

  /*
  fprintf(stderr, "!%s: rr(%u,%u) = %g\n", meme, me->idtag, she->idtag, rr);
  */
  if (rr > 1 || rrs > 1) {
    ELL_4V_SET(egrad, 0, 0, 0, 0);
    return 0;
  }
  if (rr == 0 && rrs == 0) {
    fprintf(stderr, "%s: pos of pts %u, %u equal: (%g,%g,%g,%g)\n",
            meme, me->idtag, she->idtag, 
            me->pos[0], me->pos[1], me->pos[2], me->pos[3]);
    ELL_4V_SET(egrad, 0, 0, 0, 0);
    return 0;
  }

  if (!task->pctx->haveScale) {
    parm = task->pctx->energySpec->parm;
    enr = task->pctx->energySpec->energy->eval(&frc, rr, parm);
    frc *= -1.0/(2*sparad*spadist);
    ELL_3V_SCALE(egrad, frc, diff);
    egrad[3] = 0;
    enrTotal = enr;
  } else {
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
** point->neighDist and point->neighMode
**
** NULL egradSum: just tell me what the energy is; and do NOT compute:
** point->neighInterNum, point->neighDist, point->neighMode
*/
static double
_energyFromPoints(pullTask *task, pullBin *bin, pullPoint *point, 
                  /* output */
                  double egradSum[4]) {
  /* char me[]="_energyFromPoints"; */
  double energySum, spaDistSqMax, distWghtSum, modeWghtSum;
  int nopt,     /* optimiziation: we enable the re-use neighbor lists, or
                   initially, the creation of neighbor lists */
    ntrue;      /* we search all possible neighbors, stored in the bins
                   (either because !nopt, or, this iter we learn true
                   subset of interacting neighbors).  This could also
                   be called "dontreuse" or something like that */
  unsigned int nidx,
    nnum;       /* how much of task->neigh[] we use */

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
  /* NOTE that you can't have both nopt and ntrue be false */
  /*
  fprintf(stderr, "!%s(%u), nopt = %d, ntrue = %d\n", me, point->idtag,
          nopt, ntrue);
  */
  /* set nnum and task->neigh[] */
  if (ntrue) {
    nnum = _neighBinPoints(task, bin, point);
    if (nopt) {
      airArrayLenSet(point->neighPointArr, 0);
    }
  } else {
    /* (nopt true) this iter we re-use existing neighbor list */
    nnum = point->neighPointNum;
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
  distWghtSum = 0;
  modeWghtSum = 0;
  energySum = 0;
  if (egradSum) {
    point->neighInterNum = 0;
    point->neighDist = 0.0;
    point->neighMode = 0.0;
  }
  if (egradSum) {
    ELL_4V_SET(egradSum, 0, 0, 0, 0);
  }
  for (nidx=0; nidx<nnum; nidx++) {
    double diff[4], spaDistSq, spaDist, enr, egrad[4];
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
    /* this uses sqrt() to get the real 3D distance (not in SS) */
    enr = _energyInterParticle(task, point, herPoint, &spaDist, egrad);
    /*
    fprintf(stderr, "!%s: energySum = %g + %g = %g\n", me, 
            energySum, enr, energySum + enr);
    */
    energySum += enr;
    if (egradSum) {
      ELL_4V_INCR(egradSum, egrad);
      if (ELL_4V_DOT(egrad, egrad)) {
        double ww, normdist;
        point->neighInterNum++;
        normdist = spaDist/task->pctx->radiusSpace;
        ww = normdist*normdist*normdist*normdist;
        ww = 1.0/(ww*ww*ww*ww); /* Lehmer mean with p-1==16 */
        point->neighDist += ww*normdist;
        distWghtSum += ww;
        if (task->pctx->ispec[pullInfoTangentMode]) {
          double mm;
          mm = _pullPointScalar(task->pctx, herPoint, pullInfoTangentMode,
                                NULL, NULL);
          ww = 1.0/spaDistSq;
          point->neighMode += ww*mm;
          modeWghtSum += ww;
        }
        if (nopt && ntrue) {
          unsigned int ii;
          ii = airArrayLenIncr(point->neighPointArr, 1);
          point->neighPoint[ii] = herPoint;
        }
      }
    }
  }
  
  /* finish computing things averaged over neighbors */
  if (egradSum) {
    if (point->neighInterNum) {
      point->neighDist /= distWghtSum;
      point->neighMode /= modeWghtSum;
    } else {
      point->neighDist = 0.0; /* shouldn't happen in any normal case */
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
  if (task->pctx->ispec[pullInfoHeight]
      && !task->pctx->ispec[pullInfoHeight]->constraint
      && (!task->pctx->ispec[pullInfoHeightLaplacian]
          || !task->pctx->ispec[pullInfoHeightLaplacian]->constraint)) {
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
  return energy;
}
#undef MAYBEPROBE

/*
** its in here that we scale from "energy gradient" to "force"
**
** NOTE that the "force" being non-NULL has consequences for what gets
** computed in _energyFromImage and _energyFromPoints:
**
** NULL "force": we're simply learning the energy (and want to know it
** as truthfully as possible) for the sake of inspecting system state
**
** non-NULL "force": we're learning the current energy, but the real point
** is to determine how to move the point to lower energy
*/
double
_pullPointEnergyTotal(pullTask *task, pullBin *bin, pullPoint *point,
                      /* output */
                      double force[4]) {
  char me[]="_pullPointEnergyTotal";
  double enrIm, enrPt, egradIm[4], egradPt[4], energy;
    
  ELL_4V_SET(egradIm, 0, 0, 0, 0); /* sssh */
  ELL_4V_SET(egradPt, 0, 0, 0, 0); /* sssh */
  enrIm = _energyFromImage(task, point, 
                           force ? egradIm : NULL);
  enrPt = _energyFromPoints(task, bin, point,
                            force ? egradPt : NULL);
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
  if (!AIR_EXISTS(energy)) {
    fprintf(stderr, "!%s(%u): HEY! non-exist energy %g\n", me, point->idtag,
            energy);
  }
  return energy;
}

double
_pullDistLimit(pullTask *task, pullPoint *point) {
  double ret;

  if (point->neighDist < 0 /* no neighbors! */
      || pullEnergyZero == task->pctx->energySpec->energy) {
    ret = task->pctx->radiusSpace;
  } else {
    ret = point->neighDist*task->pctx->radiusSpace;
  }
  /* task->pctx->constraintVoxelSize might be considered here */
  return ret;
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

  energyOld = _pullPointEnergyTotal(task, bin, point, force);
  if (!( AIR_EXISTS(energyOld) && ELL_4V_EXISTS(force) )) {
    sprintf(err, "%s: point %u non-exist energy or force", me, point->idtag);
    biffAdd(PULL, err); return 1;
  }

  if (task->pctx->constraint) {
    /* we have a constraint, so do something to get the force more
       tangential to the constriant surface */
    double proj[9], pfrc[3];
    _pullConstraintTangent(task, point, proj);
    ELL_3MV_MUL(pfrc, proj, force);
    ELL_3V_COPY(force, pfrc);
  }

  point->status = 0; /* reset status bitflag */
  ELL_4V_COPY(posOld, point->pos);
  _pullPointHistInit(point);
  _pullPointHistAdd(point, pullCondOld);
  
  if (!ELL_4V_LEN(force)) {
    /* this particle has no reason to go anywhere; we're done with it */
    point->energy = energyOld;
    return 0;
  }
  distLimit = _pullDistLimit(task, point);

  /* find capscl */
  ELL_3V_SCALE(capvec, point->stepEnergy, force);
  caplen = ELL_3V_LEN(capvec);
  if (caplen > distLimit) {
    capscl = distLimit/caplen;
  } else {
    capscl = 1;
  }

  do {
    double scl0, scl1, scl2;
    int constrFail;
    
    scl0 = posOld[3];
    giveUp = AIR_FALSE;
    ELL_3V_SCALE_ADD2(point->pos, 1.0, posOld,
                      capscl*point->stepEnergy, force);
    if (task->pctx->haveScale) {
      point->pos[3] = posOld[3] + point->stepEnergy*force[3];
    } else {
      point->pos[3] = posOld[3];
    }
    /* Constraining scale dimension */
    scl1 = point->pos[3];
    /*
    fprintf(stderr, "!%s(%u): force[3] = %g --> %g\n",
            me, point->idtag, force[3], scl1 - scl0);
    */
    if (task->pctx->haveScale) {
      point->pos[3] = AIR_CLAMP(task->pctx->bboxMin[3], 
                                point->pos[3],
                                task->pctx->bboxMax[3]);
    }
    scl1 = point->pos[3];
    _pullPointHistAdd(point, pullCondEnergyTry);
    if (task->pctx->constraint) {
      if (_pullConstraintSatisfy(task, point, &constrFail)) {
        sprintf(err, "%s: trouble", me);
        biffAdd(PULL, err); return 1;
      }
    } else {
      constrFail = AIR_FALSE;
    }
    scl2 = point->pos[3];
    if (scl2 != scl1) {
      fprintf(stderr, "!%s: constraint sat changed scl %g -> %g\n", me,
              scl1, scl2); 
      exit(1);
    }
    if (constrFail) {
      energyNew = AIR_NAN;
    } else {
      energyNew = _pullPointEnergyTotal(task, bin, point,  NULL);
    }
    stepBad = constrFail || (energyNew > energyOld);
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
          fprintf(stderr, "%s: %u stuck; (%g,%g,%g,%g) stepEnr %g"
                  "capscl %g <<<<\n\n\n", me, point->idtag, 
                  point->pos[0], point->pos[1], point->pos[2], point->pos[3],
                  point->stepEnergy, capscl);
        }
        /* This point is fuct, may as well reset its step, maybe things
           will go better next time.  Without this resetting, it will stay
           effectively frozen */
        ELL_4V_COPY(point->pos, posOld);
        energyNew = energyOld; /* to be copied into point->energy below */
        point->stepEnergy = task->pctx->stepInitial/100;
        point->status |= PULL_STATUS_STUCK_BIT;
        giveUp = AIR_TRUE;
      }
    }  
  } while (stepBad && !giveUp);
  /* now: energy decreased, and, if we have one, constraint has been met */
  /*
  fprintf(stderr, "!%s(%u): changed (%g,%g,%g,%g) -> (%g,%g,%g,%g)\n",
          me, point->idtag,
          posOld[0], posOld[1], posOld[2], posOld[3],
          point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
  */
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

/*
** this is where point nixing/adding should happen
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
    task->stuckNum += (myBin->point[myPointIdx]->status
                       & PULL_STATUS_STUCK_BIT);
  } /* for myPointIdx */

  /* probabilistically nix points that have too much company */
  if (0 && (15 == task->pctx->iter % 20) && task->pctx->constraint) {
    pullPoint *point;
    double nixProb, ndist, wantDist=1.3, wantNum, haveNum, constrDim;
    for (myPointIdx=0; myPointIdx<myBin->pointNum; myPointIdx++) {
      point = myBin->point[myPointIdx];
      /* neighDist has already been normalized by task->pctx->radiusSpace.
         maximum ndist for interaction is 2.0 */
      ndist = point->neighDist;
      constrDim = _pullConstraintDim(task, point);
      if (task->pctx->haveScale) {
        constrDim += 1;
      }
      if (!constrDim) {
        sprintf(err, "%s: got constraint dim 0", me);
        biffAdd(PULL, err); return 1;
      }
      haveNum = pow(ndist, -constrDim);
      wantNum = pow(wantDist, -constrDim);
      nixProb = (haveNum > wantNum
                 ? (haveNum - wantNum)/haveNum
                 : 0.0);
      if (airDrandMT_r(task->rng) < nixProb) {
        point->status |= PULL_STATUS_NIXME_BIT;
      }
    }
  }

  return 0;
}
