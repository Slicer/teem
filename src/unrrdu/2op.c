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

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Binary operation on two nrrds, or on a nrrd and a constant"
static const char *_unrrdu_2opInfoL
  = (INFO ". Either the first or second operand can be a float constant, "
          "but not both.  Use \"-\" for an operand to signify "
          "a nrrd to be read from stdin (a pipe).  Note, however, "
          "that \"-\" can probably only be used once (reliably).\n "
          "* Uses nrrdArithIterBinaryOp or (with -w) nrrdArithIterBinaryOpSelect");

static int
unrrdu_2opMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  NrrdIter *in1, *in2;
  Nrrd *nout, *ntmp = NULL;
  int op, type, E, pret, which;
  airArray *mop;
  unsigned int seed, seedOI;

  hestOptAdd_1_Enum(&opt, NULL, "operator", &op, NULL,
                    "Binary operator. Possibilities include:\n "
                    "\b\bo \"+\", \"-\", \"x\", \"/\": "
                    "add, subtract, multiply, divide\n "
                    "\b\bo \"+c\", \"-c\", \"xc\": add, subtract, multiply, with "
                    "clamping to range of output, in case its integral\n "
                    "\b\bo \"^\": exponentiation (pow)\n "
                    "\b\bo \"spow\": signed exponentiation: sgn(x)pow(abs(x),p)\n "
                    "\b\bo \"fpow\": like spow but with curves flipped\n "
                    "\b\bo \"%\": integer modulo\n "
                    "\b\bo \"fmod\": same as fmod() in C\n "
                    "\b\bo \"atan2\": same as atan2() in C\n "
                    "\b\bo \"min\", \"max\": minimum, maximum\n "
                    "\b\bo \"lt\", \"lte\", \"gt\", \"gte\": same as C's <, <=, >, <=\n "
                    "\b\bo \"eq\", \"neq\": same as C's == and !=\n "
                    "\b\bo \"comp\": -1, 0, or 1 if 1st value is less than, "
                    "equal to, or greater than 2nd value\n "
                    "\b\bo \"if\": if 1st value is non-zero, use it, "
                    "else use 2nd value\n "
                    "\b\bo \"exists\": if 1st value exists, use it, "
                    "else use 2nd value\n "
                    "\b\bo \"nrand\": scale unit-stdv Gaussian noise by 2nd value "
                    "and add to first value\n "
                    "\b\bo \"rrand\": sample Rician distribution with 1st value "
                    "for \"true\" mean, and 2nd value for sigma",
                    nrrdBinaryOp);
  hestOptAdd_1_Other(&opt, NULL, "in1", &in1, NULL,
                     "First input.  Can be a single value or a nrrd.", nrrdHestIter);
  hestOptAdd_1_Other(&opt, NULL, "in2", &in2, NULL,
                     "Second input.  Can be a single value or a nrrd.", nrrdHestIter);
  seedOI = hestOptAdd_1_UInt(&opt, "s,seed", "seed", &seed, "0",
                             "seed value for random number generator (RNG), to "
                             "enable repeatable results between runs, or, "
                             "by not using this option, the RNG seeding will be "
                             "based on the current time");
  hestOptAdd_1_Other(&opt, "t,type", "type", &type, "default",
                     "type to convert all INPUT nrrds to, prior to "
                     "doing operation, useful for doing, for instance, the difference "
                     "between two unsigned char nrrds.  This will also determine "
                     "output type. By default (not using this option), the types of "
                     "the input nrrds are left unchanged.",
                     &unrrduHestMaybeTypeCB);
  hestOptAdd_1_Int(&opt, "w,which", "arg", &which, "-1",
                   "Which argument (0 or 1) should be used to determine the "
                   "shape of the output nrrd. By default (not using this option), "
                   "the first non-constant argument is used. ");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_2opInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  /*
  fprintf(stderr, "%s: op = %d\n", me, op);
  fprintf(stderr, "%s: in1->left = %d, in2->left = %d\n", me,
          (int)(in1->left), (int)(in2->left));
  */
  if (nrrdTypeDefault != type) {
    /* they wanted to convert nrrds to some other type first */
    E = 0;
    if (in1->ownNrrd) {
      if (!E) E |= nrrdConvert(ntmp = nrrdNew(), in1->ownNrrd, type);
      if (!E) nrrdIterSetOwnNrrd(in1, ntmp);
    }
    if (in2->ownNrrd) {
      if (!E) E |= nrrdConvert(ntmp = nrrdNew(), in2->ownNrrd, type);
      if (!E) nrrdIterSetOwnNrrd(in2, ntmp);
    }
    if (E) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error converting input nrrd(s):\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    /* this will still leave a nrrd in the NrrdIter for nrrdIterNix()
       (called by hestParseFree() called be airMopOkay()) to clear up */
  }
  /*
  ** Used to only deal with RNG seed for particular op:
  **   if (nrrdBinaryOpNormalRandScaleAdd == op) {
  ** but then when nrrdBinaryOpRicianRand was added, then the seed wasn't being
  ** used ==> BUG.  To be more future proof, we try to parse and use seed
  ** whenever the user exercies this option, and end up *ALWAYS* calling
  ** airSrandMT, even for operations that have nothing to do with random
  ** numbers.  Could also have a new array that indicates if an op involves
  ** the RNG, but this would add rarely-needed complexity
  */
  if (hestSourceDefault != opt[seedOI].source) {
    airSrandMT(seed);
  } else {
    /* got no request for specific seed */
    airSrandMT(AIR_UINT(airTime()));
  }
  if (-1 == which ? nrrdArithIterBinaryOp(nout, op, in1, in2)
                  : nrrdArithIterBinaryOpSelect(nout, op, in1, in2, AIR_UINT(which))) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing binary operation:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(2op, INFO);
