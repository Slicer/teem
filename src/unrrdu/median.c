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
                      
  fprintf(stderr, /* 0  1        2       3       4   */
	  "usage: %s <nrrdIn> <radius> <bins> <nrrdOut>\n",
	  me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  FILE *fin, *fout;
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
  if (!(fin = fopen(inStr, "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, inStr);
    exit(1);
  }
  if (!(nin = nrrdNewRead(fin))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error reading nrrd:%s\n", me, err);
    exit(1);
  }
  fclose(fin);
  if (!(nout = nrrdNewMedian(nin, radius, bins))) {
    fprintf(stderr, "%s: error in median filtering:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  if (!(fout = fopen(outStr, "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, outStr);
    exit(1);
  }
  nout->encoding = nin->encoding;
  if (nrrdWrite(fout, nout)) {
    fprintf(stderr, "%s: error writing nrrd:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  fclose(fout);
  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
