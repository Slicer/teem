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

char *histoName = "histo";
char *histoInfo = "Create 1-D histogram of values in a nrrd";

int
histoMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int bins, type;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd(&opt, "b", "bins", airTypeInt, 1, 1, &bins, NULL,
	     "# of bins in histogram");
  hestOptAdd(&opt, "t", "type", airTypeOther, 1, 1, &type, "int",
	     "type to use for bins in output histogram",
             NULL, &unuTypeHestCB);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(histoInfo);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdHisto(nout, nin, bins, type)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error calculating histogram:\n%s", me, err);
    free(err);
    return 1;
  }

  SAVE(nout, NULL);

  airMopOkay(mop);
  return 0;
}
