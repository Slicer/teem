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
  /*              0      1        2      3        4   */  
  fprintf(stderr,
	  "usage: %s <nrrdIn> <sizeX> <sizeY> <pgmOut>\n", me);
  exit(1);
}

int
main(int argc, char **argv) {
  FILE *fin, *fout;
  char *xStr, *yStr, *inStr, *outStr;
  int sx, sy;
  Nrrd *nin, *nhist, *nimg;

  me = argv[0];
  if (5 != argc)
    usage();
  inStr = argv[1];
  xStr = argv[2];
  yStr = argv[3];
  outStr = argv[4];
  
  if (1 != sscanf(xStr, "%d", &sx) ||
      1 != sscanf(yStr, "%d", &sy)) {
    fprintf(stderr, "%s: couldn't parse %s, %s as histo image dimensions\n",
	    me, xStr, yStr);
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
  printf("reading data\n");
  if (!(nin = nrrdNewRead(fin))) {
    fprintf(stderr, "%s: trouble reading nrrd:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  printf("making histogram; width = %d\n", sx);
  if (!(nhist = nrrdNewHisto(nin, sx))) {
    fprintf(stderr, "%s: trouble making histogram:\n%s\n", 
	    me, biffGet(NRRD));
    exit(1);
  }
  printf("drawing histogram; height = %d\n", sy);
  if (!(nimg = nrrdNewDrawHisto(nhist, sy))) {
    fprintf(stderr, "%s: trouble drawing histogram:\n%s\n",
	    me, biffGet(NRRD));
    exit(1);
  }
  nimg->encoding = nrrdEncodingRaw;
  if (nrrdWritePNM(fout, nimg)) {
    fprintf(stderr, "%s: trouble writing histogram image:\n%s\n",
	    me, biffGet(NRRD));
    exit(1);
  }
  nrrdNuke(nin);
  nrrdNuke(nhist);
  nrrdNuke(nimg);
  exit(0);
}
