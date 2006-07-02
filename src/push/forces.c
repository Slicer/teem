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

enum {
  pushForceUnknown,
  pushForceSpring,        /* 1 */
  pushForceGauss,         /* 2 */
  pushForceCharge,        /* 3 */
  pushForceCotan,         /* 4 */
  pushForceNone,          /* 5 */
  pushForceLast
};
#define PUSH_FORCE_MAX       5

char
_pushForceStr[PUSH_FORCE_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_force)",
  "spring",
  "gauss",
  "charge",
  "cotan",
  "none"
};

char
_pushForceDesc[PUSH_FORCE_MAX+1][AIR_STRLEN_MED] = {
  "unknown_force",
  "Hooke's law, with a tunable region of attraction",
  "derivative of a Gaussian energy function",
  "inverse square law, with tunable cut-off",
  "Cotangent based energy function (from Meyer et al. SMI 05)",
  "no force"
};

airEnum
_pushForceEnum = {
  "force",
  PUSH_FORCE_MAX,
  _pushForceStr,  NULL,
  _pushForceDesc,
  NULL, NULL,
  AIR_FALSE
};
airEnum *
pushForceEnum = &_pushForceEnum;

/* ----------------------------------------------------------------
** ------------------------------ (stubs) -------------------------
** ----------------------------------------------------------------
*/
double
_pushForceUnknownFunc(double dist, const double *parm) {
  char me[]="_pushForceUnknownFunc";

  AIR_UNUSED(dist);
  AIR_UNUSED(parm);
  fprintf(stderr, "%s: this is not good.\n", me);
  return AIR_NAN;
}

double
_pushForceUnknownExtent(const double *parm) {
  char me[]="_pushForceUnknownExtent";

  AIR_UNUSED(parm);
  fprintf(stderr, "%s: this is not good.\n", me);
  return AIR_NAN;
}

/* ----------------------------------------------------------------
** ------------------------------ SPRING --------------------------
** ----------------------------------------------------------------
** 1 parms:
** 0: pull distance
*/
double
_pushForceSpringFunc(double dist, const double *parm) {
  /* char me[]="_pushForceSpringFunc"; */
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
_pushForceSpringExtent(const double *parm) {

  return 1.0 + parm[0];
}

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
_pushForceGaussFunc(double dist, const double *parm) {
  double sig, cut;

  sig = 1.0/SQRTTHREE;
  cut = parm[0];
  return AIR_CAST(double, _DGAUSS(dist, sig, cut));
}

double
_pushForceGaussExtent(const double *parm) {

  return (1.0/SQRTTHREE)*parm[0];
}

/* ----------------------------------------------------------------
** ------------------------------ CHARGE --------------------------
** ----------------------------------------------------------------
** 1 parms:
** (scale: distance to "1.0" in graph of x^(-2))
** parm[0]: cut-off (as multiple of "1.0")
*/
double
_pushForceChargeFunc(double dist, const double *parm) {

  return (dist > parm[0] ? 0 : -1.0/(dist*dist));
}

double
_pushForceChargeExtent(const double *parm) {

  return parm[0];
}

/* ----------------------------------------------------------------
** ------------------------------ COTAN ---------------------------
** ----------------------------------------------------------------
** 0 parms!
*/
double
_pushForceCotanFunc(double dist, const double *parm) {
  double ss;

  AIR_UNUSED(parm);
  ss = sin(dist*AIR_PI/2.0);
  return (AIR_PI/2.0)*(dist > 1 ? 0 : 1.0 - 1.0/(ss*ss));
}

double
_pushForceCotanExtent(const double *parm) {

  AIR_UNUSED(parm);
  return 1;
}

/* ----------------------------------------------------------------
** -----------------------=------- NONE ---------------------------
** ----------------------------------------------------------------
** 0 parms:
*/
double
_pushForceNoneFunc(double dist, const double *parm) {

  AIR_UNUSED(dist);
  AIR_UNUSED(parm);
  return 0.0;
}

double
_pushForceNoneExtent(const double *parm) {

  AIR_UNUSED(parm);
  return 1.0;
}

/* ----------------------------------------------------------------
** ------------------------------ arrays ... ----------------------
** ----------------------------------------------------------------
*/

int
_pushForceParmNum[PUSH_FORCE_MAX+1] = {

  1, /* pushForceSpring */
  1, /* pushForceGauss */
  1, /* pushForceCharge */
  0, /* pushForceCotan */
  0  /* pushForceNone */
};

double
(*_pushForceFunc[PUSH_FORCE_MAX+1])(double dist, const double *parm) = {
  _pushForceUnknownFunc,
  _pushForceSpringFunc,
  _pushForceGaussFunc,
  _pushForceChargeFunc,
  _pushForceCotanFunc,
  _pushForceNoneFunc,
};

double
(*_pushForceExtent[PUSH_FORCE_MAX+1])(const double *parm) = {
  _pushForceUnknownExtent,
  _pushForceSpringExtent,
  _pushForceGaussExtent,
  _pushForceChargeExtent,
  _pushForceCotanExtent,
  _pushForceNoneExtent,
};

pushForce *
_pushForceNew() {
  pushForce *force;
  int pi;

  force = (pushForce *)calloc(1, sizeof(pushForce));
  if (force) {
    force->noop = AIR_FALSE;  /* by default, not a no-op */
    force->func = NULL;
    force->extent = NULL;
    for (pi=0; pi<PUSH_FORCE_PARM_MAXNUM; pi++) {
      force->parm[pi] = AIR_NAN;
    }
  }
  return force;
}

pushForce *
pushForceNix(pushForce *force) {

  airFree(force);
  return NULL;
}

pushForce *
pushForceParse(const char *_str) {
  char me[]="pushForceParse", err[BIFF_STRLEN];
  char *str, *col, *_pstr, *pstr;
  pushForce *force;
  int fri, needParm, haveParm;
  airArray *mop;
  double pval;

  if (!_str) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PUSH, err); return NULL;
  }

  mop = airMopNew();
  str = airStrdup(_str);
  airMopAdd(mop, str, (airMopper)airFree, airMopAlways);
  force = _pushForceNew();
  airMopAdd(mop, force, (airMopper)pushForceNix, airMopOnError);

  if (!strcmp(airEnumStr(pushForceEnum, pushForceNone), str)) {
    /* special case: no parameters */
    force->noop = AIR_TRUE;
    strcpy(force->name, _pushForceStr[pushForceNone]);
    force->func = _pushForceFunc[pushForceNone];
    force->extent = _pushForceExtent[pushForceNone];
    airMopOkay(mop);
    return force;
  }

  if (!strcmp(airEnumStr(pushForceEnum, pushForceCotan), str)) {
    /* special case: no parameters */
    strcpy(force->name, _pushForceStr[pushForceCotan]);
    force->func = _pushForceFunc[pushForceCotan];
    force->extent = _pushForceExtent[pushForceCotan];
    airMopOkay(mop);
    return force;
  }

  col = strchr(str, ':');
  if (!col) {
    sprintf(err, "%s: didn't see colon separator in \"%s\"", me, str);
    biffAdd(PUSH, err); airMopError(mop); return NULL;
  }
  *col = '\0';
  fri = airEnumVal(pushForceEnum, str);
  if (pushForceUnknown == fri) {
    sprintf(err, "%s: didn't recognize \"%s\" as a force", me, str);
  }
  needParm = _pushForceParmNum[fri];
  _pstr = pstr = col+1;
  /* code lifted from teem/src/nrrd/kernel.c, should probably refactor... */
  for (haveParm=0; haveParm<needParm; haveParm++) {
    if (!pstr) {
      break;
    }
    if (1 != sscanf(pstr, "%lg", &pval)) {
      sprintf(err, "%s: trouble parsing \"%s\" as double (in \"%s\")",
              me, _pstr, _str);
      biffAdd(PUSH, err); airMopError(mop); return NULL;
    }
    force->parm[haveParm] = AIR_CAST(double, pval);
    if ((pstr = strchr(pstr, ','))) {
      pstr++;
      if (!*pstr) {
        sprintf(err, "%s: nothing after last comma in \"%s\" (in \"%s\")",
                me, _pstr, _str);
        biffAdd(PUSH, err); airMopError(mop); return NULL;
      }
    }
  }
  /* haveParm is now the number of parameters that were parsed. */
  if (haveParm < needParm) {
    sprintf(err, "%s: parsed only %d of %d required parameters (for %s force)"
            "from \"%s\" (in \"%s\")",
            me, haveParm, needParm,
            airEnumStr(pushForceEnum, fri), _pstr, _str);
      biffAdd(PUSH, err); airMopError(mop); return NULL;
  } else {
    if (pstr) {
      sprintf(err, "%s: \"%s\" (in \"%s\") has more than %d doubles",
              me, _pstr, _str, needParm);
      biffAdd(PUSH, err); airMopError(mop); return NULL;
    }
  }
  
  /* parameters have been set, now set the rest of the force info */
  strcpy(force->name, _pushForceStr[fri]);
  force->func = _pushForceFunc[fri];
  force->extent = _pushForceExtent[fri];

  airMopOkay(mop);
  return force;
}

int
_pushHestForceParse(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  pushForce **fcP;
  char me[]="_pushHestForceParse", *perr;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  fcP = (pushForce **)ptr;
  *fcP = pushForceParse(str);
  if (!(*fcP)) {
    perr = biffGetDone(PUSH);
    strncpy(err, perr, AIR_STRLEN_HUGE-1);
    free(perr);
    return 1;
  }
  return 0;
}

hestCB
_pushHestForce = {
  sizeof(pushForce*),
  "force specification",
  _pushHestForceParse,
  (airMopper)pushForceNix
};

hestCB *
pushHestForce = &_pushHestForce;
