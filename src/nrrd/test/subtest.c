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
  char *err;
  Nrrd *nrrd, *slice;

  if (3 != argc) {
    printf("gimme somethings\n");
    exit(1);
  }
  if (!(nrrd = nrrdNewLoad(argv[1]))) {
    err = biffGet(NRRD);
    printf("nrrdNewLoad failed:\n%s\n", err);
    exit(1);
  }
  slice = nrrdNew();
  if (nrrdSlice(nrrd, slice, 1, 40)) {
    err = biffGet(NRRD);
    printf("nrrdSlice failed:\n%s\n", err);
    exit(1);
  }
  slice->encoding = nrrdEncodingRaw;
  if (nrrdSave(argv[2], slice)) {
    err = biffGet(NRRD);
    printf("nrrdSave failed:\n%s\n", err);
    exit(1);
  }
  slice = nrrdNuke(slice);
  nrrd = nrrdNuke(nrrd);
  exit(0);
}
