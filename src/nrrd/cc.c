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

#include "nrrd.h"
#include "privateNrrd.h"

/*
** NOTE: it is currently a bug of all the connected component stuff
** here that unsigned int is sometimes allowed as a type for CCs, but
** int is used for CC id representation and manipulation
*/

int _nrrdCC_EqvIncr = 128;
int _nrrdCC_verb;

int
_nrrdCCFind_1(Nrrd *nout, int *numid, Nrrd *nin) {
  /* char me[]="_nrrdCCFind_1", err[AIR_STRLEN_MED]; */
  int id, lval, val, *out, (*lup)(void *, size_t);
  int sx, I;

  lup = nrrdILookup[nin->type];
  out = (int*)(nout->data);
  out[0] = id = 0;
  *numid = 1;
  
  sx = nin->axis[0].size;
  lval = lup(nin->data, 0);
  for (I=1; I<sx; I++) {
    val = lup(nin->data, I);
    if (lval != val) {
      id++;
      *numid++;
    }
    out[I] = id;
    lval = val;
  }

  return 0;
}

void
_nrrdCCEqvAdd(airArray *eqvArr, int j, int k) {
  int *eqv;
  int eqi;
  
  if (_nrrdCC_verb) {
    fprintf(stderr, "%s: ***(%d,%d)***: eqvArr->len = %d\n", "_nrrdCCEqvAdd",
	    j, k, eqvArr->len);
  }
  if (eqvArr->len) {
    eqv = (int *)(eqvArr->data);
    /* we have some equivalences, but we're only going to check against
       the last one in an effort to reduce duplicate entries */
    eqi = eqvArr->len-1;
    if ( (eqv[0 + 2*eqi] == j && eqv[1 + 2*eqi] == k) ||
	 (eqv[0 + 2*eqi] == k && eqv[1 + 2*eqi] == j) ) {
      /* don't add a duplicate */
      return;
    }
  }
  eqi = airArrayIncrLen(eqvArr, 1);
  eqv = (int *)(eqvArr->data);
  eqv[0 + 2*eqi] = j;
  eqv[1 + 2*eqi] = k;
  return;
}

int
_nrrdCCFind_2(Nrrd *nout, int *numid, airArray *eqvArr,
	      Nrrd *nin, int conny) {
  char me[]="_nrrdCCFind_2"  /* , err[AIR_STRLEN_MED]*/ ; 
  double pvl[5], vl=0;
  int id, pid[5], (*lup)(void *, size_t), *out;
  int p, x, y, sx, sy;

  id = 0; /* sssh! compiler warnings */
  lup = nrrdILookup[nin->type];
  out = (int*)(nout->data);
  sx = nin->axis[0].size;
  sy = nin->axis[1].size;
#define GETV_2(x,y) ((AIR_IN_CL(0, (x), sx-1) && AIR_IN_CL(0, (y), sy-1)) \
                     ? lup(nin->data, (x) + sx*(y)) \
                     : 0.5)
#define GETI_2(x,y) ((AIR_IN_CL(0, (x), sx-1) && AIR_IN_CL(0, (y), sy-1)) \
                     ? out[(x) + sx*(y)] \
                     : -1)

  *numid = 0;
  for (y=0; y<sy; y++) {
    for (x=0; x<sx; x++) {
      if (_nrrdCC_verb) {
	fprintf(stderr, "%s(%d,%d) -----------\n", me, x, y);
      }
      if (!x) {
	pvl[1] = GETV_2(-1, y);   pid[1] = GETI_2(-1, y);
	pvl[2] = GETV_2(-1, y-1); pid[2] = GETI_2(-1, y-1);
	pvl[3] = GETV_2(0, y-1);  pid[3] = GETI_2(0, y-1);
	
      } else {
	pvl[1] = vl;              pid[1] = id;
	pvl[2] = pvl[3];          pid[2] = pid[3];
	pvl[3] = pvl[4];          pid[3] = pid[4];
      }
      pvl[4] = GETV_2(x+1, y-1);  pid[4] = GETI_2(x+1, y-1);
      vl = GETV_2(x, y);
      p = 0;
      if (vl == pvl[1]) {
	id = pid[p=1];
      }
#define TEST(P) \
      if (vl == pvl[(P)]) {                                                   \
	if (p) { if (pid[(P)] != id) { _nrrdCCEqvAdd(eqvArr, pid[(P)], id); } \
	} else { id = pid[p=(P)]; }                                           \
      }
      TEST(3);
      if (2 == conny) {
	TEST(2);
	TEST(4);
      }
      if (!p) {
	/* didn't match anything previous */
	id = *numid;
	*numid += 1;
      }
      if (_nrrdCC_verb) {
	fprintf(stderr, "%s: pvl: %g %g %g %g (vl = %g)\n", me,
		pvl[1], pvl[2], pvl[3], pvl[4], vl);
	fprintf(stderr, "        pid: %d %d %d %d\n",
		pid[1], pid[2], pid[3], pid[4]);
	fprintf(stderr, "    --> p = %d, id = %d, *numid = %d\n",
		p, id, *numid);
      }
      out[x + sx*y] = id;
    }
  }

  return 0;
}

int
_nrrdCCFind_3(Nrrd *nout, int *numid, airArray *eqvArr,
	      Nrrd *nin, int conny) {
  /* char me[]="_nrrdCCFind_2", err[AIR_STRLEN_MED]*/ ; 
  double pvl[14], vl=0;
  int id, pid[14], *out, (*lup)(void *, size_t);
  int p, x, y, z, sx, sy, sz;  

  id = 0; /* sssh! compiler warnings */
  lup = nrrdILookup[nin->type];
  out = (int*)(nout->data);
  sx = nin->axis[0].size;
  sy = nin->axis[1].size;
  sz = nin->axis[2].size;
#define GETV_3(x,y,z) ((AIR_IN_CL(0, (x), sx-1) && AIR_IN_CL(0, (y), sy-1) \
                        && AIR_IN_CL(0, (z), sz-1))                        \
                       ? lup(nin->data, (x) + sx*((y) + sy*(z)))           \
                       : 0.5)
#define GETI_3(x,y,z) ((AIR_IN_CL(0, (x), sx-1) && AIR_IN_CL(0, (y), sy-1) \
                        && AIR_IN_CL(0, (z), sz-1))                        \
                       ? out[(x) + sx*((y) + sy*(z))]                      \
                       : -1)
  
  *numid = 0;
  for (z=0; z<sz; z++) {
    for (y=0; y<sy; y++) {
      for (x=0; x<sx; x++) {
	if (!x) {
	  pvl[ 1] = GETV_3(-1,   y,   z);  pid[ 1] = GETI_3(-1,   y,   z);
	  pvl[ 2] = GETV_3(-1, y-1,   z);  pid[ 2] = GETI_3(-1, y-1,   z);
	  pvl[ 3] = GETV_3( 0, y-1,   z);  pid[ 3] = GETI_3( 0, y-1,   z);
	  pvl[ 5] = GETV_3(-1, y-1, z-1);  pid[ 5] = GETI_3(-1, y-1, z-1);
	  pvl[ 8] = GETV_3(-1,   y, z-1);  pid[ 8] = GETI_3(-1,   y, z-1);
	  pvl[11] = GETV_3(-1, y+1, z-1);  pid[11] = GETI_3(-1, y+1, z-1);
	  pvl[ 6] = GETV_3( 0, y-1, z-1);  pid[ 6] = GETI_3( 0, y-1, z-1);
	  pvl[ 9] = GETV_3( 0,   y, z-1);  pid[ 9] = GETI_3( 0,   y, z-1);
	  pvl[12] = GETV_3( 0, y+1, z-1);  pid[12] = GETI_3( 0, y+1, z-1);
	} else {
	  pvl[ 1] = vl;                    pid[ 1] = id;
	  pvl[ 2] = pvl[ 3];               pid[ 2] = pid[ 3];
	  pvl[ 3] = pvl[ 4];               pid[ 3] = pid[ 4];
	  pvl[ 5] = pvl[ 6];               pid[ 5] = pid[ 6];
	  pvl[ 8] = pvl[ 9];               pid[ 8] = pid[ 9];
	  pvl[11] = pvl[12];               pid[11] = pid[12];
	  pvl[ 6] = pvl[ 7];               pid[ 6] = pid[ 7];
	  pvl[ 9] = pvl[10];               pid[ 9] = pid[10];
	  pvl[12] = pvl[13];               pid[12] = pid[13];
	}
	pvl[ 4] = GETV_3(x+1, y-1,   z);   pid[ 4] = GETI_3(x+1, y-1,   z);
	pvl[ 7] = GETV_3(x+1, y-1, z-1);   pid[ 7] = GETI_3(x+1, y-1, z-1);
	pvl[10] = GETV_3(x+1,   y, z-1);   pid[10] = GETI_3(x+1,   y, z-1);
	pvl[13] = GETV_3(x+1, y+1, z-1);   pid[13] = GETI_3(x+1, y+1, z-1);
	vl = GETV_3(x, y, z);
	p = 0;
	if (vl == pvl[1]) {
	  id = pid[p=1];
	}
	TEST(3);
	TEST(9);
	if (2 <= conny) {
	  TEST(2); TEST(4);
	  TEST(6); TEST(8); TEST(10); TEST(12);
	  if (3 == conny) {
	    TEST(5); TEST(7); TEST(11); TEST(13);
	  }
	}
	if (!p) {
	  /* didn't match anything previous */
	  id = *numid;
	  *numid += 1;
	}
	out[x + sx*(y + sy*z)] = id;
      }
    }
  }

  return 0;
}

int
_nrrdCCFind_N(Nrrd *nfpid, int *maxid, airArray *eqvArr,
	      Nrrd *nin, int conny) {
  char me[]="_nrrdCCFind_N", err[AIR_STRLEN_MED];

  sprintf(err, "%s: sorry, not implemented yet", me);
  biffAdd(NRRD, err); return 1;

}

int
nrrdCCFind(Nrrd *nout, Nrrd *nin, int type, int conny) {
  char me[]="nrrdCCFind", func[]="ccfind", err[AIR_STRLEN_MED];
  Nrrd *nfpid;  /* first-pass IDs */
  airArray *mop, *eqvArr;
  int ret;
  int *map, *fpid, numid, maxid;
  size_t I;
  
  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nout == nin) {
    sprintf(err, "%s: nout == nin disallowed", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!( nrrdTypeIsIntegral[nin->type] && nrrdTypeSize[nin->type] <= 4 )) {
    sprintf(err, "%s: can only find connected components in 1, 2, or 4 byte "
	    "integral values (not %s)",
	    me, airEnumStr(nrrdType, nin->type));
    biffAdd(NRRD, err); return 1;
  }
  if (!( AIR_IN_OP(nrrdTypeUnknown, type, nrrdTypeLast) )) {
    sprintf(err, "%s: got invalid target type %d", me, type);
    biffAdd(NRRD, err); return 1;
  }
  if (!( nrrdTypeIsIntegral[type] && nrrdTypeSize[type] <= 4 )) {
    sprintf(err, "%s: can only save connected components to 1, 2, or 4 byte "
	    "integral values (not %s)",
	    me, airEnumStr(nrrdType, type));
    biffAdd(NRRD, err); return 1;
  }
  if (!( AIR_IN_CL(1, conny, nin->dim) )) {
    sprintf(err, "%s: connectivity value must be in [1..%d] for %d-D "
	    "data (not %d)", me, nin->dim, nin->dim, conny);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdConvert(nfpid=nrrdNew(), nin, nrrdTypeInt)) {
    sprintf(err, "%s: couldn't allocate fpid %s array to match input size",
	    me, airEnumStr(nrrdType, nrrdTypeInt));
    biffAdd(NRRD, err); return 1;
  }

  mop = airMopNew();
  airMopAdd(mop, nfpid, (airMopper)nrrdNuke, airMopAlways);
  eqvArr = airArrayNew(NULL, NULL, 2*sizeof(int), _nrrdCC_EqvIncr);
  airMopAdd(mop, eqvArr, (airMopper)airArrayNuke, airMopAlways);
  ret = 0;
  switch(nin->dim) {
  case 1:
    ret = _nrrdCCFind_1(nfpid, &numid, nin);
    break;
  case 2:
    ret = _nrrdCCFind_2(nfpid, &numid, eqvArr, nin, conny);
    break;
  case 3:
    ret = _nrrdCCFind_3(nfpid, &numid, eqvArr, nin, conny);
    break;
  default:
    ret = _nrrdCCFind_N(nfpid, &numid, eqvArr, nin, conny);
    break;
  }
  if (ret) {
    sprintf(err, "%s: initial pass failed", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }

  if (nin->dim > 1) {
    map = (int*)calloc(numid, sizeof(int));
    airMopAdd(mop, map, airFree, airMopAlways);
    maxid = _nrrdCC_eclass(map, numid, eqvArr);
    /* convert fpid values to final id values */
    fpid = (int*)(nfpid->data);
    for (I=0; I<nrrdElementNumber(nfpid); I++) {
      fpid[I] = map[fpid[I]];
    }
  } else {
    maxid = numid-1;
  }
  
  if (maxid > nrrdTypeMax[type]) {
    sprintf(err, "%s: max cc id %d is too large to fit in output type %s",
	    me, numid, airEnumStr(nrrdType, type));
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  if (nrrdConvert(nout, nfpid, type)) {
    sprintf(err, "%s: trouble converting to final output", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  if (nrrdContentSet(nout, func, nin, "%s,%d",
		     airEnumStr(nrrdType, type), conny)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nout != nin) {
    nrrdAxesCopy(nout, nin, NULL, NRRD_AXESINFO_NONE);
  }
  
  airMopOkay(mop);
  return 0;
}

int
_nrrdCCAdj_1(unsigned char *out, int numid, Nrrd *nin) {

  return 0;
}

int
_nrrdCCAdj_2(unsigned char *out, int numid, Nrrd *nin, int conny) {
  int (*lup)(void *, size_t), x, y, sx, sy, id=0;
  double pid[5];
  
  lup = nrrdILookup[nin->type];
  sx = nin->axis[0].size;
  sy = nin->axis[1].size;
  for (y=0; y<sy; y++) {
    for (x=0; x<sx; x++) {
      if (!x) {
	pid[1] = GETV_2(-1, y);
	pid[2] = GETV_2(-1, y-1);
	pid[3] = GETV_2(0, y-1);
      } else {
	pid[1] = id;
	pid[2] = pid[3];
	pid[3] = pid[4];
      }
      pid[4] = GETV_2(x+1, y-1);
      id = GETV_2(x, y);
#define TADJ(P) \
      if (pid[(P)] != 0.5 && id != pid[(P)]) { \
        out[id + numid*((int)pid[(P)])] = \
          out[((int)pid[(P)]) + numid*id] = 1; \
      }
      TADJ(1);
      TADJ(3);
      if (2 == conny) {
	TADJ(2);
	TADJ(4);
      }
    }
  }

  return 0;
}

int
_nrrdCCAdj_3(unsigned char *out, int numid, Nrrd *nin, int conny) {
  int (*lup)(void *, size_t), x, y, z, sx, sy, sz, id=0;
  double pid[14];
  
  lup = nrrdILookup[nin->type];
  sx = nin->axis[0].size;
  sy = nin->axis[1].size;
  sz = nin->axis[2].size;
  for (z=0; z<sz; z++) {
    for (y=0; y<sy; y++) {
      for (x=0; x<sx; x++) {
	if (!x) {
	  pid[ 1] = GETV_3(-1,   y,   z);
	  pid[ 2] = GETV_3(-1, y-1,   z);
	  pid[ 3] = GETV_3( 0, y-1,   z);
	  pid[ 5] = GETV_3(-1, y-1, z-1);
	  pid[ 8] = GETV_3(-1,   y, z-1);
	  pid[11] = GETV_3(-1, y+1, z-1);
	  pid[ 6] = GETV_3( 0, y-1, z-1);
	  pid[ 9] = GETV_3( 0,   y, z-1);
	  pid[12] = GETV_3( 0, y+1, z-1);
	} else {
	  pid[ 1] = id;
	  pid[ 2] = pid[ 3];
	  pid[ 3] = pid[ 4];
	  pid[ 5] = pid[ 6];
	  pid[ 8] = pid[ 9];
	  pid[11] = pid[12];
	  pid[ 6] = pid[ 7];
	  pid[ 9] = pid[10];
	  pid[12] = pid[13];
	}
	pid[ 4] = GETV_3(x+1, y-1,   z);
	pid[ 7] = GETV_3(x+1, y-1, z-1);
	pid[10] = GETV_3(x+1,   y, z-1);
	pid[13] = GETV_3(x+1, y+1, z-1);
	id = GETV_3(x, y, z);
	TADJ(1);
	TADJ(3);
	TADJ(9);
	if (2 <= conny) {
	  TADJ(2); TADJ(4);
	  TADJ(6); TADJ(8); TADJ(10); TADJ(12);
	  if (3 == conny) {
	    TADJ(5); TADJ(7); TADJ(11); TADJ(13);
	  }
	}
      }
    }
  }

  return 0;
}

int
_nrrdCCAdj_N(unsigned char *out, int numid, Nrrd *nin, int conny) {
  char me[]="_nrrdCCAdj_N", err[AIR_STRLEN_MED];
  
  if (1) {
    sprintf(err, "%s: sorry, not implemented", me);
    biffAdd(NRRD, err); return 1;
  }

  return 0;
}

int
nrrdCCAdjacency(Nrrd *nout, Nrrd *nin, int conny) {
  char me[]="nrrdCCAdjacency", func[]="ccadj", err[AIR_STRLEN_MED];
  int ret, maxid;
  unsigned char *out;

  if (!( nout && nrrdCCValid(nin) )) {
    sprintf(err, "%s: invalid args", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nout == nin) {
    sprintf(err, "%s: nout == nin disallowed", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!( AIR_IN_CL(1, conny, nin->dim) )) {
    sprintf(err, "%s: connectivity value must be in [1..%d] for %d-D "
	    "data (not %d)", me, nin->dim, nin->dim, conny);
    biffAdd(NRRD, err); return 1;
  }
  maxid = _nrrdCC_maxid(nin);
  if (nrrdMaybeAlloc(nout, nrrdTypeUChar, 2, maxid+1, maxid+1)) {
    sprintf(err, "%s: trouble allocating output", me);
    biffAdd(NRRD, err); return 1;
  }
  out = (unsigned char *)(nout->data);
  
  switch(nin->dim) {
  case 1:
    ret = _nrrdCCAdj_1(out, maxid+1, nin);
    break;
  case 2:
    ret = _nrrdCCAdj_2(out, maxid+1, nin, conny);
    break;
  case 3:
    ret = _nrrdCCAdj_3(out, maxid+1, nin, conny);
    break;
  default:
    ret = _nrrdCCAdj_N(out, maxid+1, nin, conny);
    break;
  }
  if (ret) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdContentSet(nout, func, nin, "%d", conny)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  
  return 0;
}

/*
******** nrrdCCMeld
**
** for every cc which is adjacent to only one other component,
** if that other component is larger, then the smaller component
** gets absorbed into the larger
*/
int
nrrdCCMeld(Nrrd *nout, Nrrd *nin, int maxSize, int conny) {
  char me[]="nrrdCCMeld", func[]="ccmeld", err[AIR_STRLEN_MED];
  int i, numid, *size,
    *nn,  /* number of neighbors */
    *wn,  /* which one is our (single) neighbor */
    *map,
    (*lup)(void *, size_t), (*ins)(void *, size_t, int);
  Nrrd *nadj, *nsize, *ntmp, *nnn, *nwn;
  airArray *mop;
  size_t I;
  
  if (!( nout && nrrdCCValid(nin) )) {
    sprintf(err, "%s: invalid args", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nout != nin) {
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  mop = airMopNew();
  airMopAdd(mop, nadj = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nsize = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ntmp = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nnn = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nwn = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);

  if (nrrdCCSize(nsize, nin)
      || nrrdCCAdjacency(nadj, nin, conny)
      || nrrdProject(ntmp, nadj, 0, nrrdMeasureSum)
      || nrrdConvert(nnn, ntmp, nrrdTypeInt)
      || nrrdProject(ntmp, nadj, 0, nrrdMeasureHistoMin)
      || nrrdConvert(nwn, ntmp, nrrdTypeInt)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  size = (int*)(nsize->data);
  nn = (int*)(nnn->data);
  wn = (int*)(nwn->data);   /* NB: wn[i] is meaningful IFF nn[i] == 1 */
  
  numid = nsize->axis[0].size;
  map = (int*)calloc(numid, sizeof(int));
  airMopAdd(mop, map, airFree, airMopAlways);
  for (i=0; i<numid; i++) {
    if (1 == nn[i]                /* I have one neighbor, */
	&& size[wn[i]] > size[i]  /* my neighbor is bigger than me, */
	&& size[i] <= maxSize) {  /* and I'm no bigger than maxSize, */
      map[i] = wn[i];             /* so I become my neighbor */
    } else {
      map[i] = i;                 /* or else I stay me */
    }
  }
  _nrrdCC_settle(map, numid);
  lup = nrrdILookup[nin->type];
  ins = nrrdIInsert[nout->type];
  for (I=0; I<nrrdElementNumber(nin); I++) {
    ins(nout->data, I, map[lup(nin->data, I)]);
  }

  if (nrrdContentSet(nout, func, nin, "%d", conny)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  airMopOkay(mop);
  return 0;
}
