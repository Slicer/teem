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

char *blockName = "block";
#define INFO "Condense axis-0 scanlines into \"blocks\""
char *blockInfo = INFO;
char *blockInfoL = (INFO
		    ". Output nrrd will be of type \"block\": the type "
		    "for an opaque chunk of "
		    "memory.  Block samples can be sliced, cropped, shuffled, "
		    "permuted, etc., but there is no scalar value associated "
		    "with them, so they can not be histogrammed, quantized, "
		    "resampled, converted, etc.  The output nrrd will have "
		    "one less dimension than input; axis N information will "
		    "be shifted down to axis N-1.  Underlying data "
		    "is unchanged.");

int
blockMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  airArray *mop;

  /* if we gave a default for this, then there it would fine to have 
     no command-line arguments whatsoever, and then the user would not
     know how to get the basic usage information */
  hestOptAdd(&opt, "i", "nin", airTypeOther, 1, 1, &nin, NULL, "input nrrd",
	     NULL, &unuNrrdHestCB);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(blockInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdBlock(nout, nin)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error blocking nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(NULL);

  airMopOkay(mop);
  return 0;
}
