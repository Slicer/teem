/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/


#include "nrrd.h"

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

  /* now for the hard part */
  for (i=0; i<=n-1; i++) {
    invp[p[i]] = i;
  }

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
** factor of 5 speed up.
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
  nrrdBigInt 
    srcI, dstI,              /* indices into input and output nrrds */
    lineSize,                /* size of block of memory which can be
				moved contiguously from input to output,
				thought of as a "scanline" */
    numLines;                /* how many "scanlines" there are to permute */
  char *src, *dest;
  int coord[NRRD_DIM_MAX+1], /* coordinates in output nrrd */
    ip[NRRD_DIM_MAX+1],      /* inverse of permutation in "axes" */
    topFax,                  /* highest axis which is fixed in permutation */
    d,                       /* running index along dimensions */
    dim;                     /* copy of nin->dim */

  if (!(nin && nout && axes)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdInvertPerm(ip, axes, nin->dim)) {
    sprintf(err, "%s: couldn't compute axis permutation inverse", me);
    biffAdd(NRRD, err); return 1;
  }
  
  dim = nin->dim;
  for (d=0; d<=dim-1 && axes[d] == d; d++)
    ;
  topFax = d-1;

  if (topFax == dim-1) {
    /* we were given the identity permutation, just copy whole thing */
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s: trouble copying input", me);
      biffAdd(NRRD, err); return 1;      
    }
    return 0;
  }
  
  /* else topFax < dim-1 (actually, topFax < dim-2) */
  
  /* set information in new volume */
  if (nrrdMaybeAlloc(nout, nin->num, nin->type, nin->dim)) {
    sprintf(err, "%s: failed to allocate output", me);
    biffAdd(NRRD, err); return 1;
  }
  
  lineSize = 1;
  for (d=0; d<=topFax; d++) {
    lineSize *= nin->axis[d].size;
  }
  numLines = nin->num/lineSize;
  lineSize *= nrrdElementSize(nin);
  src = nin->data;
  dest = nout->data;
  memset(coord, 0, NRRD_DIM_MAX*sizeof(int));
  /* we march through linear index space of input nrrd */
  for (srcI=0; srcI<=numLines-1; srcI++) {
    /* from coordinates in output nrrd, find linear index into input */
    dstI = coord[dim-1];
    for (d=dim-2; d>topFax; d--)
      dstI = coord[d] + nout->axis[d].size*dstI;

    /* copy */
    /* memcpy(dest + dstI*elSize, src + srcI*elSize, elSize); */
    memcpy(dest + dstI*lineSize, src + srcI*lineSize, lineSize);

    /* increment coordinates in output nrrd */
    d = topFax+1;
    coord[ip[d]]++;
    while (d <= dim-1 && coord[ip[d]] == nout->axis[ip[d]].size) {
      coord[ip[d]] = 0;
      if (++d <= dim-1) 
	coord[ip[d]]++; 
    }
  }

  if (nrrdAxesCopy(nout, nin, axes, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s: trouble", me);
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
  nout->blockSize = nin->blockSize;
  nout->min = nin->min;
  nout->max = nin->max;
  nout->oldMin = nin->oldMin;
  nout->oldMax = nin->oldMax;

  /* bye */
  return 0;
}

/*
******** nrrdSwapAxes()
**
** for when you just want to switch the order of two axes, without
** going through the trouble of creating the permutation array 
** need to call nrrdPermuteAxes()
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
    sprintf(err, "%s: trouble swapping axes", me);
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
  char me[]="nrrdShuffle", err[NRRD_STRLEN_MED];
  int typesize, size[NRRD_DIM_MAX], d, dim, 
    ci[NRRD_DIM_MAX+1], co[NRRD_DIM_MAX+1];
  nrrdBigInt I, idxI, idxO, N;
  unsigned char *dataI, *dataO;

  if (!(nin && nout && perm)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err);
    return 1;
  }
  if (!AIR_INSIDE(0, axis, nin->dim-1)) {
    sprintf(err, "%s: axis %d outside valid range [0,%d]", 
	    me, axis, nin->dim-1);
    biffAdd(NRRD, err);
    return 1;
  }
  if (!(nout->data)) {
    /* HEY!!: this is just a hack for the time being */
    /* though it does save us the trouble of copying all the fields
       which don't change as part of the shuffle */
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s: failed to allocate output", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  
  N = nin->num;
  nrrdAxesGet(nin, nrrdAxesInfoSize, size);
  dim = nin->dim;
  dataI = nin->data;
  dataO = nout->data;
  typesize = nrrdElementSize(nin);
  memset(ci, 0, (NRRD_DIM_MAX+1)*sizeof(int));
  memset(co, 0, (NRRD_DIM_MAX+1)*sizeof(int));
  for (I=0; I<=N-1; I++) {
    memcpy(ci, co, NRRD_DIM_MAX*sizeof(int));
    ci[axis] = perm[co[axis]];
    idxI = ci[dim-1];
    idxO = co[dim-1];
    for (d=dim-2; d>=0; d--) {
      idxI = ci[d] + size[d]*idxI;
      idxO = co[d] + size[d]*idxO;
    }
    AIR_MEMCPY(dataO + typesize*idxO, dataI + typesize*idxI, typesize);
    d = 0;
    co[d]++;
    while (d <= dim-1 && co[d] == size[d]) {
      co[d] = 0;
      d++;
      co[d]++; 
    }
  }

  /* peripheral information */
  if (nrrdAxesCopy(nout, nin, NULL, NRRD_AXESINFO_MINMAX)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("shuffle()")
			   + strlen(nin->content)
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "shuffle(%s)", nin->content);
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  
  nout->axis[axis].min = nout->axis[axis].max = AIR_NAN;
  return 0;
}

/*
******** nrrdJoin()
**
** this leaks memory on error.  Still waiting for the "mop" library
**
** HEY: decide if spacing stuff needs setting
*/
int
nrrdJoin(Nrrd *nout, Nrrd **nin, int num, int axis, int incrDim) {
  char me[]="nrrdJoin", err[NRRD_STRLEN_MED];
  int mindim, maxdim, diffdim, outdim, map[NRRD_DIM_MAX],
    i, d, *trs, outlen, permute[NRRD_DIM_MAX];
  nrrdBigInt outnum, chunksize;
  char *tmpdata;
  Nrrd *nperm, **ninperm;

  /* error checking */
  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(num >= 1 && axis >= 0)) {
    sprintf(err, "%s: num (%d) or axis (%d) wacky", me, num, axis);
    biffAdd(NRRD, err); return 1;
  }
  /* HEY: think through the check on upper dimension limit!!! */
  if (axis >= NRRD_DIM_MAX) {
    sprintf(err, "%s: can't join along axis %d when NRRD_DIM_MAX=%d",
	    me, axis, NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 1;    
  }
  for (i=0; i<=num-1; i++) {
    if (!(nin[i])) {
      sprintf(err, "%s: input nrrd #%d NULL", me, i);
      biffAdd(NRRD, err); return 1;
    }
  }
  ninperm = calloc(num, sizeof(Nrrd *));
  trs = calloc(2*num, sizeof(int));
  if (!(ninperm && trs)) {
    sprintf(err, "%s: couldn't alloc tmp arrays!", me);
    biffAdd(NRRD, err); return 1;
  }

  mindim = NRRD_DIM_MAX+1;
  maxdim = -1;
  for (i=0; i<=num-1; i++) {
    mindim = AIR_MIN(mindim, nin[i]->dim);
    maxdim = AIR_MAX(maxdim, nin[i]->dim);
  }
  diffdim = maxdim - mindim;
  if (diffdim > 1) {
    sprintf(err, "%s: will only reshape up one dimension (not %d)",
	    me, diffdim);
    biffAdd(NRRD, err); return 1;
  }
  if (axis > maxdim) {
    sprintf(err, "%s: can't join along axis %d with highest input dim = %d",
	    me, axis, maxdim);
    biffAdd(NRRD, err); return 1;
  }
  
  /* figure out dimension of output (outdim) */
  if (diffdim) {
    /* case A */
    outdim = maxdim;
  }
  else {
    /* diffdim == 0 */
    if (axis == maxdim) {
      /* case B */
      outdim = maxdim+1;
    }
    else {
      /* case C: axis < maxdim; maxdim == mindim */
      /* case C1: simple join, outdim = maxdim */
      /* case C2: stitch: outdim = maxdim+1 */
      outdim = incrDim ? maxdim + 1 : maxdim;
    }
  }
  
  /* do tacit reshaping, and possibly permuting, as needed */
  for (i=0; i<=outdim-1; i++) {
    permute[i] = (i < axis
		  ? i 
		  : (i < outdim-1
		     ? i + 1
		     : axis));
    /* fprintf(stderr, "%s: 1st permute[%d] = %d\n", me, i, permute[i]); */
  }
  for (i=0; i<=num-1; i++) {
    trs[0 + 2*i] = outdim - nin[i]->dim;
    if (trs[0 + 2*i]) {
      /* we do a tacit reshaping, which actually includes
	 a tacit permuting, so we don't have to call permute
	 on the parts that don't actually need it */
      trs[1 + 2*i] = nin[i]->axis[NRRD_DIM_MAX-1].size;
      for (d=NRRD_DIM_MAX-1; d>=mindim+1; d--) {
	nin[i]->axis[d].size = nin[i]->axis[d-1].size;
      }
      nin[i]->axis[mindim].size = 1;
      nin[i]->dim++;
      /* 
      fprintf(stderr, "%s: reshaped part %d -> ", me, i);
      for (d=0; d<=nin[i]->dim-1; d++) {
	fprintf(stderr, "%03d ", nin[i]->axis[d].size);
      }
      fprintf(stderr, "\n");
      */
      ninperm[i] = nin[i];
    }
    else {
      /* on this part, we permute (no need for a reshape) */
      ninperm[i] = nrrdNew();
      if (nrrdPermuteAxes(ninperm[i], nin[i], permute)) {
	sprintf(err, "%s: trouble permuting input part %d", me, i);
	biffAdd(NRRD, err); return 1;
      }
    }
  }

  /* make sure all parts are compatible in type and shape,
     determine length of final output along axis (outlen) */
  outlen = 0;
  for (i=0; i<=num-1; i++) {
    if (ninperm[i]->type != ninperm[0]->type) {
      sprintf(err, "%s: type (%d) of part %d unlike first's (%d)",
	      me, ninperm[i]->type, i, ninperm[0]->type);
      biffAdd(NRRD, err); return 1;
    }
    if (!(nrrdElementSize(ninperm[i]) > 0)) {
      sprintf(err, "%s: got wacky elements size (%d) for part %d",
	      me, nrrdElementSize(ninperm[i]), i);
      biffAdd(NRRD, err); return 1;
    }
    /* HEY: blocks of differing sizes will slip by here */
    /* fprintf(stderr, "%s: part %03d shape: ", me, i); */
    for (d=0; d<=outdim-2; d++) {
      /* fprintf(stderr, "%03d ", ninperm[i]->axis[d].size); */
      if (ninperm[i]->axis[d].size != ninperm[0]->axis[d].size) {
	sprintf(err, "%s: axis %d size (%d) of part %d unlike first's (%d)",
		me, d, ninperm[i]->axis[d].size, i, ninperm[0]->axis[d].size);
	biffAdd(NRRD, err); return 1;
      }
    }
    /* fprintf(stderr, "%03d\n", ninperm[i]->axis[outdim-1].size); */
    outlen += ninperm[i]->axis[outdim-1].size;
  }
  /* fprintf(stderr, "%s: outlen = %d\n", me, outlen); */

  /* allocate temporary nrrd and concat input into it */
  outnum = 1;
  for (d=0; d<=outdim-2; d++) {
    outnum *= ninperm[0]->axis[d].size;
  }
  outnum *= outlen;
  if (nrrdAlloc(nperm = nrrdNew(), outnum, ninperm[0]->type, outdim)) {
    sprintf(err, "%s: trouble allocating temporory nrrd", me);
    biffAdd(NRRD, err); return 1;
  }
  tmpdata = nperm->data;
  for (i=0; i<=num-1; i++) {
    /* here is where the actual joining happens */
    chunksize = ninperm[i]->num*nrrdElementSize(ninperm[i]);
    memcpy(tmpdata, ninperm[i]->data, chunksize);
    tmpdata += chunksize;
  }
  
  /* copy other axis-specific fields from nin[0] to nperm */
  for (d=0; d<=outdim-2; d++)
    map[d] = d;
  map[outdim-1] = -1;
  nrrdAxesCopy(nperm, ninperm[0], map, NRRD_AXESINFO_NONE);
  nperm->axis[outdim-1].size = outlen;

  /* do the permutation required to get output in right order */
  for (i=0; i<=outdim-1; i++) {
    permute[i] = (i < axis 
		  ? i 
		  : (i == axis
		     ? outdim-1
		     : i - 1));
    /* fprintf(stderr, "%s: 2nd permute[%d] = %d\n", me, i, permute[i]); */
  }
  if (nrrdPermuteAxes(nout, nperm, permute)) {
    sprintf(err, "%s: error permuting temporary nrrd", me);
    biffAdd(NRRD, err); return 1;
  }

  /* undo the trickery involved in tacit reshaping/permuting */
  for (i=0; i<=num-1; i++) {
    if (trs[0 + 2*i]) {
      for (d=mindim+1; d<=NRRD_DIM_MAX-1; d++) {
	nin[i]->axis[d-1].size = nin[i]->axis[d].size;
      }
      nin[i]->axis[NRRD_DIM_MAX-1].size = trs[1 + 2*i];
      nin[i]->dim--;
    }
  }

  /* clean up */
  trs = airFree(trs);
  for (i=0; i<=num-1; i++) {
    /* we only nuke the nrrds we created */
    if (ninperm[i] != nin[i]) {
      ninperm[i] = nrrdNuke(ninperm[i]);
    }
  }
  ninperm = airFree(ninperm);
  nperm = nrrdNuke(nperm);

  return 0;
}

/*
******** nrrdFlip()
**
** reverse the order of slices along the given axis.
** Actually, just a wrapper around nrrdShuffle()
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

/*
******** nrrdReshape()
**
*/
int
nrrdReshape(Nrrd *nout, Nrrd *nin, int dim, int *size) {
  char me[]="nrrdReshape", err[NRRD_STRLEN_MED],
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
  if (numOut != nin->num) {
    sprintf(err, "%s: new sizes product (" NRRD_BIG_INT_PRINTF ") "
	    "!= # elements (" NRRD_BIG_INT_PRINTF ")",
	    me, numOut, nin->num);
    biffAdd(NRRD, err); return 1;
  }

  /* HEY: non-essential, non-axis info (like comments) has been copied,
     perhaps that's not appropriate */
  if (nrrdCopy(nout, nin)) {
    sprintf(err, "%s: trouble", me);
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

int
nrrdReshape_va(Nrrd *nout, Nrrd *nin, int dim, ...) {
  char me[]="nrrdReshape_va", err[NRRD_STRLEN_MED];
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

  if (nrrdReshape(nout, nin, dim, size)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  
  return 0;
}

int
nrrdBlock(Nrrd *nout, Nrrd *nin) {
  char me[]="nrrdBlock", err[NRRD_STRLEN_MED];
  int d, numEl, map[NRRD_DIM_MAX];

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (1 == nin->dim) {
    sprintf(err, "%s: can't blockify 1-D nrrd", me);
    biffAdd(NRRD, err); return 1;
  }

  numEl = nin->axis[0].size;;
  nout->blockSize = numEl*nrrdElementSize(nin);
  for (d=0; d<=nin->dim-2; d++) {
    map[d] = d+1;
  }
  
  if (nrrdMaybeAlloc(nout, nin->num/numEl, nrrdTypeBlock, nin->dim-1)) {
    sprintf(err, "%s: failed to allocate output", me);
    biffAdd(NRRD, err); return 1;
  }
  memcpy(nout->data, nin->data, nin->num*nrrdElementSize(nin));
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

int
nrrdUnblock(Nrrd *nout, Nrrd *nin, int type) {
  char me[]="nrrdUnblock", err[NRRD_STRLEN_MED];
  int size, d, map[NRRD_DIM_MAX], outElSz;

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

  nout->type = type;
  outElSz = nrrdElementSize(nout);
  if (nin->blockSize % outElSz) {
    sprintf(err, "%s: in blockSize (%d) not multiple of out element size (%d)",
	    me, nin->blockSize, outElSz);
    biffAdd(NRRD, err); return 1;
  }
  size = nin->blockSize / outElSz;
  for (d=0; d<=nin->dim; d++) {
    map[d] = !d ? -1 : d-1;
  }
  if (nrrdMaybeAlloc(nout, nin->num*size, type, nin->dim+1)) {
    sprintf(err, "%s: failed to allocate output", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdAxesCopy(nout, nin, map, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s: failed to copy axes", me);
    biffAdd(NRRD, err); return 1;
  }
  _nrrdAxisInit(&(nout->axis[0]));
  nout->axis[0].size = size;
  
  nout->content = airFree(nout->content);
  if (nin->content) {
    nout->content = calloc(strlen("unblock(,,)")
			   + strlen(nin->content)
			   + strlen(nrrdEnumValToStr(nrrdEnumType, type))
			   + 11
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "block(%s,%s,%d)", 
	      nin->content, nrrdEnumValToStr(nrrdEnumType, type), size);
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  return 0;
}

