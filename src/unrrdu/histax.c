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
  FILE *fin, *fout;
  char *inStr, *axStr, *binStr, *outStr;
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
  if (!(fin = fopen(inStr, "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, inStr);
    exit(1);
  }
  if (!(fout = fopen(outStr, "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, outStr);
    exit(1);
  }
  if (!(nin = nrrdNewRead(fin))) {
    fprintf(stderr, "%s: trouble reading nrrd:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  if (!(nout = nrrdNewHistoAxis(nin, axis, bins))) {
    fprintf(stderr, "%s: trouble in nrrdNewHistoAxis:\n%s\n", 
	    me, biffGet(NRRD));
    exit(1);
  }
  nout->encoding = nrrdEncodingRaw;
  if (nrrdWrite(fout, nout)) {
    fprintf(stderr, "%s: trouble writing output:\n%s\n",
	    me, biffGet(NRRD));
    exit(1);
  }
}
