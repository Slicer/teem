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

char *unblockName = "unblock";
#define INFO "Expand \"blocks\" into scanlines on axis 0"
char *unblockInfo = INFO;
char *unblockInfoL = (INFO
		      ". Based on the requested output type, the number of "
		      "samples along axis 0 will be determined automatically. "
		      "Axis N information will be bumped up to axis N+1. "
		      "Underlying data is unchanged.");

int
unblockMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int type, blockSize;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_TYPE(type, "type to unblock to");
  hestOptAdd(&opt, "bs", "blocksize", airTypeInt, 1, 1, &blockSize, "0",
	     "Useful only if _output_ type is also block: the size of "
	     "blocks in output nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(unblockInfo);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  nout->blockSize = blockSize;
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdUnblock(nout, nin, type)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error unblocking nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(NULL);

  airMopOkay(mop);
  return 0;
}
