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

char *rmapName = "rmap";
#define INFO "Map nrrd through *regular* univariate map (\"colormap\")"
char *rmapInfo = INFO;
char *rmapInfoL = (INFO
		   ". A map is regular if the control points are evenly "
		   "spaced along the domain, and hence their position isn't "
		   "explicitly represented in the map; the axis min, axis "
		   "max, and number of points determine their location. "
		   "The map can be 1D (\"grayscale\"), in which case the "
		   "output has the same dimension as the input, "
		   "or 2D (\"color\"), in which case "
		   "the output has one more dimension than the input.  In "
		   "either case, the output is the result of linearly "
		   "interpolating between map points, either scalar values "
		   "(\"grayscale\"), or scanlines along axis 0 "
		   "(\"color\").");

int
rmapMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nmap, *nout;
  airArray *mop;
  int mapax;

  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd(&opt, "m", "map", airTypeOther, 1, 1, &nmap, NULL,
	     "regular map to map input nrrd through",
	     NULL, NULL, nrrdHestNrrd);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(rmapInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  mapax = nmap->dim - 1;
  if (!(AIR_EXISTS(nmap->axis[mapax].min) &&
	AIR_EXISTS(nmap->axis[mapax].max))) {
    /* gracelessly set the map min and max to the data range */
    nrrdMinMaxCleverSet(nin);
    nmap->axis[mapax].min = nin->min;
    nmap->axis[mapax].max = nin->max;
  }
  if (nrrdApply1DRegMap(nout, nin, nmap, AIR_FALSE)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble applying map:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(nout, NULL);

  airMopOkay(mop);
  return 0;
}
