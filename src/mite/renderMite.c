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

miteShadeSpec *
miteShadeSpecNew(void) {
  miteShadeSpec *shpec;

  shpec = (miteShadeSpec *)calloc(1, sizeof(miteShadeSpec));
  if (shpec) {
    shpec->shadeMethod = miteShadeMethodUnknown;
    shpec->vec0 = gageQuerySpecNew();
    shpec->vec1 = gageQuerySpecNew();
    shpec->scl0 = gageQuerySpecNew();
    shpec->scl1 = gageQuerySpecNew();
    if (!( shpec->vec0 && shpec->vec1 && 
	   shpec->scl0 && shpec->scl1 )) {
      return NULL;
    }
  }
  return shpec;
}

miteShadeSpec *
miteShadeSpecNix(miteShadeSpec *shpec) {

  if (shpec) {
    shpec->vec0 = gageQuerySpecNix(shpec->vec0);
    shpec->vec1 = gageQuerySpecNix(shpec->vec1);
    shpec->scl0 = gageQuerySpecNix(shpec->scl0);
    shpec->scl1 = gageQuerySpecNix(shpec->scl1);
    AIR_FREE(shpec);
  }
  return NULL;
}

miteRender *
_miteRenderNew(void) {
  miteRender *mrr;

  mrr = (miteRender *)calloc(1, sizeof(miteRender));
  if (mrr) {
    mrr->rmop = airMopNew();
    if (!mrr->rmop) {
      AIR_FREE(mrr);
      return mrr;
    }
    mrr->shpec = miteShadeSpecNew();
    airMopAdd(mrr->rmop, mrr->shpec,
	      (airMopper)miteShadeSpecNix, airMopAlways);
  }
  return mrr;
}

miteRender *
_miteRenderNix(miteRender *mrr) {
  
  if (mrr) {
    airMopOkay(mrr->rmop);
    AIR_FREE(mrr);
  }
  return NULL;
}

int
miteShadeParse(miteShadeSpec *shpec, char *shadeStr) {
  char me[]="miteShadeParse", err[AIR_STRLEN_MED], *buff, *qstr, *tok, *state;
  airArray *mop;
  int ansLength;

  mop = airMopNew();
  if (!( shpec && airStrlen(shadeStr) )) {
    sprintf(err, "%s: got NULL pointer and/or empty string", me);
    biffAdd(MITE, err); airMopError(mop); return 1;
  }
  buff = airToLower(airStrdup(shadeStr));
  if (!buff) {
    sprintf(err, "%s: couldn't strdup shading spec", me);
    biffAdd(MITE, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, buff, airFree, airMopAlways);
  if (!strcmp("none", buff)) {
    shpec->shadeMethod = miteShadeMethodNone;
  } else if (buff == strstr("phong:", buff)) {
    qstr = buff + strlen("phong:");
    if (miteVariableParse(shpec->vec0, qstr)) {
      sprintf(err, "%s: couldn't parse \"%s\" as shading vector", me, qstr);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
    ansLength = shpec->vec0->kind->ansLength[shpec->vec0->query];
    if (3 != ansLength) {
      sprintf(err, "%s: \"%s\" isn't a vector (answer length is %d, not 3)",
	      me, qstr, ansLength);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
    fprintf(stderr, "!%s: got phong:%s(%s)\n", me,
	    shpec->vec0->kind->name, 
	    airEnumStr(shpec->vec0->kind->enm, shpec->vec0->query));
  } else if (buff == strstr("litten:", buff)) {
    qstr = buff + strlen("litten:");
    /* ---- first vector */
    tok = airStrtok(qstr, ",", &state);
    if (miteVariableParse(shpec->vec0, tok)) {
      sprintf(err, "%s: couldn't parse \"%s\" as lit-tensor vector", me, tok);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
    ansLength = shpec->vec0->kind->ansLength[shpec->vec0->query];
    if (3 != ansLength) {
      sprintf(err, "%s: \"%s\" isn't a vector (answer length is %d, not 3)",
	      me, qstr, ansLength);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
    /* ---- second vector */
    tok = airStrtok(qstr, ",", &state);
    if (miteVariableParse(shpec->vec1, tok)) {
      sprintf(err, "%s: couldn't parse \"%s\" as lit-tensor vector", me, tok);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
    ansLength = shpec->vec1->kind->ansLength[shpec->vec1->query];
    if (3 != ansLength) {
      sprintf(err, "%s: \"%s\" isn't a vector (answer length is %d, not 3)",
	      me, qstr, ansLength);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
    /* ---- first scalar */
    tok = airStrtok(qstr, ",", &state);
    if (miteVariableParse(shpec->scl0, tok)) {
      sprintf(err, "%s: couldn't parse \"%s\" as lit-tensor scalar", me, tok);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
    ansLength = shpec->scl0->kind->ansLength[shpec->scl0->query];
    if (1 != ansLength) {
      sprintf(err, "%s: \"%s\" isn't a scalar (answer length is %d, not 1)",
	      me, qstr, ansLength);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
    /* ---- second scalar */
    tok = airStrtok(qstr, ",", &state);
    if (miteVariableParse(shpec->scl1, tok)) {
      sprintf(err, "%s: couldn't parse \"%s\" as lit-tensor scalar", me, tok);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
    ansLength = shpec->scl1->kind->ansLength[shpec->scl1->query];
    if (1 != ansLength) {
      sprintf(err, "%s: \"%s\" isn't a scalar (answer length is %d, not 1)",
	      me, qstr, ansLength);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
    fprintf(stderr, "!%s: got litten:%s(%s),\n", me,
	    shpec->vec0->kind->name, 
	    airEnumStr(shpec->vec0->kind->enm, shpec->vec0->query));
  } else {
    sprintf(err, "%s: shading specification \"%s\" not understood",
	    me, shadeStr);
    biffAdd(MITE, err); airMopError(mop); return 1;
  }
  airMopOkay(mop);
  return 0;
}

int 
miteRenderBegin(miteRender **mrrP, miteUser *muu) {
  char me[]="miteRenderBegin", err[AIR_STRLEN_MED];
  gagePerVolume *pvl;
  int E, T, thr, axi;
  unsigned int queryScl, queryVec, queryTen;
 
  if (!(mrrP && muu)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(MITE, err); return 1;
  }
  if (_miteUserCheck(muu)) {
    sprintf(err, "%s: problem with user-set parameters", me);
    biffAdd(MITE, err); return 1;
  }
  if (!( *mrrP = _miteRenderNew() )) {
    sprintf(err, "%s: couldn't alloc miteRender", me);
    biffAdd(MITE, err); return 1;
  }
  if (_miteNtxfAlphaAdjust(*mrrP, muu)) {
    sprintf(err, "%s: trouble copying and alpha-adjusting txfs", me);
    biffAdd(MITE, err); return 1;
  }

  queryScl = 0;
  queryTen = 0;
  for (T=0; T<muu->ntxfNum; T++) {
    for (axi=1; axi<=muu->ntxf[T]->dim-1; axi++) {
      _miteNtxfQuery(&queryScl, &queryVec, &queryTen,
		     muu->ntxf[T]->axis[axi]->label);
  }
  if (!muu->noDirLight) {
    query |= 1<<gageSclNormal;
  }
  /*
  fprintf(stderr, "!%s: gageScl query: \n", me);
  gageQueryPrint(stderr, gageKindScl, query);
  */

  E = 0;
  if (!muu->nsin) {
    sprintf(err, "%s: sorry- can't proceed without scalar volume", me);
    biffAdd(MITE, err); return 1;
  }
  if (!E) E |= !(pvl = gagePerVolumeNew(muu->gctx0, muu->nsin, gageKindScl));
  if (!E) E |= gagePerVolumeAttach(muu->gctx0, pvl);
  if (!E) E |= gageKernelSet(muu->gctx0, gageKernel00,
			     muu->ksp[gageKernel00]->kernel,
			     muu->ksp[gageKernel00]->parm);
  if (!E) E |= gageKernelSet(muu->gctx0, gageKernel11,
			     muu->ksp[gageKernel11]->kernel,
			     muu->ksp[gageKernel11]->parm);
  if (!E) E |= gageKernelSet(muu->gctx0, gageKernel22,
			     muu->ksp[gageKernel22]->kernel,
			     muu->ksp[gageKernel22]->parm);
  if (!E) E |= gageQuerySet(muu->gctx0, pvl, query);
  if (!E) E |= gageUpdate(muu->gctx0);
  if (E) {
    sprintf(err, "%s: gage trouble", me);
    biffMove(MITE, err, GAGE); return 1;
  }
  fprintf(stderr, "!%s: kernel support = %d^3 samples\n",
	  me, GAGE_FD(muu->gctx0));
  
  if (nrrdMaybeAlloc(muu->nout, mite_nt, 3, 4,
		     muu->hctx->imgSize[0], muu->hctx->imgSize[1])) {
    sprintf(err, "%s: nrrd trouble", me);
    biffMove(MITE, err, NRRD);
    return 1;
  }
  muu->nout->axis[1].center = nrrdCenterCell;
  muu->nout->axis[1].min = muu->hctx->cam->uRange[0];
  muu->nout->axis[1].max = muu->hctx->cam->uRange[1];
  muu->nout->axis[2].center = nrrdCenterCell;
  muu->nout->axis[2].min = muu->hctx->cam->vRange[0];
  muu->nout->axis[2].max = muu->hctx->cam->vRange[1];

  for (thr=0; thr<muu->hctx->numThreads; thr++) {
    (*mrrP)->tt[thr] = (miteThread *)calloc(1, sizeof(miteThread));
    airMopAdd((*mrrP)->rmop, (*mrrP)->tt[thr], airFree, airMopAlways);
  }

  (*mrrP)->time0 = airTime();
  return 0;
}

int
miteRenderEnd(miteRender *mrr, miteUser *muu) {
  int thr;
  double samples;

  muu->rendTime = airTime() - mrr->time0;
  samples = 0;
  for (thr=0; thr<muu->hctx->numThreads; thr++) {
    samples += mrr->tt[thr]->samples;
  }
  muu->sampRate = samples/(1000.0*muu->rendTime);
  _miteRenderNix(mrr);
  return 0;
}
