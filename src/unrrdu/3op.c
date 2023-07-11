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

#define INFO "Ternary operation on three nrrds or constants"
static const char *_unrrdu_3opInfoL
  = (INFO ". Can have one, two, or three nrrds, but not zero. "
          "Use \"-\" for an operand to signify "
          "a nrrd to be read from stdin (a pipe).  Note, however, "
          "that \"-\" can probably only be used once (reliably).\n "
          "* Uses nrrdArithIterTernaryOp or (with -w) nrrdArithIterTernaryOpSelect");

static int
unrrdu_3opMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  NrrdIter *in1, *in2, *in3;
  Nrrd *nout, *ntmp = NULL;
  int op, type, E, pret, which;
  airArray *mop;

  hestOptAdd_1_Enum(&opt, NULL, "operator", &op, NULL,
                    "Ternary operator. Possibilities include:\n "
                    "\b\bo \"+\", \"x\": sum or product of three values\n "
                    "\b\bo \"min\", \"max\": minimum, maximum\n "
                    "\b\bo \"min_sm\": smoothed minimum function; "
                    "min_sm(x, w, M) is like min(x,M) but for x > M-w (with w > 0) "
                    "there is a smooth transition from x to asymptotic to M\n "
                    "\b\bo \"max_sm\": smoothed maximum function; "
                    "max_sm(M, w, x) is like max(M,x) but for x < m+w (with w > m) "
                    "there is a smooth transition from x to asymptotic to m\n "
                    "\b\bo \"lt_sm\": 1st less than 3rd, smoothed by 2nd\n "
                    "\b\bo \"gt_sm\": 1st greater than 3rd, smoothed by 2nd\n "
                    "\b\bo \"clamp\": 2nd value is clamped to range between "
                    "the 1st and the 3rd\n "
                    "\b\bo \"ifelse\": if 1st value non-zero, then 2nd value, else "
                    "3rd value\n "
                    "\b\bo \"lerp\": linear interpolation between the 2nd and "
                    "3rd values, as the 1st value varies between 0.0 and 1.0, "
                    "respectively\n "
                    "\b\bo \"exists\": if the 1st value exists, use the 2nd "
                    "value, otherwise use the 3rd\n "
                    "\b\bo \"in_op\": 1 iff 2nd value is > 1st and < 3rd, "
                    "0 otherwise\n "
                    "\b\bo \"in_cl\": 1 iff 2nd value is >= 1st and <= 3rd, "
                    "0 otherwise\n "
                    "\b\bo \"gauss\": evaluate (at 1st value) Gaussian with mean=2nd "
                    "and stdv=3rd value\n "
                    "\b\bo \"rician\": evaluate (at 1st value) Rician with mean=2nd "
                    "and stdv=3rd value",
                    nrrdTernaryOp);
  hestOptAdd_1_Other(&opt, NULL, "in1", &in1, NULL,
                     "First input.  Can be a single value or a nrrd.", nrrdHestIter);
  hestOptAdd_1_Other(&opt, NULL, "in2", &in2, NULL,
                     "Second input.  Can be a single value or a nrrd.", nrrdHestIter);
  hestOptAdd_1_Other(&opt, NULL, "in3", &in3, NULL,
                     "Third input.  Can be a single value or a nrrd.", nrrdHestIter);
  hestOptAdd_1_Other(&opt, "t,type", "type", &type, "default",
                     "type to convert all nrrd inputs to, prior to "
                     "doing operation.  This also determines output type. "
                     "By default (not using this option), the types of the input "
                     "nrrds are left unchanged.",
                     &unrrduHestMaybeTypeCB);
  hestOptAdd_1_Int(&opt, "w,which", "arg", &which, "-1",
                   "Which argument (0, 1, or 2) should be used to determine the "
                   "shape of the output nrrd. By default (not using this option), "
                   "the first non-constant argument is used. ");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_3opInfoL);
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
    if (in3->ownNrrd) {
      if (!E) E |= nrrdConvert(ntmp = nrrdNew(), in3->ownNrrd, type);
      if (!E) nrrdIterSetOwnNrrd(in3, ntmp);
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

  /* HEY: will need to add handling of RNG seed (as in 1op and 2op)
     if there are any 3ops involving random numbers */

  if (-1 == which
        ? nrrdArithIterTernaryOp(nout, op, in1, in2, in3)
        : nrrdArithIterTernaryOpSelect(nout, op, in1, in2, in3, AIR_UINT(which))) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing ternary operation:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(3op, INFO);
