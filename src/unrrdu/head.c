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

#define INFO "Print header of one or more nrrd files"
static const char *_unrrdu_headInfoL
  = (INFO ".  The value of this is simply to print the contents of nrrd "
          "headers.  This avoids the use of \"head -N\", where N has to be "
          "determined manually, which always risks printing raw binary data "
          "(following the header) to screen, which tends to clobber terminal "
          "settings, make pointless beeps, and be annoying.\n "
          "* Uses nrrdOneLine");

static int /* Biff: 1 */
unrrdu_headDoit(const char *me, NrrdIoState *nio, const char *inS, FILE *fout) {
  airArray *mop;
  unsigned int len;
  FILE *fin;

  if (!strcmp("-", inS) && isatty(STDIN_FILENO)) {
    biffAddf(me, "declining to try reading Nrrd from stdin as tty (terminal)");
    return 1;
  }
  mop = airMopNew();
  if (!(fin = airFopen(inS, stdin, "rb"))) {
    biffAddf(me, "couldn't fopen(\"%s\",\"rb\"): %s\n", inS, strerror(errno));
    airMopError(mop);
    return 1;
  }
  airMopAdd(mop, fin, (airMopper)airFclose, airMopAlways);

  if (nrrdOneLine(&len, nio, fin)) {
    biffAddf(me, "error getting first line of file \"%s\"", inS);
    airMopError(mop);
    return 1;
  }
  if (!len) {
    biffAddf(me, "immediately hit EOF\n");
    airMopError(mop);
    return 1;
  }
  if (!(nrrdFormatNRRD->contentStartsLike(nio))) {
    biffAddf(me, "first line (\"%s\") isn't a nrrd magic\n", nio->line);
    airMopError(mop);
    return 1;
  }
  while (len > 1) {
    fprintf(fout, "%s\n", nio->line);
    nrrdOneLine(&len, nio, fin);
  };

  /* experience has shown that on at least windows and darwin, the writing
     process's fwrite() to stdout will fail if we exit without consuming
     everything from stdin */
  if (stdin == fin) {
    int c = getc(fin);
    while (EOF != c) {
      c = getc(fin);
    }
  }

  airMopOkay(mop);
  return 0;
}

static int
unrrdu_headMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *err, **inS;
  NrrdIoState *nio;
  airArray *mop;
  int pret, okay;
  unsigned int ni, ninLen;

  mop = airMopNew();
  hparm->noArgsIsNoProblem = AIR_TRUE;
  hestOptAdd_Nv_String(&opt, NULL, "nin1", 1, -1, &inS, "-",
                       "input nrrd(s). By default tries to read from stdin", &ninLen);
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_headInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nio = nrrdIoStateNew();
  airMopAdd(mop, nio, (airMopper)nrrdIoStateNix, airMopAlways);

  okay = AIR_FALSE;
  for (ni = 0; ni < ninLen; ni++) {
    if (ninLen > 1) {
      fprintf(stdout, "==> %s <==\n", inS[ni]);
    }
    if (unrrdu_headDoit(me, nio, inS[ni], stdout)) {
      airMopAdd(mop, err = biffGetDone(me), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble reading from \"%s\":\n%s", me, inS[ni], err);
      /* continue working on the remaining files */
    } else {
      /* processed at least one file ok */
      okay = AIR_TRUE;
    }
    if (ninLen > 1 && ni < ninLen - 1) {
      fprintf(stdout, "\n");
    }
  }
  if (!okay) {
    /* none of the given files could be read; something is wrong */
    if (ninLen > 1) {
      fprintf(stderr, "\n%s: Unable to read from any file\n", me);
    }
    hestUsage(stderr, opt, me, hparm);
    fprintf(stderr, "\nFor more info: \"%s --help\"\n", me);
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(head, INFO);
