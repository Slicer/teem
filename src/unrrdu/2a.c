#include <nrrd.h>

char *me;

void
usage() {
  /*             0      1        2     */
  fprintf(stderr,
	  "usage: 2a <nrrdIn> <nrrdOut>\n");
  exit(1);
}

int
main(int argc, char **argv) {
  FILE *fin, *fout;
  Nrrd *nin;

  me = argv[0];
  if (3 != argc)
    usage();
  if (!(fin = fopen(argv[1], "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, argv[1]);
    exit(1);
  }
  if (!(fout = fopen(argv[2], "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, argv[2]);
    exit(1);
  }
  if (!(nin = nrrdNewRead(fin))) {
    fprintf(stderr, "%s: trouble reading nrrd:\n%s\n", me, 
	    biffGet(NRRD));
    exit(1);
  }
  nin->encoding = nrrdEncodingAscii;
  if (nrrdWrite(fout, nin)) {
    fprintf(stderr, "%s: trouble writing nrrd:\n%s\n",
	    me, biffGet(NRRD));
    exit(1);
  }
}
