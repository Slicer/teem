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

int
main(int argc, char **argv) {
  char *err, *infoCP[NRRD_DIM_MAX];
  Nrrd *nin, *nout;
  int axis, *perm, infoI[NRRD_DIM_MAX];
  double infoD[NRRD_DIM_MAX];
  FILE *file;

  if (3 != argc) {
    printf("gimme 2 names\n");
    exit(1);
  }
  if (!(file = fopen(argv[1], "rb"))) {
    printf("couldn't fopen(%s) for reading\n", argv[1]);
    exit(1);
  }
  if (nrrdRead(nin=nrrdNew(), file, NULL)) {
    err = biffGet(NRRD);
    printf("nrrdLoad failed:\n%s\n", err); free(err);
    exit(1);
  }
  /*
  if (nrrdLoad(nin=nrrdNew(), argv[1])) {
    err = biffGet(NRRD);
    printf("nrrdLoad failed:\n%s\n", err); free(err);
    exit(1);
  }
  */

  axis = 0;
  perm = calloc(nin->axis[axis].size, sizeof(int));
  /*
  for (i=0; i<=nin->size[axis]-1; i++) {
    perm[i] = i;
    printf("perm[%d] = %d\n", i, perm[i]);
  }
  */
  perm[0] = 2;
  perm[1] = 1;
  perm[2] = 0;
  /*
  perm[0] = 0;
  perm[1] = 1;
  perm[2] = 4;
  perm[3] = 5;
  perm[4] = 2;
  perm[5] = 6;
  perm[6] = 3;
  */
  nout = nrrdNew();
  if (nrrdShuffle(nout, nin, axis, perm)) {
    fprintf(stderr, "%s\n", biffGet(NRRD));
    exit(1);
  }
  if (nrrdSave(argv[2], nout, NULL)) {
    fprintf(stderr, "%s\n", biffGet(NRRD));
    exit(1);
  }
  nin = nrrdNuke(nin);

  nrrdAxesGet(nout, nrrdAxesInfoSize, 
	      &(infoI[0]), &(infoI[1]), &(infoI[2]));
  fprintf(stderr, "sizes: %d %d %d\n", infoI[0], infoI[1], infoI[2]);
  nrrdAxesGet(nout, nrrdAxesInfoMin, 
	      &(infoD[0]), &(infoD[1]), &(infoD[2]));
  fprintf(stderr, "mins: %g %g %g\n", infoD[0], infoD[1], infoD[2]);
  nrrdAxesGet(nout, nrrdAxesInfoMax, 
	      &(infoD[0]), &(infoD[1]), &(infoD[2]));
  fprintf(stderr, "maxs: %g %g %g\n", infoD[0], infoD[1], infoD[2]);
  nrrdAxesGet(nout, nrrdAxesInfoSpacing, 
	      &(infoD[0]), &(infoD[1]), &(infoD[2]));
  fprintf(stderr, "spacings: %g %g %g\n", infoD[0], infoD[1], infoD[2]);
  nrrdAxesGet(nout, nrrdAxesInfoLabel, 
	      &(infoCP[0]), &(infoCP[1]), &(infoCP[2]));
  nout = nrrdNuke(nout);

  fprintf(stderr, "labels: |%s| |%s| |%s|\n", infoCP[0], infoCP[1], infoCP[2]);

  exit(0);
}
