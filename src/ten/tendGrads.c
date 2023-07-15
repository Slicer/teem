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

#define INFO "Calculate balanced gradient directions for DWI acquisition"
static const char *_tend_gradsInfoL
  = (INFO ", based on a simulation of anti-podal point pairs repelling each other "
          "on the unit sphere surface. This can either distribute more uniformly "
          "a given set of gradients, or it can make a new distribution from scratch. "
          "A more clever implementation could decrease drag with time, as the "
          "solution converges, to get closer to the minimum energy configuration "
          "faster.  In the mean time, you can run a second pass on the output of "
          "the first pass, using lower drag. A second phase of the algorithm "
          "tries sign changes in gradient directions in trying to find an optimally "
          "balanced set of directions.  This uses a randomized search, so if it "
          "doesn't seem to be finishing in a reasonable amount of time, try "
          "restarting with a different \"-seed\".");

static int
tend_gradsMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  int num, E;
  Nrrd *nin, *nout;
  char *outS;
  tenGradientParm *tgparm;
  unsigned int seed;

  mop = airMopNew();
  tgparm = tenGradientParmNew();
  airMopAdd(mop, tgparm, (airMopper)tenGradientParmNix, airMopAlways);

  hestOptAdd_1_Int(&hopt, "n", "# dir", &num, "6",
                   "desired number of diffusion gradient directions");
  hestOptAdd_1_Other(&hopt, "i", "grads", &nin, "",
                     "initial gradient directions to start with, instead "
                     "of default random initial directions (overrides \"-n\")",
                     nrrdHestNrrd);
  hestOptAdd_1_UInt(&hopt, "seed", "value", &seed, "42",
                    "seed value to used with airSrandMT()");
  hestOptAdd_1_Double(&hopt, "step", "step", &(tgparm->initStep), "1.0",
                      "time increment in solver");
  hestOptAdd_Flag(&hopt, "single", &(tgparm->single),
                  "instead of the default behavior of tracking a pair of "
                  "antipodal points (appropriate for determining DWI gradients), "
                  "use only single points (appropriate for who knows what).");
  hestOptAdd_1_UInt(&hopt, "snap", "interval", &(tgparm->snap), "0",
                    "specifies an interval between which snapshots of the point "
                    "positions should be saved out.  By default (not using this "
                    "option), there is no such snapshot behavior");
  hestOptAdd_1_Double(&hopt, "jitter", "jitter", &(tgparm->jitter), "0.1",
                      "amount by which to perturb points when given an input nrrd");
  hestOptAdd_1_UInt(&hopt, "miniter", "# iters", &(tgparm->minIteration), "0",
                    "max number of iterations for which to run the simulation");
  hestOptAdd_1_UInt(&hopt, "maxiter", "# iters", &(tgparm->maxIteration), "1000000",
                    "max number of iterations for which to run the simulation");
  hestOptAdd_1_Double(&hopt, "minvelo", "vel", &(tgparm->minVelocity), "0.00001",
                      "low threshold on mean velocity of repelling points, "
                      "at which point repulsion phase of algorithm terminates.");
  hestOptAdd_1_Double(&hopt, "exp", "exponent", &(tgparm->expo_d), "1",
                      "the exponent n that determines the potential energy 1/r^n.");
  hestOptAdd_1_Double(&hopt, "dp", "potential change", &(tgparm->minPotentialChange),
                      "0.000000001",
                      "low threshold on fractional change of potential at "
                      "which point repulsion phase of algorithm terminates.");
  hestOptAdd_1_Double(&hopt, "minimprov", "delta", &(tgparm->minMeanImprovement),
                      "0.00005",
                      "in the second phase of the algorithm, "
                      "when stochastically balancing the sign of the gradients, "
                      "the (small) improvement in length of mean gradient "
                      "which triggers termination (as further improvements "
                      "are unlikely.");
  hestOptAdd_1_Double(&hopt, "minmean", "len", &(tgparm->minMean), "0.0001",
                      "if length of mean gradient falls below this, finish "
                      "the balancing phase");
  hestOptAdd_1_Bool(&hopt, "izv", "insert", &(tgparm->insertZeroVec), "false",
                    "adding zero vector at beginning of grads");
  hestOptAdd_1_String(&hopt, "o", "filename", &outS, "-",
                      "file to write output nrrd to");
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_gradsInfoL);
  JUSTPARSE(_tend_gradsInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  /* see if it was an integral exponent */
  tgparm->expo = AIR_UINT(tgparm->expo_d);
  if (tgparm->expo == tgparm->expo_d) {
    /* ooo, it was */
    tgparm->expo_d = 0;
  } else {
    /* no, its non-integral, indicate this as follows */
    tgparm->expo = 0;
  }
  tgparm->seed = seed;
  if (tgparm->snap) {
    tgparm->report = tgparm->snap;
  }
  E = (nin ? tenGradientDistribute(nout, nin, tgparm)
           : tenGradientGenerate(nout, num, tgparm));
  if (E) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble making distribution:\n%s\n", me, err);
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
TEND_CMD(grads, INFO);
