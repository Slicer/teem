/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/


#include "ell.h"

/*
******** ell3mNullspace1()()
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
  double t0[3], t1[3], t2[3], norm;
  
  /* find the three cross-products of pairs of column vectors of n */
  ELL_3V_CROSS(t0, n+0, n+3);
  ELL_3V_CROSS(t1, n+0, n+6);
  ELL_3V_CROSS(t2, n+3, n+6);

  /* point them the same way */
  if (ELL_3V_DOT(t0, t1) < 0)
    ELL_3V_SCALE(t1, t1, -1);
  if (ELL_3V_DOT(t1, t2) < 0)
    ELL_3V_SCALE(t2, t2, -1);

  /* add them up (longer, hence more accurate, ones will dominate) */
  ELL_3V_ADD3(ans, t0, t1, t2);

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

  /* copy to local matrix */
  memcpy(n, _n, 9*sizeof(double));
  
  /* make the column vectors point the same way */
  if (ELL_3V_DOT(n+0, n+3) < 0)
    ELL_3V_SCALE(n+3, n+3, -1);
  if (ELL_3V_DOT(n+3, n+6) < 0)
    ELL_3V_SCALE(n+6, n+6, -1);

  /* add them up (longer, hence more accurate, ones will dominate) */
  ELL_3V_ADD3(tmp, n+0, n+3, n+6);
  ELL_3V_NORM(tmp, tmp, norm);
  
  /* any two vectors which are perpendicular to the (supposedly 1D)
     span of the column vectors span the nullspace */
  ell3vPerp(ans0, tmp);
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
ell3mEigenvalues(double eval[3], double m[9], int polish) {
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
  return ellCubic(eval, A, B, C, polish);
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
ell3mEigensolve(double eval[3], double evec[9], double m[9], int polish) {
  double n[9], e0, e1, e2, t /* , tmpv[3] */ ;
  int roots;

  /* if (ellDebug) {
    printf("lineal3Eigensolve: input matrix:\n");
    printf("{{%20.15f,\t%20.15f,\t%20.15f},\n", m[0], m[3], m[6]);
    printf(" {%20.15f,\t%20.15f,\t%20.15f},\n", m[1], m[4], m[7]);
    printf(" {%20.15f,\t%20.15f,\t%20.15f}};\n",m[2], m[5], m[8]);
    } */
  
  roots = ell3mEigenvalues(eval, m, polish);
  ELL_3V_GET(e0, e1, e2, eval);
  /* if (linealDebug) {
    printf("lineal3Eigensolve: numroots = %d\n", numroots);
    } */

  /* we form m - lambda.I by doing a memcpy from m, and then
     (repeatedly) over-writing the diagonal elements */
  memcpy(n, m, 9*sizeof(double));
  /* ELL_3M_COPY(n, m); */
  switch (roots) {
  case ellCubicRootThree:
    ELL_SORT3(e0, e1, e2, t);
    /* if (linealDebug) {
      printf("lineal3Eigensolve: evals: %20.15f %20.15f %20.15f\n", 
	     eval[0], eval[1], eval[2]);
	     } */
    ELL_3M_SETDIAG(n, m[0]-e0, m[4]-e0, m[8]-e0);
    ell3mNullspace1(evec+0, n);
    ELL_3M_SETDIAG(n, m[0]-e1, m[4]-e1, m[8]-e1);
    ell3mNullspace1(evec+3, n);
    ELL_3M_SETDIAG(n, m[0]-e2, m[4]-e2, m[8]-e2);
    ell3mNullspace1(evec+6, n);
    ELL_3V_SET(eval, e0, e1, e2);
    break;
  case ellCubicRootSingleDouble:
    if (e0 > e1) {
      /* one big (e0) , two small (e1) : more like a cigar */
      ELL_3M_SETDIAG(n, m[0]-e0, m[4]-e0, m[8]-e0);
      ell3mNullspace1(evec+0, n);
      ELL_3M_SETDIAG(n, m[0]-e1, m[4]-e1, m[8]-e1);
      ell3mNullspace2(evec+3, evec+6, n);
    }
    else {
      /* two big (e1), one small (e0): more like a pancake */
      ELL_3M_SETDIAG(n, m[0]-e1, m[4]-e1, m[8]-e1);
      ell3mNullspace2(evec+0, evec+3, n);
      ELL_3M_SETDIAG(n, m[0]-e0, m[4]-e0, m[8]-e0);
      ell3mNullspace1(evec+6, n);
      /* e2 == e1 */
      ELL_SWAP2(e0, e1, t);
      ELL_3V_SET(eval, e0, e1, e2);
    }
    break;
  case ellCubicRootTriple:
    /* one triple root; use any basis as the eigenvectors */
    ELL_3V_SET(evec+0, 1, 0, 0);
    ELL_3V_SET(evec+3, 0, 1, 0);
    ELL_3V_SET(evec+6, 0, 0, 1);
    break;
  case ellCubicRootSingle:
    /* only one real root */
    ELL_3M_SETDIAG(n, m[0]-e0, m[4]-e0, m[8]-e0);
    ell3mNullspace1(evec+0, n);
    ELL_3V_SET(evec+3, AIR_NAN, AIR_NAN, AIR_NAN);
    ELL_3V_SET(evec+6, AIR_NAN, AIR_NAN, AIR_NAN);
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
