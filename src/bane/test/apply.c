#include "../bane.h"

char *me;

void
usage() {
  /*                      0     1     2       3   (4) */
  fprintf(stderr, "usage: %s <nin> <measr> <nout>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  FILE *file;
  int measr;
  char *iStr, *mStr, *oStr;
  Nrrd *nin, *nout;
  
  me = argv[0];
  if (4 != argc)
    usage();

  iStr = argv[1];
  mStr = argv[2];
  oStr = argv[3];
  
  if (!(file = fopen(iStr, "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, iStr);
    usage();
  }
  if (!(nin = nrrdNewRead(file))) {
    fprintf(stderr, "%s: trouble reading input nrrd:\n%s\n", me, 
	    biffGet(NRRD));
    usage();
  }
  fclose(file);
  if (1 != sscanf(mStr, "%d", &measr)) {
    fprintf(stderr, "%s: couldn't parse %s as int\n", me, mStr);
    usage();
  }

  if (!(nout = baneNewApplyMeasr(nin, measr))) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(BANE));
    exit(1);
  }
  if (!(file = fopen(oStr, "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, oStr);
    exit(1);
  }
  if (nrrdWrite(file, nout)) {
    fprintf(stderr, "%s: trouble writing output nrrd:\n%s\n", me,
	    biffGet(NRRD));
    usage();
  }
  exit(0);
}
