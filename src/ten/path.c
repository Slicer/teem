/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
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


#include "ten.h"
#include "privateTen.h"

tenPathParm *
tenPathParmNew(void) {
  tenPathParm *tpp;

  tpp = AIR_CAST(tenPathParm *, malloc(sizeof(tenPathParm)));
  if (tpp) {
    tpp->verbose = AIR_FALSE;
    tpp->convStep = 0.2;
    tpp->minNorm = 0.0;
    tpp->convEps = 0.0000001;
    tpp->enableRecurse = AIR_TRUE;
    tpp->maxIter = 0;
    tpp->numSteps = 100;
    tpp->lengthFancy = AIR_FALSE;
    tpp->numIter = 0;
    tpp->convFinal = AIR_NAN;
    tpp->lengthShape = AIR_NAN;
    tpp->lengthOrient = AIR_NAN;
  }
  return tpp;
}

tenPathParm *
tenPathParmNix(tenPathParm *tpp) {

  if (tpp) {
    free(tpp);
  }
  return NULL;
}

void
tenPathInterpTwo(double oten[7], 
                 double tenA[7], double tenB[7],
                 int ptype, double aa,
                 tenPathParm *tpp) {
  char me[]="tenPathInterpTwo";
  double logA[7], logB[7], tmp1[7], tmp2[7], sqrtA[7], isqrtA[7],
    logMean[7], mean[7], sqrtB[7], isqrtB[7],
    mat1[9], mat2[9], mat3[9];

  AIR_UNUSED(tpp);
  if (ptype == tenPathTypeLerp
      || ptype == tenPathTypeLogLerp
      || ptype == tenPathTypeAffineInvariant
      || ptype == tenPathTypeWang) {
    switch (ptype) {
    case tenPathTypeLerp:
      TEN_T_LERP(oten, aa, tenA, tenB);
      break;
    case tenPathTypeLogLerp:
      tenLogSingle_d(logA, tenA);
      tenLogSingle_d(logB, tenB);
      TEN_T_LERP(logMean, aa, logA, logB);
      tenExpSingle_d(oten, logMean);
      break;
    case tenPathTypeAffineInvariant:
      tenSqrtSingle_d(sqrtA, tenA);
      tenInv_d(isqrtA, sqrtA);
      TEN_T2M(mat1, tenB);
      TEN_T2M(mat2, isqrtA);
      ELL_3M_MUL(mat3, mat1, mat2);   /*  B * is(A) */
      ELL_3M_MUL(mat1, mat2, mat3);   /*  is(A) * B * is(A) */
      TEN_M2T(tmp2, mat1);
      tenPowSingle_d(tmp1, tmp2, aa); /*  m = (is(A) * B * is(A))^aa */
      TEN_T2M(mat1, tmp1);
      TEN_T2M(mat2, sqrtA);
      ELL_3M_MUL(mat3, mat1, mat2);   /*  m * sqrt(A) */
      ELL_3M_MUL(mat1, mat2, mat3);   /*  sqrt(A) * m * sqrt(A) */
      TEN_M2T(oten, mat1);
      oten[0] = AIR_LERP(aa, tenA[0], tenB[0]);
      if (tpp->verbose) {
        fprintf(stderr, "%s:\nA= %g %g %g   %g %g  %g\n"
                "B = %g %g %g   %g %g  %g\n"
                "foo = %g %g %g   %g %g  %g\n"
                "bar(%g) = %g %g %g   %g %g  %g\n", me,
                tenA[1], tenA[2], tenA[3], tenA[4], tenA[5], tenA[6],
                tenB[1], tenB[2], tenB[3], tenB[4], tenB[5], tenB[6],
                tmp1[1], tmp1[2], tmp1[3], tmp1[4], tmp1[5], tmp1[6],
                aa, oten[1], oten[2], oten[3], oten[4], oten[5], oten[6]);
      }
      break;
    case tenPathTypeWang:
      /* HEY: this seems to be broken */
      TEN_T_LERP(mean, aa, tenA, tenB);    /* "A" = mean */
      tenLogSingle_d(logA, tenA);
      tenLogSingle_d(logB, tenB);
      TEN_T_LERP(logMean, aa, logA, logB); /* "B" = logMean */
      tenSqrtSingle_d(sqrtB, logMean);
      tenInv_d(isqrtB, sqrtB);
      TEN_T2M(mat1, mean);
      TEN_T2M(mat2, isqrtB);
      ELL_3M_MUL(mat3, mat1, mat2);
      ELL_3M_MUL(mat1, mat2, mat3);
      TEN_M2T(tmp1, mat1);
      tenSqrtSingle_d(oten, tmp1);
      oten[0] = AIR_LERP(aa, tenA[0], tenB[0]);
      break;
    }
  } else {
    /* otherwise (currently) no closed-form expression for these */
    TEN_T_SET(oten, AIR_NAN, AIR_NAN, AIR_NAN, AIR_NAN,
              AIR_NAN, AIR_NAN, AIR_NAN);
  }
  return;
}

int
_tenPathGeodeLoxoRelaxOne(Nrrd *nodata, Nrrd *ntdata, Nrrd *nigrtdata,
                          unsigned int ii, int rotnoop, double scl,
                          tenPathParm *tpp) {
  char me[]="_tenPathGeodeLoxoRelaxOne", err[BIFF_STRLEN];
  double *tdata, *odata, *igrtdata, *tt[5], *igrt[5][6], d02[7], d24[7],
    len02, len24, tmp, tng[7], correct, half, update[7];
  unsigned int jj, NN;

  NN = (ntdata->axis[1].size-1)/2;
  half = (NN+1)/2;

  if (tpp->verbose) {
    fprintf(stderr, "---- %u --> %u %u %u %u %u\n", ii,
            2*ii - 2, 2*ii - 1, 2*ii, 2*ii + 1, 2*ii + 2);
  }
  tdata = AIR_CAST(double *, ntdata->data);
  odata = AIR_CAST(double *, nodata->data);
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
  /* HEY: should I be worrying about aligning the mode normal
     when it had to be computed from eigenvectors? */
  for (jj=3; jj<6; jj++) {
    if (TEN_T_DOT(igrt[1][jj], igrt[2][jj]) < 0) {
      TEN_T_SCALE(igrt[1][jj], -1, igrt[1][jj]);
    }
    if (TEN_T_DOT(igrt[3][jj], igrt[2][jj]) < 0) {
      TEN_T_SCALE(igrt[3][jj], -1, igrt[1][jj]);
    }
  }  

  TEN_T_SUB(tng, tt[4], tt[0]);
  tmp = 1.0/TEN_T_NORM(tng);
  TEN_T_SCALE(tng, tmp, tng);

  TEN_T_SUB(d02, tt[2], tt[0]);
  TEN_T_SUB(d24, tt[4], tt[2]);
  TEN_T_SET(update, 1,   0, 0, 0,   0, 0,   0);
  for (jj=0; jj<(rotnoop ? 3 : 6); jj++) {
    len02 = TEN_T_DOT(igrt[1][jj], d02);
    len24 = TEN_T_DOT(igrt[3][jj], d24);
    correct = (len24 - len02)/2;
    TEN_T_SCALE_INCR(update, correct*scl, igrt[2][jj]);
    if (tpp->verbose) {
      fprintf(stderr, "igrt[1][%u] = %g %g %g   %g %g   %g\n", jj,
              igrt[1][jj][1], igrt[1][jj][2], igrt[1][jj][3],
              igrt[1][jj][4], igrt[1][jj][5], igrt[1][jj][6]);
      fprintf(stderr, "igrt[3][%u] = %g %g %g   %g %g   %g\n", jj,
              igrt[3][jj][1], igrt[3][jj][2], igrt[3][jj][3],
              igrt[3][jj][4], igrt[3][jj][5], igrt[3][jj][6]);
      fprintf(stderr, "(jj=%u) len = %g %g --> (d = %g) "
              "update = %g %g %g     %g %g   %g\n",
              jj, len02, len24,
              TEN_T_DOT(igrt[2][0], update),
              update[1], update[2], update[3],
              update[4], update[5], update[6]);
    }
  }
  if (rotnoop) {
    double avg[7], diff[7], len;
    TEN_T_LERP(avg, 0.5, tt[0], tt[4]);
    TEN_T_SUB(diff, avg, tt[2]);
    for (jj=0; jj<3; jj++) {
      len = TEN_T_DOT(igrt[2][jj], diff);
      TEN_T_SCALE_INCR(diff, -len, igrt[2][jj]);
    }
    TEN_T_SCALE_INCR(update, scl*0.2, diff);  /* HEY: scaling is a hack */
    if (tpp->verbose) {
      fprintf(stderr, "(rotnoop) (d = %g) "
              "update = %g %g %g     %g %g   %g\n",
              TEN_T_DOT(igrt[2][0], update),
              update[1], update[2], update[3],
              update[4], update[5], update[6]);
    }
  }
  /*
  TEN_T_SUB(d02, tt[2], tt[0]);
  TEN_T_SUB(d24, tt[4], tt[2]);
  len02 = TEN_T_DOT(tng, d02);
  len24 = TEN_T_DOT(tng, d24);
  correct = (len24 - len02);
  TEN_T_SCALE_INCR(update, scl*correct, tng);
  */

  if (!TEN_T_EXISTS(update)) {
    sprintf(err, "%s: computed non-existant update (step-size too big?)", me);
    biffAdd(TEN, err); return 1;
  }

  TEN_T_ADD(odata + 7*(2*ii + 0), tt[2], update);

  return 0;
}

void
_tenPathGeodeLoxoIGRT(double *igrt, double *ten, int useK, int rotNoop,
                      double minnorm) {
  /* char me[]="_tenPathGeodeLoxoIGRT"; */
  double eval[3], evec[9];

  if (useK) {
    tenInvariantGradientsK_d(igrt + 7*0, igrt + 7*1, igrt + 7*2, ten, minnorm);
  } else {
    tenInvariantGradientsR_d(igrt + 7*0, igrt + 7*1, igrt + 7*2, ten, minnorm);
  }
  if (rotNoop) {
    /* these shouldn't be used */
    TEN_T_SET(igrt + 7*3, 1, AIR_NAN, AIR_NAN, AIR_NAN,
              AIR_NAN, AIR_NAN, AIR_NAN);
    TEN_T_SET(igrt + 7*4, 1, AIR_NAN, AIR_NAN, AIR_NAN,
              AIR_NAN, AIR_NAN, AIR_NAN);
    TEN_T_SET(igrt + 7*5, 1, AIR_NAN, AIR_NAN, AIR_NAN,
              AIR_NAN, AIR_NAN, AIR_NAN);
  } else {
    tenEigensolve_d(eval, evec, ten);
    tenRotationTangents_d(igrt + 7*3, igrt + 7*4, igrt + 7*5, evec);
  }
  return;
}

/*
** if "doubling" is non-zero, this assumes that the real
** vertices are on the even-numbered indices:
** (0   1   2   3   4)
**  0   2   4   6   8 --> size=9 --> NN=4
**    1   3   5   7
*/
double
tenPathLength(Nrrd *ntt, int doubleVerts, int fancy, int shape) {
  double *tt, len, diff[7], *tenA, *tenB;
  unsigned int ii, NN;

  tt = AIR_CAST(double *, ntt->data);
  if (doubleVerts) {
    NN = AIR_CAST(unsigned int, (ntt->axis[1].size-1)/2);
  } else {
    NN = AIR_CAST(unsigned int, ntt->axis[1].size-1);
  }
  len = 0;
  for (ii=0; ii<NN; ii++) {
    if (doubleVerts) {
      tenA = tt + 7*2*(ii + 1);
      tenB = tt + 7*2*(ii + 0);
    } else {
      tenA = tt + 7*(ii + 1);
      tenB = tt + 7*(ii + 0);
    }
    TEN_T_SUB(diff, tenA, tenB);
    if (fancy) {
      double mean[7], igrt[7*6], dot, incr;
      unsigned int ii, lo, hi;

      TEN_T_LERP(mean, 0.5, tenA, tenB);
      _tenPathGeodeLoxoIGRT(igrt, mean, AIR_FALSE, AIR_FALSE, 0.0);
      if (shape) {
        lo = 0;
        hi = 2;
      } else {
        lo = 3;
        hi = 5;
      }
      incr = 0;
      for (ii=lo; ii<=hi; ii++) {
        dot = TEN_T_DOT(igrt + 7*ii, diff);
        incr += dot*dot;
      }
      len += sqrt(incr);
    } else {
      len += TEN_T_NORM(diff);
    }
  }
  return len;
}

double
_tenPathSpacingEqualize(Nrrd *nout, Nrrd *nin) {
  /* char me[]="_tenPathSpacingEqualize"; */
  double *in, *out, len, diff[7],
    lenTotal,  /* total length of input */
    lenStep,   /* correct length on input polyline between output vertices */
    lenIn,     /* length along input processed so far */
    lenHere,   /* length of segment associated with current input index */
    lenRmn,    /* length along past input segments as yet unmapped to output */
    *tenHere, *tenNext;
  unsigned int idxIn, idxOut, NN;

  in = AIR_CAST(double *, nin->data);
  out = AIR_CAST(double *, nout->data);
  NN = (nin->axis[1].size-1)/2;
  lenTotal = tenPathLength(nin, AIR_TRUE, AIR_FALSE, AIR_FALSE);
  lenStep = lenTotal/NN;
  /*
  fprintf(stderr, "!%s: lenTotal/NN = %g/%u = %g = lenStep\n", me,
          lenTotal, NN, lenStep);
  */
  TEN_T_COPY(out + 7*2*(0 + 0), in + 7*2*(0 + 0));
  lenIn = lenRmn = 0;
  idxOut = 1;
  for (idxIn=0; idxIn<NN; idxIn++) {
    tenNext = in + 7*2*(idxIn + 1);
    tenHere = in + 7*2*(idxIn + 0);
    TEN_T_SUB(diff, tenNext, tenHere);
    lenHere = TEN_T_NORM(diff);
    /*
    fprintf(stderr, "!%s(%u): %g + %g >(%s)= %g\n", me, idxIn,
            lenRmn, lenHere, 
            (lenRmn + lenHere >= lenStep ? "yes" : "no"),
            lenStep);
    */
    if (lenRmn + lenHere >= lenStep) {
      len = lenRmn + lenHere;
      while (len > lenStep) {
        len -= lenStep;
        /*
        fprintf(stderr, "!%s(%u): len = %g -> %g\n", me, idxIn,
                len + lenStep, len);
        */
        TEN_T_AFFINE(out + 7*(2*idxOut + 0),
                     lenHere, len, 0, tenHere, tenNext);
        /*
        fprintf(stderr, "!%s(%u): out[%u] ~ %g\n", me, idxIn, idxOut,
                AIR_AFFINE(lenHere, len, 0, 0, 1));
        */
        idxOut++;
      }
      lenRmn = len;
    } else {
      lenRmn += lenHere;
      /*
      fprintf(stderr, "!%s(%u):   (==> lenRmn = %g -> %g)\n", me, idxIn,
              lenRmn - lenHere, lenRmn);
      */
    }
    /* now lenRmn < lenStep */
    lenIn += lenHere;
  }
  /* copy very last one in case we didn't get to it somehow */
  TEN_T_COPY(out + 7*2*(NN + 0), in + 7*2*(NN + 0));

  /* fill in vertex mid-points */
  for (idxOut=0; idxOut<NN; idxOut++) {
    TEN_T_LERP(out + 7*(2*idxOut + 1), 
               0.5, out + 7*(2*idxOut + 0), out + 7*(2*idxOut + 2));
  }
  return lenTotal;
}

int
_tenPathGeodeLoxoPolyLine(Nrrd *ngeod, unsigned int *numIter,
                          double tenA[7], double tenB[7],
                          unsigned int NN, int useK, int rotnoop,
                          tenPathParm *tpp) {
  char me[]="_tenPathGeodeLoxoPolyLine", err[BIFF_STRLEN];
  Nrrd *nigrt, *ntt, *nss, *nsub;
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
  nss = nrrdNew();
  airMopAdd(mop, nss, (airMopper)nrrdNuke, airMopAlways);
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
  if (NN > 14 && tpp->enableRecurse) {
    unsigned int subIter;
    int E;
    NrrdResampleContext *rsmc;
    double kparm[3] = {1.0, 0.0, 0.5};
    /* recurse and find geodesic with smaller number of vertices */
    if (_tenPathGeodeLoxoPolyLine(nsub, &subIter, tenA, tenB,
                                  NN/2, useK, rotnoop, tpp)) {
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
    if (!E) E |= nrrdResampleKernelSet(rsmc, 1, nrrdKernelTent, kparm);
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
  for (ii=0; ii<=2*NN; ii++) {
    _tenPathGeodeLoxoIGRT(igrt + 7*6*ii, tt + 7*ii, useK, rotnoop,
                          tpp->minNorm);
  }
  nrrdCopy(nss, ntt);
  
  newlen = tenPathLength(ntt, AIR_TRUE, AIR_FALSE, AIR_FALSE);
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
    if (tpp->verbose) {
      fprintf(stderr, "%s: ======= iter = %u (NN=%u)\n", me, *numIter, NN);
    }
    for (ii=lo; ii!=hi; ii+=dd) {
      double sclHack;
      sclHack = ii*4.0/NN - ii*ii*4.0/NN/NN;
      if (_tenPathGeodeLoxoRelaxOne(nss, ntt, nigrt, ii, rotnoop,
                                    sclHack*tpp->convStep, tpp)) {
        sprintf(err, "%s: problem on vert %u, iter %u\n", me, ii, *numIter);
        biffAdd(TEN, err); return 1;
      }
    }
    newlen = _tenPathSpacingEqualize(ntt, nss);
    /* try doing this less often */
    for (ii=0; ii<=2*NN; ii++) {
      _tenPathGeodeLoxoIGRT(igrt + 7*6*ii, tt + 7*ii, useK, rotnoop,
                            tpp->minNorm);
    }
    *numIter += 1;
  } while ((0 == tpp->maxIter || *numIter < tpp->maxIter)
           && 2*AIR_ABS(newlen - len)/(newlen + len) > tpp->convEps);

  /* copy final result to output */
  for (ii=0; ii<=NN; ii++) {
    TEN_T_COPY(geod + 7*ii, tt + 7*2*ii);
  }
  /* values from outer-most recursion will stick */
  tpp->numIter = *numIter;
  tpp->convFinal = 2*AIR_ABS(newlen - len)/(newlen + len);

  airMopOkay(mop);
  return 0;
}

int
tenPathInterpTwoDiscrete(Nrrd *nout, 
                         double tenA[7], double tenB[7],
                         int ptype, unsigned int num,
                         tenPathParm *_tpp) {
  char me[]="tenPathInterpTwoDiscrete", err[BIFF_STRLEN];
  double *out;
  unsigned int ii;
  airArray *mop;
  tenPathParm *tpp;

  if (!nout) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (airEnumValCheck(tenPathType, ptype)) {
    sprintf(err, "%s: path type %d not a valid %s", me, ptype, 
            tenPathType->name);
    biffAdd(TEN, err); return 1;
  }

  mop = airMopNew();
  if (_tpp) {
    tpp = _tpp;
  } else {
    tpp = tenPathParmNew();
    airMopAdd(mop, tpp, (airMopper)tenPathParmNix, airMopAlways);
  }
  if (!( num >= 2 )) {
    sprintf(err, "%s: need num >= 2 (not %u)", me, num);
    biffAdd(TEN, err); return 1;
  }
  if (nrrdMaybeAlloc_va(nout, nrrdTypeDouble, 2, 
                        AIR_CAST(size_t, 7),
                        AIR_CAST(size_t, num))) {
    sprintf(err, "%s: trouble allocating output", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  out = AIR_CAST(double *, nout->data);

  if (ptype == tenPathTypeLerp
      || ptype == tenPathTypeLogLerp
      || ptype == tenPathTypeAffineInvariant
      || ptype == tenPathTypeWang) {
    for (ii=0; ii<num; ii++) {
      double *oten;
      oten = out + 7*ii;
      /* yes, this is often doing a lot of needless recomputations. */
      tenPathInterpTwo(out + 7*ii, tenA, tenB, 
                       ptype, (double)ii/(num-1), tpp);
    }
  } else if (ptype == tenPathTypeGeodeLoxoK
             || ptype == tenPathTypeGeodeLoxoR
             || ptype == tenPathTypeLoxoK
             || ptype == tenPathTypeLoxoR) {
    /* we have slow iterative code for these */
    unsigned int numIter;
    int useK, rotnoop;
    
    useK = (tenPathTypeGeodeLoxoK == ptype
            || tenPathTypeLoxoK == ptype);
    rotnoop = (tenPathTypeGeodeLoxoK == ptype
               || tenPathTypeGeodeLoxoR == ptype);
    fprintf(stderr, "!%s: useK = %d, rotnoop = %d\n", me, useK, rotnoop);
    if (_tenPathGeodeLoxoPolyLine(nout, &numIter,
                                  tenA, tenB,
                                  num, useK, rotnoop, tpp)) {
      sprintf(err, "%s: trouble finding path", me);
      biffAdd(TEN, err); return 1;
    }
  } else {
    sprintf(err, "%s: sorry, interp for path %s not implemented", me,
            airEnumStr(tenPathType, ptype));
    biffAdd(TEN, err); return 1;
  }

  return 0;
}

double
tenPathDistance(double tenA[7], double tenB[7],
                int ptype, tenPathParm *_tpp) {
  char me[]="tenPathDistanceTwo", *err;
  tenPathParm *tpp;
  airArray *mop;
  double ret, diff[7], logA[7], logB[7], invA[7], det, siA[7],
    mat1[9], mat2[9], mat3[9], logDiff[7];
  Nrrd *npath;
  
  if (!( tenA && tenB && !airEnumValCheck(tenPathType, ptype) )) {
    return AIR_NAN;
  }

  mop = airMopNew();
  switch (ptype) {
  case tenPathTypeLerp:
    TEN_T_SUB(diff, tenA, tenB);
    ret = TEN_T_NORM(diff);
    break;
  case tenPathTypeLogLerp:
    tenLogSingle_d(logA, tenA);
    tenLogSingle_d(logB, tenB);
    TEN_T_SUB(diff, logA, logB);
    ret = TEN_T_NORM(diff);
    break;
  case tenPathTypeAffineInvariant:
    TEN_T_INV(invA, tenA, det);
    tenSqrtSingle_d(siA, invA);
    TEN_T2M(mat1, tenB);
    TEN_T2M(mat2, siA);
    ell_3m_mul_d(mat3, mat1, mat2);
    ell_3m_mul_d(mat1, mat2, mat3);
    TEN_M2T(diff, mat1);
    tenLogSingle_d(logDiff, diff);
    ret = TEN_T_NORM(logDiff);
    break;
  case tenPathTypeGeodeLoxoK:
  case tenPathTypeGeodeLoxoR:
  case tenPathTypeLoxoK:
  case tenPathTypeLoxoR:
    npath = nrrdNew();
    airMopAdd(mop, npath, (airMopper)nrrdNuke, airMopAlways);
    if (_tpp) {
      tpp = _tpp;
    } else {
      tpp = tenPathParmNew();
      airMopAdd(mop, tpp, (airMopper)tenPathParmNix, airMopAlways);
    }
    if (tenPathInterpTwoDiscrete(npath, tenA, tenB, ptype,
                                 tpp->numSteps, tpp)) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble computing path:\n%s\n", me, err);
      airMopError(mop); return AIR_NAN;
    }
    ret = tenPathLength(npath, AIR_FALSE, AIR_FALSE, AIR_FALSE);
    if (tpp->lengthFancy) {
      tpp->lengthShape = tenPathLength(npath, AIR_FALSE, AIR_TRUE, AIR_TRUE);
      tpp->lengthOrient = tenPathLength(npath, AIR_FALSE, AIR_TRUE, AIR_FALSE);
    }
    break;
  case tenPathTypeWang:
  default:
    fprintf(stderr, "%s: unimplemented %s %d!!!!\n", me,
            tenPathType->name, ptype);
    ret = AIR_NAN;
    break;
  }

  airMopOkay(mop);
  return ret;
}

