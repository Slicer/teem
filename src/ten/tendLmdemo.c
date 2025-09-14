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
Forward modeling or prediction, for all of dlevmar{_bc,}_{der,dif}, which
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
    printf("%s:  called at parmCurr =", __func__);
    for (int pi = 0; pi < mm; pi++) {
      printf(" %.17g", parmCurr[pi]);
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

/* Analytic Jacobian J[i,j] = d(predVal[i]) / d(parm[j])
 * e.g. polynomial p(x) = a + b*x + c*x*x => m = #parms = 3 => j=0..2
 *  => i'th row of J = [  1    xi   xi*xi  ]
 * levmar wants "row-major" => rows are contiguous in memory
 * => j is faster axis, and i is slower axis
 */
static void
funcJacobian(double *parmCurr, double *jac, int mm, int nn, void *adata) {
  bagOstate *bag = (bagOstate *)adata;
  if (bag->verb > 2) {
    printf("%s: called at parmCurr =", __func__);
    for (int pi = 0; pi < mm; pi++) {
      printf(" %.17g", parmCurr[pi]);
    }
    printf("\n");
  }
  assert(AIR_UINT(mm) == bag->M);
  assert(AIR_UINT(nn) == bag->N);
  for (int si = 0; si < nn; si++) { // si = sample index
    double sx = NRRD_CELL_POS(bag->xmm[0], bag->xmm[1], nn, si);
    double pxj = 1; // pow(x,j) = x^j but computed incrementally
    for (int jj = 0; jj < mm; jj++) {
      jac[si * mm + jj] = pxj; // jj faster, si slower
      /* If using this busted Jacobian: jac[jj * nn + si] = pxj; levmar still converges
      (at least it can converge due to small Dp) but with a huge residual.  There does
      NOT seem to be any internal levmar sanity check that the Jacobian callback is
      giving results consistent with the data prediction callback */
      pxj *= sx;
    }
  }
  return;
}

// Synthesize data and store in ndata
static void
dataSynth(Nrrd *ndata, const bagOstate *bag, const double rndStdvSeed[2],
          airArray *mop) {
  double *data = AIR_CAST(double *, ndata->data);
  airJSFRand *jsf = NULL;
  if (rndStdvSeed[0] > 0) {
    jsf = airJSFRandNew(AIR_UINT(rndStdvSeed[1]));
    airMopAdd(mop, jsf, (airMopper)airJSFRandNix, airMopAlways);
  }
  unsigned int N = AIR_UINT(ndata->axis[0].size); // N = # of data points
  assert(N == bag->N);
  for (unsigned int si = 0; si < N; si++) { // si = sample index
    double sx = NRRD_CELL_POS(bag->xmm[0], bag->xmm[1], N, si);
    double val = polyEval(bag->tp, bag->M, sx);
    double noise = jsf ? rndStdvSeed[0] * airJSFRandNormal_d(jsf) : 0;
    /*
    if (bag->verb > 1) {
      printf("%s: data[%u] = poly(%.17g) + noise = %.17g + %.17g = %.17g\n", __func__,
    si, sx, val, noise, val + noise);
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

Summary of info from from https://users.ics.forth.gr/~lourakis/levmar/
and https://users.ics.forth.gr/~lourakis/levmar/levmar.pdf
and the levmar-2.6 source code, specifically lm_core.c and lmbc_core.c

notation:
e in R^n = epsilon vector = (given data x  -  predicted data hx or \hat{x})
p in R^m = parameter vector
Dp in R^m = delta p = last step taken in parameter space
J nxm matrix = Jacobian of i'th predicted data w.r.t j'th parm; J_ij = d hx_i / d p_j
mu in R = damping term added to diagonal of [J^T J] to normalize it
   (mu set to tau * max[J^T J]_ii)
lb, ub = lower (per-parameter) bounds, upper bounds

double opts[5] for controlling levmar functions:
opts[0] = (\tau) the scale factor for initial \mu  (default LM_INIT_MU)
(GLK renamed these eps from "epsilon" to avoid confusion with epsilon vector e)
opts[1] = (\eps1) stopping threshold for ||J^T e||_inf (default LM_STOP_THRESH)
opts[2] = (\eps2) stopping threshold for ||Dp||_2 (default LM_STOP_THRESH)
opts[3] = (\eps3) stopping threshold for ||e||_2 (default LM_STOP_THRESH)
(opts[4] is only for dlevmar_dif(), and it is not a stopping threshold;
 dlevmar_der() only reads opts[0..3])
opts[4] = (\delta) step size used in difference approximation of Jacobian.
If \delta<0, the Jacobian is approximated with central differences which
are more accurate (but slower!) compared to the forward differences
(as employed by default). (default LM_DIFF_DELTA)
Set to opts=NULL for defaults to be used (listed above)

GLK's understanding of these functions is also in the `if (bag->verb)` diagnostics
about the info[LM_INFO_SZ] vector and interpreting the stopping info[6]
*/
#if TEEM_LEVMAR
// these constants come from levmar.h
static double optsDefault[5] = {LM_INIT_MU,     // 0  (from looking at lm_core.c)
                                LM_STOP_THRESH, // 1
                                LM_STOP_THRESH, // 2
                                LM_STOP_THRESH, // 3
                                LM_DIFF_DELTA}; // 4
#else
static double optsDefault[5] = {0, 0, 0, 0, 0};
#endif

static void
levmarCall(Nrrd *ndata, int ajac, int itmax,
           /* const (levmar sadly has no const correctness) */ double *bndLo,
           /* const */ double *bndHi,
           /* const */ double opts[5], bagOstate *bag) {
  int IT = -1;     // short variable name just for formatting
  double info[10]; // == LM_INFO_SZ, but hard-coding so non-LEVMAR version compiles
  // NaN-out the output info[] vector
  for (unsigned int ii = 0; ii < 10; ii++) {
    info[ii] = AIR_NAN;
  }
  int M = (int)(bag->M);
  int N = (int)(bag->N);
  assert(!!bndLo == !!bndHi);           // either have both bndLo and bndHi or neither
  double *ym = (double *)(ndata->data); // measured y values
  AIR_UNUSED(ndata);
  AIR_UNUSED(itmax);
  AIR_UNUSED(M);
  AIR_UNUSED(N);
  AIR_UNUSED(ym);
  char lmfname[AIR_STRLEN_SMALL + 1];
  if (ajac) {     // analytic Jacobian
    if (!bndLo) { // no lower bounds or upper bounds
      strcpy(lmfname, "dlevmar_der");
      /* extern int dlevmar_der(
         void (*func)(double *p, double *hx, int m, int n, void *adata),
         void (*jacf)(double *p, double *j, int m, int n, void *adata),
         double *p, double *x, int m, int n, int itmax, double *opts,
         double *info, double *work, double *covar, void *adata); */
#if TEEM_LEVMAR
      IT = dlevmar_der(funcPredict,                          //
                       funcJacobian,                         //
                       bag->parmCurr, ym, M, N, itmax, opts, //
                       info, NULL, NULL, AIR_VOIDP(bag));
#endif
    } else { // bndLo and bndHi form _b_ox _c_constraints -> bc
      strcpy(lmfname, "dlevmar_bc_der");
      /* Why do the bc functions need dscl?  GLK not sure. Quoting from lmbc_core.c:
      "The algorithm implemented by this function employs projected gradient steps. Since
      steepest descent is very sensitive to poor scaling, diagonal scaling has been
      implemented through the dscl argument: Instead of minimizing f(p) for p, f(D*q) is
      minimized for q=D^-1*p, D being a diagonal scaling matrix whose diagonal equals
      dscl (see Nocedal-Wright p.27). dscl should contain "typical" magnitudes for the
      parameters p. A NULL value for dscl implies no scaling. i.e. D=I. To account for
      scaling, the code divides the starting point and box bounds pointwise by dscl.
      Moreover, before calling func and jacf the scaling has to be undone (by
      multiplying), as should be done with the final point. Note also that jac_q=jac_p*D,
      where jac_q, jac_p are the jacobians w.r.t. q & p, resp."
      (but nothing there about dscl is specific to having box constraints?) */
      /* extern int dlevmar_bc_der(
      void (*func)(double *p, double *hx, int m, int n, void *adata),
      void (*jacf)(double *p, double *j, int m, int n, void *adata),
      double *p, double *x, int m, int n, double *lb, double *ub, double *dscl,
      int itmax, double *opts, double *info, double *work, double *covar, void *adata);
      */
#if TEEM_LEVMAR
      IT = dlevmar_bc_der(funcPredict,                                 //
                          funcJacobian,                                //
                          bag->parmCurr, ym, M, N, bndLo, bndHi, NULL, //
                          itmax, opts, info, NULL, NULL, AIR_VOIDP(bag));
#endif
    }
  } else { // !ajac: numerical derivatives of predicted data
    if (!bndLo) {
      strcpy(lmfname, "dlevmar_dif");
      /* extern int dlevmar_dif(
         void (*func)(double *p, double *hx, int m, int n, void *adata),
         double *p, double *x, int m, int n, int itmax, double *opts,
         double *info, double *work, double *covar, void *adata); */
#if TEEM_LEVMAR
      IT = dlevmar_dif(funcPredict,                          //
                       bag->parmCurr, ym, M, N, itmax, opts, //
                       info, NULL, NULL, AIR_VOIDP(bag));
#endif
    } else {
      strcpy(lmfname, "dlevmar_bc_dif");
      /* extern int dlevmar_bc_dif(
      void (*func)(double *p, double *hx, int m, int n, void *adata),
      double *p, double *x, int m, int n, double *lb, double *ub, double *dscl,
      int itmax, double *opts, double *info, double *work, double *covar, void *adata);
      */
#if TEEM_LEVMAR
      IT = dlevmar_bc_dif(funcPredict,                                 //
                          bag->parmCurr, ym, M, N, bndLo, bndHi, NULL, //
                          itmax, opts, info, NULL, NULL, AIR_VOIDP(bag));
#endif
    }
  }
#if !(TEEM_LEVMAR)
  printf("%s: Did NOT call %s() because we don't have TEEM_LEVMAR\n", __func__, lmfname);
#endif
  printf("%s: After %d iters, %s ended at parmCurr =\n", __func__, IT, lmfname);
  for (unsigned int pi = 0; pi < bag->M; pi++) {
    printf(" %.17g", bag->parmCurr[pi]);
  }
  printf("\n");
  if (bag->verb) {
    printf("info[0]: Chi^2 error at initial p (||e||^2): %.17g\n", info[0]);
    printf("info[1]: Chi^2 error at final p (||e||^2): %.17g\n", info[1]);
    printf("info[2]: Gradient norm at final p (||J^T e||_inf): %.17g\n", info[2]);
    printf("info[3]: Norm of last step (||Dp||_2): %.17g\n", info[3]);
    printf("info[4]: Final damping scaling tau (mu/max[J^T J]_ii): %.17g\n", info[4]);
    printf("info[5]: # of iterations: %.0f\n", info[5]);
    printf("info[6]: Termination reason: ");
    char obuff[AIR_STRLEN_SMALL + 1];
#define OPTS(I)                                                                         \
  (opts ? (sprintf(obuff, "%.17g", opts[I]), obuff)                                     \
        : (sprintf(obuff, "default %.17g", optsDefault[I]), obuff))
    switch ((int)(info[6])) {
    case 1:
      printf("(1) stopped by small gradient ||J^T e||_inf < %s = eps1\n", OPTS(1));
      break;
    case 2:
      printf("(2) stopped by small parameter step ||Dp||_2 < %s = eps2\n", OPTS(2));
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
      printf("(6) stopped by small residual ||e||_2 < %s = eps3\n", OPTS(3));
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
  hestOptAdd_Nv_Double(&hopt, "tp", "true poly coeffs", 1, -1, &(bag->tp), NULL,
                       "coefficients of (ground-truth) polynomial to sample, to "
                       "synthsize the data to later fit. \"-tp A B C\" means "
                       "A + Bx + Cx^2.  These coefficients are the _M_ "
                       "parameters that the LM method seeks to recover",
                       &(bag->M));
  // HEY should add hest opt for initial parameter vector
  hestOptAdd_1_UInt(&hopt, "N", "# points", &(bag->N), "42",
                    "How many times to sample polynomial to generate data _N_ data "
                    "points. Part of the purpose of this demo is to connect the "
                    "stupid single-letter variable names used in the levmar docs "
                    "to this concrete example.  NOTE that how the levmar code and docs "
                    "use \"n\" vs \"m\" is *flipped* from many other presentations of "
                    "Levenberg-Marquardt.");
  hestOptAdd_2_Double(&hopt, "xmm", "xmin xmax", bag->xmm, "-1 1",
                      "polynomial will be evaluated at _N_ cell-centered points along "
                      "this interval on X axis");
  double rndStdvSeed[2];
  hestOptAdd_2_Double(&hopt, "noise", "stdv seed", rndStdvSeed, "0 67",
                      "Gaussian noise of this stdv will be added to polynomial "
                      "evaluations to generate data, and the seed value (cast to "
                      "a uint) will be used to initialized the RNG");
  char *outS;
  hestOptAdd_1_String(&hopt, "o", "data out", &outS, NULL,
                      "If given a filename here, will save out synthetic data");
  int itmax;
  hestOptAdd_1_Int(&hopt, "itmax", "# iters", &itmax, "100", "cap on # iterations");
// https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html#Stringizing
#if TEEM_LEVMAR
#  define _str(s) #s
#  define str(s)  _str(s)
#else
#  define str(s) ("nan")
#endif
  double tau;
  hestOptAdd_1_Double(&hopt, "tau", "tau", &tau, str(LM_INIT_MU),
                      "Initial damping mu is found by multiplying max[J^T J]_ii by this "
                      "number, called tau");
  double eps1;
  hestOptAdd_1_Double(&hopt, "eps1", "thresh", &eps1, str(LM_STOP_THRESH),
                      "stopping thresh on (Linf of) the parm gradient [J^T (x-hx)]");
  double eps2;
  hestOptAdd_1_Double(&hopt, "eps2", "thresh", &eps2, str(LM_STOP_THRESH),
                      "stopping thresh on (L2 of) the parm delta Dp");
  double eps3;
  hestOptAdd_1_Double(&hopt, "eps3", "thresh", &eps3, str(LM_STOP_THRESH),
                      "stopping thresh on (L2 of) the (x-hx) residual vector");
  int ajac;
  hestOptAdd_Flag(&hopt, "ajac", &ajac,
                  "use analytic Jacobian of predicted (modeled) data w.r.t parameters");
  double bc;
  hestOptAdd_1_Double(&hopt, "bc", "scaling", &bc, "0.0",
                      "If > 0, then create box constraints around the parameter "
                      "by first finding pmax the max absolute value of the "
                      "ground truth parameters, then box is bc*[-pmax,pmax]^M. "
                      "So to give breathing room want bc well above 1, but can "
                      "set bc < 1 for testing purposes.");
  double delta;
  hestOptAdd_1_Double(&hopt, "delta", "delta", &delta, str(LM_DIFF_DELTA),
                      "if not using analytic Jacobian, this is the "
                      "per-parameter delta to use for numerically computing it, "
                      "via forward (delta>0) or central (delta<0) differences");
  int nulopt;
  hestOptAdd_Flag(&hopt, "nulopt", &nulopt,
                  "Instead of creating the opts[] vector from the previous "
                  "tau,eps{1,2,3},delta options, just use NULL, which invokes "
                  "levmar's internal defaults");
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
  dataSynth(ndata, bag, rndStdvSeed, mop);
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

  // prepare for levmar call
  bag->parmCurr = AIR_MALLOC(bag->M, double);
  assert(bag->parmCurr);
  airMopAdd(mop, bag->parmCurr, airFree, airMopAlways);
  double *bndLo = NULL, *bndHi = NULL;
  if (bc > 0) {
    double pmax = 0;
    for (unsigned int pi = 0; pi < bag->M; pi++) {
      pmax = AIR_MAX(pmax, fabs(bag->tp[pi]));
    }
    if (!pmax) {
      pmax = 1;
    }
    bndLo = AIR_MALLOC(bag->M, double);
    assert(bndLo);
    airMopAdd(mop, bndLo, airFree, airMopAlways);
    bndHi = AIR_MALLOC(bag->M, double);
    assert(bndHi);
    airMopAdd(mop, bndHi, airFree, airMopAlways);
    double bnd = bc * pmax;
    for (unsigned int pi = 0; pi < bag->M; pi++) {
      bndLo[pi] = -bnd;
      bndHi[pi] = bnd;
    }
    if (bag->verb) {
      printf("%s: bc %g, pmax %g => box constraints [%g,%g]\n", me, bc, pmax, -bnd, bnd);
    }
  }
  // initialize parms that will be fitted
  for (unsigned int pi = 0; pi < bag->M; pi++) {
    bag->parmCurr[pi] = 0;
  }
  AIR_UNUSED(funcPredict);  // to quiet warnings if no TEEM_LEVMAR
  AIR_UNUSED(funcJacobian); // to quiet warnings if no TEEM_LEVMAR
  double opts[5] = {tau, eps1, eps2, eps3, delta};
  levmarCall(ndata, ajac, itmax, bndLo, bndHi, nulopt ? NULL : opts, bag);

  airMopOkay(mop);
  return 0;
}
TEND_CMD(lmdemo, INFO);
