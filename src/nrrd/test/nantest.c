/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/


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
