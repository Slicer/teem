/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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


#include "../air.h"

int
main(int argc, char *argv[]) {
  char *me, *NS;
  int ii, N;

  me = argv[0];
  if (2 != argc) {
    /*                       0  1   (2) */
    fprintf(stderr, "usage: %s <N>\n", me);
    exit(1);
  }
  NS = argv[1];
  if (1 != sscanf(NS, "%d", &N)) {
    fprintf(stderr, "%s: couldn't parse %s as int N\n", me, NS);
    exit(1);
  }

  for (ii=0; ii<N; ii++) {
    printf("%g\n", airDrand48());
  }

  exit(0);
}
