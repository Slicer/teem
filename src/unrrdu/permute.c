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
                      
  fprintf(stderr, /* 0  1        2        3           argc-2     argc-1*/ 
	  "usage: %s <nrrdIn> <axis_0> <axis_1> ... <axis_n-1> <nrrdOut>\n",
	  me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  FILE *fin, *fout;
  char *in, *out, *err;
  int i, ax, udim, axis[NRRD_MAX_DIM], used[NRRD_MAX_DIM];
  Nrrd *nin, *nout;

  me = argv[0];
  udim = argc - 3;
  if (!(udim > 1)) {
    usage();
  }
  in = argv[1];
  out = argv[argc-1];
  memset(used, 0, NRRD_MAX_DIM*sizeof(int));
  for (i=0; i<=udim-1; i++) {
    if (1 != sscanf(argv[i+2], "%d", &ax)) {
      fprintf(stderr, "%s: couldn't parse axis \"%s\" as int\n",
	      me, argv[i+2]);
      exit(1);
    }
    axis[i] = ax;
  }
  if (!(fin = fopen(in, "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, in);
    exit(1);
  }
  if (!(nin = nrrdNewRead(fin))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error reading nrrd:%s\n", me, err);
    exit(1);
  }
  fclose(fin);
  if (udim != nin->dim) {
    fprintf(stderr, "%s: input nrrd has dimension %d, but given %d axes\n",
	    me, nin->dim, udim);
    exit(1);
  }
  if (!(nout = nrrdNewPermuteAxes(nin, axis))) {
    fprintf(stderr, "%s: error permuting nrrd:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  if (!(fout = fopen(out, "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, out);
    exit(1);
  }
  nout->encoding = nin->encoding;
  if (nrrdWrite(fout, nout)) {
    fprintf(stderr, "%s: error writing nrrd:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  fclose(fout);
  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
