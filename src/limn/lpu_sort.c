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

#include "limn.h"
#include "privateLimn.h"

#define INFO "Sorts primitives according to given values"
static const char *myinfo = (INFO ". ");

static int
limnPu_sortMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *hopt = NULL;
  char *err, *perr;
  airArray *mop;
  int pret;

  limnPolyData *pld;
  Nrrd *nin;
  char *out;

  hestOptAdd_1_Other(&hopt, NULL, "input lpld", &pld, NULL, "input polydata filename",
                     limnHestPolyDataLMPD);
  hestOptAdd_1_Other(&hopt, NULL, "input nrrd", &nin, NULL, "input nrrd filename",
                     nrrdHestNrrd);
  hestOptAdd_1_String(&hopt, NULL, "output", &out, NULL, "output lpld filename");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);

  USAGE(myinfo);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (limnPolyDataPrimitiveSort(pld, nin)) {
    airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:%s", me, err);
    airMopError(mop);
    return 1;
  }

  if (limnPolyDataSave(out, pld)) {
    airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:%s", me, err);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}

const unrrduCmd limnPu_sortCmd = {"sort", INFO, limnPu_sortMain, AIR_FALSE};
