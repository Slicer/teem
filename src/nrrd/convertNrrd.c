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

/* clang-format off */
#include "nrrd.h"
#include "privateNrrd.h"

/*
** making these typedefs here allows us to use one token for both
** constructing function names, and for specifying argument types
*/
typedef signed char CH;
typedef unsigned char UC;
typedef signed short SH;
typedef unsigned short US;
/* Microsoft apparently uses 'IN' as a keyword, so we changed 'IN' to 'JN'. */
typedef signed int JN;
typedef unsigned int UI;
typedef airLLong LL;
/* ui64 to double conversion is not implemented, sorry */
#if _MSC_VER < 1300
typedef airLLong UL;
#else
typedef airULLong UL;
#endif
typedef float FL;
typedef double DB;
typedef size_t IT;
/* typedef long double LD; */

/*
** GLK doesn't think we can avoid defining this macro twice,
** because of the rules of C preprocessor macro expansion.
** Do you see a way to avoid this?
**
** >>> MAP1 and MAP2 need to be identical <<<
*/

#define MAP1(F, A) \
F(A, CH) \
F(A, UC) \
F(A, SH) \
F(A, US) \
F(A, JN) \
F(A, UI) \
F(A, LL) \
F(A, UL) \
F(A, FL) \
F(A, DB)
/* F(A, LD) */

#define MAP2(F, A) \
F(A, CH) \
F(A, UC) \
F(A, SH) \
F(A, US) \
F(A, JN) \
F(A, UI) \
F(A, LL) \
F(A, UL) \
F(A, FL) \
F(A, DB)
/* F(A, LD) */

/*
** _nrrdConv<Ta><Tb>()
**
** given two arrays, a and b, of different types (Ta and Tb) but equal
** size N, _nrrdConvTaTb(a, b, N) will copy all the values from b into
** a, thereby effecting the same type-conversion as one gets with a
** cast.  See K+R Appendix A6 (pg. 197) for the details of what that
** entails.  There are plenty of situations where the results are
** "undefined" (assigning -23 to an unsigned char); the point here is
** simply to make available on arrays all the same behavior you can
** get from scalars.
*/
#define CONV_DEF(TA, TB) \
static void \
_nrrdConv##TA##TB(TA *a, const TB *b, IT N) { \
  size_t ii; \
  for (ii=0; ii<N; ii++) { \
    a[ii] = AIR_CAST(TA, b[ii]); \
  } \
}

/*
** _nrrdClCv<Ta><Tb>()
**
** same as _nrrdConv<Ta><Tb>(), but with clamping to the representable
** range of values of the output type, using a double as intermediate
** storage type HEY WHICH MEANS THAT LONG LONG (BOTH SIGNED AND UNSIGNED)
** may suffer loss of data!!!
*/
#define CLCV_DEF(TA, TB) \
static void \
_nrrdClCv##TA##TB(TA *a, const TB *b, IT N) { \
  size_t ii; \
  for (ii=0; ii<N; ii++) { \
    a[ii] = AIR_CAST(TA, _nrrdDClamp##TA(AIR_CAST(double, b[ii]))); \
  } \
}

/*
** _nrrdCcrd<Ta><Tb>()
**
** like _nrrdClCv<Ta><Tb>() and _nrrdConv<Ta><Tb>(), but with the
** ability to control if there is rounding and/or clamping. As above,
** there may be loss of precision with long long input.
*/
#define CCRD_DEF(TA, TB)                                        \
static void                                                     \
 _nrrdCcrd##TA##TB(TA *a, const TB *b, IT N,                    \
                   int doClamp, int roundd) {                   \
   size_t ii;                                                   \
  for (ii=0; ii<N; ii++) {                                      \
    double ccrdTmp = AIR_CAST(double, b[ii]);                   \
    ccrdTmp = (roundd > 0                                       \
               ? floor(ccrdTmp + 0.5)                           \
               : (roundd < 0                                    \
                  ? ceil(ccrdTmp - 0.5)                         \
                  : ccrdTmp));                                  \
    ccrdTmp = (doClamp ? _nrrdDClamp##TA(ccrdTmp) : ccrdTmp);   \
    a[ii] = AIR_CAST(TA, ccrdTmp);                              \
  }                                                             \
}

/*
** These makes the definition of later arrays shorter
*/
typedef void (*CF)(void *, const void *, IT);
typedef void (*CN)(void *, const void *, IT, int, int);

/*
** the individual converter's appearance in the array initialization,
** using the cast to the typedefs above
*/
#define CONV_LIST(TA, TB) (CF)_nrrdConv##TA##TB,
#define CLCV_LIST(TA, TB) (CF)_nrrdClCv##TA##TB,
#define CCRD_LIST(TA, TB) (CN)_nrrdCcrd##TA##TB,

/*
** the brace-delimited list of all converters _to_ type TA
*/
#define CONVTO_LIST(_dummy_, TA) {NULL, MAP2(CONV_LIST, TA) NULL},
#define CLCVTO_LIST(_dummy_, TA) {NULL, MAP2(CLCV_LIST, TA) NULL},
#define CCRDTO_LIST(_dummy_, TA) {NULL, MAP2(CCRD_LIST, TA) NULL},



/*
** This is where the actual emitted code starts ...
*/


/*
** the clamping functions were moved here from accessors.c in order
** to create the combined clamp-and-convert functions
*/

/*
******** nrrdFClamp
**
** The rationale for this has been: "for integral types, clamps a given float to the
** range representable by that type; for floating point types, just return the given
** number, since every float must fit in a double".  However, thinking for the v1.13
** release (which became the v2 release) finally recognized the fact that INT_MAX is
** not representable as a float, so you could have: int 2147483584, passed through
** nrrdFClamp[nrrdTypeInt] to get float 2.14748365e+09 > INT_MAX==2147483647, which
** when cast back to int gives you -2147483648, which completely violates the intent
** of these functions! So, now the rationale is just "clamps given float to a range
** that won't create big surprises due to integer overflow upon casting to that type".
** There could still be small surprises, as in from negative input generating output
** that is less than the input (but still negative). The solution adopted here is
** possibly too slow, but we'll let profiling tell us that, rather than trying to
** cleverly use bounds known at compile-time when valid.
**
** Btw, this is a good example of where warnings about implicit floating-point
** conversion warnings highlighted actual bugs in code.
*/
static float _nrrdFClampCH(FL v) { return AIR_CLAMP(SCHAR_MIN, v, SCHAR_MAX);}
static float _nrrdFClampUC(FL v) { return AIR_CLAMP(0, v, UCHAR_MAX);}
static float _nrrdFClampSH(FL v) { return AIR_CLAMP(SHRT_MIN, v, SHRT_MAX);}
static float _nrrdFClampUS(FL v) { return AIR_CLAMP(0, v, USHRT_MAX);}
static float _nrrdFClampJN(FL v) {
  airFloat umax, umin; /* float, uint union */
  umax.f = (float)INT_MAX;
  umax.i -= 1; /* decrease by one ULP */
  umin.f = (float)INT_MIN;
  umin.i += 1;
  return AIR_CLAMP(umin.f, v, umax.f);
}
static float _nrrdFClampUI(FL v) {
  airFloat umax; /* float, uint union */
  umax.f = (float)UINT_MAX;
  umax.i -= 1;
  return AIR_CLAMP(0, v, umax.f);
}
static float _nrrdFClampLL(FL v)  { /* HEY copy-pasta from above */
  airFloat umax, umin;
  umax.f = (float)NRRD_LLONG_MAX;
  umax.i -= 1;
  umin.f = (float)NRRD_LLONG_MIN;
  umin.i += 1;
  return AIR_CLAMP(umin.f, v, umax.f);
}
static float _nrrdFClampUL(FL v)  {
  airFloat umax;
  umax.f = (float)NRRD_ULLONG_MAX;
  umax.i -= 1;
  return AIR_CLAMP(0, v, umax.f);
}
static float _nrrdFClampFL(FL v) { return v; }
static float _nrrdFClampDB(FL v) { return v; }
float (* const
nrrdFClamp[NRRD_TYPE_MAX+1])(FL) = {
  NULL,
  _nrrdFClampCH,
  _nrrdFClampUC,
  _nrrdFClampSH,
  _nrrdFClampUS,
  _nrrdFClampJN,
  _nrrdFClampUI,
  _nrrdFClampLL,
  _nrrdFClampUL,
  _nrrdFClampFL,
  _nrrdFClampDB,
  NULL};

/*
******** nrrdDClamp
**
** same as nrrdDClamp, but for doubles; here the long long int extrema
** have to be cast to double to avoid implicit conversion warnings.
** One change: for floats, doubles are clamped to [-FLT_MAX, FLT_MAX].
*/
static double _nrrdDClampCH(DB v) { return AIR_CLAMP(SCHAR_MIN, v, SCHAR_MAX);}
static double _nrrdDClampUC(DB v) { return AIR_CLAMP(0, v, UCHAR_MAX);}
static double _nrrdDClampSH(DB v) { return AIR_CLAMP(SHRT_MIN, v, SHRT_MAX);}
static double _nrrdDClampUS(DB v) { return AIR_CLAMP(0, v, USHRT_MAX);}
static double _nrrdDClampJN(DB v) { return AIR_CLAMP(INT_MIN, v, INT_MAX);}
static double _nrrdDClampUI(DB v) { return AIR_CLAMP(0, v, UINT_MAX);}
static double _nrrdDClampLL(DB v) { /* HEY copy-pasta from above */
  airDouble umax, umin;
  umax.d = (double)NRRD_LLONG_MAX;
  umax.i -= 1;
  umin.d = (double)NRRD_LLONG_MIN;
  umin.i += 1;
  return AIR_CLAMP(umin.d, v, umax.d);
}
static double _nrrdDClampUL(DB v) {
  airDouble umax;
  umax.d = (double)NRRD_ULLONG_MAX;
  umax.i -= 1;
  return AIR_CLAMP(0, v, umax.d);
}
static double _nrrdDClampFL(DB v) { return AIR_CLAMP(-FLT_MAX, v, FLT_MAX); }
static double _nrrdDClampDB(DB v) { return v; }
double (* const
nrrdDClamp[NRRD_TYPE_MAX+1])(DB) = {
  NULL,
  _nrrdDClampCH,
  _nrrdDClampUC,
  _nrrdDClampSH,
  _nrrdDClampUS,
  _nrrdDClampJN,
  _nrrdDClampUI,
  _nrrdDClampLL,
  _nrrdDClampUL,
  _nrrdDClampFL,
  _nrrdDClampDB,
  NULL};


/*
** Define all the converters.
*/
MAP1(MAP2, CONV_DEF)
MAP1(MAP2, CLCV_DEF)
MAP1(MAP2, CCRD_DEF)


/*
** Initialize the converter arrays.
**
** Each definition generates one incredibly long line of text, which
** hopefully will not break a poor compiler with limitations on
** line-length...
*/
CF const
_nrrdConv[NRRD_TYPE_MAX+1][NRRD_TYPE_MAX+1] = {
{NULL},
MAP1(CONVTO_LIST, _dummy_)
{NULL}
};

CF const
_nrrdClampConv[NRRD_TYPE_MAX+1][NRRD_TYPE_MAX+1] = {
{NULL},
MAP1(CLCVTO_LIST, _dummy_)
{NULL}
};

CN const
_nrrdCastClampRound[NRRD_TYPE_MAX+1][NRRD_TYPE_MAX+1] = {
{NULL},
MAP1(CCRDTO_LIST, _dummy_)
{NULL}
};
/* clang-format on */
