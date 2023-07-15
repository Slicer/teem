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

#define INFO "convert from one model to another"
static const char *_tend_mconvInfoL = (INFO ". More docs here.");

static int
tend_mconvMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  Nrrd *nin, *nout;
  char *outS, *modelSrcS, *modelDstS;
  const tenModel *modelDst, *modelSrc;
  int saveB0;

  hestOptAdd_1_String(&hopt, "mo", "model", &modelDstS, NULL,
                      "which model to convert to");
  hestOptAdd_1_String(&hopt, "mi", "model", &modelSrcS, "",
                      "model converting from; if not set, will try to determine "
                      "from input nrrd");
  hestOptAdd_1_Other(&hopt, "i", "nin", &nin, "-", "input nrrd of model parms",
                     nrrdHestNrrd);
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-", "output nrrd of model parms");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE_JUSTPARSE(_tend_mconvInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (tenModelParse(&modelDst, &saveB0, AIR_FALSE, modelDstS)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble parsing model \"%s\":\n%s\n", me, modelDstS, err);
    airMopError(mop);
    return 1;
  }
  if (saveB0) {
    printf("%s: warning: saving B0 is determined by input nrrd "
           "having B0 info.\n",
           me);
  }
  if (airStrlen(modelSrcS)) {
    if (tenModelParse(&modelSrc, &saveB0, AIR_FALSE, modelSrcS)) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble parsing model \"%s\":\n%s\n", me, modelSrcS, err);
      airMopError(mop);
      return 1;
    }
  } else {
    modelSrc = NULL;
  }
  if (tenModelConvert(nout, NULL, modelDst, nin, modelSrc)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble converting:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }
  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
TEND_CMD(mconv, INFO);
