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

#define INFO "Collapse scanlines to scalars along some axis"
char *_unrrdu_projectInfoL = (INFO
			   ". The scanline is reduced to a single scalar by "
			   "\"measuring\" all the values in the scanline "
			   "with some measure.  The output nrrd has dimension "
			   "one less than input; the output type depends on "
			   "the measure.");

int
unrrdu_projectMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int axis, measr;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_AXIS(axis, "axis to project along");
  hestOptAdd(&opt, "m", "measr", airTypeEnum, 1, 1, &measr, NULL,
	     "How to \"measure\" a scanline.  Possibilities include:\n "
	     "\b\bo \"min\", \"max\", \"mean\", \"median\", \"mode\", "
	     "\"variance\"\n "
	     "(self-explanatory)\n "
	     "\b\bo \"SD\": standard deviation\n "
	     "\b\bo \"product\", \"sum\": product or sum of all values along "
	     "scanline\n "
	     "\b\bo \"L1\", \"L2\", \"Linf\": different norms\n "
	     "\b\bo \"histo-min\",  \"histo-max\", \"histo-mean\", "
	     "\"histo-median\", \"histo-mode\", \"histo-product\", "
	     "\"histo-sum\", \"histo-variance\": same measures, but for when "
	     "the scanlines are histograms of values, not the values "
	     "themselves.", 
	     NULL, nrrdMeasure);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_projectInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdProject(nout, nin, axis, measr)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error projecting nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(project, INFO);
