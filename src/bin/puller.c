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

/* for Teem v2 the Deft stuff was removed from this file; see
   puller-with-Deft.c for that code */

#include <teem/pull.h>
#include <teem/meet.h>

static const char *info
  = ("Command-line interface to the \"pull\" library. "
     "Published research using this tool or the \"pull\" library "
     "should cite the paper: \n "
     "\t\tGordon L. Kindlmann, Ra{\\'u}l San Jos{\\'e} Est{\\'e}par, Stephen M. "
     "Smith,\n "
     "\t\tCarl-Fredrik Westin. Sampling and Visualizing Creases with Scale-Space\n "
     "\t\tParticles. IEEE Trans. on Visualization and Computer Graphics,\n "
     "\t\t15(6):1415-1424 (2009).");

int
main(int argc, const char **argv) {
  hestOpt *hopt = NULL;
  hestParm *hparm;
  airArray *mop;
  const char *me;

  char *err, *outS, *extraOutBaseS, *addLogS, *cachePathSS;
  FILE *addLog;
  meetPullVol **vspec;
  meetPullInfo **idef;
  Nrrd *nPosIn = NULL, *nPosOut;
  pullEnergySpec *enspR, *enspS, *enspWin;
  NrrdKernelSpec *k00, *k11, *k22, *kSSrecon, *kSSblur;
  NrrdBoundarySpec *bspec;
  pullContext *pctx;
  int E = 0, ret = 0;
  unsigned int vspecNum, idefNum;
  double scaleVec[3], glyphScaleRad;
  /* things that used to be set directly inside pullContext */
  int energyFromStrength, nixAtVolumeEdgeSpace, constraintBeforeSeedThresh, binSingle,
    liveThresholdOnInit, permuteOnRebin, noPopCntlWithZeroAlpha, useBetaForGammaLearn,
    restrictiveAddToBins, noAdd, unequalShapesAllow, popCntlEnoughTest,
    convergenceIgnoresPopCntl, zeroZ;
  int verbose;
  int interType, allowCodimension3Constraints, scaleIsTau, useHalton, pointPerVoxel;
  unsigned int samplesAlongScaleNum, pointNumInitial, ppvZRange[2], snap, iterMax,
    stuckIterMax, constraintIterMax, popCntlPeriod, addDescent, iterCallback, rngSeed,
    progressBinMod, threadNum, eipHalfLife, kssOpi, kssFinished, bspOpi, bspFinished;
  double jitter, stepInitial, constraintStepMin, radiusSpace, binWidthSpace, radiusScale,
    alpha, beta, _gamma, wall, energyIncreasePermit, backStepScale, opporStepScale,
    energyDecreaseMin, energyDecreasePopCntlMin, neighborTrueProb, probeProb,
    fracNeighNixedMax;

  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);

  nPosOut = nrrdNew();
  airMopAdd(mop, nPosOut, (airMopper)nrrdNuke, airMopAlways);

  hparm->respFileEnable = AIR_TRUE;
  hparm->respectDashDashHelp = AIR_TRUE;
  me = argv[0];

  hestOptAdd_1_Enum(&hopt, "int", "int", &interType, "justr",
                    "inter-particle energy type", pullInterType);
  hestOptAdd_1_Other(&hopt, "enr", "spec", &enspR, "cotan",
                     "inter-particle energy, radial component", pullHestEnergySpec);
  hestOptAdd_1_Other(&hopt, "ens", "spec", &enspS, "zero",
                     "inter-particle energy, scale component", pullHestEnergySpec);
  hestOptAdd_1_Other(&hopt, "enw", "spec", &enspWin, "butter:16,0.8",
                     "windowing to create locality with additive "
                     "scale-space interaction (\"-int add\")",
                     pullHestEnergySpec);
  hestOptAdd_1_Bool(&hopt, "zz", "bool", &zeroZ, "false",
                    "always constrain Z=0, to process 2D images");
  hestOptAdd_1_Bool(&hopt, "efs", "bool", &energyFromStrength, "false",
                    "whether or not strength contributes to particle-image energy");
  hestOptAdd_1_Bool(&hopt, "nave", "bool", &nixAtVolumeEdgeSpace, "false",
                    "whether or not to nix points at edge of volume, where gage had "
                    "to invent values for kernel support");
  hestOptAdd_1_Bool(&hopt, "cbst", "bool", &constraintBeforeSeedThresh, "false",
                    "during initialization, try constraint satisfaction before "
                    "testing seedThresh");
  hestOptAdd_Flag(&hopt, "noadd", &noAdd, "turn off adding during population control");
  hestOptAdd_1_Bool(&hopt, "usa", "bool", &unequalShapesAllow, "false",
                    "allow volumes to have different shapes (false is safe as "
                    "different volume sizes are often accidental)");
  hestOptAdd_1_Bool(&hopt, "pcet", "bool", &popCntlEnoughTest, "true",
                    "use neighbor-counting \"enough\" heuristic to "
                    "bail out of pop cntl");
  hestOptAdd_1_Bool(&hopt, "cipc", "bool", &convergenceIgnoresPopCntl, "false",
                    "convergence test doesn't care if there has been "
                    "recent changes due to population control");
  hestOptAdd_Flag(&hopt, "nobin", &binSingle,
                  "turn off spatial binning (which prevents multi-threading "
                  "from being useful), for debugging or speed-up measurement");
  hestOptAdd_1_Bool(&hopt, "lti", "bool", &liveThresholdOnInit, "true",
                    "impose liveThresh on initialization");
  hestOptAdd_1_Bool(&hopt, "por", "bool", &permuteOnRebin, "true",
                    "permute points during rebinning");
  hestOptAdd_1_Bool(&hopt, "npcwza", "bool", &noPopCntlWithZeroAlpha, "false",
                    "no pop cntl with zero alpha");
  hestOptAdd_1_Bool(&hopt, "ubfgl", "bool", &useBetaForGammaLearn, "false",
                    "use beta for gamma learning");
  hestOptAdd_1_Bool(&hopt, "ratb", "bool", &restrictiveAddToBins, "true",
                    "be choosy when adding points to bins to avoid overlap");
  hestOptAdd_3_Double(&hopt, "svec", "vec", scaleVec, "0 0 0",
                      "if non-zero (length), vector to use for displaying scale "
                      "in 3-space");
  hestOptAdd_1_Double(&hopt, "gssr", "rad", &glyphScaleRad, "0.0",
                      "if non-zero (length), scaling of scale to cylindrical tensors");
  hestOptAdd_1_Int(&hopt, "v", "verbosity", &verbose, "1", "verbosity level");
  hestOptAdd_Nv_Other(&hopt, "vol", "vol0 vol1", 1, -1, &vspec, NULL,
                      "input volumes, in format <filename>:<kind>:<volname>", &vspecNum,
                      meetHestPullVol);
  hestOptAdd_Nv_Other(&hopt, "info", "info0 info1", 1, -1, &idef, NULL,
                      "info definitions, in format "
                      "<info>[-c]:<volname>:<item>[:<zero>:<scale>]",
                      &idefNum, meetHestPullInfo);

  hestOptAdd_1_Other(&hopt, "k00", "kern00", &k00, "cubic:1,0",
                     "kernel for gageKernel00", nrrdHestKernelSpec);
  hestOptAdd_1_Other(&hopt, "k11", "kern11", &k11, "cubicd:1,0",
                     "kernel for gageKernel11", nrrdHestKernelSpec);
  hestOptAdd_1_Other(&hopt, "k22", "kern22", &k22, "cubicdd:1,0",
                     "kernel for gageKernel22", nrrdHestKernelSpec);

  hestOptAdd_1_String(&hopt, "sscp", "path", &cachePathSS, "./",
                      "path (without trailing /) for where to read/write "
                      "pre-blurred volumes for scale-space");
  kssOpi = hestOptAdd_1_Other(&hopt, "kssb", "kernel", &kSSblur, "dgauss:1,5",
                              "default blurring kernel, to sample scale space",
                              nrrdHestKernelSpec);
  bspOpi = hestOptAdd_1_Other(&hopt, "bsp", "boundary", &bspec, "wrap",
                              "default boundary behavior of scale-space blurring",
                              nrrdHestBoundarySpec);
  hestOptAdd_1_Other(&hopt, "kssr", "kernel", &kSSrecon, "hermite",
                     "kernel for reconstructing from scale space samples",
                     nrrdHestKernelSpec);
  hestOptAdd_1_UInt(&hopt, "nss", "# scl smpls", &samplesAlongScaleNum, "1",
                    "if using \"-ppv\", number of samples along scale axis "
                    "for each spatial position");

  hestOptAdd_1_UInt(&hopt, "np", "# points", &pointNumInitial, "1000",
                    "number of points to start in system");
  hestOptAdd_Flag(&hopt, "halton", &useHalton,
                  "use Halton sequence initialization instead of "
                  "uniform random");
  /* really signed; see pull.h */
  hestOptAdd_1_Int(&hopt, "ppv", "# pnts/vox", &pointPerVoxel, "0",
                   "number of points per voxel to start in simulation "
                   "(need to have a seed thresh vol, overrides \"-np\")");
  hestOptAdd_2_UInt(&hopt, "ppvzr", "z range", ppvZRange, "1 0",
                    "range of Z slices (1st num < 2nd num) to do ppv in, or, "
                    "\"1 0\" for whole volume");
  hestOptAdd_1_Double(&hopt, "jit", "jitter", &jitter, "0",
                      "amount of jittering to do with ppv");
  hestOptAdd_1_Other(&hopt, "pi", "npos", &nPosIn, "",
                     "4-by-N array of positions to start at (overrides \"-np\")",
                     nrrdHestNrrd);
  hestOptAdd_1_Double(&hopt, "step", "step", &stepInitial, "1",
                      "initial step size for gradient descent");
  hestOptAdd_1_Double(&hopt, "csm", "step", &constraintStepMin, "0.0001",
                      "convergence criterion for constraint satisfaction");
  hestOptAdd_1_UInt(&hopt, "snap", "# iters", &snap, "0",
                    "if non-zero, # iters between saved snapshots");
  hestOptAdd_1_UInt(&hopt, "maxi", "# iters", &iterMax, "0",
                    "if non-zero, max # iterations to run whole system");
  hestOptAdd_1_UInt(&hopt, "stim", "# iters", &stuckIterMax, "5",
                    "if non-zero, max # iterations to allow a particle "
                    " to be stuck before nixing");
  hestOptAdd_1_UInt(&hopt, "maxci", "# iters", &constraintIterMax, "15",
                    "if non-zero, max # iterations for contraint enforcement");
  hestOptAdd_1_Double(&hopt, "irad", "scale", &radiusSpace, "1",
                      "particle radius in spatial domain");
  hestOptAdd_1_Double(&hopt, "srad", "scale", &radiusScale, "1",
                      "particle radius in scale domain");
  hestOptAdd_1_Double(&hopt, "bws", "bin width", &binWidthSpace, "1.001",
                      "spatial bin width as multiple of spatial radius");
  hestOptAdd_1_Double(&hopt, "alpha", "alpha", &alpha, "0.5",
                      "blend between particle-image (alpha=0) and "
                      "inter-particle (alpha=1) energies");
  hestOptAdd_1_Double(&hopt, "beta", "beta", &beta, "1.0",
                      "when using Phi2 energy, blend between pure "
                      "space repulsion (beta=0) and "
                      "scale attraction (beta=1)");
  hestOptAdd_1_Double(&hopt, "gamma", "gamma", &_gamma, "1.0",
                      "scaling factor on energy from strength");
  hestOptAdd_1_Double(&hopt, "wall", "k", &wall, "0.0", "spring constant on walls");
  hestOptAdd_1_Double(&hopt, "eip", "k", &energyIncreasePermit, "0.0",
                      "amount by which its okay for *per-particle* energy to increase "
                      "during gradient descent process");
  hestOptAdd_1_Double(&hopt, "ess", "scl", &backStepScale, "0.5",
                      "when energy goes up instead of down, scale step "
                      "size by this");
  hestOptAdd_1_Double(&hopt, "oss", "scl", &opporStepScale, "1.0",
                      "opportunistic scaling (hopefully up, >1) of step size "
                      "on every iteration");
  hestOptAdd_1_Double(&hopt, "edmin", "frac", &energyDecreaseMin, "0.0001",
                      "convergence threshold: stop when fractional improvement "
                      "(decrease) in energy dips below this");
  hestOptAdd_1_Double(&hopt, "edpcmin", "frac", &energyDecreasePopCntlMin, "0.01",
                      "population control is triggered when energy improvement "
                      "goes below this threshold");
  hestOptAdd_1_Double(&hopt, "fnnm", "frac", &fracNeighNixedMax, "0.25",
                      "don't nix if this fraction (or more) of neighbors "
                      "have been nixed");
  hestOptAdd_1_UInt(&hopt, "pcp", "period", &popCntlPeriod, "20",
                    "# iters to wait between attempts at population control");
  hestOptAdd_1_UInt(&hopt, "iad", "# iters", &addDescent, "10",
                    "# iters to run descent on tentative new points during PC");
  hestOptAdd_1_UInt(&hopt, "icb", "# iters", &iterCallback, "1",
                    "periodicity of calling rendering callback");

  hestOptAdd_1_Bool(&hopt, "ac3c", "ac3c", &allowCodimension3Constraints, "false",
                    "allow codimensions 3 constraints");
  hestOptAdd_1_Bool(&hopt, "sit", "sit", &scaleIsTau, "false", "scale is tau");
  hestOptAdd_1_UInt(&hopt, "rng", "seed", &rngSeed, "42", "base seed value for RNGs");
  hestOptAdd_1_UInt(&hopt, "pbm", "mod", &progressBinMod, "50", "progress bin mod");
  hestOptAdd_1_UInt(&hopt, "eiphl", "hl", &eipHalfLife, "0",
                    "half-life of energyIncreasePermute (\"-eip\")");
  hestOptAdd_1_UInt(&hopt, "nt", "# threads", &threadNum, "1",
                    (airThreadCapable
                       ? "number of threads hoover should use"
                       : "if threads where enabled in this Teem build, this is how "
                         "you would control the number of threads to use"));
  hestOptAdd_1_Double(&hopt, "nprob", "prob", &neighborTrueProb, "1.0",
                      "do full neighbor discovery with this probability");
  hestOptAdd_1_Double(&hopt, "pprob", "prob", &probeProb, "1.0",
                      "probe local image values with this probability");

  hestOptAdd_1_String(&hopt, "addlog", "fname", &addLogS, "",
                      "name of file in which to log all particle additions");
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-",
                      "filename for saving computed positions");
  hestOptAdd_1_String(&hopt, "eob", "base", &extraOutBaseS, "",
                      "save extra info (besides position), and use this string as "
                      "the base of the filenames.  Not using this means the extra "
                      "info is not saved.");

  hestParseOrDie(hopt, argc - 1, argv + 1, hparm, me, info, AIR_TRUE, AIR_TRUE,
                 AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  /*
  airEnumPrint(stderr, gageScl);
  exit(0);
  */
  if (airStrlen(addLogS)) {
    if (!(addLog = airFopen(addLogS, stdout, "w"))) {
      fprintf(stderr, "%s: couldn't open %s for writing", me, addLogS);
      airMopError(mop);
      return 1;
    }
    airMopAdd(mop, addLog, (airMopper)airFclose, airMopAlways);
  } else {
    addLog = NULL;
  }

  pctx = pullContextNew();
  airMopAdd(mop, pctx, (airMopper)pullContextNix, airMopAlways);
  if (pullVerboseSet(pctx, verbose) || pullFlagSet(pctx, pullFlagZeroZ, zeroZ)
      || pullFlagSet(pctx, pullFlagEnergyFromStrength, energyFromStrength)
      || pullFlagSet(pctx, pullFlagNixAtVolumeEdgeSpace, nixAtVolumeEdgeSpace)
      || pullFlagSet(pctx, pullFlagConstraintBeforeSeedThresh,
                     constraintBeforeSeedThresh)
      || pullFlagSet(pctx, pullFlagPopCntlEnoughTest, popCntlEnoughTest)
      || pullFlagSet(pctx, pullFlagConvergenceIgnoresPopCntl, convergenceIgnoresPopCntl)
      || pullFlagSet(pctx, pullFlagBinSingle, binSingle)
      || pullFlagSet(pctx, pullFlagNoAdd, noAdd)
      || pullFlagSet(pctx, pullFlagPermuteOnRebin, permuteOnRebin)
      || pullFlagSet(pctx, pullFlagNoPopCntlWithZeroAlpha, noPopCntlWithZeroAlpha)
      || pullFlagSet(pctx, pullFlagUseBetaForGammaLearn, useBetaForGammaLearn)
      || pullFlagSet(pctx, pullFlagRestrictiveAddToBins, restrictiveAddToBins)
      || pullFlagSet(pctx, pullFlagAllowCodimension3Constraints,
                     allowCodimension3Constraints)
      || pullFlagSet(pctx, pullFlagScaleIsTau, scaleIsTau)
      || pullInitUnequalShapesAllowSet(pctx, unequalShapesAllow)
      || pullIterParmSet(pctx, pullIterParmSnap, snap)
      || pullIterParmSet(pctx, pullIterParmMax, iterMax)
      || pullIterParmSet(pctx, pullIterParmStuckMax, stuckIterMax)
      || pullIterParmSet(pctx, pullIterParmConstraintMax, constraintIterMax)
      || pullIterParmSet(pctx, pullIterParmPopCntlPeriod, popCntlPeriod)
      || pullIterParmSet(pctx, pullIterParmAddDescent, addDescent)
      || pullIterParmSet(pctx, pullIterParmCallback, iterCallback)
      || pullIterParmSet(pctx, pullIterParmEnergyIncreasePermitHalfLife, eipHalfLife)
      || pullSysParmSet(pctx, pullSysParmStepInitial, stepInitial)
      || pullSysParmSet(pctx, pullSysParmConstraintStepMin, constraintStepMin)
      || pullSysParmSet(pctx, pullSysParmRadiusSpace, radiusSpace)
      || pullSysParmSet(pctx, pullSysParmRadiusScale, radiusScale)
      || pullSysParmSet(pctx, pullSysParmBinWidthSpace, binWidthSpace)
      || pullSysParmSet(pctx, pullSysParmAlpha, alpha)
      || pullSysParmSet(pctx, pullSysParmBeta, beta)
      || pullSysParmSet(pctx, pullSysParmGamma, _gamma)
      || pullSysParmSet(pctx, pullSysParmWall, wall)
      || pullSysParmSet(pctx, pullSysParmEnergyIncreasePermit, energyIncreasePermit)
      || pullSysParmSet(pctx, pullSysParmEnergyDecreaseMin, energyDecreaseMin)
      || pullSysParmSet(pctx, pullSysParmFracNeighNixedMax, fracNeighNixedMax)
      || pullSysParmSet(pctx, pullSysParmEnergyDecreasePopCntlMin,
                        energyDecreasePopCntlMin)
      || pullSysParmSet(pctx, pullSysParmBackStepScale, backStepScale)
      || pullSysParmSet(pctx, pullSysParmOpporStepScale, opporStepScale)
      || pullSysParmSet(pctx, pullSysParmNeighborTrueProb, neighborTrueProb)
      || pullSysParmSet(pctx, pullSysParmProbeProb, probeProb)
      || pullRngSeedSet(pctx, rngSeed) || pullProgressBinModSet(pctx, progressBinMod)
      || pullThreadNumSet(pctx, threadNum)
      || pullInterEnergySet(pctx, interType, enspR, enspS, enspWin)
      || pullInitLiveThreshUseSet(pctx, liveThresholdOnInit)
      || pullLogAddSet(pctx, addLog)) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble with flags:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  if (nPosIn) {
    E = pullInitGivenPosSet(pctx, nPosIn);
  } else if (pointPerVoxel) {
    E = pullInitPointPerVoxelSet(pctx, pointPerVoxel, ppvZRange[0], ppvZRange[1],
                                 samplesAlongScaleNum, jitter);
  } else if (useHalton) {
    E = pullInitHaltonSet(pctx, pointNumInitial, 0);
  } else {
    E = pullInitRandomSet(pctx, pointNumInitial);
  }
  if (E) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble with flags:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (meetPullVolStackBlurParmFinishMulti(vspec, vspecNum, &kssFinished, &bspFinished,
                                          kSSblur, bspec)
      || meetPullVolLoadMulti(vspec, vspecNum, cachePathSS, verbose)
      || meetPullVolAddMulti(pctx, vspec, vspecNum, k00, k11, k22, kSSrecon)
      || meetPullInfoAddMulti(pctx, idef, idefNum)) {
    airMopAdd(mop, err = biffGetDone(MEET), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble with volumes or infos:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (!kssFinished && hestSourceUser == hopt[kssOpi].source) {
    fprintf(stderr,
            "\n\n%s: WARNING! Used the -%s flag, but the "
            "meetPullVol specified blurring kernels\n\n\n",
            me, hopt[kssOpi].flag);
  }
  if (!bspFinished && hestSourceUser == hopt[bspOpi].source) {
    fprintf(stderr,
            "\n\n%s: WARNING! Used the -%s flag, but the "
            "meetPullVol specified boundary specs\n\n\n",
            me, hopt[bspOpi].flag);
  }
  if (pullStart(pctx)) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble starting system:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  /* -------------------------------------------------- */

  /* not sure when this table was created, don't have heart to nix it
   *
   *                 hght scl   tang1    tang2   mode scl  strength
   *  ridge surface:    -1      evec2      -        -       -eval2
   *     ridge line:    -1      evec2    evec1      -       -eval1
   *     all ridges:    -1      evec2    evec1     +1        ??
   * valley surface:    +1      evec0      -        -        eval0
   *    valley line:    +1      evec0    evec1      -        eval1
   *      all lines:    +1      evec0    evec1     -1
   */

  pullFinish(pctx);
  airMopOkay(mop);
  return ret;
}
