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
main() {
  Nrrd *nrrd, *slice;
  unsigned char *data;
  FILE *file;
  int i;
  char err[NRRD_ERR_STRLEN];

  /*
  nrrd = nrrdNewAlloc(6*6*6, nrrdTypeUChar, 3);
  data = nrrd->data;
  for(i=0; i<=6*6*6-1; i++) {
    data[i] = i;
  }
  file = fopen("wee.nrrd", "w");
  nrrd->encoding = nrrdEncodingRaw;
  nrrd->size[0] = 6;
  nrrd->size[1] = 6;
  nrrd->size[2] = 6;
  if (nrrdWrite(file, nrrd)) {
    nrrdGetErr(err);
    printf("oh dear.\n%s", err);
    exit(1);
  }
  nrrdNuke(nrrd);
  */

  file = fopen("wee.nrrd", "r");
  nrrd = nrrdNewRead(file);
  fclose(file);
  printf("calling slice\n");
  if (!(slice = nrrdNewSlice(nrrd, 2, 5))) {
    nrrdGetErr(err);
    printf("oh dear.\n%s", err);
    exit(1);
  }  
  printf("slice done\n");
  slice->encoding = nrrdEncodingRaw;
  file = fopen("slice.nrrd", "w");
  if (nrrdWrite(file, slice)) {
    nrrdGetErr(err);
    printf("oh dear.\n%s", err);
    exit(1);
  }
  nrrdNuke(slice);
  nrrdNuke(nrrd);
  fclose(file);
}
