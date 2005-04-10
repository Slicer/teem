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
  int ii;

  pctx = (pushContext *)calloc(1, sizeof(pushContext));
  if (pctx) {
    pctx->nin = NULL;
    pctx->npos = NULL;
    pctx->mass = 1.0;
    pctx->scale = 0.2;
    pctx->margin = 0.3;
    pctx->seed = 42;
    for (ii=0; ii<PUSH_STAGE_MAX; ii++) {
      pctx->process[ii] = _pushProcessDummy;
    }
    pctx->ksp00 = nrrdKernelSpecNew();
    pctx->ksp11 = nrrdKernelSpecNew();
    pctx->nten = NULL;
    pctx->nmask = NULL;
    pctx->nPointAttr = nrrdNew();
    pctx->nVelAcc = nrrdNew();
    pctx->gctx = NULL;
    pctx->tenAns = NULL;
    pctx->cntAns = NULL;
    pctx->pidx = NULL;
    pctx->pidxArr = NULL;
    pctx->task = NULL;
    pctx->binMutex = NULL;
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
    pctx->nPointAttr = nrrdNuke(pctx->nPointAttr);
    pctx->nVelAcc = nrrdNuke(pctx->nVelAcc);
    pctx->ksp00 = nrrdKernelSpecNix(pctx->ksp00);
    pctx->ksp11 = nrrdKernelSpecNix(pctx->ksp11);
    free(pctx);
  }
  return NULL;
}

