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
  /*              0    1      2    ...  n+1    n+2   ...  2*n+1  (2*n+2) */
  fprintf(stderr,
	  "usage: %s <nin0> <nin1> ... <bin0> <bin1> ... <nout>\n", me);
  return 1;
}

int
main(int argc, char **argv) {
  char *me, *err;
  Nrrd *nin[NRRD_DIM_MAX], *nout;
  int d, n, bin[NRRD_DIM_MAX], clamp[NRRD_DIM_MAX];
  double min[NRRD_DIM_MAX], max[NRRD_DIM_MAX];

  me = argv[0];
  if (argc < 4 || 0 != argc%2) 
    return usage(me);
  n = (argc-2)/2;
  if (n > NRRD_DIM_MAX) {
    fprintf(stderr, "%s: sorry, can only deal with up to %d nrrds\n", 
	    me, NRRD_DIM_MAX);
    return 1;
  }
  fprintf(stderr, "%s: will try to parse 2*%d args\n", me, n);
  for (d=0; d<=n-1; d++) {
    if (nrrdLoad(nin[d]=nrrdNew(), argv[1+d])) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: trouble reading nrrd %d from \"%s\":\n%s\n", me,
	      d, argv[1+d], err);
      free(err);
      return 1;
    }
    if (1 != sscanf(argv[1+n+d], "%d", bin+d)) {
      fprintf(stderr, "%s: couldn't parse %s as in int\n", me, argv[1+n+d]);
      return 1;
    }
  }

  for (d=0; d<=n-1; d++) {
    if (nrrdSetMinMax(nin[d])) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: trouble determining range in nrrd %d (%s):\n%s\n",
	      me, d, argv[1+d], err);
      free(err);
      return 1;
    }
    min[d] = nin[d]->min;
    max[d] = nin[d]->max;
    fprintf(stderr, "range %d: %f %f\n", d, min[d], max[d]);
    fprintf(stderr, "bin %d: %d\n", d, bin[d]);
    clamp[d] = 0;
  }
  
  fprintf(stderr, "%s: computing multi-histogram ... ", me); fflush(stdout);
  nout = nrrdNew();
  if (nrrdHistoMulti(nout, nin, n, bin, nrrdTypeUShort, clamp)) {
    fprintf(stderr, "%s: trouble doing multi-histogram:\n%s\n", 
	    me, biffGet(NRRD));
    return 1;
  }
  fprintf(stderr, "done\n");
  for (d=0; d<=n-1; d++) {
    nrrdNuke(nin[d]);
  }

  if (nrrdSave(argv[2*n+1], nout, NULL)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble writing output nrrd to \"%s\":\n%s\n", 
	    me, argv[2*n+1], err);
    return 1;
  }
  
  nrrdNuke(nout);
  return 0;
}
