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

#include "unrrdu.h"
#include "privateUnrrdu.h"

#include <unistd.h> /* for isatty() and STDIN_FILENO */

#define INFO "Compute 32-bit CRC of nrrd data (same as via \"cksum\")"
static const char *_unrrdu_cksumInfoL
  = (INFO ". Unlike other commands, this doesn't produce a nrrd.  It only "
          "prints to standard out the CRC and byte counts for the input nrrd(s), "
          "seeking to emulate the formatting of cksum output.\n "
          "* Uses nrrdCRC32");

static int /* Biff: 1 */
unrrdu_cksumDoit(const char *me, char *inS, int endian, int printendian, FILE *fout) {
  Nrrd *nrrd;
  airArray *mop;
  unsigned int crc;
  char stmp[AIR_STRLEN_SMALL + 1], ends[AIR_STRLEN_SMALL + 1];
  size_t nn;

  if (!strcmp("-", inS) && isatty(STDIN_FILENO)) {
    biffAddf(me, "declining to try reading Nrrd from stdin as tty (terminal)");
    return 1;
  }

  mop = airMopNew();
  airMopAdd(mop, nrrd = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  if (nrrdLoad(nrrd, inS, NULL)) {
    biffMovef(me, NRRD, "%s: trouble loading \"%s\"", me, inS);
    airMopError(mop);
    return 1;
  }
  crc = nrrdCRC32(nrrd, endian);
  nn = nrrdElementNumber(nrrd) * nrrdElementSize(nrrd);
  sprintf(ends, "(%s)", airEnumStr(airEndian, endian));
  fprintf(fout, "%u%s %s%s%s\n", crc, printendian ? ends : "", airSprintSize_t(stmp, nn),
          strcmp("-", inS) ? " " : "", strcmp("-", inS) ? inS : "");

  airMopOkay(mop);
  return 0;
}

static int
unrrdu_cksumMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *err, **inS;
  airArray *mop;
  int pret, endian, printend, okay;
  unsigned int ni, ninLen;

  mop = airMopNew();
  hparm->noArgsIsNoProblem = AIR_TRUE;
  hestOptAdd_1_Enum(&opt, "en,endian", "end", &endian,
                    airEnumStr(airEndian, airMyEndian()),
                    "Endianness in which to compute CRC; \"little\" for Intel and "
                    "friends; \"big\" for everyone else. "
                    "Defaults to endianness of this machine",
                    airEndian);
  hestOptAdd_1_Bool(&opt, "pen,printendian", "bool", &printend, "false",
                    "whether or not to indicate after the CRC value the endianness "
                    "with which the CRC was computed; doing so clarifies "
                    "that the CRC result depends on endianness and may remove "
                    "confusion in comparing results on platforms of different "
                    "endianness");
  hestOptAdd_Nv_String(&opt, NULL, "nin1", 1, -1, &inS, "-",
                       "input nrrd(s). By default tries to read from stdin", &ninLen);
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_cksumInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  /* HEY "okay" logic copied from head.c */
  okay = AIR_FALSE;
  for (ni = 0; ni < ninLen; ni++) {
    if (unrrdu_cksumDoit(me, inS[ni], endian, printend, stdout)) {
      airMopAdd(mop, err = biffGetDone(me), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with \"%s\":\n%s", me, inS[ni], err);
      /* continue working on the remaining files */
    } else {
      okay = AIR_TRUE;
    }
  }
  if (!okay) {
    /* none of the given files could be read; something is wrong */
    if (ninLen > 1) {
      fprintf(stderr, "\n%s: Unable to read any file\n", me);
    }
    hestUsage(stderr, opt, me, hparm);
    fprintf(stderr, "\nFor more info: \"%s --help\"\n", me);
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(cksum, INFO);
