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

char *heqName = "heq";
#define INFO "Perform histogram equalization"
char *heqInfo = INFO;
char *heqInfoL = (INFO
		   ". ");

int
heqMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nrrd;
  int bins, smart;
  airArray *mop;

  OPT_ADD_NIN(nrrd, "input nrrd");
  hestOptAdd(&opt, "b", "bins", airTypeInt, 1, 1, &bins, NULL,
	     "# bins to use in histogram that is created in order to "
	     "calculate the mapping that achieves the equalization.");
  hestOptAdd(&opt, "s", "bins", airTypeInt, 0, 1, &smart, "0",
	     "# bins in value histogram to ignore in calculating the mapping. "
	     "Bins are ignored when they get more hits than other bins, and "
	     "when the values that fall in them are constant.  This is an "
	     "effective way to prevent large regions of background value "
	     "from distorting the equalization mapping.");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(heqInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (nrrdHistoEq(nrrd, NULL, bins, smart)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble histogram equalizing:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(nrrd, NULL);

  airMopOkay(mop);
  return 0;
}
