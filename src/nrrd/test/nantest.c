#include <nrrd.h>

/*
extern int nrrdIsNand(double);
extern int nrrdIsNanf(float);
extern double nrrdNand(void);
extern float nrrdNanf(void);
*/

void
main() {
  float nanf;
  double nand;

  nanf = 1.0;
  printf("%d\n", nrrdIsNanf(nanf));
  nanf /= 0.0;
  printf("%d\n", nrrdIsNanf(nanf));
  nanf /= nanf;
  printf("%d\n", nrrdIsNanf(nanf));
  nanf = nrrdNanf();
  printf("%d\n", nrrdIsNanf(nanf));

  nand = 1.0;
  printf("%d\n", nrrdIsNand(nand));
  nand /= 0.0;
  printf("%d\n", nrrdIsNand(nand));
  nand /= nand;
  printf("%d\n", nrrdIsNand(nand));
  nand = nrrdNand();
  printf("%d\n", nrrdIsNand(nand));
}
