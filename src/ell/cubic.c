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
******** ellCubic(): 
**
** finds real roots of x^3 + A*x^2 + B*x + C.
**
** records the found real roots in the given root array. 
**
** returns information about the roots according to ellCubicRoot enum,
** the set the following values in given root[] array:
**   ellCubicRootSingle: root[0], root[1] == root[2] == AIR_NAN
**   ellCubicRootTriple: root[0] == root[1] == root[2]
**   ellCubicRootSingleDouble: single root[0]; double root[1] == root[2]
**   ellCubicRootThree: root[0], root[1], root[2]
**
** This does NOT use biff
*/
int
ellCubic(double root[3], double A, double B, double C, int polish) {
  double epsilon = 1.0E-11, sq_A, p, q, cb_p, D, sqrt_D, 
    u, v, x, phi, t, sub;

  sub = 1.0/3.0*A;
  sq_A = A*A;
  p = 1.0/3.0*(-1.0/3.0*sq_A + B);
  q = 1.0/2.0*(2.0/27.0*A*sq_A - 1.0/3.0*A*B + C);
  cb_p = p*p*p;
  D = q*q + cb_p;
  if (D < -epsilon) {
    /* three distinct roots- this is the most common case, it has 
       been tested the most, its code should go first */
    phi = 1.0/3.0*acos(-q/sqrt(-cb_p));
    t = 2*sqrt(-p);
    root[0] = t*cos(phi) - sub;
    root[1] = -t*cos(phi + M_PI/3.0) - sub;
    root[2] = -t*cos(phi - M_PI/3.0) - sub;
    return ellCubicRootThree;
  }
  else if (D > epsilon) {
    double nr, fnr;
    /* one real solution, except maybe also a "rescued" double root */
    sqrt_D = sqrt(D);
    u = cbrt(sqrt_D-q);
    v = -cbrt(sqrt_D+q);
    x = u+v - sub;
    if (!polish) {
      root[0] = x;
      root[1] = root[2] = AIR_NAN;
      return ellCubicRootSingle;
    }
    /* else polish the hell out of x, the known root, so as to get the 
       most accurate possible calculation for nr, the possible new root */
    x = x - (((x + A)*x + B)*x + C)/((3.0*x + 2.0*A)*x + B);
    x = x - (((x + A)*x + B)*x + C)/((3.0*x + 2.0*A)*x + B);
    nr = -(A + x)/2.0;
    fnr = ((nr + A)*nr + B)*nr + C;  /* the polynomial evaluated at nr */
    /* if (ellDebug) {
      printf("ellCubic(): root = %g -> %g, nr=% 20.15f; fnr=% 20.15f\n", 
	     x, (((x + A)*x + B)*x + C), nr, fnr);
	     } */
    if (fnr < -epsilon || fnr > epsilon) {
      root[0] = x;
      return ellCubicRootSingle;
    }
    else {
      if (ellDebug) {
	printf("ellCubic(): rescued double root:% 20.15f\n", nr);
      } 
      root[0] = x;
      root[1] = nr;
      root[2] = nr;
      return ellCubicRootSingleDouble;
    }
  } 
  else {
    /* else D is in the interval [-epsilon, +epsilon] */
    if (q < -epsilon || q > epsilon) {
      /* one double root and one single root */
      u = cbrt(-q);
      root[0] = 2*u - sub;
      root[1] = -u - sub;
      root[2] = -u - sub;
      return ellCubicRootSingleDouble;
    } 
    else {
      /* one triple root */
      root[0] = root[1] = root[2] = -sub;
      return ellCubicRootTriple;
    }
  }
  /* shouldn't ever get here */
  return ellCubicRootUnknown;
}








