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
