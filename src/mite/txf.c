/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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

/* learned: don't confuse allocate an array of structs with an array
   of pointers to structs.  Don't be surprised when you bus error
   because of the difference 
*/

#include "mite.h"
#include "privateMite.h"

char
miteRangeChar[MITE_RANGE_NUM] = "ARGBEadsp";

int
_miteDomainParse(char *label, gageKind *kind) {
  char me[]="_miteDomainParse", err[AIR_STRLEN_MED], *buff, *paren, *qstr;
  int domI;

  if (!label) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(MITE, err); return -1;
  }
  if (label == strstr(label, "gage(")) {
    /* txf domain variable is to be measured directly by gage */
    buff = airStrdup(label);
    if (!buff) {
      sprintf(err, "%s: this is so annoying", me);
      biffAdd(MITE, err); return -1;
    }
    if (!(paren = strstr(buff, ")"))) {
      sprintf(err, "%s: didn't see close paren after \"gage(\"", me);
      biffAdd(MITE, err); return -1;
    }
    *paren = 0;
    qstr = buff + strlen("gage(");
    domI = airEnumVal(gageScl, qstr);
    if (gageSclUnknown == domI) {
      sprintf(err, "%s: couldn't parse \"%s\" as a gageScl varable", me, qstr);
      biffAdd(MITE, err); free(buff); return -1;
    }
    if (1 != kind->ansLength[domI]) {
      sprintf(err, "%s: %s isn't a scalar, so it can't a txf domain variable",
	      me, airEnumStr(gageScl, domI));
      biffAdd(MITE, err); free(buff); return -1;
    }
  } else {
    /* txf domain variable is not directly measured by gage */
    sprintf(err, "%s: sorry, only txf domain variables currently supported "
	    "are those directly measured by gage", me);
    biffAdd(MITE, err); return -1;
  }
  return domI;
}

int
miteNtxfCheck(Nrrd *ntxf, gageKind *kind) {
  char me[]="miteNtxfCheck", err[AIR_STRLEN_MED], *range, *domS;
  int i;

  if (!( ntxf && kind )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(MITE, err); return 1;
  }
  if (!nrrdValid(ntxf)) {
    sprintf(err, "%s: basic nrrd validity check failed", me);
    biffMove(MITE, err, NRRD); return 1;
  }
  if (!( nrrdTypeFloat == ntxf->type || 
	 nrrdTypeDouble == ntxf->type || 
	 nrrdTypeUChar == ntxf->type )) {
    sprintf(err, "%s: need a type %s, %s or %s nrrd (not %s)", me,
	    airEnumStr(nrrdType, nrrdTypeFloat),
	    airEnumStr(nrrdType, nrrdTypeDouble),
	    airEnumStr(nrrdType, nrrdTypeUChar),
	    airEnumStr(nrrdType, ntxf->type));
    biffAdd(MITE, err); return 1;
  }
  if (!( 2 <= ntxf->dim )) {
    sprintf(err, "%s: nrrd dim (%d) isn't at least 2 (for a 1-D txf)",
	    me, ntxf->dim);
    biffAdd(MITE, err); return 1;
  }
  if (1 == ntxf->dim) {
    sprintf(err, "%s: dimension is 1, must be 2 or greater", me);
    biffAdd(MITE, err); return 1;
  }
  range = ntxf->axis[0].label;
  if (0 == airStrlen(range)) {
    sprintf(err, "%s: axis[0]'s label doesn't specify txf range", me);
    biffAdd(MITE, err); return 1;
  }
  if (airStrlen(range) != ntxf->axis[0].size) {
    sprintf(err, "%s: axis[0]'s size is %d, but label specifies %d values",
	    me, ntxf->axis[0].size, (int)airStrlen(range));
    biffAdd(MITE, err); return 1;
  }
  for (i=0; i<airStrlen(range); i++) {
    if (!strchr(miteRangeChar, range[i])) {
      sprintf(err, "%s: char %d of axis[0]'s label (\"%c\") isn't a valid "
	      "transfer function range specifier (not in \"%s\")",
	      me, i, range[i], miteRangeChar);
      biffAdd(MITE, err); return 1;
    }
  }
  for (i=1; i<ntxf->dim; i++) {
    if (!( AIR_EXISTS(ntxf->axis[i].min) && AIR_EXISTS(ntxf->axis[i].max) )) {
      sprintf(err, "%s: min and max of axis %d aren't both set", me, i);
      biffAdd(MITE, err); return 1;
    }
    if (!( ntxf->axis[i].min < ntxf->axis[i].max )) {
      sprintf(err, "%s: min (%g) not less than max (%g) on axis %d", 
	      me, ntxf->axis[i].min, ntxf->axis[i].max, i);
      biffAdd(MITE, err); return 1;
    }
    if (1 == ntxf->axis[i].size) {
      sprintf(err, "%s: # samples on axis %d must be > 1", me, i);
      biffAdd(MITE, err); return 1;
    }
    domS = ntxf->axis[i].label;
    if (0 == airStrlen(domS)) {
      sprintf(err, "%s: axis[%d] of txf didn't specify a domain variable",
	      me, i);
      biffAdd(MITE, err); return 1;
    }
    if (-1 == _miteDomainParse(domS, kind)) {
      sprintf(err, "%s: problem with txf domain \"%s\" for axis %d\n", 
	      me, domS, i);
      biffAdd(MITE, err); return 1;
    }
  }
  
  return 0;
}

unsigned int
_miteNtxfQuery(Nrrd *ntxf, gageKind *kind) {
  int i;
  unsigned int query;

  query = 0;
  for (i=1; i<ntxf->dim; i++) {
    query |= 1 << _miteDomainParse(ntxf->axis[i].label, kind);
  }
  return query;
}

int
_miteNtxfCopy(miteRender *mrr, miteUser *muu) {
  char me[]="_miteNtxfCopy", err[AIR_STRLEN_MED];
  int ni, E;
  
  mrr->ntxf = (Nrrd **)calloc(muu->ntxfNum, sizeof(Nrrd *));
  if (!mrr->ntxf) {
    sprintf(err, "%s: couldn't calloc %d ntxf pointers", me, muu->ntxfNum);
    biffAdd(MITE, err); return 1;
  }
  mrr->ntxfNum = muu->ntxfNum;
  airMopAdd(mrr->rmop, mrr->ntxf, airFree, airMopAlways);
  E = 0;
  for (ni=0; ni<mrr->ntxfNum; ni++) {
    mrr->ntxf[ni] = nrrdNew();
    if (!E) airMopAdd(mrr->rmop, mrr->ntxf[ni],
		      (airMopper)nrrdNuke, airMopAlways);
    /* this assumes that ntxf type is float, double, or uchar */
    switch(muu->ntxf[ni]->type) {
    case nrrdTypeUChar:
      if (!( AIR_EXISTS(muu->ntxf[ni]->min) &&
	     AIR_EXISTS(muu->ntxf[ni]->max) )) {
	muu->ntxf[ni]->min = 0.0;
	muu->ntxf[ni]->max = 1.0;
      }
      if (!E) E |= nrrdUnquantize(mrr->ntxf[ni], muu->ntxf[ni], nrrdTypeUChar);
      break;
    case mite_nt:
      if (!E) E |= nrrdCopy(mrr->ntxf[ni], muu->ntxf[ni]);
      break;
    default:
      if (!E) E |= nrrdConvert(mrr->ntxf[ni], muu->ntxf[ni], mite_nt);
      break;
    }
  }
  if (E) {
    sprintf(err, "%s: troubling copying/converting all ntxfs", me);
    biffMove(MITE, err, NRRD); return 1;
  }
  return 0;
}

int
_miteNtxfAlphaAdjust(miteRender *mrr, miteUser *muu) {
  char me[]="_miteNtxfAlphaAdjust", err[AIR_STRLEN_MED];
  int ni, ei, ri, rnum;
  Nrrd *ntxf;
  mite_t *data, alpha, frac;
  
  if (_miteNtxfCopy(mrr, muu)) {
    sprintf(err, "%s: trouble copying/converting transfer functions", me);
    biffAdd(MITE, err); return 1;
  }
  frac = muu->rayStep/muu->refStep;
  for (ni=0; ni<mrr->ntxfNum; ni++) {
    ntxf = mrr->ntxf[ni];
    if (!strchr(ntxf->axis[0].label, miteRangeChar[miteRangeAlpha]))
      continue;
    /* else this txf sets opacity */
    data = ntxf->data;
    rnum = ntxf->axis[0].size;
    for (ei=0; ei<nrrdElementNumber(ntxf)/rnum; ei++) {
      for (ri=0; ri<rnum; ri++) {
	if (ntxf->axis[0].label[ri] == miteRangeChar[miteRangeAlpha]) {
	  alpha = data[ri + rnum*ei];
	  data[ri + rnum*ei] = 1 - pow(1 - alpha, frac);
	}
      }
    }
  }
  return 0;
}

int
_miteStageNum(miteRender *mrr) {
  int num, ni;

  num = 0;
  for (ni=0; ni<mrr->ntxfNum; ni++) {
    num += mrr->ntxf[ni]->dim - 1;
  }
  return num;
}

int
_miteStageSet(miteThread *mtt, miteRender *mrr, gageKind *kind) {
  char me[]="_miteStageSet", err[AIR_STRLEN_MED];
  int ni, di, si, rii, dom, stageNum;
  Nrrd *ntxf;
  miteStage *stage;
  char rc;
  
  stageNum = _miteStageNum(mrr);
  fprintf(stderr, "!%s: stageNum = %d\n", me, stageNum);
  mtt->stage = (miteStage *)calloc(stageNum, sizeof(miteStage));
  if (!mtt->stage) {
    sprintf(err, "%s: couldn't alloc array of %d stages", me, stageNum);
    biffAdd(MITE, err); return 1;
  }
  airMopAdd(mrr->rmop, mtt->stage, airFree, airMopAlways);
  mtt->stageNum = stageNum;
  si = 0;
  for (ni=0; ni<mrr->ntxfNum; ni++) {
    ntxf = mrr->ntxf[ni];
    for (di=ntxf->dim-1; di>=1; di--) {
      stage = mtt->stage + si;
      dom = _miteDomainParse(ntxf->axis[di].label, kind);
      stage->val = mtt->ans + kind->ansOffset[dom];
      fprintf(stderr, "!%s: ans=%p + offset[%d]=%d == %p\n", me,
	      mtt->ans, dom, kind->ansOffset[dom], stage->val);
      stage->size = ntxf->axis[di].size;
      stage->min =  ntxf->axis[di].min;
      stage->max =  ntxf->axis[di].max;
      if (di > 1) {
	stage->data = NULL;
      } else {
	stage->data = ntxf->data;
	stage->rangeNum = ntxf->axis[0].size;
	for (rii=0; rii<stage->rangeNum; rii++) {
	  rc = ntxf->axis[0].label[rii];
	  stage->rangeIdx[rii] = strchr(miteRangeChar, rc) - miteRangeChar;
	  fprintf(stderr, "!%s: range: %c -> %d\n", "_miteStageSet",
		  ntxf->axis[0].label[rii], stage->rangeIdx[rii]);
	}
      }
      si++;
    }
  }
  return 0;
}

void
_miteStageRun(miteThread *mtt) {
  int si, ri, rii, idx, index;
  miteStage *stage;
  mite_t *rangeData;

  index = 0;
  for (si=0; si<mtt->stageNum; si++) {
    stage = &(mtt->stage[si]);
    AIR_INDEX(stage->min, *(stage->val), stage->max, stage->size, idx);
    idx = AIR_CLAMP(0, idx, stage->size-1);
    index = stage->size*index + idx;
    if (stage->data) {
      rangeData = stage->data + stage->rangeNum*index;
      for (rii=0; rii<stage->rangeNum; rii++) {
	ri = stage->rangeIdx[rii];
	mtt->range[ri] *= rangeData[rii];
      }
      index = 0;
    }
  }
  return;
}
