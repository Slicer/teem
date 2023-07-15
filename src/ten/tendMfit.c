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

#define INFO "Estimate models from a set of DW images"
static const char *_tend_mfitInfoL = (INFO ". More docs here.");

static int
tend_mfitMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  Nrrd *nin, *nout, *nterr, *nconv, *niter;
  char *outS, *terrS, *convS, *iterS, *modS;
  int knownB0, saveB0, verbose, mlfit, typeOut;
  unsigned int maxIter, minIter, starts;
  double sigma, eps;
  const tenModel *model;
  tenExperSpec *espec;

  hestOptAdd_1_Int(&hopt, "v", "verbose", &verbose, "0", "verbosity level");
  hestOptAdd_1_String(&hopt, "m", "model", &modS, NULL,
                      "which model to fit. Use optional \"b0+\" prefix to "
                      "indicate that the B0 image should also be saved "
                      "(independent of whether it was known or had to be "
                      "estimated, according to \"-knownB0\").");
  hestOptAdd_1_UInt(&hopt, "ns", "# starts", &starts, "1",
                    "number of random starting points at which to initialize "
                    "fitting");
  hestOptAdd_Flag(&hopt, "ml", &mlfit,
                  "do ML fitting, rather than least-squares, which also "
                  "requires setting \"-sigma\"");
  hestOptAdd_1_Double(&hopt, "sigma", "sigma", &sigma, "nan",
                      "Gaussian/Rician noise parameter");
  hestOptAdd_1_Double(&hopt, "eps", "eps", &eps, "0.01", "convergence epsilon");
  hestOptAdd_1_UInt(&hopt, "mini", "min iters", &minIter, "3",
                    "minimum required # iterations for fitting.");
  hestOptAdd_1_UInt(&hopt, "maxi", "max iters", &maxIter, "100",
                    "maximum allowable # iterations for fitting.");
  hestOptAdd_1_Bool(&hopt, "knownB0", "bool", &knownB0, NULL,
                    "Indicates if the B=0 non-diffusion-weighted reference image "
                    "is known (\"true\") because it appears one or more times "
                    "amongst the DWIs, or, if it has to be estimated along with "
                    "the other model parameters (\"false\")");
  /* (this is now specified as part of the "-m" model description)
  hestOptAdd_1_Bool(&hopt, "saveB0", "bool", &saveB0, NULL,
                    "Indicates if the B=0 non-diffusion-weighted value "
                    "should be saved in output, regardless of whether it was "
                    "known or had to be esimated");
  */
  hestOptAdd_1_Enum(&hopt, "t", "type", &typeOut, "float",
                    "output type of model parameters", nrrdType);
  hestOptAdd_1_Other(&hopt, "i", "dwi", &nin, "-",
                     "all the diffusion-weighted images in one 4D nrrd", nrrdHestNrrd);
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-", "output parameter vector image");
  hestOptAdd_1_String(&hopt, "eo", "filename", &terrS, "",
                      "Giving a filename here allows you to save out the per-sample "
                      "fitting error.  By default, no such error is saved.");
  hestOptAdd_1_String(&hopt, "co", "filename", &convS, "",
                      "Giving a filename here allows you to save out the per-sample "
                      "convergence fraction.  By default, no such error is saved.");
  hestOptAdd_1_String(&hopt, "io", "filename", &iterS, "",
                      "Giving a filename here allows you to save out the per-sample "
                      "number of iterations needed for fitting.  "
                      "By default, no such error is saved.");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE_JUSTPARSE(_tend_mfitInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nterr = NULL;
  nconv = NULL;
  niter = NULL;
  espec = tenExperSpecNew();
  airMopAdd(mop, espec, (airMopper)tenExperSpecNix, airMopAlways);
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (tenModelParse(&model, &saveB0, AIR_FALSE, modS)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble parsing model \"%s\":\n%s\n", me, modS, err);
    airMopError(mop);
    return 1;
  }
  if (tenExperSpecFromKeyValueSet(espec, nin)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble getting exper from kvp:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }
  if (tenModelSqeFit(nout, airStrlen(terrS) ? &nterr : NULL,
                     airStrlen(convS) ? &nconv : NULL, airStrlen(iterS) ? &niter : NULL,
                     model, espec, nin, knownB0, saveB0, typeOut, minIter, maxIter,
                     starts, eps, NULL, verbose)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble fitting:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  if (nrrdSave(outS, nout, NULL) || (airStrlen(terrS) && nrrdSave(terrS, nterr, NULL))
      || (airStrlen(convS) && nrrdSave(convS, nconv, NULL))
      || (airStrlen(iterS) && nrrdSave(iterS, niter, NULL))) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing output:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
TEND_CMD(mfit, INFO);
