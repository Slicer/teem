/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "moss.h"
#include "privateMoss.h"

static int
_mossHestTransformParse(void *ptr, const char *_str, char err[AIR_STRLEN_HUGE + 1]) {
  static const char me[] = "_mossHestTransformParse";
  char *str;
  double **matP, tx, ty, sx, sy, angle, mat[6], shf, sha;
  airArray *mop;

  if (!(ptr && _str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  matP = (double **)ptr;
  mop = airMopNew();
  *matP = (double *)calloc(6, sizeof(double));
  airMopMem(mop, matP, airMopOnError);
  str = airToLower(airStrdup(_str));
  airMopMem(mop, &str, airMopAlways);

  if (!strcmp("identity", str)) {
    mossMatIdentitySet(*matP);

  } else if (1 == sscanf(str, "flip:%lf", &angle)) {
    mossMatFlipSet(*matP, angle);

  } else if (2 == sscanf(str, "translate:%lf,%lf", &tx, &ty)) {
    mossMatTranslateSet(*matP, tx, ty);
  } else if (2 == sscanf(str, "t:%lf,%lf", &tx, &ty)) {
    mossMatTranslateSet(*matP, tx, ty);

  } else if (1 == sscanf(str, "rotate:%lf", &angle)) {
    mossMatRotateSet(*matP, angle);
  } else if (1 == sscanf(str, "r:%lf", &angle)) {
    mossMatRotateSet(*matP, angle);

  } else if (2 == sscanf(str, "scale:%lf,%lf", &sx, &sy)) {
    mossMatScaleSet(*matP, sx, sy);
  } else if (2 == sscanf(str, "s:%lf,%lf", &sx, &sy)) {
    mossMatScaleSet(*matP, sx, sy);

  } else if (2 == sscanf(str, "shear:%lf,%lf", &shf, &sha)) {
    mossMatShearSet(*matP, shf, sha);

  } else if (6
             == sscanf(str, "%lf,%lf,%lf,%lf,%lf,%lf", mat + 0, mat + 1, mat + 2,
                       mat + 3, mat + 4, mat + 5)) {
    MOSS_MAT_COPY(*matP, mat);

  } else {
    sprintf(err, "%s: couldn't parse \"%s\" as a transform", me, _str);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}

static const hestCB _mossHestTransform = {sizeof(double *), "2D transform",
                                          _mossHestTransformParse, airFree};

const hestCB *const mossHestTransform = &_mossHestTransform;

/* ----------------------------------------------------------------- */

/*
** _mossHestOriginParse()
**
** parse an origin specification
** p(x,y): absolute pixel position --> val[3] = (0,x,y)
** u(x,y): position in unit box [0,1]x[0,1] --> val[3] = (1,x,y)
*/
static int
_mossHestOriginParse(void *ptr, const char *str, char err[AIR_STRLEN_HUGE + 1]) {
  static const char me[] = "_mossHestOriginParse";
  double **valP;
  airArray *mop;

  valP = (double **)ptr;
  mop = airMopNew();
  *valP = (double *)calloc(3, sizeof(double));
  airMopMem(mop, valP, airMopOnError);
  if (2 == sscanf(str, "p:%lf,%lf", *valP + 1, *valP + 2)) {
    (*valP)[0] = 0;
  } else if (2 == sscanf(str, "u:%lf,%lf", *valP + 1, *valP + 2)) {
    (*valP)[0] = 1;
  } else {
    sprintf(err, "%s: couldn't parse \"%s\" as origin", me, str);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}

static const hestCB _mossHestOrigin = {sizeof(double *), "origin specification",
                                       _mossHestOriginParse, airFree};

const hestCB *const mossHestOrigin = &_mossHestOrigin;
