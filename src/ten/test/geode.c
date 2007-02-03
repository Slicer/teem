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

double
relax(Nrrd *ntt, unsigned int ii, int useK, double step) {
  unsigned int NN, jj, kk;
  double *tt, *t0, *t1, *t2, igrt[6][7], eval[3], evec[9], tmp,
    d01[7], d12[7], orig[7], diff[7], tng[7], len01, len12, correct;

  NN = ntt->axis[1].size;
  tt = AIR_CAST(double *, ntt->data);
  t0 = tt + (ii-1)*7;
  t1 = tt + (ii+0)*7;
  t2 = tt + (ii+1)*7;
  TEN_T_COPY(orig, t1);

  if (useK) {
    tenInvariantGradientsK_d(igrt[0], igrt[1], igrt[2], t1, 0.0000001);
  } else {
    tenInvariantGradientsR_d(igrt[0], igrt[1], igrt[2], t1, 0.0000001);
  }
  tenEigensolve_d(eval, evec, t1);
  tenRotationTangents_d(igrt[3], igrt[4], igrt[5], evec);

  /*
  fprintf(stderr, "------\n");
  for (jj=0; jj<6; jj++) {
    for (kk=jj; kk<6; kk++) {
      fprintf(stderr, "%g ", TEN_T_DOT(igrt[jj], igrt[kk]));
    }
    fprintf(stderr, "\n");
  }
  */
  
  
  TEN_T_SUB(d01, t1, t0);
  TEN_T_SUB(d12, t2, t1);
  for (jj=0; jj<6; jj++) {

    len01 = TEN_T_DOT(igrt[jj], d01);
    len12 = TEN_T_DOT(igrt[jj], d12);
    correct = (len12 - len01)/2;
    TEN_T_SCALE_INCR(t1, correct*step, igrt[jj]);
    if (1 == jj) {
      fprintf(stderr, "%u %u: %f %f   %f    %f\n", ii, jj, 
              len01, len12, correct, len12/len01);
    }
    /*
    double dd01[7], dd12[7];
    TEN_T_SUB(dd01, t1, t0);
    TEN_T_SUB(dd12, t2, t1);
    len01 = TEN_T_DOT(igrt[jj], dd01);
    len12 = TEN_T_DOT(igrt[jj], dd12);
    fprintf(stderr, "%u %u: %f --> %f\n", ii, jj,
            correct*100000000, (len12 - len01)*100000000/2);
    */
  }

#if 0
  TEN_T_SUB(d01, t1, t0);
  TEN_T_SUB(d12, t2, t1);
  TEN_T_SUB(tng, t2, t0);
  tmp = 1.0/TEN_T_NORM(tng);
  TEN_T_SCALE(tng, tmp, tng);
  len01 = TEN_T_DOT(tng, d01);
  len12 = TEN_T_DOT(tng, d12);
  correct = (len12 - len01)/2;
  TEN_T_SCALE_INCR(t1, correct*step, tng);
#endif

  TEN_T_SUB(diff, orig, t1);
  return TEN_T_NORM(diff);
}

int
main(int argc, char *argv[]) {
  char *me, *err;
  hestOpt *hopt=NULL;
  airArray *mop;

  char *outS;
  double _tA[6], tA[7], _tB[6], tB[7], *tt,
    pA[3], pB[3], qA[4], qB[4], rA[9], rB[9], mat1[9], mat2[9], tmp;
  unsigned int ii, jj, NN;
  Nrrd *ntt;

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
  
  ntt = nrrdNew();
  airMopAdd(mop, ntt, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdAlloc_va(ntt, nrrdTypeDouble, 2, 7, NN)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble allocating output:\n%s\n",
            me, err);
    airMopError(mop); 
    return 1;
  }
  tt = AIR_CAST(double *, ntt->data);

  for (ii=0; ii<NN; ii++) {
    double ss;
    ss = AIR_AFFINE(0, ii, NN-1, 0.0, 1.0);
    ss = pow(ss, 1);
    TEN_T_AFFINE(tt + ii*7, 0.0, ss, 1.0, tA, tB);
  }
  nrrdSave("orig.nrrd", ntt, NULL);

  for (ii=NN/2; ii<NN/2+1; ii++) {
    relax(ntt, ii, AIR_FALSE, 1);
  }
  
  if (nrrdSave(outS, ntt, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble saving output:\n%s\n", me, err);
    airMopError(mop); 
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
