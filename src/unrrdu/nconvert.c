#include <nrrd.h>

char *me;

void
usage() {
  /*   f                     0       1        2       3    */
  fprintf(stderr, "usage: convert <nrrdIn> <type> <nrrdOut>\n");
  exit(1);
}

int
main(int argc, char **argv) {
  FILE *fin, *fout;
  Nrrd *nin, *nout;
  int type;

  me = argv[0];
  if (4 != argc)
    usage();
  if (!(fin = fopen(argv[1], "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, argv[1]);
    exit(1);
  }
  if (nrrdTypeUnknown == (type = nrrdStr2Type(argv[2]))) {
    fprintf(stderr, "%s: didn't recognize \"%s\" as a type\n", me, argv[2]);
    exit(1);
  }
  if (!(nin = nrrdNewRead(fin))) {
    fprintf(stderr, "%s: trouble reading nrrd:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  fclose(fin);
  if (!(nout = nrrdNewConvert(nin, type))) {
    fprintf(stderr, "%s: couldn't create output nrrd:\n%s", 
	    me, biffGet(NRRD));
    exit(1);
  }

  if (!(fout = fopen(argv[3], "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, argv[2]);
    exit(1);
  }
  nout->encoding = nrrdEncodingRaw;
  if (nrrdWrite(fout, nout)) {
    fprintf(stderr, "%s: trouble writing nrrd:\n%s\n",
	    me, biffGet(NRRD));
    exit(1);
  }
  fclose(fout);

  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
