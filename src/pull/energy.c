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

#include "pull.h"
#include "privatePull.h"

#define SPRING  "spring"
#define GAUSS   "gauss"
#define COULOMB "coulomb"
#define COTAN   "cotan"
#define QUARTIC "quartic"
#define ZERO    "zero"

char
_pullEnergyTypeStr[PULL_ENERGY_TYPE_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_energy)",
  SPRING,
  GAUSS,
  COULOMB,
  COTAN,
  QUARTIC,
  ZERO
};

char
_pullEnergyTypeDesc[PULL_ENERGY_TYPE_MAX+1][AIR_STRLEN_MED] = {
  "unknown_energy",
  "Hooke's law-based potential, with a tunable region of attraction",
  "Gaussian potential",
  "Coulomb electrostatic potential, with tunable cut-off",
  "Cotangent-based potential (from Meyer et al. SMI '05)",
  "Quartic thing",
  "no energy"
};

airEnum
_pullEnergyType = {
  "energy",
  PULL_ENERGY_TYPE_MAX,
  _pullEnergyTypeStr,  NULL,
  _pullEnergyTypeDesc,
  NULL, NULL,
  AIR_FALSE
};
airEnum *
pullEnergyType = &_pullEnergyType;

/* ----------------------------------------------------------------
** ------------------------------ UNKNOWN -------------------------
** ----------------------------------------------------------------
*/
void
_pullEnergyUnknownEval(double *enr, double *frc,
                       double dist, const double *parm) {
  char me[]="_pullEnergyUnknownEval";

  AIR_UNUSED(dist);
  AIR_UNUSED(parm);
  *enr = AIR_NAN;
  *frc = AIR_NAN;
  fprintf(stderr, "%s: ERROR- using unknown energy.\n", me);
  return;
}

double
_pullEnergyUnknownSupport(const double *parm) {
  char me[]="_pullEnergyUnknownSupport";

  AIR_UNUSED(parm);
  fprintf(stderr, "%s: ERROR- using unknown energy.\n", me);
  return AIR_NAN;
}

pullEnergy
_pullEnergyUnknown = {
  "unknown",
  0,
  _pullEnergyUnknownEval,
  _pullEnergyUnknownSupport
};
const pullEnergy *const
pullEnergyUnknown = &_pullEnergyUnknown;

/* ----------------------------------------------------------------
** ------------------------------ SPRING --------------------------
** ----------------------------------------------------------------
** 1 parms:
** parm[0]: width of pull region (beyond 1.0)
**
** learned: "1/2" is not 0.5 !!!!!
*/
void
_pullEnergySpringEval(double *enr, double *frc,
                      double dist, const double *parm) {
  /* char me[]="_pullEnergySpringEval"; */
  double xx, pull;

  pull = parm[0];
  xx = dist - 1.0;
  if (xx > pull) {
    *enr = 0;
    *frc = 0;
  } else if (xx > 0) {
    *enr = xx*xx*(xx*xx/(4*pull*pull) - 2*xx/(3*pull) + 1.0/2.0);
    *frc = xx*(xx*xx/(pull*pull) - 2*xx/pull + 1);
  } else {
    *enr = xx*xx/2;
    *frc = xx;
  }
  /*
  if (!AIR_EXISTS(ret)) {
    fprintf(stderr, "!%s: dist=%g, pull=%g, blah=%d --> ret=%g\n",
            me, dist, pull, blah, ret);
  }
  */
  return;
}

double
_pullEnergySpringSupport(const double *parm) {

  return 1.0 + parm[0];
}

const pullEnergy
_pullEnergySpring = {
  SPRING,
  1,
  _pullEnergySpringEval,
  _pullEnergySpringSupport
};
const pullEnergy *const
pullEnergySpring = &_pullEnergySpring;

/* ----------------------------------------------------------------
** ------------------------------ GAUSS --------------------------
** ----------------------------------------------------------------
** 1 parms:
** (distance to inflection point of force function is always 1.0)
** parm[0]: cut-off (as a multiple of standard dev (which is 1.0))
*/
/* HEY: copied from teem/src/nrrd/kernel.c */
#define _GAUSS(x, sig, cut) ( \
   x >= sig*cut ? 0           \
   : exp(-x*x/(2.0*sig*sig))/(sig*2.50662827463100050241))

#define _DGAUSS(x, sig, cut) ( \
   x >= sig*cut ? 0            \
   : -exp(-x*x/(2.0*sig*sig))*x/(sig*sig*sig*2.50662827463100050241))

void
_pullEnergyGaussEval(double *enr, double *frc,
                     double dist, const double *parm) {
  double cut;

  cut = parm[0];
  *enr = _GAUSS(dist, 1.0, cut);
  *frc = _DGAUSS(dist, 1.0, cut);
  return;
}

double
_pullEnergyGaussSupport(const double *parm) {

  return parm[0];
}

const pullEnergy
_pullEnergyGauss = {
  GAUSS,
  1,
  _pullEnergyGaussEval,
  _pullEnergyGaussSupport
};
const pullEnergy *const
pullEnergyGauss = &_pullEnergyGauss;

/* ----------------------------------------------------------------
** ------------------------------ CHARGE --------------------------
** ----------------------------------------------------------------
** 1 parms:
** (scale: distance to "1.0" in graph of x^(-2))
** parm[0]: cut-off (as multiple of "1.0")
*/
void
_pullEnergyCoulombEval(double *enr, double *frc,
                       double dist, const double *parm) {

  *enr = (dist > parm[0] ? 0 : 1.0/dist);
  *frc = (dist > parm[0] ? 0 : -1.0/(dist*dist));
  return;
}

double
_pullEnergyCoulombSupport(const double *parm) {

  return parm[0];
}

const pullEnergy
_pullEnergyCoulomb = {
  COULOMB,
  1,
  _pullEnergyCoulombEval,
  _pullEnergyCoulombSupport
};
const pullEnergy *const
pullEnergyCoulomb = &_pullEnergyCoulomb;

/* ----------------------------------------------------------------
** ------------------------------ COTAN ---------------------------
** ----------------------------------------------------------------
** 0 parms!
*/
void
_pullEnergyCotanEval(double *enr, double *frc,
                     double dist, const double *parm) {
  double pot, cc;

  AIR_UNUSED(parm);
  pot = AIR_PI/2.0;
  cc = 1.0/(FLT_MIN + tan(dist*pot));
  *enr = dist > 1 ? 0 : cc + dist*pot - pot;
  *frc = dist > 1 ? 0 : -cc*cc*pot;
  return;
}

double
_pullEnergyCotanSupport(const double *parm) {

  AIR_UNUSED(parm);
  return 1;
}

const pullEnergy
_pullEnergyCotan = {
  COTAN,
  0,
  _pullEnergyCotanEval,
  _pullEnergyCotanSupport
};
const pullEnergy *const
pullEnergyCotan = &_pullEnergyCotan;

/* ----------------------------------------------------------------
** ----------------------------- QUARTIC --------------------------
** ----------------------------------------------------------------
** 0 parms!
*/
void
_pullEnergyQuarticEval(double *enr, double *frc,
                       double dist, const double *parm) {
  double omr;

  AIR_UNUSED(parm);
  if (dist <= 1) {
    omr = 1 - dist;
    *enr = 2.132*omr*omr*omr*omr;
    *frc = -4*2.132*omr*omr*omr;
  } else {
    *enr = *frc = 0;
  }
  return;
}

double
_pullEnergyQuarticSupport(const double *parm) {

  AIR_UNUSED(parm);
  return 1;
}

const pullEnergy
_pullEnergyQuartic = {
  QUARTIC,
  0,
  _pullEnergyQuarticEval,
  _pullEnergyQuarticSupport
};
const pullEnergy *const
pullEnergyQuartic = &_pullEnergyQuartic;

/* ----------------------------------------------------------------
** ------------------------------- ZERO ---------------------------
** ----------------------------------------------------------------
** 0 parms:
*/
void
_pullEnergyZeroEval(double *enr, double *frc,
                    double dist, const double *parm) {

  AIR_UNUSED(dist);
  AIR_UNUSED(parm);
  *enr = 0;
  *frc = 0;
  return;
}

double
_pullEnergyZeroSupport(const double *parm) {

  AIR_UNUSED(parm);
  return 1.0;
}

const pullEnergy
_pullEnergyZero = {
  ZERO,
  0,
  _pullEnergyZeroEval,
  _pullEnergyZeroSupport
};
const pullEnergy *const
pullEnergyZero = &_pullEnergyZero;

/* ----------------------------------------------------------------
** ----------------------------------------------------------------
** ----------------------------------------------------------------
*/

const pullEnergy *const pullEnergyAll[PULL_ENERGY_TYPE_MAX+1] = {
  &_pullEnergyUnknown,  /* 0 */
  &_pullEnergySpring,   /* 1 */
  &_pullEnergyGauss,    /* 2 */
  &_pullEnergyCoulomb,  /* 3 */
  &_pullEnergyCotan,    /* 4 */
  &_pullEnergyQuartic,  /* 5 */
  &_pullEnergyZero      /* 6 */
};

pullEnergySpec *
pullEnergySpecNew() {
  pullEnergySpec *ensp;
  int pi;

  ensp = (pullEnergySpec *)calloc(1, sizeof(pullEnergySpec));
  if (ensp) {
    ensp->energy = pullEnergyUnknown;
    for (pi=0; pi<PULL_ENERGY_PARM_NUM; pi++) {
      ensp->parm[pi] = AIR_NAN;
    }
  }
  return ensp;
}

int
pullEnergySpecSet(pullEnergySpec *ensp, const pullEnergy *energy,
                  const double parm[PULL_ENERGY_PARM_NUM]) {
  char me[]="pullEnergySpecSet", err[BIFF_STRLEN];
  unsigned int pi;

  if (!( ensp && energy )) {
    sprintf(err, "%s: got NULL pointer", me); 
    biffAdd(PULL, err); return 1;
  }
  if (ensp && energy && parm) {
    ensp->energy = energy;
    for (pi=0; pi<PULL_ENERGY_PARM_NUM; pi++) {
      ensp->parm[pi] = parm[pi];
    }
  }
  return 0;
}

pullEnergySpec *
pullEnergySpecNix(pullEnergySpec *ensp) {

  airFree(ensp);
  return NULL;
}

int
pullEnergySpecParse(pullEnergySpec *ensp, const char *_str) {
  char me[]="pullEnergySpecParse", err[BIFF_STRLEN];
  char *str, *col, *_pstr, *pstr;
  int etype;
  unsigned int pi, haveParm;
  airArray *mop;
  double pval;

  if (!( ensp && _str )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PULL, err); return 1;
  }

  /* see if its the name of something that needs no parameters */
  etype = airEnumVal(pullEnergyType, _str);
  if (pullEnergyTypeUnknown != etype) {
    /* the string is the name of some energy */
    ensp->energy = pullEnergyAll[etype];
    if (0 != ensp->energy->parmNum) {
      sprintf(err, "%s: need %u parms for %s energy, but got none", me,
              ensp->energy->parmNum, ensp->energy->name);
      biffAdd(PULL, err); return 1;
    }
    /* the energy needs 0 parameters */
    for (pi=0; pi<PULL_ENERGY_PARM_NUM; pi++) {
      ensp->parm[pi] = AIR_NAN;
    }
    return 0;
  }

  /* start parsing parms after ':' */
  mop = airMopNew();
  str = airStrdup(_str);
  airMopAdd(mop, str, (airMopper)airFree, airMopAlways);
  col = strchr(str, ':');
  if (!col) {
    sprintf(err, "%s: \"%s\" isn't a parameter-free energy, but it has no "
            "\":\" separator to indicate parameters", me, str);
    biffAdd(PULL, err); airMopError(mop); return 1;
  }
  *col = '\0';
  etype = airEnumVal(pullEnergyType, str);
  if (pullEnergyTypeUnknown == etype) {
    sprintf(err, "%s: didn't recognize \"%s\" as a %s", me,
            str, pullEnergyType->name);
    biffAdd(PULL, err); airMopError(mop); return 1;
  }

  ensp->energy = pullEnergyAll[etype];
  if (0 == ensp->energy->parmNum) {
    sprintf(err, "%s: \"%s\" energy has no parms, but got something", me,
            ensp->energy->name);
    biffAdd(PULL, err); return 1;
  }

  _pstr = pstr = col+1;
  /* code lifted from teem/src/nrrd/kernel.c, should probably refactor... */
  for (haveParm=0; haveParm<ensp->energy->parmNum; haveParm++) {
    if (!pstr) {
      break;
    }
    if (1 != sscanf(pstr, "%lg", &pval)) {
      sprintf(err, "%s: trouble parsing \"%s\" as double (in \"%s\")",
              me, _pstr, _str);
      biffAdd(PULL, err); airMopError(mop); return 1;
    }
    ensp->parm[haveParm] = pval;
    if ((pstr = strchr(pstr, ','))) {
      pstr++;
      if (!*pstr) {
        sprintf(err, "%s: nothing after last comma in \"%s\" (in \"%s\")",
                me, _pstr, _str);
        biffAdd(PULL, err); airMopError(mop); return 1;
      }
    }
  }
  /* haveParm is now the number of parameters that were parsed. */
  if (haveParm < ensp->energy->parmNum) {
    sprintf(err, "%s: parsed only %u of %u required parms (for %s energy)"
            "from \"%s\" (in \"%s\")",
            me, haveParm, ensp->energy->parmNum,
            ensp->energy->name, _pstr, _str);
    biffAdd(PULL, err); airMopError(mop); return 1;
  } else {
    if (pstr) {
      sprintf(err, "%s: \"%s\" (in \"%s\") has more than %u doubles",
              me, _pstr, _str, ensp->energy->parmNum);
      biffAdd(PULL, err); airMopError(mop); return 1;
    }
  }
  
  airMopOkay(mop);
  return 0;
}

int
_pullHestEnergyParse(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  pullEnergySpec **enspP;
  char me[]="_pullHestForceParse", *perr;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  enspP = (pullEnergySpec **)ptr;
  *enspP = pullEnergySpecNew();
  if (pullEnergySpecParse(*enspP, str)) {
    perr = biffGetDone(PULL);
    strncpy(err, perr, AIR_STRLEN_HUGE-1);
    free(perr);
    return 1;
  }
  return 0;
}

hestCB
_pullHestEnergySpec = {
  sizeof(pullEnergySpec*),
  "energy specification",
  _pullHestEnergyParse,
  (airMopper)pullEnergySpecNix
};

hestCB *
pullHestEnergySpec = &_pullHestEnergySpec;
