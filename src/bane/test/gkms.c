#include "../bane.h"

char *me;

void
usage() {
  /*                      0    1      2       3    (4) */
  fprintf(stderr, "usage: %s <nin> <perc> <hvolout>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  Nrrd *nin, *hvol;
  FILE *file;
  char *iStr, *oStr, *pStr;
  float perc;
  
  me = argv[0];
  if (4 != argc)
    usage();
  iStr = argv[1];
  pStr = argv[2];
  oStr = argv[3];

  if (1 != sscanf(pStr, "%g", &perc)) {
    fprintf(stderr, "%s: couldn't parse %s as float\n", me, pStr);
    exit(1);
  }
  if (!(file = fopen(iStr, "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, iStr);
    exit(1);
  }
  if (!(nin = nrrdNewRead(file))) {
    fprintf(stderr, "%s: trouble reading input nrrd:\n%s\n", me, 
	    biffGet(NRRD));
    exit(1);
  }
  fclose(file);
  if (!(hvol = baneGKMSHVol(nin, perc))) {
    fprintf(stderr, "%s: trouble creating GKMS histovol:\n%s\n", me,
	    biffGet(BANE));
    exit(1);
  }
  if (!(file = fopen(oStr, "w"))) {
    fprintf(stderr, "%s: trouble opening %s for writing\n", me, oStr);
    exit(1);
  }
  if (nrrdWrite(file, hvol)) {
    fprintf(stderr, "%s: trouble writing histovol to %s\n", me, oStr);
    exit(1);
  }
  nrrdNuke(hvol);
  nrrdNuke(nin);
  exit(0);
}
