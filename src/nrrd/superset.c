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

/*
******** nrrdPad()
**
** strictly for padding
*/
int
nrrdPad(Nrrd *nout, Nrrd *nin, int *min, int *max, int boundary, ...) {
  char me[]="nrrdPad", func[]="pad", err[AIR_STRLEN_MED],
    buff1[NRRD_DIM_MAX*30], buff2[AIR_STRLEN_SMALL];
  double padValue=AIR_NAN;
  int d, outside, dim, typeSize,
    cIn[NRRD_DIM_MAX],       /* coords for line start, in input */
    cOut[NRRD_DIM_MAX],      /* coords for line start, in output */
    szIn[NRRD_DIM_MAX],
    szOut[NRRD_DIM_MAX];
  size_t
    idxIn, idxOut,           /* linear indices for input and output */
    numOut;                  /* number of elements in output nrrd */
  va_list ap;
  char *dataIn, *dataOut;
  
  if (!(nout && nin && min && max)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nout == nin) {
    sprintf(err, "%s: nout==nin disallowed", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_BETWEEN(nrrdBoundaryUnknown, boundary, nrrdBoundaryLast)) {
    sprintf(err, "%s: boundary behavior %d invalid", me, boundary);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdBoundaryWeight == boundary) {
    sprintf(err, "%s: boundary strategy %s not applicable here", me,
	    airEnumStr(nrrdBoundary, boundary));
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == nin->type && nrrdBoundaryPad == boundary) {
    sprintf(err, "%s: with nrrd type %s, boundary %s not valid", me,
	    airEnumStr(nrrdType, nrrdTypeBlock),
	    airEnumStr(nrrdBoundary, nrrdBoundaryPad));
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdBoundaryPad == boundary) {
    va_start(ap, boundary);
    padValue = va_arg(ap, double);
    va_end(ap);
  }
  switch(boundary) {
  case nrrdBoundaryPad:
  case nrrdBoundaryBleed:
  case nrrdBoundaryWrap:
    break;
  default:
    fprintf(stderr, "%s: PANIC: boundary %d unimplemented\n", 
	    me, boundary); exit(1); break;
  }
  /*
  printf("!%s: boundary = %d, padValue = %g\n", me, boundary, padValue);
  */

  dim = nin->dim;
  nrrdAxesGet_nva(nin, nrrdAxesInfoSize, szIn);
  for (d=0; d<dim; d++) {
    if (!(min[d] <= 0)) {
      sprintf(err, "%s: axis %d min (%d) not <= 0", 
	      me, d, min[d]);
      biffAdd(NRRD, err); return 1;
    }
    if (!(max[d] >= szIn[d]-1)) {
      sprintf(err, "%s: axis %d max (%d) not >= size-1 (%d)", 
	      me, d, max[d], szIn[d]-1);
      biffAdd(NRRD, err); return 1;
    }
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nin)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }

  /* allocate */
  numOut = 1;
  for (d=0; d<dim; d++) {
    numOut *= (szOut[d] = -min[d] + max[d] + 1);
  }
  nout->blockSize = nin->blockSize;
  if (nrrdMaybeAlloc_nva(nout, nin->type, dim, szOut)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  
  /* the skinny */
  typeSize = nrrdElementSize(nin);
  dataIn = nin->data;
  dataOut = nout->data;
  memset(cOut, 0, NRRD_DIM_MAX*sizeof(int));
  for (idxOut=0; idxOut<numOut; idxOut++) {
    outside = 0;
    for (d=0; d<dim; d++) {
      cIn[d] = cOut[d] + min[d];
      switch(boundary) {
      case nrrdBoundaryPad:
      case nrrdBoundaryBleed:
	if (!AIR_INSIDE(0, cIn[d], szIn[d]-1)) {
	  cIn[d] = AIR_CLAMP(0, cIn[d], szIn[d]-1);
	  outside = 1;
	}
	break;
      case nrrdBoundaryWrap:
	if (!AIR_INSIDE(0, cIn[d], szIn[d]-1)) {
	  cIn[d] = AIR_MOD(cIn[d], szIn[d]);
	  outside = 1;
	}
	break;
      }
    }
    NRRD_COORD_INDEX(idxIn, cIn, szIn, dim);
    if (!outside) {
      /* the cIn coords are within the input nrrd: do memcpy() of whole
	 1-D scanline, then artificially bump for-loop to the end of
	 the scanline */
      memcpy(dataOut + idxOut*typeSize, dataIn + idxIn*typeSize,
	     szIn[0]*typeSize);
      idxOut += nin->axis[0].size-1;
      cOut[0] += nin->axis[0].size-1;
    } else {
      /* we copy only a single value */
      if (nrrdBoundaryPad == boundary) {
	nrrdDInsert[nout->type](dataOut, idxOut, padValue);
      } else {
	memcpy(dataOut + idxOut*typeSize, dataIn + idxIn*typeSize, typeSize);
      }
    }
    NRRD_COORD_INCR(cOut, szOut, dim, 0);
  }
  if (nrrdAxesCopy(nout, nin, NULL, (NRRD_AXESINFO_SIZE_BIT |
				     NRRD_AXESINFO_MIN_BIT |
				     NRRD_AXESINFO_MAX_BIT ))) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  for (d=0; d<dim; d++) {
    nrrdAxisPosRange(&(nout->axis[d].min), &(nout->axis[d].max),
		     nin, d, min[d], max[d]);
  }
  strcpy(buff1, "");
  for (d=0; d<dim; d++) {
    sprintf(buff2, "%s[%d,%d]", (d ? "x" : ""), min[d], max[d]);
    strcat(buff1, buff2);
  }
  if (nrrdContentSet(nout, func, nin, "%s", buff1)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  nrrdPeripheralInit(nout);
  /* leave comments alone */

  return 0;
}

/*
******** nrrdPad_nva()
**
** unlike other {X,X_nva} pairs, nrrdPad_nva() is a wrapper around
** nrrdPad() instead of the other way around.
*/
int
nrrdPad_nva(Nrrd *nout, Nrrd *nin, int *min, int *max,
	    int boundary, double padValue) {
  char me[]="nrrdPad_nva", err[AIR_STRLEN_MED];
  int E;

  if (!AIR_BETWEEN(nrrdBoundaryUnknown, boundary, nrrdBoundaryLast)) {
    sprintf(err, "%s: boundary behavior %d invalid", me, boundary);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdBoundaryPad == boundary) {
    E = nrrdPad(nout, nin, min, max, boundary, padValue);
  } else {
    E = nrrdPad(nout, nin, min, max, boundary);
  }
  if (E) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  
  return 0;
}

/*
******** nrrdSimplePad()
**
** pads by a given amount on top and bottom of EVERY axis
*/
int
nrrdSimplePad(Nrrd *nout, Nrrd *nin, int pad, int boundary, ...) {
  char me[]="nrrdSimplePad", err[AIR_STRLEN_MED];
  int d, min[NRRD_DIM_MAX], max[NRRD_DIM_MAX], ret;
  double padValue;
  va_list ap;

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  for (d=0; d<nin->dim; d++) {
    min[d] = -pad;
    max[d] = nin->axis[d].size-1 + pad;
  }
  if (nrrdBoundaryPad == boundary) {
    va_start(ap, boundary);
    padValue = va_arg(ap, double);
    va_end(ap);
    ret = nrrdPad(nout, nin, min, max, boundary, padValue);
  } else {
    ret = nrrdPad(nout, nin, min, max, boundary);
  }
  if (ret) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

/*
******** nrrdSimplePad_nva()
**
** unlike other {X,X_nva} pairs, nrrdSimplePad_nva() is a wrapper
** around nrrdSimplePad() instead of the other way around.
*/
int
nrrdSimplePad_nva(Nrrd *nout, Nrrd *nin, int pad,
		  int boundary, double padValue) {
  char me[]="nrrdSimplePad_nva", err[AIR_STRLEN_MED];
  int E;

  if (!AIR_BETWEEN(nrrdBoundaryUnknown, boundary, nrrdBoundaryLast)) {
    sprintf(err, "%s: boundary behavior %d invalid", me, boundary);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdBoundaryPad == boundary) {
    E = nrrdSimplePad(nout, nin, pad, boundary, padValue);
  } else {
    E = nrrdSimplePad(nout, nin, pad, boundary);
  }
  if (E) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }

  return 0;
}
