/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

/*
** learned: valgrind is sometimes far off when reporting the line
** number for an invalid read- have to comment out various lines in
** order to find the real offending line
*/

#include "alan.h"

int
_alanCheck(alanContext *actx) {
  char me[]="alanCheck", err[AIR_STRLEN_MED];

  if (!actx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(ALAN, err); return 1;
  }
  if (0 == actx->dim) {
    sprintf(err, "%s: dimension of texture not set", me);
    biffAdd(ALAN, err); return 1;
  }
  if (alanTextureTypeUnknown == actx->textureType) {
    sprintf(err, "%s: texture type not set", me);
    biffAdd(ALAN, err); return 1;
  }
  if (!( actx->size[0] > 0 && actx->size[1] > 0
	 && (2 == actx->dim || actx->size[2] > 0) )) {
    sprintf(err, "%s: texture sizes invalid", me);
    biffAdd(ALAN, err); return 1;
  }
  if (0 == actx->speed) {
    sprintf(err, "%s: speed == 0", me);
    biffAdd(ALAN, err); return 1;
  }

  return 0;
}

int
alanUpdate(alanContext *actx) {
  char me[]="alanUpdate", err[AIR_STRLEN_MED];
  int ret;

  if (_alanCheck(actx)) {
    sprintf(err, "%s: ", me);
    biffAdd(ALAN, err); return 1;
  }
  if (actx->nlev[0] || actx->nlev[0]) {
    sprintf(err, "%s: confusion: nlev[0,1] already allocated?", me);
    biffAdd(ALAN, err); return 1;
  }
  actx->nlev[0] = nrrdNew();
  actx->nlev[1] = nrrdNew();
  if (2 == actx->dim) {
    ret = (nrrdMaybeAlloc(actx->nlev[0], alan_nt, 3,
			  4, actx->size[0], actx->size[1])
	   || nrrdCopy(actx->nlev[1], actx->nlev[0]));
  } else {
    ret = (nrrdMaybeAlloc(actx->nlev[0], alan_nt, 4,
			  4, actx->size[0], actx->size[1], actx->size[2])
	   || nrrdCopy(actx->nlev[1], actx->nlev[0]));
  }
  if (ret) {
    sprintf(err, "%s: trouble allocating buffers", me);
    biffMove(ALAN, err, NRRD); return 1;
  }
  
  return 0;
}

int 
alanInit(alanContext *actx, const Nrrd *ninit) {
  char me[]="alanInit", err[AIR_STRLEN_MED];
  alan_t *init=NULL, *lev0, *lev1;
  size_t I, N;

  if (_alanCheck(actx)) {
    sprintf(err, "%s: ", me);
    biffAdd(ALAN, err); return 1;
  }
  if (!( actx->nlev[0] && actx->nlev[0] )) {
    sprintf(err, "%s: nlev[0,1] not allocated: call alanUpdate", me);
    biffAdd(ALAN, err); return 1;
  }
  
  if (ninit) {
    if (nrrdCheck(ninit)) {
      sprintf(err, "%s: given ninit has problems", me);
      biffMove(ALAN, err, NRRD); return 1;
    }
    if (!( alan_nt == ninit->type 
	   && ninit->dim == 1 + actx->dim
	   && actx->nlev[0]->axis[0].size == ninit->axis[0].size
	   && actx->size[0] == ninit->axis[1].size
	   && actx->size[1] == ninit->axis[2].size 
	   && (2 == actx->dim || actx->size[2] == ninit->axis[3].size) )) {
      sprintf(err, "%s: type/size mismatch with given ninit", me);
      biffAdd(ALAN, err); return 1;
    }
    init = (alan_t*)(ninit->data);
  }

#define RRR AIR_AFFINE(0, airRand(), 1, -actx->randRange, actx->randRange)

  N = nrrdElementNumber(actx->nlev[0])/actx->nlev[0]->axis[0].size;
  lev0 = (alan_t*)(actx->nlev[0]->data);
  lev1 = (alan_t*)(actx->nlev[1]->data);
  for (I=0; I<N; I++) {
    if (init) {
      lev0[0 + 4*I] = init[0 + 4*I];
      lev0[1 + 4*I] = init[1 + 4*I];
      lev0[2 + 4*I] = init[2 + 4*I];
      lev0[3 + 4*I] = init[3 + 4*I];
    } else {
      lev0[0 + 4*I] = actx->initA + RRR;
      lev0[1 + 4*I] = actx->initB + RRR;
      lev0[2 + 4*I] = actx->alpha;
      lev0[3 + 4*I] = actx->beta;
    }
  }
  return 0;
}

/*
**  0 1 2
**  3 4 5
**  6 7 8
*/

int
_alanTuring2DIter(alanContext *actx) {
  alan_t *lev0, *lev1, *v[9], lapA, lapB, deltaA, deltaB, diffA, diffB;
  int idx, px, mx, py, my, sx, sy, x, y;

  sx = actx->size[0];
  sy = actx->size[1];
  lev0 = (alan_t*)(actx->nlev[actx->iter % 2]->data);
  lev1 = (alan_t*)(actx->nlev[(actx->iter+1) % 2]->data);

  diffA = actx->diffA/(actx->H*actx->H);
  diffB = actx->diffB/(actx->H*actx->H);
  actx->tada = 0;
  for (y=0; y<sy; y++) {
    py = AIR_MOD(y+1, sy);
    my = AIR_MOD(y-1, sy);
    for (x=0; x<sx; x++) {
      idx = x + sx*(y);
      px = AIR_MOD(x+1, sx);
      mx = AIR_MOD(x-1, sx);
      v[0] = lev0 + 4*(mx + sx*(my));
      v[1] = lev0 + 4*( x + sx*(my));
      v[2] = lev0 + 4*(px + sx*(my));
      v[3] = lev0 + 4*(mx + sx*( y));
      v[4] = lev0 + 4*( x + sx*( y));
      v[5] = lev0 + 4*(px + sx*( y));
      v[6] = lev0 + 4*(mx + sx*(py));
      v[7] = lev0 + 4*( x + sx*(py));
      v[8] = lev0 + 4*(px + sx*(py));
      lapA = v[1][0] + v[3][0] + v[5][0] + v[7][0] - 4*v[4][0];
      lapB = v[1][1] + v[3][1] + v[5][1] + v[7][1] - 4*v[4][1];
      
      deltaA = actx->K*(v[4][2] - v[4][0]*v[4][1]) + diffA*lapA;
      if (AIR_ABS(deltaA) > actx->maxAda) {
	return alanStopDiverged;
      }
      actx->tada += AIR_ABS(deltaA);
      deltaB = actx->K*(v[4][0]*v[4][1] - v[4][1] - v[4][3]) + diffB*lapB;
      if (!( AIR_EXISTS(deltaA) && AIR_EXISTS(deltaB) )) {
	return alanStopNonExist;
      }
      
      lev1[0 + 4*idx] = actx->speed*deltaA + lev0[0 + 4*idx];
      lev1[1 + 4*idx] = actx->speed*deltaB + lev0[1 + 4*idx];
      lev1[1 + 4*idx] = AIR_MAX(0, lev1[1 + 4*idx]);
      lev1[2 + 4*idx] = lev0[2 + 4*idx];
      lev1[3 + 4*idx] = lev0[3 + 4*idx];
    }
  }
  actx->tada *= 1.0/(sx*sy);
  if (actx->tada < actx->minTada) {
    return alanStopConverged;
  }
  return alanStopNot;
}

int
_alanGrayScott2DIter(alanContext *actx) {
  
  return alanStopNot;
}

int
_alanTuring3DIter(alanContext *actx) {
  
  return alanStopNot;
}

int
_alanGrayScott3DIter(alanContext *actx) {
  
  return alanStopNot;
}

int
alanRun(alanContext *actx) {
  char me[]="alanRun", err[AIR_STRLEN_MED], fname[AIR_STRLEN_MED];
  int stop, (*iter)(alanContext *)=NULL;
  Nrrd *nslc, *nimg;
  airArray *mop;

  if (_alanCheck(actx)) {
    sprintf(err, "%s: ", me);
    biffAdd(ALAN, err); return 1;
  }
  if (!( actx->nlev[0] && actx->nlev[0] )) {
    sprintf(err, "%s: nlev[0,1] not allocated: "
	    "call alanUpdate + alanInit", me);
    biffAdd(ALAN, err); return 1;
  }
  
  switch(actx->textureType) {
  case alanTextureTypeTuring:
    iter = (2 == actx->dim 
	    ? _alanTuring2DIter 
	    : _alanTuring3DIter);
    break;
  case alanTextureTypeGrayScott:
    iter = (2 == actx->dim 
	    ? _alanGrayScott2DIter 
	    : _alanGrayScott3DIter);
    break;
  }
  
  mop = airMopNew();
  nslc = nrrdNew();
  nimg = nrrdNew();
  airMopAdd(mop, nslc, (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nimg, (airMopper)nrrdNuke, airMopAlways);
  for (actx->iter=0; actx->iter<actx->maxIteration; actx->iter++) {
    if (actx->saveInterval && !(actx->iter % actx->saveInterval)) {
      fprintf(stderr, "%s: iter = %d, tada = %g\n",
	      me, actx->iter, actx->tada);
      sprintf(fname, "%06d.nrrd", actx->iter);
      nrrdSave(fname, actx->nlev[actx->iter % 2], NULL);
    }
    if (actx->frameInterval && !(actx->iter % actx->frameInterval)) {
      nrrdSlice(nslc, actx->nlev[actx->iter % 2], 0, 0);
      nrrdQuantize(nimg, nslc, NULL, 8);
      sprintf(fname, "%06d.png", actx->iter);
      nrrdSave(fname, nimg, NULL);
    }
    stop = iter(actx);
    if (alanStopNot != stop) {
      actx->stop = stop;
      return 0;
    }
  }
  actx->stop = alanStopMaxIteration;
  
  airMopOkay(mop);
  return 0;
}
