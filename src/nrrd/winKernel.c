/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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

#include "nrrd.h"

#define _SINC(x) (sin(M_PI*x)/(M_PI*x))

#define _COS(x, R) \
   (x > R ? 0 : (x < -R ? 0 : (x == 0 ? 1.0 : \
    (1 + cos(M_PI*x/R))*_SINC(x)/2)))

double
_nrrdCosInt(double *parm) {

  /* This isn't true, but I don't have an approximation for
     SinIntegral which is good for large arguments */
  return 1.0;
}

double
_nrrdCosSup(double *parm) {
  double S;

  S = parm[0];
  return parm[1]*S;
}

double
_nrrdCos1_d(double x, double *parm) {
  double R, S;
  
  S = parm[0]; R = parm[1];
  x /= S;
  return _COS(x, R)/S;
}

float
_nrrdCos1_f(float x, double *parm) {
  float R, S;
  
  S = parm[0]; R = parm[1];
  x /= S;
  return _COS(x, R)/S;
}

void
_nrrdCosN_d(double *f, double *x, size_t len, double *parm) {
  double S, R, t;
  size_t i;
  
  S = parm[0]; R = parm[1];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _COS(t, R)/S;
  }
}

void
_nrrdCosN_f(float *f, float *x, size_t len, double *parm) {
  float S, R, t;
  size_t i;
  
  S = parm[0]; R = parm[1];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _COS(t, R)/S;
  }
}

NrrdKernel
_nrrdKernelCos = {
  "cos",
  2, _nrrdCosSup,  _nrrdCosInt,   
  _nrrdCos1_f,   _nrrdCosN_f,   _nrrdCos1_d,   _nrrdCosN_d
};
NrrdKernel *
nrrdKernelCos = &_nrrdKernelCos;

/* ------------------------------------------------------------ */

#define _DCOS(x, R)                                            \
   (x > R ? 0.0 : (x < -R ? 0.0 : (x == 0 ? 0.0 :              \
    (R*(1 + cos(M_PI*x/R))*(M_PI*x*cos(M_PI*x) - sin(M_PI*x))  \
       - M_PI*x*sin(M_PI*x)*sin(M_PI*x/R))/(2*R*M_PI*x*x))))
double
_nrrdDCosInt(double *parm) {

  /* again, not correct, but I don't have a good approximation ... */
  return 0.0;
}

double
_nrrdDCosSup(double *parm) {
  double S;

  S = parm[0];
  return parm[1]*S;
}

double
_nrrdDCos1_d(double x, double *parm) {
  double R, S;
  
  S = parm[0]; R = parm[1];
  x /= S;
  return _DCOS(x, R)/(S*S);
}

float
_nrrdDCos1_f(float x, double *parm) {
  float R, S;
  
  S = parm[0]; R = parm[1];
  x /= S;
  return _DCOS(x, R)/(S*S);
}

void
_nrrdDCosN_d(double *f, double *x, size_t len, double *parm) {
  double t, R, S;
  size_t i;
  
  S = parm[0]; R = parm[1];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _DCOS(t, R)/(S*S);
  }
}

void
_nrrdDCosN_f(float *f, float *x, size_t len, double *parm) {
  float t, R, S;
  size_t i;
  
  S = parm[0]; R = parm[1];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _DCOS(t, R)/(S*S);
  }
}

NrrdKernel
_nrrdKernelDCos = {
  "cosD",
  2, _nrrdDCosSup, _nrrdDCosInt,  
  _nrrdDCos1_f,  _nrrdDCosN_f,  _nrrdDCos1_d,  _nrrdDCosN_d
};
NrrdKernel *
nrrdKernelCosD = &_nrrdKernelDCos;

/* ------------------------------------------------------------ */

#define _DDCOS_A(x, R) \
  (-2*M_PI*R*x*cos(M_PI*x)*(R + R*cos(M_PI*x/R) + M_PI*x*sin(M_PI*x/R)))
#define _DDCOS_B(x, R) \
  (-(-2*R*R + M_PI*M_PI*(1 + R*R)*x*x)*cos(M_PI*x/R) + \
   R*(2*R - M_PI*M_PI*R*x*x + 2*M_PI*x*sin(M_PI*x/R)))

#define _DDCOS(x, R)                                                        \
   (x > R ? 0 : (x < -R ? 0 : (x == 0 ? -M_PI*M_PI*(3 + 2*R*R)/(6*R*R) :    \
    (_DDCOS_A(x,R) + sin(M_PI*x)*_DDCOS_B(x,R))/(2*M_PI*R*R*x*x*x)          \
    )))

double
_nrrdDDCosInt(double *parm) {

  /* again, not correct */
  return 0.0;
}

double
_nrrdDDCosSup(double *parm) {
  double S;

  S = parm[0];
  return parm[1]*S;
}

double
_nrrdDDCos1_d(double x, double *parm) {
  double R, S;
  
  S = parm[0]; R = parm[1];
  x /= S;
  return _DDCOS(x, R)/(S*S*S);
}

float
_nrrdDDCos1_f(float x, double *parm) {
  float R, S;
  
  S = parm[0]; R = parm[1];
  x /= S;
  return _DDCOS(x, R)/(S*S*S);
}

void
_nrrdDDCosN_d(double *f, double *x, size_t len, double *parm) {
  double t, S, R;
  size_t i;
  
  S = parm[0]; R = parm[1];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _DDCOS(t, R)/(S*S*S);
  }
}

void
_nrrdDDCosN_f(float *f, float *x, size_t len, double *parm) {
  float t, S, R;
  size_t i;
  
  S = parm[0]; R = parm[1];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _DDCOS(t, R)/(S*S*S);
  }
}

NrrdKernel
_nrrdKernelDDCos = {
  "cosDD",
  2, _nrrdDDCosSup,_nrrdDDCosInt, 
  _nrrdDDCos1_f, _nrrdDDCosN_f, _nrrdDDCos1_d, _nrrdDDCosN_d
};
NrrdKernel *
nrrdKernelCosDD = &_nrrdKernelDDCos;
