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


#include <ten.h>

char *me;

float b = 1.0;    /* supposedly reasonable default diffusion weighting */

void
usage() {
  /*                      0     1        2       3        4     (5) */
  fprintf(stderr, "usage: %s <nrrdIn> <slope> <thresh> <nrrdOut>\n", me);
  fprintf(stderr, "       slope: around 0.5\n");
  fprintf(stderr, "       thresh: around 710\n");
  exit(1);
}

int
main(int argc, char **argv) {
  char *inS, *slopeS, *threshS, *outS;
  Nrrd *nin, *nout;
  float slope, thresh;

  me = argv[0];
  if (5 != argc) {
    usage();
  }
  inS = argv[1];
  slopeS = argv[2];
  threshS = argv[3];
  outS = argv[4];

  if (2 != (sscanf(slopeS, "%g", &slope) + 
	    sscanf(threshS, "%g", &thresh)) ) {
    fprintf(stderr, "%s: couldn't parse %s and %s as floats\n", 
	    me, slopeS, threshS);
    exit(1);
  }

  if (nrrdLoad(nin=nrrdNew(), inS)) {
    printf("%s: couldn't read input \"%s\":\n%s", me, inS, biffGet(NRRD));
    exit(1);
  }

  printf("calculating tensors ..."); fflush(stdout);
  tenVerbose = 2;
  if (tenCalcTensor(nout=nrrdNew(), nin, thresh, slope, b)) {
    printf("%s: tenCalcTensor failed:\n%s", me, biffGet(TEN));
    exit(1);
  }
  printf("done\n");

  if (nrrdSave(outS, nout, NULL)) {
    printf("%s: couldn't save output to \"%s\":\n%s", me, outS, biffGet(NRRD));
    exit(1);
  }

  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
