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

int
usage() {
  /*                      0     1        2     3     4       5  */
  fprintf(stderr, "usage: %s <nrrdIn> <min> <max> <bits> <nrrdOut>\n", me);
  fprintf(stderr, "     <min> and/or <max> can be given as \"#\" so as to\n");
  fprintf(stderr, "     to use the min and/or max value within the data\n");
  return 1;
}

int
main(int argc, char **argv) {
  Nrrd *nin, *nout;
  double min, max;
  int bits;
  char *err, *minStr, *maxStr, *bitStr, *ninStr, *noutStr;

  me = argv[0];
  if (6 != argc)
    return usage();
  ninStr = argv[1];
  minStr = argv[2];
  maxStr = argv[3];
  bitStr = argv[4];
  noutStr = argv[5];

  min = max = AIR_NAN;
  if (strcmp(minStr, "#")) {
    if (1 != sscanf(minStr, "%lg", &min)) {
      fprintf(stderr, "%s: can't parse %s as a double\n", me, minStr);
      return 1;
    }
  }
  if (strcmp(maxStr, "#")) {
    if (1 != sscanf(maxStr, "%lg", &max)) {
      fprintf(stderr, "%s: can't parse %s as a double\n", me, maxStr);
      return 1;
    }
  }
  if (1 != sscanf(bitStr, "%d", &bits)) {
    fprintf(stderr, "%s: didn't recognize \"%s\" as an int\n", me, bitStr);
    return 1;
  }
  if (nrrdLoad(nin=nrrdNew(), ninStr)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble reading nrrd from \"%s\":\n%s\n", 
	    me, ninStr, err);
    free(err);
    return 1;
  }
  nin->min = min;
  nin->max = max;
  if (nrrdCleverMinMax(nin)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    free(err);
    return 1;
  }
  if (nrrdQuantize(nout=nrrdNew(), nin, bits)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: couldn't create output nrrd:\n%s", me, err);
    free(err);
    return 1;
  }
  if (nrrdSave(noutStr, nout, NULL)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble writing nrrd to \"%s\":\n%s\n",
	    me, noutStr, err);
    free(err);
    return 1;
  }

  nrrdNuke(nin);
  nrrdNuke(nout);
  return 0;
}
