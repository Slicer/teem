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

/* ------------------------------------------------------------ */

/* the "zero" kernel is here more as a template than for anything else */

#define _ZERO(x) 0

double
_nrrdZeroInt(double *param) {
  
  return 0.0;
}

double
_nrrdZeroSup(double *param) {
  double S;
  
  S = param[0];
  return S;
}

double
_nrrdZero1_d(double x, double *param) {
  double S;

  S = param[0];
  x = AIR_ABS(x)/S;
  return _ZERO(x)/S;
}

float
_nrrdZero1_f(float x, double *param) {
  float S;

  S = param[0];
  x = AIR_ABS(x)/S;
  return _ZERO(x)/S;
}

void
_nrrdZeroN_d(double *f, double *x, int len, double *param) {
  double S;
  double t;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _ZERO(t)/S;
  }
}

void
_nrrdZeroN_f(float *f, float *x, int len, double *param) {
  float t, S;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _ZERO(t)/S;
  }
}

NrrdKernel
_nrrdKernelZero = {
  1, _nrrdZeroSup, _nrrdZeroInt,
  _nrrdZero1_f, _nrrdZeroN_f, _nrrdZero1_d, _nrrdZeroN_d
};
NrrdKernel *
nrrdKernelZero = &_nrrdKernelZero;

/* ------------------------------------------------------------ */

#define _BOX(x) (x > 0.5 ? 0 : (x < 0.5 ? 1 : 0.5))

double
_nrrdBoxInt(double *param) {
  
  return 1.0;
}

double
_nrrdBoxSup(double *param) {
  double S;
  
  S = param[0];
  /* adding the 0.5 is to insure that weights computed within the
     support really do catch all the non-zero values */
  return S/2 + 0.5;
}

double
_nrrdBox1_d(double x, double *param) {
  double S;

  S = param[0];
  x = AIR_ABS(x)/S;
  return _BOX(x)/S;
}

float
_nrrdBox1_f(float x, double *param) {
  float S;

  S = param[0];
  x = AIR_ABS(x)/S;
  return _BOX(x)/S;
}

void
_nrrdBoxN_d(double *f, double *x, int len, double *param) {
  double S;
  double t;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _BOX(t)/S;
  }
}

void
_nrrdBoxN_f(float *f, float *x, int len, double *param) {
  float t, S;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _BOX(t)/S;
  }
}

NrrdKernel
_nrrdKernelBox = {
  1, _nrrdBoxSup, _nrrdBoxInt,  
  _nrrdBox1_f,  _nrrdBoxN_f,  _nrrdBox1_d,  _nrrdBoxN_d
};
NrrdKernel *
nrrdKernelBox = &_nrrdKernelBox;

/* ------------------------------------------------------------ */

#define _TENT(x) (x >= 1 ? 0 : 1 - x)

double
_nrrdTentInt(double *param) {
  
  return 1.0;
}

double
_nrrdTentSup(double *param) {
  double S;
  
  S = param[0];
  return S;
}

double
_nrrdTent1_d(double x, double *param) {
  double S;
  
  S = param[0];
  x = AIR_ABS(x)/S;
  return _TENT(x)/S;
}

float
_nrrdTent1_f(float x, double *param) {
  float S;
  
  S = param[0];
  x = AIR_ABS(x)/S;
  return _TENT(x)/S;
}

void
_nrrdTentN_d(double *f, double *x, int len, double *param) {
  double S;
  double t;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _TENT(t)/S;
  }
}

void
_nrrdTentN_f(float *f, float *x, int len, double *param) {
  float t, S;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _TENT(t)/S;
  }
}

NrrdKernel
_nrrdKernelTent = {
  1, _nrrdTentSup,_nrrdTentInt, 
  _nrrdTent1_f, _nrrdTentN_f, _nrrdTent1_d, _nrrdTentN_d
};
NrrdKernel *
nrrdKernelTent = &_nrrdKernelTent;

/* ------------------------------------------------------------ */

#define _FORDIF(x) (x <= -1 ?  0 :        \
                   (x <=  0 ?  1 :        \
                   (x <=  1 ? -1 : 0 )))

double
_nrrdFDInt(double *param) {
  
  return 0.0;
}

double
_nrrdFDSup(double *param) {
  double S;
  
  S = param[0];
  return S+0.0001;  /* sigh */
}

double
_nrrdFD1_d(double x, double *param) {
  double S;
  
  S = param[0];
  x /= S;
  return _FORDIF(x)/(S*S);
}

float
_nrrdFD1_f(float x, double *param) {
  float S;
  
  S = param[0];
  x /= S;
  return _FORDIF(x)/(S*S);
}

void
_nrrdFDN_d(double *f, double *x, int len, double *param) {
  double S;
  double t;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _FORDIF(t)/(S*S);
  }
}

void
_nrrdFDN_f(float *f, float *x, int len, double *param) {
  float t, S;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _FORDIF(t)/(S*S);
  }
}

NrrdKernel
_nrrdKernelFD = {
  1, _nrrdFDSup,  _nrrdFDInt,   
  _nrrdFD1_f,   _nrrdFDN_f,   _nrrdFD1_d,   _nrrdFDN_d
};
NrrdKernel *
nrrdKernelForwDiff = &_nrrdKernelFD;

/* ------------------------------------------------------------ */

#define _CENDIF(x) (x <= -2 ?  0         :        \
                   (x <= -1 ?  0.5*x + 1 :        \
		   (x <=  1 ? -0.5*x     :        \
                   (x <=  2 ?  0.5*x - 1 : 0 ))))

double
_nrrdCDInt(double *param) {
  
  return 0.0;
}

double
_nrrdCDSup(double *param) {
  double S;
  
  S = param[0];
  return 2*S;
}

double
_nrrdCD1_d(double x, double *param) {
  double S;
  
  S = param[0];
  x /= S;
  return _CENDIF(x)/(S*S);
}

float
_nrrdCD1_f(float x, double *param) {
  float S;
  
  S = param[0];
  x /= S;
  return _CENDIF(x)/(S*S);
}

void
_nrrdCDN_d(double *f, double *x, int len, double *param) {
  double S;
  double t;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _CENDIF(t)/(S*S);
  }
}

void
_nrrdCDN_f(float *f, float *x, int len, double *param) {
  float t, S;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _CENDIF(t)/(S*S);
  }
}

NrrdKernel
_nrrdKernelCD = {
  1, _nrrdCDSup,  _nrrdCDInt,   
  _nrrdCD1_f,   _nrrdCDN_f,   _nrrdCD1_d,   _nrrdCDN_d
};
NrrdKernel *
nrrdKernelCentDiff = &_nrrdKernelCD;

/* ------------------------------------------------------------ */

#define _BCCUBIC(x, B, C)                                     \
  (x >= 2.0 ? 0 :                                             \
  (x >= 1.0                                                   \
   ? (((-B/6 - C)*x + B + 5*C)*x -2*B - 8*C)*x + 4*B/3 + 4*C  \
   : ((2 - 3*B/2 - C)*x - 3 + 2*B + C)*x*x + 1 - B/3))

double
_nrrdBCInt(double *param) {

  return 1.0;
}

double
_nrrdBCSup(double *param) {
  double S;

  S = param[0];
  return 2*S;
}

double
_nrrdBC1_d(double x, double *param) {
  double S;
  double B, C;
  
  S = param[0]; B = param[1]; C = param[2]; 
  x = AIR_ABS(x)/S;
  return _BCCUBIC(x, B, C)/S;
}

float
_nrrdBC1_f(float x, double *param) {
  float B, C, S;
  
  S = param[0]; B = param[1]; C = param[2]; 
  x = AIR_ABS(x)/S;
  return _BCCUBIC(x, B, C)/S;
}

void
_nrrdBCN_d(double *f, double *x, int len, double *param) {
  double S;
  double t, B, C;
  int i;
  
  S = param[0]; B = param[1]; C = param[2]; 
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _BCCUBIC(t, B, C)/S;
  }
}

void
_nrrdBCN_f(float *f, float *x, int len, double *param) {
  float S, t, B, C;
  int i;
  
  S = param[0]; B = param[1]; C = param[2]; 
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _BCCUBIC(t, B, C)/S;
  }
}

NrrdKernel
_nrrdKernelBC = {
  3, _nrrdBCSup,  _nrrdBCInt,   
  _nrrdBC1_f,   _nrrdBCN_f,   _nrrdBC1_d,   _nrrdBCN_d
};
NrrdKernel *
nrrdKernelBCCubic = &_nrrdKernelBC;

/* ------------------------------------------------------------ */

#define _DBCCUBIC(x, B, C)                        \
   (x >= 2.0 ? 0 :                                \
   (x >= 1.0                                      \
    ? ((-B/2 - 3*C)*x + 2*B + 10*C)*x -2*B - 8*C  \
    : ((6 - 9*B/2 - 3*C)*x - 6 + 4*B + 2*C)*x))

double
_nrrdDBCInt(double *param) {

  return 0.0;
}

double
_nrrdDBCSup(double *param) {
  double S;

  S = param[0];
  return 2*S;
}

double
_nrrdDBC1_d(double x, double *param) {
  double S;
  double B, C;
  int sgn = 1;
  
  S = param[0]; B = param[1]; C = param[2]; 
  if (x < 0) { x = -x; sgn = -1; }
  x /= S;
  return sgn*_DBCCUBIC(x, B, C)/(S*S);
}

float
_nrrdDBC1_f(float x, double *param) {
  float B, C, S;
  int sgn = 1;
  
  S = param[0]; B = param[1]; C = param[2]; 
  if (x < 0) { x = -x; sgn = -1; }
  x /= S;
  return sgn*_DBCCUBIC(x, B, C)/(S*S);
}

void
_nrrdDBCN_d(double *f, double *x, int len, double *param) {
  double S;
  double t, B, C;
  int i, sgn;
  
  S = param[0]; B = param[1]; C = param[2]; 
  for (i=0; i<len; i++) {
    t = x[i]/S;
    if (t < 0) { t = -t; sgn = -1; } else { sgn = 1; }
    f[i] = sgn*_DBCCUBIC(t, B, C)/(S*S);
  }
}

void
_nrrdDBCN_f(float *f, float *x, int len, double *param) {
  float S, t, B, C;
  int i, sgn;
  
  S = param[0]; B = param[1]; C = param[2]; 
  for (i=0; i<len; i++) {
    t = x[i]/S;
    if (t < 0) { t = -t; sgn = -1; } else { sgn = 1; }
    f[i] = sgn*_DBCCUBIC(t, B, C)/(S*S);
  }
}

NrrdKernel
_nrrdKernelDBC = {
  3, _nrrdDBCSup, _nrrdDBCInt,  
  _nrrdDBC1_f,  _nrrdDBCN_f,  _nrrdDBC1_d,  _nrrdDBCN_d
};
NrrdKernel *
nrrdKernelBCCubicD = &_nrrdKernelDBC;

/* ------------------------------------------------------------ */

#define _DDBCCUBIC(x, B, C)                    \
   (x >= 2.0 ? 0 :                             \
   (x >= 1.0                                   \
    ? (-B - 6*C)*x + 2*B + 10*C                \
    : (12 - 9*B - 6*C)*x - 6 + 4*B + 2*C  ))

double
_nrrdDDBCInt(double *param) {

  return 0.0;
}

double
_nrrdDDBCSup(double *param) {
  double S;

  S = param[0];
  return 2*S;
}

double
_nrrdDDBC1_d(double x, double *param) {
  double S;
  double B, C;
  
  S = param[0]; B = param[1]; C = param[2]; 
  x = AIR_ABS(x)/S;
  return _DDBCCUBIC(x, B, C)/(S*S*S);
}

float
_nrrdDDBC1_f(float x, double *param) {
  float B, C, S;
  
  S = param[0]; B = param[1]; C = param[2]; 
  x = AIR_ABS(x)/S;
  return _DDBCCUBIC(x, B, C)/(S*S*S);
}

void
_nrrdDDBCN_d(double *f, double *x, int len, double *param) {
  double S;
  double t, B, C;
  int i;
  
  S = param[0]; B = param[1]; C = param[2]; 
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _DDBCCUBIC(t, B, C)/(S*S*S);
  }
}

void
_nrrdDDBCN_f(float *f, float *x, int len, double *param) {
  float S, t, B, C;
  int i;
  
  S = param[0]; B = param[1]; C = param[2]; 
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _DDBCCUBIC(t, B, C)/(S*S*S);
  }
}

NrrdKernel
_nrrdKernelDDBC = {
  3, _nrrdDDBCSup,_nrrdDDBCInt, 
  _nrrdDDBC1_f, _nrrdDDBCN_f, _nrrdDDBC1_d, _nrrdDDBCN_d
};
NrrdKernel *
nrrdKernelBCCubicDD = &_nrrdKernelDDBC;

/* ------------------------------------------------------------ */

#define _AQUARTIC(x, A) \
   (x >= 3.0 ? 0 :      \
   (x >= 2.0            \
    ? A*(-54 + x*(81 + x*(-45 + x*(11 - x)))) \
    : (x >= 1.0                               \
       ? 4 - 6*A + x*(-10 + 25*A + x*(9 - 33*A                         \
				 + x*(-3.5 + 17*A + x*(0.5 - 3*A))))   \
       : 1 + x*x*(-3 + 6*A + x*((2.5 - 10*A) + x*(-0.5 + 4*A))))))

double
_nrrdA4Int(double *param) {

  return 1.0;
}

double
_nrrdA4Sup(double *param) {
  double S;

  S = param[0];
  return 3*S;
}

double
_nrrdA41_d(double x, double *param) {
  double S;
  double A;
  
  S = param[0]; A = param[1];
  x = AIR_ABS(x)/S;
  return _AQUARTIC(x, A)/S;
}

float
_nrrdA41_f(float x, double *param) {
  float A, S;
  
  S = param[0]; A = param[1];
  x = AIR_ABS(x)/S;
  return _AQUARTIC(x, A)/S;
}

void
_nrrdA4N_d(double *f, double *x, int len, double *param) {
  double S;
  double t, A;
  int i;
  
  S = param[0]; A = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _AQUARTIC(t, A)/S;
  }
}

void
_nrrdA4N_f(float *f, float *x, int len, double *param) {
  float S, t, A;
  int i;
  
  S = param[0]; A = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _AQUARTIC(t, A)/S;
  }
}

NrrdKernel
_nrrdKernelA4 = {
  2, _nrrdA4Sup,  _nrrdA4Int,   
  _nrrdA41_f,   _nrrdA4N_f,   _nrrdA41_d,   _nrrdA4N_d
};
NrrdKernel *
nrrdKernelAQuartic = &_nrrdKernelA4;

/* ------------------------------------------------------------ */

#define _DAQUARTIC(x, A) \
   (x >= 3.0 ? 0 :       \
   (x >= 2.0             \
    ? A*(81 + x*(-90 + x*(33 - 4*x))) \
    : (x >= 1.0                       \
       ? -10 + 25*A + x*(18 - 66*A + x*(-10.5 + 51*A + x*(2 - 12*A))) \
       : x*(-6 + 12*A + x*(7.5 - 30*A + x*(-2 + 16*A))))))

double
_nrrdDA4Int(double *param) {

  return 0.0;
}

double
_nrrdDA4Sup(double *param) {
  double S;

  S = param[0];
  return 3*S;
}

double
_nrrdDA41_d(double x, double *param) {
  double S;
  double A;
  int sgn = 1;
  
  S = param[0]; A = param[1];
  if (x < 0) { x = -x; sgn = -1; }
  x /= S;
  return sgn*_DAQUARTIC(x, A)/(S*S);
}

float
_nrrdDA41_f(float x, double *param) {
  float A, S;
  int sgn = 1;
  
  S = param[0]; A = param[1];
  if (x < 0) { x = -x; sgn = -1; }
  x /= S;
  return sgn*_DAQUARTIC(x, A)/(S*S);
}

void
_nrrdDA4N_d(double *f, double *x, int len, double *param) {
  double S;
  double t, A;
  int i, sgn;
  
  S = param[0]; A = param[1];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    if (t < 0) { t = -t; sgn = -1; } else { sgn = 1; }
    f[i] = sgn*_DAQUARTIC(t, A)/(S*S);
  }
}

void
_nrrdDA4N_f(float *f, float *x, int len, double *param) {
  float S, t, A;
  int i, sgn;
  
  S = param[0]; A = param[1];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    if (t < 0) { t = -t; sgn = -1; } else { sgn = 1; }
    f[i] = sgn*_DAQUARTIC(t, A)/(S*S);
  }
}

NrrdKernel
_nrrdKernelDA4 = {
  2, _nrrdDA4Sup, _nrrdDA4Int,  
  _nrrdDA41_f,  _nrrdDA4N_f,  _nrrdDA41_d,  _nrrdDA4N_d
};
NrrdKernel *
nrrdKernelAQuarticD = &_nrrdKernelDA4;

/* ------------------------------------------------------------ */

#define _DDAQUARTIC(x, A) \
   (x >= 3.0 ? 0 :        \
   (x >= 2.0              \
    ? A*(-90 + x*(66 - x*12)) \
    : (x >= 1.0               \
       ? 18 - 66*A + x*(-21 + 102*A + x*(6 - 36*A))   \
       : -6 + 12*A + x*(15 - 60*A + x*(-6 + 48*A)))))

double
_nrrdDDA4Int(double *param) {

  return 0.0;
}

double
_nrrdDDA4Sup(double *param) {
  double S;

  S = param[0];
  return 3*S;
}

double
_nrrdDDA41_d(double x, double *param) {
  double S;
  double A;
  
  S = param[0]; A = param[1];
  x = AIR_ABS(x)/S;
  return _DDAQUARTIC(x, A)/(S*S*S);
}

float
_nrrdDDA41_f(float x, double *param) {
  float S, A;
  
  S = param[0]; A = param[1];
  x = AIR_ABS(x)/S;
  return _DDAQUARTIC(x, A)/(S*S*S);
}

void
_nrrdDDA4N_d(double *f, double *x, int len, double *param) {
  double S;
  double t, A;
  int i;
  
  S = param[0]; A = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _DDAQUARTIC(t, A)/(S*S*S);
  }
}

void
_nrrdDDA4N_f(float *f, float *x, int len, double *param) {
  float S, t, A;
  int i;
  
  S = param[0]; A = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _DDAQUARTIC(t, A)/(S*S*S);
  }
}

NrrdKernel
_nrrdKernelDDA4 = {
  2, _nrrdDDA4Sup,_nrrdDDA4Int, 
  _nrrdDDA41_f, _nrrdDDA4N_f, _nrrdDDA41_d, _nrrdDDA4N_d
};
NrrdKernel *
nrrdKernelAQuarticDD = &_nrrdKernelDDA4;

/* ------------------------------------------------------------ */

#define _GAUSS(x, sig, cut) ( \
   x >= sig*cut ? 0           \
   : exp(-x*x/(2.0*sig*sig))/(sig*2.50662827463100050241))

double
_nrrdGInt(double *param) {
  double cut;
  
  cut = param[1];
  return airErf(cut/sqrt(2.0));
}

double
_nrrdGSup(double *param) {
  double sig, cut;

  sig = param[0];
  cut = param[1];
  return sig*cut;
}

double
_nrrdG1_d(double x, double *param) {
  double sig, cut;
  
  sig = param[0];
  cut = param[1];
  x = AIR_ABS(x);
  return _GAUSS(x, sig, cut);
}

float
_nrrdG1_f(float x, double *param) {
  float sig, cut;
  
  sig = param[0];
  cut = param[1];
  x = AIR_ABS(x);
  return _GAUSS(x, sig, cut);
}

void
_nrrdGN_d(double *f, double *x, int len, double *param) {
  double sig, cut, t;
  int i;
  
  sig = param[0];
  cut = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t);
    f[i] = _GAUSS(t, sig, cut);
  }
}

void
_nrrdGN_f(float *f, float *x, int len, double *param) {
  float sig, cut, t;
  int i;
  
  sig = param[0];
  cut = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t);
    f[i] = _GAUSS(t, sig, cut);
  }
}

NrrdKernel
_nrrdKernelG = {
  2, _nrrdGSup,  _nrrdGInt,   
  _nrrdG1_f,   _nrrdGN_f,   _nrrdG1_d,   _nrrdGN_d
};
NrrdKernel *
nrrdKernelGaussian = &_nrrdKernelG;

/* ------------------------------------------------------------ */

#define _DGAUSS(x, sig, cut) (                                               \
   x >= sig*cut ? 0                                                          \
   : -exp(-x*x/(2.0*sig*sig))*x/(sig*sig*sig*2.50662827463100050241))

double
_nrrdDGInt(double *param) {
  
  return 0;
}

double
_nrrdDGSup(double *param) {
  double sig, cut;

  sig = param[0];
  cut = param[1];
  return sig*cut;
}

double
_nrrdDG1_d(double x, double *param) {
  double sig, cut;
  int sgn = 1;
  
  sig = param[0];
  cut = param[1];
  if (x < 0) { x = -x; sgn = -1; }
  return sgn*_DGAUSS(x, sig, cut);
}

float
_nrrdDG1_f(float x, double *param) {
  float sig, cut;
  int sgn = 1;
  
  sig = param[0];
  cut = param[1];
  if (x < 0) { x = -x; sgn = -1; }
  return sgn*_DGAUSS(x, sig, cut);
}

void
_nrrdDGN_d(double *f, double *x, int len, double *param) {
  double sig, cut, t;
  int i, sgn;
  
  sig = param[0];
  cut = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    if (t < 0) { t = -t; sgn = -1; } else { sgn = 1; }
    f[i] = sgn*_DGAUSS(t, sig, cut);
  }
}

void
_nrrdDGN_f(float *f, float *x, int len, double *param) {
  float sig, cut, t;
  int i, sgn;
  
  sig = param[0];
  cut = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    if (t < 0) { t = -t; sgn = -1; } else { sgn = 1; }
    f[i] = sgn*_DGAUSS(t, sig, cut);
  }
}

NrrdKernel
_nrrdKernelDG = {
  2, _nrrdDGSup,  _nrrdDGInt,   
  _nrrdDG1_f,   _nrrdDGN_f,   _nrrdDG1_d,   _nrrdDGN_d
};
NrrdKernel *
nrrdKernelGaussianD = &_nrrdKernelDG;

/* ------------------------------------------------------------ */

#define _DDGAUSS(x, sig, cut) ( \
   x >= sig*cut ? 0             \
   : exp(-x*x/(2.0*sig*sig))*(x*x-sig*sig) /       \
     (sig*sig*sig*sig*sig*2.50662827463100050241))

double
_nrrdDDGInt(double *param) {
  double sig, cut;
  
  sig = param[0];
  cut = param[1];
  return -0.79788456080286535587*cut*exp(-cut*cut/2)/(sig*sig);
}

double
_nrrdDDGSup(double *param) {
  double sig, cut;

  sig = param[0];
  cut = param[1];
  return sig*cut;
}

double
_nrrdDDG1_d(double x, double *param) {
  double sig, cut;
  
  sig = param[0];
  cut = param[1];
  x = AIR_ABS(x);
  return _DDGAUSS(x, sig, cut);
}

float
_nrrdDDG1_f(float x, double *param) {
  float sig, cut;
  
  sig = param[0];
  cut = param[1];
  x = AIR_ABS(x);
  return _DDGAUSS(x, sig, cut);
}

void
_nrrdDDGN_d(double *f, double *x, int len, double *param) {
  double sig, cut, t;
  int i;
  
  sig = param[0];
  cut = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t);
    f[i] = _DDGAUSS(t, sig, cut);
  }
}

void
_nrrdDDGN_f(float *f, float *x, int len, double *param) {
  float sig, cut, t;
  int i;
  
  sig = param[0];
  cut = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t);
    f[i] = _DDGAUSS(t, sig, cut);
  }
}

NrrdKernel
_nrrdKernelDDG = {
  2, _nrrdDDGSup,  _nrrdDDGInt,   
  _nrrdDDG1_f,   _nrrdDDGN_f,   _nrrdDDG1_d,   _nrrdDDGN_d
};
NrrdKernel *
nrrdKernelGaussianDD = &_nrrdKernelDDG;


/* ------------------------------------------------------------ */

NrrdKernel *
_nrrdKernelStrToKern(char *str) {
  
  if (!strcmp("zero", str))       return nrrdKernelZero;
  if (!strcmp("z", str))          return nrrdKernelZero;
  if (!strcmp("box", str))        return nrrdKernelBox;
  if (!strcmp("b", str))          return nrrdKernelBox;
  if (!strcmp("tent", str))       return nrrdKernelTent;
  if (!strcmp("t", str))          return nrrdKernelTent;
  if (!strcmp("forwdiff", str))   return nrrdKernelForwDiff;
  if (!strcmp("fordif", str))     return nrrdKernelForwDiff;
  if (!strcmp("centdiff", str))   return nrrdKernelCentDiff;
  if (!strcmp("cendif", str))     return nrrdKernelCentDiff;
  if (!strcmp("cubic", str))      return nrrdKernelBCCubic;
  if (!strcmp("cubicd", str))     return nrrdKernelBCCubicD;
  if (!strcmp("cubicdd", str))    return nrrdKernelBCCubicDD;
  if (!strcmp("c", str))          return nrrdKernelBCCubic;
  if (!strcmp("cd", str))         return nrrdKernelBCCubicD;
  if (!strcmp("cdd", str))        return nrrdKernelBCCubicDD;
  if (!strcmp("quartic", str))    return nrrdKernelAQuartic;
  if (!strcmp("quarticd", str))   return nrrdKernelAQuarticD;
  if (!strcmp("quarticdd", str))  return nrrdKernelAQuarticDD;
  if (!strcmp("q", str))          return nrrdKernelAQuartic;  
  if (!strcmp("qd", str))         return nrrdKernelAQuarticD;  
  if (!strcmp("qdd", str))        return nrrdKernelAQuarticDD;  
  if (!strcmp("gaussian", str))   return nrrdKernelGaussian;
  if (!strcmp("gaussiand", str))  return nrrdKernelGaussianD;
  if (!strcmp("gaussiandd", str)) return nrrdKernelGaussianDD;
  if (!strcmp("gauss", str))      return nrrdKernelGaussian;
  if (!strcmp("gaussd", str))     return nrrdKernelGaussianD;
  if (!strcmp("gaussdd", str))    return nrrdKernelGaussianDD;
  if (!strcmp("g", str))          return nrrdKernelGaussian;
  if (!strcmp("gd", str))         return nrrdKernelGaussianD;
  if (!strcmp("gdd", str))        return nrrdKernelGaussianDD;
  return NULL;
}

int
nrrdKernelParse(NrrdKernel **kernelP, double *param, char *_str) {
  char me[]="nrrdKernelParse", err[128], str[AIR_STRLEN_HUGE],
    kstr[AIR_STRLEN_MED], *_pstr=NULL, *pstr;
  int i, j, NP;
  
  if (!(kernelP && param && _str)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  strcpy(str, _str);
  strcpy(kstr, "");
  pstr = NULL;
  pstr = strchr(str, ':');
  if (pstr) {
    *pstr = '\0';
    _pstr = ++pstr;
  }
  strcpy(kstr, str);
  airToLower(kstr);
  if (!(*kernelP = _nrrdKernelStrToKern(kstr))) {
    sprintf(err, "%s: kernel \"%s\" not recognized", me, kstr);
    biffAdd(NRRD, err); return 1;
  }
  NP = (*kernelP)->numParam;
  for (i=0; i<=NP-1; i++) {
    if (!pstr)
      break;
    if (1 != sscanf(pstr, "%lg", param+i)) {
      sprintf(err, "%s: trouble parsing \"%s\"", me, _pstr);
      biffAdd(NRRD, err); return 1;
    }
    if ((pstr = strchr(pstr, ','))) {
      pstr++;
      if (!*pstr) {
	sprintf(err, "%s: nothing after last comma in \"%s\"", me, _pstr);
	biffAdd(NRRD, err); return 1;
      }
    }
  }
  if (i < NP-1) {
    sprintf(err, "%s: couldn't parse minimum %d doubles from \"%s\"",
	    me, NP-1, _pstr);
    biffAdd(NRRD, err); return 1;
  }
  if (i == NP-1) {
    /* shift up parsed values, and set param[0] to default */
    for (j=NP-1; j>=1; j--) {
      param[j] = param[j-1];
    }
    param[0] = nrrdDefKernelParam0;
  }
  else {
    if (pstr) {
      sprintf(err, "%s: \"%s\" has more than %d doubles",
	      me, _pstr, NP);
      biffAdd(NRRD, err); return 1;
    }
  }
  return 0;
}
