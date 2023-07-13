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

#define INFO "Modify attributes of one or more axes"
static const char *_unrrdu_axinfoInfoL
  = (INFO ". The only attributes which are set are those for which command-line "
          "options are given.\n "
          "* Uses no particular function; just sets fields in the NrrdAxisInfo");

static int
unrrdu_axinfoMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err, *label, *units, *centerStr, *kindStr, *_dirStr, *dirStr, *mmStr[2];
  Nrrd *nin, *nout;
  int pret, center, kind;
  unsigned int *axes, axesLen, axi, mmIdx, spIdx;
  double mm[2], spc, sdir[NRRD_SPACE_DIM_MAX];
  airArray *mop;

  hestOptAdd_Nv_UInt(&opt, "a,axes", "ax0", 1, -1, &axes, NULL,
                     "the one or more axes that should be modified", &axesLen);
  hestOptAdd_1_String(&opt, "l,label", "label", &label, "",
                      "label to associate with axis");
  hestOptAdd_1_String(&opt, "u,units", "units", &units, "", "units of measurement");
  mmIdx = hestOptAdd_2_String(&opt, "mm,minmax", "min max", mmStr, "nan nan",
                              "min and max values along axis");
  spIdx = hestOptAdd_1_Double(&opt, "sp,spacing", "spacing", &spc, "nan",
                              "spacing between samples along axis");
  /* There used to be a complaint here about how hest doesn't allow
     you to learn whether the option was parsed from the supplied
     default versus from the command-line itself.  That issue has been
     solved: opt[oi].source now takes on values from the hestSource*
     enum; axinsert.c now provides an example of this. However,
     parsing from a string here is still needed here, because here we
     need to allow the string that represents "no centering"; this
     is a current weakness of airEnumStr.
  hestOptAdd_1_Enum(&opt, "c,center", "center", &cent, "unknown",
                    "centering of axis: \"cell\" or \"node\"",
                    nrrdCenter);
  */
  hestOptAdd_1_String(&opt, "c,center", "center", &centerStr, "",
                      "axis centering: \"cell\" or \"node\".  Not using this option "
                      "leaves the centering as it is on input");
  hestOptAdd_1_String(&opt, "dir,direction", "svec", &_dirStr, "",
                      "(NOTE: must quote vector) The \"space direction\": the vector "
                      "in space spanned by incrementing (by one) the axis index.");
  hestOptAdd_1_String(&opt, "k,kind", "kind", &kindStr, "",
                      "axis kind. Not using this option "
                      "leaves the kind as it is on input");

  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_axinfoInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  for (axi = 0; axi < axesLen; axi++) {
    if (!(axes[axi] < nin->dim)) {
      fprintf(stderr, "%s: axis %u not in valid range [0,%u]\n", me, axes[axi],
              nin->dim - 1);
      airMopError(mop);
      return 1;
    }
  }
  /* parse the strings given via -mm */
  if (2
      != airSingleSscanf(mmStr[0], "%lf", mm + 0)
           + airSingleSscanf(mmStr[1], "%lf", mm + 1)) {
    fprintf(stderr,
            "%s: couldn't parse both \"%s\" and \"%s\" "
            "(from \"-mm\") as doubles\n",
            me, mmStr[0], mmStr[1]);
    airMopError(mop);
    return 1;
  }

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdCopy(nout, nin)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error copying input:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (airStrlen(_dirStr)) {
    if (!nin->spaceDim) {
      fprintf(stderr,
              "%s: wanted to add space direction, but input "
              "doesn't have space dimension set",
              me);
      airMopError(mop);
      return 1;
    }
    /* mindlessly copying logic from unu make; unsure of the value */
    if ('\"' == _dirStr[0] && '\"' == _dirStr[strlen(_dirStr) - 1]) {
      _dirStr[strlen(_dirStr) - 1] = 0;
      dirStr = _dirStr + 1;
    } else {
      dirStr = _dirStr;
    }
    if (nrrdSpaceVectorParse(sdir, dirStr, nin->spaceDim, AIR_TRUE)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: couldn't parse space vector:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
  } else {
    dirStr = NULL;
  }

  for (axi = 0; axi < axesLen; axi++) {
    unsigned int axis;
    axis = axes[axi];
    if (strlen(label)) {
      nout->axis[axis].label = (char *)airFree(nout->axis[axis].label);
      nout->axis[axis].label = airStrdup(label);
    }
    if (strlen(units)) {
      nout->axis[axis].units = (char *)airFree(nout->axis[axis].units);
      nout->axis[axis].units = airStrdup(units);
    }
    if (hestSourceUser == opt[mmIdx].source) {
      /* if it came from user, set the value, even if its nan. Actually,
         especially if its nan: that is the purpose of this extra logic */
      nout->axis[axis].min = mm[0];
      nout->axis[axis].max = mm[1];
    } else {
      if (AIR_EXISTS(mm[0])) {
        nout->axis[axis].min = mm[0];
      }
      if (AIR_EXISTS(mm[1])) {
        nout->axis[axis].max = mm[1];
      }
    }
    if (hestSourceUser == opt[spIdx].source) {
      /* same logic as with min,max above */
      nout->axis[axis].spacing = spc;
    } else {
      if (AIR_EXISTS(spc)) {
        nout->axis[axis].spacing = spc;
      }
    }
    /* see above
    if (nrrdCenterUnknown != cent) {
      nout->axis[axis].center = cent;
    }
    */
    if (airStrlen(centerStr)) {
      if (!strcmp("none", centerStr) || !strcmp("???", centerStr)) {
        center = nrrdCenterUnknown;
      } else {
        if (!(center = airEnumVal(nrrdCenter, centerStr))) {
          fprintf(stderr, "%s: couldn't parse \"%s\" as %s\n", me, centerStr,
                  nrrdCenter->name);
          airMopError(mop);
          return 1;
        }
      }
      nout->axis[axis].center = center;
    }
    if (airStrlen(kindStr)) {
      if (!strcmp("none", kindStr) || !strcmp("???", kindStr)) {
        kind = nrrdKindUnknown;
      } else {
        if (!(kind = airEnumVal(nrrdKind, kindStr))) {
          fprintf(stderr, "%s: couldn't parse \"%s\" as %s\n", me, kindStr,
                  nrrdKind->name);
          airMopError(mop);
          return 1;
        }
      }
      nout->axis[axis].kind = kind;
    }
    if (dirStr) {
      nrrdSpaceVecCopy(nout->axis[axis].spaceDirection, sdir);
    }
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(axinfo, INFO);
