/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Binary operation on two nrrds, or on a nrrd and a constant"
char *_unrrdu_2opInfoL =
(INFO
 ". Either the first or second operand can be a float, "
 "but not both.  Use \"-\" for an operand to signify "
 "a nrrd to be read from stdin (a pipe).  Note, however, "
 "that \"-\" can probably only be used once (reliably).");

int
unrrdu_2opMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  NrrdIter *in1, *in2;
  Nrrd *nout, *ntmp=NULL;
  int op, type, E;
  airArray *mop;

  hestOptAdd(&opt, NULL, "operator", airTypeEnum, 1, 1, &op, NULL,
	     "Binary operator. Possibilities include:\n "
	     "\b\bo \"+\", \"-\", \"x\", \"/\": "
	     "add, subtract, multiply, divide\n "
	     "\b\bo \"^\": exponentiation\n "
	     "\b\bo \"%\": integer modulo\n "
	     "\b\bo \"fmod\": same as fmod() in C\n "
	     "\b\bo \"atan2\": same as atan2() in C\n "
	     "\b\bo \"min\", \"max\": minimum, maximum\n "
	     "\b\bo \"lt\": 1 if 1st value less than 2nd value, otherwise 0\n "
	     "\b\bo \"comp\": -1, 0, or 1 if 1st value is less than,"
             "equal to, or greater than 2nd value",
	     NULL, nrrdBinaryOp);
  hestOptAdd(&opt, NULL, "in1", airTypeOther, 1, 1, &in1, NULL,
	     "First input.  Can be float or nrrd.",
	     NULL, NULL, nrrdHestNrrdIter);
  hestOptAdd(&opt, NULL, "in2", airTypeOther, 1, 1, &in2, NULL,
	     "Second input.  Can be float or nrrd.",
	     NULL, NULL, nrrdHestNrrdIter);
  hestOptAdd(&opt, "t", "type", airTypeOther, 1, 1, &type, "unknown",
	     "type to convert all nrrd inputs to, prior to "
	     "doing operation.  This also determines output type. "
	     "By default (not using this option), the types of the input "
	     "nrrds are left unchanged.",
             NULL, NULL, &unrrduMaybeTypeHestCB);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_2opInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  /*
  fprintf(stderr, "%s: op = %d\n", me, op);
  fprintf(stderr, "%s: in1->left = %d, in2->left = %d\n", me, 
	  (int)(in1->left), (int)(in2->left));
  */
  if (nrrdTypeUnknown != type) {
    /* they wanted to convert nrrds to some other type first */
    E = 0;
    if (in1->nrrd) {
      if (!E) E |= nrrdConvert(ntmp=nrrdNew(), in1->nrrd, type);
      if (!E) { nrrdNuke(in1->nrrd); nrrdIterSetNrrd(in1, ntmp); }
    }
    if (in2->nrrd) {
      if (!E) E |= nrrdConvert(ntmp=nrrdNew(), in2->nrrd, type);
      if (!E) { nrrdNuke(in2->nrrd); nrrdIterSetNrrd(in2, ntmp); }
    }
    if (E) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error converting input nrrd(s):\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    /* this will still leave a nrrd in the NrrdIter for nrrdIterNuke()
       (called by hestParseFree() called be airMopOkay()) to clear up */
  }
  if (nrrdArithBinaryOp(nout, op, in1, in2)) {
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
