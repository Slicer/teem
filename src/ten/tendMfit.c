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

#include "ten.h"
#include "privateTen.h"

#define INFO "Estimate models from a set of DW images"
char *_tend_mfitInfoL =
  (INFO
   ". More docs here.");

int
tend_mfitMain(int argc, char **argv, char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  Nrrd *nin, *nout;
  char *outS;
  int EE, knownB0, verbose, mlfit;
  unsigned int maxIter;
  double valueMin, thresh, sigma;
  const tenModel *model;

  hestOptAdd(&hopt, "v", "verbose", airTypeInt, 1, 1, &verbose, "0",
             "verbosity level");
  hestOptAdd(&hopt, "ml", NULL, airTypeInt, 0, 0, &mlfit, NULL,
             "do ML fitting, rather than least-squares, which also "
             "requires setting \"-sigma\"");
  hestOptAdd(&hopt, "sigma", "sigma", airTypeDouble, 1, 1, &sigma, "nan",
             "Rician noise parameter");
  hestOptAdd(&hopt, "maxi", "max iters", airTypeUInt, 1, 1, &maxIter, "10",
             "maximum allowable iterations for fitting.");
  hestOptAdd(&hopt, "knownB0", "bool", airTypeBool, 1, 1, &knownB0, NULL,
             "Indicates if the B=0 non-diffusion-weighted reference image "
             "is known (\"true\"), or if it has to be estimated along with "
             "the other model parameters (\"false\")");
  hestOptAdd(&hopt, "i", "dwi", airTypeOther, 1, 1, &nin, "-",
             "all the diffusion-weighted images in one 4D nrrd",
             NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "-",
             "output tensor volume");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_mfitInfoL);
  JUSTPARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  /*  
  if (tenModelSqeFit(nout, NULL, 
                              const tenModel *model,
                              const tenExperSpec *espec, const Nrrd *ndwi,
                              int knownB0, int saveB0, int typeOut,
                              unsigned int maxIter, double convEps);

  if (tenEstimate1TensorVolume4D(tec, nout, &nB0,
                                 airStrlen(terrS) 
                                 ? &nterr 
                                 : NULL, 
                                 nin4d, nrrdTypeFloat)) {
    airMopAdd(mop, err=biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble doing estimation:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  */
  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
/* TEND_CMD(mfit, INFO); */
unrrduCmd tend_mfitCmd = { "mfit", INFO, tend_mfitMain };
