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
  /*                      0    1  (2) */
  fprintf(stderr, "usage: %s <nin>\n", me);
  exit(1);
}

int
main(int argc, char **argv) {
  char *me, *err;
  Nrrd *nrrd;

  me = argv[0];
  if (2 != argc)
    usage(me);

  nrrdStateVerboseIO = 10;
  
  if (nrrdLoad(nrrd=nrrdNew(), argv[1])) {
    fprintf(stderr, "%s: trouble loading \"%s\":\n%s", 
	    me, argv[1], err = biffGet(NRRD));
    free(err);
    exit(1);
  }

  if (nrrdMinMaxClever(nrrd)) {
    fprintf(stderr, "%s: trouble finding min/max \"%s\":\n%s", 
	    me, argv[1], err = biffGet(NRRD));
    free(err);
    exit(1);
  }

  printf("%s: min = %g; max = %g, nrrd->hasNonExist = %d\n", 
	 me, nrrd->min, nrrd->max, nrrd->hasNonExist);

  nrrdNuke(nrrd);

  exit(0);
}
