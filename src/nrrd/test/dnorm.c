/*
  Teem: Tools to process and visualize scientific data and images              
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


#include "../nrrd.h"

char *dnormInfo = ("Normalizes nrrd representation for Diderot. "
                   "Forces information about kind and orientation into "
                   "a consistent form, and nixes other information. ");

int
main(int argc, char **argv) {
  char *me, *outS;
  hestOpt *hopt;
  hestParm *hparm;
  airArray *mop;

  char *err;
  Nrrd *nin, *nout;
  NrrdIoState *nio;
  int kindIn, kindOut;
  unsigned int kindAxis, axi;
  
  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  hopt = NULL;
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nin, NULL,
             "input image", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "-",
             "output filename", NULL);

  hestParseOrDie(hopt, argc-1, argv+1, hparm,
                 me, dnormInfo, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  /* make sure all kinds are set to something */
  /* see if there's a range kind, verify that there's only one */
  kindIn = nrrdKindUnknown;
  kindAxis = 0;
  for (axi=0; axi<nin->dim; axi++) {
    if (nrrdKindUnknown == nin->axis[axi].kind
        || nrrdKindIsDomain(nin->axis[axi].kind)) {
      continue;
    }
    if (nrrdKindUnknown != kindIn) {
      fprintf(stderr, "%s: got non-domain kind %s on axis %u, but already "
              "have %s from axis %u\n", me, 
              airEnumStr(nrrdKind, nin->axis[axi].kind), axi,
              airEnumStr(nrrdKind, kindIn), kindAxis);
      airMopError(mop); exit(1);
    }
    kindIn = nin->axis[axi].kind;
    kindAxis = axi;
  }
  /* see if the non-domain kind is something we can interpret as a tensor */
  if (nrrdKindUnknown != kindIn) {
    switch (kindIn) {
      /* ======= THESE are the kinds that we can possibly output ======= */
    case nrrdKind2Vector:
    case nrrdKind3Vector:
    case nrrdKind4Vector:
    case nrrdKind2DSymMatrix:
    case nrrdKind2DMatrix:
    case nrrdKind3DSymMatrix:
    case nrrdKind3DMatrix:
      /* =============================================================== */
      kindOut = kindIn;
      break;
    case nrrdKind3Color:
    case nrrdKindRGBColor:
      kindOut = nrrdKind3Vector;
      break;
    case nrrdKind4Color:
    case nrrdKindRGBAColor:
      kindOut = nrrdKind4Vector;
      break;
    default:
      fprintf(stderr, "%s: got non-conforming kind %s on axis %u\n", me,
              airEnumStr(nrrdKind, kindIn), kindAxis);
      airMopError(mop); exit(1);
      break;
    }
  } else {
    kindOut = nrrdKindUnknown;
  }

  /* prepare output (this may eventually to cropping or something else) */
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdCopy(nout, nin)) {
    airMopAdd(mop, err = biffGet(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble copying:\n%s", me, err);
    airMopError(mop); exit(1);
    
  }

  /* no comments, either advertising the format URL or anything else */
  nio = nrrdIoStateNew();
  airMopAdd(mop, nio, (airMopper)nrrdIoStateNix, airMopAlways);
  nio->skipFormatURL = AIR_TRUE;
  nrrdCommentClear(nout);
  
  /* no key/value pairs */
  nrrdKeyValueClear(nout);

  /* no content field */
  nout->content = airFree(nout->content);

  /* normalize domain kinds to "space" */
  /* turn off centers (perhaps Diderot should assume cell-centered) */
  /* turn off thickness */
  /* turn off labels and units */
  for (axi=0; axi<nout->dim; axi++) {
    if (nrrdKindUnknown == kindOut) {
      nout->axis[axi].kind = nrrdKindSpace;
    } else {
      nout->axis[axi].kind = (kindAxis == axi
                              ? kindOut
                              : nrrdKindSpace);
    }
    nout->axis[axi].center = nrrdCenterUnknown;
    nout->axis[axi].thickness = AIR_NAN;
    nout->axis[axi].label = airFree(nout->axis[axi].label);
    nout->axis[axi].units = airFree(nout->axis[axi].units);
    nout->axis[axi].min = AIR_NAN;
    nout->axis[axi].max = AIR_NAN;
    nout->axis[axi].spacing = AIR_NAN;
  }

  /* logic of orientation definition: 
     if space dimension is known:
        set origin to zero if not already set
        set space direction to unit vector if not already set
     else:
        set origin to zero and all space directions to units
     might be nice to use gage's logic for mapping from world to index,
     but we have to accept a greater variety of kinds and dimensions
     than gage ever has to process.
  */
  if (nout->spaceDim) {
    int saxi = 0;
    if (!nrrdSpaceVecExists(nout->spaceDim, nout->spaceOrigin)) {
      nrrdSpaceVecSetZero(nout->spaceOrigin);
    }
    for (axi=0; axi<nout->dim; axi++) {
      if (nrrdKindUnknown == kindOut || kindAxis != axi) {
        if (!nrrdSpaceVecExists(nout->spaceDim,
                                nout->axis[axi].spaceDirection)) {
          nrrdSpaceVecSetZero(nout->axis[axi].spaceDirection);
          nout->axis[axi].spaceDirection[saxi] = 1.0;
        }
        saxi++;
      } else {
        nrrdSpaceVecSetNaN(nout->axis[axi].spaceDirection);
      }
    }
  } else {
    int saxi = 0;
    nrrdSpaceVecSetZero(nout->spaceOrigin);
    for (axi=0; axi<nout->dim; axi++) {
      if (nrrdKindUnknown == kindOut || kindAxis != axi) {
        nrrdSpaceVecSetZero(nout->axis[axi].spaceDirection);
        nout->axis[axi].spaceDirection[saxi] = 1.0;
        saxi++;
      } else {
        nrrdSpaceVecSetNaN(nout->axis[axi].spaceDirection);
      }
    }
    nout->spaceDim = saxi;
  }

  if (nrrdSave(outS, nout, nio)) {
    airMopAdd(mop, err = biffGet(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble saving \"%s\":\n%s", 
            me, outS, err);
    airMopError(mop); exit(1);
  }
  
  airMopOkay(mop);
  exit(0);
}
