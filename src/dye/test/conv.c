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


#include <dye.h>

char *me;

void
usage() {
  /*                      0     1        2       (3) */
  fprintf(stderr, "usage: %s <colIn> <spaceOut> \n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  char *inS, *spcS, buff[512];
  dyeColor *col;
  int spc;
  
  me = argv[0];
  if (3 != argc)
    usage();
  inS = argv[1];
  spcS = argv[2];
  
  if (dyeColorParse(col = dyeColorNew(), inS)) {
    fprintf(stderr, "%s: trouble parsing \"%s\":\n%s", me, inS, biffGet(DYE));
    exit(1);
  }
  spc = dyeStrToSpace(spcS);
  if (dyeSpaceUnknown == spc) {
    fprintf(stderr, "%s: couldn't parse \"%s\" as colorspace\n", me, spcS);
    exit(1);
  }
  if (dyeConvert(col, spc)) {
    fprintf(stderr, "%s: trouble converting to %s:\n%s", 
	    me, spcS, biffGet(DYE));
    exit(1);
  }
  printf("%s\n", dyeColorSprintf(buff, col));

  col = dyeColorNix(col);
  exit(0);
}
