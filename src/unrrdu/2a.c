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
  /*             0      1        2     */
  fprintf(stderr,
	  "usage: 2a <nrrdIn> <nrrdOut>\n");
  exit(1);
}

int
main(int argc, char **argv) {
  Nrrd *nin;
  char *err;

  me = argv[0];
  if (3 != argc)
    usage();
  if (!(nin = nrrdNewOpen(argv[1]))) {
    err = biffGet(NRRD);
    fprintf(stderr, 
	    "%s: trouble reading nrrd from \"%s\":\n%s\n", me, argv[1], err);
    free(err);
    exit(1);
  }
  nin->encoding = nrrdEncodingAscii;
  if (nrrdSave(argv[2], nin)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble writing nrrd to \"%s\":\n%s\n",
	    me, argv[2], err);
    free(err);
    exit(1);
  }
  
  nrrdNuke(nin);
  exit(0);
}
