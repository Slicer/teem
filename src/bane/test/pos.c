#include "../bane.h"

char *me;

void
usage() {
  /*                      0     1        2       3          4   (5) */
  fprintf(stderr, "usage: %s <infoIn> <sigma> <gthresh> <posOut>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  FILE *file;
  Nrrd *info, *pos;
  float sigma, gthresh;
  char *iStr, *oStr, *sigStr, *gthStr;

  me = argv[0];
  if (argc != 5) {
    usage();
  }
  iStr = argv[1];
  sigStr = argv[2];
  gthStr = argv[3];
  oStr = argv[4];

  if (1 != sscanf(sigStr, "%g", &sigma) ||
      1 != sscanf(gthStr, "%g", &gthresh)) {
    fprintf(stderr, "%s: couldn't parse %s and %s as floats\n", me, 
	    sigStr, gthStr);
    usage();
  }

  if (!(file = fopen(iStr, "r"))) {
    fprintf(stderr, "%s: couldn't open \"%s\" for reading\n", me, iStr);
    exit(1);
  }
  if (!(info = nrrdNewRead(file))) {
    fprintf(stderr, "%s: trouble reading \"%s\" :\n%s\n", me, 
	    iStr, biffGet(NRRD));
    exit(1);
  }
  fclose(file);
  if (2 == info->dim) {
    if (!(pos = baneNewPosCalc1D(sigma, gthresh, info))) {
      fprintf(stderr, "%s: trouble calculating p(v):\n%s\n", me,
	      biffGet(BANE));
      exit(1);
    }
  }
  else {
    if (!(pos = baneNewPosCalc2D(sigma, gthresh, info))) {
      fprintf(stderr, "%s: trouble calculating p(v,g):\n%s\n", me,
	      biffGet(BANE));
      exit(1);
    }
  }
  if (!(file = fopen(oStr, "w"))) {
    fprintf(stderr, "%s: couldn't open \"%s\" for writing\n", me, oStr);
    exit(1);
  }
  if (nrrdWrite(file, pos)) {
    fprintf(stderr, "%s: trouble writing output to \"%s\"\n", me, oStr);
    exit(1);
  }
  nrrdNuke(info);
  nrrdNuke(pos);
  exit(0);
}

