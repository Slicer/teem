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

#define INFO "generate list of oriented grid locations"
static const char *_unrrdu_gridInfoL =
(INFO ". For a N-D grid, the output is a 2-D M-by-S array of grid sample "
 "locations, where M is the space dimension of the oriented grid, and S "
 "is the total number of real samples in the grid. "
 "Implementation currently incomplete, because of the number of "
 "unresolved design questions.\n "
 "* (not based on any particular nrrd function)");

static int
gridGen(Nrrd *nout, int typeOut, const Nrrd *nin) {
  static const char me[]="gridGen";
  size_t II, NN, size[NRRD_DIM_MAX], coord[NRRD_DIM_MAX];
  double loc[NRRD_SPACE_DIM_MAX],
    sdir[NRRD_DIM_MAX][NRRD_SPACE_DIM_MAX],
    (*ins)(void *v, size_t I, double d);
  unsigned int axi, dim, sdim, base;
  void *out;

  if (nrrdTypeBlock == typeOut) {
    biffAddf(UNRRDU, "%s: can't use type %s", me,
             airEnumStr(nrrdType, nrrdTypeBlock));
    return 1;
  }
  if (!(nin->spaceDim)) {
    biffAddf(UNRRDU, "%s: can currently only work on arrays "
             "with space directions and space origin", me);
    return 1;
  }
  dim = nin->dim;
  sdim = nin->spaceDim;
  if (!nrrdSpaceVecExists(sdim, nin->spaceOrigin)) {
    biffAddf(UNRRDU, "%s: space origin didn't exist", me);
    return 1;
  }
  if (!( sdim <= dim )) {
    biffAddf(UNRRDU, "%s: can't handle space dimension %u > dimension %u",
             me, sdim, dim);
    return 1;
  }
  base = dim - sdim;
  NN = 1;
  nrrdAxisInfoGet_nva(nin, nrrdAxisInfoSize, size);
  nrrdAxisInfoGet_nva(nin, nrrdAxisInfoSpaceDirection, sdir);
  for (axi=base; axi<dim; axi++) {
    if (!nrrdSpaceVecExists(sdim, sdir[axi])) {
      biffAddf(UNRRDU, "%s: axis %u space dir didn't exist", me, axi);
      return 1;
    }
    NN *= size[axi];
  }
  ins = nrrdDInsert[typeOut];

  if (nrrdMaybeAlloc_va(nout, typeOut, 2,
                        AIR_CAST(size_t, sdim),
                        NN)) {
    biffMovef(UNRRDU, NRRD, "%s: couldn't allocate output", me);
    return 1;
  }
  out = AIR_CAST(void *, nout->data);
  for (axi=0; axi<dim; axi++) {
    coord[axi] = 0;
  }
  for (II=0; II<NN; II++) {
    nrrdSpaceVecCopy(loc, nin->spaceOrigin);
    for (axi=base; axi<dim; axi++) {
      nrrdSpaceVecScaleAdd2(loc, 1, loc, coord[axi], sdir[axi]);
    }
    /*
    fprintf(stderr, "!%s: (%u) %u %u %u: %g %g\n", me,
            AIR_CAST(unsigned int, II),
            AIR_CAST(unsigned int, coord[0]),
            AIR_CAST(unsigned int, coord[1]),
            AIR_CAST(unsigned int, coord[2]),
            loc[0], loc[1]);
    */
    for (axi=0; axi<sdim; axi++) {
      ins(out, axi + sdim*II, loc[axi]);
    }
    NRRD_COORD_INCR(coord, size, dim, base);
  }

  return 0;
}

int
unrrdu_gridMain(int argc, const char **argv, const char *me,
                   hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int pret;
  airArray *mop;

  int typeOut;

  hestOptAdd(&opt, "i,input", "nin", airTypeOther, 1, 1, &nin, NULL,
             "input nrrd.  That this argument is required instead of "
             "optional, as with most unu commands, is a quirk caused by the "
             "need to have \"unu grid\" generate usage info, combined "
             "with the fact that the other arguments have sensible "
             "defaults",
             NULL, NULL, nrrdHestNrrd);
  OPT_ADD_TYPE(typeOut, "type of output", "double");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_gridInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (gridGen(nout, typeOut, nin)) {
    airMopAdd(mop, err = biffGetDone(UNRRDU), airFree, airMopAlways);
    fprintf(stderr, "%s: error generating output:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD_HIDE(grid, INFO);
