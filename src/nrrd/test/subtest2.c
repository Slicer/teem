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


#include "../nrrd.h"

int
main(int argc, char **argv) {
  FILE *file;
  char err[1024], *me;
  Nrrd *nrrd, *slice;
  int i, num, *data;

  me = argv[0];
  num = 7*11*13;
  if (!(nrrd = nrrdNewAlloc(num, nrrdTypeInt, 3))) {
    printf("%s: couldn't NewAlloc the nrrd\n", me);
    exit(1);
  }
  data = (int*)(nrrd->data);
  for (i=0; i<=num-1; i++) {
    
  if (2 != argc) {
    printf("gimme something\n");
    exit(1);
  }
  if (!(file = fopen(argv[1], "r"))) {
    printf("can't open %s for reading\n", argv[1]);
    exit(1);
  }
  if (!(nrrd = nrrdNewRead(file))) {
    nrrdGetErr(err);
    printf("nrrdNewRead failed:\n%s\n", err);
    exit(1);
  }
  fclose(file);
  slice = nrrdNew();
  if (nrrdSlice(nrrd, slice, 1, 40)) {
    nrrdGetErr(err);
    printf("nrrdSlice failed:\n%s\n", err);
    exit(1);
  }
  file = fopen("tmp.nrrd", "w");
  slice->encoding = nrrdEncodingRaw;
  if (nrrdWrite(file, slice)) {
    nrrdGetErr(err);
    printf("nrrdWrite failed:\n%s\n", err);
    exit(1);
  }
  fclose(file);
  slice = nrrdNuke(slice);
  nrrd = nrrdNuke(nrrd);
  exit(0);
}
