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
                      /*  0     1       2          3    */
  fprintf(stderr, "usage: %s <nrrdIn> <axis> <baseNameOut>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  FILE *fin, *fout;
  char *in, *base, out[128], *err, format[10];
  int top, axis, pos;
  Nrrd *nin, *nout = NULL;

  me = argv[0];
  if (4 != argc)
    usage();
  if (1 != sscanf(argv[2], "%d", &axis)) {
    fprintf(stderr, "%s: couldn't parse %s as axis\n", me, argv[2]);
    exit(1);
  }
  in = argv[1];
  base = argv[3];
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
  if (!(NRRD_INSIDE(0, axis, nin->dim-1))) {
    fprintf(stderr, "%s: given axis (%d) outside range [0,%d]\n",
	    me, axis, nin->dim-1);
    exit(1);
  }
  top = nin->size[axis]-1;
  if (top > 9999)
    sprintf(format, "%%s%%05d.nrrd");
  else if (top > 999)
    sprintf(format, "%%s%%04d.nrrd");
  else if (top > 99)
    sprintf(format, "%%s%%03d.nrrd");
  else if (top > 9)
    sprintf(format, "%%s%%02d.nrrd");
  else
    sprintf(format, "%%s%%01d.nrrd");
  for (pos=0; pos<=top; pos++) {
    sprintf(out, format, base, pos);
    printf("%s\n", out);
    if (!(nout = nrrdNewSlice(nin, axis, pos))) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: error slicing nrrd:%s\n", me, err);
      exit(1);
    }
    nout->encoding = nin->encoding;
    if (!(fout = fopen(out, "w"))) {
      fprintf(stderr, "%s: couldn't open %s for writing\n", me, out);
      exit(1);
    }
    if (nrrdWrite(fout, nout)) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: error writing nrrd:%s\n", me, err);
      exit(1);
    }
    fclose(fout);
    nrrdNuke(nout);
  }
  nrrdNuke(nin);
  exit(0);
}
