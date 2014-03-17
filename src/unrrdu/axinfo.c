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

#define INFO "Modify attributes of one or more axes"
static const char *_unrrdu_axinfoInfoL =
(INFO
 ". The only attributes which are set are those for which command-line "
 "options are given.\n "
 "* Uses no particular function; just sets fields in the NrrdAxisInfo");

int
unrrdu_axinfoMain(int argc, const char **argv, const char *me,
                  hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err, *label, *units, *centerStr, *kindStr,
    *_dirStr, *dirStr;
  Nrrd *nin, *nout;
  int pret, center, kind;
  unsigned int *axes, axesLen, axi;
  double mm[2], spc, sdir[NRRD_SPACE_DIM_MAX];
  airArray *mop;

  hestOptAdd(&opt, "a,axes", "ax0", airTypeUInt, 1, -1, &axes, NULL,
             "the one or more axes that should be modified", &axesLen);
  hestOptAdd(&opt, "l,label", "label", airTypeString, 1, 1, &label, "",
             "label to associate with axis");
  hestOptAdd(&opt, "u,units", "units", airTypeString, 1, 1, &units, "",
             "units of measurement");
  hestOptAdd(&opt, "mm,minmax", "min max", airTypeDouble, 2, 2, mm, "nan nan",
             "min and max values along axis");
  hestOptAdd(&opt, "sp,spacing", "spacing", airTypeDouble, 1, 1, &spc, "nan",
             "spacing between samples along axis");
  /* There used to be a complaint here about how hest doesn't allow
     you to learn whether the option was parsed from the supplied
     default versus from the command-line itself.  That issue has been
     solved: opt[oi].source now takes on values from the hestSource*
     enum; axinsert.c now provides an example of this. However,
     parsing from a string here is still needed here, because here we
     need to allow the string that represents "no centering"; this
     is a current weakness of airEnumStr.
  hestOptAdd(&opt, "c,center", "center", airTypeEnum, 1, 1, &cent, "unknown",
             "centering of axis: \"cell\" or \"node\"",
             NULL, nrrdCenter);
  */
  hestOptAdd(&opt, "c,center", "center", airTypeString, 1, 1, &centerStr, "",
             "axis centering: \"cell\" or \"node\".  Not using this option "
             "leaves the centering as it is on input");
  hestOptAdd(&opt, "dir,direction", "svec", airTypeString, 1, 1, &_dirStr, "",
             "(NOTE: must quote vector) The \"space direction\": the vector "
             "in space spanned by incrementing (by one) the axis index.");
  hestOptAdd(&opt, "k,kind", "kind", airTypeString, 1, 1, &kindStr, "",
             "axis kind. Not using this option "
             "leaves the kind as it is on input");

  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_axinfoInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  for (axi=0; axi<axesLen; axi++) {
    if (!( axes[axi] < nin->dim )) {
      fprintf(stderr, "%s: axis %u not in valid range [0,%u]\n",
              me, axes[axi], nin->dim-1);
      airMopError(mop);
      return 1;
    }
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
      fprintf(stderr, "%s: wanted to add space direction, but input "
              "doesn't have space dimension set", me);
      airMopError(mop);
      return 1;
    }
    /* mindlessly copying logic from unu make; unsure of the value */
    if ('\"' == _dirStr[0] && '\"' == _dirStr[strlen(_dirStr)-1]) {
      _dirStr[strlen(_dirStr)-1] = 0;
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

  for (axi=0; axi<axesLen; axi++) {
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
    if (AIR_EXISTS(mm[0])) {
      nout->axis[axis].min = mm[0];
    }
    if (AIR_EXISTS(mm[1])) {
      nout->axis[axis].max = mm[1];
    }
    if (AIR_EXISTS(spc)) {
      nout->axis[axis].spacing = spc;
    }
    /* see above
    if (nrrdCenterUnknown != cent) {
      nout->axis[axis].center = cent;
    }
    */
    if (airStrlen(centerStr)) {
      if (!strcmp("none", centerStr)
          || !strcmp("???", centerStr)) {
        center = nrrdCenterUnknown;
      } else {
        if (!(center = airEnumVal(nrrdCenter, centerStr))) {
          fprintf(stderr, "%s: couldn't parse \"%s\" as %s\n", me,
                  centerStr, nrrdCenter->name);
          airMopError(mop);
          return 1;
        }
      }
      nout->axis[axis].center = center;
    }
    if (airStrlen(kindStr)) {
      if (!strcmp("none", kindStr)
          || !strcmp("???", kindStr)) {
        kind = nrrdKindUnknown;
      } else {
        if (!(kind = airEnumVal(nrrdKind, kindStr))) {
          fprintf(stderr, "%s: couldn't parse \"%s\" as %s\n", me,
                  kindStr, nrrdKind->name);
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
