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

char *me;

void
usage() {
  /*                0      1        2      3        4   */  
  fprintf(stderr,
	  "usage: histax <nrrdIn> <axis> <bins> <nrrdOut>\n");
  exit(1);
}

int
main(int argc, char **argv) {
  char *err, *inStr, *axStr, *binStr, *outStr;
  int axis, bins;
  Nrrd *nin, *nout;

  me = argv[0];
  if (5 != argc)
    usage();
  inStr = argv[1];
  axStr = argv[2];
  binStr = argv[3];
  outStr = argv[4];
  
  if (1 != sscanf(axStr, "%d", &axis) ||
      1 != sscanf(binStr, "%d", &bins)) {
    fprintf(stderr, "%s: couldn't parse %s as axis or %s as bins\n",
	    me, axStr, binStr);
    exit(1);
  }
  if (!(nin = nrrdNewLoad(inStr))) {
    err = biffGet(NRRD);
    fprintf(stderr, 
	    "%s: trouble reading nrrd from \"%s\":\n%s\n", me, inStr, err);
    free(err);
    exit(1);
  }
  
  if (!(nout = nrrdNewHistoAxis(nin, axis, bins))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble in nrrdNewHistoAxis:\n%s\n", me, err);
    free(err);
    exit(1);
  }
  nout->encoding = nrrdEncodingRaw;
  if (nrrdSave(outStr, nout)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble writing output to \"%s\":\n%s\n",
	    me, outStr, err);
    free(err);
    exit(1);
  }
  exit(0);
}
