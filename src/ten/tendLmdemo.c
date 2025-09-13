/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2025  University of Chicago
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

#include <assert.h>

#include "ten.h"
#include "privateTen.h"
#if TEEM_LEVMAR
#  include <levmar.h>
#endif

#if TEEM_LEVMAR
#  define INFO "Demonstrates using levmar "
static const char *_tend_lmdemoInfoL
  = (INFO " https://users.ics.forth.gr/~lourakis/levmar/ implementation of "
          "Levenberg-Marquardt (LM), with an example of fitting a polynomial "
          "to noisy data.  Also demonstrates the new airJSFRand way of generating "
          "pseudo-random numbers.");
// AND, this is the first file in Teem that tries using C99 features,
// like these C++-style comments, and variables declared as you go, and __func__
// which makes writing C a little less annoying
#else
#  define INFO "(no LEVMAR => cannot run) Demo of levmar"
static const char *_tend_lmdemoInfoL
  = (INFO ". Because this Teem was built withOUT the "
          "https://users.ics.forth.gr/~lourakis/levmar/ implementation of "
          "Levenberg-Marquardt (LM), this demo does not do anything useful. Try "
          "CMake-configuring with Teem_USE_LEVMAR, or "
          "GNUmake-ing with environment variable TEEM_LEVMAR.");
#endif

/* excerpts from levmar.h:

-- double precision LM, with & without Jacobian --
-- unconstrained minimization --
extern int dlevmar_der(
      void (*func)(double *p, double *hx, int m, int n, void *adata),
      void (*jacf)(double *p, double *j, int m, int n, void *adata),
      double *p, double *x, int m, int n, int itmax, double *opts,
      double *info, double *work, double *covar, void *adata);

extern int dlevmar_dif(
      void (*func)(double *p, double *hx, int m, int n, void *adata),
      double *p, double *x, int m, int n, int itmax, double *opts,
      double *info, double *work, double *covar, void *adata);

-- box-constrained minimization --
extern int dlevmar_bc_der(
       void (*func)(double *p, double *hx, int m, int n, void *adata),
       void (*jacf)(double *p, double *j, int m, int n, void *adata),
       double *p, double *x, int m, int n, double *lb, double *ub, double *dscl,
       int itmax, double *opts, double *info, double *work, double *covar, void *adata);

extern int dlevmar_bc_dif(
       void (*func)(double *p, double *hx, int m, int n, void *adata),
       double *p, double *x, int m, int n, double *lb, double *ub, double *dscl,
       int itmax, double *opts, double *info, double *work, double *covar, void *adata);
*/

static double
polyEval(const double *parmCurr, unsigned int M, double xx) {
  double ret = 0;
  for (unsigned int ci = M; ci-- > 0;) { // ci = coefficient index, with "-->" operator
    ret = ret * xx + parmCurr[ci];
  }
  return ret;
}

// the bag-of-state struct passed via "adata" arg for additional data
typedef struct {
  double *tp;       // ground truth polynomial coefficients (should really be const)
  unsigned int M,   // # parameters to estimate = length of tp[]
    N;              // # of samples in interval
  double xmm[2];    // interval in which we do N cell-centered samples
  int verb;         // verbosity
  double *parmCurr; // the polynomial as currently being fitted
} bagOstate;

/*
funcPredict: the "func" callback for levmar functions
Forward modeling or prediction callback for both dlevmar_der and dlevmar_dif, which
the https://users.ics.forth.gr/~lourakis/levmar/ docs describe as:
   functional relation describing measurements.
   A pc[] \in R^mm yields a \hat{y} \in  R^nn
except that we've done some renaming:
   "p"  (parameter vector) --> "pc" = parmCurr = current polynomial we're trying
   "m"  (# parameters)     --> "mm" to be less annoying
   "hx" (predicted data)   --> "hy" predicted y=pc(x) polynomial values
   "n"  (# data points)    --> "nn" to be less annoying
*/
static void
funcPredict(double *parmCurr, double *hy, int mm, int nn, void *adata) {
  bagOstate *bag = (bagOstate *)adata;
  if (bag->verb > 2) {
    printf("%s: called at parmCurr =", __func__);
    for (int pi = 0; pi < mm; pi++) {
      printf(" %g", parmCurr[pi]);
    }
    printf("\n");
  }
  assert(AIR_UINT(mm) == bag->M);
  assert(AIR_UINT(nn) == bag->N);
  for (int si = 0; si < nn; si++) { // si = sample index
    double sx = NRRD_CELL_POS(bag->xmm[0], bag->xmm[1], nn, si);
    hy[si] = polyEval(parmCurr, mm, sx);
  }
  return;
}

// Synthesize data and store in ndata
static void
dataSynth(Nrrd *ndata, const bagOstate *bag, double rnStdv, unsigned int rnSeed,
          airArray *mop) {
  double *data = AIR_CAST(double *, ndata->data);
  airJSFRand *jsf = NULL;
  if (rnStdv > 0) {
    jsf = airJSFRandNew(rnSeed);
    airMopAdd(mop, jsf, (airMopper)airJSFRandNix, airMopAlways);
  }
  unsigned int N = AIR_UINT(ndata->axis[0].size); // N = # of data points
  assert(N == bag->N);
  for (unsigned int si = 0; si < N; si++) { // si = sample index
    double sx = NRRD_CELL_POS(bag->xmm[0], bag->xmm[1], N, si);
    double val = polyEval(bag->tp, bag->M, sx);
    double noise = jsf ? rnStdv * airJSFRandNormal_d(jsf) : 0;
    /*
    if (bag->verb > 1) {
      printf("%s: data[%u] = poly(%g) + noise = %g + %g = %g\n", __func__, si, sx, val,
             noise, val + noise);
    }
    */
    data[si] = val + noise;
  }
  ndata->axis[0].min = bag->xmm[0];
  ndata->axis[0].max = bag->xmm[1];
  ndata->axis[0].center = nrrdCenterCell;
  return;
}

/*
To be instructive, this levmar wrapper function is trying to be general purpose;
the fact that funcPredict is evaluating a polynomial and that we're here to fit a
polynomial should not be exposed by this function.
*/
static void
doLM(Nrrd *ndata, int itmax, /* const */ double opts[5], bagOstate *bag) {
  int iters = -1;
  AIR_UNUSED(ndata);
  AIR_UNUSED(itmax);
  double info[LM_INFO_SZ];
  for (unsigned int ii = 0; ii < LM_INFO_SZ; ii++) {
    info[ii] = AIR_NAN;
  }
#if TEEM_LEVMAR
  double *ym = (double *)(ndata->data); // measured y values
  /* extern int dlevmar_dif(
      void (*func)(double *p, double *hx, int m, int n, void *adata),
      double *p, double *x, int m, int n, int itmax, double *opts,
      double *info, double *work, double *covar, void *adata); */
  iters = dlevmar_dif(funcPredict,                                                  //
                      bag->parmCurr, ym, (int)(bag->M), (int)(bag->N), itmax, opts, //
                      info, NULL, NULL, AIR_VOIDP(bag));
#else
  printf("%s: not calling dlevmar_dif() because we don't have TEEM_LEVMAR\n", __func__);
#endif
  printf("%s: after %d iters, ended at parmCurr =", __func__, iters);
  for (unsigned int pi = 0; pi < bag->M; pi++) {
    printf(" %g", bag->parmCurr[pi]);
  }
  printf("\n");
  if (bag->verb) {
    /*
    Summary of info from from https://users.ics.forth.gr/~lourakis/levmar/
    and https://users.ics.forth.gr/~lourakis/levmar/levmar.pdf
    e in R^n = epsilon vector = given data - predicted data
    p in R^m = parameter vector
    dp in R^m = delta p = last step taken in parameter space
    J nxm matrix = Jacobian; J_ij = d hx_i / d p_j
    mu in R = damping term added to diagonal of [J^T J] to normalize it
       (mu set to tau * max[J^T J]_ii)

    double opts[5] for controlling levmar functions:
    opts[0-4] =  [\tau, \epsilon1, \epsilon2, \epsilon3, \delta]. Respectively
    opts[0] = (\tau) the scale factor for initial \mu,
    opts[1] = (\epsilon1) stopping threshold for ||J^T e||_inf,
    opts[2] = (\epsilon2) stopping threshold for ||Dp||_2
    opts[3] = (\epsilon3) stopping threshold for ||e||_2
    opts[4] = (\delta) stopping thresh for step used in difference approximation of
    Jacobian. If \delta<0, the Jacobian is approximated with central differences which
    are more accurate (but slower!) compared to the forward differences employed by
    default. Set to opts=NULL for defaults to be used.
    */
    printf("info[0]: Error at initial p (||e||^2): %g\n", info[0]);
    printf("info[1]: Error at final p (||e||^2): %g\n", info[1]);
    printf("info[2]: Gradient norm at final p (||J^T e||_inf): %g\n", info[2]);
    printf("info[3]: Norm of last step (||dp||_2): %g\n", info[3]);
    printf("info[4]: Final damping scaling tau (mu/max[J^T J]_ii): %g\n", info[4]);
    printf("info[5]: # of iterations: %.0f\n", info[5]);
    printf("info[6]: Termination reason: ");
    switch ((int)(info[6] + 0.5)) { // round to nearest int
    case 1:
      printf("(1) stopped by small gradient J^T e\n");
      break;
    case 2:
      printf("(2) stopped by small dp\n");
      break;
    case 3:
      printf("(3) stopped by itmax %d\n", itmax);
      break;
    case 4:
      printf("(4) singular (augmented normal) matrix. Restart from current p with "
             "increased mu\n");
      break;
    case 5:
      printf("(5) no further error reduction is possible. Restart with increased mu\n");
      break;
    case 6:
      printf("(6) stopped by small ||e||_2\n");
      break;
    case 7:
      printf("(7) stopped by invalid (i.e. NaN or Inf) func values; a user error\n");
      break;
    default:
      printf("Unknown code %.0f\n", info[6]);
    }
    printf("info[7]: # of function evals: %.0f\n", info[7]);
    printf("info[8]: # of Jacobian evals: %.0f\n", info[8]);
    printf(
      "info[9]: # linear systems solved, i.e. # attempts for reducing error: %.0f\n",
      info[9]);
  }
  return;
}

static int
tend_lmdemoMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;

  bagOstate bag[1]; // allocate one bag-o-state; can access member m with bag->m
  hestOptAdd_Nv_Double(&hopt, "tp", "true poly coeffs", 1, -1, &(bag->tp), "1",
                       "coefficients of (ground-truth) polynomial to sample, to "
                       "synthsize the data to later fit. \"-tp A B C\" means "
                       "A + Bx + Cx^2.  These coefficients are the _M_ "
                       "parameters that the LM method seeks to recover",
                       &(bag->M));
  hestOptAdd_1_UInt(&hopt, "N", "# points", &(bag->N), "42",
                    "How many times to sample polynomial to generate data _N_ data "
                    "points.  Part of the purpose of this demo is to connect the "
                    "stupid single-letter variable names used in the levmar docs "
                    "to this concrete example.");
  hestOptAdd_2_Double(&hopt, "xmm", "xmin xmax", bag->xmm, "-1 1",
                      "polynomial will be evaluated at _N_ cell-centered points along "
                      "this interval on X axis");
  double rnStdv;
  hestOptAdd_1_Double(&hopt, "r", "noise", &rnStdv, "0",
                      "Gaussian noise of this stdv will be added to polynomial "
                      "evaluations to generate data");
  unsigned int rnSeed;
  hestOptAdd_1_UInt(&hopt, "e", "rng sEed", &rnSeed, "67",
                    "Seed for random number generator");
  char *outS;
  hestOptAdd_1_String(&hopt, "o", "data out", &outS, NULL,
                      "If given a filename here, will save out synthetic data");
  int itmax;
  hestOptAdd_1_Int(&hopt, "im", "itmax", &itmax, "100", "cap on # iterations");
  hestOptAdd_1_Int(&hopt, "v", "verbosity", &(bag->verb), "1", "verbosity level");

  airArray *mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_lmdemoInfoL);
  JUSTPARSE(_tend_lmdemoInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);
  if (!(bag->N >= 1)) {
    fprintf(stderr, "%s: Need at least N=1 datapoint, not N=%u\n", me, bag->N);
    airMopError(mop);
    return 1;
  }
  if (!(bag->N >= bag->M)) {
    printf("%s: WARNING: have only N=%u datapoints to recover M=%u parameters; "
           "  this may not end well.\n",
           me, bag->M, bag->N);
  }
  Nrrd *ndata = nrrdNew();
  airMopAdd(mop, ndata, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdMaybeAlloc_va(ndata, nrrdTypeDouble, 1, bag->N)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble allocating data:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }
  // synthesize the data and maybe save it out
  dataSynth(ndata, bag, rnStdv, rnSeed, mop);
  if (airStrlen(outS)) {
    NrrdIoState *nio = nrrdIoStateNew();
    airMopAdd(mop, nio, (airMopper)nrrdIoStateNix, airMopAlways);
    nio->bareText = AIR_FALSE;
    nio->moreThanFloatInText = AIR_TRUE;
    if (nrrdSave(outS, ndata, nio)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
      airMopError(mop);
      return 1;
    }
  }

  // prepare for LM call
  bag->parmCurr = AIR_MALLOC(bag->M, double);
  assert(bag->parmCurr);
  airMopAdd(mop, bag->parmCurr, airFree, airMopAlways);
  // initialize parms that will be fitted
  for (unsigned int pi = 0; pi < bag->M; pi++) {
    bag->parmCurr[pi] = 0;
  }
  AIR_UNUSED(funcPredict); // to quiet warnings if not levmar
  doLM(ndata, itmax, /* opts */ NULL, bag);

  airMopOkay(mop);
  return 0;
}
TEND_CMD(lmdemo, INFO);
