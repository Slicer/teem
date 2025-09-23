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

  // airEnumPrint(stdout, hestSource);
  char enerr[AIR_STRLEN_LARGE + 1];
  if (airEnumCheck(enerr, hestSource)) {
    fprintf(stderr, "%s: problem:\n%s\n", argv[0], enerr);
    exit(1);
  }

  int ret = 0;
  hestOpt *opt = NULL;
  hestParm *hparm = hestParmNew();
  hparm->respectDashDashHelp = AIR_TRUE;
  hparm->responseFileEnable = AIR_TRUE;
  hparm->verbosity = 10;

  int verb;
  hestOptAdd_1_Int(&opt, "v", "verb", &verb, "0", "verbosity");
  int res[2];
  hestOptAdd_2_Int(&opt, "s,size", "sx sy", res, NULL, "image resolution");
  int flag;
  hestOptAdd_Flag(&opt, "b,bingo", &flag, "a flag");
  char *err = NULL;
  if (hestParse2(opt, argc - 1, argv + 1, &err, hparm)) {
    fprintf(stderr, "%s: problem:\n%s\n", argv[0], err);
    free(err);
    ret = 1;
  }
  if (opt->helpWanted) {
    printf("%s: help wanted!\n", argv[0]);
  }

  hestOptFree(opt);
  hestParmFree(hparm);

  exit(ret);
}
