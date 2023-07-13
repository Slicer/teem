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

#define INFO "Save all slices along one axis into separate files"
static const char *_unrrdu_diceInfoL
  = (INFO ". Calls \"unu slice\" for each position "
          "along the indicated axis, and saves out a different "
          "file for each sample along that axis.\n "
          "* Uses repeated calls to nrrdSlice and nrrdSave");

static int
unrrdu_diceMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *base, *err, fnout[AIR_STRLEN_MED + 1], /* file name out */
    fffname[AIR_STRLEN_MED + 1],               /* format for filename */
    *ftmpl;                                    /* format template */
  Nrrd *nin, *nout;
  int pret, fit;
  unsigned int axis, start, pos, top, size, sanity;
  airArray *mop;

  OPT_ADD_AXIS(axis, "axis to slice along");
  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd_1_UInt(&opt, "s,start", "start", &start, "0",
                    "integer value to start numbering with");
  hestOptAdd_1_String(&opt, "ff,format", "form", &ftmpl, "",
                      "a printf-style format to use for generating all "
                      "filenames.  Use this to override the number of characters "
                      "used to represent the slice position, or the file format "
                      "of the output, e.g. \"-ff %03d.ppm\" for 000.ppm, "
                      "001.ppm, etc. By default (not using this option), slices "
                      "are saved in NRRD format (or PNM or PNG where possible) "
                      "with shortest possible filenames.");
  /* the fact that we're using unsigned int instead of size_t is
     its own kind of sanity check */
  hestOptAdd_1_UInt(&opt, "l,limit", "max#", &sanity, "9999",
                    "a sanity check on how many slice files should be saved "
                    "out, to prevent accidentally dicing the wrong axis "
                    "or the wrong array. Can raise this value if needed.");
  hestOptAdd_1_String(&opt, "o,output", "prefix", &base, NULL,
                      "output filename prefix (excluding info set via \"-ff\"), "
                      "basically to set path of output files (so be sure to end "
                      "with \"/\".");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_diceInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (!(axis < nin->dim)) {
    fprintf(stderr, "%s: given axis (%u) outside range [0,%u]\n", me, axis,
            nin->dim - 1);
    airMopError(mop);
    return 1;
  }
  if (nin->axis[axis].size > sanity) {
    char stmp[AIR_STRLEN_SMALL + 1];
    fprintf(stderr,
            "%s: axis %u size %s > sanity limit %u; "
            "increase via \"-l\"\n",
            me, axis, airSprintSize_t(stmp, nin->axis[axis].size), sanity);
    airMopError(mop);
    return 1;
  }
  size = AIR_UINT(nin->axis[axis].size);

  /* HEY: this should use nrrdSaveMulti(), and if there's additional
     smarts here, they should be moved into nrrdSaveMulti() */
  if (airStrlen(ftmpl)) {
    if (!(nrrdContainsPercentThisAndMore(ftmpl, 'd')
          || nrrdContainsPercentThisAndMore(ftmpl, 'u'))) {
      fprintf(stderr,
              "%s: given filename format \"%s\" doesn't seem to "
              "have the converstion specification to print an integer\n",
              me, ftmpl);
      airMopError(mop);
      return 1;
    }
    sprintf(fffname, "%%s%s", ftmpl);
  } else {
    unsigned int dignum = 0, tmps;
    tmps = top = start + size - 1;
    do {
      dignum++;
      tmps /= 10;
    } while (tmps);
    /* sprintf the number of digits into the string that will be used
       to sprintf the slice number into the filename */
    sprintf(fffname, "%%s%%0%uu.nrrd", dignum);
  }
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  for (pos = 0; pos < size; pos++) {
    if (nrrdSlice(nout, nin, axis, pos)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error slicing nrrd:%s\n", me, err);
      airMopError(mop);
      return 1;
    }
    if (0 == pos && !airStrlen(ftmpl)) {
      /* See if these slices would be better saved as PNG or PNM images.
         Altering the file name will tell nrrdSave() to use a different
         file format.  We wait till now to check this so that we can
         work from the actual slice */
      if (nrrdFormatPNG->fitsInto(nout, nrrdEncodingRaw, AIR_FALSE)) {
        strcpy(fffname + strlen(fffname) - 4, "png");
      } else {
        fit = nrrdFormatPNM->fitsInto(nout, nrrdEncodingRaw, AIR_FALSE);
        if (2 == fit) {
          strcpy(fffname + strlen(fffname) - 4, "pgm");
        } else if (3 == fit) {
          strcpy(fffname + strlen(fffname) - 4, "ppm");
        }
      }
    }
    sprintf(fnout, fffname, base, pos + start);
    if (nrrdStateVerboseIO > 0) {
      fprintf(stderr, "%s: %s ...\n", me, fnout);
    }
    if (nrrdSave(fnout, nout, NULL)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error writing nrrd to \"%s\":%s\n", me, fnout, err);
      airMopError(mop);
      return 1;
    }
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(dice, INFO);
