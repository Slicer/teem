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

