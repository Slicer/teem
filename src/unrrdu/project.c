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
                      /*  0     1       2       3       4    (5) */
  fprintf(stderr, "usage: %s <nrrdIn> <axis> <measr> <nrrdOut>\n", me);
  return 1;
}

int
main(int argc, char *argv[]) {
  char *in, *out, *err;
  int axis, measr;
  Nrrd *nin, *nout;
  double t1, t2;

  me = argv[0];
  if (5 != argc)
    return usage();

  if (2 != (sscanf(argv[2], "%d", &axis) + 
	    sscanf(argv[3], "%d", &measr))) {
    fprintf(stderr, "%s: couldn't parse (%s,%s) as (axis,measr)\n", 
	    me, argv[2], argv[3]);
    return 1;
  }
  in = argv[1];
  out = argv[4];

  if (nrrdLoad(nin=nrrdNew(), in)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error reading nrrd from \"%s\":\n%s\n", me, in, err);
    free(err);
    return 1;
  }
  t1 = airTime();
  if (nrrdProject(nout = nrrdNew(), nin, axis, measr)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error projecting nrrd:\n%s\n", me, err);
    free(err);
    return 1;
  }
  t2 = airTime();
  printf("%s: projection took %g seconds\n", me, t2-t1);
  if (nrrdSave(out, nout, NULL)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error writing nrrd:\n%s\n", me, err);
    free(err);
    return 1;
  }

  nrrdNuke(nin);
  nrrdNuke(nout);
  return 0;
}
