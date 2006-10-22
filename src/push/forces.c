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

#include "push.h"
#include "privatePush.h"

#define SPRING  "spring"
#define GAUSS   "gauss"
#define COULOMB "coulomb"
#define COTAN   "cotan"
#define ZERO    "zero"

char
_pushEnergyTypeStr[PUSH_ENERGY_TYPE_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_energy)",
  SPRING,
  GAUSS,
  COULOMB,
  COTAN,
  ZERO
};

char
_pushEnergyTypeDesc[PUSH_ENERGY_TYPE_MAX+1][AIR_STRLEN_MED] = {
  "unknown_energy",
  "Hooke's law-based potential, with a tunable region of attraction",
  "Gaussian potential",
  "Coulomb electrostatic potential, with tunable cut-off",
  "Cotangent-based potential (from Meyer et al. SMI '05)",
  "no energy"
};

airEnum
_pushEnergyType = {
  "energy",
  PUSH_ENERGY_TYPE_MAX,
  _pushEnergyTypeStr,  NULL,
  _pushEnergyTypeDesc,
  NULL, NULL,
  AIR_FALSE
};
airEnum *
pushEnergyType = &_pushEnergyType;

/* ----------------------------------------------------------------
** ------------------------------ UNKNOWN -------------------------
** ----------------------------------------------------------------
*/
double
_pushEnergyUnknownEnergy(double dist, const double *parm) {
  char me[]="_pushEnergyUnknownEnergy";

  AIR_UNUSED(dist);
  AIR_UNUSED(parm);
  fprintf(stderr, "%s: this is not good.\n", me);
  return AIR_NAN;
}

double
_pushEnergyUnknownForce(double dist, const double *parm) {
  char me[]="_pushEnergyUnknownForce";

  AIR_UNUSED(dist);
  AIR_UNUSED(parm);
  fprintf(stderr, "%s: this is not good.\n", me);
  return AIR_NAN;
}

double
_pushEnergyUnknownSupport(const double *parm) {
  char me[]="_pushEnergyUnknownSupport";

  AIR_UNUSED(parm);
  fprintf(stderr, "%s: this is not good.\n", me);
  return AIR_NAN;
}

pushEnergy
_pushEnergyUnknown = {
  "unknown",
  0,
  _pushEnergyUnknownEnergy,
  _pushEnergyUnknownForce,
  _pushEnergyUnknownSupport
};
const pushEnergy *const
pushEnergyUnknown = &_pushEnergyUnknown;

/* ----------------------------------------------------------------
** ------------------------------ SPRING --------------------------
** ----------------------------------------------------------------
** 1 parms:
** 0: pull distance
*/
double
_pushEnergySpringEnergy(double dist, const double *parm) {
  /* char me[]="_pushEnergySpringEnergy"; */
  double xx, ret, pull;

  pull = parm[0];
  xx = dist - 1.0;
  if (xx > pull) {
    ret = 0;
  } else if (xx > 0) {
    ret = xx*xx*(xx*xx/(4*pull*pull) - 2*xx/(3*pull) + 1/2);
  } else {
    ret = xx*xx/2;
  }
  /*
  if (!AIR_EXISTS(ret)) {
    fprintf(stderr, "!%s: dist=%g, pull=%g, blah=%d --> ret=%g\n",
            me, dist, pull, blah, ret);
  }
  */
  return ret;
}

double
_pushEnergySpringForce(double dist, const double *parm) {
  /* char me[]="_pushEnergySpringForce"; */
  double xx, ret, pull;

  pull = parm[0];
  xx = dist - 1.0;
  if (xx > pull) {
    ret = 0;
  } else if (xx > 0) {
    ret = xx*(xx*xx/(pull*pull) - 2*xx/pull + 1);
  } else {
    ret = xx;
  }
  /*
  if (!AIR_EXISTS(ret)) {
    fprintf(stderr, "!%s: dist=%g, pull=%g, blah=%d --> ret=%g\n",
            me, dist, pull, blah, ret);
  }
  */
  return ret;
}

double
_pushEnergySpringSupport(const double *parm) {

  return 1.0 + parm[0];
}

const pushEnergy
_pushEnergySpring = {
  SPRING,
  1,
  _pushEnergySpringEnergy,
  _pushEnergySpringForce,
  _pushEnergySpringSupport
};
const pushEnergy *const
pushEnergySpring = &_pushEnergySpring;

/* ----------------------------------------------------------------
** ------------------------------ GAUSS --------------------------
** ----------------------------------------------------------------
** 1 parms:
** (scale: distance to inflection point of force function)
** parm[0]: cut-off (as a multiple of standard dev)
*/
#define _DGAUSS(x, sig, cut) (                                               \
   x >= sig*cut ? 0                                                          \
   : -exp(-x*x/(2.0*sig*sig))*x)
#define SQRTTHREE 1.73205080756887729352

double
_pushEnergyGaussEnergy(double dist, const double *parm) {
  double sig, cut;

  sig = 1.0/SQRTTHREE;
  cut = parm[0];
  return AIR_CAST(double, _DGAUSS(dist, sig, cut));
}

double
_pushEnergyGaussForce(double dist, const double *parm) {
  double sig, cut;

  sig = 1.0/SQRTTHREE;
  cut = parm[0];
  return AIR_CAST(double, _DGAUSS(dist, sig, cut));
}

double
_pushEnergyGaussSupport(const double *parm) {

  return (1.0/SQRTTHREE)*parm[0];
}

const pushEnergy
_pushEnergyGauss = {
  GAUSS,
  1,
  _pushEnergyGaussEnergy,
  _pushEnergyGaussForce,
  _pushEnergyGaussSupport
};
const pushEnergy *const
pushEnergyGauss = &_pushEnergyGauss;

/* ----------------------------------------------------------------
** ------------------------------ CHARGE --------------------------
** ----------------------------------------------------------------
** 1 parms:
** (scale: distance to "1.0" in graph of x^(-2))
** parm[0]: cut-off (as multiple of "1.0")
*/
double
_pushEnergyCoulombEnergy(double dist, const double *parm) {

  return (dist > parm[0] ? 0 : 1.0/dist));
}

double
_pushEnergyCoulombForce(double dist, const double *parm) {

  return (dist > parm[0] ? 0 : -1.0/(dist*dist));
}

double
_pushEnergyCoulombSupport(const double *parm) {

  return parm[0];
}

const pushEnergy
_pushEnergyCoulomb = {
  COULOMB,
  1,
  _pushEnergyCoulombEnergy,
  _pushEnergyCoulombForce,
  _pushEnergyCoulombSupport
};
const pushEnergy *const
pushEnergyCoulomb = &_pushEnergyCoulomb;

/* ----------------------------------------------------------------
** ------------------------------ COTAN ---------------------------
** ----------------------------------------------------------------
** 0 parms!
*/
double
_pushEnergyCotanEnergy(double dist, const double *parm) {
  double ss;

  AIR_UNUSED(parm);
  ss = sin(dist*AIR_PI/2.0);
  return (AIR_PI/2.0)*(dist > 1 ? 0 : 1.0 - 1.0/(ss*ss));
}

double
_pushEnergyCotanForce(double dist, const double *parm) {
  double ss;

  AIR_UNUSED(parm);
  ss = sin(dist*AIR_PI/2.0);
  return (AIR_PI/2.0)*(dist > 1 ? 0 : 1.0 - 1.0/(ss*ss));
}

double
_pushEnergyCotanSupport(const double *parm) {

  AIR_UNUSED(parm);
  return 1;
}

const pushEnergy
_pushEnergyCotan = {
  COTAN,
  0,
  _pushEnergyCotanEnergy,
  _pushEnergyCotanForce,
  _pushEnergyCotanSupport
};
const pushEnergy *const
pushEnergyCotan = &_pushEnergyCotan;

/* ----------------------------------------------------------------
** ------------------------------- ZERO ---------------------------
** ----------------------------------------------------------------
** 0 parms:
*/
double
_pushEnergyZeroFunc(double dist, const double *parm) {

  AIR_UNUSED(dist);
  AIR_UNUSED(parm);
  return 0.0;
}

double
_pushEnergyZeroSupport(const double *parm) {

  AIR_UNUSED(parm);
  return 1.0;
}

const pushEnergy
_pushEnergyZero = {
  ZERO,
  0,
  _pushEnergyZeroFunc,
  _pushEnergyZeroFunc,
  _pushEnergyZeroSupport
};
const pushEnergy *const
pushEnergyZero = &_pushEnergyZero;

/* ----------------------------------------------------------------
** ----------------------------------------------------------------
** ----------------------------------------------------------------
*/

const pushEnergy *const pushEnergyAll[PUSH_ENERGY_TYPE_MAX+1] = {
  &_pushEnergyUnknown,  /* 0 */
  &_pushEnergySpring,   /* 1 */
  &_pushEnergyGauss,    /* 2 */
  &_pushEnergyCoulomb,  /* 3 */
  &_pushEnergyCotan,    /* 4 */
  &_pushEnergyZero      /* 5 */
};

pushEnergySpec *
pushEnergySpecNew() {
  pushEnergySpec *ensp;
  int pi;

  ensp = (pushEnergySpec *)calloc(1, sizeof(pushEnergySpec));
  if (ensp) {
    ensp->energy = pushEnergyUnknown;
    for (pi=0; pi<PUSH_ENERGY_PARM_NUM; pi++) {
      ensp->parm[pi] = AIR_NAN;
    }
  }
  return ensp;
}

pushEnergySpec *
pushEnergySpecNix(pushEnergySpec *ensp) {

  airFree(ensp);
  return NULL;
}

int
pushEnergySpecParse(pushEnergySpec *ensp, const char *_str) {
  char me[]="pushEnergySpecParse", err[BIFF_STRLEN];
  char *str, *col, *_pstr, *pstr;
  int etype;
  unsigned int pi, haveParm;
  airArray *mop;
  double pval;

  if (!( ensp && _str )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PUSH, err); return 1;
  }

  /* see if its the name of something that needs no parameters */
  etype = airEnumVal(pushEnergyType, _str);
  if (pushEnergyTypeUnknown != etype) {
    /* the string is the name of some energy */
    ensp->energy = pushEnergyAll[etype];
    if (0 != ensp->energy->parmNum) {
      sprintf(err, "%s: need %u parms for %s energy, but got none", me,
              ensp->energy->parmNum, ensp->energy->name);
      biffAdd(PUSH, err); return 1;
    }
    /* the energy needs 0 parameters */
    for (pi=0; pi<PUSH_ENERGY_PARM_NUM; pi++) {
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
    biffAdd(PUSH, err); airMopError(mop); return 1;
  }
  *col = '\0';
  etype = airEnumVal(pushEnergyType, str);
  if (pushEnergyTypeUnknown == etype) {
    sprintf(err, "%s: didn't recognize \"%s\" as a %s", me,
            str, pushEnergyType->name);
    biffAdd(PUSH, err); airMopError(mop); return 1;
  }

  ensp->energy = pushEnergyAll[etype];
  if (0 == ensp->energy->parmNum) {
    sprintf(err, "%s: \"%s\" energy has no parms, but got something", me,
            ensp->energy->name);
    biffAdd(PUSH, err); return 1;
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
      biffAdd(PUSH, err); airMopError(mop); return 1;
    }
    ensp->parm[haveParm] = pval;
    if ((pstr = strchr(pstr, ','))) {
      pstr++;
      if (!*pstr) {
        sprintf(err, "%s: nothing after last comma in \"%s\" (in \"%s\")",
                me, _pstr, _str);
        biffAdd(PUSH, err); airMopError(mop); return 1;
      }
    }
  }
  /* haveParm is now the number of parameters that were parsed. */
  if (haveParm < ensp->energy->parmNum) {
    sprintf(err, "%s: parsed only %u of %u required parms (for %s energy)"
            "from \"%s\" (in \"%s\")",
            me, haveParm, ensp->energy->parmNum,
            ensp->energy->name, _pstr, _str);
    biffAdd(PUSH, err); airMopError(mop); return 1;
  } else {
    if (pstr) {
      sprintf(err, "%s: \"%s\" (in \"%s\") has more than %u doubles",
              me, _pstr, _str, ensp->energy->parmNum);
      biffAdd(PUSH, err); airMopError(mop); return 1;
    }
  }
  
  airMopOkay(mop);
  return 0;
}

int
_pushHestEnergyParse(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  pushEnergySpec **enspP;
  char me[]="_pushHestForceParse", *perr;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  enspP = (pushEnergySpec **)ptr;
  *enspP = pushEnergySpecNew();
  if (pushEnergySpecParse(*enspP, str)) {
    perr = biffGetDone(PUSH);
    strncpy(err, perr, AIR_STRLEN_HUGE-1);
    free(perr);
    return 1;
  }
  return 0;
}

hestCB
_pushHestEnergySpec = {
  sizeof(pushEnergySpec*),
  "energy specification",
  _pushHestEnergyParse,
  (airMopper)pushEnergySpecNix
};

hestCB *
pushHestEnergySpec = &_pushHestEnergySpec;
