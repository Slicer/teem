/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2012, 2011, 2010, 2009  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "teem/meet.h"

/*
** describes how this Teem was built
*/

int
main(int argc, const char **argv) {

#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  char explibs[] = "*ON!*";
#else
  char explibs[] = "_off_";
#endif

  /* apparently TEEM_BUILD_EXPERIMENTAL_APPS is not disclosed to
     the compilation of this file? */
#if defined(TEEM_BUILD_EXPERIMENTAL_APPS)
  char expapps[] = "*ON!*";
#else
  char expapps[] = "_off_";
#endif

  char liblist[AIR_STRLEN_LARGE];
  char stmp1[AIR_STRLEN_SMALL], stmp2[AIR_STRLEN_SMALL];
  AIR_UNUSED(argc);
  AIR_UNUSED(argv);

  printf("Teem version %s, %s\n", airTeemVersion, airTeemReleaseDate);

  /* some of the things from airSanity */
  printf("airMyEndian() == %d\n", airMyEndian());
  printf("AIR_QNANHIBIT == %d\n", AIR_QNANHIBIT);
  printf("sizeof(size_t) = %s; sizeof(void*) = %s\n",
         airSprintSize_t(stmp1, sizeof(size_t)),
         airSprintSize_t(stmp2, sizeof(void*)));

  strcpy(liblist, "");
  /* TEEM_LIB_LIST */
  strcat(liblist, "air ");
  strcat(liblist, "hest ");
  strcat(liblist, "biff ");
  strcat(liblist, "nrrd ");
  strcat(liblist, "ell ");
  strcat(liblist, "unrrdu ");
#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  strcat(liblist, "alan ");
#endif
  strcat(liblist, "moss ");
#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  strcat(liblist, "tijk ");
#endif
  strcat(liblist, "gage ");
  strcat(liblist, "dye ");
#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  strcat(liblist, "bane ");
#endif
  strcat(liblist, "limn ");
  strcat(liblist, "echo ");
  strcat(liblist, "hoover ");
  strcat(liblist, "seek ");
  strcat(liblist, "ten ");
#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  strcat(liblist, "elf ");
#endif
  strcat(liblist, "pull ");
#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  strcat(liblist, "coil ");
  strcat(liblist, "push ");
#endif
  strcat(liblist, "mite ");
  strcat(liblist, "meet ");

  printf("experimental libs %s; apps %s\n", explibs, expapps);
  printf("libs = %s\n", liblist);

  printf("airThreadCapable = %d\n", airThreadCapable);

  printf("nrrdFFTWEnabled = %d\n", nrrdFFTWEnabled);

#if TEEM_LEVMAR
  printf("yes, TEEM_LEVMAR #defined\n");
#else
  printf(" no, TEEM_LEVMAR not #defined\n");
#endif

  return 0;
}
