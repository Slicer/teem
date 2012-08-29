/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2012, 2011, 2010, 2009  University of Chicago
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

static NrrdKernel
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

static NrrdKernel
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

static NrrdKernel
_nrrdKernelBSpline3DD = {
  "bspl3dd",
  BSPL_DECL(3, 2)
};
NrrdKernel *const
nrrdKernelBSpline3DD = &_nrrdKernelBSpline3DD;

/* ---------------------- order *3* deriv *3* -------------------------- */

static double
_bspl3d3_int(const double *parm) {
  AIR_UNUSED(parm);
  return 0.0;
}

/* t: tmp; ax: abs(x) */
#define BSPL3D3(ret, t, x)                     \
  if (x < 1) {                                 \
    ret = 3;                                   \
  } else if (x < 2) {                          \
    ret = -1;                                  \
  } else {                                     \
    ret = 0;                                   \
  }

/* note that the tmp arg is not needed or used in the macro,
   so we can pass any bogus value, in this case 42.42 */
static double
_bspl3d3_1d(double x, const double *parm) {
  double ax, r;
  int sgn;
  AIR_UNUSED(parm);

  ABS_SGN(ax, sgn, x);
  BSPL3D3(r, 42.42, ax);
  return sgn*r;
}

static float
_bspl3d3_1f(float x, const double *parm) {
  float ax, r;
  int sgn;
  AIR_UNUSED(parm);

  ABS_SGN(ax, sgn, x);
  BSPL3D3(r, 42.42, ax);
  return sgn*r;
}

static void
_bspl3d3_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, r;
  int sgn;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ABS_SGN(ax, sgn, x[i]);
    BSPL3D3(r, 42.42, ax);
    f[i] = sgn*r;
  }
}

static void
_bspl3d3_Nf(float *f, const float *x, size_t len, const double *parm) {
  float ax, r;
  int sgn;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ABS_SGN(ax, sgn, x[i]);
    BSPL3D3(r, 42.42, ax);
    f[i] = sgn*r;
  }
}

static NrrdKernel
_nrrdKernelBSpline3DDD = {
  "bspl3ddd",
  BSPL_DECL(3, 3)
};
NrrdKernel *const
nrrdKernelBSpline3DDD = &_nrrdKernelBSpline3DDD;

/* ------------- order *3* approximate numerical inverse -------------- */
/* still need to implement:
**   Unser et al B-Spline Signal Processing: Part I & II, IEEE
**   Transactions on Signal Processing, 1993, 41(2):821-833, 834--848
** but until then here's a slower way of approximating the prefiltering,
** which is still faster than doing iterative deconvolution.  These
** weights were determined by GLK with Mathematica, by inverting the
** matrix representing discrete convolution with the spline
**
** Note that with all the approx inverse kernels, the support really
** does end at a half-integer (they are piece-wise constant on unit 
** intervals centered at integers)
*/

#define BSPL3_AI_LEN 12
static double
_bspl3_ANI_kvals[BSPL3_AI_LEN] = {
  2672279.0/1542841.0,
  -(716035.0/1542841.0),
  191861.0/1542841.0,
  -(51409.0/1542841.0),
  13775.0/1542841.0,
  -(3691.0/1542841.0),
  989.0/1542841.0,
  -(265.0/1542841.0),
  71.0/1542841.0,
  -(19.0/1542841.0),
  5.0/1542841.0,
  -(1.0/1542841.0)};

static double
_bspl3_ANI_sup(const double *parm) {
  AIR_UNUSED(parm);
  return BSPL3_AI_LEN + 0.5;
}

static double
_bspl3_ANI_int(const double *parm) {
  AIR_UNUSED(parm);
  return 1.0;
}

#define BSPL3_ANI(ret, tmp, x)                  \
  tmp = AIR_CAST(unsigned int, x+0.5);          \
  if (tmp < BSPL3_AI_LEN) {                     \
    ret = _bspl3_ANI_kvals[tmp];                \
  } else {                                      \
    ret = 0.0;                                  \
  }

static double
_bspl3_ANI_1d(double x, const double *parm) {
  double ax, r; int tmp;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL3_ANI(r, tmp, ax);
  return r;
}

static float
_bspl3_ANI_1f(float x, const double *parm) {
  double ax, r; int tmp;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL3_ANI(r, tmp, ax);
  return AIR_CAST(float, r);
}

static void
_bspl3_ANI_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, r; int tmp;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = x[i]; ax = AIR_ABS(ax);
    BSPL3_ANI(r, tmp, ax);
    f[i] = r;
  }
}

static void
_bspl3_ANI_Nf(float *f, const float *x, size_t len, const double *parm) {
  double ax, r; int tmp;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = x[i]; ax = AIR_ABS(ax);
    BSPL3_ANI(r, tmp, ax);
    f[i] = AIR_CAST(float, r);
  }
}

static NrrdKernel
_nrrdKernelBSpline3ApproxInverse = {
  "bspl3ai", 0,
  _bspl3_ANI_sup, _bspl3_ANI_int,
  _bspl3_ANI_1f, _bspl3_ANI_Nf,
  _bspl3_ANI_1d, _bspl3_ANI_Nd
};
NrrdKernel *const
nrrdKernelBSpline3ApproxInverse = &_nrrdKernelBSpline3ApproxInverse;

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

static NrrdKernel
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

static NrrdKernel
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

static NrrdKernel
_nrrdKernelBSpline5DD = {
  "bspl5dd",
  BSPL_DECL(5, 2)
};
NrrdKernel *const
nrrdKernelBSpline5DD = &_nrrdKernelBSpline5DD;

/* ---------------------- order *5* deriv *3* -------------------------- */

static double
_bspl5d3_int(const double *parm) {
  AIR_UNUSED(parm);
  return 0.0;
}

#define BSPL5D3(ret, t, x)                              \
  if (x < 1) {                                          \
    ret = (6 - 5*x)*x;                                  \
  } else if (x < 2) {                                   \
    ret = 15.0/2.0 - 9*x + 5*x*x/2;                     \
  } else if (x < 3) {                                   \
    t = -3 + x;                                         \
    ret = -t*t/2;                                        \
  } else {                                              \
    ret = 0;                                            \
  }

static double
_bspl5d3_1d(double x, const double *parm) {
  double ax, tmp, r;
  int sgn;
  AIR_UNUSED(parm);

  ABS_SGN(ax, sgn, x);
  BSPL5D3(r, tmp, ax);
  return sgn*r;
}

static float
_bspl5d3_1f(float x, const double *parm) {
  float ax, tmp, r;
  int sgn;
  AIR_UNUSED(parm);

  ABS_SGN(ax, sgn, x);
  BSPL5D3(r, tmp, ax);
  return sgn*r;
}

static void
_bspl5d3_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, tmp, r;
  int sgn;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ABS_SGN(ax, sgn, x[i]);
    BSPL5D3(r, tmp, ax);
    f[i] = sgn*r;
  }
}

static void
_bspl5d3_Nf(float *f, const float *x, size_t len, const double *parm) {
  float ax, tmp, r;
  int sgn;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ABS_SGN(ax, sgn, x[i]);
    BSPL5D3(r, tmp, ax);
    f[i] = sgn*r;
  }
}

static NrrdKernel
_nrrdKernelBSpline5DDD = {
  "bspl5ddd",
  BSPL_DECL(5, 3)
};
NrrdKernel *const
nrrdKernelBSpline5DDD = &_nrrdKernelBSpline5DDD;

/* ------------- order *5* approximate numerical inverse -------------- */

#define BSPL5_AI_LEN 19
static double
_bspl5_ANI_kvals[BSPL5_AI_LEN] = {
  2.842170922021427870236333,
  -1.321729472987239796417307,
  0.5733258709611149890510146,
  -0.2470419274010479815114381,
  0.1063780046404650785440854,
  -0.04580408418467518130037713,
  0.01972212399699206014654736,
  -0.008491860984275658620122180,
  0.003656385950780789716770681,
  -0.001574349495225446217828165,
  0.0006778757185045443332966769,
  -0.0002918757322635763049702028,
  0.0001256725426338698784062181,
  -0.00005410696497728715841372199,
  0.00002328659592249373987497103,
  -0.00001000218170092531503506361,
  4.249940115067599514119408e-6,
  -1.698979738236873388431330e-6,
  4.475539012615912040164139e-7};

static double
_bspl5_ANI_sup(const double *parm) {
  AIR_UNUSED(parm);
  return BSPL5_AI_LEN + 0.5;
}

static double
_bspl5_ANI_int(const double *parm) {
  AIR_UNUSED(parm);
  return 1.0;
}

#define BSPL5_ANI(ret, tmp, x)                  \
  tmp = AIR_CAST(unsigned int, x+0.5);          \
  if (tmp < BSPL5_AI_LEN) {                     \
    ret = _bspl5_ANI_kvals[tmp];                \
  } else {                                      \
    ret = 0.0;                                  \
  }

static double
_bspl5_ANI_1d(double x, const double *parm) {
  double ax, r; int tmp;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL5_ANI(r, tmp, ax);
  return r;
}

static float
_bspl5_ANI_1f(float x, const double *parm) {
  double ax, r; int tmp;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL5_ANI(r, tmp, ax);
  return AIR_CAST(float, r);
}

static void
_bspl5_ANI_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, r; int tmp;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = x[i]; ax = AIR_ABS(ax);
    BSPL5_ANI(r, tmp, ax);
    f[i] = r;
  }
}

static void
_bspl5_ANI_Nf(float *f, const float *x, size_t len, const double *parm) {
  double ax, r; int tmp;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = x[i]; ax = AIR_ABS(ax);
    BSPL5_ANI(r, tmp, ax);
    f[i] = AIR_CAST(float, r);
  }
}

static NrrdKernel
_nrrdKernelBSpline5ApproxInverse = {
  "bspl5ai", 0,
  _bspl5_ANI_sup, _bspl5_ANI_int,
  _bspl5_ANI_1f, _bspl5_ANI_Nf,
  _bspl5_ANI_1d, _bspl5_ANI_Nd
};
NrrdKernel *const
nrrdKernelBSpline5ApproxInverse = &_nrrdKernelBSpline5ApproxInverse;

/* ============================= order *6* ============================= */

/*
static double
_bspl6_sup(const double *parm) {
  AIR_UNUSED(parm);
  return 3.5;
}
*/

/* ============================= order *7* ============================= */

static double
_bspl7_sup(const double *parm) {
  AIR_UNUSED(parm);
  return 4.0;
}

/* ---------------------- order *7* deriv *0* -------------------------- */

static double
_bspl7d0_int(const double *parm) {
  AIR_UNUSED(parm);
  return 1.0;
}

#define BSPL7D0(ret, t, x)                                              \
  if (x < 1) {                                                          \
    ret = 151.0/315.0 + x*x*(-48.0 + x*x*(16.0 + x*x*(-4 + x)))/144.0;  \
  } else if (x < 2) {                                                   \
    ret = (2472 - 7*x*(56 + x*(72 + x*(280 + 3*(-6 + x)*x*(20 + (-6 + x)*x)))))/5040.0; \
  } else if (x < 3) {                                                   \
    ret = (-1112 + 7*x*(1736 + x*(-2760 + x*(1960 + x*(-760 + x*(168 + (-20 + x)*x))))))/5040.0; \
  } else if (x < 4) {                                                   \
    t = x - 4;                                                          \
    ret = -t*t*t*t*t*t*t/5040;                                          \
  } else {                                                              \
    ret = 0;                                                            \
  }

static double
_bspl7d0_1d(double x, const double *parm) {
  double ax, tmp, r;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL7D0(r, tmp, ax);
  return r;
}

static float
_bspl7d0_1f(float x, const double *parm) {
  float ax, tmp, r;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL7D0(r, tmp, ax);
  return r;
}

static void
_bspl7d0_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, tmp, r;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = x[i]; ax = AIR_ABS(ax);
    BSPL7D0(r, tmp, ax);
    f[i] = r;
  }
}

static void
_bspl7d0_Nf(float *f, const float *x, size_t len, const double *parm) {
  float ax, tmp, r;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = x[i]; ax = AIR_ABS(ax);
    BSPL7D0(r, tmp, ax);
    f[i] = r;
  }
}

static NrrdKernel
_nrrdKernelBSpline7 = {
  "bspl7",
  BSPL_DECL(7, 0)
};
NrrdKernel *const
nrrdKernelBSpline7 = &_nrrdKernelBSpline7;

/* ---------------------- order *7* deriv *1* -------------------------- */

static double
_bspl7d1_int(const double *parm) {
  AIR_UNUSED(parm);
  return 0.0;
}

#define BSPL7D1(ret, t, x)                                              \
  if (x < 1) {                                                          \
    ret = x*(-96.0 + x*x*(64.0 + x*x*(-24.0 + 7.0*x)))/144.0;           \
  } else if (x < 2) {                                                   \
    ret = -7.0/90.0 - (-2 + x)*x*(-24 + (-2 + x)*x*(76 + x*(-44 + 7*x)))/240.0; \
  } else if (x < 3) {                                                   \
    ret = (2 + (-4 + x)*x)*(868 + x*(-1024 + x*(458 + x*(-92 + 7*x))))/720.0; \
  } else if (x < 4) {                                                   \
    t = -4 + x;                                                         \
    ret = -t*t*t*t*t*t/720;                                             \
  } else {                                                              \
    ret = 0.0;                                                          \
  }

static double
_bspl7d1_1d(double x, const double *parm) {
  double ax, tmp, r;
  int sgn;
  AIR_UNUSED(parm);

  ABS_SGN(ax, sgn, x);
  BSPL7D1(r, tmp, ax);
  return sgn*r;
}

static float
_bspl7d1_1f(float x, const double *parm) {
  float ax, tmp, r;
  int sgn;
  AIR_UNUSED(parm);

  ABS_SGN(ax, sgn, x);
  BSPL7D1(r, tmp, ax);
  return sgn*r;
}

static void
_bspl7d1_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, tmp, r;
  int sgn;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ABS_SGN(ax, sgn, x[i]);
    BSPL7D1(r, tmp, ax);
    f[i] = sgn*r;
  }
}

static void
_bspl7d1_Nf(float *f, const float *x, size_t len, const double *parm) {
  float ax, tmp, r;
  int sgn;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ABS_SGN(ax, sgn, x[i]);
    BSPL7D1(r, tmp, ax);
    f[i] = sgn*r;
  }
}

static NrrdKernel
_nrrdKernelBSpline7D = {
  "bspl7d",
  BSPL_DECL(7, 1)
};
NrrdKernel *const
nrrdKernelBSpline7D = &_nrrdKernelBSpline7D;

/* ---------------------- order *7* deriv *2* -------------------------- */

static double
_bspl7d2_int(const double *parm) {
  AIR_UNUSED(parm);
  return 0.0;
}

#define BSPL7D2(ret, t, x)                                              \
  if (x < 1) {                                                          \
    ret = (-16.0 + x*x*(32 + x*x*(-20 + 7*x)))/24.0;                    \
  } else if (x < 2) {                                                   \
    ret = -1.0/5.0 - 7*x/3 + 6*x*x - 14*x*x*x/3 + 3*x*x*x*x/2 - 7*x*x*x*x*x/40; \
  } else if (x < 3) {                                                   \
    ret = (-920 + x*(1960 + x*(-1520 + x*(560 + x*(-100 + 7*x)))))/120.0; \
  } else if (x < 4) {                                                   \
    t = -4 + x;                                                         \
    ret = -t*t*t*t*t/120;                                               \
  } else {                                                              \
    ret = 0;                                                            \
  }

static double
_bspl7d2_1d(double x, const double *parm) {
  double ax, tmp, r;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL7D2(r, tmp, ax);
  return r;
}

static float
_bspl7d2_1f(float x, const double *parm) {
  float ax, tmp, r;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL7D2(r, tmp, ax);
  return r;
}

static void
_bspl7d2_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, tmp, r;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = AIR_ABS(x[i]);
    BSPL7D2(r, tmp, ax);
    f[i] = r;
  }
}

static void
_bspl7d2_Nf(float *f, const float *x, size_t len, const double *parm) {
  float ax, tmp, r;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = AIR_ABS(x[i]);
    BSPL7D2(r, tmp, ax);
    f[i] = r;
  }
}

static NrrdKernel
_nrrdKernelBSpline7DD = {
  "bspl7dd",
  BSPL_DECL(7, 2)
};
NrrdKernel *const
nrrdKernelBSpline7DD = &_nrrdKernelBSpline7DD;

/* ---------------------- order *7* deriv *3* -------------------------- */

static double
_bspl7d3_int(const double *parm) {
  AIR_UNUSED(parm);
  return 0.0;
}

#define BSPL7D3(ret, t, x)                                              \
  if (x < 1) {                                                          \
    ret = x*(64 + 5*x*x*(-16 + 7*x))/24;                                \
  } else if (x < 2) {                                                   \
    ret = -7.0/3.0 + x*(12 + x*(-14 + x*(6 - 7*x/8)));                  \
  } else if (x < 3) {                                                   \
    ret = (392 + x*(-608 + x*(336 + x*(-80 + 7*x))))/24;                \
  } else if (x < 4) {                                                   \
    t = -4 + x;                                                         \
    ret = -t*t*t*t/24;                                                  \
  } else {                                                              \
    ret = 0.0;                                                          \
  }

static double
_bspl7d3_1d(double x, const double *parm) {
  double ax, tmp, r;
  int sgn;
  AIR_UNUSED(parm);

  ABS_SGN(ax, sgn, x);
  BSPL7D3(r, tmp, ax);
  return sgn*r;
}

static float
_bspl7d3_1f(float x, const double *parm) {
  float ax, tmp, r;
  int sgn;
  AIR_UNUSED(parm);

  ABS_SGN(ax, sgn, x);
  BSPL7D3(r, tmp, ax);
  return sgn*r;
}

static void
_bspl7d3_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, tmp, r;
  int sgn;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ABS_SGN(ax, sgn, x[i]);
    BSPL7D3(r, tmp, ax);
    f[i] = sgn*r;
  }
}

static void
_bspl7d3_Nf(float *f, const float *x, size_t len, const double *parm) {
  float ax, tmp, r;
  int sgn;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ABS_SGN(ax, sgn, x[i]);
    BSPL7D3(r, tmp, ax);
    f[i] = sgn*r;
  }
}

static NrrdKernel
_nrrdKernelBSpline7DDD = {
  "bspl7ddd",
  BSPL_DECL(7, 3)
};
NrrdKernel *const
nrrdKernelBSpline7DDD = &_nrrdKernelBSpline7DDD;

/* ------------- order *7* approximate numerical inverse -------------- */

#define BSPL7_AI_LEN 26
static double
_bspl7_ANI_kvals[BSPL7_AI_LEN] = {
  4.964732886301469059137801,
  -3.091042499769118182213297,                             
  1.707958936669135515487259,
  -0.9207818274511302808978934,
  0.4936786139601599067344824, 
  -0.2643548049418435742509870,
  0.1415160014538524997926456, 
  -0.07575222270391683956827192,
  0.04054886334181815702759984, 
  -0.02170503519322401084648773,
  0.01161828326728242899507066,
  -0.006219039932262414977444894,
  0.003328930278070297807163008,
  -0.001781910982713036390230280,
  0.0009538216015244754251250379,
  -0.0005105611456814427816916412,
  0.0002732917233015012426069489,
  -0.0001462845976614043380333786,
  0.00007829746549013888268504229,
  -0.00004190023413676309286922788,
  0.00002240807576972098806040711,
  -0.00001195669542335526044896263,
  6.329480796176889498331054e-6,
  -3.256910241436675950084186e-6,
  1.506132735770447868981087e-6,
  -4.260433183779953604188120e-7};

static double
_bspl7_ANI_sup(const double *parm) {
  AIR_UNUSED(parm);
  return BSPL7_AI_LEN + 0.5;
}

static double
_bspl7_ANI_int(const double *parm) {
  AIR_UNUSED(parm);
  return 1.0;
}

#define BSPL7_ANI(ret, tmp, x)                  \
  tmp = AIR_CAST(unsigned int, x+0.5);          \
  if (tmp < BSPL7_AI_LEN) {                     \
    ret = _bspl7_ANI_kvals[tmp];                \
  } else {                                      \
    ret = 0.0;                                  \
  }

static double
_bspl7_ANI_1d(double x, const double *parm) {
  double ax, r; int tmp;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL7_ANI(r, tmp, ax);
  return r;
}

static float
_bspl7_ANI_1f(float x, const double *parm) {
  double ax, r; int tmp;
  AIR_UNUSED(parm);

  ax = AIR_ABS(x);
  BSPL7_ANI(r, tmp, ax);
  return AIR_CAST(float, r);
}

static void
_bspl7_ANI_Nd(double *f, const double *x, size_t len, const double *parm) {
  double ax, r; int tmp;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = x[i]; ax = AIR_ABS(ax);
    BSPL7_ANI(r, tmp, ax);
    f[i] = r;
  }
}

static void
_bspl7_ANI_Nf(float *f, const float *x, size_t len, const double *parm) {
  double ax, r; int tmp;
  size_t i;
  AIR_UNUSED(parm);
  
  for (i=0; i<len; i++) {
    ax = x[i]; ax = AIR_ABS(ax);
    BSPL7_ANI(r, tmp, ax);
    f[i] = AIR_CAST(float, r);
  }
}

static NrrdKernel
_nrrdKernelBSpline7ApproxInverse = {
  "bspl7ai", 0,
  _bspl7_ANI_sup, _bspl7_ANI_int,
  _bspl7_ANI_1f, _bspl7_ANI_Nf,
  _bspl7_ANI_1d, _bspl7_ANI_Nd
};
NrrdKernel *const
nrrdKernelBSpline7ApproxInverse = &_nrrdKernelBSpline7ApproxInverse;

