#include <nrrd.h>

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
