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
  Nrrd *nin, *nout;
  int i, axis, *perm;

  if (3 != argc) {
    printf("gimme something\n");
    exit(1);
  }
  if (!(nin = nrrdNewLoad(argv[1]))) {
    err = biffGet(NRRD);
    printf("nrrdNewLoad failed:\n%s\n", err);
    exit(1);
  }

  axis = 0;
  perm = calloc(nin->size[axis], sizeof(int));
  /*
  for (i=0; i<=nin->size[axis]-1; i++) {
    perm[i] = i;
    printf("perm[%d] = %d\n", i, perm[i]);
  }
  */
  perm[0] = 0;
  perm[1] = 1;
  perm[2] = 4;
  perm[3] = 5;
  perm[4] = 2;
  perm[5] = 6;
  perm[6] = 3;
  
  nout = nrrdNew();
  if (nrrdShuffle(nin, nout, axis, perm)) {
    printf("%s\n", biffGet(NRRD));
  }
  nout->encoding = nrrdEncodingRaw;
  if (nrrdSave(argv[2], nout)) {
    printf("%s\n", biffGet(NRRD));
  }
  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
