/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "../nrrd.h"

char *me;

void
usage() {
  /*                      0     1     2       (3) */
  fprintf(stderr, "usage: %s <enum> <str>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  char *enmS, *str;
  int enm, val;

  me = argv[0];
  if (3 != argc)
    usage();
  enmS = argv[1];
  str = argv[2];

  if (1 != sscanf(enmS, "%d", &enm)) {
    fprintf(stderr, "%s: couldn't parse %s as int\n", me, enmS);
    usage();
  }
  val = nrrdEnumStrToVal(enm, str);
  printf("%d (%s)\n", val, nrrdEnumValToStr(enm, val));
  
  exit(0);
}
