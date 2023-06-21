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

#include "air.h"
#include "privateAir.h"
#include <teemQnanhibit.h>

/* clang-format off */
static const char *_airFPClass_Str[AIR_FP_MAX+1] = {
  "(unknown_class)",
  "snan",
  "qnan",
  "pinf",
  "ninf",
  "pnorm",
  "nnorm",
  "pdenorm",
  "ndenorm",
  "pzero",
  "nzero",
};

static const char *_airFPClass_Desc[AIR_FP_MAX+1] = {
  "unknown_class",
  "signalling nan",
  "quiet nan",
  "positive infinity",
  "negative infinity",
  "positive normalized",
  "negative normalized",
  "positive denormalized",
  "negative denormalized",
  "positive zero",
  "negative zero",
};

static const char *_airFPClass_StrEqv[] = {
  "snan", "signan",
  "qnan", "nan",
  "pinf", "posinf", "+inf", "inf",
  "ninf", "neginf", "-inf",
  "pnorm", "posnorm", "+norm", "norm",
  "nnorm", "negnorm", "-norm",
  "pdenorm", "posdenorm", "+denorm", "denorm",
  "ndenorm", "negdenorm", "-denorm",
  "pzero", "+0", "+zero", "zero", "0",
  "nzero", "-0", "-zero",
  "",
};

static const int _airFPClass_ValEqv[] = {
  airFP_SNAN, airFP_SNAN,
  airFP_QNAN, airFP_QNAN,
  airFP_POS_INF, airFP_POS_INF, airFP_POS_INF, airFP_POS_INF,
  airFP_NEG_INF, airFP_NEG_INF, airFP_NEG_INF,
  airFP_POS_NORM, airFP_POS_NORM, airFP_POS_NORM, airFP_POS_NORM,
  airFP_NEG_NORM, airFP_NEG_NORM, airFP_NEG_NORM,
  airFP_POS_DENORM, airFP_POS_DENORM, airFP_POS_DENORM, airFP_POS_DENORM,
  airFP_NEG_DENORM, airFP_NEG_DENORM, airFP_NEG_DENORM,
  airFP_POS_ZERO, airFP_POS_ZERO, airFP_POS_ZERO, airFP_POS_ZERO, airFP_POS_ZERO,
  airFP_NEG_ZERO, airFP_NEG_ZERO, airFP_NEG_ZERO,
};

static const airEnum _airFPClass_ae = {"FP_class",
                                        AIR_FP_MAX,
                                        _airFPClass_Str,
                                        NULL,
                                        _airFPClass_Desc,
                                        _airFPClass_StrEqv,
                                        _airFPClass_ValEqv,
                                        AIR_FALSE};
const airEnum *const airFPClass_ae = &_airFPClass_ae;
/* clang-format on */

/*
** all this is based on a reading of
** Hennessy + Patterson "Computer Architecture, A Quantitative Approach"
** pages A-13 - A-17
**
** and some assorted web pages, such as:
** http://en.wikipedia.org/wiki/NaN#Encoding
** which explains what Teem calls qnanhibit, and
** http://grouper.ieee.org/groups/754/email/msg04192.html
** which includes some discussion on signal-vs-quiet nan
*/

/*
** The hex numbers in braces are examples of C's "initial member of a union"
** aggregate initialization.
*/

#if TEEM_QNANHIBIT == 1
const unsigned int airMyQNaNHiBit = 1;
const airFloat airFloatQNaN = {0x7fffffff};
const airFloat airFloatSNaN = {0x7fbfffff};
#else
const unsigned int airMyQNaNHiBit = 0;
const airFloat airFloatQNaN = {0x7fbfffff};
const airFloat airFloatSNaN = {0x7fffffff};
#endif

const airFloat airFloatPosInf = {0x7f800000};
const airFloat airFloatNegInf = {0xff800000}; /* why does solaris whine? */

/*
** these shouldn't be needed, but here they are if need be:

in this file:
const airFloat airFloatMax = {0x7f7fffff};
const airFloat airFloatMin = {0x00800000};
const airDouble airDoubleMax = {AIR_ULLONG(0x7fefffffffffffff)};
const airDouble airDoubleMin = {AIR_ULLONG(0x0010000000000000)};

in air.h:
extern air_export const airFloat airFloatMax;
extern air_export const airFloat airFloatMin;
extern air_export const airDouble airDoubleMax;
extern air_export const airDouble airDoubleMin;
#define AIR_FLT_MIN (airFloatMin.f)
#define AIR_FLT_MAX (airFloatMax.f)
#define AIR_DBL_MIN (airDoubleMin.d)
#define AIR_DBL_MAX (airDoubleMax.d)
*/

#define PARTSHIFT_F(sign, expo, mant)                                                   \
  (((sign & 1u) << (8 + 23)) |        /* */                                             \
   ((expo & ((1u << 8) - 1)) << 23) | /* */                                             \
   (mant & ((1u << 23) - 1)))

#define PARTSHIFT_D(sign, expo, mant0, mant1)                                           \
  (((sign & AIR_ULLONG(1)) << (11 + 52)) |         /* */                                \
   ((expo & ((AIR_ULLONG(1) << 11) - 1)) << 52) |  /* */                                \
   ((mant0 & ((AIR_ULLONG(1) << 20) - 1)) << 32) | /* */                                \
   (mant1 & ((AIR_ULLONG(1) << 32) - 1)))

float
airFPPartsToVal_f(unsigned int sign, unsigned int expo, unsigned int mant) {
  airFloat af;

  af.i = PARTSHIFT_F(sign, expo, mant);
  return af.f;
}

double
airFPPartsToVal_d(unsigned int sign,
                  unsigned int expo,
                  unsigned int mant0,
                  unsigned int mant1) {
  airDouble ad;

  ad.i = PARTSHIFT_D(sign, expo, mant0, mant1);
  return ad.d;
}

void
airFPValToParts_f(unsigned int *signP, unsigned int *expoP, unsigned int *mantP,
                  float v) {
  airFloat af;
  unsigned int ui;

  af.f = v;
  ui = af.i;
  *mantP = ui & ((1u << 23) - 1);
  ui >>= 23;
  *expoP = ui & ((1u << 8) - 1);
  ui >>= 8;
  *signP = ui & 1u;
}

void
airFPValToParts_d(unsigned int *signP, unsigned int *expoP, unsigned int *mant0P,
                  unsigned int *mant1P, double v) {
  airDouble ad;
  airULLong ui;

  ad.d = v;
  ui = ad.i;
  *mant1P = AIR_UINT(ui & ((AIR_ULLONG(1) << 32) - 1));
  ui >>= 32;
  *mant0P = AIR_UINT(ui & ((AIR_ULLONG(1) << 20) - 1));
  ui >>= 20;
  *expoP = AIR_UINT(ui & ((AIR_ULLONG(1) << 11) - 1));
  ui >>= 11;
  *signP = AIR_UINT(ui & AIR_ULLONG(1));
}

/*
******** airFPGen_f()
**
** generates a floating point value which is a member of the given class
*/
float
airFPGen_f(int cls) {
  airFloat af;
  unsigned int sign, expo, mant;

#define SET_SEM(ss, ee, mm)                                                             \
  sign = (ss);                                                                          \
  expo = (ee);                                                                          \
  mant = (mm)

  switch (cls) {
  case airFP_SNAN:
    /* sgn: anything, mant: anything non-zero with high bit !TEEM_QNANHIBIT */
    SET_SEM(0, 0xff, (!TEEM_QNANHIBIT << 22) | 0x3fffff);
    break;
  case airFP_QNAN:
    SET_SEM(0, 0xff, (TEEM_QNANHIBIT << 22) | 0x3fffff);
    break;
  case airFP_POS_INF:
    SET_SEM(0, 0xff, 0);
    break;
  case airFP_NEG_INF:
    SET_SEM(1, 0xff, 0);
    break;
  case airFP_POS_NORM:
    /* exp: anything non-zero but < 0xff, mant: anything */
    SET_SEM(0, 0x80, 0x7ff000);
    break;
  case airFP_NEG_NORM:
    /* exp: anything non-zero but < 0xff, mant: anything */
    SET_SEM(1, 0x80, 0x7ff000);
    break;
  case airFP_POS_DENORM:
    /* mant: anything non-zero */
    SET_SEM(0, 0, 0xff);
    break;
  case airFP_NEG_DENORM:
    /* mant: anything non-zero */
    SET_SEM(1, 0, 0xff);
    break;
  case airFP_NEG_ZERO:
    SET_SEM(1, 0, 0);
    break;
  case airFP_POS_ZERO:
  default:
    SET_SEM(0, 0, 0);
    break;
  }
#undef SET_SEM
  af.i = PARTSHIFT_F(sign, expo, mant);
  /* printf("%s(0x%x, 0x%x, 0x%x) = 0x%x -> %.9g\n", __func__, sign, expo, mant, af.i,
         af.f); */
  return af.f;
}

/*
******** airFPGen_d()
**
** generates a floating point value which is a member of the given class
*/
double
airFPGen_d(int cls) {
  airDouble ad;
  unsigned int sign, expo, mant0, mant1;

#define SET_SEM(ss, ee, m0, m1)                                                         \
  sign = (ss);                                                                          \
  expo = (ee);                                                                          \
  mant0 = (m0);                                                                         \
  mant1 = (m1)

  switch (cls) {
  case airFP_SNAN:
    /* sgn: anything, mant: anything non-zero with high bit !TEEM_QNANHIBIT */
    SET_SEM(0, 0x7ff, (!TEEM_QNANHIBIT << 19) | 0x7ffff, 0xffffffff);
    break;
  case airFP_QNAN:
    /* sgn: anything, mant anything non-zero with high bit TEEM_QNANHIBIT */
    SET_SEM(0, 0x7ff, (TEEM_QNANHIBIT << 19) | 0x7ffff, 0xffffffff);
    break;
  case airFP_POS_INF:
    SET_SEM(0, 0x7ff, 0, 0);
    break;
  case airFP_NEG_INF:
    SET_SEM(1, 0x7ff, 0, 0);
    break;
  case airFP_POS_NORM:
    /* exp: anything non-zero but < 0xff, mant: anything */
    SET_SEM(0, 0x400, 0x0ff00, 0);
    break;
  case airFP_NEG_NORM:
    /* exp: anything non-zero but < 0xff, mant: anything */
    SET_SEM(1, 0x400, 0x0ff00, 0);
    break;
  case airFP_POS_DENORM:
    /* mant: anything non-zero */
    SET_SEM(0, 0, 0xff, 0);
    break;
  case airFP_NEG_DENORM:
    /* mant: anything non-zero */
    SET_SEM(1, 0, 0xff, 0);
    break;
  case airFP_NEG_ZERO:
    SET_SEM(1, 0, 0, 0);
    break;
  case airFP_POS_ZERO:
  default:
    SET_SEM(0, 0, 0, 0);
    break;
  }
#undef SET_SEM
  ad.i = PARTSHIFT_D(sign, expo, mant0, mant1);
  return ad.d;
}

static int
wutClass(unsigned int index, int expoMax, unsigned int nanHiBit) {
  int ret = airFP_Unknown;
  switch (index) {
  case 0:
    /* all fields are zero */
    ret = airFP_POS_ZERO;
    break;
  case 1:
    /* only mantissa is non-zero */
    ret = airFP_POS_DENORM;
    break;
  case 2:
    /* only exponent field is non-zero */
    if (expoMax) {
      ret = airFP_POS_INF;
    } else {
      ret = airFP_POS_NORM;
    }
    break;
  case 3:
    /* exponent and mantissa fields are non-zero */
    if (expoMax) {
      if (TEEM_QNANHIBIT == nanHiBit) {
        ret = airFP_QNAN;
      } else {
        ret = airFP_SNAN;
      }
    } else {
      ret = airFP_POS_NORM;
    }
    break;
  case 4:
    /* only sign field is non-zero */
    ret = airFP_NEG_ZERO;
    break;
  case 5:
    /* sign and mantissa fields are non-zero */
    ret = airFP_NEG_DENORM;
    break;
  case 6:
    /* sign and exponent fields are non-zero */
    if (expoMax) {
      ret = airFP_NEG_INF;
    } else {
      ret = airFP_NEG_NORM;
    }
    break;
  case 7:
    /* all fields are non-zero */
    if (expoMax) {
      if (TEEM_QNANHIBIT == nanHiBit) {
        ret = airFP_QNAN;
      } else {
        ret = airFP_SNAN;
      }
    } else {
      ret = airFP_NEG_NORM;
    }
    break;
  }
  return ret;
}
/*
******** airFPClass_f()
**
** given a floating point number, tells which class its in
*/
int
airFPClass_f(float val) {
  unsigned int sign, expo, mant, indexv;

  airFPValToParts_f(&sign, &expo, &mant, val);
  /* "!" produces an int: https://en.cppreference.com/w/c/language/operator_logical */
  indexv = (AIR_UINT(!!sign) << 2) | (AIR_UINT(!!expo) << 1) | AIR_UINT(!!mant);
  return wutClass(indexv, 0xff == expo, mant >> 22);
}

/*
******** airFPClass_d()
**
** given a double, tells which class its in
*/
int
airFPClass_d(double val) {
  unsigned int sign, expo, mant0, mant1, indexv;

  airFPValToParts_d(&sign, &expo, &mant0, &mant1, val);
  indexv = (AIR_UINT(!!sign) << 2) | (AIR_UINT(!!expo) << 1)
         | (AIR_UINT(!!mant0) || AIR_UINT(!!mant1));
  return wutClass(indexv, 0x7ff == expo, mant0 >> 19);
}

/*
******** airIsNaN()
**
** returns 1 if input is either kind of NaN, 0 otherwise.  It is okay
** to only have a double version of this function, as opposed to
** having one for float and one for double, because Section 6.2 of the
** 754 spec tells us that that NaN is to be preserved across precision
** changes (and airSanity() explicitly checks for this).
*/
int
airIsNaN(double g) {
  unsigned int sign, expo, mant;

  airFPValToParts_f(&sign, &expo, &mant, AIR_FLOAT(g));
  AIR_UNUSED(sign);
  return (0xff == expo && mant);
}

/*
******** airIsInf_f(), airIsInf_d()
**
** returns 1 if input is positive infinity,
** -1 if negative infinity,
** or 0 otherwise (including NaN)
**
** thus the non-zero-ness of the return is an easy way to do a
** boolean check of whether the value is infinite
*/
int
airIsInf_f(float f) {
  int c, ret;

  c = airFPClass_f(f);
  if (airFP_POS_INF == c) {
    ret = 1;
  } else if (airFP_NEG_INF == c) {
    ret = -1;
  } else {
    ret = 0;
  }
  return ret;
}
int
airIsInf_d(double d) {
  int c, ret;

  c = airFPClass_d(d);
  if (airFP_POS_INF == c) {
    ret = 1;
  } else if (airFP_NEG_INF == c) {
    ret = -1;
  } else {
    ret = 0;
  }
  return ret;
}

/* airExists_f() airExists_d() were nixed because they weren't used-
  you can just use AIR_EXISTS_F and AIR_EXISTS_D directly */

/*
******** airExists()
**
** an optimization-proof alternative to AIR_EXISTS
*/
int
airExists(double val) {
  airDouble ad;
  airULLong ui;

  ad.d = val;
  ui = ad.i;
  ui >>= 52;
  return 0x7ff != AIR_UINT(ui & 0x7ff);
}

/*
******** airNaN()
**
** returns a float quiet NaN
*/
float
airNaN(void) {

  return airFPGen_f(airFP_QNAN);
}

/*
******** airFPFprintf_f()
**
** prints out the bits of a "float", indicating the three different fields
*/
void
airFPFprintf_f(FILE *file, float val) {
  int i, cls;
  unsigned int sign, expo, mant;
  airFloat af;

  if (file) {
    af.f = val;
    airFPValToParts_f(&sign, &expo, &mant, val);
    cls = airFPClass_f(val);
    fprintf(file, "%.9g (class %d=%s) 0x%08x = ", (double)val, cls,
            airEnumStr(airFPClass_ae, cls), af.i);
    fprintf(file, "sign:0x%x, expo:0x%02x, mant:0x%06x = \n", sign, expo, mant);
    fprintf(file, " S [ . . Exp . . ] "
                  "[ . . . . . . . . . Mant. . . . . . . . . . ]\n");
    fprintf(file, " %d ", sign);
    for (i = 7; i >= 0; i--) {
      fprintf(file, "%d ", (expo >> i) & 1);
    }
    for (i = 22; i >= 0; i--) {
      fprintf(file, "%d ", (mant >> i) & 1);
    }
    fprintf(file, "\n");
  }
}

/*
******** airFPFprintf_d()
**
** prints out the bits of a "double", indicating the three different fields
*/
void
airFPFprintf_d(FILE *file, double val) {
  int i, cls;
  unsigned int sign, expo, mant0, mant1, half0, half1;
  airDouble ad;
  airULLong ui;

  if (file) {
    ad.d = val;
    ui = ad.i;
    half1 = AIR_UINT(ui & 0xffffffff);
    ui >>= 32;
    half0 = AIR_UINT(ui & 0xffffffff);
    cls = airFPClass_d(val);
    fprintf(file, "%.17g (class %d=%s) 0x%08x %08x = \n", val, cls,
            airEnumStr(airFPClass_ae, cls), half0, half1);
    airFPValToParts_d(&sign, &expo, &mant0, &mant1, val);
    fprintf(file, "sign:0x%x, expo:0x%03x, mant:0x%05x %08x = \n", sign, expo, mant0,
            mant1);
    fprintf(file, "S[...Exp...][.......................Mant.......................]\n");
    fprintf(file, "%d", sign);
    for (i = 10; i >= 0; i--) {
      fprintf(file, "%d", (expo >> i) & 1);
    }
    for (i = 19; i >= 0; i--) {
      fprintf(file, "%d", (mant0 >> i) & 1);
    }
    for (i = 31; i >= 0; i--) {
      fprintf(file, "%d", (mant1 >> i) & 1);
    }
    fprintf(file, "\n");
  }
}
