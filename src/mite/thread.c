/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

#include "mite.h"
#include "privateMite.h"

miteThread *
miteThreadNew() {
  miteThread *mtt;
  
  mtt = (miteThread *)calloc(1, sizeof(miteThread));
  if (!mtt) {
    return NULL;
  }

  mtt->gctx = NULL;
  mtt->ansScl = mtt->ansVec = mtt->ansTen = NULL;
  mtt->shadeVec0 = mtt->shadeVec1 = NULL;
  mtt->shadeScl0 = mtt->shadeScl1 = NULL;
  mtt->ansMiteVal = 
    (gage_t *)calloc(gageKindTotalAnswerLength(miteValGageKind), 
		     sizeof(gage_t));
  mtt->verbose = 0;
  mtt->thrid = -1;
  mtt->ui = mtt->vi = -1;
  mtt->samples = 0;
  mtt->stage = NULL;
  /* mtt->range[], rayStep, V, RR, GG, BB, TT  initialized in 
     miteRayBegin or in miteSample */
  
  return mtt;
}

miteThread *
miteThreadNix(miteThread *mtt) {

  AIR_FREE(mtt->ansMiteVal);
  AIR_FREE(mtt);
  return NULL;
}

/*
******** miteThreadBegin()
**
** this has some of the body of what would be miteThreadInit
*/
int 
miteThreadBegin(miteThread **mttP, miteRender *mrr,
		miteUser *muu, int whichThread) {
  char me[]="miteThreadBegin", err[AIR_STRLEN_MED];

  /* all the miteThreads have already been allocated */
  (*mttP) = mrr->tt[whichThread];

  if (!whichThread) {
    /* this is the first thread- it just points to the parent gageContext */
    (*mttP)->gctx = muu->gctx0;
  } else {
    /* we have to generate a new gageContext */
    (*mttP)->gctx = gageContextCopy(muu->gctx0);
    if (!(*mttP)->gctx) {
      sprintf(err, "%s: couldn't set up thread %d", me, whichThread);
      biffMove(MITE, err, GAGE); return 1;
    }
  }

  (*mttP)->ansScl = (-1 != mrr->sclPvlIdx
		     ? (*mttP)->gctx->pvl[mrr->sclPvlIdx]->answer
		     : NULL);
  (*mttP)->ansVec = (-1 != mrr->vecPvlIdx
		     ? (*mttP)->gctx->pvl[mrr->vecPvlIdx]->answer
		     : NULL);
  (*mttP)->ansTen = (-1 != mrr->tenPvlIdx
		     ? (*mttP)->gctx->pvl[mrr->tenPvlIdx]->answer
		     : NULL);

#if 0
  (*mttP)->nPerp = (*mttP)->ans + gageKindScl->ansOffset[gageSclNPerp];
  (*mttP)->gten = (*mttP)->ans + gageKindScl->ansOffset[gageSclGeomTens];
#endif

  (*mttP)->thrid = whichThread;
  (*mttP)->samples = 0;
  (*mttP)->verbose = 0;
  
  /* set up shading answers */
  switch(mrr->shpec->method) {
  case miteShadeMethodNone:
    /* nothing to do */
    break;
  case miteShadeMethodPhong:
    (*mttP)->shadeVec0 = _miteAnswerPointer(*mttP, mrr->shpec->vec0);
    break;
  case miteShadeMethodLitTen:
    (*mttP)->shadeVec0 = _miteAnswerPointer(*mttP, mrr->shpec->vec0);
    (*mttP)->shadeVec1 = _miteAnswerPointer(*mttP, mrr->shpec->vec1);
    (*mttP)->shadeScl0 = _miteAnswerPointer(*mttP, mrr->shpec->scl0);
    (*mttP)->shadeScl1 = _miteAnswerPointer(*mttP, mrr->shpec->scl1);
    break;
  default:
    sprintf(err, "%s: shade method %d not implemented!",
	    me, mrr->shpec->method);
    biffAdd(MITE, err); return 1;
    break;
  }

  if (_miteStageSet(*mttP, mrr)) {
    sprintf(err, "%s: trouble setting up stage array", me);
    biffAdd(MITE, err); return 1;
  }
  return 0;
}

int 
miteThreadEnd(miteThread *mtt, miteRender *mrr,
	      miteUser *muu) {

  return 0;
}

