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
  /*                      0      1       2        3        4  */
  fprintf(stderr, "usage: %s <nrrdIn> <# bins> <smart> <nrrdOut>\n", me);
  exit(1);
}

int
main(int argc, char **argv) {
  FILE *file;
  Nrrd *nrrd, *hist, *pgm;
  int smart, bins;
  int (*writer)(FILE *, Nrrd *);
  
  me = argv[0];

  if (5 != argc)
    usage();

  if (!(file = fopen(argv[1], "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, argv[1]);
    exit(1);
  }
  if (1 != sscanf(argv[2], "%d", &bins)) {
    fprintf(stderr, "%s: couldn't parse \"%s\" as int\n", me, argv[2]);
    exit(1);
  }
  if (1 != sscanf(argv[3], "%d", &smart)) {
    fprintf(stderr, "%s: couldn't parse \"%s\" as int\n", me, argv[3]);
    exit(1);
  }
  if (!(nrrd = nrrdNewRead(file))) {
    fprintf(stderr, "%s: trouble reading nrrd from %s:\n%s", 
	    me, argv[1], biffGet(NRRD));
    exit(1);
  }
  fclose(file);
  if (nrrdHistoEq(nrrd, &hist, bins, smart)) {
    fprintf(stderr, "%s: trouble doing histogram equalization:\n%s",
	    me, biffGet(NRRD));
    exit(1);
  }
  if (file = fopen("hist0.pgm", "w")) {
    if (!(pgm = nrrdNewDrawHisto(hist, 800))) {
      fprintf(stderr, "%s: trouble drawing data histogram:\n%s",
	      me, biffGet(NRRD));
      exit(1);
    }
    pgm->encoding = nrrdEncodingRaw;
    if (nrrdWritePNM(file, pgm)) {
      fprintf(stderr, "%s: trouble writing histogram image:\n%s",
	      me, biffGet(NRRD));
      exit(1);
    }
    printf("%s: wrote \"BEFORE\" histogram image in hist0.pgm\n", me);
    fclose(file);
    nrrdNuke(pgm);
    nrrdNuke(hist);
  }
  if (file = fopen("hist1.pgm", "w")) {
    if (!(hist = nrrdNewHisto(nrrd, bins))) {
      fprintf(stderr, "%s: trouble generating post-HEQ histogram:\n%s",
	      me, biffGet(NRRD));
      exit(1);
    }
    if (!(pgm = nrrdNewDrawHisto(hist, 800))) {
      fprintf(stderr, "%s: trouble drawing data histogram:\n%s",
	      me, biffGet(NRRD));
      exit(1);
    }
    pgm->encoding = nrrdEncodingRaw;
    if (nrrdWritePNM(file, pgm)) {
      fprintf(stderr, "%s: trouble writing histogram image:\n%s",
	      me, biffGet(NRRD));
      exit(1);
    }
    printf("%s: wrote \"AFTER\" histogram image in hist1.pgm\n", me);
    fclose(file);
    nrrdNuke(pgm);
    nrrdNuke(hist);
  }    
    
  if (!(file = fopen(argv[4], "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, argv[4]);
    exit(1);
  }
  writer = (strstr(argv[4], ".pgm")
	    ? nrrdWritePNM
	    : nrrdWrite);
  if (writer(file, nrrd)) {
    fprintf(stderr, "%s: trouble in writer:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  fclose(file);
  nrrdNuke(nrrd);
  exit(0);
}
