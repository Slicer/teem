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

#define INFO "Merge islands into their surrounding"
char *_unrrdu_ccmergeInfoL =
(INFO
 ".  This operates on the output of \"ccfind\". "
 "Connected components (CCs) with only one adjacent CC (that is, islands) "
 "are absorbed into their surrounding if they are smaller than the "
 "surround, and are smaller then some given maximum significant size. ");

int
unrrdu_ccmergeMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout, *nval;
  airArray *mop;
  int conny, pret, maxSize, dir, maxNeigh;

  hestOptAdd(&opt, "v", "values", airTypeOther, 1, 1, &nval, "",
	     "result of using \"ccfind -v\", the record of which values "
	     "were originally associated with each CC.  This is required "
	     "for value-directed merging (with non-zero \"-d\" option), "
	     "but is not needed otherwise",
	     NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&opt, "d", "dir", airTypeInt, 1, 1, &dir, "0",
	     "do value-driven merging.  Using (positive) \"1\" says that "
	     "dark islands get merged with bright surrounds, while \"-1\" "
	     "says the opposite.  By default, merging can go either way. ");
  hestOptAdd(&opt, "s", "max size", airTypeInt, 1, 1, &maxSize, "0",
	     "a cap on the CC size that will be absorbed into its "
	     "surround.  CCs larger than this are deemed too significant "
	     "to mess with.  Or, use \"0\" to remove any such restriction "
	     "on merging.");
  hestOptAdd(&opt, "n", "max # neigh", airTypeInt, 1, 1, &maxNeigh, "1",
	     "a cap on the number of neighbors that a CC may have if it is "
	     "to be be merged.  \"1\" allows only islands to be merged, "
	     "\"2\" does merging with bigger of two neighbors, etc, while "
	     "\"0\" says that number of neighbors is no constraint");
  hestOptAdd(&opt, "c", "connectivity", airTypeInt, 1, 1, &conny, NULL,
	     "what kind of connectivity to use: the number of coordinates "
	     "that vary in order to traverse the neighborhood of a given "
	     "sample.  In 2D: \"1\": 4-connected, \"2\": 8-connected");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_ccmergeInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdCCMerge(nout, nin, nval, dir, maxSize, maxNeigh, conny)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing merging:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(ccmerge, INFO);
