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

int
usage() {
                      /*  0     1       2       3    (4) */
  fprintf(stderr, "usage: %s <nrrdIn> <axis> <nrrdOut>\n", me);
  return 1;
}

int
main(int argc, char *argv[]) {
  char *in, *out, *err;
  int axis;
  Nrrd *nin, *nout;

  me = argv[0];
  if (4 != argc)
    return usage();

  if (1 != sscanf(argv[2], "%d", &axis)) {
    fprintf(stderr, "%s: couldn't parse %s as axis\n", me, argv[2]);
    return 1;
  }
  in = argv[1];
  out = argv[3];

  if (nrrdLoad(nin=nrrdNew(), in)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: couldn't get nrrd from %s:\n%s\n", me, in, err);
    free(err);
    return 1;
  }
  nout = nrrdNew();
  if (nrrdFlip(nout, nin, axis)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error flipping nrrd:\n%s\n", me, err);
    free(err);
    return 1;
  }
  if (nrrdSave(out, nout, NULL)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error saving nrrd to %s:\n%s\n", me, out, err);
    free(err);
    return 1;
  }
    
  nrrdNuke(nin);
  nrrdNuke(nout);
  return 0;
}
