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

char *me;

void
usage() {
  /*                      0     1     2       (3) */
  fprintf(stderr, "usage: %s <enum> <val>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  char *enmS, *valS, *out;
  int enm, val;

  me = argv[0];
  if (3 != argc)
    usage();
  enmS = argv[1];
  valS = argv[2];

  if (2 != sscanf(enmS, "%d", &enm) + sscanf(valS, "%d", &val)) {
    fprintf(stderr, "%s: couldn't parse %s and %s as ints\n", me, enmS, valS);
    usage();
  }
  out = nrrdEnumValToStr(enm, val);
  printf("%s\n", out);
  
  exit(0);
}
