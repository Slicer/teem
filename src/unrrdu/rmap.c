/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

#define INFO "Map nrrd through *regular* univariate map (\"colormap\")"
char *_unrrdu_rmapInfoL =
(INFO
 ". A map is regular if the control points are evenly "
 "spaced along the domain, and hence their position isn't "
 "explicitly represented in the map; the axis min, axis "
 "max, and number of points determine their location. "
 "The map can be a 1D nrrd (for \"grayscale\"), "
 "in which case the "
 "output has the same dimension as the input, "
 "or a 2D nrrd (for \"color\"), in which case "
 "the output has one more dimension than the input.  In "
 "either case, the output is the result of linearly "
 "interpolating between map points, either scalar values "
 "(\"grayscale\"), or scanlines along axis 0 "
 "(\"color\").");

int
unrrdu_rmapMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nmap, *nout;
  airArray *mop;
  int mapax, typeOut, rescale, pret;
  double min, max;

  hestOptAdd(&opt, "m", "map", airTypeOther, 1, 1, &nmap, NULL,
	     "regular map to map input nrrd through",
	     NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&opt, "r", NULL, airTypeInt, 0, 0, &rescale, NULL,
	     "rescale the values from the input nrrd range to the "
	     "map domain, assuming it is explicitly defined");
  hestOptAdd(&opt, "min", "value", airTypeDouble, 1, 1, &min, "nan",
	     "Value to map to low end of map. Defaults to lowest value "
	     "found in input nrrd.  Explicitly setting this (and the "
	     "same for the max) is useful only with rescaling (\"-r\") "
	     "or if map doesn't know its domain");
  hestOptAdd(&opt, "max", "value", airTypeDouble, 1, 1, &max, "nan",
	     "Value to map to high end of map. Defaults to highest value "
	     "found in input nrrd.");
  hestOptAdd(&opt, "t", "type", airTypeOther, 1, 1, &typeOut, "default",
	     "specify the type (\"int\", \"float\", etc.) of the "
	     "output nrrd. "
	     "By default (not using this option), the output type "
	     "is the map's type.",
             NULL, NULL, &unrrduHestMaybeTypeCB);
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_rmapInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (AIR_EXISTS(min))
    nin->min = min;
  if (AIR_EXISTS(max))
    nin->max = max;
  mapax = nmap->dim - 1;
  if (!(AIR_EXISTS(nmap->axis[mapax].min) &&
	AIR_EXISTS(nmap->axis[mapax].max))) {
    if (rescale) {
      fprintf(stderr, "%s: can't rescale to non-existant rmap domain\n", me);
      airMopError(mop);
      return 1;
    } else {
      /* set the map domain to the data range */
      nrrdMinMaxCleverSet(nin);
      nmap->axis[mapax].min = nin->min;
      nmap->axis[mapax].max = nin->max;
      /* fprintf(stderr, "%s: setting rmap domain to (%g,%g)\n",
	 me, nin->min, nin->max); */
    }
  }
  if (rescale) {
    nrrdMinMaxCleverSet(nin);
  }
  if (nrrdTypeDefault == typeOut) {
    typeOut = nmap->type;
  }
  if (nrrdApply1DRegMap(nout, nin, nmap, typeOut, rescale)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble applying map:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(rmap, INFO);
