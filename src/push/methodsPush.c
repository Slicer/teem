/*
  Teem: Gordon Kindlmann's research software
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

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


#include "push.h"
#include "privatePush.h"

pushContext *
pushContextNew(void) {
  pushContext *pctx;
  int sidx;

  pctx = (pushContext *)calloc(1, sizeof(pushContext));
  if (pctx) {
    pctx->nin = NULL;
    for (sidx=0; sidx<PUSH_STAGE_MAX; sidx++) {
      pctx->process[sidx] = _pushProcessDummy;
    }
    pctx->nin3D = NULL;
    pctx->nmask = NULL;
    pctx->nPosVel = nrrdNew();
    pctx->nVelAcc = nrrdNew();
    pctx->gctx = NULL;
    pctx->task = NULL;
    pctx->batchMutex = NULL;
    pctx->stageBarrierA = NULL;
    pctx->stageBarrierB = NULL;
    pctx->noutPos = NULL;
    pctx->noutTen = NULL;
  }
  return pctx;
}

pushContext *
pushContextNix(pushContext *pctx) {
  
  if (pctx) {
    pctx->nPosVel = nrrdNuke(pctx->nPosVel);
    pctx->nVelAcc = nrrdNuke(pctx->nVelAcc);
    free(pctx);
  }
  return NULL;
}

