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


#include <stdio.h>
#include "../ell.h"

char *me;

void
usage() {
  /*                      0   1   2   3   (4) */
  fprintf(stderr, "usage: %s <a> <b> <c>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  char *aS, *bS, *cS;
  float a, b, c, t;

  me = argv[0];
  if (4 != argc) {
    usage();
  }

  aS = argv[1];
  bS = argv[2];
  cS = argv[3];
  if (3 != (sscanf(aS, "%g", &a) +
	    sscanf(bS, "%g", &b) +
	    sscanf(cS, "%g", &c))) {
    fprintf(stderr, "%s: couldn't parse \"%s\", \"%s\", \"%s\" as floats\n",
	    me, aS, bS, cS);
    usage();
  }
  
  printf("%g %g %g --> ", a, b, c);
  ELL_SORT3(a, b, c, t);
  printf("%g %g %g\n", a, b, c);

  exit(0);
}
