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

char *heqName = "heq";
#define INFO "Perform histogram equalization"
char *heqInfo = INFO;
char *heqInfoL = (INFO
		  ". ");

int
heqMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err, *mapS;
  Nrrd *nin, *nout, *nmap;
  int bins, smart;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd(&opt, "b", "bins", airTypeInt, 1, 1, &bins, NULL,
	     "# bins to use in histogram that is created in order to "
	     "calculate the mapping that achieves the equalization.");
  hestOptAdd(&opt, "s", "bins", airTypeInt, 0, 1, &smart, "0",
	     "# bins in value histogram to ignore in calculating the mapping. "
	     "Bins are ignored when they get more hits than other bins, and "
	     "when the values that fall in them are constant.  This is an "
	     "effective way to prevent large regions of background value "
	     "from distorting the equalization mapping.");
  hestOptAdd(&opt, "m", "filename", airTypeString, 1, 1, &mapS, "",
	     "The value mapping used to achieve histogram equalization is "
	     "represented by a univariate regular map.  By giving a filename "
	     "here, that map can be saved out and applied to other nrrds "
	     "with \"unu rmap\"");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(heqInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdHistoEq(nout, nin, airStrlen(mapS) ? &nmap : NULL, bins, smart)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble histogram equalizing:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  if (airStrlen(mapS)) {
    SAVE(mapS, nmap, NULL);
  }
  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}
