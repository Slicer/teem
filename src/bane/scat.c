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


#include "bane.h"

int
baneRawScatterplots(Nrrd *nvg, Nrrd *nvh, Nrrd *hvol, int histEq) {
  Nrrd *gA, *hA, *gB, *hB;
  char me[]="baneRawScatterplots", err[AIR_STRLEN_MED];
  int E;
  
  if (!( nvg && nvh && hvol )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(BANE, err); return 1;
  }
  if (!baneValidHVol(hvol)) {
    sprintf(err, "%s: didn't get a valid histogram volume", me);
    biffAdd(BANE, err); return 1;
  }

  /* create initial projections */
  E = 0;
  if (!E) E |= nrrdProject(gA = nrrdNew(), hvol, 1, nrrdMeasureSum);
  if (!E) E |= nrrdProject(hA = nrrdNew(), hvol, 0, nrrdMeasureSum);
  if (E) {
    sprintf(err, "%s: trouble creating raw scatterplots", me);
    biffMove(BANE, err, NRRD); return 1;
  }

  /* do histogram equalization on them */
  if (histEq) {
    if (!E) E |= nrrdHistoEq(gA, NULL, BANE_HIST_EQ_BINS, 1);
    if (!E) E |= nrrdHistoEq(hA, NULL, BANE_HIST_EQ_BINS, 1);
    if (E) {
      sprintf(err, "%s: couldn't histogram equalize scatterplots", me);
      biffMove(BANE, err, NRRD); return 1;
    }
  }

  /* re-orient them so they look correct on the screen */
  if (!E) E |= nrrdSwapAxes(gB = nrrdNew(), gA, 0, 1);
  if (!E) E |= nrrdSwapAxes(hB = nrrdNew(), hA, 0, 1);
  if (!E) E |= nrrdFlip(gA, gB, 1);
  if (!E) E |= nrrdFlip(hA, hB, 1);
  if (E) {
    sprintf(err, "%s: couldn't re-orient scatterplots", me);
    biffMove(BANE, err, NRRD); return 1;
  }
  
  if (!E) E |= nrrdCopy(nvg, gA);
  if (!E) E |= nrrdCopy(nvh, hA);
  if (E) {
    sprintf(err, "%s: trouble saving results to given nrrds", me);
    biffMove(BANE, err, NRRD); return 1;
  }

  return 0;
}
