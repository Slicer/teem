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

/*
******** airSanity()
**
** Does run-time checks to see if the compile-time constants are correct.
** Returns a value from the airInsane* enum; airInsane_not means all
** the checks came back without detecting any problems.
*/
int
airSanity(void) {
  double nanValue, pinf, ninf;
  float nanF, pinfF, ninfF;
  unsigned int sign, expvalue, mant, tmpI;
  unsigned char endian;
  unsigned char uc0, uc1;
  /* for Teem v1.13 GLK decided to remove this optimization, which meant that this
     function could only run through its tests once. Global state, especially if hidden
     like this, is fishy (and flagged by teem/src/_util/scan-symbols.py). Things like
     floating point rounding mode can be changed at run-time, which makes it more
     reasonable to re-run the FP-related tests. The non-FP tests are simple and should be
     fast to do. If profiling reveals this to be a bottleneck we can reconsider.
  static int _airSanity = 0;
  if (_airSanity) {
    return airInsane_not;
  }
  */

  /* now that there is no more compile-time endian info, this is merely double checking
    that airMyEndian() works, and returns the constants (either 1234, pronounced
    "little endian", or 4321, "big endian") that are defined in air.h */
  tmpI = 1;
  endian = !(*((unsigned char *)(&tmpI)));
  if (endian) {
    /* big endian */
    if (4321 != airMyEndian()) {
      return airInsane_endian;
    }
  } else {
    if (1234 != airMyEndian()) {
      return airInsane_endian;
    }
  }
  /* checks on sizes of uchar, float, int, double, airLLong */
  uc0 = 255;
  uc1 = AIR_UCHAR(AIR_INT(uc0) + 1); /* want to overflow */
  if (!(255 == uc0 && 0 == uc1)) {
    return airInsane_UCSize;
  }
  /* 2002 GLK: "these justify the AIR_EXISTS_F and AIR_EXISTS_D macros"
     well, probably: those macros depend on knowing which bits are for exponent
     vs significand, which is not revealed by sizeof(). But IEEE 754 Table 3.2
     gives those bit allocations for 32- and 64-bit floats. But the value of
     this sanity check is much larger than those macros. */
  if (!((sizeof(float) == sizeof(int)) && (4 == sizeof(int)))) {
    return airInsane_FISize;
  }
  if (!((sizeof(double) == sizeof(airLLong)) && (8 == sizeof(airLLong)))) {
    return airInsane_DLSize;
  }

  /* run-time NaN checks */
  ninf = -1e+300; /* pretty close to -DBL_MAX */
  ninf = _airSanityHelper(ninf);
  ninf = _airSanityHelper(ninf);
  ninf = _airSanityHelper(ninf);
  if (AIR_EXISTS(ninf)) {
    return airInsane_nInfExists;
  }
  pinf = 1e+300; /* pretty close to DBL_MAX */
  pinf = _airSanityHelper(pinf);
  pinf = _airSanityHelper(pinf);
  pinf = _airSanityHelper(pinf);
  if (AIR_EXISTS(pinf)) {
    return airInsane_pInfExists;
  }
  /* (commenting out until this FP rounding-mode-dependent surprise can be replicated)
  if (0 && AIR_EXISTS(pinf)) {
    / * on at least one computer GLK used, if fesetround(FE_DOWNWARD) has been
       called, then the above run-time generation of positive infinity fails;
       it instead rounds down to DBL_MAX. We err on the side of
       permissiveness, and opt not to try to detect the current rounding mode
       (because doing so in a C89-compliant way would be a pain), and thus
       flag AIR_EXISTS(pinf) as a problem only if pinf != DBL_MAX. On that
       one computer, fesetround(FE_UPWARD) did not hamper the above run-time
       negative infinity generation * /
    if (pinf != DBL_MAX) {
      return airInsane_pInfExists;
    } else {
      / * as best we can tell, pinf would have been +inf if it
         weren't for fesetround(FE_DOWNWARD), so fix pinf so that
         later tests can use it. * /
      pinf = -ninf;
    }
  } */
  nanValue = pinf / pinf;
  if (AIR_EXISTS(nanValue)) {
    return airInsane_NaNExists;
  }
  if (!(AIR_EXISTS(0.0) && AIR_EXISTS(-0.0)    /* */
        && AIR_EXISTS(1.0) && AIR_EXISTS(-1.0) /* */
        && AIR_EXISTS(42.42) && AIR_EXISTS(AIR_PI))) {
    return airInsane_ExistsBad;
  }
  nanF = (float)nanValue;
  pinfF = (float)pinf;
  ninfF = (float)ninf;
  airFPValToParts_f(&sign, &expvalue, &mant, nanF);
  mant >>= 22;
  if (AIR_QNANHIBIT != mant) {
    return airInsane_QNaNHiBit;
  }
  /* this a rough test of the _F and _D macros at the end of air.h; they aren't currently
     used within Teem so it is harder to justify making an airInsane case just for these;
     but if that happens this might be a start
  do {
    double ff = 1234.123412f;
    double dd = 1234.123412;
    printf("AIR_EXISTS_F(nanF) = %d\n", AIR_EXISTS_F(nanF));
    printf("AIR_EXISTS_F(pinfF) = %d\n", AIR_EXISTS_F(pinfF));
    printf("AIR_EXISTS_F(ninfF) = %d\n", AIR_EXISTS_F(ninfF));
    printf("AIR_EXISTS_F(ff) = %d\n", AIR_EXISTS_F(ff));
    printf("AIR_ISNAN_F(nanF) = %d\n", AIR_ISNAN_F(nanF));
    printf("AIR_ISNAN_F(pinfF) = %d\n", AIR_ISNAN_F(pinfF));
    printf("AIR_ISNAN_F(ninfF) = %d\n", AIR_ISNAN_F(ninfF));
    printf("AIR_ISNAN_F(ff) = %d\n", AIR_ISNAN_F(ff));
    printf("AIR_EXISTS_D(nanValue) = %d\n", AIR_EXISTS_D(nanValue));
    printf("AIR_EXISTS_D(pinf) = %d\n", AIR_EXISTS_D(pinf));
    printf("AIR_EXISTS_D(ninf) = %d\n", AIR_EXISTS_D(ninf));
    printf("AIR_EXISTS_D(dd) = %d\n", AIR_EXISTS_F(dd));
  } while (0);
   */

  if (!(airFP_QNAN == airFPClass_f(AIR_NAN)
        && airFP_QNAN == airFPClass_f(AIR_QNAN)
        /*
          As of July 4 2012 GLK decides that the signalling NaN tests are
          more trouble than they're worth: the signal-ness of the NaN is not
          preserved in double-float conversion for some platforms (so
          airFP_SNAN == airFPClass_d(AIR_SNAN) has never been enforced), and
          there are more platforms for which (apparently) passing AIR_SNAN to
          airFPClass_d changes it to a quiet NaN, which defeats the purpose
          of the test.  To summarize, given that:
          ** AIR_NAN and AIR_QNAN are checked here to be quiet NaN, after
             casting to both float and double,
          ** quiet NaN "hi bit" is tested above, and that
          ** quiet and signalling NaN are mutually exclusive,
          skipping the signalling NaN tests is unlikely to undermine knowing
          the correctness of the compile-time representation of NaNs.  So the
          following line is now commented out for all platforms.
        */
        /* && airFP_SNAN == airFPClass_f((double)AIR_SNAN) */
        && airFP_QNAN == airFPClass_d((double)AIR_NAN)
        && airFP_QNAN == airFPClass_d((double)AIR_QNAN))) {
    return airInsane_AIR_NAN;
  }
  if (!(airFP_QNAN == airFPClass_f(nanF) && airFP_POS_INF == airFPClass_f(pinfF)
        && airFP_NEG_INF == airFPClass_f(ninfF))) {
    /* really, this is verifying that assigning from a double to a
       float maintains the FPClass for non-existent values */
    return airInsane_FltDblFPClass;
  }

  /* just make sure AIR_DIO is reasonably set
     (actually, this should be done by include/teemDio.h) */
  switch (AIR_DIO) {
  case 0:
    break;
  case 1:
    break;
  default:
    return airInsane_dio;
  }

  /* _airSanity = 1; (see above) */
  return airInsane_not;
}

static const char _airInsaneErr[AIR_INSANE_MAX + 1][AIR_STRLEN_MED + 1] = {
  "sanity checked PASSED!",                           /* 0: airInsane_not */
  "airMyEndian() is wrong",                           /* 1: airInsane_endian */
  "AIR_EXISTS(+inf) was true",                        /* 2: airInsane_pInfExists */
  "AIR_EXISTS(-inf) was true",                        /* 3: airInsane_nInfExists */
  "AIR_EXISTS(NaN) was true",                         /* 4: airInsane_NaNExists */
  "AIR_EXISTS() was false for some finite values",    /* 5: airInsane_ExistsBad */
  "air_FPClass_f() wrong after double->float assign", /* 6: airInsane_FltDblFPClass */
  "TEEM_QNANHIBIT is wrong",                          /* 7: airInsane_QNaNHiBit */
  "airFPClass(AIR_QNAN) wrong",                       /* 8: airInsane_AIR_NAN */
  "TEEM_DIO has invalid value",                       /* 9: airInsane_dio */
  "unsigned char isn't 8 bits",                       /* 10: airInsane_UCSize */
  "sizeof(float), sizeof(int) not both == 4",         /* 11: airInsane_FISize */
  "sizeof(double), sizeof(airLLong) not both == 8",   /* 12: airInsane_DLSize */
};

static const char _airBadInsane[] = "(invalid insane value)";

const char *
airInsaneErr(int insane) {

  if (AIR_IN_CL(0, insane, AIR_INSANE_MAX)) {
    return _airInsaneErr[insane];
  } else {
    return _airBadInsane;
  }
}
