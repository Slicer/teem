#include <nrrd.h>

int
main() {
  Nrrd *nrrd, *slice;
  unsigned char *data;
  FILE *file;
  int i;
  char err[NRRD_ERR_STRLEN];

  /*
  nrrd = nrrdNewAlloc(6*6*6, nrrdTypeUChar, 3);
  data = nrrd->data;
  for(i=0; i<=6*6*6-1; i++) {
    data[i] = i;
  }
  file = fopen("wee.nrrd", "w");
  nrrd->encoding = nrrdEncodingRaw;
  nrrd->size[0] = 6;
  nrrd->size[1] = 6;
  nrrd->size[2] = 6;
  if (nrrdWrite(file, nrrd)) {
    nrrdGetErr(err);
    printf("oh dear.\n%s", err);
    exit(1);
  }
  nrrdNuke(nrrd);
  */

  file = fopen("wee.nrrd", "r");
  nrrd = nrrdNewRead(file);
  fclose(file);
  printf("calling slice\n");
  if (!(slice = nrrdNewSlice(nrrd, 2, 5))) {
    nrrdGetErr(err);
    printf("oh dear.\n%s", err);
    exit(1);
  }  
  printf("slice done\n");
  slice->encoding = nrrdEncodingRaw;
  file = fopen("slice.nrrd", "w");
  if (nrrdWrite(file, slice)) {
    nrrdGetErr(err);
    printf("oh dear.\n%s", err);
    exit(1);
  }
  nrrdNuke(slice);
  nrrdNuke(nrrd);
  fclose(file);
}
