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
    gmi->item = NULL;
    gmi->ansDir = NULL;
    gmi->ansLen = NULL;
    gmi->nans = nrrdNew();
  }
  return gmi;
}

gageMultiItem *
gageMultiItemNix(gageMultiItem *gmi) {

  if (gmi) {
    airFree(gmi->item);
    airFree(gmi);
  }
  return NULL;
}

gageMultiItem *
gageMultiItemNuke(gageMultiItem *gmi) {

  if (gmi) {
    nrrdNuke(gmi->nans);
  }
  return gageMultiItemNix(gmi);
}

/*
******** gageMultiItemSet_va
**
** How to set (in one call) the multiple items in a gageMultiItem.
**
** does use biff
*/
int
gageMultiItemSet_va(gageMultiItem *gmi, unsigned int itemNum,
                    ... /* itemNum items */) {
  static const char me[]="gageMultiItemSet_va";
  unsigned int ii;
  va_list ap;

  if (!gmi) {
    biffAddf(GAGE, "%s: got NULL pointer", me);
    return 1;
  }
  if (!itemNum) {
    biffAddf(GAGE, "%s: can't currently set zero items", me);
    return 1;
  }
  if (!( gmi->item = AIR_CALLOC(itemNum, int) )) {
    biffAddf(GAGE, "%s: couldn't allocate %u ints for items", me, itemNum);
    return 1;
  }
  
  /* consume and check items */
  va_start(ap, itemNum);
  for (ii=0; ii<itemNum; ii++) {
    gmi->item[ii] = va_arg(ap, int);
  }
  va_end(ap);

  for (ii=0; ii<itemNum; ii++) {
    if (airEnumValCheck(gmi->kind->enm, gmi->item[ii])) {
      biffAddf(GAGE, "%s: item[%u] %d not a valid %s value", me,
               ii, gmi->item[ii], gmi->kind->enm->name);
      return 1;
    }
  }
  
  return 0;
}

/* ----------------------------------------------------------- */

gageMultiQuery *
gageMultiQueryNew(const gageContext *gctx) {
  gageMultiQuery *gmq = NULL;

  if (gctx && (gmq = AIR_CALLOC(1, gageMultiQuery))) {
    gmq->pvlNum = gctx->pvlNum;
    gmq->queryNum = AIR_CALLOC(gmq->pvlNum, unsigned int);
    gmq->query = AIR_CALLOC(gmq->pvlNum, gageMultiItem **);
    gmq->nidx = nrrdNew();
    if (!( gmq->queryNum && gmq->query && gmq->nidx )) {
      /* bail */
      airFree(gmq->queryNum);
      airFree(gmq->query);
      nrrdNuke(gmq->nidx);
      airFree(gmq);
    } else {
      unsigned int qi;

    }
  }
  return gmq;
}

/*
******** gageMultiQueryAdd_va
**
** add multi-items for one particular pvl
**
** does use biff
*/
int
gageMultiQueryAdd_va(gageMultiQuery *gmq, unsigned int pvlIdx,
                     unsigned int queryNum,
                     ... /* queryNum gageMultiItem* */) {
  static const char me[]="gageMultiQueryAdd_va";
  unsigned int qi;
  va_list ap;

  if (!gmq) {
    biffAddf(GAGE, "%s: got NULL pointer", me);
    return 1;
  }
  if (!( pvlIdx < gmq->pvlNum )) {
    biffAddf(GAGE, "%s: pvlIdx %u not in valid range [0,%u]", me,
             pvlIdx, gmq->pvlNum-1);
    return 1;
  }
  
  gmq->queryNum[pvlIdx] = queryNum;
  gmq->query[pvlIdx] = AIR_CALLOC(queryNum, gageMultiItem*);
  /* consume and check item s*/
  va_start(ap, queryNum);
  for (qi=0; qi<queryNum; qi++) {
    gmq->query[pvlIdx][qi] = va_arg(ap, gageMultiItem*);
  }
  va_end(ap);
  
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
