#include "../nrrd.h"

int
main(int argc, char **argv) {
  char *err;
  Nrrd *nrrd, *slice;

  if (3 != argc) {
    printf("gimme somethings\n");
    exit(1);
  }
  if (!(nrrd = nrrdNewOpen(argv[1]))) {
    err = biffGet(NRRD);
    printf("nrrdNewOpen failed:\n%s\n", err);
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
