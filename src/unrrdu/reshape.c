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
#include <limits.h>

char *me; 

int
usage() {
  /*              0    1      2      3        4        argc-1 */ 
  fprintf(stderr, 
	  "usage: %s <nIn> <size0> <size1> <size2> ... <nOut>\n",
	  me);
  return 1;
}

int
main(int argc, char *argv[]) {
  char *inStr, *outStr, *err;
  int d, size[NRRD_DIM_MAX], dim;
  Nrrd *nin, *nout;
  
  me = argv[0];
  if (!( argc >= 4 ))
    return usage();
  inStr = argv[1];
  outStr = argv[argc-1];

  dim = argc - 3;
  for (d=0; d<=dim-1; d++) {
    if (1 != sscanf(argv[d+2], "%d", &(size[d]))) {
      fprintf(stderr, "%s: can't parse \"%s\" as int\n", me, argv[d+2]);
      return usage();
    }
  }

  if (nrrdLoad(nin=nrrdNew(), inStr)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble reading input:%s\n", me, err);
    free(err);
    return 1;
  }

  if (nrrdReshape_nva(nout=nrrdNew(), nin, dim, size)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error reshaping nrrd:\n%s", me, err);
    free(err);
    return 1;
  }

  if (nrrdSave(outStr, nout, NULL)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error writing nrrd:\n%s", me, err);
    free(err);
    return 1;
  }

  nrrdNuke(nin);
  nrrdNuke(nout);
  return 0;
}
