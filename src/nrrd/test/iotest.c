#include "../nrrd.h"

int
main(int argc, char **argv) {
  FILE *file;
  char *err;
  Nrrd *nrrd;

  if (2 != argc) {
    printf("gimme something\n");
    exit(1);
  }
  if (!(file = fopen(argv[1], "r"))) {
    printf("can't open %s for reading\n", argv[1]);
    exit(1);
  }
  if (!(nrrd = nrrdNewRead(file))) {
    err = biffGet(NRRD);
    printf("nrrdNewRead failed:\n%s\n", err);
    exit(1);
  }
  fclose(file);
  nrrdDescribe(stdout, nrrd);
  file = fopen("out.nrrd", "w");
  if (nrrdWrite(file, nrrd)) {
    err = biffGet(NRRD);
    printf("nrrdWrite failed:\n%s\n", err);
    exit(1);
  }
  fclose(file);
  nrrdNuke(nrrd);
  exit(0);
}
