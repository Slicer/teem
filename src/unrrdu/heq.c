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
  /*                      0      1       2        3        4  */
  fprintf(stderr, "usage: %s <nrrdIn> <# bins> <smart> <nrrdOut>\n", me);
  return 1;
}

int
main(int argc, char **argv) {
  Nrrd *nrrd, *hist;
  /* Nrrd *pgm; */
  int smart, bins;
  char *me, *err;
  
  me = argv[0];
  if (5 != argc)
    return usage(me);

  if (1 != sscanf(argv[2], "%d", &bins)) {
    fprintf(stderr, "%s: couldn't parse \"%s\" as int\n", me, argv[2]);
    return 1;
  }
  if (1 != sscanf(argv[3], "%d", &smart)) {
    fprintf(stderr, "%s: couldn't parse \"%s\" as int\n", me, argv[3]);
    return 1;
  }
  if (nrrdLoad(nrrd=nrrdNew(), argv[1])) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble reading nrrd from %s:\n%s", 
	    me, argv[1], err);
    free(err);
    return 1;
  }
  if (nrrdHistoEq(nrrd, &hist, bins, smart)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble doing histogram equalization:\n%s", me, err);
    free(err);
    return 1;
  }
  /*
  if (file = fopen("hist0.pgm", "w")) {
    pgm = nrrdNew();
    if (nrrdHistoDraw(pgm, hist, 800)) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: trouble drawing data histogram:\n%s", me, err);
      free(err);
      return 1;
    }
    pgm->encoding = nrrdEncodingRaw;
    if (nrrdWritePNM(file, pgm)) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: trouble writing histogram image:\n%s", me, err);
      free(err);
      return 1;
    }
    fprintf(stderr, "%s: wrote \"BEFORE\" histogram image in hist0.pgm\n", me);
    fclose(file);
    nrrdNuke(pgm);
    nrrdNuke(hist);
  }
  if (file = fopen("hist1.pgm", "w")) {
    hist = nrrdNew();
    if (nrrdHisto(hist, nrrd, bins)) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: trouble generating post-HEQ histogram:\n%s",
	      me, err);
      free(err);
      return 1;
    }
    pgm = nrrdNew();
    if (nrrdHistoDraw(pgm, hist, 800)) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: trouble drawing data histogram:\n%s", me, err);
      free(err);
      return 1;
    }
    pgm->encoding = nrrdEncodingRaw;
    if (nrrdWritePNM(file, pgm)) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: trouble writing histogram image:\n%s", me, err);
      free(err);
      return 1;
    }
    fprintf(stderr, "%s: wrote \"AFTER\" histogram image in hist1.pgm\n", me);
    fclose(file);
    nrrdNuke(pgm);
    nrrdNuke(hist);
  } 
  */   
  
  if (nrrdSave(argv[4], nrrd, NULL)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble writing nrrd:\n%s", me, err);
    free(err);
    return 1;
  }
  nrrdNuke(nrrd);
  return 0;
}
