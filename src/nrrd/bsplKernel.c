/*
  Teem: Tools to process and visualize scientific data and images              
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

#include "nrrd.h"

/*
** These kernels are the cardinal B-splines of different orders
** Using them with convolution assumes that the data has been pre-filtered
** so that the spline interpolates the original values.
*/

/* helper macros for doing abs() and remembering sign */
#define ABS_SGN(ax, sgn, x)                     \
  if (x < 0) {                                  \
    sgn = -1;                                   \
    ax = -x;                                    \
  } else {                                      \
    sgn = 1;                                    \
    ax = x;                                     \
  }

/* helper macro for listing the various members of the kernel */
#define BSPL_DECL(ord, deriv)                   \
  0,                                            \
    _bspl##ord##_sup,                           \
    _bspl##ord##d##deriv##_int,                 \
    _bspl##ord##d##deriv##_1f,                  \
    _bspl##ord##d##deriv##_Nf,                  \
    _bspl##ord##d##deriv##_1d,                  \
    _bspl##ord##d##deriv##_Nd

/* ============================= order *3* ============================= */

static double
_bspl3_sup(const double *parm) {
  AIR_UNUSED(parm);
  return 2.0;
}

/* ---------------------- order *3* deriv *0* -------------------------- */

static double
_bspl3d0_int(const double *parm) {
  AIR_UNUSED(parm);
  return 1.0;
}

/* t: tmp; ax: abs(x) */
#define BSPL3D0(ret, t, x)                     \
  if (x < 1) {                                 \
    ret = (4 + 3*(-2 + x)*x*x)/6;              \
  } else if (x < 2) {                          \
    t = (-2 + x);                              \
    ret = -t*t*t/6;                            \
  } else {                                     \
    ret = 0;                                   \
  }

static double
_bspl3d0_1d(double x, const double *parm) {
  double ax, tmp, r;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL3D0(r, tmp, ax);
  return r;
}

static float
_bspl3d0_1f(float x, const double *parm) {
  float ax, tmp, r;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL3D0(r, tmp, ax);
  return r;
}

static void
_bspl3d0_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, tmp, r;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = x[i]; ax = AIR_ABS(ax);
    BSPL3D0(r, tmp, ax);
    f[i] = r;
  }
}

static void
_bspl3d0_Nf(float *f, const float *x, size_t len, const double *parm) {
  float ax, tmp, r;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = x[i]; ax = AIR_ABS(ax);
    BSPL3D0(r, tmp, ax);
    f[i] = r;
  }
}

NrrdKernel
_nrrdKernelBSpline3 = {
  "bspl3",
  BSPL_DECL(3, 0)
};
NrrdKernel *const
nrrdKernelBSpline3 = &_nrrdKernelBSpline3;

/* ---------------------- order *3* deriv *1* -------------------------- */

static double
_bspl3d1_int(const double *parm) {
  AIR_UNUSED(parm);
  return 0.0;
}

/* t: tmp; ax: abs(x) */
#define BSPL3D1(ret, t, x)                     \
  if (x < 1) {                                 \
    ret = (-4 + 3*x)*x/2;                      \
  } else if (x < 2) {                          \
    t = (-2 + x);                              \
    ret = -t*t/2;                              \
  } else {                                     \
    ret = 0;                                   \
  }

static double
_bspl3d1_1d(double x, const double *parm) {
  double ax, tmp, r;
  int sgn;
  AIR_UNUSED(parm);

  ABS_SGN(ax, sgn, x);
  BSPL3D1(r, tmp, ax);
  return sgn*r;
}

static float
_bspl3d1_1f(float x, const double *parm) {
  float ax, tmp, r;
  int sgn;
  AIR_UNUSED(parm);

  ABS_SGN(ax, sgn, x);
  BSPL3D1(r, tmp, ax);
  return sgn*r;
}

static void
_bspl3d1_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, tmp, r;
  int sgn;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ABS_SGN(ax, sgn, x[i]);
    BSPL3D1(r, tmp, ax);
    f[i] = sgn*r;
  }
}

static void
_bspl3d1_Nf(float *f, const float *x, size_t len, const double *parm) {
  float ax, tmp, r;
  int sgn;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ABS_SGN(ax, sgn, x[i]);
    BSPL3D1(r, tmp, ax);
    f[i] = sgn*r;
  }
}

NrrdKernel
_nrrdKernelBSpline3D = {
  "bspl3d",
  BSPL_DECL(3, 1)
};
NrrdKernel *const
nrrdKernelBSpline3D = &_nrrdKernelBSpline3D;

/* ---------------------- order *3* deriv *2* -------------------------- */

static double
_bspl3d2_int(const double *parm) {
  AIR_UNUSED(parm);
  return 0.0;
}

/* NOTE: the tmp variable wasn't actually needed here, and this will
** likely be optimized out.  But the tmp argument to the macro is kept
** here (and the macro uses it to avoid a unused variable warning) to
** facilitate copy-and-paste for higher-order splines
*/
#define BSPL3D2(ret, tmp, x)                   \
  if (x < 1) {                                 \
    ret = -2 + 3*x;                            \
  } else if (x < 2) {                          \
    tmp = 2 - x;                               \
    ret = tmp;                                 \
  } else {                                     \
    ret = 0;                                   \
  }

static double
_bspl3d2_1d(double x, const double *parm) {
  double ax, tmp, r;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL3D2(r, tmp, ax);
  return r;
}

static float
_bspl3d2_1f(float x, const double *parm) {
  float ax, tmp, r;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL3D2(r, tmp, ax);
  return r;
}

static void
_bspl3d2_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, tmp, r;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = AIR_ABS(x[i]);
    BSPL3D2(r, tmp, ax);
    f[i] = r;
  }
}

static void
_bspl3d2_Nf(float *f, const float *x, size_t len, const double *parm) {
  float ax, tmp, r;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = AIR_ABS(x[i]);
    BSPL3D2(r, tmp, ax);
    f[i] = r;
  }
}

NrrdKernel
_nrrdKernelBSpline3DD = {
  "bspl3dd",
  BSPL_DECL(3, 2)
};
NrrdKernel *const
nrrdKernelBSpline3DD = &_nrrdKernelBSpline3DD;

/* ============================= order *4* ============================= */

/*
static double
_bspl4_sup(const double *parm) {
  AIR_UNUSED(parm);
  return 2.5;
}
*/

/* ============================= order *5* ============================= */

static double
_bspl5_sup(const double *parm) {
  AIR_UNUSED(parm);
  return 3.0;
}

/* ---------------------- order *5* deriv *0* -------------------------- */

static double
_bspl5d0_int(const double *parm) {
  AIR_UNUSED(parm);
  return 1.0;
}

#define BSPL5D0(ret, t, x)                                      \
  if (x < 1) {                                                  \
    t = x*x;                                                    \
    ret = (33 - 5*t*(6 + (x-3)*t))/60;                          \
  } else if (x < 2) {                                           \
    ret = (51 + 5*x*(15 + x*(-42 + x*(30 + (-9 + x)*x))))/120;  \
  } else if (x < 3) {                                           \
    t = x - 3;                                                  \
    ret = -t*t*t*t*t/120;                                       \
  } else {                                                      \
    ret = 0;                                                    \
  }

static double
_bspl5d0_1d(double x, const double *parm) {
  double ax, tmp, r;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL5D0(r, tmp, ax);
  return r;
}

static float
_bspl5d0_1f(float x, const double *parm) {
  float ax, tmp, r;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL5D0(r, tmp, ax);
  return r;
}

static void
_bspl5d0_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, tmp, r;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = x[i]; ax = AIR_ABS(ax);
    BSPL5D0(r, tmp, ax);
    f[i] = r;
  }
}

static void
_bspl5d0_Nf(float *f, const float *x, size_t len, const double *parm) {
  float ax, tmp, r;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = x[i]; ax = AIR_ABS(ax);
    BSPL5D0(r, tmp, ax);
    f[i] = r;
  }
}

NrrdKernel
_nrrdKernelBSpline5 = {
  "bspl5",
  BSPL_DECL(5, 0)
};
NrrdKernel *const
nrrdKernelBSpline5 = &_nrrdKernelBSpline5;

/* ---------------------- order *5* deriv *1* -------------------------- */

static double
_bspl5d1_int(const double *parm) {
  AIR_UNUSED(parm);
  return 0.0;
}

#define BSPL5D1(ret, t, x)                              \
  if (x < 1) {                                          \
    t = x*x*x;                                          \
    ret = -x + t - (5*t*x)/12;                          \
  } else if (x < 2) {                                   \
    ret = (15 + x*(-84 + x*(90 + x*(-36 + 5*x))))/24;   \
  } else if (x < 3) {                                   \
    t = -3 + x;                                         \
    ret = -t*t*t*t/24;                                  \
  } else {                                              \
    ret = 0;                                            \
  }

static double
_bspl5d1_1d(double x, const double *parm) {
  double ax, tmp, r;
  int sgn;
  AIR_UNUSED(parm);

  ABS_SGN(ax, sgn, x);
  BSPL5D1(r, tmp, ax);
  return sgn*r;
}

static float
_bspl5d1_1f(float x, const double *parm) {
  float ax, tmp, r;
  int sgn;
  AIR_UNUSED(parm);

  ABS_SGN(ax, sgn, x);
  BSPL5D1(r, tmp, ax);
  return sgn*r;
}

static void
_bspl5d1_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, tmp, r;
  int sgn;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ABS_SGN(ax, sgn, x[i]);
    BSPL5D1(r, tmp, ax);
    f[i] = sgn*r;
  }
}

static void
_bspl5d1_Nf(float *f, const float *x, size_t len, const double *parm) {
  float ax, tmp, r;
  int sgn;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ABS_SGN(ax, sgn, x[i]);
    BSPL5D1(r, tmp, ax);
    f[i] = sgn*r;
  }
}

NrrdKernel
_nrrdKernelBSpline5D = {
  "bspl5d",
  BSPL_DECL(5, 1)
};
NrrdKernel *const
nrrdKernelBSpline5D = &_nrrdKernelBSpline5D;

/* ---------------------- order *5* deriv *2* -------------------------- */

static double
_bspl5d2_int(const double *parm) {
  AIR_UNUSED(parm);
  return 0.0;
}

#define BSPL5D2(ret, t, x)                      \
  if (x < 1) {                                  \
    t = x*x;                                    \
    ret = -1 + 3*t - (5*t*x)/3;                 \
  } else if (x < 2) {                           \
    ret = (-21 + x*(45 + x*(-27 + 5*x)))/6;     \
  } else if (x < 3) {                           \
    t = -3 + x;                                 \
    ret = -t*t*t/6;                             \
  } else {                                      \
    ret = 0;                                    \
  }

static double
_bspl5d2_1d(double x, const double *parm) {
  double ax, tmp, r;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL5D2(r, tmp, ax);
  return r;
}

static float
_bspl5d2_1f(float x, const double *parm) {
  float ax, tmp, r;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL5D2(r, tmp, ax);
  return r;
}

static void
_bspl5d2_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, tmp, r;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = AIR_ABS(x[i]);
    BSPL5D2(r, tmp, ax);
    f[i] = r;
  }
}

static void
_bspl5d2_Nf(float *f, const float *x, size_t len, const double *parm) {
  float ax, tmp, r;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = AIR_ABS(x[i]);
    BSPL5D2(r, tmp, ax);
    f[i] = r;
  }
}

NrrdKernel
_nrrdKernelBSpline5DD = {
  "bspl5dd",
  BSPL_DECL(5, 2)
};
NrrdKernel *const
nrrdKernelBSpline5DD = &_nrrdKernelBSpline5DD;

