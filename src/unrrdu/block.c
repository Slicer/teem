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

int
usage(char *me) {
  /*              0    1     2   (3) */
  fprintf(stderr, 
	  "usage: %s <nIn> <nOut>\n",
	  me);
  return 1;
}

int
main(int argc, char *argv[]) {
  char *me, *inStr, *outStr, *err;
  Nrrd *nin, *nout;

  me = argv[0];
  if (3 != argc) {
    return usage(me);
  }
  inStr = argv[1];
  outStr = argv[2];
  if (nrrdLoad(nin=nrrdNew(), inStr)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble reading input:%s\n", me, err);
    free(err);
    return 1;
  }
  if (nrrdBlock(nout = nrrdNew(), nin)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error blockifying nrrd:\n%s", me, err);
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
