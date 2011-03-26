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

#define ROOT_SINGLE 1           /* ell_cubic_root_single */
#define ROOT_TRIPLE 2           /* ell_cubic_root_triple */
#define ROOT_SINGLE_DOUBLE 3    /* ell_cubic_root_single_double */
#define ROOT_THREE 4            /* ell_cubic_root_three */

/*
** makes sure that v+3*2 has a positive dot product with
** cross product of v+3*0 and v+3*1
*/
void
make_right_handed_d(double v[9]) {
  double x[3];
  
  ELL_3V_CROSS(x, v+3*0, v+3*1);
  if (0 > ELL_3V_DOT(x, v+3*2)) {
    ELL_3V_SCALE(v+3*2, -1, v+3*2);
  }
}

/*
** Stand-alone eigensolve for symmetric 3x3 matrix:
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
** Relies on acos(), cos(), sqrt(), and airCbrt(), defined as:

  double
  airCbrt(double v) {
  #if defined(_WIN32) || defined(__STRICT_ANSI__)
    return (v < 0.0 ? -pow(-v,1.0/3.0) : pow(v,1.0/3.0));
  #else
    return cbrt(v);
  #endif
  }

** Also uses AIR_NAN (the compile-time quiet NaN) to fill in info
** that can't be known because there mysteriously was only one root
** instead of three; a run-time error might be more appropriate.
*/
int
eigensolve(double eval[3], double evec[9],
           double M00, double M01, double M02, 
           double M11, double M12, 
           double M22) {
  double mean, norm, rnorm, B, C, Q, R, QQQ, D, sqrt_D, u, v, theta, t;
  double epsilon = 1.0E-11;
  int roots;

  /*
  ** subtract out the eigenvalue mean (will add back to evals later);
  ** helps with numerical stability
  */
  mean = (M00 + M11 + M22)/3.0;
  M00 -= mean;
  M11 -= mean;
  M22 -= mean;
  /* 
  ** divide out frobenius (L2) norm (will multiply later);
  ** also seems to help with numerical stability
  */
  norm = sqrt(M00*M00 + 2*M01*M01 + 2*M02*M02 +
              M11*M11 + 2*M12*M12 +
              M22*M22);
  rnorm = norm ? 1.0/norm : 1.0;
  M00 *= rnorm;
  M01 *= rnorm;
  M02 *= rnorm;
  M11 *= rnorm;
  M12 *= rnorm;
  M22 *= rnorm;
  
  /*
  ** create coefficients of cubic characteristic polynomial in x:
  ** det(x*I - M) =  x^3 + 0*x^2 + B*x + C.
  ** x^2 term is zero because of subtracting out eval mean above
  */
  B = M00*M11 + M00*M22 + M11*M22 - M01*M01 - M02*M02 - M12*M12;
  C = M02*M02*M11 - 2*M01*M02*M12 + M01*M01*M22 + M00*(M12*M12 - M11*M22);
  /* solve cubic */
  Q = -B/3.0;
  R = -C/2.0;
  QQQ = Q*Q*Q;
  D = R*R - QQQ;
  if (D < -epsilon) {
    /* three distinct roots- this is the most common case */
    theta = acos(R/sqrt(QQQ))/3.0;
    t = 2*sqrt(Q);
    /* yes, these should be sorted, since the definition of acos says
       that it returns values in in [0, pi] (HEY: CHECK THIS for OpenCL) */
    eval[0] = t*cos(theta);
    eval[1] = t*cos(theta - 2*AIR_PI/3.0);
    eval[2] = t*cos(theta + 2*AIR_PI/3.0);
    roots = ROOT_THREE;
  } else if (D < epsilon) {
    /* else D is in the interval [-epsilon, +epsilon] */
    if (R < -epsilon || epsilon < R) {
      /* one double root and one single root */
      u = airCbrt(R); /* cube root function */
      if (u > 0) {
        eval[0] = 2*u;
        eval[1] = -u;
        eval[2] = -u;
      } else {
        eval[0] = -u;
        eval[1] = -u;
        eval[2] = 2*u;
      }
      roots = ROOT_SINGLE_DOUBLE;
    } else {
      /* a triple root! */
      eval[0] = eval[1] = eval[2] = 0.0;
      roots = ROOT_TRIPLE;
    }
  } else {
    /* D >= epsilon; apparently only one root; this should not happen !! */
    sqrt_D = sqrt(D);
    eval[0] = airCbrt(sqrt_D + R) - airCbrt(sqrt_D - R);
    eval[1] = eval[2] = AIR_NAN;
    roots = ROOT_SINGLE;
  }

  /* find eigenvectors, if requested */
  if (evec) {
    /* HEY: this part still needs to be processed in order to be 
       make it as self-contained as possible */
    double n[9], m[9], e0, e1, e2;
    double col[9];
    ELL_3M_SET(m, 
               M00, M01, M02,
               M01, M11, M12,
               M02, M12, M22);
    ELL_3M_COPY(n, m);
    e0 = eval[0];
    e1 = eval[1];
    e2 = eval[2];
    switch (roots) {
    case ROOT_THREE:
      ELL_3M_DIAG_SET(n, m[0]-e0, m[4]-e0, m[8]-e0);
      ell_3m_1d_nullspace_d(evec+0, n);
      ELL_3M_DIAG_SET(n, m[0]-e1, m[4]-e1, m[8]-e1);
      ell_3m_1d_nullspace_d(evec+3, n);
      ELL_3M_DIAG_SET(n, m[0]-e2, m[4]-e2, m[8]-e2);
      ell_3m_1d_nullspace_d(evec+6, n);
      break;
    case ROOT_SINGLE_DOUBLE:
      if (e0 > e1) {
        /* one big (e0) , two small (e1, e2) : more like a cigar */
        ELL_3M_DIAG_SET(n, m[0]-e0, m[4]-e0, m[8]-e0);
        ell_3m_1d_nullspace_d(evec+0, n);
        ELL_3M_DIAG_SET(n, m[0]-e1, m[4]-e1, m[8]-e1);
        ell_3m_2d_nullspace_d(evec+3, evec+6, n);
      }
      else {
        /* two big (e0, e1), one small (e2): more like a pancake */
        ELL_3M_DIAG_SET(n, m[0]-e0, m[4]-e0, m[8]-e0);
        ell_3m_2d_nullspace_d(evec+0, evec+3, n);
        ELL_3M_DIAG_SET(n, m[0]-e2, m[4]-e2, m[8]-e2);
        ell_3m_1d_nullspace_d(evec+6, n);
      }
      break;
    case ROOT_TRIPLE:
      /* use any basis as the eigenvectors */
      ELL_3V_SET(evec+0, 1, 0, 0);
      ELL_3V_SET(evec+3, 0, 1, 0);
      ELL_3V_SET(evec+6, 0, 0, 1);
      break;
    case ROOT_SINGLE:
    default:
      /* only one real root, shouldn't happen !! */
      ELL_3M_DIAG_SET(n, m[0]-e0, m[4]-e0, m[8]-e0);
      ell_3m_1d_nullspace_d(evec+0, n);
      ELL_3V_SET(evec+3, AIR_NAN, AIR_NAN, AIR_NAN);
      ELL_3V_SET(evec+6, AIR_NAN, AIR_NAN, AIR_NAN);
      break;
    }
  }

  /* multiply back in the eigenvalue L2 norm */
  eval[0] /= rnorm;
  eval[1] /= rnorm;
  eval[2] /= rnorm;
  /* add back in the eigenvalue mean */
  eval[0] += mean;
  eval[1] += mean;
  eval[2] += mean;
  return roots;
}

void
testeigen(double tt[7], double eval[3], double evec[9]) {
  double mat[9], dot[3];
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
