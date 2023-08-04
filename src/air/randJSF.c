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
The "Jenkins Small Fast" (JSF) pseudo-random number generator is from Bob Jenkins,
http://burtleburtle.net/bob/rand/smallprng.html  (circa 2009 ?)
who called it "A small noncryptographic PRNG", but didn't provide a tidier name.
GLK learned about this from Melissa O'Neill's 2018 writing here
https://www.pcg-random.org/posts/bob-jenkins-small-prng-passes-practrand.html
where she notes how other have identified it as "JSF" in another RNG library.
The JSF name is used by others as well, e.g.
https://en.wikipedia.org/wiki/List_of_random_number_generators
*/

/*
These #defines allow the (following) original code for "ranval" and "raninit" to BE the
definition of Teem's airJSFRandVal and airJSFRandSeed, respectively, and for "randctx" to
refer to the airJSFRand struct which has been already and identically defined in air.h
*/
#define ranval  airJSFRandVal
#define raninit airJSFRandSeed
#define ranctx  airJSFRand

/* the following (public-domain) code is copied verbatim from Jenkins' web page,
except for two changes:
1) "typedef unsigned long int u4;" --> "typedef unsigned int u4;"
   airRandJSFSanity() ensures that sizeof(unsigned int) == 4
2) commenting out the ranctx == airJSFRand struct definition */

/* clang-format off */
typedef unsigned int u4;
/* typedef struct ranctx { u4 a; u4 b; u4 c; u4 d; } ranctx; */

#define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))
u4 ranval( ranctx *x ) {
    u4 e = x->a - rot(x->b, 27);
    x->a = x->b ^ rot(x->c, 17);
    x->b = x->c + x->d;
    x->c = x->d + e;
    x->d = e + x->a;
    return x->d;
}

void raninit( ranctx *x, u4 seed ) {
    u4 i;
    x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
    for (i=0; i<20; ++i) {
        (void)ranval(x);
    }
}
/* clang-format on */
#undef rot

airJSFRand *
airJSFRandNew(unsigned int seed) {
  airJSFRand *jsf;
  jsf = malloc(sizeof(airJSFRand));
  airJSFRandSeed(jsf, seed); /* == "raninit" above */
  return jsf;
}

airJSFRand *
airJSFRandNix(airJSFRand *jsf) {
  return airFree(jsf);
}

unsigned int
airJSFRandValMod(airJSFRand *jsf, unsigned int N) {
  unsigned int cap, val;
  if (!N) {
    /* no specific range requested; so provide val in range [0, UINT_MAX] */
    return ranval(jsf);
  }
  /* else want values in range [0, N-1].
     Set cap to biggest multiple of N that fits in a uint32 */
  cap = 0xffffffff;
  cap -= cap % N;
  /* uniformly sample the remainders mod N by generating uints until less than cap */
  do {
    val = ranval(jsf);
  } while (val >= cap);
  /* return result mod N */
  return val % N;
}

int
airJSFRandSanity(void) {
  int sane = 0;
  airJSFRand jsf;
  if (4 != sizeof(unsigned int)) {
    return 0;
  }
  airJSFRandSeed(&jsf, 2600);
  sane = (3114645624 == airJSFRandVal(&jsf) && /* */
          580265137 == airJSFRandVal(&jsf) &&  /* */
          3377642734 == airJSFRandVal(&jsf) && /* */
          630323219 == airJSFRandVal(&jsf) &&  /* */
          3984528821 == airJSFRandVal(&jsf) && /* */
          849682424 == airJSFRandVal(&jsf) &&  /* */
          3735540612 == airJSFRandVal(&jsf) && /* */
          2696920995 == airJSFRandVal(&jsf) && /* */
          155857509 == airJSFRandVal(&jsf) &&  /* */
          1578235471 == airJSFRandVal(&jsf));
  return sane;
}

/******************************************************************************
 ******************************************************************************
 End of basic random number generation, start of floating-point specific stuff
 ******************************************************************************
 ******************************************************************************/

/* airJSFRandUni_f,airJSFRandUni_d functions are for accurately sampling floats,doubles
between 0 and 1, making sure that it is possible for all finite values in that range to
be generated, which is NOT true of the naive approach of dividing some int in [0,N-1] by
N. Random values really close to 0 turn into (via the RandNormal functions below) samples
far into the tails of the Gaussian.

The idea for these is described in Section 3.1 of:

    David B. Thomas, Wayne Luk, Philip H.W. Leong, and John D. Villasenor.
    Gaussian random number generators. ACM Comput. Surv. 39, 4, Article 11
    (November 2007). DOI=http://dx.doi.org/10.1145/1287620.1287622

  which is also the same basic idea as Taylor Campbell describes here:

    http://mumble.net/~campbell/2014/04/28/uniform-random-float
    http://mumble.net/~campbell/2014/04/28/random_real.c

Essentially: the probability of returning a float X should be proportional to the width
of the interval containing all the reals closer to X than any other float, which is
(except when the fraction field is all zeros) simply the ulp(X), or the distance to the
next representable float.  In IEEE754 the value of the exponent field determines the ulp,
and the ulp is halved by decrementing the exponent.  Starting with the highest possible
exponent for values below 1 (that is, the bias minus 1), a stream of random bits
determines whether to decrement the exponent, stopping when a 1 bit is seen. The ulp
within the denormals is the same as for the lowest normal.
*/

/* https://en.wikipedia.org/wiki/Find_first_set */
static unsigned int
uint32_clz(unsigned int nn) {
  /* HEY should figure out a configure-time test for having __builtin_clz */
#if 0
  return __builtin_clz(nn);
#else
  unsigned int ret = 0;
  if (!(nn & 0xFFFF0000)) {
    ret += 16;
    nn <<= 16;
  }
  if (!(nn & 0xFF000000)) {
    ret += 8;
    nn <<= 8;
  }
  if (!(nn & 0xF0000000)) {
    ret += 4;
    nn <<= 4;
  }
  if (!(nn & 0xC0000000)) {
    ret += 2;
    nn <<= 2;
  }
  if (!(nn & 0x80000000)) {
    ret += 1;
  }
  return ret;
#endif
}

float
airJSFRandUni_f(airJSFRand *rng) {
  unsigned int expo = 126, /* one less than bias => values in [0.5,1) */
    nz,                    /* number of leading zeros seen */
    val;
  airFloat ret;

  val = airJSFRandVal(rng);
  while (!val && expo > 32) {
    /* got 32 bits of zeros (!) and can decrement expo by 32; try again */
    expo -= 32;
    val = airJSFRandVal(rng);
  }
  /* possible (though super unlikely) to leave loop with expo <= 32 and val == 0,
  but builtin clz(0) is undefined, so need extra "val ?" check here */
  nz = val ? uint32_clz(val) : 32;
  /* either we saw a 1 before expo hits zero, or not, in which case
     we're into the denormals (expo == 0), which is fine */
  expo = nz > expo ? 0 : expo - nz;
  /* # zero bits has determined expo; now determine fraction.
  (binary) rng is 1 followed by R = 32 - 1 - nz random bits.
  We need 23 bits for the fraction, but can't use the (non-random) 1
  that ended the zero bit counting loop above.
  Requiring 23 <= R  ==>  23 <= 32 - 1 - nz  ==>  nz <= 8.
  So if nz > 8, we need to get more random bits */
  if (nz > 8) {
    val = airJSFRandVal(rng);
  }
  ret.i = ((expo << 23)       /* move expo up past frac */
           | (val & 0x7fffff) /* lo 23 bits of val */
  );
  return ret.f;
}

float
airJSFRandBiUni_f(airJSFRand *rng) {
  /* HEY copy-pasta (minus comments) from above,
  but now we use more bit of randomness to set the sign */
  unsigned int expo = 126, nz, val;
  airFloat ret;

  val = airJSFRandVal(rng);
  while (!val && expo > 32) {
    expo -= 32;
    val = airJSFRandVal(rng);
  }
  nz = val ? uint32_clz(val) : 32;
  expo = nz > expo ? 0 : expo - nz;
  if (nz > 7) { /* vs 8 above; need 24 bits of randomness */
    val = airJSFRandVal(rng);
  }
  ret.i = (((val & 0x800000) << 8) /* 24th bit of val, move up 8 to 32nd bit */
           | (expo << 23)          /* move expo up past frac */
           | (val & 0x7fffff)      /* low 23 bits of val */
  );
  return ret.f;
}

double
airJSFRandUni_d(airJSFRand *rng) {
  /* HEY more mostly uncommented copy-pasta from airJSFRandUni_f above */
  unsigned int expo = 1022 /* one less than bias */, nz, val, rlo;
  airDouble ret;

  val = airJSFRandVal(rng);
  while (!val && expo > 32) {
    expo -= 32;
    val = airJSFRandVal(rng);
  }
  nz = val ? uint32_clz(val) : 32;
  expo = nz > expo ? 0 : expo - nz;
  /* # zero bits has determined expo; now determine fraction.
  (binary) rng is 1 followed by R = 32 - 1 - nz random bits.
  We need *52* bits for the fraction, but can't use the (non-random) 1
  that ended the zero bit counting loop above.
  First get 32 more bits into rlo */
  rlo = airJSFRandVal(rng);
  /* We now need 52 - 32 = 20 bits more.
  Requiring 20 <= R  ==>  20 <= 32 - 1 - nz  ==>  nz <= 11.
  So if nz > 11, we need to get more random bits */
  if (nz > 11) {
    val = airJSFRandVal(rng);
  }
  ret.i = (((airULLong)expo) << 52              /* move expo up past frac */
           | ((airULLong)(val & 0xfffff)) << 32 /* lo 20 bits of rng, moved up 32 */
           | rlo                                /* lo 32 bits */
  );
  return ret.d;
}

double
airJSFRandBiUni_d(airJSFRand *rng) {
  /* HEY yet more uncommented copy-pasta */
  unsigned int expo = 1022, nz, val, rlo;
  airDouble ret;

  val = airJSFRandVal(rng);
  while (!val && expo > 32) {
    expo -= 32;
    val = airJSFRandVal(rng);
  }
  nz = val ? uint32_clz(val) : 32;
  expo = nz > expo ? 0 : expo - nz;
  rlo = airJSFRandVal(rng);
  if (nz > 10) { /* vs 11 above; need 21 bits of randomness */
    val = airJSFRandVal(rng);
  }
  /* sign is 21st bit of rng, moved up 11 to 32nd, and then up 32 to 64rth */
  ret.i = (((airULLong)((val & 0x100000) << 11)) << 32
           | ((airULLong)expo) << 52            /* move expo up past frac */
           | ((airULLong)(val & 0xfffff)) << 32 /* lo 20 bits of rng, moved up 32 */
           | rlo                                /* lo 32 bits */
  );
  return ret.d;
}

/* Polar Box–Muller
https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform#Polar_form
which generates two values at once */
void
airJSFRandNormal2_f(airJSFRand *rng, float val[2]) {
  float xx, yy, rr;
  do {
    xx = airJSFRandBiUni_f(rng);
    yy = airJSFRandBiUni_f(rng);
    rr = xx * xx + yy * yy;
  } while (!rr || rr >= 1);
  rr = sqrtf((-2 * logf(rr)) / rr);
  val[0] = xx * rr;
  val[1] = yy * rr;
  return;
}

/* If you really only want one value */
float
airJSFRandNormal_f(airJSFRand *rng) {
  float xx, yy, rr;
  do {
    xx = airJSFRandBiUni_f(rng);
    yy = airJSFRandBiUni_f(rng);
    rr = xx * xx + yy * yy;
  } while (!rr || rr >= 1);
  rr = sqrtf((-2 * logf(rr)) / rr);
  /* sum of two normals has variance 2 or stdv sqrt(2) */
  return rr * (xx + yy) * 0.7071067811865475244f;
}

/* HEY more copy-pasta! */
void
airJSFRandNormal2_d(airJSFRand *rng, double val[2]) {
  double xx, yy, rr;
  do {
    xx = airJSFRandBiUni_d(rng);
    yy = airJSFRandBiUni_d(rng);
    rr = xx * xx + yy * yy;
  } while (!rr || rr >= 1);
  rr = sqrt((-2 * log(rr)) / rr);
  val[0] = xx * rr;
  val[1] = yy * rr;
  return;
}

double
airJSFRandNormal_d(airJSFRand *rng) {
  double xx, yy, rr;
  do {
    xx = airJSFRandBiUni_d(rng);
    yy = airJSFRandBiUni_d(rng);
    rr = xx * xx + yy * yy;
  } while (!rr || rr >= 1);
  rr = sqrt((-2 * log(rr)) / rr);
  /* sum of two normals has variance 2 or stdv sqrt(2) */
  return rr * (xx + yy) * 0.7071067811865475244;
}
