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
  char *err;
  Nrrd *nrrd, *slice;

  if (3 != argc) {
    printf("gimme somethings\n");
    exit(1);
  }
  if (!(nrrd = nrrdNewLoad(argv[1]))) {
    err = biffGet(NRRD);
    printf("nrrdNewLoad failed:\n%s\n", err);
    exit(1);
  }
  slice = nrrdNew();
  if (nrrdSlice(nrrd, slice, 1, 40)) {
    err = biffGet(NRRD);
    printf("nrrdSlice failed:\n%s\n", err);
    exit(1);
  }
  slice->encoding = nrrdEncodingRaw;
  if (nrrdSave(argv[2], slice)) {
    err = biffGet(NRRD);
    printf("nrrdSave failed:\n%s\n", err);
    exit(1);
  }
  slice = nrrdNuke(slice);
  nrrd = nrrdNuke(nrrd);
  exit(0);
}
