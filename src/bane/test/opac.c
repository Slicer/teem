#include "../bane.h"

char *me;
void
usage() {
  /*                      0    1     2       3   (4) */
  fprintf(stderr, "usage: %s <bIn> <pIn> <opacOut>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  char *bStr, *pStr, *oStr;
  FILE *file;
  Nrrd *b, *p, *o;

  me = argv[0];
  if (argc != 4) 
    usage();
  bStr = argv[1];
  pStr = argv[2];
  oStr = argv[3];
  if (!(file = fopen(bStr, "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, bStr);
    usage();
  }
  if (!(b = nrrdNewRead(file))) {
    fprintf(stderr, "%s: trouble reading %s:\n%s\n", me, bStr, biffGet(NRRD));
    usage();
  }
  fclose(file);

  if (!(file = fopen(pStr, "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, pStr);
    usage();
  }
  if (!(p = nrrdNewRead(file))) {
    fprintf(stderr, "%s: trouble reading %s:\n%s\n", me, pStr, biffGet(NRRD));
    usage();
  }
  fclose(file);

  if (1 == p->dim) {
    if (!(o = baneNewOpacCalc1Dcpts(b, p))) {
      fprintf(stderr, "%s: trouble calculating opac:\n%s", me, biffGet(BANE));
      exit(1);
    }
  }
  else {
    if (!(o = baneNewOpacCalc2Dcpts(b, p))) {
      fprintf(stderr, "%s: trouble calculating opac:\n%s", me, biffGet(BANE));
      exit(1);
    }
  }
  if (!(file = fopen(oStr, "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, oStr);
    usage();
  }
  if (nrrdWrite(file, o)) {
    fprintf(stderr, "%s: trouble writing %s:\n%s\n", me, oStr, biffGet(NRRD));
    exit(1);
  }
  fclose(file);

  nrrdNuke(o);
  nrrdNuke(b);
  nrrdNuke(p);
  exit(0);
}
