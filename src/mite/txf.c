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

/* learned: don't confuse allocate an array of structs with an array
   of pointers to structs.  Don't be surprised when you bus error
   because of the difference 
*/

#include "mite.h"
#include "privateMite.h"

char
miteRangeChar[MITE_RANGE_NUM] = "ARGBEadsp";

void
miteVariablePrint(char *buff, const gageQuerySpec *qsp) {
  char me[]="miteVariablePrint";
  
  if (gageKindScl == qsp->kind
      || gageKindVec == qsp->kind
      || tenGageKind == qsp->kind) {
    sprintf(buff, "gage(%s:%s)", qsp->kind->name, 
	    airEnumStr(qsp->kind->enm, qsp->query));
  } else if (miteValGageKind == qsp->kind) {
    sprintf(buff, "%s(%s)", qsp->kind->name, 
	    airEnumStr(qsp->kind->enm, qsp->query));
  } else {
    sprintf(buff, "(%s: unknown variable!)", me);
  }
  return;
}


/*
******** miteVariableParse()
**
** takes a string (usually the label from a nrrd axis) and parses it
** to determine the gageQuerySpec from it (which means finding the
** kind and query).  The valid formats are (currently):
**
**   <query>              : miteValGageKind (DEPRECATED)
**   gage(<query>)        : gageKindScl (DEPRECATED)
**   gage(scalar:<query>) : gageKindScl (preferred)
**   gage(vector:<query>) : gageKindVec
**   gage(tensor:<query>) : tenGageKind
**   mite(<query>)        : miteValGageKind
**
** Notice that "scalar", "vector", and "tensor" to NOT refer to the type 
** of the quantity being measured, but rather to the type of volume in 
** which quantity is measured (i.e., the gageKind used)
*/
int
miteVariableParse(gageQuerySpec *qsp, const char *label) {
  char me[]="miteVariableParse", err[AIR_STRLEN_MED], *buff, *endparen,
    *kqstr, *col, *kstr, *qstr;
  airArray *mop;
  
  if (!( qsp && label )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(MITE, err); return 1;
  }
  mop = airMopNew();
  buff = airStrdup(label);
  if (!buff) {
    sprintf(err, "%s: couldn't strdup label!", me);
    biffAdd(MITE, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, buff, airFree, airMopAlways);
  if (strstr(buff, "gage(") == buff) {
    /* txf domain variable is to be measured directly by gage */
    if (!(endparen = strstr(buff, ")"))) {
      sprintf(err, "%s: didn't see close paren after \"gage(\"", me);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
    *endparen = 0;
    kqstr = buff + strlen("gage(");
    /* first see if its a gageKindScl specification */
    qsp->query = airEnumVal(gageScl, kqstr);
    if (-1 != qsp->query) {
      qsp->kind = gageKindScl;
      fprintf(stderr, "\n%s: WARNING: deprecated use of txf domain "
	      "\"gage(%s)\" without explicit gage kind specification; "
	      "should use \"gage(%s:%s)\" instead\n\n",
	      me, kqstr, gageKindScl->name, kqstr);
    } else {
      /* should be of form "<kind>:<query>" */
      col = strstr(kqstr, ":");
      if (!col) {
	sprintf(err, "%s: didn't see \":\" seperator between gage "
		"kind and query", me);
	biffAdd(MITE, err); airMopError(mop); return 1;
      }
      *col = 0;
      kstr = kqstr;
      qstr = col+1;
      if (!strcmp(gageKindScl->name, kstr)) {
	qsp->kind = gageKindScl;
      } else if (!strcmp(gageKindVec->name, kstr)) {
	qsp->kind = gageKindVec;
      } else if (!strcmp(tenGageKind->name, kstr)) {
	qsp->kind = tenGageKind;
      } else {
	sprintf(err, "%s: don't recognized \"%s\" gage kind", me, kstr);
	biffAdd(MITE, err); airMopError(mop); return 1;
      }
      qsp->query = airEnumVal(qsp->kind->enm, qstr);
      if (-1 == qsp->query) {
	sprintf(err, "%s: couldn't parse \"%s\" as a %s varable",
		me, qstr, qsp->kind->name);
	biffAdd(MITE, err); airMopError(mop); return 1;
      }
    }
  } else if (strstr(buff, "mite(") == buff) {
    /* txf domain variable is *not* directly measured by gage */
    if (!(endparen = strstr(buff, ")"))) {
      sprintf(err, "%s: didn't see close paren after \"mite(\"", me);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
    *endparen = 0;
    kqstr = buff + strlen("mite(");
    qsp->query = airEnumVal(miteVal, kqstr);
    if (-1 == qsp->query) {
      sprintf(err, "%s: couldn't parse \"%s\" as a miteVal variable",
	      me, kqstr);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
    qsp->kind = miteValGageKind;
  } else {
    /* didn't start with "gage(" or "mite(" */
    qsp->query = airEnumVal(miteVal, label);
    if (-1 != qsp->query) {
      /* its measured by mite */
      qsp->kind = miteValGageKind;
      fprintf(stderr, "\n%s: WARNING: deprecated use of txf domain "
	      "\"%s\"; should use \"mite(%s)\" instead\n\n",
	      me, label, label);
    } else {
      sprintf(err, "%s: \"%s\" not a recognized variable", me, label);
      biffAdd(MITE, err); airMopError(mop); return 1;
    }
  }
  airMopOkay(mop);
  return 0;
}

int
miteNtxfCheck(const Nrrd *ntxf) {
  char me[]="miteNtxfCheck", err[AIR_STRLEN_MED], *range, *domS;
  gageQuerySpec qsp;
  int rii, axi;

  if (nrrdCheck(ntxf)) {
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
  for (rii=0; rii<airStrlen(range); rii++) {
    if (!strchr(miteRangeChar, range[rii])) {
      sprintf(err, "%s: char %d of axis[0]'s label (\"%c\") isn't a valid "
	      "transfer function range specifier (not in \"%s\")",
	      me, rii, range[rii], miteRangeChar);
      biffAdd(MITE, err); return 1;
    }
  }
  for (axi=1; axi<ntxf->dim; axi++) {
    if (!( AIR_EXISTS(ntxf->axis[axi].min) && 
	   AIR_EXISTS(ntxf->axis[axi].max) )) {
      sprintf(err, "%s: min and max of axis %d aren't both set", me, axi);
      biffAdd(MITE, err); return 1;
    }
    if (!( ntxf->axis[axi].min < ntxf->axis[axi].max )) {
      sprintf(err, "%s: min (%g) not less than max (%g) on axis %d", 
	      me, ntxf->axis[axi].min, ntxf->axis[axi].max, axi);
      biffAdd(MITE, err); return 1;
    }
    if (1 == ntxf->axis[axi].size) {
      sprintf(err, "%s: # samples on axis %d must be > 1", me, axi);
      biffAdd(MITE, err); return 1;
    }
    domS = ntxf->axis[axi].label;
    if (0 == airStrlen(domS)) {
      sprintf(err, "%s: axis[%d] of txf didn't specify a domain variable",
	      me, axi);
      biffAdd(MITE, err); return 1;
    }
    if (miteVariableParse(&qsp, domS)) {
      sprintf(err, "%s: couldn't parse txf domain \"%s\" for axis %d\n", 
	      me, domS, axi);
      biffAdd(MITE, err); return 1;
    }
    if (!( 1 == qsp.kind->ansLength[qsp.query] ||
	   3 == qsp.kind->ansLength[qsp.query] )) {
      sprintf(err, "%s: %s not a scalar or vector: can't be a txf "
	      "domain variable", me, domS);
      biffAdd(MITE, err); return 1;
    }
    if (3 == qsp.kind->ansLength[qsp.query]) {
      /* has to be right length for one of the quantization schemes */
      if (!( limnQNBins[limnQN16checker] == ntxf->axis[axi].size ||
	     limnQNBins[limnQN14checker] == ntxf->axis[axi].size ||
	     limnQNBins[limnQN12checker] == ntxf->axis[axi].size )) {
	sprintf(err, "%s: vector %s can be quantized into %d, %d, or %d bins "
		"but not %d", me, domS, limnQNBins[limnQN16checker],
		limnQNBins[limnQN14checker], limnQNBins[limnQN12checker],
		ntxf->axis[axi].size);
	biffAdd(MITE, err); return 1;
      }
    }
  }
  
  return 0;
}

/*
** _miteQuery()
**
** This looks a given gageQuerySpec and sets the bits in the
** gageKindScl and tenGageKind queries that are required to calculate
** the quantity
**
** NOTE: This does NOT initialize the *query{Scl,Vec,Ten}: it
** just bit-wise or's on new stuff
**
** HEY: this is really unfortunate: each new gage type use for
** volume rendering in mite will have to explicitly added as
** arguments here, which is part of the reason that mite may end
** up explicitly depending on the libraries supplying those gageKinds
** (like how new mite depends on ten)
*/
void
_miteQuery(unsigned int *queryScl, 
	   unsigned int *queryVec, 
	   unsigned int *queryTen, gageQuerySpec *qsp) {
  char me[]="_miteQuery";
  
  /*
  for (i=1; i<ntxf->dim; i++) {
    miteVariableParse(qsp, ntxf->axis[i].label);
  */
  if (gageKindScl == qsp->kind) {
    *queryScl |= 1 << qsp->query;
  } else if (gageKindVec == qsp->kind) {
    *queryVec |= 1 << qsp->query;
  } else if (tenGageKind == qsp->kind) {
    *queryTen |= 1 << qsp->query;
  } else if (miteValGageKind == qsp->kind) {
    /* HEY: the first of these two have useful analogs for tensor
       data, but I won't be able to express them ... */
    switch(qsp->query) {
    case miteValNdotV: 
      *queryScl |= 1 << gageSclNormal;
      break;
    case miteValNdotL:
      *queryScl |= 1 << gageSclNormal;
      break;
    case miteValGTdotV:
      *queryScl |= 1 << gageSclGeomTens;
      break;
    case miteValVrefN:
      *queryScl |= 1 << gageSclNormal;
      break;
    case miteValVdefT:
    case miteValVdefTdotV:
      *queryTen |= 1 << tenGageTensor;
      break;
    }
  } else {
    fprintf(stderr, "%s: PANIC: unrecognized non-null gageKind\n", me);
    exit(1);
  }
  return;
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
    if (!( nrrdTypeUChar == muu->ntxf[ni]->type 
	   || nrrdTypeFloat == muu->ntxf[ni]->type 
	   || nrrdTypeDouble == muu->ntxf[ni]->type )) {
      sprintf(err, "%s: sorry, can't handle txf of type %s (only %s, %s, %s)",
	      me, airEnumStr(nrrdType, muu->ntxf[ni]->type),
	      airEnumStr(nrrdType, nrrdTypeUChar),
	      airEnumStr(nrrdType, nrrdTypeFloat),
	      airEnumStr(nrrdType, nrrdTypeDouble));
      biffAdd(MITE, err); return 1;
    }
    switch(muu->ntxf[ni]->type) {
    case nrrdTypeUChar:
      if (!E) E |= nrrdUnquantize(mrr->ntxf[ni], muu->ntxf[ni], nrrdTypeUChar);
      break;
    case mite_nt:
      if (!E) E |= nrrdCopy(mrr->ntxf[ni], muu->ntxf[ni]);
      break;
    default:  /* will be either float or double (whatever mite_nt isn't) */
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
  int ni, ei, ri, nnum, rnum;
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
    nnum = nrrdElementNumber(ntxf)/rnum;
    for (ei=0; ei<nnum; ei++) {
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

void
_miteStageInit(miteStage *stage) {
  int rii;

  stage->val = NULL;
  stage->size = -1;
  stage->op = miteStageOpUnknown;
  stage->qn = NULL;
  stage->min = stage->max = AIR_NAN;
  stage->data = NULL;
  for (rii=0; rii<=MITE_RANGE_NUM-1; rii++) {
    stage->rangeIdx[rii] = -1;
  }
  stage->rangeNum = -1;
  return;
}

int
_miteStageSet(miteThread *mtt, miteRender *mrr) {
  char me[]="_miteStageSet", err[AIR_STRLEN_MED];
  int ni, di, stageIdx, rii, stageNum;
  Nrrd *ntxf;
  miteStage *stage;
  gageQuerySpec qsp;
  char rc;
  
  stageNum = _miteStageNum(mrr);
  /* fprintf(stderr, "!%s: stageNum = %d\n", me, stageNum); */
  mtt->stage = (miteStage *)calloc(stageNum, sizeof(miteStage));
  if (!mtt->stage) {
    sprintf(err, "%s: couldn't alloc array of %d stages", me, stageNum);
    biffAdd(MITE, err); return 1;
  }
  airMopAdd(mrr->rmop, mtt->stage, airFree, airMopAlways);
  mtt->stageNum = stageNum;
  stageIdx = 0;
  for (ni=0; ni<mrr->ntxfNum; ni++) {
    ntxf = mrr->ntxf[ni];
    for (di=ntxf->dim-1; di>=1; di--) {
      stage = mtt->stage + stageIdx;
      _miteStageInit(stage);
      miteVariableParse(&qsp, ntxf->axis[di].label);
      if (gageKindScl == qsp.kind) {
	stage->val = mtt->ansScl;
      } else if (gageKindVec == qsp.kind) {
	stage->val = mtt->ansVec;
      } else if (tenGageKind == qsp.kind) {
	stage->val = mtt->ansTen;
      } else if (miteValGageKind == qsp.kind) {
	stage->val = mtt->ansMiteVal;
      } else {
	sprintf(err, "%s: don't handle gageKind for \"%s\"",
		me, ntxf->axis[di].label);
	biffAdd(MITE, err); return 1;
      }
      stage->val += qsp.kind->ansOffset[qsp.query];
      /*
      fprintf(stderr, "!%s: ans=%p + offset[%d]=%d == %p\n", me,
	      mtt->ans, dom, kind->ansOffset[dom], stage->val);
      */
      stage->size = ntxf->axis[di].size;
      stage->min =  ntxf->axis[di].min;
      stage->max =  ntxf->axis[di].max;
      if (di > 1) {
	stage->data = NULL;
      } else {
	stage->data = ntxf->data;
	stage->op = miteStageOpMultiply;
	if (1 == qsp.kind->ansLength[qsp.query]) {
	  stage->qn = NULL;
	} else if (3 == 1 == qsp.kind->ansLength[qsp.query]) {
	  if (limnQNBins[limnQN16checker] == ntxf->axis[di].size) {
	    stage->qn = limnVtoQN_GT[limnQN16checker];
	  } else if (limnQNBins[limnQN14checker] == ntxf->axis[di].size) {
	    stage->qn = limnVtoQN_GT[limnQN14checker];
	  } else if (limnQNBins[limnQN12checker] == ntxf->axis[di].size) {
	    stage->qn = limnVtoQN_GT[limnQN12checker];
	  } else {
	    sprintf(err, "%s: txf axis %d size %d not usable for vector "
		    "txf domain variable %s", me,
		    di, ntxf->axis[di].size, ntxf->axis[di].label);
	    biffAdd(MITE, err); return 1;
	  }
	} else {
	  sprintf(err, "%s: %s not scalar or vector (len = %d): can't be "
		  "a txf domain variable", me,
		  ntxf->axis[di].label, qsp.kind->ansLength[qsp.query]);
	  biffAdd(MITE, err); return 1;
	}
	stage->rangeNum = ntxf->axis[0].size;
	for (rii=0; rii<stage->rangeNum; rii++) {
	  rc = ntxf->axis[0].label[rii];
	  stage->rangeIdx[rii] = strchr(miteRangeChar, rc) - miteRangeChar;
	  /*
	  fprintf(stderr, "!%s: range: %c -> %d\n", "_miteStageSet",
		  ntxf->axis[0].label[rii], stage->rangeIdx[rii]);
	  */
	}
      }
      stageIdx++;
    }
  }
  return 0;
}

void
_miteStageRun(miteThread *mtt) {
  int stageIdx, ri, rii, txfIdx, finalIdx;
  miteStage *stage;
  mite_t *rangeData;

  finalIdx = 0;
  for (stageIdx=0; stageIdx<mtt->stageNum; stageIdx++) {
    stage = &(mtt->stage[stageIdx]);
    if (stage->qn) {
      /* its a vector-valued txf domain variable */
      txfIdx = stage->qn(stage->val);
    } else {
      /* its a scalar txf domain variable */
      AIR_INDEX(stage->min, *(stage->val), stage->max, stage->size, txfIdx);
    }
    txfIdx = AIR_CLAMP(0, txfIdx, stage->size-1);
    finalIdx = stage->size*finalIdx + txfIdx;
    if (stage->data) {
      rangeData = stage->data + stage->rangeNum*finalIdx;
      for (rii=0; rii<stage->rangeNum; rii++) {
	ri = stage->rangeIdx[rii];
	mtt->range[ri] *= rangeData[rii];
      }
      finalIdx = 0;
    }
  }
  return;
}
