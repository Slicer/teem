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


#include <nrrd.h>

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
_nrrdZeroED(double x, double *param) {
  double S;

  S = param[0];
  x = AIR_ABS(x)/S;
  return _ZERO(x)/S;
}

float
_nrrdZeroEF(float x, double *param) {
  float S;

  S = param[0];
  x = AIR_ABS(x)/S;
  return _ZERO(x)/S;
}

void
_nrrdZeroVD(double *f, double *x, int len, double *param) {
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
_nrrdZeroVF(float *f, float *x, int len, double *param) {
  float t, S;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _ZERO(t)/S;
  }
}

nrrdKernel
_nrrdKernelZero = {
  1, _nrrdZeroSup, _nrrdZeroInt,
  _nrrdZeroEF, _nrrdZeroVF, _nrrdZeroED, _nrrdZeroVD
};
nrrdKernel *
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
_nrrdBoxED(double x, double *param) {
  double S;

  S = param[0];
  x = AIR_ABS(x)/S;
  return _BOX(x)/S;
}

float
_nrrdBoxEF(float x, double *param) {
  float S;

  S = param[0];
  x = AIR_ABS(x)/S;
  return _BOX(x)/S;
}

void
_nrrdBoxVD(double *f, double *x, int len, double *param) {
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
_nrrdBoxVF(float *f, float *x, int len, double *param) {
  float t, S;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _BOX(t)/S;
  }
}

nrrdKernel
_nrrdKernelBox = {
  1, _nrrdBoxSup, _nrrdBoxInt,  
  _nrrdBoxEF,  _nrrdBoxVF,  _nrrdBoxED,  _nrrdBoxVD
};
nrrdKernel *
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
_nrrdTentED(double x, double *param) {
  double S;
  
  S = param[0];
  x = AIR_ABS(x)/S;
  return _TENT(x)/S;
}

float
_nrrdTentEF(float x, double *param) {
  float S;
  
  S = param[0];
  x = AIR_ABS(x)/S;
  return _TENT(x)/S;
}

void
_nrrdTentVD(double *f, double *x, int len, double *param) {
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
_nrrdTentVF(float *f, float *x, int len, double *param) {
  float t, S;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _TENT(t)/S;
  }
}

nrrdKernel
_nrrdKernelTent = {
  1, _nrrdTentSup,_nrrdTentInt, 
  _nrrdTentEF, _nrrdTentVF, _nrrdTentED, _nrrdTentVD
};
nrrdKernel *
nrrdKernelTent = &_nrrdKernelTent;

/* ------------------------------------------------------------ */

#define _FORDIF(x) (x <= -1 ?  0 :        \
                   (x <=  0 ? -1 :        \
                   (x <=  1 ?  1 : 0 )))

double
_nrrdFDInt(double *param) {
  
  return 0.0;
}

double
_nrrdFDSup(double *param) {
  double S;
  
  S = param[0];
  return S;
}

double
_nrrdFDED(double x, double *param) {
  double S;
  
  S = param[0];
  x /= S;
  return _FORDIF(x)/S;
}

float
_nrrdFDEF(float x, double *param) {
  float S;
  
  S = param[0];
  x /= S;
  return _FORDIF(x)/S;
}

void
_nrrdFDVD(double *f, double *x, int len, double *param) {
  double S;
  double t;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _FORDIF(t)/S;
  }
}

void
_nrrdFDVF(float *f, float *x, int len, double *param) {
  float t, S;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _FORDIF(t)/S;
  }
}

nrrdKernel
_nrrdKernelFD = {
  1, _nrrdFDSup,  _nrrdFDInt,   
  _nrrdFDEF,   _nrrdFDVF,   _nrrdFDED,   _nrrdFDVD
};
nrrdKernel *
nrrdKernelForwDiff = &_nrrdKernelFD;

/* ------------------------------------------------------------ */

#define _CENDIF(x) (x <= -2 ?  0         :        \
                   (x <= -1 ? -0.5*x - 1 :        \
		   (x <=  1 ?  0.5*x     :        \
                   (x <=  2 ? -0.5*x + 1 : 0 ))))

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
_nrrdCDED(double x, double *param) {
  double S;
  
  S = param[0];
  x /= S;
  return _CENDIF(x)/S;
}

float
_nrrdCDEF(float x, double *param) {
  float S;
  
  S = param[0];
  x /= S;
  return _CENDIF(x)/S;
}

void
_nrrdCDVD(double *f, double *x, int len, double *param) {
  double S;
  double t;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _CENDIF(t)/S;
  }
}

void
_nrrdCDVF(float *f, float *x, int len, double *param) {
  float t, S;
  int i;
  
  S = param[0];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _CENDIF(t)/S;
  }
}

nrrdKernel
_nrrdKernelCD = {
  1, _nrrdCDSup,  _nrrdCDInt,   
  _nrrdCDEF,   _nrrdCDVF,   _nrrdCDED,   _nrrdCDVD
};
nrrdKernel *
nrrdKernelCentDiff = &_nrrdKernelCD;

/* ------------------------------------------------------------ */

#define _BCCUBIC(x, B, C) (                                         \
   x >= 2.0                                                         \
   ? 0 : (x >= 1.0                                                  \
          ? (((-B/6 - C)*x + B + 5*C)*x -2*B - 8*C)*x + 4*B/3 + 4*C \
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
_nrrdBCED(double x, double *param) {
  double S;
  double B, C;
  
  S = param[0]; B = param[1]; C = param[2]; 
  x = AIR_ABS(x)/S;
  return _BCCUBIC(x, B, C)/S;
}

float
_nrrdBCEF(float x, double *param) {
  float B, C, S;
  
  S = param[0]; B = param[1]; C = param[2]; 
  x = AIR_ABS(x)/S;
  return _BCCUBIC(x, B, C)/S;
}

void
_nrrdBCVD(double *f, double *x, int len, double *param) {
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
_nrrdBCVF(float *f, float *x, int len, double *param) {
  float S, t, B, C;
  int i;
  
  S = param[0]; B = param[1]; C = param[2]; 
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _BCCUBIC(t, B, C)/S;
  }
}

nrrdKernel
_nrrdKernelBC = {
  3, _nrrdBCSup,  _nrrdBCInt,   
  _nrrdBCEF,   _nrrdBCVF,   _nrrdBCED,   _nrrdBCVD
};
nrrdKernel *
nrrdKernelBCCubic = &_nrrdKernelBC;

/* ------------------------------------------------------------ */

#define _BCCUBICD(x, B, C) (                            \
   x >= 2.0                                             \
   ? 0 : (x >= 1.0                                      \
          ? ((-B/2 - 3*C)*x + 2*B + 10*C)*x -2*B - 8*C  \
          : ((6 - 9*B/2 - 3*C)*x - 6 + 4*B + 2*C)*x))

double
_nrrdBCDInt(double *param) {

  return 0.0;
}

double
_nrrdBCDSup(double *param) {
  double S;

  S = param[0];
  return 2*S;
}

double
_nrrdBCDED(double x, double *param) {
  double S;
  double B, C;
  int sgn = 1;
  
  S = param[0]; B = param[1]; C = param[2]; 
  if (x < 0) { x = -x; sgn = -1; }
  x /= S;
  return sgn*_BCCUBICD(x, B, C)/S;
}

float
_nrrdBCDEF(float x, double *param) {
  float B, C, S;
  int sgn = 1;
  
  S = param[0]; B = param[1]; C = param[2]; 
  if (x < 0) { x = -x; sgn = -1; }
  x /= S;
  return sgn*_BCCUBICD(x, B, C)/S;
}

void
_nrrdBCDVD(double *f, double *x, int len, double *param) {
  double S;
  double t, B, C;
  int i, sgn;
  
  S = param[0]; B = param[1]; C = param[2]; 
  for (i=0; i<len; i++) {
    t = x[i]/S;
    if (t < 0) { t = -t; sgn = -1; } else { sgn = 1; }
    f[i] = sgn*_BCCUBICD(t, B, C)/S;
  }
}

void
_nrrdBCDVF(float *f, float *x, int len, double *param) {
  float S, t, B, C;
  int i, sgn;
  
  S = param[0]; B = param[1]; C = param[2]; 
  for (i=0; i<len; i++) {
    t = x[i]/S;
    if (t < 0) { t = -t; sgn = -1; } else { sgn = 1; }
    f[i] = sgn*_BCCUBICD(t, B, C)/S;
  }
}

nrrdKernel
_nrrdKernelBCD = {
  3, _nrrdBCDSup, _nrrdBCDInt,  
  _nrrdBCDEF,  _nrrdBCDVF,  _nrrdBCDED,  _nrrdBCDVD
};
nrrdKernel *
nrrdKernelBCCubicD = &_nrrdKernelBCD;

/* ------------------------------------------------------------ */

#define _BCCUBICDD(x, B, C) (                        \
   x >= 2.0                                          \
   ? 0 : (x >= 1.0                                   \
          ? (-B - 6*C)*x + 2*B + 10*C                \
          : (12 - 9*B - 6*C)*x - 6 + 4*B + 2*C  ))

double
_nrrdBCDDInt(double *param) {

  return 0.0;
}

double
_nrrdBCDDSup(double *param) {
  double S;

  S = param[0];
  return 2*S;
}

double
_nrrdBCDDED(double x, double *param) {
  double S;
  double B, C;
  
  S = param[0]; B = param[1]; C = param[2]; 
  x = AIR_ABS(x)/S;
  return _BCCUBICDD(x, B, C)/S;
}

float
_nrrdBCDDEF(float x, double *param) {
  float B, C, S;
  
  S = param[0]; B = param[1]; C = param[2]; 
  x = AIR_ABS(x)/S;
  return _BCCUBICDD(x, B, C)/S;
}

void
_nrrdBCDDVD(double *f, double *x, int len, double *param) {
  double S;
  double t, B, C;
  int i;
  
  S = param[0]; B = param[1]; C = param[2]; 
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _BCCUBICDD(t, B, C)/S;
  }
}

void
_nrrdBCDDVF(float *f, float *x, int len, double *param) {
  float S, t, B, C;
  int i;
  
  S = param[0]; B = param[1]; C = param[2]; 
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _BCCUBICDD(t, B, C)/S;
  }
}

nrrdKernel
_nrrdKernelBCDD = {
  3, _nrrdBCDDSup,_nrrdBCDDInt, 
  _nrrdBCDDEF, _nrrdBCDDVF, _nrrdBCDDED, _nrrdBCDDVD
};
nrrdKernel *
nrrdKernelBCCubicDD = &_nrrdKernelBCDD;

/* ------------------------------------------------------------ */

#define _AQUARTIC(x, A) (                                                    \
   x >= 3.0                                                                  \
   ? 0 : (x >= 2.0                                                           \
         ? A*(-54 + x*(81 + x*(-45 + x*(11 - x))))                           \
         : (x >= 1.0                                                         \
            ? 4 - 6*A + x*(-10 + 25*A + x*(9 - 33*A +                        \
					   x*(-3.5 + 17*A + x*(0.5 - 3*A)))) \
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
_nrrdA4ED(double x, double *param) {
  double S;
  double A;
  
  S = param[0]; A = param[1];
  x = AIR_ABS(x)/S;
  return _AQUARTIC(x, A)/S;
}

float
_nrrdA4EF(float x, double *param) {
  float A, S;
  
  S = param[0]; A = param[1];
  x = AIR_ABS(x)/S;
  return _AQUARTIC(x, A)/S;
}

void
_nrrdA4VD(double *f, double *x, int len, double *param) {
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
_nrrdA4VF(float *f, float *x, int len, double *param) {
  float S, t, A;
  int i;
  
  S = param[0]; A = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _AQUARTIC(t, A)/S;
  }
}

nrrdKernel
_nrrdKernelA4 = {
  2, _nrrdA4Sup,  _nrrdA4Int,   
  _nrrdA4EF,   _nrrdA4VF,   _nrrdA4ED,   _nrrdA4VD
};
nrrdKernel *
nrrdKernelAQuartic = &_nrrdKernelA4;

/* ------------------------------------------------------------ */

#define _DAQUARTIC(x, A) (                                                   \
   x >= 3.0                                                                  \
   ? 0 : (x >= 2.0                                                           \
         ? A*(81 + x*(-90 + x*(33 - 4*x)))                                   \
         : (x >= 1.0                                                         \
            ? -10 + 25*A + x*(18 - 66*A + x*(-10.5 + 51*A + x*(2 - 12*A)))   \
            : x*(-6 + 12*A + x*(7.5 - 30*A + x*(-2 + 16*A))))))

double
_nrrdA4DInt(double *param) {

  return 0.0;
}

double
_nrrdA4DSup(double *param) {
  double S;

  S = param[0];
  return 3*S;
}

double
_nrrdA4DED(double x, double *param) {
  double S;
  double A;
  int sgn = 1;
  
  S = param[0]; A = param[1];
  if (x < 0) { x = -x; sgn = -1; }
  x /= S;
  return sgn*_DAQUARTIC(x, A)/S;
}

float
_nrrdA4DEF(float x, double *param) {
  float A, S;
  int sgn = 1;
  
  S = param[0]; A = param[1];
  if (x < 0) { x = -x; sgn = -1; }
  x /= S;
  return sgn*_DAQUARTIC(x, A)/S;
}

void
_nrrdA4DVD(double *f, double *x, int len, double *param) {
  double S;
  double t, A;
  int i, sgn;
  
  S = param[0]; A = param[1];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    if (t < 0) { t = -t; sgn = -1; } else { sgn = 1; }
    f[i] = sgn*_DAQUARTIC(t, A)/S;
  }
}

void
_nrrdA4DVF(float *f, float *x, int len, double *param) {
  float S, t, A;
  int i, sgn;
  
  S = param[0]; A = param[1];
  for (i=0; i<len; i++) {
    t = x[i]/S;
    if (t < 0) { t = -t; sgn = -1; } else { sgn = 1; }
    f[i] = sgn*_DAQUARTIC(t, A)/S;
  }
}

nrrdKernel
_nrrdKernelA4D = {
  2, _nrrdA4DSup, _nrrdA4DInt,  
  _nrrdA4DEF,  _nrrdA4DVF,  _nrrdA4DED,  _nrrdA4DVD
};
nrrdKernel *
nrrdKernelAQuarticD = &_nrrdKernelA4D;

/* ------------------------------------------------------------ */

#define _DDAQUARTIC(x, A) (                                                  \
   x >= 3.0                                                                  \
   ? 0 : (x >= 2.0                                                           \
         ? A*(-90 + x*(66 - x*12))                                           \
         : (x >= 1.0                                                         \
            ? 18 - 66*A + x*(-21 + 102*A + x*(6 - 36*A))                     \
            : -6 + 12*A + x*(15 - 60*A + x*(-6 + 48*A)))))

double
_nrrdA4DDInt(double *param) {

  return 0.0;
}

double
_nrrdA4DDSup(double *param) {
  double S;

  S = param[0];
  return 3*S;
}

double
_nrrdA4DDED(double x, double *param) {
  double S;
  double A;
  
  S = param[0]; A = param[1];
  x = AIR_ABS(x)/S;
  return _DDAQUARTIC(x, A)/S;
}

float
_nrrdA4DDEF(float x, double *param) {
  float S, A;
  
  S = param[0]; A = param[1];
  x = AIR_ABS(x)/S;
  return _DDAQUARTIC(x, A)/S;
}

void
_nrrdA4DDVD(double *f, double *x, int len, double *param) {
  double S;
  double t, A;
  int i;
  
  S = param[0]; A = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _DDAQUARTIC(t, A)/S;
  }
}

void
_nrrdA4DDVF(float *f, float *x, int len, double *param) {
  float S, t, A;
  int i;
  
  S = param[0]; A = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _DDAQUARTIC(t, A)/S;
  }
}

nrrdKernel
_nrrdKernelA4DD = {
  2, _nrrdA4DDSup,_nrrdA4DDInt, 
  _nrrdA4DDEF, _nrrdA4DDVF, _nrrdA4DDED, _nrrdA4DDVD
};
nrrdKernel *
nrrdKernelAQuarticDD = &_nrrdKernelA4DD;

/* ------------------------------------------------------------ */

#define _GAUSS(x, sig, cut) (                                                \
   x >= sig*cut ? 0                                                          \
   : exp(-x*x/(2.0*sig*sig))/(sig*2.50662827463100050241))

#define _DGAUSS(x, sig, cut) (                                               \
   x >= sig*cut ? 0                                                          \
   : -exp(-x*x/(2.0*sig*sig))*x/(sig*sig*sig*2.50662827463100050241))

#define _DDGAUSS(x, sig, cut) (                                              \
   x >= sig*cut ? 0                                                          \
   : -exp(-x*x/(2.0*sig*sig))*(x*x-sig*sig) /                                \
      (sig*sig*sig*sig*sig*2.50662827463100050241))

double
_nrrdGInt(double *param) {
  double cut;

  cut = param[2];
  return cut;
}

double
_nrrdGSup(double *param) {
  double S, cut;

  S = param[0];
  cut = param[2];
  return S*cut;
}

double
_nrrdGED(double x, double *param) {
  double S;
  double A;
  
  S = param[0]; A = param[1];
  x = AIR_ABS(x)/S;
  return _AQUARTIC(x, A)/S;
}

float
_nrrdGEF(float x, double *param) {
  float A, S;
  
  S = param[0]; A = param[1];
  x = AIR_ABS(x)/S;
  return _AQUARTIC(x, A)/S;
}

void
_nrrdGVD(double *f, double *x, int len, double *param) {
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
_nrrdGVF(float *f, float *x, int len, double *param) {
  float S, t, A;
  int i;
  
  S = param[0]; A = param[1];
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _AQUARTIC(t, A)/S;
  }
}

nrrdKernel
_nrrdKernelG = {
  3, _nrrdGSup,  _nrrdGInt,   
  _nrrdGEF,   _nrrdGVF,   _nrrdGED,   _nrrdGVD
};
nrrdKernel *
nrrdKernelGaussian = &_nrrdKernelG;
