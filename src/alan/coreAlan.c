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
  actx->nparm = nrrdNew();
  if (2 == actx->dim) {
    ret = (nrrdMaybeAlloc(actx->nlev[0], alan_nt, 3,
			  2, actx->size[0], actx->size[1])
	   || nrrdCopy(actx->nlev[1], actx->nlev[0])
	   || nrrdMaybeAlloc(actx->nparm, alan_nt, 3,
			     3, actx->size[0], actx->size[1]));
  } else {
    ret = (nrrdMaybeAlloc(actx->nlev[0], alan_nt, 4,
			  2, actx->size[0], actx->size[1], actx->size[2])
	   || nrrdCopy(actx->nlev[1], actx->nlev[0])
	   || nrrdMaybeAlloc(actx->nparm, alan_nt, 4,
			     3, actx->size[0], actx->size[1], actx->size[2]));
  }
  if (ret) {
    sprintf(err, "%s: trouble allocating buffers", me);
    biffMove(ALAN, err, NRRD); return 1;
  }
  
  return 0;
}

int 
alanInit(alanContext *actx, const Nrrd *nlevInit, const Nrrd *nparmInit) {
  char me[]="alanInit", err[AIR_STRLEN_MED];
  alan_t *levInit=NULL, *lev0, *parmInit=NULL, *parm;
  size_t I, N;

  if (_alanCheck(actx)) {
    sprintf(err, "%s: ", me);
    biffAdd(ALAN, err); return 1;
  }
  if (!( actx->nlev[0] && actx->nlev[0] && actx->nparm )) {
    sprintf(err, "%s: nlev[0,1] not allocated: call alanUpdate", me);
    biffAdd(ALAN, err); return 1;
  }
  
  if (nlevInit) {
    if (nrrdCheck(nlevInit)) {
      sprintf(err, "%s: given nlevInit has problems", me);
      biffMove(ALAN, err, NRRD); return 1;
    }
    if (!( alan_nt == nlevInit->type 
	   && nlevInit->dim == 1 + actx->dim
	   && actx->nlev[0]->axis[0].size == nlevInit->axis[0].size
	   && actx->size[0] == nlevInit->axis[1].size
	   && actx->size[1] == nlevInit->axis[2].size 
	   && (2 == actx->dim || actx->size[2] == nlevInit->axis[3].size) )) {
      sprintf(err, "%s: type/size mismatch with given nlevInit", me);
      biffAdd(ALAN, err); return 1;
    }
    levInit = (alan_t*)(nlevInit->data);
  }
  if (nparmInit) {
    if (nrrdCheck(nparmInit)) {
      sprintf(err, "%s: given nparmInit has problems", me);
      biffMove(ALAN, err, NRRD); return 1;
    }
    if (!( alan_nt == nparmInit->type 
	   && nparmInit->dim == 1 + actx->dim
	   && 3 == nparmInit->axis[0].size
	   && actx->size[0] == nparmInit->axis[1].size
	   && actx->size[1] == nparmInit->axis[2].size 
	   && (2 == actx->dim || actx->size[2] == nparmInit->axis[3].size) )) {
      sprintf(err, "%s: type/size mismatch with given nparmInit", me);
      biffAdd(ALAN, err); return 1;
    }
    parmInit = (alan_t*)(nparmInit->data);
  }

#define RAND AIR_AFFINE(0, airRand(), 1, -actx->randRange, actx->randRange)

  N = nrrdElementNumber(actx->nlev[0])/actx->nlev[0]->axis[0].size;
  lev0 = (alan_t*)(actx->nlev[0]->data);
  parm = (alan_t*)(actx->nparm->data);
  for (I=0; I<N; I++) {
    if (levInit) {
      lev0[0 + 2*I] = levInit[0 + 2*I];
      lev0[1 + 2*I] = levInit[1 + 2*I];
    } else {
      lev0[0 + 2*I] = actx->initA + RAND;
      lev0[1 + 2*I] = actx->initB + RAND;
    }
    if (parmInit) {
      parm[0 + 3*I] = parmInit[0 + 3*I];
      parm[1 + 3*I] = parmInit[1 + 3*I];
      parm[2 + 3*I] = parmInit[2 + 3*I];
    } else {
      parm[0 + 3*I] = actx->speed;
      parm[1 + 3*I] = actx->alpha;
      parm[2 + 3*I] = actx->beta;
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
  alan_t *lev0, *lev1, *parm, speed, alpha, beta, A, B,
    *v[9], lapA, lapB, deltaA, deltaB, diffA, diffB;
  int idx, px, mx, py, my, sx, sy, x, y;

  sx = actx->size[0];
  sy = actx->size[1];
  lev0 = (alan_t*)(actx->nlev[actx->iter % 2]->data);
  lev1 = (alan_t*)(actx->nlev[(actx->iter+1) % 2]->data);
  parm = (alan_t*)(actx->nparm->data);

  diffA = actx->diffA/(actx->H*actx->H);
  diffB = actx->diffB/(actx->H*actx->H);
  actx->tada = 0;
  for (y=0; y<sy; y++) {
    py = AIR_MOD(y+1, sy);
    my = AIR_MOD(y-1, sy);
    for (x=0; x<sx; x++) {
      px = AIR_MOD(x+1, sx);
      mx = AIR_MOD(x-1, sx);
      idx = x + sx*(y);
      A = lev0[0 + 2*idx];
      B = lev0[1 + 2*idx];
      speed = parm[0 + 3*idx];
      alpha = parm[1 + 3*idx];
      beta = parm[2 + 3*idx];
      v[0] = lev0 + 2*(mx + sx*(my));
      v[1] = lev0 + 2*( x + sx*(my));
      v[2] = lev0 + 2*(px + sx*(my));
      v[3] = lev0 + 2*(mx + sx*( y));
      v[5] = lev0 + 2*(px + sx*( y));
      v[6] = lev0 + 2*(mx + sx*(py));
      v[7] = lev0 + 2*( x + sx*(py));
      v[8] = lev0 + 2*(px + sx*(py));
      lapA = v[1][0] + v[3][0] + v[5][0] + v[7][0] - 4*A;
      lapB = v[1][1] + v[3][1] + v[5][1] + v[7][1] - 4*B;
      
      deltaA = actx->K*(alpha - A*B) + diffA*lapA;
      if (AIR_ABS(deltaA) > actx->maxAda) {
	return alanStopDiverged;
      }
      actx->tada += AIR_ABS(deltaA);
      deltaB = actx->K*(A*B - B - beta) + diffB*lapB;
      if (!( AIR_EXISTS(deltaA) && AIR_EXISTS(deltaB) )) {
	return alanStopNonExist;
      }

      A += speed*deltaA;
      B += speed*deltaB;  
      B = AIR_MAX(0, B);
      lev1[0 + 2*idx] = A;
      lev1[1 + 2*idx] = B; 
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
