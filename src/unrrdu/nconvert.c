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
  /*                         0       1        2       3    (4) */
  fprintf(stderr, "usage: convert <nrrdIn> <type> <nrrdOut>\n");
  exit(1);
}

int
main(int argc, char **argv) {
  Nrrd *nin, *nout;
  int type;
  char *in, *out, *typeStr, *err;

  me = argv[0];
  if (4 != argc)
    usage();
  
  in = argv[1];
  typeStr = argv[2];
  out = argv[3];
  if (nrrdTypeUnknown == (type = nrrdStr2Type(typeStr))) {
    fprintf(stderr, "%s: didn't recognize \"%s\" as a type\n", me, typeStr);
    exit(1);
  }

  if (!(nin = nrrdNewOpen(in))) {
    err = biffGet(NRRD);
    fprintf(stderr, 
	    "%s: trouble reading nrrd from \"%s\":\n%s\n", me, in, err);
    free(err);
    exit(1);
  }
  if (!(nout = nrrdNewConvert(nin, type))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: couldn't create output nrrd:\n%s", me, err);
    free(err);
    exit(1);
  }

  nout->encoding = nrrdEncodingRaw;
  if (nrrdSave(out, nout)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble writing nrrd to \"%s\":\n%s\n", me, out, err);
    free(err);
    exit(1);
  }

  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
