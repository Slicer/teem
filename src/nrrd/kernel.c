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

float
_nrrdZeroSup(float *param) {
  float S;
  
  S = param ? *param : 1.0;
  return S;
}

double
_nrrdZeroED(double x, float *param) {
  float S;

  S = param ? *param : 1.0;
  x = AIR_ABS(x)/S;
  return _ZERO(x)/S;
}

float
_nrrdZeroEF(float x, float *param) {
  float S;

  S = param ? *param : 1.0;
  x = AIR_ABS(x)/S;
  return _ZERO(x)/S;
}

void
_nrrdZeroVD(double *f, double *x, int len, float *param) {
  float S;
  double t;
  int i;
  
  S = param ? *param : 1.0;
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _ZERO(t)/S;
  }
}

void
_nrrdZeroVF(float *f, float *x, int len, float *param) {
  float t, S;
  int i;
  
  S = param ? *param : 1.0;
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _ZERO(t)/S;
  }
}

/* ------------------------------------------------------------ */

#define _BOX(x) (x > 0.5 ? 0 : (x < 0.5 ? 1 : 0.5))

float
_nrrdBoxSup(float *param) {
  float S;
  
  S = param ? *param : 1.0;
  return S/2;
}

double
_nrrdBoxED(double x, float *param) {
  float S;

  S = param ? *param : 1.0;
  x = AIR_ABS(x)/S;
  return _BOX(x)/S;
}

float
_nrrdBoxEF(float x, float *param) {
  float S;

  S = param ? *param : 1.0;
  x = AIR_ABS(x)/S;
  return _BOX(x)/S;
}

void
_nrrdBoxVD(double *f, double *x, int len, float *param) {
  float S;
  double t;
  int i;
  
  S = param ? *param : 1.0;
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _BOX(t)/S;
  }
}

void
_nrrdBoxVF(float *f, float *x, int len, float *param) {
  float t, S;
  int i;
  
  S = param ? *param : 1.0;
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _BOX(t)/S;
  }
}

/* ------------------------------------------------------------ */

#define _TENT(x) (x >= 1 ? 0 : 1 - x)

float
_nrrdTentSup(float *param) {
  float S;
  
  S = param ? *param : 1.0;
  return S;
}

double
_nrrdTentED(double x, float *param) {
  float S;
  
  S = param ? *param : 1.0;
  x = AIR_ABS(x)/S;
  return _TENT(x)/S;
}

float
_nrrdTentEF(float x, float *param) {
  float S;
  
  S = param ? *param : 1.0;
  x = AIR_ABS(x)/S;
  return _TENT(x)/S;
}

void
_nrrdTentVD(double *f, double *x, int len, float *param) {
  float S;
  double t;
  int i;
  
  S = param ? *param : 1.0;
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _TENT(t)/S;
  }
}

void
_nrrdTentVF(float *f, float *x, int len, float *param) {
  float t, S;
  int i;
  
  S = param ? *param : 1.0;
  for (i=0; i<len; i++) {
    t = x[i]; t = AIR_ABS(t)/S;
    f[i] = _TENT(t)/S;
  }
}

/* ------------------------------------------------------------ */

#define _FORDIF(x) (x <= -1 ?  0 :        \
                   (x <=  0 ? -1 :        \
                   (x <=  1 ?  1 : 0 )))

float
_nrrdFDSup(float *param) {
  float S;
  
  S = param ? *param : 1.0;
  return S;
}

double
_nrrdFDED(double x, float *param) {
  float S;
  
  S = param ? *param : 1.0;
  x /= S;
  return _FORDIF(x)/S;
}

float
_nrrdFDEF(float x, float *param) {
  float S;
  
  S = param ? *param : 1.0;
  x /= S;
  return _FORDIF(x)/S;
}

void
_nrrdFDVD(double *f, double *x, int len, float *param) {
  float S;
  double t;
  int i;
  
  S = param ? *param : 1.0;
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _FORDIF(t)/S;
  }
}

void
_nrrdFDVF(float *f, float *x, int len, float *param) {
  float t, S;
  int i;
  
  S = param ? *param : 1.0;
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _FORDIF(t)/S;
  }
}

/* ------------------------------------------------------------ */

#define _CENDIF(x) (x <= -2 ?  0         :        \
                   (x <= -1 ? -0.5*x - 1 :        \
		   (x <=  1 ?  0.5*x     :        \
                   (x <=  2 ? -0.5*x + 1 : 0 ))))

float
_nrrdCDSup(float *param) {
  float S;
  
  S = param ? *param : 1.0;
  return 2*S;
}

double
_nrrdCDED(double x, float *param) {
  float S;
  
  S = param ? *param : 1.0;
  x /= S;
  return _CENDIF(x)/S;
}

float
_nrrdCDEF(float x, float *param) {
  float S;
  
  S = param ? *param : 1.0;
  x /= S;
  return _CENDIF(x)/S;
}

void
_nrrdCDVD(double *f, double *x, int len, float *param) {
  float S;
  double t;
  int i;
  
  S = param ? *param : 1.0;
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _CENDIF(t)/S;
  }
}

void
_nrrdCDVF(float *f, float *x, int len, float *param) {
  float t, S;
  int i;
  
  S = param ? *param : 1.0;
  for (i=0; i<len; i++) {
    t = x[i]/S;
    f[i] = _CENDIF(t)/S;
  }
}

/* ------------------------------------------------------------ */

#define _BCCUBIC(x, B, C) (                                         \
   x >= 2.0                                                         \
   ? 0 : (x >= 1.0                                                  \
          ? (((-B/6 - C)*x + B + 5*C)*x -2*B - 8*C)*x + 4*B/3 + 4*C \
          : ((2 - 3*B/2 - C)*x - 3 + 2*B + C)*x*x + 1 - B/3))

float
_nrrdBCSup(float *param) {
  float S;

  S = param[0];
  return 2*S;
}

double
_nrrdBCED(double x, float *param) {
  float S;
  double B, C;
  
  S = param[0]; B = param[1]; C = param[2]; 
  x = AIR_ABS(x)/S;
  return _BCCUBIC(x, B, C)/S;
}

float
_nrrdBCEF(float x, float *param) {
  float B, C, S;
  
  S = param[0]; B = param[1]; C = param[2]; 
  x = AIR_ABS(x)/S;
  return _BCCUBIC(x, B, C)/S;
}

void
_nrrdBCVD(double *f, double *x, int len, float *param) {
  float S;
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
_nrrdBCVF(float *f, float *x, int len, float *param) {
  float S, t, B, C;
  int i;
  
  S = param[0]; B = param[1]; C = param[2]; 
  for (i=0; i<len; i++) {
    t = x[i];
    t = AIR_ABS(t)/S;
    f[i] = _BCCUBIC(t, B, C)/S;
  }
}

/* ------------------------------------------------------------ */

#define _BCCUBICD(x, B, C) (                                        \
   x >= 2.0                                            \
   ? 0 : (x >= 1.0                                     \
          ? ((-B/2 - 3*C)*x + 2*B + 10*C)*x -2*B - 8*C              \
          : ((6 - 9*B/2 - 3*C)*x - 6 + 4*B + 2*C)*x))

float
_nrrdBCDSup(float *param) {
  float S;

  S = param[0];
  return 2*S;
}

double
_nrrdBCDED(double x, float *param) {
  float S;
  double B, C;
  int sgn = 1;
  
  S = param[0]; B = param[1]; C = param[2]; 
  if (x < 0) { x = -x; sgn = -1; }
  x /= S;
  return sgn*_BCCUBICD(x, B, C)/S;
}

float
_nrrdBCDEF(float x, float *param) {
  float B, C, S;
  int sgn = 1;
  
  S = param[0]; B = param[1]; C = param[2]; 
  if (x < 0) { x = -x; sgn = -1; }
  x /= S;
  return sgn*_BCCUBICD(x, B, C)/S;
}

void
_nrrdBCDVD(double *f, double *x, int len, float *param) {
  float S;
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
_nrrdBCDVF(float *f, float *x, int len, float *param) {
  float S, t, B, C;
  int i, sgn;
  
  S = param[0]; B = param[1]; C = param[2]; 
  for (i=0; i<len; i++) {
    t = x[i]/S;
    if (t < 0) { t = -t; sgn = -1; } else { sgn = 1; }
    f[i] = sgn*_BCCUBICD(t, B, C)/S;
  }
}

/* ------------------------------------------------------------ */

nrrdKernelMethods
nrrdKernel[NRRD_KERNEL_MAX+1] = {
  {NULL, NULL, NULL, NULL, NULL},
  {_nrrdZeroSup, _nrrdZeroEF, _nrrdZeroVF, _nrrdZeroED, _nrrdZeroVD},
  {_nrrdBoxSup,  _nrrdBoxEF,  _nrrdBoxVF,  _nrrdBoxED,  _nrrdBoxVD},
  {_nrrdTentSup, _nrrdTentEF, _nrrdTentVF, _nrrdTentED, _nrrdTentVD},
  {_nrrdFDSup,   _nrrdFDEF,   _nrrdFDVF,   _nrrdFDED,   _nrrdFDVD},
  {_nrrdCDSup,   _nrrdCDEF,   _nrrdCDVF,   _nrrdCDED,   _nrrdCDVD},
  {_nrrdBCSup,   _nrrdBCEF,   _nrrdBCVF,   _nrrdBCED,   _nrrdBCVD},
  {_nrrdBCDSup,  _nrrdBCDEF,  _nrrdBCDVF,  _nrrdBCDED,  _nrrdBCDVD}
};
