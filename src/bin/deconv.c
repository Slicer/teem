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

#include <stdio.h>

#include <teem/biff.h>
#include <teem/hest.h>
#include <teem/nrrd.h>
#include <teem/gage.h>
#include <teem/ten.h>
#include <teem/meet.h>

#define SPACING(spc) (AIR_EXISTS(spc) ? spc : nrrdDefaultSpacing)

/* copied this from ten.h; I don't want gage to depend on ten */
#define PROBE_MAT2LIST(l, m)                                                            \
  ((l)[1] = (m)[0], (l)[2] = (m)[3], (l)[3] = (m)[6], (l)[4] = (m)[4], (l)[5] = (m)[7], \
   (l)[6] = (m)[8])

static const char *deconvInfo = ("Does deconvolution. ");

int
main(int argc, const char *argv[]) {
  gageKind *kind;
  const char *me;
  char *outS, *err;
  hestParm *hparm;
  hestOpt *hopt = NULL;
  NrrdKernelSpec *ksp;
  int otype, separ, ret;
  unsigned int maxIter;
  double epsilon, lastDiff, step;
  Nrrd *nin, *nout;
  airArray *mop;

  mop = airMopNew();
  me = argv[0];
  hparm = hestParmNew();
  airMopAdd(mop, hparm, AIR_CAST(airMopper, hestParmFree), airMopAlways);
  hparm->elideSingleOtherType = AIR_TRUE;
  hparm->respectDashDashHelp = AIR_TRUE;
  hestOptAdd_1_Other(&hopt, "i", "nin", &nin, NULL, "input volume", nrrdHestNrrd);
  hestOptAdd_1_Other(&hopt, "k", "kind", &kind, NULL,
                     "\"kind\" of volume (\"scalar\", \"vector\", "
                     "\"tensor\", or \"dwi\")",
                     meetHestGageKind);
  hestOptAdd_1_Other(&hopt, "k00", "kernel", &ksp, NULL, "convolution kernel",
                     nrrdHestKernelSpec);
  hestOptAdd_1_UInt(&hopt, "mi", "max # iters", &maxIter, "100",
                    "maximum number of iterations with which to compute the "
                    "deconvolution");
  hestOptAdd_1_Double(&hopt, "e", "epsilon", &epsilon, "0.00000001",
                      "convergence threshold");
  hestOptAdd_1_Double(&hopt, "s", "step", &step, "1.0", "scaling of value update");
  hestOptAdd_1_Other(&hopt, "t", "type", &otype, "default",
                     "type to save output as. By default (not using this option), "
                     "the output type is the same as the input type",
                     &unrrduHestMaybeTypeCB);
  hestOptAdd_1_Bool(&hopt, "sep", "bool", &separ, "false",
                    "use fast separable deconvolution instead of brain-dead "
                    "brute-force iterative method");
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-", "output volume");
  hestParseOrDie(hopt, argc - 1, argv + 1, hparm, me, deconvInfo, AIR_TRUE, AIR_TRUE,
                 AIR_TRUE);
  airMopAdd(mop, hopt, AIR_CAST(airMopper, hestOptFree), airMopAlways);
  airMopAdd(mop, hopt, AIR_CAST(airMopper, hestParseFree), airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, AIR_CAST(airMopper, nrrdNuke), airMopAlways);

  if (separ) {
    ret = gageDeconvolveSeparable(nout, nin, kind, ksp, otype);
  } else {
    ret = gageDeconvolve(nout, &lastDiff, nin, kind, ksp, otype, maxIter, AIR_TRUE, step,
                         epsilon, 1);
  }
  if (ret) {
    airMopAdd(mop, err = biffGetDone(GAGE), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble saving output:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
