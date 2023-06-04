/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2019  University of Chicago
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

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Removes comments from a C99 input file"
static const char *_unrrdu_uncmtInfoL
  = (INFO
     ".\n "
     "This is useful for a class GLK teaches, wherein students are told not to use\n "
     "types \"float\" or \"double\" directly (instead they use a class-specific\n "
     "\"real\" typedef). Grepping for \"float\" and \"double\" isn't informative\n "
     "since they can show up in comments; hence the need for this. Catching\n "
     "implicit conversions to double-precision is handled separately.\n "
     "* (not actually based on Nrrd)");

static void
uncomment(const char *me, const char *nameIn, const char *nameOut) {
  airArray *mop;
  FILE *fin, *fout;
  int ci, co;

  mop = airMopNew();
  if (!(airStrlen(nameIn) && airStrlen(nameOut))) {
    fprintf(stderr, "%s: empty filename for in (\"%s\") or out (\"%s\")\n", me, nameIn,
            nameOut);
    airMopError(mop);
    return;
  }

  /* -------------------------------------------------------- */
  /* open input and output files  */
  fin = airFopen(nameIn, stdin, "rb");
  if (!fin) {
    fprintf(stderr, "%s: couldn't open \"%s\" for reading: \"%s\"\n", me, nameIn,
            strerror(errno));
    airMopError(mop);
    return;
  }
  airMopAdd(mop, fin, (airMopper)airFclose, airMopOnError);
  fout = airFopen(nameOut, stdout, "wb");
  if (!fout) {
    fprintf(stderr, "%s: couldn't open \"%s\" for writing: \"%s\"\n", me, nameOut,
            strerror(errno));
    airMopError(mop);
    return;
  }
  airMopAdd(mop, fout, (airMopper)airFclose, airMopOnError);

  /* -------------------------------------------------------- */
  /* do the conversion.  Some sources of inspiration:
  https://stackoverflow.com/questions/47565090/need-help-to-extract-comments-from-c-file
  https://stackoverflow.com/questions/27847725/reading-a-c-source-file-and-skipping-comments?rq=3
  */
  while ((ci = fgetc(fin)) != EOF) {
    co = ci; /* HEY: do conversion */
    fputc(co, fout);
  }
  airMopOkay(mop);
  return;
}

static int
unrrdu_uncmtMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  /* these are stock for unrrdu */
  hestOpt *opt = NULL;
  airArray *mop;
  int pret;
  char *err;
  /* these are specific to this command */
  char *nameIn, *nameOut;

  hestOptAdd(&opt, NULL, "fileIn", airTypeString, 1, 1, &nameIn, NULL,
             "Single input file to read; use \"-\" for stdin");
  hestOptAdd(&opt, NULL, "fileOut", airTypeString, 1, 1, &nameOut, NULL,
             "Single output filename; use \"-\" for stdout");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);
  USAGE(_unrrdu_uncmtInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  uncomment(me, nameIn, nameOut);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD_HIDE(uncmt, INFO);
