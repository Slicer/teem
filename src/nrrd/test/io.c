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

void
usage(char *me) { 
  /*                       0     1     2   (3) */
  fprintf(stderr, "usage: %s <nin> <nout>\n", me);
  exit(1);
}

int
main(int argc, char **argv) {
  char *me, *err;
  Nrrd *nrrd;
  NrrdIO *io;

  me = argv[0];
  if (3 != argc)
    usage(me);

  io = nrrdIONew();
  nrrdStateVerboseIO = 10;
  
  if (nrrdLoad(nrrd=nrrdNew(), argv[1])) {
    fprintf(stderr, "%s: trouble loading \"%s\":\n%s", 
	    me, argv[1], err = biffGet(NRRD));
    free(err);
    exit(1);
  }

  if (nrrdSave(argv[2], nrrd, io)) {
    fprintf(stderr, "%s: trouble saving \"%s\":\n%s", 
	    me, argv[1], err = biffGet(NRRD));
    free(err);
    exit(1);
  }
  if (nrrdSave(argv[2], nrrd, io)) {
    fprintf(stderr, "%s: trouble saving \"%s\":\n%s", 
	    me, argv[1], err = biffGet(NRRD));
    free(err);
    exit(1);
  }

  
  nrrdIONix(io);
  nrrdNuke(nrrd);

  exit(0);
}
