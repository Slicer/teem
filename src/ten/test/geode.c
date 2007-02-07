/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "../ten.h"

char *info = ("does stupid geodesics");

void
_tenGeodesicRelaxOne(Nrrd *ntdata, Nrrd *nigrtdata,
                     unsigned int ii, double scl) {
  double *tdata, *igrtdata, *tt[5], *igrt[5][6], d02[7], d24[7], len02, len24,
    tmp, tng[7], correct, half;
  unsigned int jj, NN;

  NN = (ntdata->axis[1].size-1)/2;
  half = (NN+1)/2;

  tdata = AIR_CAST(double *, ntdata->data);
  tt[0] = tdata + 7*(2*ii - 2);
  tt[1] = tdata + 7*(2*ii - 1); /* unused */
  tt[2] = tdata + 7*(2*ii + 0);
  tt[3] = tdata + 7*(2*ii + 1); /* unused */
  tt[4] = tdata + 7*(2*ii + 2);
  igrtdata = AIR_CAST(double *, nigrtdata->data);
  for (jj=0; jj<6; jj++) {
    igrt[0][jj] = igrtdata + 7*(jj + 6*(2*ii - 2)); /* unused */
    igrt[1][jj] = igrtdata + 7*(jj + 6*(2*ii - 1));
    igrt[2][jj] = igrtdata + 7*(jj + 6*(2*ii + 0));
    igrt[3][jj] = igrtdata + 7*(jj + 6*(2*ii + 1));
    igrt[4][jj] = igrtdata + 7*(jj + 6*(2*ii + 2)); /* unused */
  }

  /* re-align [1] and [3] bases relative to [2] */
  for (jj=2; jj<=5; jj++) {
    if (TEN_T_DOT(igrt[1][jj], igrt[2][jj]) < 0) {
      TEN_T_SCALE(igrt[1][jj], -1, igrt[1][jj]);
    }
    if (TEN_T_DOT(igrt[3][jj], igrt[2][jj]) < 0) {
      TEN_T_SCALE(igrt[3][jj], -1, igrt[1][jj]);
    }
  }  

  TEN_T_SUB(d02, tt[2], tt[0]);
  TEN_T_SUB(d24, tt[4], tt[2]);
  for (jj=0; jj<6; jj++) {
    len02 = TEN_T_DOT(igrt[1][jj], d02);
    len24 = TEN_T_DOT(igrt[3][jj], d24);
    correct = (len24 - len02)/2;
    TEN_T_SCALE_INCR(tt[2], correct*scl, igrt[2][jj]);
  }

  TEN_T_SUB(d02, tt[2], tt[0]);
  TEN_T_SUB(d24, tt[4], tt[2]);
  TEN_T_SUB(tng, tt[4], tt[0]);
  tmp = 1.0/TEN_T_NORM(tng);
  TEN_T_SCALE(tng, tmp, tng);
  len02 = TEN_T_DOT(tng, d02);
  len24 = TEN_T_DOT(tng, d24);
  correct = (len24 - len02)/2;
  TEN_T_SCALE_INCR(tt[2], correct*scl, tng);

  TEN_T_SCALE_INCR2(tt[1], 0.5, tt[0], 0.5, tt[2]);
  TEN_T_SCALE_INCR2(tt[3], 0.5, tt[2], 0.5, tt[4]);
  return;
}

/*
** this assumes that the real vertices are on the even-numbered indices
** (0   1   2   3   4)
**  0   2   4   6   8 --> size=9 --> NN=4
**    1   3   5   7
*/
double
_tenGeodesicLength(Nrrd *ntt) {
  double *tt, len, diff[7];
  unsigned int ii, NN;
  
  tt = AIR_CAST(double *, ntt->data);
  NN = (ntt->axis[1].size-1)/2;
  len = 0;
  for (ii=0; ii<NN; ii++) {
    TEN_T_SUB(diff, tt + 7*2*(ii + 1), tt + 7*2*(ii + 0));
    len += TEN_T_NORM(diff);
  }
  return len;
}

void
_tenGeodesicIGRT(double *igrt, double *ten, int useK, double minnorm) {
  double eval[3], evec[9];

  if (useK) {
    tenInvariantGradientsK_d(igrt + 7*0, igrt + 7*1, igrt + 7*2, ten, minnorm);
  } else {
    tenInvariantGradientsR_d(igrt + 7*0, igrt + 7*1, igrt + 7*2, ten, minnorm);
  }
  tenEigensolve_d(eval, evec, ten);
  tenRotationTangents_d(igrt + 7*3, igrt + 7*4, igrt + 7*5, evec);
  return;
}

int
_tenGeodesicPolyLine(Nrrd *ngeod, unsigned int *numIter,
                     double tenA[7], double tenB[7],
                     unsigned int NN, int useK, double scl, int recurse,
                     double minnorm, unsigned int maxIter, double conv) {
  char me[]="_tenGeodesicPolyLine", err[BIFF_STRLEN];
  Nrrd *nigrt, *ntt, *nsub;
  double *igrt, *geod, *tt, len, newlen;
  unsigned int ii;
  airArray *mop;

  if (!(ngeod && numIter && tenA && tenB)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!(NN >= 2)) {
    sprintf(err, "%s: # steps %u too small", me, NN);
    biffAdd(TEN, err); return 1;
  }

  mop = airMopNew();
  ntt = nrrdNew();
  airMopAdd(mop, ntt, (airMopper)nrrdNuke, airMopAlways);
  nigrt = nrrdNew();
  airMopAdd(mop, nigrt, (airMopper)nrrdNuke, airMopAlways);
  nsub = nrrdNew();
  airMopAdd(mop, nsub, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdMaybeAlloc_va(ngeod, nrrdTypeDouble, 2,
                        AIR_CAST(size_t, 7),
                        AIR_CAST(size_t, NN+1))
      || nrrdMaybeAlloc_va(ntt, nrrdTypeDouble, 2,
                           AIR_CAST(size_t, 7),
                           AIR_CAST(size_t, 2*NN + 1))
      || nrrdMaybeAlloc_va(nigrt, nrrdTypeDouble, 3,
                           AIR_CAST(size_t, 7),
                           AIR_CAST(size_t, 6),
                           AIR_CAST(size_t, 2*NN + 1))) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }
  geod = AIR_CAST(double *, ngeod->data);
  tt = AIR_CAST(double *, ntt->data);
  igrt = AIR_CAST(double *, nigrt->data);

  *numIter = 0;
  if (NN > 14 && recurse) {
    unsigned int subIter;
    int E;
    NrrdResampleContext *rsmc;
    double kparm[3] = {1.0, 0.0, 0.5};
    /* recurse and find geodesic with smaller number of vertices */
    if (_tenGeodesicPolyLine(nsub, &subIter, tenA, tenB,
                             NN/2, useK, scl, recurse,
                             minnorm, maxIter, conv)) {
      sprintf(err, "%s: problem with recursive call", me);
      biffAdd(TEN, err); airMopError(mop); return 1;
    }
    /* upsample coarse geodesic to higher resolution */
    rsmc = nrrdResampleContextNew();
    airMopAdd(mop, rsmc, (airMopper)nrrdResampleContextNix, airMopAlways);
    E = AIR_FALSE;
    if (!E) E |= nrrdResampleDefaultCenterSet(rsmc, nrrdCenterNode);
    if (!E) E |= nrrdResampleNrrdSet(rsmc, nsub);
    if (!E) E |= nrrdResampleKernelSet(rsmc, 0, NULL, NULL);
    if (!E) E |= nrrdResampleKernelSet(rsmc, 1, nrrdKernelBCCubic, kparm);
    if (!E) E |= nrrdResampleSamplesSet(rsmc, 1, 2*NN + 1);
    if (!E) E |= nrrdResampleRangeFullSet(rsmc, 1);
    if (!E) E |= nrrdResampleBoundarySet(rsmc, nrrdBoundaryBleed);
    if (!E) E |= nrrdResampleTypeOutSet(rsmc, nrrdTypeDefault);
    if (!E) E |= nrrdResampleRenormalizeSet(rsmc, AIR_TRUE);
    if (!E) E |= nrrdResampleExecute(rsmc, ntt);
    if (E) {
      sprintf(err, "%s: problem upsampling course solution", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    *numIter += subIter;
  } else {
    /* initialize the path, including all the segment midpoints */
    for (ii=0; ii<=2*NN; ii++) {
      TEN_T_AFFINE(tt + 7*ii, 0, ii, 2*NN, tenA, tenB);
    }
  }
  for (ii=1; ii<2*NN; ii++) {
    _tenGeodesicIGRT(igrt + 7*6*ii, tt + 7*ii, useK, minnorm);
  }
  
  newlen = _tenGeodesicLength(ntt);
  do {
    unsigned int lo, hi;
    int dd;
    len = newlen;
    if (0 == *numIter % 2) {
      lo = 1;
      hi = NN;
      dd = 1;
    } else {
      lo = NN-1;
      hi = 0;
      dd = -1;
    }
    for (ii=lo; ii!=hi; ii+=dd) {
      _tenGeodesicRelaxOne(ntt, nigrt, ii , scl);
    }
    /* try doing this less often */
    for (ii=1; ii<2*NN; ii++) {
      _tenGeodesicIGRT(igrt + 7*6*ii, tt + 7*ii, useK, minnorm);
    }
    newlen = _tenGeodesicLength(ntt);
    *numIter += 1;
  } while ((0 == maxIter || *numIter < maxIter)
           && 2*AIR_ABS(newlen - len)/(newlen + len) > conv);

  /* copy final result to output */
  for (ii=0; ii<=NN; ii++) {
    TEN_T_COPY(geod + 7*ii, tt + 7*2*ii);
  }  

  airMopOkay(mop);
  return 0;
}


int
main(int argc, char *argv[]) {
  char *me, *err;
  hestOpt *hopt=NULL;
  airArray *mop;

  char *outS;
  double _tA[6], tA[7], _tB[6], tB[7], time0, time1, conv,
    pA[3], pB[3], qA[4], qB[4], rA[9], rB[9], mat1[9], mat2[9], tmp;
  unsigned int NN, iter, maxiter;
  int recurse, noop;
  Nrrd *ngeod;

  mop = airMopNew();
  me = argv[0];
  hestOptAdd(&hopt, "a", "tensor", airTypeDouble, 6, 6, _tA, NULL,
             "first tensor");
  hestOptAdd(&hopt, "pa", "qq", airTypeDouble, 3, 3, pA, "0 0 0",
             "rotation of first tensor");
  hestOptAdd(&hopt, "b", "tensor", airTypeDouble, 6, 6, _tB, NULL,
             "second tensor");
  hestOptAdd(&hopt, "pb", "qq", airTypeDouble, 3, 3, pB, "0 0 0",
             "rotation of second tensor");
  hestOptAdd(&hopt, "n", "# steps", airTypeUInt, 1, 1, &NN, "100",
             "number of steps in between two tensors");
  hestOptAdd(&hopt, "c", "conv", airTypeDouble, 1, 1, &conv, "0.0001",
             "convergence threshold of length fraction");
  hestOptAdd(&hopt, "mi", "maxiter", airTypeUInt, 1, 1, &maxiter, "0",
             "if non-zero, max # iterations for computation");
  hestOptAdd(&hopt, "r", "recurse", airTypeInt, 0, 0, &recurse, NULL,
             "enable recursive solution");
  hestOptAdd(&hopt, "no", "no-op", airTypeInt, 0, 0, &noop, NULL,
             "no-op solution, just lerp");
  hestOptAdd(&hopt, "o", "filename", airTypeString, 1, 1, &outS, "-",
             "file to write output nrrd to");
  hestParseOrDie(hopt, argc-1, argv+1, NULL,
                 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  ELL_6V_COPY(tA + 1, _tA);
  tA[0] = 1.0;
  ELL_6V_COPY(tB + 1, _tB);
  tB[0] = 1.0;

  ELL_4V_SET(qA, 1, pA[0], pA[1], pA[2]);
  ELL_4V_NORM(qA, qA, tmp);
  ELL_4V_SET(qB, 1, pB[0], pB[1], pB[2]);
  ELL_4V_NORM(qB, qB, tmp);
  ell_q_to_3m_d(rA, qA);
  ell_q_to_3m_d(rB, qB);

  TEN_T2M(mat1, tA);
  ell_3m_mul_d(mat2, rA, mat1);
  ELL_3M_TRANSPOSE_IP(rA, tmp);
  ell_3m_mul_d(mat1, mat2, rA);
  TEN_M2T(tA, mat1);

  TEN_T2M(mat1, tB);
  ell_3m_mul_d(mat2, rB, mat1);
  ELL_3M_TRANSPOSE_IP(rB, tmp);
  ell_3m_mul_d(mat1, mat2, rB);
  TEN_M2T(tB, mat1);
  /*
  fprintf(stderr, "!%s: tA = (%g) %g %g %g\n    %g %g\n    %g\n", me,
          tA[0], tA[1], tA[2], tA[3], tA[4], tA[5], tA[6]);
  fprintf(stderr, "!%s: tB = (%g) %g %g %g\n    %g %g\n    %g\n", me,
          tB[0], tB[1], tB[2], tB[3], tB[4], tB[5], tB[6]);
  */

  ngeod = nrrdNew();
  airMopAdd(mop, ngeod, (airMopper)nrrdNuke, airMopAlways);
  time0 = airTime();
  if (noop) {
    double *geod;
    unsigned int ii;
    nrrdMaybeAlloc_va(ngeod, nrrdTypeDouble, 2,
                      AIR_CAST(size_t, 7),
                      AIR_CAST(size_t, NN));
    geod = AIR_CAST(double *, ngeod->data);
    for (ii=0; ii<NN; ii++) {
      TEN_T_AFFINE(geod + 7*ii, 0, ii, NN-1, tA, tB);
    }
  } else {
    if (_tenGeodesicPolyLine(ngeod, &iter, tA, tB, NN-1,
                             AIR_FALSE, 1.0, recurse,
                             0.000001, maxiter, conv)) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble computing path:\n%s\n",
              me, err);
      airMopError(mop); 
      return 1;
    }
  }
  time1 = airTime();
  fprintf(stderr, "%s: geodesic length = %g; iter = %u; time = %g\n",
          me, _tenGeodesicLength(ngeod), iter, time1 - time0);

  if (nrrdSave(outS, ngeod, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble saving output:\n%s\n", me, err);
    airMopError(mop); 
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
