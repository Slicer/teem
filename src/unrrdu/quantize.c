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
  fprintf(stderr, "     <min> and/or <max> can be given as \"#\" so as to\n");
  fprintf(stderr, "     to use the min and/or max value within the data\n");
  exit(1);
}

int
main(int argc, char **argv) {
  Nrrd *nin, *nout;
  double min, max;
  int bits;
  char *err, *minStr, *maxStr, *bitStr, *ninStr, *noutStr;

  me = argv[0];
  if (6 != argc)
    usage();
  ninStr = argv[1];
  minStr = argv[2];
  maxStr = argv[3];
  bitStr = argv[4];
  noutStr = argv[5];

  min = max = airNand();
  if (strcmp(minStr, "#")) {
    if (1 != sscanf(minStr, "%lg", &min)) {
      fprintf(stderr, "%s: can't parse %s as a double\n", me, minStr);
      exit(1);
    }
  }
  if (strcmp(maxStr, "#")) {
    if (1 != sscanf(maxStr, "%lg", &max)) {
      fprintf(stderr, "%s: can't parse %s as a double\n", me, maxStr);
      exit(1);
    }
  }
  if (1 != sscanf(bitStr, "%d", &bits)) {
    fprintf(stderr, "%s: didn't recognize \"%s\" as an int\n", me, bitStr);
    exit(1);
  }
  if (!(nin = nrrdNewLoad(ninStr))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble reading nrrd from \"%s\":\n%s\n", 
	    me, ninStr, err);
    free(err);
    exit(1);
  }
  if (!( AIR_EXISTS(min) && AIR_EXISTS(max) )) {
    if (nrrdRange(&nin->min, &nin->max, nin)) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: trouble with nrrdRange:\n%s", me, err);
      free(err);
      exit(1);
    }
    if (!AIR_EXISTS(min))
      min = nin->min;
    if (!AIR_EXISTS(max))
      max = nin->max;
    fprintf(stderr, "%s: using min=%g, max=%g\n", me, min, max);
  }
  nout = nrrdNew();
  if (nrrdQuantize(nout, nin, min, max, bits)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: couldn't create output nrrd:\n%s", me, err);
    free(err);
    exit(1);
  }

  nout->encoding = nrrdEncodingRaw;
  if (nrrdSave(noutStr, nout)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble writing nrrd to \"%s\":\n%s\n",
	    me, noutStr, err);
    free(err);
    exit(1);
  }

  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
