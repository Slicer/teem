#include <nrrd.h>

char *me;

void
usage() {
  /*              0    1      2    ...  n+1    n+2   ...  2*n+1  (2*n+2) */
  fprintf(stderr,
	  "usage: %s <nin0> <nin1> ... <bin0> <bin1> ... <nout>\n", me);
  exit(1);
}

int
main(int argc, char **argv) {
  FILE *fin, *fout;
  Nrrd *nin[NRRD_MAX_DIM], *nout;
  int d, n, bin[NRRD_MAX_DIM], clamp[NRRD_MAX_DIM];
  float min[NRRD_MAX_DIM], max[NRRD_MAX_DIM];

  me = argv[0];
  printf("argc = %d\n", argc);
  if (argc < 4 || 0 != argc%2) 
    usage();
  n = (argc-2)/2;
  if (n > NRRD_MAX_DIM) {
    fprintf(stderr, "%s: sorry, can only deal with up to %d nrrds\n", 
	    me, NRRD_MAX_DIM);
    exit(1);
  }
  printf("%s: will try to parse 2*%d args\n", me, n);
  for (d=0; d<=n-1; d++) {
    if (!(fin = fopen(argv[1+d], "r"))) {
      fprintf(stderr, "%s: couldn't open file %d \"%s\" for reading\n", 
	      me, d, argv[1+d]);
      exit(1);
    }
    if (!(nin[d] = nrrdNewRead(fin))) {
      fprintf(stderr, "%s: trouble reading nrrd %d \"%s\":\n%s\n", me,
	      d, argv[1+d], biffGet(NRRD));
    }
    fclose(fin);
    if (1 != sscanf(argv[1+n+d], "%d", bin+d)) {
      fprintf(stderr, "%s: couldn't parse %s as in int\n", me, argv[1+n+d]);
      exit(1);
    }
  }
  if (!(fout = fopen(argv[2*n+1], "w"))) {
    fprintf(stderr, "%s: couldn't open output file \"%s\" for writing\n",
	    me, argv[2*n+1]);
    exit(1);
  }

  for (d=0; d<=n-1; d++) {
    if (nrrdRange(nin[d])) {
      fprintf(stderr, "%s: trouble determining range in nrrd %d (%s):\n%s\n",
	      me, d, argv[1+d], biffGet(NRRD));
    }
    min[d] = nin[d]->min;
    max[d] = nin[d]->max;
    printf("range %d: %f %f\n", d, min[d], max[d]);
    printf("bin %d: %d\n", d, bin[d]);
    clamp[d] = 0;
  }
  
  printf("%s: computing multi-histogram ... ", me); fflush(stdout);
  if (!(nout = nrrdNewMultiHisto(nin, n, bin, min, max, clamp))) {
    fprintf(stderr, "%s: trouble doing multi-histogram:\n%s\n", 
	    me, biffGet(NRRD));
    exit(1);
  }
  printf("done\n");
  if (nrrdWrite(fout, nout)) {
    fprintf(stderr, "%s: trouble writing output nrrd:\n%s\n", 
	    me, biffGet(NRRD));
    exit(1);
  }
  fclose(fout);
  for (d=0; d<=n-1; d++) {
    nrrdNuke(nin[d]);
  }
  nrrdNuke(nout);
}
