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

#include "mite.h"

char
miteRangeChar[MITE_RANGE_NUM] = "ARGBEadsp";

int
_miteDomainParse(char *label) {
  char me[]="_miteDomainParse", err[AIR_STRLEN_MED], *buff;
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
    if (1 != sscanf(label, "gage(%s)", buff)) {
      sprintf(err, "%s: didn't see close paren after \"gage(\"", me);
      biffAdd(MITE, err); return -1;
    }
    domI = airEnumVal(gageScl, buff);
    if (gageSclUnknown == domI) {
      sprintf(err, "%s: couldn't parse \"%s\" as a gageScl varable", 
	      me, buff);
      biffAdd(MITE, err); free(buff); return -1;
    }
    if (1 != gageKindScl->ansLength[domI]) {
      sprintf(err, "%s: %s isn't a scalar, so it can't a txf domain variable",
	      me, airEnumStr(gageScl, domI));
      biffAdd(MITE, err); free(buff); return -1;
    }
  } else {
    /* txf domain variable is not directly measured by gage */
    sprintf(err, "%s: sorry, only txf domain variable currently supported "
	    "are those directly measured by gage", me);
    biffAdd(MITE, err); return -1;
  }
  return domI;
}

int
_miteNtxfValid(Nrrd *ntxf) {
  char me[]="_miteNtxfValid", err[AIR_STRLEN_MED], *range, *domS;
  int i;
  
  if (!nrrdValid(ntxf)) {
    sprintf(err, "%s: basic nrrd validity check failed", me);
    biffMove(MITE, err, NRRD); return 1;
  }
  if (nrrdTypeFloat != ntxf->type) {
    sprintf(err, "%s: need a type %s nrrd (not %s)", me,
	    airEnumStr(nrrdType, nrrdTypeFloat),
	    airEnumStr(nrrdType, ntxf->type));
    biffAdd(MITE, err); return 1;
  }
  if (!( AIR_IN_CL(2, ntxf->dim, MITE_TXF_NUM+1) )) {
    sprintf(err, "%s: nrrd dim (%d) isn't between 2 and %d, for "
	    "txf dim between 1 and %d",
	    me, ntxf->dim, MITE_TXF_NUM+1, MITE_TXF_NUM);
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
	    me, ntxf->axis[0].size, airStrlen(range));
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
    domS = ntxf->axis[i].label;
    if (0 == airStrlen(domS)) {
      sprintf(err, "%s: axis[%d] of txf didn't specify a domain variable",
	      me, i);
      biffAdd(MITE, err); return 1;
    }
    if (-1 == _miteDomainParse(domS)) {
      sprintf(err, "%s: problem with txf domain \"%s\" for axis %d\n", 
	      me, domS, i);
      biffAdd(MITE, err); return 1;
    }
  }
  
  return 0;
}

miteTxf *
miteTxfNew(Nrrd *ntxf) {
  char me[]="miteTxfNew", err[AIR_STRLEN_MED];
  miteTxf *txf;
  int i, *rangeIdx;

  if (!_miteNtxfValid(ntxf)) {
    sprintf(err, "%s: given nrrd can't be used as a transfer function", me);
    biffAdd(MITE, err); return NULL;
  }
  txf = (miteTxf *)calloc(1, sizeof(miteTxf));
  rangeIdx = (int *)calloc(ntxf->axis[0].size, sizeof(int));
  if (!( txf && rangeIdx )) {
    sprintf(err, "%s: couldn't alloc txf!", me);
    biffAdd(MITE, err); return NULL; 
  }
  txf->data = (float*)ntxf->data;
  txf->rangeIdx = rangeIdx;
  for (i=0; i<ntxf->axis[0].size; i++) {
    txf->rangeIdx[i] = (strchr(miteRangeChar, ntxf->axis[0].label[i])
			- miteRangeChar);
    fprintf(stderr, "!%s: range: %c -> %d\n", me,
	    ntxf->axis[0].label[i], txf->rangeIdx[i]);
  }
  for (i=1; i<ntxf->dim; i++) {
    txf->domainIdx[i-1] = _miteDomainParse(ntxf->axis[i].label);
    fprintf(stderr, "!%s: domain: %s -> %d\n", me,
	    ntxf->axis[i].label, txf->domainIdx[i]);
  }
  return txf;
}

miteTxf *
miteTxfNix(miteTxf *txf) {

  if (txf) {
    AIR_FREE(txf->rangeIdx);
    AIR_FREE(txf);
  }
  return NULL;
}

