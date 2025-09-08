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

#include <teem/air.h>

/*
** Tests:
** airSprintSize_t
** airSprintPtrdiff_t
*/

int
main(int argc, const char *argv[]) {
  const char *me;
  size_t sz;
  ptrdiff_t pd;
  char stmp[AIR_STRLEN_SMALL + 1];

  AIR_UNUSED(argc);
  me = argv[0];

  sz = 123456789;
  airSprintSize_t(stmp, sz);
  if (strcmp("123456789", stmp)) {
    fprintf(stderr, "%s: airSprintSize_t: |%s|\n", me, stmp);
    exit(1);
  }

  pd = -123456789;
  airSprintPtrdiff_t(stmp, pd);
  if (strcmp("-123456789", stmp)) {
    fprintf(stderr, "%s: airSprintPtrdiff_t: |%s|\n", me, stmp);
    exit(1);
  }

  exit(0);
}
