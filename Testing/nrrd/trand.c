/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2012, 2011, 2010, 2009  University of Chicago
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

#include "teem/nrrd.h"

/* 
** Tests: 
** airSrandMT
** airNormalRand
** nrrdNew
** nrrdAlloc_va
** nrrdHisto
** nrrdHistoDraw
** nrrdSave (to .pgm file)
** nrrdNuke
*/

#define BINS 1000
#define HGHT 1000

int
main(int argc, const char *argv[]) {
  const char *me;
  size_t vi, ii, qvalLen;
  Nrrd *nval, *nhist, *nimg;
  double aa, bb, *val;
  airArray *mop;

  AIR_UNUSED(argc);
  me = argv[0];
  mop = airMopNew();

  qvalLen = 10*BINS;
  nrrdAlloc_va(nval=nrrdNew(), nrrdTypeDouble, 1, 4*qvalLen);
  airMopAdd(mop, nval, (airMopper)nrrdNuke, airMopAlways);
  val = AIR_CAST(double*, nval->data);

  airSrandMT(999);
  vi = 0;
  for (ii=0; ii<qvalLen; ii++) {
    airNormalRand(&aa, NULL);
    val[vi++] = aa;
  }
  for (ii=0; ii<qvalLen; ii++) {
    airNormalRand(NULL, &bb);
    val[vi++] = bb;
  }
  for (ii=0; ii<qvalLen; ii++) {
    airNormalRand(&aa, &bb);
    val[vi++] = aa;
    val[vi++] = bb;
  }
  
  nrrdHisto(nhist=nrrdNew(), nval, NULL, NULL, BINS, nrrdTypeInt);
  airMopAdd(mop, nhist, (airMopper)nrrdNuke, airMopAlways);
  nrrdHistoDraw(nimg=nrrdNew(), nhist, HGHT, AIR_TRUE, 0.0);
  airMopAdd(mop, nimg, (airMopper)nrrdNuke, airMopAlways);

  nrrdSave("hist.pgm", nimg, NULL);
  
  airMopOkay(mop);
  return 0;
}
