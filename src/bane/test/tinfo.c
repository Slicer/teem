#include "../bane.h"

char *me;

void
usage() {
  /*                      0     1       2     3    (4)  */
  fprintf(stderr, "usage: %s <hvolin> <dim> <nout>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  FILE *file;
  Nrrd *hvol, *info;
  char *iStr, *dStr, *oStr;
  int dim;

  me = argv[0];
  if (argc != 4)
    usage();
  iStr = argv[1];
  dStr = argv[2];
  oStr = argv[3];

  if (!(file = fopen(iStr, "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, iStr);
    usage();
  }
  if (!(hvol = nrrdNewRead(file))) {
    fprintf(stderr, "%s: trouble reading hvol:\n%s\n", me, biffGet(NRRD));
    usage();
  }
  fclose(file);
  if (1 != sscanf(dStr, "%d", &dim)) {
    fprintf(stderr, "%s: trouble parsing %s as an int\n", me, dStr);
    usage();
  }
  if (dim != 1 && dim != 2) {
    fprintf(stderr, "%s need dim to be 1 or 2\n", me);
    exit(1);
  }
  if (dim == 1) {
    if (!(info = baneNew1DOpacInfo(hvol))) {
      fprintf(stderr, "%s: trouble calculting 1D opacity info:\n%s\n", me,
	      biffGet(BANE));
      exit(1);
    }
  }
  else {
    if (!(info = baneNew2DOpacInfo(hvol))) {
      fprintf(stderr, "%s: trouble calculting 2D opacity info:\n%s\n", me,
	      biffGet(BANE));
      exit(1);
    }
  }
  if (!(file = fopen(oStr, "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, oStr);
    exit(1);
  }
  if (nrrdWrite(file, info)) {
    fprintf(stderr, "%s: trouble writing nrrd to %s:\n%s\n", me, oStr,
	    biffGet(NRRD));
    exit(1);
  }
  fclose(file);
  nrrdNuke(hvol);
  nrrdNuke(info);
  exit(0);
}
