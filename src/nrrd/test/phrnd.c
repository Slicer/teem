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

#include "../nrrd.h"

#define NRRDNEW(X)                                                                      \
  (X) = nrrdNew();                                                                      \
  airMopAdd(mop, (X), (airMopper)nrrdNuke, airMopAlways)

const char *phrndInfo = ("randomizes phase of a real-valued array "
                         "while preserving the spectrum");

int
main(int argc, const char *argv[]) {
  const char *me;
  hestOpt *hopt;
  hestParm *hparm;
  airArray *mop;

  int rigor;
  char *err, *outS, *imagOutS, *wispath, *seedS;
  Nrrd *ntmp, *ntmp2, /* tmp */
    *njarg[2],        /* arguments to join */
    *nrin,            /* given real-valued input */
    *nrdin,           /* given real-valued input, as double */
    *ncin,            /* (padded) complex-valued input */
    *ncfin,           /* complex-valued transform of input */
    *nR,              /* real part of something */
    *nI,              /* imag part of something */
    *nP,              /* phase */
    *nM,              /* mag */
    *nlut,            /* phase look-up table */
    *ncfout,          /* complex-valued transform of output */
    *ncdout,          /* double complex-valued output */
    *ncout,           /* complex-valued output, as input type */
    *niout,           /* imaginary output */
    *nrout;           /* real output */
  double howrand, *lut, *P;
  unsigned int len, axi, seed;
  size_t II, NN, minInset[NRRD_DIM_MAX];
  FILE *fwise;
  unsigned int axes[NRRD_DIM_MAX];
  ptrdiff_t minPad[NRRD_DIM_MAX], maxPad[NRRD_DIM_MAX];

  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  hopt = NULL;
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nrin, NULL, "input array", NULL,
             NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "n", "len", airTypeUInt, 1, 1, &len, "65536",
             "length (must be EVEN) of phase look-up table, used "
             "enable being dumb (rather than clever) in asserting "
             "the phase symmetries arising from real input");
  hestOptAdd(&hopt, "r", "howrandom", airTypeDouble, 1, 1, &howrand, "1.0",
             "how much to randomize input phase; 0.0 means "
             "that output should be same as input");
  hestOptAdd(&hopt, "pr,planrigor", "pr", airTypeEnum, 1, 1, &rigor, "est",
             "rigor with which fftw plan is constructed. Options include:\n "
             "\b\bo \"e\", \"est\", \"estimate\": only an estimate\n "
             "\b\bo \"m\", \"meas\", \"measure\": standard amount of "
             "measurements of system properties\n "
             "\b\bo \"p\", \"pat\", \"patient\": slower, more measurements\n "
             "\b\bo \"x\", \"ex\", \"exhaustive\": slowest, most measurements",
             NULL, nrrdFFTWPlanRigor);
  hestOptAdd(&hopt, "w,wisdom", "filename", airTypeString, 1, 1, &wispath, "",
             "A filename here is used to read in fftw wisdom (if the file "
             "exists already), and is used to save out updated wisdom "
             "after the transform.  By default (not using this option), "
             "no wisdom is read or saved. Note: no wisdom is gained "
             "(that is, learned by FFTW) with planning rigor \"estimate\".");
  hestOptAdd(&hopt, "s,seed", "seed", airTypeString, 1, 1, &seedS, "",
             "seed value for RNG for rand and nrand, so that you "
             "can get repeatable results between runs, or, "
             "by not using this option, the RNG seeding will be "
             "based on the current time");
  hestOptAdd(&hopt, "io", "filename", airTypeString, 1, 1, &imagOutS, "",
             "if a filename is given with this option, the imaginary "
             "component of the transformed output is saved here, to "
             "permit confirming that it doesn't carry significant info");
  hestOptAdd(&hopt, "o", "filename", airTypeString, 1, 1, &outS, NULL,
             "file to write output nrrd to");
  hestParseOrDie(hopt, argc - 1, argv + 1, hparm, me, phrndInfo, AIR_TRUE, AIR_TRUE,
                 AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (0 != len % 2) {
    fprintf(stderr, "%s: given length %u is not even\n", me, len);
    airMopError(mop);
    return 1;
  }
  if (airStrlen(seedS)) {
    if (1 != sscanf(seedS, "%u", &seed)) {
      fprintf(stderr, "%s: couldn't parse seed \"%s\" as uint\n", me, seedS);
      airMopError(mop);
      return 1;
    } else {
      airSrandMT(seed);
    }
  } else {
    /* got no request for specific seed */
    airSrandMT(AIR_UINT(airTime()));
  }
  for (axi = 0; axi < NRRD_DIM_MAX; axi++) {
    minInset[axi] = 0;
  }
  /* pointless to set content */
  nrrdStateDisableContent = AIR_TRUE;

  /* ============== pad real input nrin to complex-valued input ncin */
  minPad[0] = 0;
  maxPad[0] = 1;
  for (axi = 0; axi < nrin->dim; axi++) {
    minPad[axi + 1] = 0;
    maxPad[axi + 1] = AIR_CAST(ptrdiff_t, nrin->axis[axi].size - 1);
  }
  NRRDNEW(nrdin);
  NRRDNEW(ntmp);
  NRRDNEW(ncin);
  if (nrrdConvert(nrdin, nrin, nrrdTypeDouble) || nrrdAxesInsert(ntmp, nrdin, 0)
      || nrrdPad_nva(ncin, ntmp, minPad, maxPad, nrrdBoundaryPad, 0.0)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error creating complex input:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  /* ============== learn possible wisdom */
  if (airStrlen(wispath) && nrrdFFTWEnabled) {
    fwise = fopen(wispath, "r");
    if (fwise) {
      if (nrrdFFTWWisdomRead(fwise)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: error with fft wisdom:\n%s", me, err);
        airMopError(mop);
        return 1;
      }
      fclose(fwise);
    } else {
      fprintf(stderr,
              "%s: (\"%s\" couldn't be opened, will try to save "
              "wisdom afterwards)",
              me, wispath);
    }
  }

  /* ============== transform input to phase and magnitude */
  for (axi = 0; axi < nrin->dim; axi++) {
    axes[axi] = axi + 1;
  }
  NRRDNEW(ncfin);
  NRRDNEW(nR);
  NRRDNEW(nI);
  NRRDNEW(nP);
  NRRDNEW(nM);
  if (nrrdFFT(ncfin, ncin, axes, nrin->dim, +1, AIR_TRUE, rigor)
      || nrrdSlice(nR, ncfin, 0, 0) || nrrdSlice(nI, ncfin, 0, 1)
      || nrrdArithBinaryOp(nP, nrrdBinaryOpAtan2, nI, nR)
      || nrrdProject(nM, ncfin, 0, nrrdMeasureL2, nrrdTypeDefault)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error processing input:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  /* ============== randomize phase */
  NRRDNEW(nlut);
  if (nrrdMaybeAlloc_va(nlut, nrrdTypeDouble, 1, AIR_SIZE_T(len))) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error making lut:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  lut = AIR_CAST(double *, nlut->data);
  for (II = 0; II < len; II++) {
    /* random phase */
    if (II < len / 2) {
      lut[II] = AIR_AFFINE(0, airDrandMT(), 1, -AIR_PI, AIR_PI);
    } else {
      lut[II] = -lut[len - 1 - II];
    }
  }
  NN = nrrdElementNumber(nP);
  P = AIR_CAST(double *, nP->data);
  for (II = 0; II < NN; II++) {
    /* pp is the original input phase */
    double pp = P[II];
    /* pi is the index for the input phase */
    unsigned int pi = airIndex(-AIR_PI, pp, AIR_PI, len);
    /* lin is the best approximation (up to len tables) of original value */
    /* double lin = AIR_AFFINE(-0.5, pi, len-0.5, -AIR_PI, AIR_PI); */
    /* printf("%g %u %g %g\n", P[II], pi, lut[pi], lin); */
    /* lut[pi] is the randomized phase */
    P[II] = AIR_LERP(howrand, pp, lut[pi]);
  }

  /* ============== transform (new) phase and magnitude to output */
  njarg[0] = nR;
  njarg[1] = nI;
  NRRDNEW(ncfout);
  NRRDNEW(ncdout);
  NRRDNEW(ncout);
  NRRDNEW(ntmp2);
  if (nrrdArithUnaryOp(nR, nrrdUnaryOpCos, nP)
      || nrrdArithBinaryOp(nR, nrrdBinaryOpMultiply, nR, nM)
      || nrrdArithUnaryOp(nI, nrrdUnaryOpSin, nP)
      || nrrdArithBinaryOp(nI, nrrdBinaryOpMultiply, nI, nM)
      || nrrdJoin(ncfout, AIR_CAST(const Nrrd *const *, njarg), 2, 0, AIR_TRUE)
      || nrrdFFT(ncdout, ncfout, axes, nrin->dim, -1, AIR_TRUE, rigor)
      || nrrdConvert(ntmp, ncdout, nrin->type) || nrrdConvert(ntmp2, ncin, nrin->type)
      || nrrdInset(ncout, ntmp2, ntmp, minInset)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error creating output\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  /*
  if (nrrdSave("cfin.nrrd", ncfin, NULL) ||
      nrrdSave("cout.nrrd", ncout, NULL) ||
      nrrdSave("cin.nrrd", ncin, NULL) ||
      nrrdSave("tmp.nrrd", ntmp, NULL) ||
      nrrdSave("R.nrrd", nR, NULL) ||
      nrrdSave("I.nrrd", nI, NULL) ||
      nrrdSave("P.nrrd", nP, NULL) ||
      nrrdSave("M.nrrd", nM, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error saving tmps:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  */

  /* ============== DONE. saving things to save */
  if (airStrlen(wispath) && nrrdFFTWEnabled) {
    if (!(fwise = fopen(wispath, "w"))) {
      fprintf(stderr, "%s: couldn't open %s for writing: %s\n", me, wispath,
              strerror(errno));
      airMopError(mop);
      return 1;
    }
    if (nrrdFFTWWisdomWrite(fwise)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error with fft wisdom:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    fclose(fwise);
  }
  if (airStrlen(imagOutS)) {
    NRRDNEW(niout);
    if (nrrdSlice(niout, ncout, 0, 1) || nrrdSave(imagOutS, niout, NULL)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error slicing/saving imaginary output:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
  }
  NRRDNEW(nrout);
  if (nrrdSlice(nrout, ncout, 0, 0) || nrrdSave(outS, nrout, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: problem slicing/saving real output:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  exit(0);
}
