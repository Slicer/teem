/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "nrrd.h"

/*
[Mon Jun 19 05:31:14 CDT 2023] Much of the content in here had been essentially unchanged
since revision 1327 2003-03-30 07:21:37 ("hello to the first windowed sincs"), and the
tweaks that came in the following weeks (r1328, r1329, r1346), including r1347 that
introduced using Taylor expansions zero 0, to avoid numerical instabilities.

A big bug originated in one of those early edits, wherein the _HANN macro (basis for
evaluating nrrdKernelHann) had an incorrect Taylor expansion, which meant that it
evaluated to 1.1 near 0, not 1.0; yikes!  Recent work of revisiting the various kernels
prior to a 1.13 release discovered this.

The math implementation of these kernels has been redone to fix that bug, and to make an
effort at improving numerical stability, now that GLK understand that topic better than
20 years ago. The Hann- and Blackman-windowed sinc kernels (nrrdKernelHann and
nrrdKernelBlackman) themselves did not actually need the near-zero Taylor approximation;
their numerical stability was fine. But the *derivatives* of these kernels do benefit
from a Taylor approximation near zero, which is used here. Further from zero, the
numerical stability of the derivative kernels is still pretty lousy compared
to the niceness of the other piece-wise polynomial kernels in nrrd.  For the usual
purposes of image resampling and reconstruction, that instability may not be the biggest
problem, but it should still be revisited and improved further.

The transition between the Taylor approximation and the regular evaluation (for the 1st
and 2nd derivatives of Hann and Blackmann) has been set up to be C0 continuous, as
follows.  The Taylor approximation is made of two terms, and is used whenever |x| is
below some threshold "CUT" (that does not depend on radius R, in contrast to
earlier attempts at this). The higher of the two Taylor terms is scaled (as a function of
R) so that the approximation and the correct formula agree exactly at CUT. A second
macro for that scaling function ("scl") is passed to the function bodies for all the
kernel methods. Getting that math right has benefited from an extra sanity check to be
added to the these kernels: the radius parameter (parm[1]) is now clamped from below to
be 2: you didn't really want a windowed sinc if you're not letting it ring at least once.

All the large math expressions are from manual massaging of copying from Mathematica.
In double-precision, the absolute relative error of the Taylor approximations (in the 1st
and 2nd derivatives of nrrdKernelHann and nrrdKernelBlackman) is less than 3e-8, but that
is the error bound for the minimum radius parm[1] of 2: the error goes down (the tweaked
Taylor approximation becomes more accurate) with a larger radius.

Some initial efforts were made to create versions that only compute in single-precision
float (with more type-specific arguments to macros, like cosf,sinf instead of cos,sin),
but even before that was finished, it was clear that the numerical quality was total
garbage, especially for the kernel derivatives - these formula are just so gangly that
there are too many occasions for rounding error for single-precision to work usefully.
Totally different approximation schemes (that do not ever rely on the analytic formulae)
may fare better.  Hence the kernel function bodies for the _1_f and _N_f methods don't
even bother pretending to work in single-precision, they just use double precision for
everything.

GLK's daughter was party to some of this coding, as noted.

https://en.wikipedia.org/wiki/Window_function#Hann_and_Hamming_windows
https://en.wikipedia.org/wiki/Window_function#Blackman_window
*/

static double
_nrrdWindSincInt(const double *parm) {

  AIR_UNUSED(parm);
  /* This isn't true, but there aren't good accurate, closed-form
     approximations for these integrals ... */
  return 1.0;
}

static double
_nrrdDWindSincInt(const double *parm) {

  AIR_UNUSED(parm);
  /* ... or their derivatives */
  return 0.0;
}

static double
_nrrdWindSincSup(const double *parm) {
  double S;

  S = parm[0];
  return AIR_MAX(2, parm[1]) * S;
}

#define POW1(S) (S)
#define POW2(S) ((S) * (S))
#define POW3(S) ((S) * (S) * (S))

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define PI   3.1415926535897932385
#define PIPI 9.8696044010893586188 /* Pi * Pi */
/* CUT == 1/(32*Pi) == transition between Taylor approx and full formula */
#define CUT  0.0099471839432434584856

/* NOTE these function bodies have some local variables that simplify expressing the
   windowed sincs and their derivatives:
   ax = abs(x)
   px = PI*x  (set by macros)
  */
#define WS_1_F(name, kernelmacro, sclmacro, spow)                                       \
  static float _nrrd##name##_1_f(float _x, const double *parm) {                        \
    double R, S, scl, x, ax, px;                                                        \
    AIR_UNUSED(px);                                                                     \
    S = parm[0];                                                                        \
    R = parm[1];                                                                        \
    R = AIR_MAX(R, 2);                                                                  \
    scl = sclmacro(R);                                                                  \
    AIR_UNUSED(scl);                                                                    \
    x = AIR_CAST(double, _x) / S;                                                       \
    ax = ABS(x);                                                                        \
    return AIR_FLOAT(kernelmacro(float, x, R, scl) / spow(S));                          \
  }

#define WS_N_F(name, kernelmacro, sclmacro, spow)                                       \
  static void _nrrd##name##_N_f(float *f, const float *xvec, size_t len,                \
                                const double *parm) {                                   \
    double S, R, scl, x, ax, px;                                                        \
    size_t i;                                                                           \
    AIR_UNUSED(px);                                                                     \
    S = parm[0];                                                                        \
    R = parm[1];                                                                        \
    R = AIR_MAX(R, 2);                                                                  \
    scl = sclmacro(R);                                                                  \
    AIR_UNUSED(scl);                                                                    \
    for (i = 0; i < len; i++) {                                                         \
      x = AIR_CAST(double, xvec[i]) / S;                                                \
      ax = ABS(x);                                                                      \
      f[i] = AIR_FLOAT(kernelmacro(float, x, R, scl) / spow(S));                        \
    }                                                                                   \
  }

#define WS_1_D(name, kernelmacro, sclmacro, spow)                                       \
  static double _nrrd##name##_1_d(double x, const double *parm) {                       \
    double R, S, scl, ax, px;                                                           \
    AIR_UNUSED(px);                                                                     \
    S = parm[0];                                                                        \
    R = parm[1];                                                                        \
    R = AIR_MAX(R, 2);                                                                  \
    scl = sclmacro(R);                                                                  \
    AIR_UNUSED(scl);                                                                    \
    x /= S;                                                                             \
    ax = ABS(x);                                                                        \
    return kernelmacro(double, x, R, scl) / spow(S);                                    \
  }

#define WS_N_D(name, kernelmacro, sclmacro, spow)                                       \
  static void _nrrd##name##_N_d(double *f, const double *xvec, size_t len,              \
                                const double *parm) {                                   \
    double S, R, scl, x, ax, px;                                                        \
    size_t i;                                                                           \
    AIR_UNUSED(px);                                                                     \
    S = parm[0];                                                                        \
    R = parm[1];                                                                        \
    R = AIR_MAX(R, 2);                                                                  \
    scl = sclmacro(R);                                                                  \
    AIR_UNUSED(scl);                                                                    \
    for (i = 0; i < len; i++) {                                                         \
      x = xvec[i] / S;                                                                  \
      ax = ABS(x);                                                                      \
      f[i] = kernelmacro(double, x, R, scl) / spow(S);                                  \
    }                                                                                   \
  }

/* ------------------------------------------------------------ */

/* http://www.plunk.org/~hatch/rightway.html on sin(x)/x */
#define SINC (1 + px == 1 ? 1 : sin(px) / px)

#define _HANN(T, x, R, SCL)                                                             \
  (ax < R            /* inside radius of support */                                     \
     ? (px = PI * x, /* */                                                              \
        (1 + cos(px / R)) * SINC / 2)                                                   \
     : 0)

#define SCL_ONE(R) 1

WS_1_D(Hann, _HANN, SCL_ONE, POW1)
WS_1_F(Hann, _HANN, SCL_ONE, POW1)
WS_N_F(Hann, _HANN, SCL_ONE, POW1)
WS_N_D(Hann, _HANN, SCL_ONE, POW1)

static const NrrdKernel _nrrdKernelHann = {"hann",           2,
                                           _nrrdWindSincSup, _nrrdWindSincInt,
                                           _nrrdHann_1_f,    _nrrdHann_N_f,
                                           _nrrdHann_1_d,    _nrrdHann_N_d};
const NrrdKernel *const nrrdKernelHann = &_nrrdKernelHann;

/* ------------------------------------------------------------ */

#define _DHANN(T, x, R, SCL)                                                            \
  (ax < CUT /* where to use 3rd-order Taylor expansion;                                 \
              :-)  ^__^ sometimes our cat is an evil potato MEEWW  -Jane */             \
     ? x * PIPI                                                                         \
         * (-3 - 2 * R * R                                                              \
            + SCL * (PIPI * (5 + 2 * R * R * (5 + R * R)) * x * x) / (10 * R * R))      \
         / (6 * R * R)                                                                  \
     : (ax < R            /* inside radius of support */                                \
          ? (px = PI * x, /* local vars */                                              \
             (R * (1 + cos(px / R)) * (px * cos(px) - sin(px))                          \
              - px * sin(px) * sin(px / R))                                             \
               / (2 * PI * R * x * x))                                                  \
          : 0))

#define _DHANN_SCL(R)                                                                   \
  (10240 * R * R                                                                        \
   * (3 + R * R * (2 + 3072 * cos(1.0 / 32))                                            \
      + 3072 * R                                                                        \
          * (R * cos(1.0 / (32 * R)) * (cos(1.0 / 32) - 32 * sin(1.0 / 32))             \
             - sin(1.0 / 32) * (32 * R + sin(1.0 / (32 * R))))))                        \
    / (5 + 2 * R * R * (5 + R * R))

WS_1_D(DHann, _DHANN, _DHANN_SCL, POW2)
WS_1_F(DHann, _DHANN, _DHANN_SCL, POW2)
WS_N_F(DHann, _DHANN, _DHANN_SCL, POW2)
WS_N_D(DHann, _DHANN, _DHANN_SCL, POW2)

static const NrrdKernel _nrrdKernelDHann = {"hannD",          2,
                                            _nrrdWindSincSup, _nrrdDWindSincInt,
                                            _nrrdDHann_1_f,   _nrrdDHann_N_f,
                                            _nrrdDHann_1_d,   _nrrdDHann_N_d};
const NrrdKernel *const nrrdKernelHannD = &_nrrdKernelDHann;

/* ------------------------------------------------------------ */

#define _DDHANN(T, x, R, SCL)                                                           \
  (ax < CUT /* when to use Taylor */                                                    \
     ? PIPI                                                                             \
         * (-(3 + 2 * R * R) / 3                                                        \
            + SCL * ((PIPI * (5 + 2 * R * R * (5 + R * R)) * x * x) / (10 * R * R)))    \
         / (2 * R * R)                                                                  \
     : (ax < R            /* inside support */                                          \
          ? (px = PI * x, /* local var for analytic formula */                          \
             (-2 * R * px * cos(px) * (R + R * cos(px / R) + px * sin(px / R))          \
              + sin(px)                                                                 \
                  * (-((-2 * R * R + (1 + R * R) * px * px) * cos(px / R))              \
                     + R * (2 * R - R * px * px + 2 * px * sin(px / R))))               \
               / (2 * R * R * px * x * x))                                              \
          : 0))

#define _DDHANN_SCL(R)                                                                  \
  (10240 * R * R                                                                        \
   * (3                                                                                 \
      - 96 * cos(1.0 / (32 * R))                                                        \
          * (R * R * (64 * cos(1.0 / 32) - 2047 * sin(1.0 / 32)) + sin(1.0 / 32))       \
      + 2 * R * R * (1 - 3072 * cos(1.0 / 32) + 98256 * sin(1.0 / 32))                  \
      - 192 * R * (cos(1.0 / 32) - 32 * sin(1.0 / 32)) * sin(1.0 / (32 * R)))           \
   / (15 + 6 * R * R * (5 + R * R)))

WS_1_D(DDHann, _DDHANN, _DDHANN_SCL, POW3)
WS_1_F(DDHann, _DDHANN, _DDHANN_SCL, POW3)
WS_N_F(DDHann, _DDHANN, _DDHANN_SCL, POW3)
WS_N_D(DDHann, _DDHANN, _DDHANN_SCL, POW3)

static const NrrdKernel _nrrdKernelDDHann = {"hannDD",         2,
                                             _nrrdWindSincSup, _nrrdDWindSincInt,
                                             _nrrdDDHann_1_f,  _nrrdDDHann_N_f,
                                             _nrrdDDHann_1_d,  _nrrdDDHann_N_d};
const NrrdKernel *const nrrdKernelHannDD = &_nrrdKernelDDHann;

/* ------------------------------------------------------------ */
#define _BLACK(T, x, R, SCL)                                                            \
  (ax < R            /* inside support */                                               \
     ? (px = PI * x, /* */                                                              \
        (21 * 1.0 / 50 + cos(px / R) / 2 + 2 * cos((2 * px) / R) / 25) * SINC)          \
     : 0)

WS_1_D(Black, _BLACK, SCL_ONE, POW1)
WS_1_F(Black, _BLACK, SCL_ONE, POW1)
WS_N_F(Black, _BLACK, SCL_ONE, POW1)
WS_N_D(Black, _BLACK, SCL_ONE, POW1)

static const NrrdKernel _nrrdKernelBlackman = {"blackman",       2,
                                               _nrrdWindSincSup, _nrrdWindSincInt,
                                               _nrrdBlack_1_f,   _nrrdBlack_N_f,
                                               _nrrdBlack_1_d,   _nrrdBlack_N_d};
const NrrdKernel *const nrrdKernelBlackman = &_nrrdKernelBlackman;

/* ------------------------------------------------------------ */

#define _DBLACK(T, x, R, SCL)                                                           \
  (ax < CUT /* when to use Taylor */                                                    \
     ? (PI * PI * x                                                                     \
        * ((-50 - 123 / (R * R))                                                        \
           + SCL * (PIPI * (89 + 82 * R * R + 10 * R * R * R * R) * x * x)              \
               / (2 * R * R * R * R))                                                   \
        / 150)                                                                          \
     : (ax < R            /* inside support */                                          \
          ? (px = PI * x, /* local var for analytic formula */                          \
             ((R * px * cos(px) * (21 + 25 * cos(px / R) + 4 * cos(2 * px / R))         \
               - sin(px)                                                                \
                   * (21 * R + 25 * R * cos(px / R) + 4 * R * cos(2 * px / R)           \
                      + 25 * px * sin(px / R) + 8 * px * sin(2 * px / R)))              \
              / (50 * R * px * x)))                                                     \
          : 0))

#define _DBLACK_SCL(R)                                                                  \
  ((2048 * R * R                                                                        \
    * (123 + R * R * (50 + 64512 * cos(1.0 / 32))                                       \
       + 3072 * R                                                                       \
           * (25 * R * cos(1.0 / (32 * R)) * (cos(1.0 / 32) - 32 * sin(1.0 / 32))       \
              + 4 * R * cos(1.0 / (16 * R)) * (cos(1.0 / 32) - 32 * sin(1.0 / 32))      \
              - sin(1.0 / 32)                                                           \
                  * (672 * R + 25 * sin(1.0 / (32 * R)) + 8 * sin(1.0 / (16 * R))))))   \
   / (89 + 82 * R * R + 10 * R * R * R * R))

WS_1_D(DBlack, _DBLACK, _DBLACK_SCL, POW2)
WS_1_F(DBlack, _DBLACK, _DBLACK_SCL, POW2)
WS_N_F(DBlack, _DBLACK, _DBLACK_SCL, POW2)
WS_N_D(DBlack, _DBLACK, _DBLACK_SCL, POW2)

static const NrrdKernel _nrrdKernelDBlack = {"blackmanD",      2,
                                             _nrrdWindSincSup, _nrrdDWindSincInt,
                                             _nrrdDBlack_1_f,  _nrrdDBlack_N_f,
                                             _nrrdDBlack_1_d,  _nrrdDBlack_N_d};
const NrrdKernel *const nrrdKernelBlackmanD = &_nrrdKernelDBlack;

/* ------------------------------------------------------------ */

#define _DDBLACK(T, x, R, SCL)                                                          \
  (ax < CUT /* when to use Taylor */                                                    \
     ? (PIPI                                                                            \
        * ((-50 - 123 / (R * R)) / 3                                                    \
           + SCL * (PIPI * (89 + 82 * R * R + 10 * R * R * R * R) * x * x)              \
               / (2 * R * R * R * R))                                                   \
        / 50)                                                                           \
     : (ax < R            /* inside support */                                          \
          ? (px = PI * x, /* local var for analytic formula */                          \
             ((-2 * R * px * cos(px)                                                    \
                 * (21 * R + 25 * R * cos(px / R) + 4 * R * cos((2 * px) / R)           \
                    + 25 * px * sin(px / R) + 8 * px * sin((2 * px) / R))               \
               + sin(px)                                                                \
                   * (-25 * (-2 * R * R + (1 + R * R) * px * px) * cos(px / R)          \
                      - 4 * (-2 * R * R + (4 + R * R) * px * px) * cos((2 * px) / R)    \
                      + R                                                               \
                          * (42 * R - 21 * R * px * px + 50 * px * sin(px / R)          \
                             + 16 * px * sin((2 * px) / R))))                           \
              / (50 * R * R * px * x * x)))                                             \
          : 0))

#define _DDBLACK_SCL(R)                                                                 \
  (2048 * R * R                                                                         \
   * (123 + R * R * (50 - 129024 * cos(1.0 / 32))                                       \
      + 96                                                                              \
          * (-25 * cos(1.0 / (32 * R))                                                  \
               * (R * R * (64 * cos(1.0 / 32) - 2047 * sin(1.0 / 32)) + sin(1.0 / 32))  \
             - 4 * cos(1.0 / (16 * R))                                                  \
                 * (64 * R * R * cos(1.0 / 32) + (4 - 2047 * R * R) * sin(1.0 / 32))    \
             + R                                                                        \
                 * (42987 * R * sin(1.0 / 32)                                           \
                    - 2 * (cos(1.0 / 32) - 32 * sin(1.0 / 32))                          \
                        * (25 * sin(1.0 / (32 * R)) + 8 * sin(1.0 / (16 * R))))))       \
   / (3 * (89 + 82 * R * R + 10 * R * R * R * R)))

WS_1_D(DDBlack, _DDBLACK, _DDBLACK_SCL, POW3)
WS_1_F(DDBlack, _DDBLACK, _DDBLACK_SCL, POW3)
WS_N_F(DDBlack, _DDBLACK, _DDBLACK_SCL, POW3)
WS_N_D(DDBlack, _DDBLACK, _DDBLACK_SCL, POW3)

static const NrrdKernel _nrrdKernelDDBlack = {"blackmanDD",     2,
                                              _nrrdWindSincSup, _nrrdDWindSincInt,
                                              _nrrdDDBlack_1_f, _nrrdDDBlack_N_f,
                                              _nrrdDDBlack_1_d, _nrrdDDBlack_N_d};
const NrrdKernel *const nrrdKernelBlackmanDD = &_nrrdKernelDDBlack;
