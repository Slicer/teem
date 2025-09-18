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
  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, see <https://www.gnu.org/licenses/>.
*/

#include "../coil.h"

const char *info = ("Test program for coil library.");

int
main(int argc, const char *argv[]) {
  const char *me;
  char *err;
  hestOpt *hopt = NULL;
  airArray *mop;

  mop = airMopNew();
  me = argv[0];
  unsigned int numIters;
  hestOptAdd_1_UInt(&hopt, "iter", "# iters", &numIters, "5",
                    "number of iterations to do processing for");
  unsigned int numThreads;
  hestOptAdd_1_UInt(&hopt, "nt", "# threads", &numThreads, "5",
                    "number of threads to run");
  int kindType;
  hestOptAdd_1_Enum(&hopt, "k", "kind", &kindType, NULL, //
                    "what kind of volume is input", coilKindType);
  int methodType;
  hestOptAdd_1_Enum(&hopt, "m", "method", &methodType, "test",
                    "what kind of filtering to perform", coilMethodType);
  double *_parm;
  unsigned int _parmLen;
  hestOptAdd_Nv_Double(&hopt, "p", "parms", 1, -1, &_parm, NULL,
                       "all the parameters required for filtering method", //
                       &_parmLen);
  int radius;
  hestOptAdd_1_Int(&hopt, "r", "radius", &radius, "1",
                   "radius of filtering neighborhood");
  int verbose;
  hestOptAdd_1_Int(&hopt, "v", "verbose", &verbose, "1", //
                   "verbosity level");
  Nrrd *nin;
  hestOptAdd_1_Other(&hopt, "i", "nin", &nin, "", //
                     "input volume to filter", nrrdHestNrrdNoTTY);
  char *outS;
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-",
                      "output file to save filtering result into");
  hestParseOrDie(hopt, argc - 1, argv + 1, NULL, me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  coilContext *cctx = coilContextNew();
  airMopAdd(mop, cctx, (airMopper)coilContextNix, airMopAlways);
  Nrrd *nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (_parmLen != coilMethodArray[methodType]->parmNum) {
    fprintf(stderr, "%s: %s method wants %u parms, but got %u\n", me,
            coilMethodArray[methodType]->name, coilMethodArray[methodType]->parmNum,
            _parmLen);
    airMopError(mop);
    return 1;
  }
  double parm[COIL_PARMS_NUM];
  for (unsigned int pi = 0; pi < _parmLen; pi++) {
    parm[pi] = _parm[pi];
  }
  if (coilContextAllSet(cctx, nin, coilKindArray[kindType], coilMethodArray[methodType],
                        radius, numThreads, verbose, parm)
      || coilStart(cctx) || coilIterate(cctx, numIters) || coilFinish(cctx)
      || coilOutputGet(nout, cctx)) {
    airMopAdd(mop, err = biffGetDone(COIL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble with coil:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }
  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: couldn't save output:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }
  airMopOkay(mop);
  return 0;
}
