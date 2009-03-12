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

#include "../gage.h"

char *optsigInfo = ("Computes tables of optimal sigmas for hermite-spline "
                    "reconstruction of scale space");

int
main(int argc, char *argv[]) {
  char *me, *outS;
  hestOpt *hopt;
  hestParm *hparm;
  airArray *mop;

  char *err;
  double sigmaMax, convEps, cutoff;
  int E, measr[2];
  unsigned int sampleNumMax, dim, measrSampleNum, maxIter;
  gageOptimSigParm *osparm;
  double *scalePos;

  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  hopt = NULL;
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  hestOptAdd(&hopt, "num", "# samp", airTypeUInt, 1, 1, &sampleNumMax, NULL,
             "maximum number of samples to optimize");
  hestOptAdd(&hopt, "dim", "dimension", airTypeUInt, 1, 1, &dim, "3",
             "dimension of image to work with");
  hestOptAdd(&hopt, "max", "sigma", airTypeDouble, 1, 1, &sigmaMax, NULL,
             "sigma to use for top sample (using 0 for bottom sample)");
  hestOptAdd(&hopt, "cut", "cut", airTypeDouble, 1, 1, &cutoff, "4",
             "at how many sigmas to cut-off discrete gaussian");
  hestOptAdd(&hopt, "mi", "max", airTypeUInt, 1, 1, &maxIter, "10000",
             "maximum # iterations");
  hestOptAdd(&hopt, "N", "# samp", airTypeUInt, 1, 1, &measrSampleNum, "300",
             "number of samples in the measurement of error across scales");
  hestOptAdd(&hopt, "eps", "eps", airTypeDouble, 1, 1, &convEps, "0.00001",
             "convergence threshold for optimization");
  hestOptAdd(&hopt, "m", "m1 m2", airTypeEnum, 2, 2, measr, "l2 l2",
             "how to measure error across image, and across scales",
             NULL, nrrdMeasure);
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
                 me, optsigInfo, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  osparm = gageOptimSigParmNew(sampleNumMax);
  airMopAdd(mop, osparm, AIR_CAST(airMopper, gageOptimSigParmNix),
            airMopAlways);
  scalePos = AIR_CAST(double *, calloc(sampleNumMax, sizeof(double)));
  airMopAdd(mop, scalePos, airFree, airMopAlways);
  if (gageOptimSigTruthSet(osparm, dim, sigmaMax, cutoff, measrSampleNum)) {
    airMopAdd(mop, err = biffGetDone(GAGE), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    airMopError(mop); return 1;
  }
  if (gageOptimSigCalculate(osparm, scalePos, sampleNumMax,
                            measr[0], measr[1], 
                            maxIter, convEps)) {
    airMopAdd(mop, err = biffGetDone(GAGE), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    airMopError(mop); return 1;
  }
  
  nrrdSave("true.nrrd", osparm->ntruth, NULL);
  nrrdSave("err.nrrd", osparm->nerr, NULL);

  airMopOkay(mop);
  exit(0);
}
