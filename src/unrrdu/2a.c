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
  FILE *fin, *fout;
  Nrrd *nin;

  me = argv[0];
  if (3 != argc)
    usage();
  if (!(fin = fopen(argv[1], "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, argv[1]);
    exit(1);
  }
  if (!(fout = fopen(argv[2], "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, argv[2]);
    exit(1);
  }
  if (!(nin = nrrdNewRead(fin))) {
    fprintf(stderr, "%s: trouble reading nrrd:\n%s\n", me, 
	    biffGet(NRRD));
    exit(1);
  }
  nin->encoding = nrrdEncodingAscii;
  if (nrrdWrite(fout, nin)) {
    fprintf(stderr, "%s: trouble writing nrrd:\n%s\n",
	    me, biffGet(NRRD));
    exit(1);
  }
}
