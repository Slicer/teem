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

#define INFO "Non-linear least-squares fitting of b-value curves"
static const char *_tend_bfitInfoL
  = (INFO ". Axis 0 is replaced by three values: amp, dec, err, based on a "
          "non-linear least-squares fit of amp*exp(-b*dec) to the range of DWI "
          "values along input axis 0, as a function of changing b values.  ");

static int
tend_bfitMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  Nrrd *nin, *nout;
  double *bb, *ww, *_ww, eps;
  unsigned int ii, bbLen, _wwLen;
  int iterMax;
  char *outS;

  hparm->respFileEnable = AIR_TRUE;

  hestOptAdd_1_Other(&hopt, "i", "nin", &nin, "-",
                     "Input nrrd.  List of DWIs from different b-values must "
                     "be along axis 0",
                     nrrdHestNrrd);
  hestOptAdd_Nv_Double(&hopt, "b", "b1 b2", 2, -1, &bb, NULL,
                       "b values across axis 0 of input nrrd", &bbLen);
  hestOptAdd_Nv_Double(&hopt, "w", "w1 w2", 2, -1, &_ww, "nan nan",
                       "weights for samples in non-linear fitting", &_wwLen);
  hestOptAdd_1_Int(&hopt, "imax", "# iter", &iterMax, "10",
                   "max number of iterations to use in non-linear fitting, or, "
                   "use 0 to do only initial linear fit");
  hestOptAdd_1_Double(&hopt, "eps", "epsilon", &eps, "1",
                      "epsilon convergence threshold for non-linear fitting");
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-", "output tensor volume");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_tend_bfitInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (!(bbLen == nin->axis[0].size)) {
    char stmp[AIR_STRLEN_SMALL + 1];
    fprintf(stderr, "%s: got %d b-values but axis 0 size is %s\n", me, bbLen,
            airSprintSize_t(stmp, nin->axis[0].size));
    airMopError(mop);
    return 1;
  }
  if (AIR_EXISTS(_ww[0])) {
    if (!(_wwLen == nin->axis[0].size)) {
      char stmp[AIR_STRLEN_SMALL + 1];
      fprintf(stderr, "%s: got %d weights but axis 0 size is %s\n", me, _wwLen,
              airSprintSize_t(stmp, nin->axis[0].size));
      airMopError(mop);
      return 1;
    }
    ww = _ww;
  } else {
    /* no explicit weights specified */
    ww = (double *)calloc(nin->axis[0].size, sizeof(double));
    airMopAdd(mop, ww, airFree, airMopAlways);
    for (ii = 0; ii < nin->axis[0].size; ii++) {
      ww[ii] = 1.0;
    }
  }

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (tenBVecNonLinearFit(nout, nin, bb, ww, iterMax, eps)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
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
TEND_CMD(bfit, INFO);
