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

void
main() {
  Nrrd *nrrd;
  void *data;
  int i, limit;

  NrrdInfoWrite = NULL;
  NrrdInfoParse = NULL;
  NrrdInfoFree = NrrdSimpleInfoFree;
  NrrdInfoNew = NrrdSimpleInfoNew;

  limit = 10;
  /* limit = 10000; */
  for (i=1; i<=limit; i++) {
    nrrd = nrrdNew();
    data = malloc(1024*1024);
    nrrdWrap(nrrd, data, 1024*1024, nrrdTypeUChar, 1);
    nrrdNuke(nrrd);
  }
  for (i=1; i<=limit; i++) {
    nrrd = nrrdNew();
    nrrdAlloc(nrrd, 1024*1024, nrrdTypeUChar, 1);
    nrrdEmpty(nrrd);
    nrrdNix(nrrd);
  }
  for (i=1; i<=limit; i++) {
    nrrd = nrrdNewAlloc(1024*1024, nrrdTypeUChar, 1);
    nrrdNuke(nrrd);
  }

}
