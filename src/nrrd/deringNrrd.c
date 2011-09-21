/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2011, 2010, 2009  University of Chicago
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

#include "nrrd.h"
#include "privateNrrd.h"

enum {
  flagUnknown,         /*  0 */
  flagInput,           /*  1 */
  flagCenter,          /*  2 */
  flagLast
};
#define FLAG_MAX           2


NrrdDeringContext *
nrrdDeringContextNew(void) {
  NrrdDeringContext *drc;

  drc = AIR_CALLOC(1, NrrdDeringContext);
  if (!drc) {
    return NULL;
  }
  drc->nin = NULL;
  drc->center[0] = AIR_NAN;
  drc->center[1] = AIR_NAN;
  drc->flag = AIR_CALLOC(FLAG_MAX+1, int); /* will be set to zero=false */
  if (!(drc->flag)) {
    free(drc);
    return NULL;
  }
  drc->nsliceOrig = NULL;
  drc->nslice = NULL;
  
  return drc;
}

NrrdDeringContext *
nrrdDeringContextNix(NrrdDeringContext *drc) {

  if (drc) {
    airFree(drc->flag);
    nrrdNix(drc->nsliceOrig);
    nrrdNuke(drc->nslice);
    free(drc);
  }
  return NULL;
}

int
nrrdDeringInputSet(NrrdDeringContext *drc, const Nrrd *nin) {
  static const char me[]="nrrdDeringInputSet";
  
  if (!( drc && nin )) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  if (nrrdCheck(nin)) {
    biffAddf(NRRD, "%s: problems with given nrrd", me);
    return 1;
  }
  if (nrrdTypeBlock == nin->type) {
    biffAddf(NRRD, "%s: can't resample from type %s", me,
             airEnumStr(nrrdType, nrrdTypeBlock));
    return 1;
  }

  drc->nin = nin;
  drc->flag[flagInput] = AIR_TRUE;

  return 0;
}

int
nrrdDeringCenterSet(NrrdDeringContext *drc, double cx, double cy) {
  static const char me[]="nrrdDeringCenterSet";

  if (!drc) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  if (!( AIR_EXISTS(cx) && AIR_EXISTS(cy) )) {
    biffAddf(NRRD, "%s: center (%g,%g) doesn't exist", me, cx, cy);
    return 1;
  }
  
  drc->center[0] = cx;
  drc->center[1] = cy;
  drc->flag[flagCenter] = AIR_TRUE;

  return 0;
}

int
nrrdDeringExecute(NrrdDeringContext *drc, Nrrd *nout) {
  static const char me[]="nrrdDeringExecute";

  if (!( drc && nout )) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  

  return 0;
}
