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
                      /*  0     1       2      3       4    */
  fprintf(stderr, "usage: %s <nrrdIn> <axis> <pos> <nrrdOut>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  FILE *fin, *fout;
  char *in, *out, *err;
  int axis, pos;
  Nrrd *nin, *nout;

  me = argv[0];
  if (5 != argc)
    usage();
  if (2 != (sscanf(argv[2], "%d", &axis) + 
	    sscanf(argv[3], "%d", &pos))) {
    fprintf(stderr, "%s: couldn't parse (%s,%s) as (axis,pos)\n", 
	    me, argv[2], argv[3]);
    exit(1);
  }
  in = argv[1];
  out = argv[4];
  if (!(fin = fopen(in, "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, in);
    exit(1);
  }
  if (!(nin = nrrdNewRead(fin))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error reading nrrd:\n%s\n", me, err);
    exit(1);
  }
  fclose(fin);
  if (!(nout = nrrdNewSlice(nin, axis, pos))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error slicing nrrd:\n%s\n", me, err);
    exit(1);
  }
  if (!(fout = fopen(out, "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, out);
    exit(1);
  }
  nout->encoding = nin->encoding;
  if (nrrdWrite(fout, nout)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error writing nrrd:\n%s\n", me, err);
    exit(1);
  }
  fclose(fout);
  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
