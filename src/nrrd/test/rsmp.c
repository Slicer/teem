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
  int i;
  NrrdResampleInfo *info[NRRD_MAX_DIM];

  if (2 != argc) {
    printf("gimme something\n");
    exit(1);
  }
  if (!(nin = nrrdNewOpen(argv[1]))) {
    err = biffGet(NRRD);
    printf("nrrdNewOpen failed:\n%s\n", err);
    exit(1);
  }
  
  for (i=0; i<=nin->dim-1; i++) {
    info[i] = nrrdResampleInfoNew();
    info[i]->samples = nin->size[i]/2;
    printf("info[%d]->samples = %d\n", i, (int)info[i]->samples);
  }
  /*
  info[0] = nrrdResampleInfoNix(info[0]);
  info[2] = nrrdResampleInfoNix(info[2]);
  info[3] = nrrdResampleInfoNix(info[3]);
  */
  nout = nrrdNew();
  if (nrrdSpatialResample(nout, nin, info, 1)) {
    printf("%s\n", biffGet(NRRD));
  }
  nrrdSave("out.nhdr", nout);
  exit(0);
}
