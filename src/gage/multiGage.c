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

#include "gage.h"
#include "privateGage.h"

/*
** does Not use biff
*/
gageMultiItem *
gageMultiItemNew(const gageKind *kind) {
  gageMultiItem *gmi = NULL;

  if (kind && (gmi = AIR_CALLOC(1, gageMultiItem))) {
    gmi->kind = kind;
    gmi->nans = nrrdNew();
  }
  return gmi;
}

gageMultiItem *
gageMultiItemNix(gageMultiItem *gmi) {

  airFree(gmi);
  return NULL;
}

gageMultiItem *
gageMultiItemNuke(gageMultiItem *gmi) {

  if (gmi) {
    nrrdNuke(gmi->nans);
    airFree(gmi);
  }
  return NULL;
}

int
gageMultiItemSet_va(gageMultiItem *gmi, unsigned int itemNum,
                    ... /* itemNum items */) {

  AIR_UNUSED(gmi);
  AIR_UNUSED(itemNum);
  return 0;
}




gageMultiQuery *
gageMultiQueryNew(gageContext *gctx) {

  AIR_UNUSED(gctx);
  return NULL;
}

int
gageMultiQueryAdd_va(gageMultiQuery *gmq, unsigned int pvlIdx,
                     unsigned int queryNum,
                     ... /* queryNum gageMultiItem* */) {
  
  AIR_UNUSED(gmq);
  AIR_UNUSED(pvlIdx);
  AIR_UNUSED(queryNum);
  return 0;
}

int
gageMultiProbe(gageMultiQuery *gmq, gageContext *gctx,
               const Nrrd *npos) {

  AIR_UNUSED(gmq);
  AIR_UNUSED(gctx);
  AIR_UNUSED(npos);
  return 0;
}

int
gageMultiProbeSpace(gageMultiQuery *gmq, gageContext *gctx,
                    const Nrrd *npos, int indexSpace, int clamp) {

  AIR_UNUSED(gmq);
  AIR_UNUSED(gctx);
  AIR_UNUSED(npos);
  AIR_UNUSED(indexSpace);
  AIR_UNUSED(clamp);
  return 0;
}

gageMultiQuery *
gageMultiQueryNix(gageMultiQuery *gmq) {

  AIR_UNUSED(gmq);
  return NULL;
}

gageMultiQuery *
gageMultiQueryNuke(gageMultiQuery *gmq) {

  AIR_UNUSED(gmq);
  return NULL;
}
