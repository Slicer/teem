/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
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
	     NULL, &unuBitsHestCB);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(quantizeInfo);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  nin->min = min;
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
