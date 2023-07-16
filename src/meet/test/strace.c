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

#include <teem/pull.h>
#include "../meet.h"

/*

gcc -g  -W -Wall -arch x86_64   strace.c  -o strace -Wl,-prebind \
   -I/Users/gk/teem-install/include \
  -L/Users/gk/teem-install/lib/ \
 -lteem  -lpng -lz -lpthread -lfftw3 -lm

 */

int
findAndTraceMorePoints(Nrrd *nplot, pullContext *pctx, pullVolume *scaleVol,
                       int strengthUse, int smooth, int flatWght, double scaleStep,
                       double scaleHalfLen, double orientTestLen,
                       unsigned int traceArrIncr, pullTraceMulti *mtrc,
                       unsigned int pointNum) {
  static const char me[] = "findAndTraceMorePoints";
  unsigned int pointsSoFar, idtagBase, pidx, addedNum;
  pullTrace *trace;
  pullPoint *point;
  Nrrd *nPosOut;
  airArray *mop;
  double *pos;
  char doneStr[AIR_STRLEN_SMALL + 1];

  mop = airMopNew();
  pointsSoFar = pullPointNumber(pctx);
  idtagBase = pctx->idtagNext;
  point = NULL;
  printf("%s: adding %u new points (to %u; %s) . . .       ", me, pointNum, pointsSoFar,
         airPrettySprintSize_t(doneStr, pullTraceMultiSizeof(mtrc)));
  for (pidx = 0; pidx < pointNum; pidx++) {
    printf("%s", airDoneStr(0, pidx, pointNum, doneStr));
    fflush(stdout);
    if (!point) {
      point = pullPointNew(pctx);
    }
    if (pullPointInitializeRandomOrHalton(pctx, pidx + pointsSoFar, point, scaleVol)) {
      biffAddf(PULL, "%s: trouble trying point %u (id %u)", me, pidx, point->idtag);
      airMopError(mop);
      return 1;
    }
    if (pullBinsPointAdd(pctx, point, NULL)) {
      biffAddf(PULL, "%s: trouble binning point %u", me, point->idtag);
      airMopError(mop);
      return 1;
    }
    point = NULL;
  }
  printf("%s\n", airDoneStr(0, pidx, pointNum, doneStr));
  if (point) {
    /* we created a new test point, but it was never placed in the volume */
    /* so, HACK: undo pullPointNew . . . */
    point = pullPointNix(point);
    /* pctx->idtagNext -= 1; */
  }

  nPosOut = nrrdNew();
  airMopAdd(mop, nPosOut, (airMopper)nrrdNuke, airMopAlways);
  if (pullOutputGetFilter(nPosOut, NULL, NULL, NULL, 0.0, pctx, idtagBase, 0)) {
    biffAddf(PULL, "%s: trouble A", me);
    airMopError(mop);
    return 1;
  }
  pos = AIR_CAST(double *, nPosOut->data);
  addedNum = nPosOut->axis[1].size;
  printf("%s: tracing . . .       ", me);
  for (pidx = 0; pidx < addedNum; pidx++) {
    double seedPos[4];
    int added;
    printf("%s", airDoneStr(0, pidx, addedNum, doneStr));
    fflush(stdout);
    trace = pullTraceNew();
    ELL_4V_COPY(seedPos, pos + 4 * pidx);
    if (pullTraceSet(pctx, trace, AIR_TRUE /* recordStrength */, scaleStep, scaleHalfLen,
                     orientTestLen, traceArrIncr, seedPos)
        || pullTraceMultiAdd(mtrc, trace, &added)) {
      biffAddf(PULL, "%s: trouble on point %u", me, pidx);
      airMopError(mop);
      return 1;
    }
    if (!added) {
      trace = pullTraceNix(trace);
    }
  }
  printf("%s\n", airDoneStr(0, pidx, pointNum, doneStr));

  if (pullTraceMultiPlotAdd(nplot, mtrc, NULL, strengthUse, smooth, flatWght, 0, 0, NULL,
                            NULL)) {
    biffAddf(PULL, "%s: trouble w/ PlotAdd (A)", me);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}

#define LOFF 1.000001

int
resamplePlot(Nrrd *nprob, const Nrrd *nplot) {
  static const char me[] = "resamplePlot";
  unsigned int ii, nn, sx, sy;

  double scls[2] = {0.2, 0.2}, kparm[2] = {1.0, 0.0};
  const NrrdKernel *kern = nrrdKernelTent;

  /*
  double scls[2] = {1.0, 1.0}, kparm[2] = {2.0, 4.0};
  const NrrdKernel *kern = nrrdKernelGaussian;
  */
  double sum, *prob;

  if (nrrdSimpleResample(nprob, nplot, kern, kparm, NULL, scls)) {
    biffMovef(PULL, NRRD, "%s: trouble B", me);
    return 1;
  }
  prob = AIR_CAST(double *, nprob->data);
  sx = AIR_UINT(nprob->axis[0].size);
  sy = AIR_UINT(nprob->axis[1].size);
  for (ii = 0; ii < sx; ii++) {
    /* zero out bottom line, useless crap piles up there */
    prob[ii + sx * (sy - 1)] = 0;
  }
  sum = 0;
  nn = AIR_UINT(nrrdElementNumber(nprob));
  for (ii = 0; ii < nn; ii++) {
    /* sum += log(LOFF+prob[ii]); why? */
    sum += prob[ii];
  }
  if (sum) {
    for (ii = 0; ii < nn; ii++) {
      /* prob[ii] = log(LOFF+prob[ii])/sum; why? */
      prob[ii] = prob[ii] / sum;
    }
  }
  return 0;
}

double
distanceProb(Nrrd *npp, Nrrd *nqq) {
  double *pp, *qq, dist = 0.0;
  unsigned int ii, nn;

  nn = AIR_UINT(nrrdElementNumber(npp));
  pp = AIR_CAST(double *, npp->data);
  qq = AIR_CAST(double *, nqq->data);
  for (ii = 0; ii < nn; ii++) {
    if (qq[ii]) {
      dist += fabs(pp[ii] * log2(0.0000001 + pp[ii] / qq[ii]));
    }
  }
  return dist;
}

void
savePlot(Nrrd *nout) {
  static const char me[] = "saveProb";
  static int count = 0;
  char fname[128], *err;
  sprintf(fname, "pdf-%03u.nrrd", count);
  if (nrrdSave(fname, nout, NULL)) {
    err = biffGetDone(NRRD);
    fprintf(stderr, "%s: HEY couldn't save pdf to %s; moving on ...\n", me, fname);
    free(err);
  }
  count++;
  return;
}

static const char *info = ("Endless hacking!");

int
main(int argc, const char **argv) {
  hestOpt *hopt = NULL;
  hestParm *hparm;
  airArray *mop;
  const char *me;

  char *err, *posOutS, *outS, *extraOutBaseS, *addLogS, *cachePathSS, *tracesInS,
    *tracesOutS, *trcListOutS = NULL, *trcVolOutS = NULL, *traceMaskPosOutS = NULL;
  FILE *addLog, *tracesFile;
  meetPullVol **vspec;
  meetPullInfo **idef;
  Nrrd *nPosIn = NULL, *nPosOut, *nplot, *nplotA, *nplotB, *nfilt, *nTraceMaskIn,
       *nmaskedpos;
  pullEnergySpec *enspR, *enspS, *enspWin;
  NrrdKernelSpec *k00, *k11, *k22, *kSSrecon, *kSSblur;
  NrrdBoundarySpec *bspec;
  pullContext *pctx = NULL;
  pullVolume *scaleVol = NULL;
  pullTraceMulti *mtrc = NULL;
  int ret = 0;
  unsigned int vspecNum, idefNum;
  /* things that used to be set directly inside pullContext */
  int nixAtVolumeEdgeSpace, constraintBeforeSeedThresh, binSingle, liveThresholdOnInit,
    permuteOnRebin, noAdd, unequalShapesAllow, zeroZ, strnUse;
  int verbose;
  int interType, allowCodimension3Constraints, scaleIsTau, smoothPlot, flatWght;
  unsigned int samplesAlongScaleNum, pointNumInitial, pointPerVoxel, ppvZRange[2], snap,
    stuckIterMax, constraintIterMax, rngSeed, progressBinMod, threadNum, tracePointNum,
    passNumMax, kssOpi, kssFinished, bspOpi, bspFinished;
  double jitter, stepInitial, constraintStepMin, radiusSpace, binWidthSpace, radiusScale,
    orientTestLen, backStepScale, opporStepScale, energyDecreaseMin, tpdThresh;

  double sstep, sswin, ssrange[2];
  unsigned int pres[2];

  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);

  nPosOut = nrrdNew();
  airMopAdd(mop, nPosOut, (airMopper)nrrdNuke, airMopAlways);

  mtrc = pullTraceMultiNew();
  airMopAdd(mop, mtrc, (airMopper)pullTraceMultiNix, airMopAlways);

  nplot = nrrdNew();
  airMopAdd(mop, nplot, (airMopper)nrrdNuke, airMopAlways);
  nplotA = nrrdNew();
  airMopAdd(mop, nplotA, (airMopper)nrrdNuke, airMopAlways);
  nplotB = nrrdNew();
  airMopAdd(mop, nplotB, (airMopper)nrrdNuke, airMopAlways);
  nfilt = nrrdNew();
  airMopAdd(mop, nfilt, (airMopper)nrrdNuke, airMopAlways);

  hparm->respFileEnable = AIR_TRUE;
  me = argv[0];

  /* these don't need to be visible on the command-line */
  interType = pullInterTypeUnivariate;
  enspR = pullEnergySpecNew();
  airMopAdd(mop, enspR, (airMopper)pullEnergySpecNix, airMopAlways);
  pullEnergySpecParse(enspR, "cotan");
  enspS = pullEnergySpecNew();
  airMopAdd(mop, enspS, (airMopper)pullEnergySpecNix, airMopAlways);
  pullEnergySpecParse(enspS, "zero");
  enspWin = pullEnergySpecNew();
  airMopAdd(mop, enspWin, (airMopper)pullEnergySpecNix, airMopAlways);
  pullEnergySpecParse(enspWin, "butter:16,0.8");

  hestOptAdd(&hopt, "zz", "bool", airTypeBool, 1, 1, &zeroZ, "false",
             "always constrain Z=0, to process 2D images");
  hestOptAdd(&hopt, "su", "bool", airTypeBool, 1, 1, &strnUse, "false",
             "weigh contributions to traces with strength");
  hestOptAdd(&hopt, "nave", "bool", airTypeBool, 1, 1, &nixAtVolumeEdgeSpace, "false",
             "whether or not to nix points at edge of volume, where gage had "
             "to invent values for kernel support");
  hestOptAdd(&hopt, "cbst", "bool", airTypeBool, 1, 1, &constraintBeforeSeedThresh,
             "false",
             "during initialization, try constraint satisfaction before "
             "testing seedThresh");
  hestOptAdd(&hopt, "noadd", NULL, airTypeBool, 0, 0, &noAdd, NULL,
             "turn off adding during population control");
  hestOptAdd(&hopt, "usa", "bool", airTypeBool, 1, 1, &unequalShapesAllow, "false",
             "allow volumes to have different shapes (false is safe as "
             "different volume sizes are often accidental)");
  hestOptAdd(&hopt, "nobin", NULL, airTypeBool, 0, 0, &binSingle, NULL,
             "turn off spatial binning (which prevents multi-threading "
             "from being useful), for debugging or speed-up measurement");
  hestOptAdd(&hopt, "lti", "bool", airTypeBool, 1, 1, &liveThresholdOnInit, "true",
             "impose liveThresh on initialization");
  hestOptAdd(&hopt, "por", "bool", airTypeBool, 1, 1, &permuteOnRebin, "true",
             "permute points during rebinning");
  hestOptAdd(&hopt, "v", "verbosity", airTypeInt, 1, 1, &verbose, "1",
             "verbosity level");
  hestOptAdd(&hopt, "vol", "vol0 vol1", airTypeOther, 1, -1, &vspec, NULL,
             "input volumes, in format <filename>:<kind>:<volname>", &vspecNum, NULL,
             meetHestPullVol);
  hestOptAdd(&hopt, "info", "info0 info1", airTypeOther, 1, -1, &idef, NULL,
             "info definitions, in format "
             "<info>[-c]:<volname>:<item>[:<zero>:<scale>]",
             &idefNum, NULL, meetHestPullInfo);

  hestOptAdd(&hopt, "k00", "kern00", airTypeOther, 1, 1, &k00, "cubic:1,0",
             "kernel for gageKernel00", NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k11", "kern11", airTypeOther, 1, 1, &k11, "cubicd:1,0",
             "kernel for gageKernel11", NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k22", "kern22", airTypeOther, 1, 1, &k22, "cubicdd:1,0",
             "kernel for gageKernel22", NULL, NULL, nrrdHestKernelSpec);

  hestOptAdd(&hopt, "sscp", "path", airTypeString, 1, 1, &cachePathSS, "./",
             "path (without trailing /) for where to read/write "
             "pre-blurred volumes for scale-space");
  kssOpi = hestOptAdd(&hopt, "kssb", "kernel", airTypeOther, 1, 1, &kSSblur, "ds:1,5",
                      "default blurring kernel, to sample scale space", NULL, NULL,
                      nrrdHestKernelSpec);
  bspOpi = hestOptAdd(&hopt, "bsp", "boundary", airTypeOther, 1, 1, &bspec, "wrap",
                      "default boundary behavior of scale-space blurring", NULL, NULL,
                      nrrdHestBoundarySpec);
  hestOptAdd(&hopt, "kssr", "kernel", airTypeOther, 1, 1, &kSSrecon, "hermite",
             "kernel for reconstructing from scale space samples", NULL, NULL,
             nrrdHestKernelSpec);
  hestOptAdd(&hopt, "nss", "# scl smpls", airTypeUInt, 1, 1, &samplesAlongScaleNum, "1",
             "if using \"-ppv\", number of samples along scale axis "
             "for each spatial position");

  hestOptAdd(&hopt, "np", "# points", airTypeUInt, 1, 1, &pointNumInitial, "1000",
             "number of points to initialize with");
  hestOptAdd(&hopt, "tnp", "# points", airTypeUInt, 1, 1, &tracePointNum, "1000",
             "number of points to add in each iteration of "
             "estimation of plot");
  hestOptAdd(&hopt, "pnm", "# passes", airTypeUInt, 1, 1, &passNumMax, "10",
             "max number of passes in plot estimation");
  hestOptAdd(&hopt, "tpdt", "thresh", airTypeDouble, 1, 1, &tpdThresh, "1.0",
             "KL-distance threshold");
  hestOptAdd(&hopt, "ti", "fname", airTypeString, 1, 1, &tracesInS, "",
             "input file of *pre-computed* traces");
  hestOptAdd(&hopt, "to", "fname", airTypeString, 1, 1, &tracesOutS, "",
             "file for saving out *computed* traces");

  hestOptAdd(&hopt, "ppv", "# pnts/vox", airTypeUInt, 1, 1, &pointPerVoxel, "0",
             "number of points per voxel to start in simulation "
             "(need to have a seed thresh vol, overrides \"-np\")");
  hestOptAdd(&hopt, "ppvzr", "z range", airTypeUInt, 2, 2, ppvZRange, "1 0",
             "range of Z slices (1st num < 2nd num) to do ppv in, or, "
             "\"1 0\" for whole volume");
  hestOptAdd(&hopt, "jit", "jitter", airTypeDouble, 1, 1, &jitter, "0",
             "amount of jittering to do with ppv");
  hestOptAdd(&hopt, "pi", "npos", airTypeOther, 1, 1, &nPosIn, "",
             "4-by-N array of positions to start at (overrides \"-np\")", NULL, NULL,
             nrrdHestNrrd);
  hestOptAdd(&hopt, "step", "step", airTypeDouble, 1, 1, &stepInitial, "1",
             "initial step size for gradient descent");
  hestOptAdd(&hopt, "csm", "step", airTypeDouble, 1, 1, &constraintStepMin, "0.0001",
             "convergence criterion for constraint satisfaction");
  hestOptAdd(&hopt, "snap", "# iters", airTypeUInt, 1, 1, &snap, "0",
             "if non-zero, # iters between saved snapshots");
  hestOptAdd(&hopt, "stim", "# iters", airTypeUInt, 1, 1, &stuckIterMax, "5",
             "if non-zero, max # iterations to allow a particle "
             " to be stuck before nixing");
  hestOptAdd(&hopt, "maxci", "# iters", airTypeUInt, 1, 1, &constraintIterMax, "15",
             "if non-zero, max # iterations for contraint enforcement");
  hestOptAdd(&hopt, "irad", "scale", airTypeDouble, 1, 1, &radiusSpace, "1",
             "particle radius in spatial domain");
  hestOptAdd(&hopt, "srad", "scale", airTypeDouble, 1, 1, &radiusScale, "1",
             "particle radius in scale domain");
  hestOptAdd(&hopt, "bws", "bin width", airTypeDouble, 1, 1, &binWidthSpace, "1.001",
             "spatial bin width as multiple of spatial radius");
  hestOptAdd(&hopt, "ess", "scl", airTypeDouble, 1, 1, &backStepScale, "0.5",
             "when energy goes up instead of down, scale step "
             "size by this");
  hestOptAdd(&hopt, "oss", "scl", airTypeDouble, 1, 1, &opporStepScale, "1.0",
             "opportunistic scaling (hopefully up, >1) of step size "
             "on every iteration");
  hestOptAdd(&hopt, "edmin", "frac", airTypeDouble, 1, 1, &energyDecreaseMin, "0.0001",
             "convergence threshold: stop when fractional improvement "
             "(decrease) in energy dips below this");

  hestOptAdd(&hopt, "ac3c", "ac3c", airTypeBool, 1, 1, &allowCodimension3Constraints,
             "false", "allow codimensions 3 constraints");
  hestOptAdd(&hopt, "sit", "sit", airTypeBool, 1, 1, &scaleIsTau, "false",
             "scale is tau");
  hestOptAdd(&hopt, "rng", "seed", airTypeUInt, 1, 1, &rngSeed, "42",
             "base seed value for RNGs (and as a hack, start index for "
             "Halton-based sampling)");
  hestOptAdd(&hopt, "pbm", "mod", airTypeUInt, 1, 1, &progressBinMod, "50",
             "progress bin mod");
  hestOptAdd(&hopt, "nt", "# threads", airTypeInt, 1, 1, &threadNum, "1",
             (airThreadCapable
                ? "number of threads to use"
                : "IF threads had beeen enabled in this Teem build (but "
                  "they are NOT), this is how you would control the number "
                  "of threads to use"));
  hestOptAdd(&hopt, "addlog", "fname", airTypeString, 1, 1, &addLogS, "",
             "name of file in which to log all particle additions");
  hestOptAdd(&hopt, "po", "nout", airTypeString, 1, 1, &posOutS, "",
             "if a filename is given here, the positions of the initialized "
             "particles are saved");
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "trace.nrrd",
             "plotted trace image");
  hestOptAdd(&hopt, "eob", "base", airTypeString, 1, 1, &extraOutBaseS, "",
             "save extra info (besides position), and use this string as "
             "the base of the filenames.  Not using this means the extra "
             "info is not saved.");

  hestOptAdd(&hopt, "ss", "sstep", airTypeDouble, 1, 1, &sstep, "0.0003",
             "fraction of SS range used as step along scale for tracing");
  hestOptAdd(&hopt, "sw", "swin", airTypeDouble, 1, 1, &sswin, "0.1",
             "fraction of SS range that caps length of trace along scale");
  hestOptAdd(&hopt, "otl", "len", airTypeDouble, 1, 1, &orientTestLen, "0",
             "when probing the tangents of the contraint surface, this is "
             "(when nonzero) the length in world-space of "
             "the offset to the second test position at which to sample "
             "the constraint");
  hestOptAdd(&hopt, "pr", "sx sy", airTypeUInt, 2, 2, pres, "1000 420",
             "resolution of the 2D plot");
  hestOptAdd(&hopt, "sp", "smoothing", airTypeInt, 1, 1, &smoothPlot, "0",
             "if greater than zero, amount of smoothing of computed "
             "stabilities, prior to plotting");
  hestOptAdd(&hopt, "fw", "flatwght", airTypeInt, 1, 1, &flatWght, "0",
             "if greater than zero, amount by which to increase "
             "weighting of more horizontal (flat) sections of the plot");
  hestOptAdd(&hopt, "tlo", "fname", airTypeString, 1, 1, &trcListOutS, "",
             "output filename of list of all points in all traces");
  hestOptAdd(&hopt, "tmi", "nmask", airTypeOther, 1, 1, &nTraceMaskIn, "",
             "if this is an 8-bit 2D array, of the same size as given by "
             "the \"-pr\" option, then it is used as a mask on the "
             "pre-computed trace given by \"-ti\", to find points in traces "
             "that overlap with the trace mask, and save them in the file "
             "given by \"-tmpo\"",
             NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "tmpo", "nout", airTypeString, 1, 1, &traceMaskPosOutS, "",
             "if a filename is given here, this is where to save positions "
             "of positions on traces masked by \"-tmi\"");
  hestOptAdd(&hopt, "tvo", "fname", airTypeString, 1, 1, &trcVolOutS, "",
             "output filename for rasterized trace of scale-space volume");

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
      fprintf(stderr, "%s: couldn't open %s for writing\n", me, addLogS);
      airMopError(mop);
      return 1;
    }
    airMopAdd(mop, addLog, (airMopper)airFclose, airMopAlways);
  } else {
    addLog = NULL;
  }

  if (nTraceMaskIn && airStrlen(traceMaskPosOutS)) {
    nmaskedpos = nrrdNew();
    airMopAdd(mop, nmaskedpos, (airMopper)nrrdNuke, airMopAlways);
  } else {
    nmaskedpos = NULL;
  }

  pctx = pullContextNew();
  airMopAdd(mop, pctx, (airMopper)pullContextNix, airMopAlways);
  if (pullVerboseSet(pctx, verbose) || pullFlagSet(pctx, pullFlagZeroZ, zeroZ)
      || pullFlagSet(pctx, pullFlagNixAtVolumeEdgeSpace, nixAtVolumeEdgeSpace)
      || pullFlagSet(pctx, pullFlagConstraintBeforeSeedThresh,
                     constraintBeforeSeedThresh)
      || pullFlagSet(pctx, pullFlagBinSingle, binSingle)
      || pullFlagSet(pctx, pullFlagNoAdd, noAdd)
      || pullFlagSet(pctx, pullFlagPermuteOnRebin, permuteOnRebin)
      /* want this to be true; tracing is different than regular particles */
      || pullFlagSet(pctx, pullFlagRestrictiveAddToBins, AIR_TRUE)
      || pullFlagSet(pctx, pullFlagAllowCodimension3Constraints,
                     allowCodimension3Constraints)
      || pullFlagSet(pctx, pullFlagScaleIsTau, scaleIsTau)
      || pullInitUnequalShapesAllowSet(pctx, unequalShapesAllow)
      || pullIterParmSet(pctx, pullIterParmSnap, snap)
      || pullIterParmSet(pctx, pullIterParmStuckMax, stuckIterMax)
      || pullIterParmSet(pctx, pullIterParmConstraintMax, constraintIterMax)
      || pullSysParmSet(pctx, pullSysParmStepInitial, stepInitial)
      || pullSysParmSet(pctx, pullSysParmConstraintStepMin, constraintStepMin)
      || pullSysParmSet(pctx, pullSysParmRadiusSpace, radiusSpace)
      || pullSysParmSet(pctx, pullSysParmRadiusScale, radiusScale)
      || pullSysParmSet(pctx, pullSysParmBinWidthSpace, binWidthSpace)
      || pullSysParmSet(pctx, pullSysParmEnergyDecreaseMin, energyDecreaseMin)
      || pullSysParmSet(pctx, pullSysParmBackStepScale, backStepScale)
      || pullSysParmSet(pctx, pullSysParmOpporStepScale, opporStepScale)
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

  if (pullInitHaltonSet(pctx, pointNumInitial, rngSeed)) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble setting halton:\n%s", me, err);
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

  if (airStrlen(tracesInS)) {
    /* don't need to initialize points if we're reading a trace,
       but annoyingly we do need the rest of the pull set up,
       *JUST* so that we can read off the scale-space range
       associated with the constraint */
    pullFlagSet(pctx, pullFlagStartSkipsPoints, AIR_TRUE);
  }
  if (pctx->verbose) {
    fprintf(stderr, "%s: about to pullStart\n", me);
  }
  if (pullStart(pctx)) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble starting system:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  if (nrrdMaybeAlloc_va(nplotA, nrrdTypeDouble, 2, AIR_SIZE_T(pres[0]),
                        AIR_SIZE_T(pres[1]))) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble creating output:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (pullConstraintScaleRange(pctx, ssrange)) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble C:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  fprintf(stderr, "!%s: ================== ssrange %g %g\n", me, ssrange[0], ssrange[1]);
  if (vspec[0]->sbp) {
    char stmp[AIR_STRLEN_LARGE + 1];
    gageStackBlurParmSprint(stmp, vspec[0]->sbp, NULL, NULL);
    fprintf(stderr, "!%s: ======== %s\n", me, stmp);
  }
  nplotA->axis[0].min = ssrange[0];
  nplotA->axis[0].max = ssrange[1];
  nplotA->axis[1].min = 1.0;
  nplotA->axis[1].max = 0.0;
  nrrdCopy(nplotB, nplotA);
  if (airStrlen(tracesInS)) {
    if (!(tracesFile = airFopen(tracesInS, stdin, "rb"))) {
      fprintf(stderr, "%s: couldn't open %s for reading\n", me, tracesInS);
      airMopError(mop);
      return 1;
    }
    if (pullTraceMultiRead(mtrc, tracesFile)) {
      airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble reading:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    airFclose(tracesFile);
    goto plotting;
  }
  /* else not reading in traces, have to compute them */

  if (!pctx->constraint) {
    fprintf(stderr, "%s: this programs requires a constraint\n", me);
    airMopError(mop);
    return 1;
  }
  scaleVol = NULL;
  {
    unsigned int ii;
    for (ii = 0; ii < pctx->volNum; ii++) {
      if (pctx->vol[ii]->ninScale) {
        scaleVol = pctx->vol[ii];
        break;
      }
    }
  }
  if (!scaleVol) {
    fprintf(stderr, "%s: this program requires scale-space\n", me);
    airMopError(mop);
    return 1;
  }

  if (pullOutputGet(nPosOut, NULL, NULL, NULL, 0.0, pctx)) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble D:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (airStrlen(posOutS)) {
    nrrdSave(posOutS, nPosOut, NULL);
  }

  {
    double *pos, seedPos[4], scaleWin, scaleStep, dist = 0;
    unsigned int pidx, pnum, passIdx;
    pullTrace *pts;
    Nrrd *nsplot, *nprogA, *nprogB, *nlsplot;
    char doneStr[AIR_STRLEN_SMALL + 1];

    pos = AIR_CAST(double *, nPosOut->data);
    nlsplot = nrrdNew();
    airMopAdd(mop, nlsplot, (airMopper)nrrdNuke, airMopAlways);
    nsplot = nrrdNew();
    airMopAdd(mop, nsplot, (airMopper)nrrdNuke, airMopAlways);
    nprogA = nrrdNew();
    airMopAdd(mop, nprogA, (airMopper)nrrdNuke, airMopAlways);
    nprogB = nrrdNew();
    airMopAdd(mop, nprogB, (airMopper)nrrdNuke, airMopAlways);
    if (nrrdCopy(nprogA, nplotA) || nrrdCopy(nprogB, nplotA)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble creating nprogs:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    scaleWin = sswin * (ssrange[1] - ssrange[0]);
    scaleStep = sstep * (ssrange[1] - ssrange[0]);

    pnum = nPosOut->axis[1].size;
    printf("!%s: tracing initial %u points . . .       ", me, pnum);
    for (pidx = 0; pidx < pnum; pidx++) {
      int added;
      printf("%s", airDoneStr(0, pidx, pnum, doneStr));
      fflush(stdout);
      pts = pullTraceNew();
      ELL_4V_COPY(seedPos, pos + 4 * pidx);
      if (pullTraceSet(pctx, pts, AIR_TRUE /* recordStrength */, scaleStep, scaleWin / 2,
                       orientTestLen, AIR_UINT((scaleWin / 2) / scaleStep), seedPos)
          || pullTraceMultiAdd(mtrc, pts, &added)) {
        airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble on point %u:\n%s", me, pidx, err);
        airMopError(mop);
        return 1;
      }
      if (!added) {
        /*
        fprintf(stderr, "!%s: whynowhere %s, why stop %s(%s)  %s(%s)\n", me,
                airEnumStr(pullTraceStop, pts->whyNowhere),
                airEnumStr(pullTraceStop, pts->whyStop[0]),
                (pts->whyStop[0] == pullTraceStopConstrFail
                 ? airEnumStr(pullConstraintFail, pts->whyConstrFail[0])
                 : ""),
                airEnumStr(pullTraceStop, pts->whyStop[1]),
                (pts->whyStop[1] == pullTraceStopConstrFail
                 ? airEnumStr(pullConstraintFail, pts->whyConstrFail[1])
                 : ""));
        */
        pts = pullTraceNix(pts);
      }
    }
    printf("%s\n", airDoneStr(0, pidx, pnum, doneStr));
    if (!mtrc->traceNum) {
      fprintf(stderr, "%s: %u initial points led to zero traces\n", me, pnum);
      airMopError(mop);
      return 1;
    }
    if (pullTraceMultiPlotAdd(nprogA, mtrc, NULL, strnUse, smoothPlot, flatWght, 0, 0,
                              NULL, NULL)) {
      airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble PlotAdd'ing (B):\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    resamplePlot(nlsplot, nprogA);
    /* savePlot(nlsplot); */

    for (passIdx = 0; passIdx < passNumMax; passIdx++) {
      double dd;
      fprintf(stderr, "!%s: pass %u/%u ==================\n", me, passIdx, passNumMax);
      nrrdZeroSet(nprogB);
      if (findAndTraceMorePoints(nprogB, pctx, scaleVol, strnUse, smoothPlot, flatWght,
                                 scaleStep, scaleWin / 2, orientTestLen,
                                 AIR_UINT((scaleWin / 2) / scaleStep), mtrc,
                                 tracePointNum)
          || resamplePlot(nsplot, nprogB)) {
        airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble on pass %u:\n%s", me, passIdx, err);
        airMopError(mop);
        return 1;
      }
      /* savePlot(nsplot); */
      dd = distanceProb(nsplot, nlsplot) / tracePointNum;
      if (!passIdx) {
        dist = dd;
      } else {
        dist = AIR_LERP(0.9, dist, dd);
      }
      fprintf(stderr, "%s: dd = %g -> dist = %g (%s %g)\n", me, dd, dist,
              dist < tpdThresh ? "<" : ">=", tpdThresh);
      nrrdCopy(nlsplot, nsplot);
      if (dist < tpdThresh) {
        fprintf(stderr, "%s: converged: dist %g < thresh %g\n", me, dist, tpdThresh);
        break;
      }
    }
    if (dist >= tpdThresh) {
      fprintf(stderr, "%s: WARNING did NOT converge: dist %g >= thresh %g\n", me, dist,
              tpdThresh);
    }
    if (airStrlen(tracesOutS) && !airStrlen(tracesInS)) {
      tracesFile = airFopen(tracesOutS, stdout, "wb");
      if (pullTraceMultiWrite(tracesFile, mtrc)) {
        airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble writing:\n%s", me, err);
        airMopError(mop);
        return 1;
      }
      fclose(tracesFile);
    }
  plotting:
    if (airStrlen(trcListOutS)) {
      /* format:
      ** trcIdx  isSeed  X  Y  Z  S  f(stab)  strn qual
      **   0        1    2  3  4  5    6       7     8  (9)
      */
      Nrrd *ntlo;
      double *tlo;
      size_t sx = 9, totn = 0, toti = 0;
      pullTrace *trc;
      pullPoint *lpnt;
      unsigned int ti;
      for (ti = 0; ti < mtrc->traceNum; ti++) {
        trc = mtrc->trace[ti];
        fprintf(stderr, "!%s: ti %u/%u : stab (%d-D) %u %u\n", me, ti, mtrc->traceNum,
                trc->nstab->dim, AIR_UINT(trc->nstab->axis[0].size),
                AIR_UINT(trc->nstab->axis[1].size));
        totn += trc->nstab->axis[0].size;
      }
      ntlo = nrrdNew();
      lpnt = pullPointNew(pctx);
      if (nrrdMaybeAlloc_va(ntlo, nrrdTypeDouble, 2, sx, totn)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: couldn't alloc output:\n%s", me, err);
        airMopError(mop);
        return 1;
      }
      airMopAdd(mop, ntlo, (airMopper)nrrdNuke, airMopAlways);
      tlo = AIR_CAST(double *, ntlo->data);
      for (ti = 0; ti < mtrc->traceNum; ti++) {
        unsigned int vi, vn;
        double *vert, *stab, *strn, qual;
        trc = mtrc->trace[ti];
        vn = AIR_UINT(trc->nstab->axis[0].size);
        vert = AIR_CAST(double *, trc->nvert->data);
        stab = AIR_CAST(double *, trc->nstab->data);
        strn = AIR_CAST(double *, (trc->nstrn ? trc->nstrn->data : NULL));
        for (vi = 0; vi < vn; vi++) {
          tlo[sx * toti + 0] = AIR_CAST(double, ti);
          tlo[sx * toti + 1] = (vi == trc->seedIdx);
          ELL_4V_COPY(tlo + sx * toti + 2, vert + 4 * vi);
          ELL_4V_COPY(lpnt->pos, vert + 4 * vi);
          if (pctx->ispec[pullInfoQuality]) {
            pullProbe(pctx->task[0], lpnt);
            qual = pullPointScalar(pctx, lpnt, pullInfoQuality, NULL, NULL);
          } else {
            qual = 0.0;
          }
          tlo[sx * toti + 6] = stab[vi];
          tlo[sx * toti + 7] = strn ? strn[vi] : 0.0;
          tlo[sx * toti + 8] = qual;
          toti++;
        }
      }
      if (nrrdSave(trcListOutS, ntlo, NULL)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: couldn't save output:\n%s", me, err);
        airMopError(mop);
        return 1;
      }
    }
    if (airStrlen(trcVolOutS)) {
      /* HEY: copy and paste from above */
      Nrrd *nout;
      const gagePoint *pnt;
      meetPullVol *mpv;
      pullTrace *trc;
      pullPoint *lpnt;
      unsigned int size[4], idx[4], iii, ti, si;
      double idxd[4], val, (*lup)(const void *v, size_t I),
        (*ins)(void *v, size_t I, double d);
      /* HEY this will segfault around here if the mask volume is
         first, because of the following line of code */
      if (!(mpv = meetPullVolCopy(vspec[0]))) {
        airMopAdd(mop, err = biffGetDone(MEET), airFree, airMopAlways);
        fprintf(stderr, "%s: couldn't copy volume:\n%s", me, err);
        airMopError(mop);
        return 1;
      }
      airMopAdd(mop, mpv, (airMopper)meetPullVolNix, airMopAlways);
      /* at this point we actually have to hijack the mpv in case its
         a non-scalar kind, because all we want is a scalar output */
      if (0 != mpv->kind->baseDim) {
        unsigned int ni;
        Nrrd *nslice;
        nslice = nrrdNew();
        airMopAdd(mop, nslice, (airMopper)nrrdNuke, airMopAlways);
        fprintf(stderr, "!%s: slicing %u %s vols to %s\n", me, mpv->sbp->num,
                mpv->kind->name, gageKindScl->name);
        for (ni = 0; ni < mpv->sbp->num; ni++) {
          if (nrrdSlice(nslice, mpv->ninSS[ni], 0, 0)
              || nrrdCopy(mpv->ninSS[ni], nslice)) {
            airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
            fprintf(stderr, "%s: couldn't slice volume %u:\n%s", me, ni, err);
            airMopError(mop);
            return 1;
          }
        }
        /* can we just do this? */
        mpv->kind = gageKindScl;
      }
      size[0] = AIR_UINT(mpv->ninSS[0]->axis[0].size);
      size[1] = AIR_UINT(mpv->ninSS[0]->axis[1].size);
      size[2] = AIR_UINT(mpv->ninSS[0]->axis[2].size);
      size[3] = mpv->sbp->num;
      printf("!%s: size = (%u,%u,%u,%u)\n", me, size[0], size[1], size[2], size[3]);
      lpnt = pullPointNew(pctx);
      airMopAdd(mop, lpnt, (airMopper)pullPointNix, airMopAlways);
      for (si = 0; si < size[3]; si++) {
        nrrdZeroSet(mpv->ninSS[si]);
      }
      lup = nrrdDLookup[mpv->ninSS[0]->type];
      ins = nrrdDInsert[mpv->ninSS[0]->type];
      /* HEY here also assuming scale-space volume is first volume  */
      pnt = &(pctx->task[0]->vol[0]->gctx->point);
      for (ti = 0; ti < mtrc->traceNum; ti++) {
        unsigned int vi, vn, orn, ori;
        double *vert, *stab, *strn, *orin, wght;
        trc = mtrc->trace[ti];
        vn = AIR_UINT(trc->nstab->axis[0].size);
        vert = AIR_CAST(double *, trc->nvert->data);
        stab = AIR_CAST(double *, trc->nstab->data);
        orin = AIR_CAST(double *, trc->norin->data);
        strn = AIR_CAST(double *, (strnUse && trc->nstrn ? trc->nstrn->data : NULL));
        /* orn = (orin ? 20 : 1); */
        orn = 1;
        for (vi = 0; vi < vn; vi++) {
          for (ori = 0; ori < orn; ori++) {
            ELL_4V_COPY(lpnt->pos, vert + 4 * vi);
            if (orn > 1) {
              /* hack to draw a dotted line along tangent */
              double oscl = AIR_AFFINE(0, ori, orn - 1, -100, 100);
              lpnt->pos[0] += oscl * (orin + 3 * vi)[0];
              lpnt->pos[1] += oscl * (orin + 3 * vi)[1];
            }
            if (zeroZ && lpnt->pos[2] != 0) {
              fprintf(stderr, "%s: zeroZ violated\n", me);
              airMopError(mop);
              return 1;
            }
            wght = stab[vi];
            if (strn) {
              wght *= strn[vi];
            }
            /* probe just to get the transform to idx-space from gage */
            if (pullProbe(pctx->task[0], lpnt)) {
              airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
              fprintf(stderr, "%s: couldn't probe:\n%s", me, err);
              airMopError(mop);
              return 1;
            }
            ELL_4V_ADD2(idxd, pnt->frac, pnt->idx);
            /* because of gage subtlety that gagePoint->idx is index
               of upper, not lower, corner, idxd is too big by 1 */
            idx[0] = airIndexClamp(-0.5, idxd[0] - 1, size[0] - 0.5, size[0]);
            idx[1] = airIndexClamp(-0.5, idxd[1] - 1, size[1] - 0.5, size[1]);
            idx[2] = airIndexClamp(-0.5, idxd[2] - 1, size[2] - 0.5, size[2]);
            idx[3] = airIndexClamp(0, idxd[3], size[3] - 1, size[3]);
            iii = idx[0] + size[0] * (idx[1] + size[1] * idx[2]);
            val = lup(mpv->ninSS[idx[3]]->data, iii);
            ins(mpv->ninSS[idx[3]]->data, iii, wght + val);
            /*
              printf("!%s: (%g,%g,%g,%g) -> (%g,%g,%g,%g) -> (%u,%u,%u,%u) -> %u: %g\n",
              me, lpnt->pos[0], lpnt->pos[1], lpnt->pos[2], lpnt->pos[3],
              idxd[0], idxd[1], idxd[2], idxd[3],
              idx[0], idx[1], idx[2], idx[3], iii, val);
            */
          }
        }
      }
      nout = nrrdNew();
      airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
      if (nrrdJoin(nout, AIR_CAST(const Nrrd *const *, mpv->ninSS), size[3], 3,
                   AIR_FALSE)
          || nrrdSave(trcVolOutS, nout, NULL)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: couldn't join or save SS output:\n%s", me, err);
        airMopError(mop);
        return 1;
      }
    }
    if (pullTraceMultiPlotAdd(nplotA, mtrc, NULL, strnUse, smoothPlot, flatWght, 0, 0,
                              nmaskedpos, nTraceMaskIn)
        /* || pullTraceMultiFilterConcaveDown(nfilt, mtrc, 0.05) */
        /* The filter concave down idea didn't really work */
        || pullTraceMultiPlotAdd(nplotB, mtrc, NULL /* nfilt */, AIR_TRUE, smoothPlot,
                                 flatWght, 0, 0, NULL, NULL)) {
      airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble PlotAdd'ing (C):\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    {
      const Nrrd *nin[2];
      nin[0] = nplotA;
      nin[1] = nplotB;
      if (nrrdJoin(nplot, nin, 2, 0, AIR_TRUE)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble E:\n%s", me, err);
        airMopError(mop);
        return 1;
      }
    }
    if (nrrdSave(outS, nplot, NULL)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble F:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    if (nmaskedpos && nmaskedpos->data && airStrlen(traceMaskPosOutS)) {
      if (nrrdSave(traceMaskPosOutS, nmaskedpos, NULL)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble G:\n%s", me, err);
        airMopError(mop);
        return 1;
      }
    }
  }

  pullFinish(pctx);
  airMopOkay(mop);
  return ret;
}
