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

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "../hest.h"

int
main(int argc, const char **argv) {
  hestOpt *opt = NULL;
  hestParm *parm;
  char *err = NULL,
       info[] = "This program does nothing in particular, though it does attempt "
                "to pose as some sort of command-line image processing program. "
                "As usual, any implied functionality is purely coincidental, "
                "especially since this is the output of a gray-haired unicyclist.";

  parm = hestParmNew();
  parm->respFileEnable = AIR_TRUE;
  parm->respectDashDashHelp = AIR_TRUE;
  parm->verbosity = 0;

  opt = NULL;
  /* going past C89 to have declarations here */
  int flag;
  hestOptAdd_Flag(&opt, "f,flag", &flag, "a flag created via hestOptAdd_Flag");
  int b1;
  hestOptAdd_1_Bool(&opt, "b1", "bool1", &b1, "false", "test of hestOptAdd_1_Bool");
  int i1;
  hestOptAdd_1_Int(&opt, "i1", "int1", &i1, "42", "test of hestOptAdd_1_Int");
  unsigned int ui1;
  hestOptAdd_1_UInt(&opt, "ui1", "uint1", &ui1, "42", "test of hestOptAdd_1_UInt");
  long int li1;
  hestOptAdd_1_LongInt(&opt, "li1", "lint1", &li1, "42", "test of hestOptAdd_1_LongInt");
  unsigned long int uli1;
  hestOptAdd_1_ULongInt(&opt, "uli1", "ulint1", &uli1, "42",
                        "test of hestOptAdd_1_ULongInt");
  size_t sz1;
  hestOptAdd_1_Size_t(&opt, "sz1", "size1", &sz1, "42", "test of hestOptAdd_1_Size_t");
  float fl1;
  hestOptAdd_1_Float(&opt, "fl1", "float1", &fl1, "4.2", "test of hestOptAdd_1_Float");
  double db1;
  hestOptAdd_1_Double(&opt, "db1", "double1", &db1, "4.2",
                      "test of hestOptAdd_1_Double");
  char c1;
  hestOptAdd_1_Char(&opt, "c1", "char1", &c1, "x", "test of hestOptAdd_1_Char");
  char *s1;
  hestOptAdd_1_String(&opt, "s1", "string1", &s1, "bingo",
                      "test of hestOptAdd_1_String");
  int e1;
  hestOptAdd_1_Enum(&opt, "e1", "enum1", &e1, "little", "test of hestOptAdd_1_Enum",
                    airEndian);
  /*
HEST_EXPORT unsigned int hestOptAdd_1_Other(hestOpt * *optP, const char *flag,
                       const char *name, void *valueP,
                       const char *dflt, const char *info,
                       const hestCB *CB);
*/

  if (1 == argc) {
    /* didn't get anything at all on command line */
    /* print program information ... */
    hestInfo(stderr, argv[0], info, parm);
    /* ... and usage information ... */
    hestUsage(stderr, opt, argv[0], parm);
    hestGlossary(stderr, opt, parm);
    /* ... and avoid memory leaks */
    opt = hestOptFree(opt);
    parm = hestParmFree(parm);
    exit(1);
  }

  /* else we got something, see if we can parse it */
  if (hestParse(opt, argc - 1, argv + 1, &err, parm)) {
    fprintf(stderr, "ERROR: %s\n", err);
    free(err);
    /* print usage information ... */
    hestUsage(stderr, opt, argv[0], parm);
    hestGlossary(stderr, opt, parm);
    /* ... and then avoid memory leaks */
    opt = hestOptFree(opt);
    parm = hestParmFree(parm);
    exit(1);
  } else if (opt->helpWanted) {
    hestUsage(stdout, opt, argv[0], parm);
    hestGlossary(stdout, opt, parm);
    opt = hestOptFree(opt);
    parm = hestParmFree(parm);
    exit(1);
  }

  {
    unsigned int opi, numO;
    numO = hestOptNum(opt);
    for (opi = 0; opi < numO; opi++) {
      printf("opt %u/%u:\n", opi, numO);
      printf("  flag=%s; ", opt[opi].flag ? opt[opi].flag : "(null)");
      printf("  name=%s\n", opt[opi].name ? opt[opi].name : "(null)");
      printf("  source=%s; ", hestSourceDefault == opt[opi].source
                                ? "default"
                                : (hestSourceUser == opt[opi].source ? "user" : "???"));
      printf("  parmStr=|%s|\n", opt[opi].parmStr ? opt[opi].parmStr : "(null)");
    }
  }
  printf("(err = %s)\n", err ? err : "(null)");
  printf("flag = %d\n", flag);
  printf("b1 = %d\n", b1);
  printf("i1 = %d\n", i1);
  printf("ui1 = %u\n", ui1);
  printf("li1 = %ld\n", li1);
  printf("uli1 = %lu\n", uli1);
  printf("sz1 = %zu\n", sz1);
  printf("fl1 = %g\n", fl1);
  printf("db1 = %g\n", db1);
  printf("c1 = %c\n", c1);
  printf("s1 = %s\n", s1);
  printf("e1 = %d\n", e1);

  /* free the memory allocated by parsing ... */
  hestParseFree(opt);
  /* ... and the other stuff */
  opt = hestOptFree(opt);
  parm = hestParmFree(parm);
  exit(0);
}
