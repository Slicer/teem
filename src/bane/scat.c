/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/


#include "bane.h"

int
baneRawScatterplots(Nrrd *nvg, Nrrd *nvh, Nrrd *hvol, int histEq) {
  Nrrd *gA, *hA, *gB, *hB;
  char me[]="baneRawScatterplots", err[512];
  int E;
  
  if (!( nvg && nvh && hvol )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(BANE, err); return 1;
  }
  if (!baneValidHVol(hvol)) {
    sprintf(err, "%s: didn't get a valid histogram volume", me);
    biffAdd(BANE, err); return 1;
  }

  /* create initial projections */
  E = 0;
  if (!E) E |= nrrdMeasureAxis(gA = nrrdNew(), hvol, 1, nrrdMeasureSum);
  if (!E) E |= nrrdMeasureAxis(hA = nrrdNew(), hvol, 0, nrrdMeasureSum);
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
