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


#include <nrrd.h>

int
usage(char *me) {
  /*               0      1        2      3        4   */  
  fprintf(stderr,
	  "usage: %s <nrrdIn> <axis> <bins> <nrrdOut>\n", me);
  return 1;
}

int
main(int argc, char **argv) {
  char *me, *err, *inStr, *axStr, *binStr, *outStr;
  int axis, bins;
  Nrrd *nin, *nout;

  me = argv[0];
  if (5 != argc)
    return usage(me);
  inStr = argv[1];
  axStr = argv[2];
  binStr = argv[3];
  outStr = argv[4];
  
  if (1 != sscanf(axStr, "%d", &axis) ||
      1 != sscanf(binStr, "%d", &bins)) {
    fprintf(stderr, "%s: couldn't parse %s as axis or %s as bins\n",
	    me, axStr, binStr);
    return 1;
  }
  if (nrrdLoad(nin=nrrdNew(), inStr)) {
    err = biffGet(NRRD);
    fprintf(stderr, 
	    "%s: trouble reading nrrd from \"%s\":\n%s\n", me, inStr, err);
    free(err);
    return 1;
  }
  
  nout = nrrdNew();
  if (nrrdHistoAxis(nout, nin, axis, bins, nrrdTypeUChar)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble in nrrdHistoAxis:\n%s\n", me, err);
    free(err);
    return 1;
  }
  if (nrrdSave(outStr, nout, NULL)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble writing output to \"%s\":\n%s\n",
	    me, outStr, err);
    free(err);
    return 1;
  }

  nrrdNuke(nin);
  nrrdNuke(nout);
  return 0;
}
