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

#include "private.h"

char *topName = "3op";
#define INFO "Ternary operation on three nrrds or constants"
char *topInfo = INFO;
char *topInfoL = (INFO
		  ". Can have one, two, or three nrrds, but not zero. "
		  "Use \"-\" for an operand to signify "
		  "a nrrd to be read from stdin (a pipe).  Note, however, "
		  "that \"-\" can probably only be used once (reliably).");

int
topMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  NrrdIter *in1, *in2, *in3;
  Nrrd *nout, *ntmp;
  int op, type, E;
  airArray *mop;

  hestOptAdd(&opt, NULL, "operator", airTypeEnum, 1, 1, &op, NULL,
	     "Ternary operator. Possibilities include:\n "
	     "\b\bo \"clamp\": second value is clamped to range between "
	     "the first and the third\n "
	     "\b\bo \"lerp\": linear interpolation between the second and "
	     "third values, as the first value varies between 0.0 and 1.0, "
	     "respectively",
	     NULL, nrrdTernaryOp);
  hestOptAdd(&opt, NULL, "in1", airTypeOther, 1, 1, &in1, NULL,
	     "First input.  Can be float or nrrd.",
	     NULL, NULL, nrrdHestNrrdIter);
  hestOptAdd(&opt, NULL, "in2", airTypeOther, 1, 1, &in2, NULL,
	     "Second input.  Can be float or nrrd.",
	     NULL, NULL, nrrdHestNrrdIter);
  hestOptAdd(&opt, NULL, "in3", airTypeOther, 1, 1, &in3, NULL,
	     "Third input.  Can be float or nrrd.",
	     NULL, NULL, nrrdHestNrrdIter);
  hestOptAdd(&opt, "t", "type", airTypeOther, 1, 1, &type, "unknown",
	     "type to convert all nrrd inputs to, prior to "
	     "doing operation.  This also determines output type. "
	     "By default (not using this option), the types of the input "
	     "nrrds are left unchanged.",
             NULL, NULL, &unuMaybeTypeHestCB);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(topInfoL);
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
    if (in3->nrrd) {
      if (!E) E |= nrrdConvert(ntmp=nrrdNew(), in3->nrrd, type);
      if (!E) { nrrdNuke(in3->nrrd); nrrdIterSetNrrd(in3, ntmp); }
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
  if (nrrdArithTernaryOp(nout, op, in1, in2, in3)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing ternary operation:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  
  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}
