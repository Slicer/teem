/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2025  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, see <https://www.gnu.org/licenses/>.
*/

#include <teem/meet.h>

/*
** describes how this Teem was built
*/

int
main(int argc, const char **argv) {
  unsigned int ii;

  char stmp1[AIR_STRLEN_SMALL + 1], stmp2[AIR_STRLEN_SMALL + 1];
  AIR_UNUSED(argc);
  AIR_UNUSED(argv);

  printf("Teem version %s, %s\n", airTeemVersion, airTeemReleaseDate);

  /* some of the things from airSanity */
  printf("airMyEndian() == %d\n", airMyEndian());
  printf("sizeof(size_t) = %s; sizeof(void*) = %s\n",
         airSprintSize_t(stmp1, sizeof(size_t)),
         airSprintSize_t(stmp2, sizeof(void *)));

  printf("libs = ");
  ii = 0;
  do {
    printf("%s ", meetTeemLibs[ii]);
    ii++;
  } while (meetTeemLibs[ii]);
  printf("(%u)\n", ii);

  printf("airThreadCapable = %d\n", airThreadCapable);

  printf("nrrdFFTWEnabled = %d\n", nrrdFFTWEnabled);

#if TEEM_LEVMAR
  printf("yes, TEEM_LEVMAR #defined\n");
#else
  printf(" no, TEEM_LEVMAR not #defined\n");
#endif

  return 0;
}
