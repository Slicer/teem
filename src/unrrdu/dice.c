/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "private.h"

char *diceName = "dice";
#define INFO "Slice *everywhere* along one axis"
char *diceInfo = INFO;
char *diceInfoL = (INFO
		   ". Calls \"unu slice\" for each position "
		   "along the indicated axis, and saves out a different "
		   "nrrd for each position.");

int
diceMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *base, out[512], *err, format[512];
  Nrrd *nin, *nout;
  int fit, pos, axis, top;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_AXIS(axis, "axis to slice along");
  hestOptAdd(&opt, "o", "prefix", airTypeString, 1, 1, &base, NULL,
	     "output filename prefix. Output nrrds will be saved out as "
	     "<prefix>00.nrrd, <prefix>01.nrrd, and so on." );

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(diceInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (!(AIR_INSIDE(0, axis, nin->dim-1))) {
    fprintf(stderr, "%s: given axis (%d) outside range [0,%d]\n",
	    me, axis, nin->dim-1);
    airMopError(mop);
    return 1;
  }

  top = nin->axis[axis].size-1;
  if (top > 99999)
    sprintf(format, "%%s%%06d.nrrd");
  else if (top > 9999)
    sprintf(format, "%%s%%05d.nrrd");
  else if (top > 999)
    sprintf(format, "%%s%%04d.nrrd");
  else if (top > 99)
    sprintf(format, "%%s%%03d.nrrd");
  else if (top > 9)
    sprintf(format, "%%s%%02d.nrrd");
  else
    sprintf(format, "%%s%%01d.nrrd");
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  for (pos=0; pos<=top; pos++) {
    if (nrrdSlice(nout, nin, axis, pos)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error slicing nrrd:%s\n", me, err);
      airMopError(mop);
      return 1;
    }
    if (0 == pos) {
      /* See if these slices would be better saved as PNM images.
	 Altering the file name will tell nrrdSave() to use a different
	 file format. */
      fit = nrrdFitsInFormat(nout, nrrdFormatPNM, AIR_FALSE);
      if (2 == fit) {
	strcpy(format + strlen(format) - 4, "pgm");
      }
      else if (3 == fit) {
	strcpy(format + strlen(format) - 4, "ppm");
      }
    }
    sprintf(out, format, base, pos);
    fprintf(stderr, "%s: %s ...\n", me, out);
    if (nrrdSave(out, nout, NULL)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error writing nrrd to \"%s\":%s\n", me, out, err);
      airMopError(mop);
      return 1;
    }
  }

  airMopOkay(mop);
  return 0;
}
