/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "../nrrd.h"

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
