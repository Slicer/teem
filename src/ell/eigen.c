/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "ell.h"

/* lop A
  fprintf(stderr, "_ellAlign3: ----------\n");
  fprintf(stderr, "_ellAlign3: v0 = %g %g %g\n", (v+0)[0], (v+0)[1], (v+0)[2]);
  fprintf(stderr, "_ellAlign3: v3 = %g %g %g\n", (v+3)[0], (v+3)[1], (v+3)[2]);
  fprintf(stderr, "_ellAlign3: v6 = %g %g %g\n", (v+6)[0], (v+6)[1], (v+6)[2]);
  fprintf(stderr, "_ellAlign3: d = %g %g %g -> %d %d %d\n",
	  d0, d1, d2, Mi, ai, bi);
  fprintf(stderr, "_ellAlign3:  pre dot signs (03, 06, 36): %d %d %d\n",
	  airSgn(ELL_3V_DOT(v+0, v+3)),
	  airSgn(ELL_3V_DOT(v+0, v+6)),
	  airSgn(ELL_3V_DOT(v+3, v+6)));
  */

/* lop B
  fprintf(stderr, "_ellAlign3: v0 = %g %g %g\n", (v+0)[0], (v+0)[1], (v+0)[2]);
  fprintf(stderr, "_ellAlign3: v3 = %g %g %g\n", (v+3)[0], (v+3)[1], (v+3)[2]);
  fprintf(stderr, "_ellAlign3: v6 = %g %g %g\n", (v+6)[0], (v+6)[1], (v+6)[2]);
  fprintf(stderr, "_ellAlign3:  post dot signs %d %d %d\n",
	  airSgn(ELL_3V_DOT(v+0, v+3)),
	  airSgn(ELL_3V_DOT(v+0, v+6)),
	  airSgn(ELL_3V_DOT(v+3, v+6)));
  if (airSgn(ELL_3V_DOT(v+0, v+3)) < 0
      || airSgn(ELL_3V_DOT(v+0, v+6)) < 0
      || airSgn(ELL_3V_DOT(v+3, v+6)) < 0) {
    exit(1);
  }
  */

void
_ellAlign3(double v[9]) {
  double d0, d1, d2;
  int Mi, ai, bi;
  
  d0 = ELL_3V_DOT(v+0, v+0);
  d1 = ELL_3V_DOT(v+3, v+3);
  d2 = ELL_3V_DOT(v+6, v+6);
  Mi = ELL_MAX3_IDX(d0, d1, d2);
  ai = (Mi + 1) % 3;
  bi = (Mi + 2) % 3;
  /* lop A */
  if (ELL_3V_DOT(v+3*Mi, v+3*ai) < 0) {
    ELL_3V_SCALE(v+3*ai, -1, v+3*ai);
  }
  if (ELL_3V_DOT(v+3*Mi, v+3*bi) < 0) {
    ELL_3V_SCALE(v+3*bi, -1, v+3*bi);
  }
  /* lob B */
  /* we can't guarantee that dot(v+3*ai,v+3*bi) > 0 ... */
}

void
_ellFixerUpper(double v[9]) {
  double x[3];
  
  ELL_3V_CROSS(x, v+3*0, v+3*1);
  if (0 > ELL_3V_DOT(x, v+3*2)) {
    ELL_3V_SCALE(v+3*2, -1, v+3*2);
  }
}

/* lop A
  fprintf(stderr, "===  pre ===\n");
  fprintf(stderr, "crosses:  %g %g %g\n", (t+0)[0], (t+0)[1], (t+0)[2]);
  fprintf(stderr, "          %g %g %g\n", (t+3)[0], (t+3)[1], (t+3)[2]);
  fprintf(stderr, "          %g %g %g\n", (t+6)[0], (t+6)[1], (t+6)[2]);
  fprintf(stderr, "cross dots:  %g %g %g\n",
	  ELL_3V_DOT(t+0, t+3), ELL_3V_DOT(t+0, t+6), ELL_3V_DOT(t+3, t+6));
*/

/* lop B
  fprintf(stderr, "=== post ===\n");
  fprintf(stderr, "crosses:  %g %g %g\n", (t+0)[0], (t+0)[1], (t+0)[2]);
  fprintf(stderr, "          %g %g %g\n", (t+3)[0], (t+3)[1], (t+3)[2]);
  fprintf(stderr, "          %g %g %g\n", (t+6)[0], (t+6)[1], (t+6)[2]);
  fprintf(stderr, "cross dots:  %g %g %g\n",
	  ELL_3V_DOT(t+0, t+3), ELL_3V_DOT(t+0, t+6), ELL_3V_DOT(t+3, t+6));
*/

/*
******** ell3mNullspace1()
**
** the given matrix is assumed to have a nullspace of dimension one.
** The COLUMNS of the matrix are n+0, n+3, n+6.  A normalized vector
** which spans the nullspace is put into ans.
**
** The given nullspace matrix is NOT modified.  
**
** This does NOT use biff
*/
void
ell3mNullspace1(double ans[3], double n[9]) {
  double t[9], norm;
  
  /* find the three cross-products of pairs of column vectors of n */
  ELL_3V_CROSS(t+0, n+0, n+3);
  ELL_3V_CROSS(t+3, n+0, n+6);
  ELL_3V_CROSS(t+6, n+3, n+6);
  /* lop A */
  _ellAlign3(t);
  /* lop B */
  /* add them up (longer, hence more accurate, should dominate) */
  ELL_3V_ADD3(ans, t+0, t+3, t+6);

  /* normalize */
  ELL_3V_NORM(ans, ans, norm);

  return;
}

/*
******** ell3mNullspace2()
**
** the given matrix is assumed to have a nullspace of dimension two.
** The COLUMNS of the matrix are n+0, n+3, n+6
**
** The given nullspace matrix is NOT modified 
**
** This does NOT use biff
*/
void
ell3mNullspace2(double ans0[3], double ans1[3], double _n[9]) {
  double n[9], tmp[3], norm;

  ELL_3M_COPY(n, _n);
  _ellAlign3(n);
  ELL_3V_ADD3(tmp, n+0, n+3, n+6);
  ELL_3V_NORM(tmp, tmp, norm);
  
  /* any two vectors which are perpendicular to the (supposedly 1D)
     span of the column vectors span the nullspace */
  ell3vPerp_d(ans0, tmp);
  ELL_3V_NORM(ans0, ans0, norm);
  ELL_3V_CROSS(ans1, tmp, ans0);

  return;
}

/*
******** ell3mEigenvalues()
**
** finds eigenvalues of given matrix.
**
** m+0, m+3, m+6, are the COLUMNS of the matrix
**
** returns information about the roots according to ellCubeRoot enum,
** see header for ellCubic for details.
**
** given matrix is NOT modified
**
** This does NOT use biff
*/
int
ell3mEigenvalues(double eval[3], double m[9], int newton) {
  double A, B, C;

  /* 
  ** from gordon with mathematica; these are the coefficients of the
  ** cubic polynomial in x: det(x*I - M).  The full cubic is
  ** x^3 + A*x^2 + B*x + C.
  */
  A = -m[0] - m[4] - m[8];
  B = m[0]*m[4] - m[1]*m[3] 
    + m[0]*m[8] - m[2]*m[6] 
    + m[4]*m[8] - m[5]*m[7];
  C = (m[2]*m[4] - m[1]*m[5])*m[6]
    + (m[0]*m[5] - m[2]*m[3])*m[7]
    + (m[1]*m[3] - m[0]*m[4])*m[8];
  return ellCubic(eval, A, B, C, newton);
}

/*
******** ell3mEigensolve()
**
** finds eigenvalues and eigenvectors of given matrix m,
** m+0, m+3, m+6, are the COLUMNS of the matrix.
**
** returns information about the roots according to ellCubeRoot enum,
** see header for ellCubic for details.  When eval[i] is set, evec+3*i
** is set to a corresponding eigenvector.  The eigenvectors are
** (evec+0)[], (evec+3)[], and (evec+6)[]
**
** The eigenvalues (and associated eigenvectors) are sorted in
** descending order.
**
** This does NOT use biff
*/
int
ell3mEigensolve(double eval[3], double evec[9], double m[9], int newton) {
  double n[9], e0, e1, e2, t /* , tmpv[3] */ ;
  int roots;

  /* if (ellDebug) {
    printf("lineal3Eigensolve: input matrix:\n");
    printf("{{%20.15f,\t%20.15f,\t%20.15f},\n", m[0], m[3], m[6]);
    printf(" {%20.15f,\t%20.15f,\t%20.15f},\n", m[1], m[4], m[7]);
    printf(" {%20.15f,\t%20.15f,\t%20.15f}};\n",m[2], m[5], m[8]);
    } */
  
  roots = ell3mEigenvalues(eval, m, newton);
  ELL_3V_GET(e0, e1, e2, eval);
  /* if (linealDebug) {
    printf("lineal3Eigensolve: numroots = %d\n", numroots);
    } */

  /* we form m - lambda*I by doing a memcpy from m, and then
     (repeatedly) over-writing the diagonal elements */
  ELL_3M_COPY(n, m);
  switch (roots) {
  case ellCubicRootThree:
    ELL_SORT3(e0, e1, e2, t);
    /* if (ellDebug) {
      printf("lineal3Eigensolve: evals: %20.15f %20.15f %20.15f\n", 
	     eval[0], eval[1], eval[2]);
	     } */
    ELL_3M_DIAG_SET(n, m[0]-e0, m[4]-e0, m[8]-e0);
    ell3mNullspace1(evec+0, n);
    ELL_3M_DIAG_SET(n, m[0]-e1, m[4]-e1, m[8]-e1);
    ell3mNullspace1(evec+3, n);
    ELL_3M_DIAG_SET(n, m[0]-e2, m[4]-e2, m[8]-e2);
    ell3mNullspace1(evec+6, n);
    _ellFixerUpper(evec);
    ELL_3V_SET(eval, e0, e1, e2);
    break;
  case ellCubicRootSingleDouble:
    ELL_SORT3(e0, e1, e2, t);
    if (e0 > e1) {
      /* one big (e0) , two small (e1, e2) : more like a cigar */
      ELL_3M_DIAG_SET(n, m[0]-e0, m[4]-e0, m[8]-e0);
      ell3mNullspace1(evec+0, n);
      ELL_3M_DIAG_SET(n, m[0]-e1, m[4]-e1, m[8]-e1);
      ell3mNullspace2(evec+3, evec+6, n);
    }
    else {
      /* two big (e0, e1), one small (e2): more like a pancake */
      ELL_3M_DIAG_SET(n, m[0]-e0, m[4]-e0, m[8]-e0);
      ell3mNullspace2(evec+0, evec+3, n);
      ELL_3M_DIAG_SET(n, m[0]-e2, m[4]-e2, m[8]-e2);
      ell3mNullspace1(evec+6, n);
    }
    _ellFixerUpper(evec);
    ELL_3V_SET(eval, e0, e1, e2);
    break;
  case ellCubicRootTriple:
    /* one triple root; use any basis as the eigenvectors */
    ELL_3V_SET(evec+0, 1, 0, 0);
    ELL_3V_SET(evec+3, 0, 1, 0);
    ELL_3V_SET(evec+6, 0, 0, 1);
    ELL_3V_SET(eval, e0, e1, e2);
    break;
  case ellCubicRootSingle:
    /* only one real root */
    ELL_3M_DIAG_SET(n, m[0]-e0, m[4]-e0, m[8]-e0);
    ell3mNullspace1(evec+0, n);
    ELL_3V_SET(evec+3, AIR_NAN, AIR_NAN, AIR_NAN);
    ELL_3V_SET(evec+6, AIR_NAN, AIR_NAN, AIR_NAN);
    ELL_3V_SET(eval, e0, AIR_NAN, AIR_NAN);
    break;
  }
  /* if (ellDebug) {
    printf("lineal3Eigensolve (numroots = %d): evecs: \n", numroots);
    ELL_3MV_MUL(tmpv, m, evec[0]);
    printf(" (%g:%g): %20.15f %20.15f %20.15f\n", 
	   eval[0], ELL_3V_DOT(evec[0], tmpv), 
	   evec[0][0], evec[0][1], evec[0][2]);
    ELL_3MV_MUL(tmpv, m, evec[1]);
    printf(" (%g:%g): %20.15f %20.15f %20.15f\n", 
	   eval[1], ELL_3V_DOT(evec[1], tmpv), 
	   evec[1][0], evec[1][1], evec[1][2]);
    ELL_3MV_MUL(tmpv, m, evec[2]);
    printf(" (%g:%g): %20.15f %20.15f %20.15f\n", 
	   eval[2], ELL_3V_DOT(evec[2], tmpv), 
	   evec[2][0], evec[2][1], evec[2][2]);
	   } */
  return roots;
}
