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
  FILE *file;
  char err[1024], *me;
  Nrrd *nrrd, *slice;
  int i, num, *data;

  me = argv[0];
  num = 7*11*13;
  if (!(nrrd = nrrdNewAlloc(num, nrrdTypeInt, 3))) {
    printf("%s: couldn't NewAlloc the nrrd\n", me);
    exit(1);
  }
  data = (int*)(nrrd->data);
  for (i=0; i<=num-1; i++) {
    
  if (2 != argc) {
    printf("gimme something\n");
    exit(1);
  }
  if (!(file = fopen(argv[1], "r"))) {
    printf("can't open %s for reading\n", argv[1]);
    exit(1);
  }
  if (!(nrrd = nrrdNewRead(file))) {
    nrrdGetErr(err);
    printf("nrrdNewRead failed:\n%s\n", err);
    exit(1);
  }
  fclose(file);
  slice = nrrdNew();
  if (nrrdSlice(nrrd, slice, 1, 40)) {
    nrrdGetErr(err);
    printf("nrrdSlice failed:\n%s\n", err);
    exit(1);
  }
  file = fopen("tmp.nrrd", "w");
  slice->encoding = nrrdEncodingRaw;
  if (nrrdWrite(file, slice)) {
    nrrdGetErr(err);
    printf("nrrdWrite failed:\n%s\n", err);
    exit(1);
  }
  fclose(file);
  slice = nrrdNuke(slice);
  nrrd = nrrdNuke(nrrd);
  exit(0);
}
