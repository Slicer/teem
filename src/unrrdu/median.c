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
  /*               0    1        2       3       4     (5) */
  fprintf(stderr, 
	  "usage: %s <nrrdIn> <radius> <bins> <nrrdOut>\n",
	  me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  char *inStr, *outStr, *radiusStr, *binsStr, *err;
  int radius, bins;
  Nrrd *nin, *nout;

  me = argv[0];
  if (argc != 5) {
    usage();
  }

  inStr = argv[1];
  radiusStr = argv[2];
  binsStr = argv[3];
  outStr = argv[4];

  if (1 != sscanf(radiusStr, "%d", &radius)) {
    fprintf(stderr, "%s: couldn't parse %s as int\n", me, radiusStr);
    exit(1);
  }
  if (1 != sscanf(binsStr, "%d", &bins)) {
    fprintf(stderr, "%s: couldn't parse %s as int\n", me, binsStr);
    exit(1);
  }

  if (!(nin = nrrdNewOpen(inStr))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error reading nrrd from \"%s\":%s\n", me, inStr, err);
    free(err);
    exit(1);
  }
  if (!(nout = nrrdNewMedian(nin, radius, bins))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error in median filtering:\n%s", me, err);
    free(err);
    exit(1);
  }
  nout->encoding = nrrdEncodingRaw;
  if (nrrdSave(outStr, nout)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error writing nrrd to \"%s\":\n%s", me, outStr, err);
    free(err);
    exit(1);
  }

  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
