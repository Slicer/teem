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

/* ------------------------------------------------------------ */

void
_nrrdIOInit(nrrdIO *io) {

  if (io) {
    strcpy(io->dir, "");
    strcpy(io->base, "");
    strcpy(io->line, "");
    io->pos = 0;
    io->dataFile = NULL;
    io->magic = nrrdMagicUnknown;
    io->format = nrrdFormatUnknown;
    io->encoding = nrrdDefWrtEncoding;
    io->endian = airEndianUnknown;
    io->lineSkip = 0;
    io->byteSkip = 0;
    io->seperateHeader = nrrdDefWrtSeperateHeader;
    io->bareTable = nrrdDefWrtBareTable;
    io->charsPerLine = nrrdDefWrtCharsPerLine;
    io->valsPerLine = nrrdDefWrtValsPerLine;
    memset(io->seen, 0, (NRRD_FIELD_MAX+1)*sizeof(int));
  }
}

nrrdIO *
nrrdIONew(void) {
  nrrdIO *io;
  
  io = calloc(1, sizeof(nrrdIO));
  if (io) {
    /* explicitly sets pointers to NULL */
    _nrrdIOInit(io);
  }
  return io;
}

/*
******** nrrdIOReset
**
** an attempt at resetting all but those things which it makes sense
** to re-use across multiple nrrd reads or writes.  This is somewhat
** complicated, and I haven't thought through all the possibilities...
*/
void
nrrdIOReset(nrrdIO *io) {

  /* this started as a copy of the body of _nrrdIOInit() */
  if (io) {
    strcpy(io->dir, "");
    strcpy(io->base, "");
    strcpy(io->line, "");
    io->pos = 0;
    io->dataFile = NULL;
    io->magic = nrrdMagicUnknown;
    /* io->format = nrrdDefWrtFormat; */
    /* io->encoding = nrrdDefWrtEncoding; */
    io->endian = airEndianUnknown;
    io->lineSkip = 0;
    io->byteSkip = 0;
    /* io->seperateHeader = nrrdDefWrtSeperateHeader; */
    /* io->bareTable = nrrdDefWrtBareTable; */
    memset(io->seen, 0, (NRRD_FIELD_MAX+1)*sizeof(int));
  }
}

nrrdIO *
nrrdIONix(nrrdIO *io) {

  return airFree(io);
}

/* ------------------------------------------------------------ */

void
_nrrdResampleInfoInit(nrrdResampleInfo *info) {
  int i, d;

  for (d=0; d<=NRRD_DIM_MAX-1; d++) {
    info->kernel[d] = NULL;
    info->samples[d] = 0;
    info->param[d][0] = nrrdDefRsmpScale;
    for (i=1; i<=NRRD_KERNEL_PARAMS_MAX-1; i++)
      info->param[d][i] = AIR_NAN;
    info->min[d] = info->max[d] = AIR_NAN;
  }
  info->boundary = nrrdDefRsmpBoundary;
  info->type = nrrdDefRsmpType;
  info->renormalize = nrrdDefRsmpRenormalize;
  info->clamp = nrrdDefRsmpClamp;
  info->padValue = nrrdDefRsmpPadValue;
}

nrrdResampleInfo *
nrrdResampleInfoNew(void) {
  nrrdResampleInfo *info;

  info = (nrrdResampleInfo*)(calloc(1, sizeof(nrrdResampleInfo)));
  if (info) {
    /* explicitly sets pointers to NULL */
    _nrrdResampleInfoInit(info);
  }
  return info;
}

nrrdResampleInfo *
nrrdResampleInfoNix(nrrdResampleInfo *info) {
  
  return airFree(info);
}

/* ------------------------------------------------------------ */

/* see axes.c for axis-specific "methods" */

/* ------------------------------------------------------------ */

/*
******* nrrdInit
**
** initializes a nrrd to default state.  All nrrd functions in the
** business of initializing a nrrd struct use this function.  Mostly
** just sets values to 0, NaN, "", NULL, or Unknown
*/
void
nrrdInit(Nrrd *nrrd) {
  int i;

  if (nrrd) {
    nrrd->data = airFree(nrrd->data);
    nrrd->type = nrrdTypeUnknown;
    nrrd->dim = 0;
    
    for (i=0; i<=NRRD_DIM_MAX-1; i++) {
      _nrrdAxisInit(&(nrrd->axis[i]));
    }
    
    nrrd->content = airFree(nrrd->content);
    nrrd->blockSize = 0;
    nrrd->min = nrrd->max = AIR_NAN;
    nrrd->oldMin = nrrd->oldMax = AIR_NAN;
    /* nrrd->ptr = NULL; */
    nrrd->hasNonExist = nrrdNonExistUnknown;
    
    /* the comment airArray should be already been allocated, 
       though perhaps empty */
    nrrdCommentClear(nrrd);
  }
}

/*
******** nrrdNew()
**
** creates and initializes a Nrrd
**
** this does NOT use biff
*/
Nrrd *
nrrdNew(void) {
  int i;
  Nrrd *nrrd;
  
  nrrd = (Nrrd*)(calloc(1, sizeof(Nrrd)));
  if (!nrrd)
    return NULL;

  /* explicitly set pointers to NULL */
  nrrd->data = NULL;
  nrrd->content = NULL;
  /* HEY: this is a symptom of some stupidity, no? */
  for (i=0; i<=NRRD_DIM_MAX-1; i++) {
    nrrd->axis[i].label = NULL;
  }

  /* create comment airArray (even though it starts empty) */
  nrrd->cmtArr = airArrayNew((void**)(&(nrrd->cmt)), NULL, 
			     sizeof(char *), NRRD_COMMENT_INCR);
  if (!nrrd->cmtArr)
    return NULL;
  airArrayPointerCB(nrrd->cmtArr, airNull, airFree);

  /* finish initializations */
  nrrdInit(nrrd);

  return nrrd;
}

/*
******** nrrdNix()
**
** does nothing with the array, just does whatever is needed
** to free the nrrd itself
**
** returns NULL
**
** this does NOT use biff
*/
Nrrd *
nrrdNix(Nrrd *nrrd) {
  int i;
  
  if (nrrd) {
    nrrd->content = airFree(nrrd->content);
    /* HEY: this is a symptom of some stupidity, no? */
    for (i=0; i<=NRRD_DIM_MAX-1; i++) {
      nrrd->axis[i].label = airFree(nrrd->axis[i].label);
    }
    nrrdCommentClear(nrrd);
    nrrd->cmtArr = airArrayNix(nrrd->cmtArr);
    nrrd = airFree(nrrd);
  }
  return NULL;
}

/*
******** nrrdEmpty()
**
** frees data inside nrrd AND resets all its state, so its the
** same as what comes from nrrdNew().  This includes free()ing
** any comments.
*/
Nrrd *
nrrdEmpty(Nrrd *nrrd) {
  
  if (nrrd) {
    nrrd->data = airFree(nrrd->data);
    nrrdInit(nrrd);
  }
  return nrrd;
}

/*
******** nrrdNuke()
**
** blows away the nrrd and everything inside
**
** always returns NULL
*/
Nrrd *
nrrdNuke(Nrrd *nrrd) {
  
  if (nrrd) {
    nrrdEmpty(nrrd);
    nrrdNix(nrrd);
  }
  return NULL;
}

/* ------------------------------------------------------------ */

int
_nrrdSizeValid(int dim, int *size) {
  char me[]="_nrrdSizeValid", err[NRRD_STRLEN_MED];
  int d;
  
  for (d=0; d<=dim-1; d++) {
    if (!(size[d] > 0)) {
      sprintf(err, "%s: invalid size (%d) for axis %d (of %d)",
	      me, size[d], d, dim-1);
      biffAdd(NRRD, err); return AIR_FALSE;
    }
  }
  return AIR_TRUE;
}

/*
******** nrrdWrap_nva()
**
** wraps a given Nrrd around a given array
*/
int
nrrdWrap_nva(Nrrd *nrrd, void *data, int type, int dim, int *size) {
  char me[] = "nrrdWrap_nva", err[NRRD_STRLEN_MED];
  int d;
  
  if (!(nrrd && size)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  nrrd->data = data;
  nrrd->type = type;
  nrrd->dim = dim;
  if (!_nrrdSizeValid(dim, size)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  for (d=0; d<=dim-1; d++) {
    nrrd->axis[d].size = size[d];
  }
  return 0;
}

/*
******** nrrdWrap()
**
** Minimal var args wrapper around nrrdWrap_nva, with the advantage of 
** taking all the axes sizes as the var args.
**
** This is THE BEST WAY to wrap a nrrd around existing raster data,
** assuming that the dimension is known at compile time.
**
** If successful, returns 0, otherwise, 1.
** This does use biff.
*/
int
nrrdWrap(Nrrd *nrrd, void *data, int type, int dim, ...) {
  char me[] = "nrrdWrap", err[NRRD_STRLEN_MED];
  va_list ap;
  int d, size[NRRD_DIM_MAX];
  
  if (!(nrrd && data)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  va_start(ap, dim);
  for (d=0; d<=dim-1; d++) {
    size[d] = va_arg(ap, int);
  }
  va_end(ap);
  if (!_nrrdSizeValid(dim, size)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  
  return nrrdWrap_nva(nrrd, data, type, dim, size);
}


/*
******** nrrdUnwrap()
**
** a wrapper around nrrdNix()
*/
Nrrd *
nrrdUnwrap(Nrrd *nrrd) {
  
  return nrrdNix(nrrd);
}

void
_nrrdTraverse(Nrrd *nrrd) {
  char *test, tval;
  nrrdBigInt I, N;
  int S;
  
  N = nrrdElementNumber(nrrd);
  S = nrrdElementSize(nrrd);
  tval = 0;
  test = nrrd->data;
  for (I=0; I<N*S; I++) {
    tval += test[I];
  }
}

/*
******** nrrdAlloc_nva()
**
** allocates data array and sets information.
**
** This function will always allocate more memory (via calloc), but
** it will free() nrrd->data if it is non-NULL when passed in.
**
** Note to Gordon: don't get clever and change ANY axis-specific
** information here.  It may be very convenient to set that before
** nrrdAlloc() or nrrdMaybeAlloc()
**
** Note: This function DOES use biff
*/
int 
nrrdAlloc_nva(Nrrd *nrrd, int type, int dim, int *size) {
  char me[] = "nrrdAlloc_nva", err[NRRD_STRLEN_MED];
  nrrdBigInt num;
  int d, esize;

  if (!(nrrd && size)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_BETWEEN(nrrdTypeUnknown, type, nrrdTypeLast)) {
    sprintf(err, "%s: type (%d) is invalid", me, type);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == type) {
    if (!(0 < nrrd->blockSize)) {
      sprintf(err, "%s: given nrrd->blockSize %d invalid", 
	      me, nrrd->blockSize);
      biffAdd(NRRD, err); return 1;
    }
  }
  if (!AIR_INSIDE(1, dim, NRRD_DIM_MAX)) {
    sprintf(err, "%s: dim (%d) in invalid", me, dim);
    biffAdd(NRRD, err); return 1;
  }

  nrrd->type = type;
  nrrd->data = airFree(nrrd->data);
  num = 1;
  for (d=0; d<=dim-1; d++) {
    num *= (nrrd->axis[d].size = size[d]);
  }
  esize = nrrdElementSize(nrrd);
  nrrd->data = calloc(num, esize);
  if (!(nrrd->data)) {
    sprintf(err, "%s: calloc(" NRRD_BIG_INT_PRINTF ",%d) failed", 
	    me, num, nrrdElementSize(nrrd));
    biffAdd(NRRD, err); return 1 ;
  }
  nrrd->dim = dim;

  return 0;
}

/*
******** nrrdAlloc()
**
** Handy wrapper around nrrdAlloc_nva, which takes, as its vararg list,
** all the axes sizes.
*/
int 
nrrdAlloc(Nrrd *nrrd, int type, int dim, ...) {
  char me[]="nrrdAlloc", err[NRRD_STRLEN_MED];
  int size[NRRD_DIM_MAX], d;
  va_list ap;
  
  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  va_start(ap, dim);
  for (d=0; d<=dim-1; d++) {
    size[d] = va_arg(ap, int);
  }
  va_end(ap);
  if (!_nrrdSizeValid(dim, size)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdAlloc_nva(nrrd, type, dim, size)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}


/*
******** nrrdMaybeAlloc_nva
**
** calls nrrdAlloc_nva if the requested space is different than
** what is currently held
*/
int
nrrdMaybeAlloc_nva(Nrrd *nrrd, int type, int dim, int *size) {
  char me[]="nrrdMaybeAlloc_nva", err[NRRD_STRLEN_MED];
  nrrdBigInt sizeWant, sizeHave, numWant;
  int d, need;

  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_BETWEEN(nrrdTypeUnknown, type, nrrdTypeLast)) {
    sprintf(err, "%s: type (%d) is invalid", me, type);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == type) {
    if (!(0 < nrrd->blockSize)) {
      sprintf(err, "%s: given nrrd->blockSize %d invalid", 
	      me, nrrd->blockSize);
      biffAdd(NRRD, err); return 1;
    }
  }
  if (!_nrrdSizeValid(dim, size)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }

  if (!(nrrd->data)) {
    need = 1;
  }
  else {
    numWant = 1;
    for (d=0; d<=dim-1; d++) {
      numWant *= size[d];
    }
    /* this shouldn't actually be necessary ... */
    if (!nrrdElementSize(nrrd)) {
      sprintf(err, "%s: nrrd reports zero element size!", me);
      biffAdd(NRRD, err); return 1;
    }
    sizeHave = nrrdElementNumber(nrrd) * nrrdElementSize(nrrd);
    sizeWant = numWant * nrrdElementSize(nrrd);
    need = sizeHave != sizeWant;
  }
  if (need) {
    if (nrrdAlloc_nva(nrrd, type, dim, size)) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
  }

  /* we need to set these here because if need was NOT true above,
     then these things would not be set by nrrdAlloc_nva(), but they
     need to be set in accordance with the function arguments.
     Blocksize would have been already set by caller. */
  nrrd->type = type;
  nrrd->dim = dim;

  return 0;
}

/*
******** nrrdMaybeAlloc()
**
** Handy wrapper around nrrdAlloc, which takes, as its vararg list
** all the axes sizes, thereby calculating the total number.
*/
int 
nrrdMaybeAlloc(Nrrd *nrrd, int type, int dim, ...) {
  char me[]="nrrdMaybeAlloc", err[NRRD_STRLEN_MED];
  int d, size[NRRD_DIM_MAX];
  nrrdBigInt num;
  va_list ap;
  
  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  num = 1;
  va_start(ap, dim);
  for (d=0; d<=dim-1; d++) {
    num *= (size[d] = va_arg(ap, int));
  }
  va_end(ap);
  if (!_nrrdSizeValid(dim, size)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  nrrdAxesGet_nva(nrrd, nrrdAxesInfoSize, size);
  if (nrrdMaybeAlloc_nva(nrrd, type, dim, size)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

/*
******** nrrdCopy
**
** copy method for nrrds.  nout will end up as an "exact" copy of nin.
** New space for data is allocated here, and output nrrd points to it.
** Comments from old are added to comments for new, so these are also
** newly allocated.  nout->ptr is not set, nin->ptr is not read.
*/
int
nrrdCopy(Nrrd *nout, Nrrd *nin) {
  char me[]="nrrdCopy", err[NRRD_STRLEN_MED];
  int size[NRRD_DIM_MAX];

  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nout == nin) {
    /* I guess there's nothing to do! */
    return 0;
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nin)) {
    sprintf(err, "%s: input nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }
  nrrdAxesGet_nva(nin, nrrdAxesInfoSize, size);
  if (nrrdMaybeAlloc_nva(nout, nin->type, nin->dim, size)) {
    sprintf(err, "%s: couldn't allocate data", me);
    biffAdd(NRRD, err); return 1;
  }
  memcpy(nout->data, nin->data, nrrdElementNumber(nin)*nrrdElementSize(nin));
  nrrdAxesCopy(nout, nin, NULL, NRRD_AXESINFO_NONE);

  nout->content = airStrdup(nin->content);
  if (nin->content && !nout->content) {
    sprintf(err, "%s: couldn't copy content", me);
    biffAdd(NRRD, err); return 1;
  }
  nout->blockSize = nin->blockSize;
  nout->min = nin->min;
  nout->max = nin->max;
  nout->oldMin = nin->oldMin;
  nout->oldMax = nin->oldMax;
  /* nout->ptr = nin->ptr; */
    
  if (nrrdCommentCopy(nout, nin)) {
    sprintf(err, "%s: trouble copying comments", me);
    biffAdd(NRRD, err); return 1;
  }

  return 0;
}

/*
******** nrrdPPM()
**
** for making a nrrd suitable for holding PPM data
*/
int
nrrdPPM(Nrrd *ppm, int sx, int sy) {
  char me[]="nrrdPPM", err[NRRD_STRLEN_MED];

  if (!(sx > 0 && sy > 0)) {
    sprintf(err, "%s: got invalid sizes (%d,%d)", me, sx, sy);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdMaybeAlloc(ppm, nrrdTypeUChar, 3, 3, sx, sy)) {
    sprintf(err, "%s: couldn't allocate %d x %d 24-bit image", me, sx, sy);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

/*
******** nrrdPGM()
**
** for making a nrrd suitable for holding PGM data
*/
int
nrrdPGM(Nrrd *pgm, int sx, int sy) {
  char me[]="nrrdNewPGM", err[NRRD_STRLEN_MED];

  if (!(sx > 0 && sy > 0)) {
    sprintf(err, "%s: got invalid sizes (%d,%d)", me, sx, sy);
    biffAdd(NRRD, err);
    return 1;
  }
  if (nrrdMaybeAlloc(pgm, nrrdTypeUChar, 2, sx, sy)) {
    sprintf(err, "%s: couldn't allocate %d x %d 8-bit image", me, sx, sy);
    biffAdd(NRRD, err);
    return 1;
  }
  return 0;
}

/*
******** nrrdTable()
**
** for making a nrrd suitable for holding "table" data
*/
int
nrrdTable(Nrrd *table, int sx, int sy) {
  char me[]="nrrdTable", err[NRRD_STRLEN_MED];

  if (!(sx > 0 && sy > 0)) {
    sprintf(err, "%s: got invalid sizes (%d,%d)", me, sx, sy);
    biffAdd(NRRD, err);
    return 1;
  }
  if (nrrdMaybeAlloc(table, nrrdTypeFloat, 2, sx, sy)) {
    sprintf(err, "%s: couldn't allocate %d x %d table of floats", me, sx, sy);
    biffAdd(NRRD, err);
    return 1;
  }
  return 0;
}
