#include "../bane.h"

int
main(int argc, char *argv[]) {
  FILE *file;
  Nrrd *info;
  float sigma;
  
  file = fopen(argv[1], "r");
  info = nrrdNewRead(file);
  if (info) {
    if (2 == info->dim) {
      if (baneSigmaCalc1D(&sigma, info)) {
	fprintf(stderr, "trouble:\n%s\n", biffGet(BANE));
      }
    }
    else {
      if (baneSigmaCalc2D(&sigma, info)) {
	fprintf(stderr, "trouble:\n%s\n", biffGet(BANE));
      }
    }
    printf("%g\n", sigma);
  }
}
