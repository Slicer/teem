/*
  Teem: Tools to process and visualize scientific data and images             .
  Copyright (C) 2013, 2012, 2011, 2010, 2009  University of Chicago
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

#define INFO "Add a \"stub\" (length 1) axis to a nrrd"
static const char *_unrrdu_axinsertInfoL =
(INFO
 ". The underlying linear ordering of the samples is "
 "unchanged, and the information about the other axes is "
 "shifted upwards as needed.\n "
 "* Uses nrrdAxesInsert, and with \"-s\", nrrdPad_nva");

int
unrrdu_axinsertMain(int argc, const char **argv, const char *me,
                    hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err, *label;
  Nrrd *nin, *nout;
  int pret, kind;
  unsigned int axis, size, opi;
  double mm[2];
  airArray *mop;
  NrrdBoundarySpec *bspec;

  hparm->elideSingleOtherDefault = AIR_FALSE;
  OPT_ADD_AXIS(axis, "dimension (axis index) at which to insert the new axis");
  hestOptAdd(&opt, "l,label", "label", airTypeString, 1, 1, &label, "",
             "label to associate with new axis");
  opi = hestOptAdd(&opt, "k,kind", "kind", airTypeEnum, 1, 1, &kind, "stub",
                   "axis kind to associate with new axis", NULL, nrrdKind);
  hestOptAdd(&opt, "mm,minmax", "min max", airTypeDouble, 2, 2, mm, "nan nan",
             "min and max values along new axis");
  hestOptAdd(&opt, "s,size", "size", airTypeUInt, 1, 1, &size, "1",
             "after inserting stub axis, also pad out to some length, "
             "according to the \"-b\" option");
  hestOptAdd(&opt, "b,boundary", "behavior", airTypeOther, 1, 1, &bspec,
             "bleed",
             "How to handle samples beyond the input bounds:\n "
             "\b\bo \"pad:<val>\": use specified value\n "
             "\b\bo \"bleed\": extend border values outward\n "
             "\b\bo \"mirror\": repeated reflections\n "
             "\b\bo \"wrap\": wrap-around to other side",
             NULL, NULL, nrrdHestBoundarySpec);
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_axinsertInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdAxesInsert(nout, nin, axis)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error inserting axis:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (1 < size) {
    /* we also do padding here */
    ptrdiff_t min[NRRD_DIM_MAX], max[NRRD_DIM_MAX];
    unsigned int ai;
    Nrrd *npad;
    for (ai=0; ai<nout->dim; ai++) {
      min[ai] = 0;
      max[ai] = nout->axis[ai].size - 1;
    }
    max[axis] = size-1;
    npad = nrrdNew();
    airMopAdd(mop, npad, (airMopper)nrrdNuke, airMopAlways);
    if (nrrdPad_nva(npad, nout, min, max,
                    bspec->boundary, bspec->padValue)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error padding:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    /* sneaky, but ok; nothing changes in the mops */
    nout = npad;
    /* only set output kind if explicitly requested
       (since the default is not appropriate) */
    if (hestSourceUser == opt[opi].source) {
      nout->axis[axis].kind = kind;
    }
  } else {
    /* no request to pad; setting the default "stub" kind is sensible */
    nout->axis[axis].kind = kind;
  }
  if (strlen(label)) {
    nout->axis[axis].label = (char *)airFree(nout->axis[axis].label);
    nout->axis[axis].label = airStrdup(label);
  }
  if (AIR_EXISTS(mm[0])) {
    nout->axis[axis].min = mm[0];
  }
  if (AIR_EXISTS(mm[1])) {
    nout->axis[axis].max = mm[1];
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(axinsert, INFO);
