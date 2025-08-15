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

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Configuration and platform parameters of this \"unu\""
static const char *_unrrdu_builtInfoL
  = (INFO ". Not every configuration/compilation choice made when building Teem "
          " matters for nrrd and the other libraries that \"unu\" depends on; "
          "see output of \"XXXX HEY WUT? XXXX\" for a view of those. "
          "This documents things visible to \"unu\".");

static int
unrrdu_builtMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  airArray *mop;
  char *err;
  int pret, all, enc, form;
  hestOpt *opt = NULL;

  AIR_UNUSED(argc);
  AIR_UNUSED(argv);
  AIR_UNUSED(me);
  hestOptAdd_Flag(&opt, "a", &all,
                  "list all known info, not just the encoding "
                  "and formats supported by nrrd");
  hparm->noArgsIsNoProblem = AIR_TRUE;

  mop = airMopNew();
  USAGE_OR_PARSE(_unrrdu_builtInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  /*
  Much of the below is based on teem/src/bin/nrrdSanity.c
  That program starts by running nrrdSanity, but there's no reason to do that here,
  because teem/src/bin/unu.c itself runs nrrdSanityOrDie()
  */
  printf("# nrrd file data encodings:\n");
  for (enc = nrrdEncodingTypeUnknown + 1; enc < nrrdEncodingTypeLast; enc++) {
    int avail = nrrdEncodingArray[enc]->available();
    printf("%c encoding %s %s available\n", avail ? '+' : '-',
           airEnumStr(nrrdEncodingType, enc), avail ? "YES is" : "NO not");
  }
  printf("# file formats handled by nrrd library:\n");
  for (form = nrrdFormatTypeUnknown + 1; form < nrrdFormatTypeLast; form++) {
    int avail = nrrdFormatArray[form]->available();
    printf("%c format %s %s available\n", avail ? '+' : '-',
           airEnumStr(nrrdFormatType, form), avail ? "YES is" : "NO not");
  }
  if (all) {
    printf("# optional libraries:\n");
    printf("%c library fftw %s available\n", nrrdFFTWEnabled ? '+' : '-',
           nrrdFFTWEnabled ? "YES is" : "NO not");
    printf("%c library pthread %s available\n", airThreadCapable ? '+' : '-',
           airThreadCapable ? "YES is" : "NO not");
    /* NOT shown: status of Levmar */
    printf("# platform parameters:\n");
    printf("%u = sizeof(void*)\n", (unsigned int)sizeof(void *));
    printf("%u = airMyQNaNHiBit\n", airMyQNaNHiBit);
    printf("%s = airMyEndian()\n", airEnumStr(airEndian, airMyEndian()));
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(built, INFO);
