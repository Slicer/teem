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
  /*              0      1        2      3        4   */  
  fprintf(stderr,
	  "usage: %s <nrrdIn> <sizeX> <sizeY> <pgmOut>\n", me);
  return 1;
}

int
main(int argc, char **argv) {
  char *me, *err, *xStr, *yStr, *inStr, *outStr;
  int sx, sy;
  Nrrd *nin, *nhist, *nimg;
  nrrdIO *io;

  me = argv[0];
  if (5 != argc)
    return usage(me);
  inStr = argv[1];
  xStr = argv[2];
  yStr = argv[3];
  outStr = argv[4];
  
  if (1 != sscanf(xStr, "%d", &sx) ||
      1 != sscanf(yStr, "%d", &sy)) {
    fprintf(stderr, "%s: couldn't parse %s, %s as histo image dimensions\n",
	    me, xStr, yStr);
    return 1;
  }
  if (!strcmp(outStr, "-")) {
    fprintf(stderr, "%s: sorry, can't write PNMs to stdout yet\n", me);
    return 1;
  }
  if (nrrdLoad(nin=nrrdNew(), inStr)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: can't read nrrd from \"%s\":\n%s", me, inStr, err);
    free(err);
    return 1;
  }
  nhist = nrrdNew();
  if (nrrdHisto(nhist, nin, sx, nrrdTypeInt)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble making histogram:\n%s\n", me, err);
    free(err);
    return 1;
  }
  nimg = nrrdNew();
  if (nrrdHistoDraw(nimg, nhist, sy)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble drawing histogram:\n%s\n", me, err);
    free(err);
    return 1;
  }
  io = nrrdIONew();
  if (nrrdSave(outStr, nimg, io)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble writing histogram image to \"%s\":\n%s\n",
	    me, outStr, err);
    free(err);
    return 1;
  }

  nrrdNuke(nin);
  nrrdNuke(nhist);
  nrrdNuke(nimg);
  return 0;
}
