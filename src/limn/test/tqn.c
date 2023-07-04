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


#include "../limn.h"

const char *info = ("inspect QN schemes.");

int
main(int argc, const char *argv[]) {
  const char *me;
  char *err;
  hestOpt *hopt=NULL;
  airArray *mop;

  char *outS;
  unsigned int reso;
  int qni;
  Nrrd *nqn;

  mop = airMopNew();
  me = argv[0];
  hestOptAdd(&hopt, "s", "size", airTypeUInt, 1, 1, &reso, "256",
             "resolution");
  hestOptAdd(&hopt, "q", "which", airTypeInt, 1, 1, &qni, NULL,
             "which quantization scheme");
  hestOptAdd(&hopt, "o", "out", airTypeString, 1, 1, &outS, "-",
             "output image");
  hestParseOrDie(hopt, argc-1, argv+1, NULL,
                 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nqn = nrrdNew();
  airMopAdd(mop, nqn, (airMopper)nrrdNuke, airMopAlways);
  if (limnQNDemo(nqn, reso, qni)) {
    airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  if (nrrdSave(outS, nqn, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}

