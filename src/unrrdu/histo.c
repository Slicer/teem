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
  FILE *fout;
  char *err, *xStr, *yStr, *inStr, *outStr;
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
  if (!strcmp(outStr, "-")) {
    fprintf(stderr, "%s: sorry, can't write PNMs to stdout yet\n", me);
    exit(1);
  }
  if (!(nin = nrrdNewLoad(inStr))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: can't read nrrd from \"%s\":\n%s", me, inStr, err);
    free(err);
    exit(1);
  }
  if (!(nhist = nrrdNewHisto(nin, sx))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble making histogram:\n%s\n", me, err);
    free(err);
    exit(1);
  }
  if (!(nimg = nrrdNewDrawHisto(nhist, sy))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble drawing histogram:\n%s\n", me, err);
    free(err);
    exit(1);
  }
  nimg->encoding = nrrdEncodingRaw;
  if (!(fout = fopen(outStr, "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, outStr);
    exit(1);
  }
  if (nrrdWritePNM(fout, nimg)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble writing histogram image to \"%s\":\n%s\n",
	    me, outStr, err);
    free(err);
    exit(1);
  }
  fclose(fout);

  nrrdNuke(nin);
  nrrdNuke(nhist);
  nrrdNuke(nimg);
  exit(0);
}
