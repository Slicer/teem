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

#include "privateUnrrdu.h"

char *uopName = "1op";
#define INFO "Unary operation on a nrrd"
char *uopInfo = INFO;
char *uopInfoL = (INFO);

int
uopMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int op;
  airArray *mop;

  hestOptAdd(&opt, NULL, "operator", airTypeEnum, 1, 1, &op, NULL,
	     "Unary operator. Possibilities include:\n "
	     "\b\bo \"-\": negative (multiply by -1.0)\n "
	     "\b\bo \"r\": reciprocal (1.0/value)\n "
	     "\b\bo \"sin\", \"cos\", \"tan\", \"asin\", \"acos\", \"atan\": "
	     "same as in C\n "
	     "\b\bo \"exp\", \"log\", \"log10\": same as in C\n "
	     "\b\bo \"sqrt\", \"ceil\", \"floor\": same as in C\n "
	     "\b\bo \"rup\", \"rdn\": round up or down to integral value\n "
	     "\b\bo \"abs\": absolute value\n "
	     "\b\bo \"sgn\": -1, 0, 1 if value is <0, ==0, or >0\n "
	     "\b\bo \"exists\": 1 iff not NaN or +/-Inf, 0 otherwise",
	     NULL, nrrdUnaryOp);
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(uopInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdArithUnaryOp(nout, op, nin)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing unary operation:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  
  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}
