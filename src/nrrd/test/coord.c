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

#include <stdio.h>
#include <stdlib.h>

#define COORD_UPDATE(coord, size, dim, d)          \
for (d=0; d < dim-1 && coord[d] == size[d]; d++) { \
  coord[d] = 0;                                    \
  coord[d+1]++;                                    \
}

#define COORD_INDEX(coord, size, dim, d, I)        \
for (d=dim-1, I=coord[d--]; d>=0; d--) {           \
  I = coord[d] + size[d]*I;                        \
}

void
increment(int *coord, int *size, int D) {
  int d;
  
  coord[0]++;
  COORD_UPDATE(coord, size, D, d);
}

void
main() {
  int i, I, D, N, d, *size, *coord;
  
  D = 4;
  size = calloc(D, sizeof(int));
  coord = calloc(D, sizeof(int));

  N = 1;
  for (d=0; d<=D-1; d++) {
    size[d] = 2 + d/2;
    N *= size[d];
  }

  printf("size = ");
  for (d=0; d<=D-1; d++)
    printf("% 3d ", size[d]);
  printf("; N = %d\n", N);

  for (i=0; i<=N-1; i++) {
    printf("% 3d: ", i);
    for (d=0; d<=D-1; d++)
      printf("% 3d ", coord[d]);
    COORD_INDEX(coord, size, D, d, I);
    printf(" --> %d\n", I);
    increment(coord, size, D);
  }

  free(size);
  free(coord);
}

