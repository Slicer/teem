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

#define INFO "Quantize floating-point values to 8, 16, or 32 bits"
char *_unrrdu_quantizeInfoL = 
(INFO ". Values are clamped to the min and max before they are "
 "quantized, so there is no risk of getting 255 where you expect 0 "
 "(with unsigned char output, for example)");

int
unrrdu_quantizeMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int bits, pret;
  double min, max;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd(&opt, "b", "bits", airTypeOther, 1, 1, &bits, NULL,
	     "Number of bits to quantize down to; determines the type "
	     "of the output nrrd:\n "
	     "\b\bo \"8\": unsigned char\n "
	     "\b\bo \"16\": unsigned short\n "
	     "\b\bo \"32\": unsigned int",
	     NULL, NULL, &unrrduHestBitsCB);
  hestOptAdd(&opt, "min", "value", airTypeDouble, 1, 1, &min, "nan",
	     "Value to map to zero. Defaults to lowest value found in "
	     "input nrrd.");
  hestOptAdd(&opt, "max", "value", airTypeDouble, 1, 1, &max, "nan",
	     "Value to map to highest unsigned integral value. "
	     "Defaults to highest value found in input nrrd.");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_quantizeInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  /* If the input nrrd never specified min and max, then they'll be
     AIR_NAN, and nrrdMinMaxCleverSet will find them.  If the input nrrd
     had a notion of min and max, we should respect it, but not if the
     user specified something else. */
  if (AIR_EXISTS(min))
    nin->min = min;
  if (AIR_EXISTS(max))
    nin->max = max;
  if (nrrdQuantize(nout, nin, bits)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error quantizing nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(quantize, INFO);
