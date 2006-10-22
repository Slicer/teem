/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "../push.h"

char *info = ("Tests parsing of energy, and its methods.");

int
main(int argc, char *argv[]) {
  char *me, *err;
  hestOpt *hopt=NULL;
  airArray *mop;
  
  pushEnergySpec *ensp;
  unsigned int pi;

  mop = airMopNew();
  me = argv[0];
  hestOptAdd(&hopt, "energy", "spec", airTypeOther, 1, 1, &ensp, NULL,
             "specification of force function to use",
             NULL, NULL, pushHestEnergySpec);
  hestParseOrDie(hopt, argc-1, argv+1, NULL,
                 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  printf("%s: parsed energy \"%s\", with %u parms.\n", me,
         ensp->energy->name, ensp->energy->parmNum);
  for (pi=0; pi<ensp->energy->parmNum; pi++) {
    printf("%u: %g\n", pi, ensp->parm[pi]);
  }

  airMopOkay(mop);
  return 0;
}

