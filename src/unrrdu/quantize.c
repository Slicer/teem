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

char *quantizeName = "quantize";
char *quantizeInfo = "Quantize floating-point values to 8, 16, or 32 bits";

int
unuParseBits(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unuParseBits";
  int *bitsP;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  bitsP = ptr;
  if (1 != sscanf(str, "%d", bitsP)) {
    sprintf(err, "%s: can't parse \"%s\" as int", me, str);
    return 1;
  }
  if (!( 8 == *bitsP || 16 == *bitsP || 32 == *bitsP )) {
    sprintf(err, "%s: bits (%d) not 8, 16, or 32", me, *bitsP);
    return 1;
  }
  return 0;
}

hestCB unuBitsHestCB = {
  sizeof(int),
  "quantization bits",
  unuParseBits,
  NULL
};

int
quantizeMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int bits;
  double min, max;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd(&opt, "min", "value", airTypeDouble, 1, 1, &min, "nan",
	     "Value to map to zero. Defaults to lowest value found in "
	     "input nrrd.");
  hestOptAdd(&opt, "max", "value", airTypeDouble, 1, 1, &max, "nan",
	     "Value to map to highest unsigned integral value. "
	     "Defaults to highest value found in input nrrd.");
  hestOptAdd(&opt, "b", "bits", airTypeOther, 1, 1, &bits, NULL,
	     "Number of bits to quantize down to; determines the type "
	     "of the output nrrd:\n "
	     "\b\bo \"8\": unsigned char\n "
	     "\b\bo \"16\": unsigned short\n "
	     "\b\bo \"32\": unsigned int",
	     NULL, NULL, &unuBitsHestCB);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(quantizeInfo);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  /* If the input nrrd never specified min and max, then they'll be
     AIR_NAN, and nrrdMinMaxClever will find them.  If the input nrrd
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

  SAVE(nout, NULL);

  airMopOkay(mop);
  return 0;
}
