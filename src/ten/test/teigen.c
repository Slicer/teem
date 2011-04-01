/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2011, 2010, 2009, University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
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

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "../ten.h"

char *info = ("tests tenEigensolve_d and new stand-alone function.");

#define ROOT_TRIPLE 2           /* ell_cubic_root_triple */
#define ROOT_SINGLE_DOUBLE 3    /* ell_cubic_root_single_double */
#define ROOT_THREE 4            /* ell_cubic_root_three */

#define ABS(a) (((a) > 0.0f ? (a) : -(a)))
#define VEC_SET(v, a, b, c) \
  ((v)[0] = (a), (v)[1] = (b), (v)[2] = (c))
#define VEC_DOT(v1, v2) \
  ((v1)[0]*(v2)[0] + (v1)[1]*(v2)[1] + (v1)[2]*(v2)[2])
#define VEC_CROSS(v3, v1, v2) \
  ((v3)[0] = (v1)[1]*(v2)[2] - (v1)[2]*(v2)[1], \
   (v3)[1] = (v1)[2]*(v2)[0] - (v1)[0]*(v2)[2], \
   (v3)[2] = (v1)[0]*(v2)[1] - (v1)[1]*(v2)[0])
#define VEC_ADD(v1, v2)  \
  ((v1)[0] += (v2)[0], \
   (v1)[1] += (v2)[1], \
   (v1)[2] += (v2)[2])
#define VEC_SUB(v1, v2)  \
  ((v1)[0] -= (v2)[0], \
   (v1)[1] -= (v2)[1], \
   (v1)[2] -= (v2)[2])
#define VEC_SCL(v1, s)  \
  ((v1)[0] *= (s),	\
   (v1)[1] *= (s),	\
   (v1)[2] *= (s))
#define VEC_LEN(v) (sqrt(VEC_DOT(v,v)))
#define VEC_NORM(v, len) ((len) = VEC_LEN(v), VEC_SCL(v, 1.0/len))
#define VEC_SCL_SUB(v1, s, v2) \
  ((v1)[0] -= (s)*(v2)[0],      \
   (v1)[1] -= (s)*(v2)[1], \
   (v1)[2] -= (s)*(v2)[2])
#define VEC_COPY(v1, v2)  \
  ((v1)[0] = (v2)[0], \
   (v1)[1] = (v2)[1], \
   (v1)[2] = (v2)[2])

/*
** All the three given vectors span only a 2D space, and this finds
** the normal to that plane.  Simply sums up all the pair-wise
** cross-products to get a good estimate.  Trick is getting the cross
** products to line up before summing.
*/
void
nullspace1(double ret[3], 
	   const double r0[3], const double r1[3], const double r2[3]) {
  double crs[3];
  
  /* ret = r0 x r1 */
  VEC_CROSS(ret, r0, r1);
  /* crs = r1 x r2 */
  VEC_CROSS(crs, r1, r2);
  /* ret += crs or ret -= crs; whichever makes ret longer */
  if (VEC_DOT(ret, crs) > 0) {
    VEC_ADD(ret, crs);
  } else {
    VEC_SUB(ret, crs);
  }
  /* crs = r0 x r2 */
  VEC_CROSS(crs, r0, r2);
  /* ret += crs or ret -= crs; whichever makes ret longer */
  if (VEC_DOT(ret, crs) > 0) {
    VEC_ADD(ret, crs);
  } else {
    VEC_SUB(ret, crs);
  }

  return;
}

/*
** All vectors are in the same 1D space, we have to find two 
** mutually vectors perpendicular to that span
*/
void
nullspace2(double reta[3], double retb[3],
	   const double r0[3], const double r1[3], const double r2[3]) {
  double sqr[3], sum[3];
  int idx;

  VEC_COPY(sum, r0);
  if (VEC_DOT(sum, r1) > 0) {
    VEC_ADD(sum, r1);
  } else {
    VEC_SUB(sum, r1);
  }
  if (VEC_DOT(sum, r2) > 0) {
    VEC_ADD(sum, r2);
  } else {
    VEC_SUB(sum, r2);
  }
  /* find largest component, to get most stable expression for a 
     perpendicular vector */
  sqr[0] = sum[0]*sum[0];
  sqr[1] = sum[1]*sum[1];
  sqr[2] = sum[2]*sum[2];
  idx = 0;
  if (sqr[0] < sqr[1])
    idx = 1;
  if (sqr[idx] < sqr[2])
    idx = 2;
  /* reta will be perpendicular to sum */
  if (0 == idx) {
    VEC_SET(reta, sum[1] - sum[2], -sum[0], sum[0]);
  } else if (1 == idx) {
    VEC_SET(reta, -sum[1], sum[0] - sum[2], sum[1]);
  } else {
    VEC_SET(reta, -sum[2], sum[2], sum[0] - sum[1]);
  }
  /* and now retb will be perpendicular to both reta and sum */
  VEC_CROSS(retb, reta, sum);
  return;
}

/*
** Eigensolver for symmetric 3x3 matrix:
**
**  M00  M01  M02
**  M01  M11  M12
**  M02  M12  M22
**
** Must be passed eval[3] vector, and will compute eigenvalues
** only if evec[9] is non-NULL.  Computed eigenvectors are at evec+0,
** evec+3, and evec+6.
**
** Return value indicates something about the eigenvalue solution to
** the cubic characteristic equation; see ROOT_ #defines above
**
** Relies on the VEC_* macros above, as well as functions math functions
** atan2(), sin(), cos(), sqrt(), and airCbrt(), defined as:

  double
  airCbrt(double v) {
  #if defined(_WIN32) || defined(__STRICT_ANSI__)
    return (v < 0.0 ? -pow(-v,1.0/3.0) : pow(v,1.0/3.0));
  #else
    return cbrt(v);
  #endif
  }

*/
int
evals(double eval[3],
      double M00, double M01, double M02, 
      double M11, double M12, 
      double M22) {
  double mean, norm, rnorm, Q, R, QQQ, D, theta, M[6];
  double epsilon = 1.0E-16;
  int roots;

#include "teigen-evals-A.c"

#include "teigen-evals-B.c"

  return roots;
}

int
evals_evecs(double eval[3], double evec[9],
	    double M00, double M01, double M02, 
	    double M11, double M12, 
	    double M22) {
  double mean, norm, rnorm, Q, R, QQQ, D, theta, M[6];
  double r0[3], r1[3], r2[3], crs[3], len, dot;
  double epsilon = 1.0E-16;
  int roots;

#include "teigen-evals-A.c"

  VEC_SET(r0, 0.0, M[1], M[2]);
  VEC_SET(r1, M[1], 0.0, M[4]);
  VEC_SET(r2, M[2], M[4], 0.0);
  if (ROOT_THREE == roots) {
    r0[0] = M[0] - eval[0]; r1[1] = M[3] - eval[0]; r2[2] = M[5] - eval[0];
    nullspace1(evec+0, r0, r1, r2);
    r0[0] = M[0] - eval[1]; r1[1] = M[3] - eval[1]; r2[2] = M[5] - eval[1];
    nullspace1(evec+3, r0, r1, r2);
    r0[0] = M[0] - eval[2]; r1[1] = M[3] - eval[2]; r2[2] = M[5] - eval[2];
    nullspace1(evec+6, r0, r1, r2);
  } else if (ROOT_SINGLE_DOUBLE == roots) {
    if (eval[0] > eval[1]) {
      /* one big (eval[0]) , two small (eval[1,2]) : more like a cigar */
      r0[0] = M[0] - eval[0]; r1[1] = M[3] - eval[0]; r2[2] = M[5] - eval[0];
      nullspace1(evec+0, r0, r1, r2);
      r0[0] = M[0] - eval[1]; r1[1] = M[3] - eval[1]; r2[2] = M[5] - eval[1];
      nullspace2(evec+3, evec+6, r0, r1, r2);
    }
    else {
      /* two big (eval[0,1]), one small (eval[2]): more like a pancake */
      r0[0] = M[0] - eval[0]; r1[1] = M[3] - eval[0]; r2[2] = M[5] - eval[0];
      nullspace2(evec+0, evec+3, r0, r1, r2);
      r0[0] = M[0] - eval[2]; r1[1] = M[3] - eval[2]; r2[2] = M[5] - eval[2];
      nullspace1(evec+6, r0, r1, r2);
    }
  } else {
    /* ROOT_TRIPLE == roots; use any basis for eigenvectors */
    VEC_SET(evec+0, 1, 0, 0);
    VEC_SET(evec+3, 0, 1, 0);
    VEC_SET(evec+6, 0, 0, 1);
  }
  /* we always make sure its really orthonormal */
  if (
  /* HEY: should process evecs in reverse order if |eval2| > |eval0| */
  VEC_NORM(evec+0, len);
  dot = VEC_DOT(evec+0, evec+3); VEC_SCL_SUB(evec+3, dot, evec+0);
  VEC_NORM(evec+3, len);
  dot = VEC_DOT(evec+0, evec+6); VEC_SCL_SUB(evec+6, dot, evec+0);
  dot = VEC_DOT(evec+3, evec+6); VEC_SCL_SUB(evec+6, dot, evec+3);
  VEC_NORM(evec+6, len);
  /* to be nice, make it right-handed */
  VEC_CROSS(crs, evec+0, evec+3);
  if (0 > VEC_DOT(crs, evec+6)) {
    VEC_SCL(evec+6, -1);
  }

#include "teigen-evals-B.c"

  return roots;
}


void
testeigen(double tt[7], double eval[3], double evec[9]) {
  double mat[9], dot[3], cross[3];
  unsigned int ii;

  TEN_T2M(mat, tt);
  printf("evals %g %g %g\n", eval[0], eval[1], eval[2]);
  printf("evec0 (%g) %g %g %g\n",
         ELL_3V_LEN(evec + 0), evec[0], evec[1], evec[2]);
  printf("evec1 (%g) %g %g %g\n",
         ELL_3V_LEN(evec + 3), evec[3], evec[4], evec[5]);
  printf("evec2 (%g) %g %g %g\n",
         ELL_3V_LEN(evec + 6), evec[6], evec[7], evec[8]);
  printf("Mv - lv: (len) X Y Z (should be ~zeros)\n");
  for (ii=0; ii<3; ii++) {
    double uu[3], vv[3], dd[3];
    ELL_3MV_MUL(uu, mat, evec + 3*ii);
    ELL_3V_SCALE(vv, eval[ii], evec + 3*ii);
    ELL_3V_SUB(dd, uu, vv);
    printf("%d: (%g) %g %g %g\n", ii, ELL_3V_LEN(dd), dd[0], dd[1], dd[2]);
  }
  dot[0] = ELL_3V_DOT(evec + 0, evec + 3);
  dot[1] = ELL_3V_DOT(evec + 0, evec + 6);
  dot[2] = ELL_3V_DOT(evec + 3, evec + 6);
  printf("pairwise dots: (%g) %g %g %g\n",
         ELL_3V_LEN(dot), dot[0], dot[1], dot[2]);
  ELL_3V_CROSS(cross, evec+0, evec+3);
  printf("right-handed: %g\n", ELL_3V_DOT(evec+6, cross));
  return;
}

int
main(int argc, char *argv[]) {
  char *me;
  hestOpt *hopt=NULL;
  airArray *mop;

  double _tt[6], tt[7], ss, pp[3], qq[4], rot[9], mat1[9], mat2[9], tmp,
    evalA[3], evecA[9], evalB[3], evecB[9];
  int roots;

  mop = airMopNew();
  me = argv[0];
  hestOptAdd(&hopt, NULL, "m00 m01 m02 m11 m12 m22",
             airTypeDouble, 6, 6, _tt, NULL, "symmtric matrix coeffs");
  hestOptAdd(&hopt, "p", "vec", airTypeDouble, 3, 3, pp, "0 0 0",
             "rotation as P vector");
  hestOptAdd(&hopt, "s", "scl", airTypeDouble, 1, 1, &ss, "1.0",
             "scaling");
  hestParseOrDie(hopt, argc-1, argv+1, NULL,
                 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  ELL_6V_COPY(tt + 1, _tt);
  tt[0] = 1.0;
  TEN_T_SCALE(tt, ss, tt);
    
  ELL_4V_SET(qq, 1, pp[0], pp[1], pp[2]);
  ELL_4V_NORM(qq, qq, tmp);
  ell_q_to_3m_d(rot, qq);
  printf("%s: rot\n", me);
  printf("  %g %g %g\n", rot[0], rot[1], rot[2]);
  printf("  %g %g %g\n", rot[3], rot[4], rot[5]);
  printf("  %g %g %g\n", rot[6], rot[7], rot[8]);
    
  TEN_T2M(mat1, tt);
  ell_3m_mul_d(mat2, rot, mat1);
  ELL_3M_TRANSPOSE_IP(rot, tmp);
  ell_3m_mul_d(mat1, mat2, rot);
  TEN_M2T(tt, mat1);
    
  printf("input matrix = \n %g %g %g\n %g %g\n %g\n",
          tt[1], tt[2], tt[3], tt[4], tt[5], tt[6]);

  printf("================== tenEigensolve_d ==================\n");
  roots = tenEigensolve_d(evalA, evecA, tt);
  printf("%s roots\n", airEnumStr(ell_cubic_root, roots));
  testeigen(tt, evalA, evecA);

  printf("================== new eigensolve ==================\n");
  roots = eigensolve(evalB, evecB,
                     tt[1], tt[2], tt[3], tt[4], tt[5], tt[6]);
  printf("%s roots\n", airEnumStr(ell_cubic_root, roots));
  testeigen(tt, evalB, evecB);

  airMopOkay(mop);
  return 0;
}
