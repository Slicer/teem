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

#include "ten.h"
#include "privateTen.h"

/*
******** tendCmdList[]
**
** NULL-terminated array of unrrduCmd pointers, as ordered by
** TEN_MAP macro
*/
const unrrduCmd *const tendCmdList[] = {TEND_MAP(TEND_LIST) NULL};

const char *const tendTitle = "tend: Diffusion Image Processing and Analysis";

/*
******** tendFiberStopParse
**
** for parsing the different ways in which a fiber should be stopped
** For the sake of laziness and uniformity, the stop information is
** stored in an array of 3 (three) doubles:
** info[0]: int value from tenFiberStop* enum
** info[1]: 1st parameter associated with stop method (always used)
** info[2]: 2nd parameter, used occasionally
*/
static int
fiberStopParse(void *ptr, const char *_str, char err[AIR_STRLEN_HUGE + 1]) {
  static const char me[] = "fiberStopParse";
  char *str, *opt, *opt2;
  double *info;
  airArray *mop;
  int integer;

  if (!(ptr && _str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  info = (double *)ptr;

  mop = airMopNew();
  str = airStrdup(_str);
  airMopMem(mop, &str, airMopAlways);
  opt = strchr(str, ':');
  if (!opt) {
    /* couldn't parse string as nrrdEncoding, but there wasn't a colon */
    sprintf(err, "%s: didn't see a colon in \"%s\"", me, str);
    airMopError(mop);
    return 1;
  }
  *opt = '\0';
  opt++;
  info[0] = AIR_INT(airEnumVal(tenFiberStop, str));
  if (tenFiberStopUnknown == AIR_INT(info[0])) {
    sprintf(err, "%s: didn't recognize \"%s\" as %s", me, str, tenFiberStop->name);
    airMopError(mop);
    return 1;
  }
  switch (AIR_INT(info[0])) {
  case tenFiberStopAniso:
    /* <aniso>,<level> : tenAniso,double */
    opt2 = strchr(opt, ',');
    if (!opt2) {
      sprintf(err, "%s: didn't see comma between aniso and level in \"%s\"", me, opt);
      airMopError(mop);
      return 1;
    }
    *opt2 = '\0';
    opt2++;
    info[1] = AIR_INT(airEnumVal(tenAniso, opt));
    if (tenAnisoUnknown == AIR_INT(info[1])) {
      sprintf(err, "%s: didn't recognize \"%s\" as %s", me, opt, tenAniso->name);
      airMopError(mop);
      return 1;
    }
    if (1 != sscanf(opt2, "%lg", info + 2)) {
      sprintf(err, "%s: couldn't parse aniso level \"%s\" as double", me, opt2);
      airMopError(mop);
      return 1;
    }
    /*
    fprintf(stderr, "!%s: parsed aniso:%s,%g\n", me,
            airEnumStr(tenAniso, AIR_INT(info[1])), info[2]);
    */
    break;
  case tenFiberStopFraction:
  case tenFiberStopLength:
  case tenFiberStopRadius:
  case tenFiberStopConfidence:
  case tenFiberStopMinLength:
    /* all of these take a single double */
    if (1 != sscanf(opt, "%lg", info + 1)) {
      sprintf(err, "%s: couldn't parse %s \"%s\" as double", me,
              airEnumStr(tenFiberStop, AIR_INT(info[0])), opt);
      airMopError(mop);
      return 1;
    }
    /*
    fprintf(stderr, "!%s: parse %s:%g\n", me,
            airEnumStr(tenFiberStop, AIR_INT(info[0])),
            info[1]);
    */
    break;
  case tenFiberStopNumSteps:
  case tenFiberStopMinNumSteps:
    /* <#steps> : int */
    if (1 != sscanf(opt, "%d", &integer)) {
      sprintf(err, "%s: couldn't parse \"%s\" as int", me, opt);
      airMopError(mop);
      return 1;
    }
    info[1] = integer;
    /* fprintf(stderr, "!%s: parse steps:%d\n", me, integer); */
    break;
  case tenFiberStopBounds:
    /* moron */
    break;
  default:
    sprintf(err, "%s: stop method %d not supported", me, AIR_INT(info[0]));
    airMopError(mop);
    return 1;
    break;
  }
  airMopOkay(mop);
  return 0;
}

static const hestCB _tendFiberStopCB = {3 * sizeof(double), "fiber stop", fiberStopParse,
                                        NULL};

const hestCB *const tendFiberStopCB = &_tendFiberStopCB;
