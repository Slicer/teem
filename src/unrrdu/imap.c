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

char *imapName = "imap";
#define INFO "Map nrrd through *irregular* univariate map (\"colormap\")"
char *imapInfo = INFO;
char *imapInfoL = (INFO
		   ". A map is irregular if the control points are not evenly "
		   "spaced along the domain, and hence their position must be "
		   "explicitly represented in the map.  As nrrds, these maps "
		   "are necessarily 2D.  Along axis 0, the first value is the "
		   "location of the control point, and the remaining values "
		   "give are the range of the map for that control point. "
		   "The output value(s) is the result of linearly "
		   "interpolating between value(s) from the map.");

int
imapMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nmap, *nout;
  airArray *mop;
  int mapax, rescale;

  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd(&opt, "m", "map", airTypeOther, 1, 1, &nmap, NULL,
	     "regular map to map input nrrd through",
	     NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&opt, "r", NULL, airTypeInt, 0, 0, &rescale, NULL,
	     "rescale the input values from the input range to the "
	     "map domain");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(imapInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  mapax = nmap->dim - 1;
  if (rescale) {
    nrrdMinMaxCleverSet(nin);
  }
  if (nrrdApply1DIrregMap(nout, nin, nmap, NULL, rescale)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble applying map:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(nout, NULL);

  airMopOkay(mop);
  return 0;
}
