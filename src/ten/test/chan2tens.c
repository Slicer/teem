/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
    fprintf(stderr, 
	    "%s: couldn't read input \"%s\":\n%s", me, inS, biffGet(NRRD));
    exit(1);
  }

  fprintf(stderr, "calculating tensors ..."); fflush(stdout);
  tenVerbose = 2;
  if (tenCalcTensor(nout=nrrdNew(), nin, thresh, slope, b)) {
    fprintf(stderr, "%s: tenCalcTensor failed:\n%s", me, biffGet(TEN));
    exit(1);
  }
  fprintf(stderr, "done\n");

  if (nrrdSave(outS, nout, NULL)) {
    fprintf(stderr, "%s: couldn't save output to \"%s\":\n%s",
	    me, outS, biffGet(NRRD));
    exit(1);
  }

  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
