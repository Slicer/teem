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

#define INFO "Map nrrd through univariate lookup table"
char *_unrrdu_lutInfoL =
(INFO
 " (itself represented as a nrrd). The lookup table "
 "can be 1D, in which case the output "
 "has the same dimension as the input, or 2D, in which case "
 "the output has one more dimension than the input, and each "
 "value is mapped to a scanline (along axis 0) from the "
 "lookup table.");

int
unrrdu_lutMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nlut, *nout;
  airArray *mop;
  int lutax, typeOut, rescale, pret;

  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd(&opt, "m", "lut", airTypeOther, 1, 1, &nlut, NULL,
	     "lookup table to map input nrrd through",
	     NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&opt, "r", NULL, airTypeInt, 0, 0, &rescale, NULL,
	     "rescale the input values from the input range to the "
	     "lut domain, assuming it is explicitly defined");
  hestOptAdd(&opt, "t", "type", airTypeOther, 1, 1, &typeOut, "unknown",
	     "specify the type (\"int\", \"float\", etc.) of the "
	     "output nrrd. "
	     "By default (not using this option), the output type "
	     "is the lut's type.",
             NULL, NULL, &unrrduHestMaybeTypeCB);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_lutInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  lutax = nlut->dim - 1;
  if (!( AIR_EXISTS(nlut->axis[lutax].min) &&
	 AIR_EXISTS(nlut->axis[lutax].max) )) {
    if (rescale) {
      fprintf(stderr, "%s: can't rescale to non-existant lut domain\n", me);
      airMopError(mop);
      return 1;
    } else {
      /* set the lut domain to the data range */
      nrrdMinMaxCleverSet(nin);
      nlut->axis[lutax].min = nin->min;
      nlut->axis[lutax].max = nin->max;
      fprintf(stderr, "%s: setting lut domain to (%g,%g)\n",
	      me, nin->min, nin->max);
    }
  }
  if (rescale) {
    nrrdMinMaxCleverSet(nin);
  }
  if (nrrdTypeUnknown == typeOut) {
    typeOut = nlut->type;
  }
  if (nrrdApply1DLut(nout, nin, nlut, typeOut, rescale)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble applying LUT:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(lut, INFO);
