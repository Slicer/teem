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
******** nrrdInvertPerm()
**
** given an array (p) which represents a permutation of n elements,
** compute the inverse permutation ip.  The value of this function
** is not its core functionality, but all the error checking it
** provides.
*/
int
nrrdInvertPerm(int *invp, int *p, int n) {
  char me[]="nrrdInvertPerm", err[NRRD_STRLEN_MED];
  int problem, i;

  if (!(invp && p && n > 0)) {
    sprintf(err, "%s: got NULL pointer or non-positive n (%d)", me, n);
    biffAdd(NRRD, err); return 1;
  }
  
  /* use the given array "invp" as a temp buffer for validity checking */
  memset(invp, 0, n*sizeof(int));
  for (i=0; i<=n-1; i++) {
    if (!(AIR_INSIDE(0, p[i], n-1))) {
      sprintf(err, "%s: permutation element #%d == %d out of bounds [0,%d]",
	      me, i, p[i], n-1);
      biffAdd(NRRD, err); return 1;
    }
    invp[p[i]]++;
  }
  problem = AIR_FALSE;
  for (i=0; i<=n-1; i++) {
    if (1 != invp[i]) {
      sprintf(err, "%s: element #%d mapped to %d times (should be once)",
	      me, i, invp[i]);
      biffAdd(NRRD, err); problem = AIR_TRUE;
    }
  }
  if (problem) {
    return 1;
  }

  /* the skinny */
  for (i=0; i<=n-1; i++) 
    invp[p[i]] = i;

  return 0;
}


/*
******** nrrdPermuteAxes
**
** changes the scanline ordering of the data in a nrrd
** 
** The basic means by which data is moved around is with memcpy().
** The goal is to call memcpy() as few times as possible, on memory 
** segments as large as possible.  Currently, this is done by 
** detecting how many of the low-index axes are left untouched by 
** the permutation- this constitutes a "scanline" which can be
** copied around as a unit.  For permuting the y and z axes of a
** matrix-x-y-z order tensor volume, this optimization produced a
** factor of 5 speed up (exhaustive multi-platform tests, of course).
**
** The axes[] array determines the permutation of the axes.
** axis[i] = j means: axis i in the output will be the input's axis j
** (axis[i] answers: "what do I put here", from the standpoint of the output,
** not "where do I put this", from the standpoint of the input)
*/
int
nrrdPermuteAxes(Nrrd *nout, Nrrd *nin, int *axes) {
  char me[]="nrrdPermuteAxes", err[NRRD_STRLEN_MED],
    tmpS[NRRD_STRLEN_SMALL];
  nrrdBigInt idxOut, idxIn,  /* indices for input and output scanlines */
    lineSize,                /* size of block of memory which can be
				moved contiguously from input to output,
				thought of as a "scanline" */
    numLines;                /* how many "scanlines" there are to permute */
  char *dataIn, *dataOut;
  int 
    szIn[NRRD_DIM_MAX], *lszIn,
    szOut[NRRD_DIM_MAX], *lszOut,
    cIn[NRRD_DIM_MAX],
    cOut[NRRD_DIM_MAX],
    ip[NRRD_DIM_MAX+1],      /* inverse of permutation in "axes" */
    laxes[NRRD_DIM_MAX+1],   /* copy of axes[], but shifted down by lowPax
				elements, to remove i such that i == axes[i] */
    lowPax,                  /* lowest axis which is "p"ermutated */
    d,                       /* running index along dimensions */
    dim,                     /* copy of nin->dim */
    ldim;                    /* dim - lowPax */

  if (!(nin && nout && axes)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nin == nout) {
    sprintf(err, "%s: can't permute from a nrrd into itself", me);
    biffAdd(NRRD, err); return 1;
  }
  /* we don't actually need ip[], computing it is for error checking */
  if (nrrdInvertPerm(ip, axes, nin->dim)) {
    sprintf(err, "%s: couldn't compute axis permutation inverse", me);
    biffAdd(NRRD, err); return 1;
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nin)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }
  
  dim = nin->dim;
  for (d=0; d<=dim-1 && axes[d] == d; d++)
    ;
  lowPax = d;

  if (lowPax == dim) {
    /* we were given the identity permutation, just copy whole thing */
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s: trouble copying input", me);
      biffAdd(NRRD, err); return 1;      
    }
    return 0;
  }
  
  /* else lowPax < dim (actually, lowPax < dim-1) */
  /*
  printf("!%s: lowPax = %d\n", me, lowPax);
  */
  
  /* allocate output */
  nrrdAxesGet_nva(nin, nrrdAxesInfoSize, szIn);
  nout->blockSize = nin->blockSize;
  for (d=0; d<=dim-1; d++) {
    szOut[d] = szIn[axes[d]];
  }
  if (nrrdMaybeAlloc_nva(nout, nin->type, nin->dim, szOut)) {
    sprintf(err, "%s: failed to allocate output", me);
    biffAdd(NRRD, err); return 1;
  }
  
  /* peripheral info */
  if (nrrdAxesCopy(nout, nin, axes, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("permute(,())")
			   + strlen(nin->content)
			   + nin->dim*(11 + strlen(","))
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "permute(%s,(", nin->content);
      for (d=0; d<=nin->dim-1; d++) {
	sprintf(tmpS, "%d%s", axes[d], d < nin->dim-1 ? "," : "))");
	strcat(nout->content, tmpS);
      }
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  nout->min = nin->min;
  nout->max = nin->max;
  nout->oldMin = nin->oldMin;
  nout->oldMax = nin->oldMax;

  /* the skinny */
  lineSize = 1;
  for (d=0; d<=lowPax-1; d++) {
    lineSize *= nin->axis[d].size;
  }
  numLines = nrrdElementNumber(nin)/lineSize;
  lineSize *= nrrdElementSize(nin);
  lszIn = szIn + lowPax;
  lszOut = szOut + lowPax;
  ldim = dim - lowPax;
  memset(laxes, 0, NRRD_DIM_MAX*sizeof(int));
  for (d=0; d<=ldim-1; d++)
    laxes[d] = axes[d+lowPax]-lowPax;
  dataIn = nin->data;
  dataOut = nout->data;
  memset(cIn, 0, NRRD_DIM_MAX*sizeof(int));
  memset(cOut, 0, NRRD_DIM_MAX*sizeof(int));
  for (idxOut=0; idxOut<=numLines-1; idxOut++) {
    /* in our representation of the coordinates of the start of the
       scanlines that we're copying, we are not even storing all the
       zeros in the coordinates prior to lowPax, and when we go to
       a linear index for the memcpy(), we multiply by lineSize */
    for (d=0; d<=ldim-1; d++)
      cIn[laxes[d]] = cOut[d];
    NRRD_COORD_INDEX(idxIn, cIn, lszIn, ldim);
    memcpy(dataOut + idxOut*lineSize, dataIn + idxIn*lineSize, lineSize);
    NRRD_COORD_INCR(cOut, lszOut, ldim, 0);
  }

  return 0;
}

/*
******** nrrdSwapAxes()
**
** for when you just want to switch the order of two axes, without
** going through the trouble of creating the permutation array 
** needed to call nrrdPermuteAxes()
*/
int
nrrdSwapAxes(Nrrd *nout, Nrrd *nin, int ax1, int ax2) {
  char me[]="nrrdSwapAxes", err[NRRD_STRLEN_MED];
  int i, axes[NRRD_DIM_MAX];

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(AIR_INSIDE(0, ax1, nin->dim-1) 
	&& AIR_INSIDE(0, ax2, nin->dim-1))) {
    sprintf(err, "%s: ax1 (%d) or ax2 (%d) out of bounds [0,%d]", 
	    me, ax1, ax2, nin->dim-1);
    biffAdd(NRRD, err); return 1;
  }

  for (i=0; i<=nin->dim-1; i++)
    axes[i] = i;
  axes[ax2] = ax1;
  axes[ax1] = ax2;
  if (nrrdPermuteAxes(nout, nin, axes)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

/*
******** nrrdShuffle
**
** rearranges hyperslices of a nrrd along a given axis according to
** given permutation.  This could be used to on a 4D array,
** representing a 3D volume of vectors, to re-order the vector 
** components.
**
** the given permutation array must allocated for at least as long as
** the input nrrd along the chosen axis.  perm[j] = i means that the
** value at position j in the _new_ array should come from position i
** in the _old_array.  The standpoint is from the new, looking at
** where to find the values amid the old array (perm answers "what do
** I put here", not "where do I put this").  This allows multiple
** positions in the new array to copy from the same old position, and
** insures that there is an source for all positions along the new
** array.
*/
int
nrrdShuffle(Nrrd *nout, Nrrd *nin, int axis, int *perm) {
  char me[]="nrrdShuffle", err[NRRD_STRLEN_MED],
    tmpS[NRRD_STRLEN_SMALL];
  int size[NRRD_DIM_MAX], *lsize,
    d, ldim, len,
    cIn[NRRD_DIM_MAX+1],
    cOut[NRRD_DIM_MAX+1];
  nrrdBigInt idxIn, idxOut, lineSize, numLines;
  unsigned char *dataIn, *dataOut;

  if (!(nin && nout && perm)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nin == nout) {
    sprintf(err, "%s: can't shuffle from a nrrd into itself", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_INSIDE(0, axis, nin->dim-1)) {
    sprintf(err, "%s: axis %d outside valid range [0,%d]", 
	    me, axis, nin->dim-1);
    biffAdd(NRRD, err); return 1;
  }
  len = nin->axis[axis].size;
  for (d=0; d<=len-1; d++) {
    if (!AIR_INSIDE(0, perm[d], len-1)) {
      sprintf(err, "%s: perm[%d] (%d) outside valid range [0,%d]", me,
	      d, perm[d], len-1);
      biffAdd(NRRD, err); return 1;
    }
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nin)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }

  /* set information in new volume */
  nout->blockSize = nin->blockSize;
  nrrdAxesGet_nva(nin, nrrdAxesInfoSize, size);
  if (nrrdMaybeAlloc_nva(nout, nin->type, nin->dim, size)) {
    sprintf(err, "%s: failed to allocate output", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdAxesCopy(nout, nin, NULL, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  /* the min and max along the shuffled axis are now meaningless */
  nout->axis[axis].min = nout->axis[axis].max = AIR_NAN;
  nout->min = nin->min;
  nout->max = nin->max;
  nout->oldMin = nin->oldMin;
  nout->oldMax = nin->oldMax;
  if (nin->content) {
    nout->content = calloc(strlen("shuffle(,,())")
			   + strlen(nin->content)
			   + 11
			   + len*(11 + strlen(","))
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "shuffle(%s,%d,(", nin->content, axis);
      for (d=0; d<=len-1; d++) {
	sprintf(tmpS, "%d%s", perm[d], d<len-1 ? "," : "))");
	strcat(nout->content, tmpS);
      }
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  
  /* the skinny */
  lineSize = 1;
  for (d=0; d<=axis-1; d++) {
    lineSize *= nin->axis[d].size;
  }
  numLines = nrrdElementNumber(nin)/lineSize;
  lineSize *= nrrdElementSize(nin);
  lsize = size + axis;
  ldim = nin->dim - axis;
  dataIn = nin->data;
  dataOut = nout->data;
  memset(cIn, 0, (NRRD_DIM_MAX+1)*sizeof(int));
  memset(cOut, 0, (NRRD_DIM_MAX+1)*sizeof(int));
  for (idxOut=0; idxOut<=numLines-1; idxOut++) {
    memcpy(cIn, cOut, ldim*sizeof(int));
    cIn[0] = perm[cOut[0]];
    NRRD_COORD_INDEX(idxIn, cIn, lsize, ldim);
    NRRD_COORD_INDEX(idxOut, cOut, lsize, ldim);
    memcpy(dataOut + idxOut*lineSize, dataIn + idxIn*lineSize, lineSize);
    NRRD_COORD_INCR(cOut, lsize, ldim, 0);
  }

  return 0;
}

/*
******** nrrdFlip()
**
** reverse the order of slices along the given axis.
** Actually, just a wrapper around nrrdShuffle() (with the minor addition
** of setting nout->axis[axis].min and .max)
*/
int
nrrdFlip(Nrrd *nout, Nrrd *nin, int axis) {
  char me[]="nrrdFlip", err[NRRD_STRLEN_MED];
  int i, *perm;

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(AIR_INSIDE(0, axis, nin->dim-1))) {
    sprintf(err, "%s: given axis (%d) is outside valid range ([0,%d])", 
	    me, axis, nin->dim-1);
    biffAdd(NRRD, err); return 1;
  }
  if (!(perm = calloc(nin->axis[axis].size, sizeof(int)))) {
    sprintf(err, "%s: couldn't alloc permutation array", me);
    biffAdd(NRRD, err); return 1;
  }
  for (i=0; i<=nin->axis[axis].size-1; i++) {
    perm[i] = nin->axis[axis].size-1-i;
  }
  if (nrrdShuffle(nout, nin, axis, perm)) {
    sprintf(err, "%s: trouble doing shuffle", me);
    biffAdd(NRRD, err); return 1;
  }
  nout->axis[axis].min = nin->axis[axis].max;
  nout->axis[axis].max = nin->axis[axis].min;
  perm = airFree(perm);
  return 0;
}

int
nrrdJoin(Nrrd *nout, Nrrd **nin, int numNin, int axis, int incrDim) {
  char me[]="nrrdJoin", err[NRRD_STRLEN_MED];
  int mindim, maxdim, diffdim, outdim, map[NRRD_DIM_MAX], size[NRRD_DIM_MAX],
    i, d, outlen, permute[NRRD_DIM_MAX], ipermute[NRRD_DIM_MAX];
  nrrdBigInt outnum, chunksize;
  char *dataPerm;
  Nrrd *nperm, **ninperm;
  airArray *mop;

  /* error checking */
  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(numNin >= 1)) {
    sprintf(err, "%s: numNin (%d) must be >= 1", me, numNin);
    biffAdd(NRRD, err); return 1;
  }
  for (i=0; i<=numNin-1; i++) {
    if (!(nin[i])) {
      sprintf(err, "%s: input nrrd #%d NULL", me, i);
      biffAdd(NRRD, err); return 1;
    }
  }

  mop = airMopInit();
  ninperm = calloc(numNin, sizeof(Nrrd *));
  if (!(ninperm)) {
    sprintf(err, "%s: couldn't calloc() temp nrrd array", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, ninperm, airFree, airMopAlways);

  maxdim = mindim = nin[0]->dim;
  for (i=0; i<=numNin-1; i++) {
    mindim = AIR_MIN(mindim, nin[i]->dim);
    maxdim = AIR_MAX(maxdim, nin[i]->dim);
  }
  diffdim = maxdim - mindim;
  if (diffdim > 1) {
    sprintf(err, "%s: will only reshape up one dimension (not %d)",
	    me, diffdim);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  if (axis > maxdim) {
    sprintf(err, "%s: can't join along axis %d with highest input dim = %d",
	    me, axis, maxdim);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  
  /* figure out dimension of output (outdim) */
  if (diffdim) {
    /* case A: (example) 2D slices and 3D slabs are being joined
     together to make a bigger 3D volume */
    outdim = maxdim;
  }
  else {
    /* diffdim == 0 */
    if (axis == maxdim) {
      /* case B: this is like the old "stitch": a bunch of equal-sized
	 slices of dimension N are being stacked together to make an
	 N+1 dimensional volume, which in terms memory, is essentially
	 just the result of concatenating the memory of individual inputs */
      outdim = maxdim + 1;
    }
    else {
      /* case C: axis < maxdim; maxdim == mindim */
      /* case C1 (!incrDim): a bunch of N-D slabs are being joined
	 together to make a bigger N-D volume.  The axis along which
	 they are being joined could be any of existing axes (from 0
	 to maxdim-1) */
      /* case C2 (incrDim): this is also a "stitch", but the new axis
	 created by the stitching is inserted into the existing
	 axes. (ex: stitch 3 PGMs (R, G, B) together into a PPM (with
	 color on axis zero) */
      outdim = maxdim + !!incrDim;
    }
  }
  if (outdim > NRRD_DIM_MAX) {
    sprintf(err, "%s: output dimension (%d) exceeds NRRD_DIM_MAX (%d)",
	    me, outdim, NRRD_DIM_MAX);
    biffAdd(NRRD, err); airMopError(mop); return 1;    
  }
  
  /* do tacit reshaping, and possibly permuting, as needed */
  for (i=0; i<=outdim-1; i++) {
    permute[i] = (i < axis
		  ? i 
		  : (i < outdim-1
		     ? i + 1
		     : axis));
    /* fprintf(stderr, "!%s: 1st permute[%d] = %d\n", me, i, permute[i]); */
  }
  for (i=0; i<=numNin-1; i++) {
    ninperm[i] = nrrdNew();
    diffdim = outdim - nin[i]->dim;
    /* fprintf(stderr, "!%s: i = %d ---> diffdim = %d\n", me, i, diffdim); */
    if (diffdim) {
      /* we do a tacit reshaping, which actually includes
	 a tacit permuting, so we don't have to call permute
	 on the parts that don't actually need it */
      /* NB: we register nrrdNix, not nrrdNuke */
      /* fprintf(stderr, "!%s: %d: tacit reshape/permute\n", me, i); */
      airMopAdd(mop, ninperm[i], (airMopper)nrrdNix, airMopAlways);
      nrrdAxesGet_nva(nin[i], nrrdAxesInfoSize, size);
      for (d=nin[i]->dim-1; d>=mindim+1; d--) {
	size[d] = size[d-1];
      }
      size[mindim] = 1;
      /* we don't have to actually call nrrdReshape(): we just nrrdWrap()
	 the input data with the reshaped size array */
      if (nrrdWrap_nva(ninperm[i], nin[i]->data, nin[i]->type,
		   nin[i]->dim+1, size)) {
	sprintf(err, "%s: trouble creating intermediate version of nrrd %d",
		me, i);
	biffAdd(NRRD, err); airMopError(mop); return 1;    
      }
    }
    else {
      /* on this part, we permute (no need for a reshape) */
      airMopAdd(mop, ninperm[i], (airMopper)nrrdNuke, airMopAlways);
      if (nrrdPermuteAxes(ninperm[i], nin[i], permute)) {
	sprintf(err, "%s: trouble permuting input part %d", me, i);
	biffAdd(NRRD, err); airMopError(mop); return 1;
      }
    }
  }

  /* make sure all parts are compatible in type and shape,
     determine length of final output along axis (outlen) */
  outlen = 0;
  for (i=0; i<=numNin-1; i++) {
    if (ninperm[i]->type != ninperm[0]->type) {
      sprintf(err, "%s: type (%d) of part %d unlike first's (%d)",
	      me, ninperm[i]->type, i, ninperm[0]->type);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    if (nrrdTypeBlock == ninperm[0]->type) {
      if (ninperm[i]->blockSize != ninperm[0]->blockSize) {
	sprintf(err, "%s: blockSize (%d) of part %d unlike first's (%d)",
		me, ninperm[i]->blockSize, i, ninperm[0]->blockSize);
	biffAdd(NRRD, err); airMopError(mop); return 1;
      }
    }
    if (!nrrdElementSize(ninperm[i])) {
      sprintf(err, "%s: got wacky elements size (%d) for part %d",
	      me, nrrdElementSize(ninperm[i]), i);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    
    /* fprintf(stderr, "%s: part %03d shape: ", me, i); */
    for (d=0; d<=outdim-2; d++) {
      /* fprintf(stderr, "%03d ", ninperm[i]->axis[d].size); */
      if (ninperm[i]->axis[d].size != ninperm[0]->axis[d].size) {
	sprintf(err, "%s: axis %d size (%d) of part %d unlike first's (%d)",
		me, d, ninperm[i]->axis[d].size, i, ninperm[0]->axis[d].size);
	biffAdd(NRRD, err); airMopError(mop); return 1;
      }
    }
    /* fprintf(stderr, "%03d\n", ninperm[i]->axis[outdim-1].size); */
    outlen += ninperm[i]->axis[outdim-1].size;
  }
  /* fprintf(stderr, "%s: outlen = %d\n", me, outlen); */

  /* allocate temporary nrrd and concat input into it */
  outnum = 1;
  for (d=0; d<=outdim-2; d++) {
    size[d] = ninperm[0]->axis[d].size;
    outnum *= size[d];
  }
  size[outdim-1] = outlen;
  outnum *= size[outdim-1];
  if (nrrdAlloc_nva(nperm = nrrdNew(), ninperm[0]->type, outdim, size)) {
    sprintf(err, "%s: trouble allocating permutation nrrd", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, nperm, (airMopper)nrrdNuke, airMopAlways);
  dataPerm = nperm->data;
  for (i=0; i<=numNin-1; i++) {
    /* here is where the actual joining happens */
    chunksize = nrrdElementNumber(ninperm[i])*nrrdElementSize(ninperm[i]);
    memcpy(dataPerm, ninperm[i]->data, chunksize);
    dataPerm += chunksize;
  }
  
  /* copy other axis-specific fields from nin[0] to nperm */
  for (d=0; d<=outdim-2; d++)
    map[d] = d;
  map[outdim-1] = -1;
  nrrdAxesCopy(nperm, ninperm[0], map, NRRD_AXESINFO_NONE);
  nperm->axis[outdim-1].size = outlen;

  /* do the permutation required to get output in right order */
  nrrdInvertPerm(ipermute, permute, outdim);
  if (nrrdPermuteAxes(nout, nperm, ipermute)) {
    sprintf(err, "%s: error permuting temporary nrrd", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }

  airMopOkay(mop); 
  return 0;
}

/*
******** nrrdReshape_nva()
**
*/
int
nrrdReshape_nva(Nrrd *nout, Nrrd *nin, int dim, int *size) {
  char me[]="nrrdReshape_nva", err[NRRD_STRLEN_MED],
    tmpS[NRRD_STRLEN_SMALL];
  nrrdBigInt numOut;
  int d;

  if (!(nout && nin && size)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(AIR_INSIDE(1, dim, NRRD_DIM_MAX))) {
    sprintf(err, "%s: given dimension (%d) outside valid range [1,%d]",
	    me, dim, NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 1;
  }
  numOut = 1;
  for (d=0; d<=dim-1; d++) {
    if (!(1 <= size[d])) {
      sprintf(err, "%s: size[%d] (%d) invalid", me, d, size[d]);
      biffAdd(NRRD, err); return 1;
    }
    numOut *= size[d];
  }
  if (numOut != nrrdElementNumber(nin)) {
    sprintf(err, "%s: new sizes product (" NRRD_BIG_INT_PRINTF ") "
	    "!= # elements (" NRRD_BIG_INT_PRINTF ")",
	    me, numOut, nrrdElementNumber(nin));
    biffAdd(NRRD, err); return 1;
  }

  /* HEY: non-essential, non-axis info (like comments) has been copied,
     perhaps that's not appropriate */
  if (nrrdCopy(nout, nin)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  nout->dim = dim;
  for (d=0; d<=dim-1; d++) {
    /* the ONLY thing we can say about the axes is the size */
    _nrrdAxisInit(&(nout->axis[d]));
    nout->axis[d].size = size[d];
  }

  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("reshape(,)")
			   + strlen(nin->content)
			   + dim*(11 + strlen("x"))
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "reshape(%s,", nin->content);
      for (d=0; d<=dim-1; d++) {
	sprintf(tmpS, "%d%s", size[d], d < dim-1 ? "x" : ")");
	strcat(nout->content, tmpS);
      }
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
    
  }

  return 0;
}

/*
******** nrrdReshape()
**
** var-args version of nrrdReshape_nva()
*/
int
nrrdReshape(Nrrd *nout, Nrrd *nin, int dim, ...) {
  char me[]="nrrdReshape", err[NRRD_STRLEN_MED];
  int d, size[NRRD_DIM_MAX];
  va_list ap;

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(AIR_INSIDE(1, dim, NRRD_DIM_MAX))) {
    sprintf(err, "%s: given dimension (%d) outside valid range [1,%d]",
	    me, dim, NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 1;
  }
  va_start(ap, dim);
  for (d=0; d<=dim-1; d++) {
    size[d] = va_arg(ap, int);
  }
  va_end(ap);

  if (nrrdReshape_nva(nout, nin, dim, size)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  
  return 0;
}

/*
******** nrrdBlock()
**
** collapse the first axis (axis 0) of the nrrd into a block, making
** an output nrrd of type nrrdTypeBlock.  The input type can be block.
** All information for other axes is shifted down one axis.
*/
int
nrrdBlock(Nrrd *nout, Nrrd *nin) {
  char me[]="nrrdBlock", err[NRRD_STRLEN_MED];
  int d, numEl, map[NRRD_DIM_MAX], size[NRRD_DIM_MAX];

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (1 == nin->dim) {
    sprintf(err, "%s: can't blockify 1-D nrrd", me);
    biffAdd(NRRD, err); return 1;
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nin)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }

  numEl = nin->axis[0].size;;
  nout->blockSize = numEl*nrrdElementSize(nin);
  /*
  fprintf(stderr, "%s: nout->blockSize = %d * %d = %d\n", me,
	  numEl, nrrdElementSize(nin), nout->blockSize);
  */
  for (d=0; d<=nin->dim-2; d++) {
    map[d] = d+1;
    size[d] = nin->axis[map[d]].size;
  }

  /* nout->blockSize set above */
  if (nrrdMaybeAlloc_nva(nout, nrrdTypeBlock, nin->dim-1, size)) {
    sprintf(err, "%s: failed to allocate output", me);
    biffAdd(NRRD, err); return 1;
  }
  memcpy(nout->data, nin->data, nrrdElementNumber(nin)*nrrdElementSize(nin));
  if (nrrdAxesCopy(nout, nin, map, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s: failed to copy axes", me);
    biffAdd(NRRD, err); return 1;
  }
  
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("block()")
			   + strlen(nin->content)
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "block(%s)", nin->content);
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  return 0;
}

/*
******** nrrdUnblock()
**
** takes a nrrdTypeBlock nrrd and breaks the blocks into elements of 
** type "type", and shifts other axis information up by one axis
*/
int
nrrdUnblock(Nrrd *nout, Nrrd *nin, int type) {
  char me[]="nrrdUnblock", err[NRRD_STRLEN_MED];
  int size[NRRD_DIM_MAX], d, map[NRRD_DIM_MAX], outElSz;

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock != nin->type) {
    sprintf(err, "%s: need input nrrd type %s", me,
	    nrrdEnumValToStr(nrrdEnumType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }
  if (NRRD_DIM_MAX == nin->dim) {
    sprintf(err, "%s: input nrrd already at dimension limit (%d)",
	    me, NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_BETWEEN(nrrdTypeUnknown, type, nrrdTypeLast)) {
    sprintf(err, "%s: invalid requested type %d", me, type);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == type && (!(0 < nout->blockSize))) {
    sprintf(err, "%s: for %s type, need nout->blockSize set", me,
	    nrrdEnumValToStr(nrrdEnumType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }
  /* this shouldn't actually be necessary ... */
  if (!(nrrdElementSize(nin))) {
    sprintf(err, "%s: nin or nout reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }

  nout->type = type;
  outElSz = nrrdElementSize(nout);
  if (nin->blockSize % outElSz) {
    sprintf(err, "%s: input blockSize (%d) not multiple of output "
	    "element size (%d)",
	    me, nin->blockSize, outElSz);
    biffAdd(NRRD, err); return 1;
  }
  for (d=0; d<=nin->dim; d++) {
    map[d] = !d ?  -1 : d-1;
    size[d] = !d ? nin->blockSize / outElSz : nin->axis[map[d]].size;
  }
  /* if nout->blockSize is needed, we've checked that its set */
  if (nrrdMaybeAlloc_nva(nout, type, nin->dim+1, size)) {
    sprintf(err, "%s: failed to allocate output", me);
    biffAdd(NRRD, err); return 1;
  }
  memcpy(nout->data, nin->data, nrrdElementNumber(nin)*nrrdElementSize(nin));
  if (nrrdAxesCopy(nout, nin, map, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s: failed to copy axes", me);
    biffAdd(NRRD, err); return 1;
  }
  
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("unblock(,,)")
			   + strlen(nin->content)
			   + strlen(nrrdEnumValToStr(nrrdEnumType, type))
			   + 11
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "unblock(%s,%s)", 
	      nin->content, nrrdEnumValToStr(nrrdEnumType, type));
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  return 0;
}

