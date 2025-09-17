/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
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

#include "../hest.h"

int
main(int argc, const char **argv) {

  AIR_UNUSED(argc);
  printf("%s: yo\n", argv[0]);
  hestArg *harg = hestArgNew();
  printf("%s: harg = %p\n", argv[0], harg);
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgAddChar(harg, 'c');
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgAddChar(harg, 'a');
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgAddChar(harg, 't');
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgAddChar(harg, 'a');
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgAddChar(harg, 's');
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgAddChar(harg, 't');
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgAddChar(harg, 'r');
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgAddChar(harg, 'o');
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgAddChar(harg, 'p');
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgAddChar(harg, 'h');
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgAddChar(harg, 'e');
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgAddChar(harg, '!');
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgAddString(harg, "bingo bob lives\n");
  printf("%s: |%s|\n", argv[0], harg->str);
  hestArgNix(harg);

  hestArgVec *havec = hestArgVecNew();
  hestArgVecPrint(argv[0], havec);
  hestArgVecAppendString(havec, "this");
  hestArgVecPrint(argv[0], havec);
  hestArgVecAppendString(havec, "is");
  hestArgVecPrint(argv[0], havec);
  hestArgVecAppendString(havec, "totally");
  hestArgVecPrint(argv[0], havec);
  hestArgVecAppendString(havec, "");
  hestArgVecPrint(argv[0], havec);
  hestArgVecAppendString(havec, "bonkers");
  hestArgVecPrint(argv[0], havec);
  hestArgVecNix(havec);

  exit(0);
}
