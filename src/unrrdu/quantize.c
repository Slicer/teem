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


#include <air.h>
#include <nrrd.h>

char *me;

void
usage() {
  /*                      0     1        2     3     4       5  */
  fprintf(stderr, "usage: %s <nrrdIn> <min> <max> <bits> <nrrdOut>\n", me);
  fprintf(stderr, "     <min> and/or <max> can be given as \"-\" in order\n");
  fprintf(stderr, "     to use the min and/or max value within the data\n");
  exit(1);
}

int
main(int argc, char **argv) {
  FILE *fin, *fout;
  Nrrd *nin, *nout;
  double min, max;
  int bits;
  char *minStr, *maxStr, *bitStr, *ninStr, *noutStr;

  me = argv[0];
  if (6 != argc)
    usage();
  ninStr = argv[1];
  minStr = argv[2];
  maxStr = argv[3];
  bitStr = argv[4];
  noutStr = argv[5];

  min = max = airNand();
  if (!(fin = fopen(ninStr, "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, ninStr);
    exit(1);
  }
  if (strcmp(minStr, "-")) {
    if (1 != sscanf(minStr, "%lg", &min)) {
      fprintf(stderr, "%s: can't parse %s as a double\n", me, minStr);
      exit(1);
    }
  }
  if (strcmp(maxStr, "-")) {
    if (1 != sscanf(maxStr, "%lg", &max)) {
      fprintf(stderr, "%s: can't parse %s as a double\n", me, maxStr);
      exit(1);
    }
  }
  if (1 != sscanf(bitStr, "%d", &bits)) {
    fprintf(stderr, "%s: didn't recognize \"%s\" as an int\n", me, bitStr);
    exit(1);
  }
  printf("reading data ...\n");
  if (!(nin = nrrdNewRead(fin))) {
    fprintf(stderr, "%s: trouble reading nrrd:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  printf("done reading data\n");
  fclose(fin);
  if (!( AIR_EXISTS(min) && AIR_EXISTS(max) )) {
    if (nrrdRange(nin)) {
      fprintf(stderr, "%s: trouble with nrrdRange:\n%s",
	      me, biffGet(NRRD));
      exit(1);
    }
    if (!AIR_EXISTS(min))
      min = nin->min;
    if (!AIR_EXISTS(max))
      max = nin->max;
    printf("%s: using min=%g, max=%g\n", me, min, max);
  }
  if (!(nout = nrrdNewQuantize(nin, min, max, bits))) {
    fprintf(stderr, "%s: couldn't create output nrrd:\n%s", 
	    me, biffGet(NRRD));
    exit(1);
  }

  if (!(fout = fopen(noutStr, "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, noutStr);
    exit(1);
  }
  nout->encoding = nrrdEncodingRaw;
  if (nrrdWrite(fout, nout)) {
    fprintf(stderr, "%s: trouble writing nrrd:\n%s\n",
	    me, biffGet(NRRD));
    exit(1);
  }
  fclose(fout);

  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
