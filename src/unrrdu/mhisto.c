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
  /*              0    1      2    ...  n+1    n+2   ...  2*n+1  (2*n+2) */
  fprintf(stderr,
	  "usage: %s <nin0> <nin1> ... <bin0> <bin1> ... <nout>\n", me);
  exit(1);
}

int
main(int argc, char **argv) {
  char *err;
  Nrrd *nin[NRRD_MAX_DIM], *nout;
  int d, n, bin[NRRD_MAX_DIM], clamp[NRRD_MAX_DIM];
  float min[NRRD_MAX_DIM], max[NRRD_MAX_DIM];

  me = argv[0];
  if (argc < 4 || 0 != argc%2) 
    usage();
  n = (argc-2)/2;
  if (n > NRRD_MAX_DIM) {
    fprintf(stderr, "%s: sorry, can only deal with up to %d nrrds\n", 
	    me, NRRD_MAX_DIM);
    exit(1);
  }
  fprintf(stderr, "%s: will try to parse 2*%d args\n", me, n);
  for (d=0; d<=n-1; d++) {
    if (!(nin[d] = nrrdNewOpen(argv[1+d]))) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: trouble reading nrrd %d from \"%s\":\n%s\n", me,
	      d, argv[1+d], err);
      free(err);
      exit(1);
    }
    if (1 != sscanf(argv[1+n+d], "%d", bin+d)) {
      fprintf(stderr, "%s: couldn't parse %s as in int\n", me, argv[1+n+d]);
      exit(1);
    }
  }

  for (d=0; d<=n-1; d++) {
    if (nrrdRange(&nin[d]->min, &nin[d]->max, nin[d])) {
      fprintf(stderr, "%s: trouble determining range in nrrd %d (%s):\n%s\n",
	      me, d, argv[1+d], biffGet(NRRD));
    }
    min[d] = nin[d]->min;
    max[d] = nin[d]->max;
    fprintf(stderr, "range %d: %f %f\n", d, min[d], max[d]);
    fprintf(stderr, "bin %d: %d\n", d, bin[d]);
    clamp[d] = 0;
  }
  
  fprintf(stderr, "%s: computing multi-histogram ... ", me); fflush(stdout);
  if (!(nout = nrrdNewMultiHisto(nin, n, bin, min, max, clamp))) {
    fprintf(stderr, "%s: trouble doing multi-histogram:\n%s\n", 
	    me, biffGet(NRRD));
    exit(1);
  }
  fprintf(stderr, "done\n");
  for (d=0; d<=n-1; d++) {
    nrrdNuke(nin[d]);
  }

  if (nrrdSave(argv[2*n+1], nout)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble writing output nrrd to \"%s\":\n%s\n", 
	    me, argv[2*n+1], err);
    exit(1);
  }
  
  nrrdNuke(nout);
  exit(0);
}
