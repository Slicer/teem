#include "../nrrd.h"

int
main(int argc, char **argv) {
  char *err;
  Nrrd *nin, *nout;
  int i;
  NrrdResampleInfo *info[NRRD_MAX_DIM];

  if (2 != argc) {
    printf("gimme something\n");
    exit(1);
  }
  if (!(nin = nrrdNewOpen(argv[1]))) {
    err = biffGet(NRRD);
    printf("nrrdNewOpen failed:\n%s\n", err);
    exit(1);
  }
  
  for (i=0; i<=nin->dim-1; i++) {
    info[i] = nrrdResampleInfoNew();
    info[i]->samples = nin->size[i]/2;
    printf("info[%d]->samples = %d\n", i, (int)info[i]->samples);
  }
  /*
  info[0] = nrrdResampleInfoNix(info[0]);
  info[2] = nrrdResampleInfoNix(info[2]);
  info[3] = nrrdResampleInfoNix(info[3]);
  */
  nout = nrrdNew();
  if (nrrdSpatialResample(nout, nin, info, 1)) {
    printf("%s\n", biffGet(NRRD));
  }
  nrrdSave("out.nhdr", nout);
  exit(0);
}
