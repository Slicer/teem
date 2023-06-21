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

#include "meet.h"

typedef union {
  const NrrdKernel ***k;
  void **v;
} _kernu;

/*
** ALLOCATES and returns a NULL-terminated array of all the
** NrrdKernels in Teem
*/
const NrrdKernel ** /* Biff: nope */
meetNrrdKernelAll(void) {
  airArray *arr;
  const NrrdKernel **kern;
  unsigned int ii;
  int di, ci, ai, dmax, cmax, amax;
  _kernu ku;

  ku.k = &kern;
  arr = airArrayNew(ku.v, NULL, sizeof(NrrdKernel *), 2);

#define ADD(K)                                                                          \
  ii = airArrayLenIncr(arr, 1);                                                         \
  kern[ii] = (K)

  /* kernel.c */
  ADD(nrrdKernelZero);
  /* NOT including nrrdKernelFlag, since it's more of an error code than a kernel */
  ADD(nrrdKernelBox);
  ADD(nrrdKernelBoxSupportDebug);
  ADD(nrrdKernelCatmullRomSupportDebug);
  ADD(nrrdKernelCatmullRomSupportDebugD);
  ADD(nrrdKernelCatmullRomSupportDebugDD);
  ADD(nrrdKernelCos4SupportDebug);
  ADD(nrrdKernelCos4SupportDebugD);
  ADD(nrrdKernelCos4SupportDebugDD);
  ADD(nrrdKernelCos4SupportDebugDDD);
  ADD(nrrdKernelCheap);
  ADD(nrrdKernelHermiteScaleSpaceFlag);
  ADD(nrrdKernelTent);
  ADD(nrrdKernelForwDiff);
  ADD(nrrdKernelCentDiff);
  ADD(nrrdKernelBCCubic);
  ADD(nrrdKernelBCCubicD);
  ADD(nrrdKernelBCCubicDD);
  ADD(nrrdKernelCatmullRom);
  ADD(nrrdKernelCatmullRomD);
  ADD(nrrdKernelCatmullRomDD);
  ADD(nrrdKernelAQuartic);
  ADD(nrrdKernelAQuarticD);
  ADD(nrrdKernelAQuarticDD);
  ADD(nrrdKernelC3Quintic);
  ADD(nrrdKernelC3QuinticD);
  ADD(nrrdKernelC3QuinticDD);
  ADD(nrrdKernelC4Hexic);
  ADD(nrrdKernelC4HexicD);
  ADD(nrrdKernelC4HexicDD);
  ADD(nrrdKernelC4HexicDDD);
  ADD(nrrdKernelC4HexicApproxInverse);
  ADD(nrrdKernelC5Septic);
  ADD(nrrdKernelC5SepticD);
  ADD(nrrdKernelC5SepticDD);
  ADD(nrrdKernelC5SepticDDD);
  ADD(nrrdKernelC5SepticApproxInverse);
  ADD(nrrdKernelGaussian);
  ADD(nrrdKernelGaussianD);
  ADD(nrrdKernelGaussianDD);
  ADD(nrrdKernelDiscreteGaussian);

  /* winKernel.c */
  ADD(nrrdKernelHann);
  ADD(nrrdKernelHannD);
  ADD(nrrdKernelHannDD);
  ADD(nrrdKernelBlackman);
  ADD(nrrdKernelBlackmanD);
  ADD(nrrdKernelBlackmanDD);

  /* bsplKernel.c */
  ADD(nrrdKernelBSpline1);
  ADD(nrrdKernelBSpline1D);
  ADD(nrrdKernelBSpline2);
  ADD(nrrdKernelBSpline2D);
  ADD(nrrdKernelBSpline2DD);
  ADD(nrrdKernelBSpline3);
  ADD(nrrdKernelBSpline3D);
  ADD(nrrdKernelBSpline3DD);
  ADD(nrrdKernelBSpline3DDD);
  ADD(nrrdKernelBSpline3ApproxInverse);
  ADD(nrrdKernelBSpline4);
  ADD(nrrdKernelBSpline4D);
  ADD(nrrdKernelBSpline4DD);
  ADD(nrrdKernelBSpline4DDD);
  ADD(nrrdKernelBSpline5);
  ADD(nrrdKernelBSpline5D);
  ADD(nrrdKernelBSpline5DD);
  ADD(nrrdKernelBSpline5DDD);
  ADD(nrrdKernelBSpline5ApproxInverse);
  ADD(nrrdKernelBSpline6);
  ADD(nrrdKernelBSpline6D);
  ADD(nrrdKernelBSpline6DD);
  ADD(nrrdKernelBSpline6DDD);
  ADD(nrrdKernelBSpline7);
  ADD(nrrdKernelBSpline7D);
  ADD(nrrdKernelBSpline7DD);
  ADD(nrrdKernelBSpline7DDD);
  ADD(nrrdKernelBSpline7ApproxInverse);

  /* tmfKernel.c
   nrrdKernelTMF[D+1][C+1][A] is d<D>_c<C>_<A>ef:
   Dth-derivative, C-order continuous ("smooth"), A-order accurate
   (for D and C, index 0 accesses the function for -1)
    NRRD_EXPORT NrrdKernel *const nrrdKernelTMF[4][5][5];
  */
  dmax = AIR_INT(nrrdKernelTMF_maxD);
  cmax = AIR_INT(nrrdKernelTMF_maxC);
  amax = AIR_INT(nrrdKernelTMF_maxA);
  for (di = -1; di <= dmax; di++) {
    for (ci = -1; ci <= cmax; ci++) {
      for (ai = 1; ai <= amax; ai++) {
        ADD(nrrdKernelTMF[di + 1][ci + 1][ai]);
      }
    }
  }

  /* NULL-terminate the list */
  ADD(NULL);
#undef ADD

  /* nix, not nuke the airArray */
  airArrayNix(arr);
  return kern;
}

/* kintegral(kd) makes an attempt to returns the kernel ki that is the integral of kd:
   the derivative of ki is kd. The knowledge here about what is a derivative of what is
   something that will be built into kernels in a future Teem version.  For now, we also
   have the new (as of June 2023) nrrdKernelDerivative, which tries to identify the
   derivative of a given kernel. nrrdKernelCheck checks on consistency of that with the
   pass "ikern" from this kintegral() */
static const NrrdKernel *
kintegral(const NrrdKernel *kd) {
  const NrrdKernel *ret = NULL;

  /* "INTGL(K)" is saying: return kernel named K if its derivative
    (only as found by adding "D" to name of K) turns out to be kd.
    This is only possible to the extent that the D suffix is used consistently
    for derivative kernels. */
#define INTGL(K)                                                                        \
  if (K##D == kd) {                                                                     \
    ret = K;                                                                            \
  }
  INTGL(nrrdKernelHann);
  INTGL(nrrdKernelHannD);
  INTGL(nrrdKernelBlackman);
  INTGL(nrrdKernelBlackmanD);

  INTGL(nrrdKernelBSpline1);
  INTGL(nrrdKernelBSpline2);
  INTGL(nrrdKernelBSpline2D);
  INTGL(nrrdKernelBSpline3);
  INTGL(nrrdKernelBSpline3D);
  INTGL(nrrdKernelBSpline3DD);
  INTGL(nrrdKernelBSpline4);
  INTGL(nrrdKernelBSpline4D);
  INTGL(nrrdKernelBSpline4DD);
  INTGL(nrrdKernelBSpline5);
  INTGL(nrrdKernelBSpline5D);
  INTGL(nrrdKernelBSpline5DD);
  INTGL(nrrdKernelBSpline6);
  INTGL(nrrdKernelBSpline6D);
  INTGL(nrrdKernelBSpline6DD);
  INTGL(nrrdKernelBSpline7);
  INTGL(nrrdKernelBSpline7D);
  INTGL(nrrdKernelBSpline7DD);

  INTGL(nrrdKernelCos4SupportDebug);
  INTGL(nrrdKernelCos4SupportDebugD);
  INTGL(nrrdKernelCos4SupportDebugDD);
  INTGL(nrrdKernelCatmullRomSupportDebug);
  INTGL(nrrdKernelCatmullRomSupportDebugD);
  INTGL(nrrdKernelBCCubic);
  INTGL(nrrdKernelBCCubicD);
  INTGL(nrrdKernelCatmullRom);
  INTGL(nrrdKernelCatmullRomD);
  INTGL(nrrdKernelAQuartic);
  INTGL(nrrdKernelAQuarticD);
  INTGL(nrrdKernelC3Quintic);
  INTGL(nrrdKernelC3QuinticD);
  INTGL(nrrdKernelC4Hexic);
  INTGL(nrrdKernelC4HexicD);
  INTGL(nrrdKernelC4HexicDD);
  INTGL(nrrdKernelC5Septic);
  INTGL(nrrdKernelC5SepticD);
  INTGL(nrrdKernelC5SepticDD);
  INTGL(nrrdKernelGaussian);
  INTGL(nrrdKernelGaussianD);
#undef INTGL
  return ret;
}

/*
** Does more than call nrrdKernelCheck on all kernels:
** makes sure that all kernels have unique names
** makes sure that derivative relationships are correct
** Also, simply calling nrrdKernelCheck requires some knowledge
** of the kernel's needed parameters
**
** HEY: its problematic that because the various kernels have different
** parameter epsilon requirements, they usually end up having to be
** enumerated in some of the if/else statements below; it would be much
** better if new kernels didn't need to be so explicitly added!
*/
int /* Biff: 1 */
meetNrrdKernelAllCheck(void) {
  static const char me[] = "meetNrrdKernelAllCheck";
  const NrrdKernel **kern, *kk, *ll;
  unsigned int ki, kj, pnum;
  airArray *mop;
  double epsl, XX, YY, parm0[NRRD_KERNEL_PARMS_NUM], parm1_1[NRRD_KERNEL_PARMS_NUM],
    parm1_X[NRRD_KERNEL_PARMS_NUM], parm[NRRD_KERNEL_PARMS_NUM];
  size_t evalNum;
  int EE;

  mop = airMopNew();
  kern = meetNrrdKernelAll();
  airMopAdd(mop, AIR_VOIDP(kern), airFree, airMopAlways);
  evalNum = 120001; /* success of kernel integral test is surprisingly
                       dependent on this, likely due to the naive way
                       the integral is numerically computed; the current
                       value here represents some experimentation */
  epsl = 0.9e-5;
  XX = 7.0 / 3.0;     /* 2.333.. */
  YY = 43.0 / 9.0;    /* 4.777.. */
  parm0[0] = AIR_NAN; /* shouldn't be read */
  parm1_1[0] = 1.0;
  parm1_X[0] = XX;
  ki = 0;
  while ((kk = kern[ki])) {
    kj = 0;
    while (kj < ki) {
      ll = kern[kj];
      if (kk == ll) {
        biffAddf(MEET, "%s: kern[%u] and [%u] were identical (%s)", me, kj, ki,
                 kk->name);
        airMopError(mop);
        return 1;
      }
      if (!airStrcmp(kk->name, ll->name)) {
        biffAddf(MEET, "%s: kern[%u] and [%u] have same name (%s)", me, kj, ki,
                 kk->name);
        airMopError(mop);
        return 1;
      }
      kj++;
    }
    pnum = kk->numParm;
    EE = 0;
    /* the second argument to CHECK is how much to scale up the
       permissible error in kernel evaluations (between float and double)
       The kernels for which this is higher should be targets for
       re-coding with an eye towards numerical accuracy */
#define CHECK(P, S, N)                                                                  \
  if (!EE) EE |= nrrdKernelCheck(kk, (P), evalNum, epsl * (S), N, N, kintegral(kk), (P));
    /* clang-format off */
    if (nrrdKernelBCCubic == kk ||
        nrrdKernelBCCubicD == kk ||
        nrrdKernelBCCubicDD == kk) {
      /* try a few settings of the 3 parms */
      ELL_3V_SET(parm, 1.0, 0.0, 0.0); CHECK(parm, 2, 3);
      ELL_3V_SET(parm, XX, 0.0, 0.0); CHECK(parm, 2, 3);
      ELL_3V_SET(parm, 1.0, 1.0/3.0, 1.0/3.0); CHECK(parm, 2, 3);
      ELL_3V_SET(parm, XX, 1.0/3.0, 1.0/3.0); CHECK(parm, 2, 3);
      ELL_3V_SET(parm, 1.0, 0.0, 1.0); CHECK(parm, 2, 3);
      ELL_3V_SET(parm, XX, 0.0, 1.0); CHECK(parm, 2, 3);
      ELL_3V_SET(parm, 1.0, 0.5, 0.0); CHECK(parm, 2, 3);
      ELL_3V_SET(parm, XX, 0.5, 0.0); CHECK(parm, 2, 3);
    } else if (2 == pnum) {
      if (nrrdKernelAQuartic == kk ||
          nrrdKernelAQuarticD == kk ||
          nrrdKernelAQuarticDD == kk) {
        ELL_2V_SET(parm, 1.0, 0.0); CHECK(parm, 10, 2);
        ELL_2V_SET(parm, 1.0, 0.5); CHECK(parm, 10, 2);
        ELL_2V_SET(parm, XX, 0.0);  CHECK(parm, 10, 2);
        ELL_2V_SET(parm, XX, 0.5);  CHECK(parm, 10, 2);
      } else if (nrrdKernelGaussian == kk ||
                 nrrdKernelGaussianD == kk ||
                 nrrdKernelGaussianDD == kk) {
        ELL_2V_SET(parm, 0.1, XX); CHECK(parm, 10, 2);
        ELL_2V_SET(parm, 0.1, YY); CHECK(parm, 10, 2);
        ELL_2V_SET(parm, 1.0, XX); CHECK(parm, 10, 2);
        ELL_2V_SET(parm, 1.0, YY); CHECK(parm, 10, 2);
        ELL_2V_SET(parm, XX, XX);  CHECK(parm, 10, 2);
        ELL_2V_SET(parm, XX, YY);  CHECK(parm, 10, 2);
      } else if (nrrdKernelHann == kk ||
                 nrrdKernelHannD == kk ||
                 nrrdKernelHannDD == kk ||
                 nrrdKernelBlackman == kk ||
                 nrrdKernelBlackmanD == kk ||
                 nrrdKernelBlackmanDD == kk) {
        /* these kernels punt on knowing their integral, hence the 80, but their June 2023
        re-write means that their numerical derivatives are finally agreeing with their
        analytic derivatives */
        ELL_2V_SET(parm, 0.5, XX); CHECK(parm, 80, 2);
        ELL_2V_SET(parm, 0.5, YY); CHECK(parm, 80, 2);
        ELL_2V_SET(parm, 1.0, XX); CHECK(parm, 80, 2);
        ELL_2V_SET(parm, 1.0, YY); CHECK(parm, 80, 2);
        ELL_2V_SET(parm, XX, XX);  CHECK(parm, 80, 2);
        ELL_2V_SET(parm, XX, YY);  CHECK(parm, 80, 2);
      } else if (nrrdKernelDiscreteGaussian == kk) {
        ELL_2V_SET(parm, 0.1, XX); CHECK(parm, 1, 2);
        ELL_2V_SET(parm, 0.1, YY); CHECK(parm, 1, 2);
        ELL_2V_SET(parm, 1.0, XX); CHECK(parm, 1, 2);
        ELL_2V_SET(parm, 1.0, YY); CHECK(parm, 1, 2);
        ELL_2V_SET(parm, XX, XX);  CHECK(parm, 1, 2);
        ELL_2V_SET(parm, XX, YY);  CHECK(parm, 1, 2);
      } else {
        biffAddf(MEET, "%s: sorry, got unexpected 2-parm kernel %s",
                 me, kk->name);
        airMopError(mop); return 1;
      }
    } else if (1 == pnum) {
      if (strstr(kk->name, "TMF")) {
        /* these take a single parm, but its not support */
        parm[0] = 0.0;     CHECK(parm, 10, 2);
        parm[0] = 1.0/3.0; CHECK(parm, 10, 2);
      } else {
        /* zero, box, boxsup, cos4sup{,D,DD,DDD}, cheap,
           ctmrsup{,D,DD}, tent, fordif, cendif */
        /* takes a single support/scale parm[0], try two different values */
        if (nrrdKernelCos4SupportDebug == kk ||
            nrrdKernelCos4SupportDebugD == kk ||
            nrrdKernelCos4SupportDebugDD == kk ||
            nrrdKernelCos4SupportDebugDDD == kk ||
            nrrdKernelCatmullRomSupportDebugD == kk ||
            nrrdKernelCatmullRomSupportDebugDD == kk) {
          CHECK(parm1_1, 10, 4);
          CHECK(parm1_X, 10, 4);
        } else if (nrrdKernelBox == kk || nrrdKernelForwDiff == kk) {
          CHECK(parm1_1, 1, 4);
          CHECK(parm1_X, 1, 4);
        } else {
          CHECK(parm1_1, 1, 2);
          CHECK(parm1_X, 1, 2);
        }
      }
    } else if (0 == pnum) {
      /* C3Quintic{,D,DD,DD}, C4Hexic{,D,DD,DDD}, C5Septic{,D},
         hermiteSS, catmull-rom{,D}, bspl{3,5,7}{,D,DD,DDD} */
      if (nrrdKernelC3Quintic == kk ||
          nrrdKernelC3QuinticD == kk ||
          nrrdKernelC3QuinticDD == kk ||
          nrrdKernelC4Hexic == kk ||
          nrrdKernelC4HexicD == kk ||
          nrrdKernelC4HexicDD == kk ||
          nrrdKernelC4HexicDDD == kk ||
          nrrdKernelC5Septic == kk ||
          nrrdKernelC5SepticD == kk ||
          nrrdKernelC5SepticDD == kk ||
          nrrdKernelC5SepticDDD == kk
          ) {
        CHECK(parm0, 1, 2);
        CHECK(parm0, 1, 2);
      } else if (nrrdKernelBSpline5DD == kk ||
                 nrrdKernelBSpline5DDD == kk ||
                 nrrdKernelBSpline7DD == kk ) {
        CHECK(parm0, 100, 2);
      } else if (nrrdKernelBSpline7ApproxInverse == kk) {
        CHECK(parm0, 30, 3);
      } else {
        CHECK(parm0, 10, 3);
      }
    } else {
      biffAddf(MEET, "%s: sorry, didn't expect %u parms for %s",
               me, pnum, kk->name);
      airMopError(mop); return 1;
    }
    /* clang-format on */
#undef CHECK
    if (EE) {
      biffMovef(MEET, NRRD, "%s: problem with kern[%u] \"%s\"", me, ki, kk->name);
      airMopError(mop);
      return 1;
    }
    ki++;
  }

  airMopOkay(mop);
  return 0;
}
