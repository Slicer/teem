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
char *histoInfo = "Create (PGM) image of histogram of values in a nrrd";

int
histoMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout, *nhist;
  int size[2];
  airArray *mop;

  OPT_ADD_NIN(nin, "input");
  hestOptAdd(&opt, "s|size", "sizeX sizeY", airTypeInt, 2, 2, size, NULL,
	     "size of output image");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(histoInfo);
  PARSE();

  nhist = nrrdNew();
  airMopAdd(mop, nhist, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdHisto(nhist, nin, size[0], nrrdTypeInt)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error calculating histogram:\n%s", me, err);
    free(err);
    return 1;
  }

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdHistoDraw(nout, nhist, size[1])) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error drawing histogram nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE();

  airMopOkay(mop);
  return 0;
}
