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
#include "private.h"

/*
** _nrrdApply1DSetUp()
**
** error checking and initializing needed for 1D LUTS and regular
** maps.  The intent is that if thing succeeds, then there is no
** need for any further error checking.
**
** Allocates the output nrrd.
*/
int
_nrrdApply1DSetUp(Nrrd *nout, Nrrd *nin, Nrrd *nmap, int lut) {
  char me[]="_nrrdApply1DSetUp", funcLut[]="lut",
    funcRmap[]="rmap", err[AIR_STRLEN_MED], *mapcnt;
  int mapax, size[NRRD_DIM_MAX], axmap[NRRD_DIM_MAX], d;

  if (nout == nin) {
    sprintf(err, "%s: due to laziness, nout==nin always disallowed", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == nin->type || nrrdTypeBlock == nmap->type) {
    sprintf(err, "%s: input nrrd or %s nrrd is of type block",
	    me, lut ? "lut" : "map");
    biffAdd(NRRD, err); return 1;
  }
  mapax = nmap->dim - 1;
  /* fprintf(stderr, "!%s: mapax = %d\n", me, mapax); */
  if (!(0 == mapax || 1 == mapax)) {
    sprintf(err, "%s: dimension of %s should be 1 or 2, not %d", 
	    me, lut ? "lut" : "map", nmap->dim);
    biffAdd(NRRD, err); return 1;
  }
  if (!(AIR_EXISTS(nmap->axis[mapax].min) &&
	AIR_EXISTS(nmap->axis[mapax].max))) {
    sprintf(err, "%s: min and max not both set along axis %d of %s",
	    me, mapax, lut ? "lut" : "map");
    biffAdd(NRRD, err); return 1;
  }
  if (mapax + nin->dim > NRRD_DIM_MAX) {
    sprintf(err, "%s: input nrrd dim %d with 2D %s exceeds NRRD_DIM_MAX (%d)",
	    me, nin->dim, lut ? "lut" : "map", NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 1;
  }
  /* HEY: this won't work for irregular maps */
  if (nrrdHasNonExistSet(nmap)) {
    sprintf(err, "%s: %s nrrd has non-existent values",
	    me, lut ? "lut" : "map");
    biffAdd(NRRD, err); return 1;
  }

  nrrdAxesGet_nva(nin, nrrdAxesInfoSize, size+mapax);
  for (d=0; d<nin->dim; d++) {
    axmap[d+mapax] = d;
    /* fprintf(stderr, "!%s: axmap[%d] = %d\n",
       me, d+mapax, axmap[d+mapax]); */
  }
  if (mapax) {
    size[0] = nmap->axis[0].size;
    axmap[0] = -1;
  }
  if (nrrdMaybeAlloc_nva(nout, nmap->type, mapax + nin->dim, size)) {
    sprintf(err, "%s: couldn't allocate output nrrd", me);
    biffAdd(NRRD, err); return 1;
  }

  /* peripheral */
  if (nrrdAxesCopy(nout, nin, axmap, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s: trouble copying axes", me);
    biffAdd(NRRD, err); return 1;
  }
  mapcnt = (nmap->content
	    ? airStrdup(nmap->content)
	    : airStrdup(nrrdStateUnknownContent));
  if (!mapcnt) {
    sprintf(err, "%s: couldn't copy %s content!",
	    me, lut ? "lut" : "map");
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdContentSet(nout, lut ? funcLut : funcRmap,
		     nin, "%s", mapcnt)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  nrrdPeripheralInit(nout);
  nout->hasNonExist = nrrdNonExistFalse;   /* really! */
  return 0;
}

/*
******** nrrdApply1DLut
**
** allows lut length to be simply 1
*/
int
nrrdApply1DLut(Nrrd *nout, Nrrd *nin, Nrrd *nlut) {
  char me[]="nrrdApply1DLut", err[AIR_STRLEN_MED];
  int mapax, szin, szlutel, lutidx, lutlen;
  nrrdBigInt N, I;
  char *din, *dout, *dlut;
  double (*load)(void *v), val;
  
  if (!(nout && nlut && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (_nrrdApply1DSetUp(nout, nin, nlut, AIR_TRUE)) {
    sprintf(err, "%s: ", me);
    biffAdd(NRRD, err); return 1;
  }
  mapax = nlut->dim - 1;

  N = nrrdElementNumber(nin);
  din = nin->data;
  dout = nout->data;
  dlut = nlut->data;
  szin = nrrdElementSize(nin);
  szlutel = nrrdElementSize(nlut);
  lutlen = nlut->axis[mapax].size;
  load = nrrdDLoad[nin->type];
  if (mapax) {
    szlutel *= nlut->axis[0].size;
  }
  for (I=0; I<N; I++) {
    val = load(din);
    AIR_INDEX(nlut->axis[mapax].min, val, nlut->axis[mapax].max,
	      lutlen, lutidx);
    lutidx = AIR_CLAMP(0, lutidx, lutlen-1);
    memcpy(dout, dlut + szlutel*lutidx, szlutel);
    din += szin;
    dout += szlutel;
  }
  
  return 0;
}

/*
******** nrrdApply1DRegMap
**
** allows map length to be simply 1
*/
int
nrrdApply1DRegMap(Nrrd *nout, Nrrd *nin, Nrrd *nmap,
		  NrrdKernel *kernel, double *param) {
  char me[]="nrrdApply1DRegMap", err[AIR_STRLEN_MED];
  int mapax, szin, szmapel, idx, maplen, ellen, i;
  nrrdBigInt N, I;
  char *din, *dout, *dmap0, *dmap1;
  double (*load)(void *v), (*lup)(void *v, nrrdBigInt I),
    (*insert)(void *v, nrrdBigInt I, double d), val, idxf;

  if (!(nout && nmap && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (_nrrdApply1DSetUp(nout, nin, nmap, AIR_FALSE)) {
    sprintf(err, "%s: ", me);
    biffAdd(NRRD, err); return 1;
  }
  mapax = nmap->dim - 1;

  if (kernel && param) {
    /* HEY */
    sprintf(err, "%s: sorry, filtered maps not yet implemented", me);
    fprintf(stderr, "%s\n", err);
    biffAdd(NRRD, err); return 1;
  }

  N = nrrdElementNumber(nin);
  din = nin->data;
  dout = nout->data;
  szin = nrrdElementSize(nin);
  maplen = nmap->axis[mapax].size;
  load = nrrdDLoad[nin->type];
  lup = nrrdDLookup[nmap->type];
  insert = nrrdDInsert[nout->type];
  ellen = mapax ? nmap->axis[0].size : 1;
  szmapel = ellen*nrrdElementSize(nmap);
  for (I=0; I<N; I++) {
    val = load(din);
    idxf = AIR_AFFINE(nmap->axis[mapax].min, val, nmap->axis[mapax].max,
		      0, maplen-1);
    idxf = AIR_CLAMP(0, idxf, maplen-1);
    idx = idxf;
    idx -= idx == maplen-1;
    idxf -= idx;
    dmap0 = (char*)(nmap->data) + idx*szmapel;
    dmap1 = (char*)(nmap->data) + (idx+1)*szmapel;
    for (i=0; i<ellen; i++) {
      insert(dout, i, (1-idxf)*lup(dmap0, i) + idxf*lup(dmap1, i));
    }
    din += szin;
    dout += szmapel;
  }
  
  return 0;
}

/*
int
nrrdApply1DIrregMap(Nrrd *nout, Nrrd *nmap, Nrrd *nacl, Nrrd *nin) {
  char me[]="nrrdApply1DIrregMap", err[AIR_STRLEN_MED];
  int mapax, szin, szmapel, idx, maplen, ellen, i, *acl, acllen,
    lo, mid, hi;
  nrrdBigInt N, I;
  char *din, *dout, *dmap0, *dmap1;
  double (*load)(void *v), (*lup)(void *v, nrrdBigInt I),
    (*insert)(void *v, nrrdBigInt I, double d), val, idxf,
    mapmin, mapmax;

  if (!(nout && nmap && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  
  if (2 != nmap->dim) {
    sprintf(err, "%s: map needs to have dimension 2, not %d", me, nmap->dim);
    biffAdd(NRRD, err); return 1;
  }
  if (!( nmap->axis[0].size > 1 && nmap->axis[1].size > 1 )) {
    sprintf(err, "%s: both map's axes sizes should be > 1 (not %d,%d)",
	    me, nmap->axis[0].size, nmap->axis[1].size);
    biffAdd(NRRD, err); return 1;
  }
  ellen = nmap->axis[0].size;
  maplen = nmap->axis[1].size;
  lup = nrrdDLookup[nmap->type];
  for (i=0; i<=maplen-2; i++) {
    if (!( lup(nmap->data, i*ellen) < lup(nmap->data, (i+1)*ellen) )) {
      sprintf(err, "%s: map point %d pos (%g) not < point %d pos (%g)",
	      me, i, lup(nmap->data, i*ellen),
	      i+1, lup(nmap->data, (i+1)*ellen));
      biffAdd(NRRD, err); return 1;
    }
  }
  if (nacl) {
    if (!( 2 == nacl->dim && nrrdTypeInt == nacl->type &&
	   nacl->axis[0]->size == 2 && nacl->axis[1]->size > 1 )) {
      sprintf(err, "%s: invalid nacl: dim %d!=1 || type %s!=int || "
	      "sizes (%d,%d) not (2,>1))",
	      me, nacl->dim, airEnumStr(nrrdType, nacl->type),
	      nacl->axis[0]->size, nacl->axis[0]->size);
      biffAdd(NRRD, err); return 1;
    }
    acl = nacl->data;
    acllen = nacl->axis[1]->size;
  } else {
    acl = NULL;
    acllen = 0;
  }
  
  N = nrrdElementNumber(nin);
  din = nin->data;
  dout = nout->data;
  szin = nrrdElementSize(nin);
  load = nrrdDLoad[nin->type];
  lup = nrrdDLookup[nmap->type];
  insert = nrrdDInsert[nout->type];
  szmapel = ellen*nrrdElementSize(nmap);
  mapmin = lup(nmap->data, 0);
  mapmax = lup(nmap->data, (maplen-1)*ellen);
  fprintf(stderr, "!%s: mapmin = %g, mapmax = %g\n", me, mapmin, mapmax);
  for (I=0; I<N; I++) {
    val = load(din);
    if (val < mapmin) {
      idx = 0;
      idxf = 0.0;
    } else if (val > mapmax) {
      idx = maplen-2;
      idxf = 1.0;
    } else {
      if (acl) {
	AIR_INDEX(mapmin, val, mapmax, maplen, idx);
	lo = acl[0 + 2*idx];
	hi = acl[1 + 2*idx];
      } else {
	lo = 0;
	hi = maplen-1;
      }
      while (lo < hi) {
	mid = (lo + hi)/2;
	
      }
    }
    dmap0 = (char*)(nmap->data) + idx*szmapel;
    dmap1 = (char*)(nmap->data) + (idx+1)*szmapel;
    for (i=0; i<ellen; i++) {
      insert(dout, i, (1-idxf)*lup(dmap0, i) + idxf*lup(dmap1, i));
    }
    din += szin;
    dout += szmapel;
  }
  
  return 0;
}
*/
