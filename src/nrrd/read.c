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
#include <teemPng.h>

#if TEEM_BZIP2
#include <bzlib.h>
#endif
#if TEEM_PNG
#include <png.h>
#endif

#include <teem32bit.h>

char _nrrdRelDirFlag[] = "./";
char _nrrdFieldSep[] = " \t";
char _nrrdTableSep[] = " ,\t";

/*
** _nrrdOneLine
**
** wrapper around airOneLine; does re-allocation of line buffer
** ("line") in the NrrdIO if needed.  The return value semantics
** are similar, except that what airOneLine would return, we put
** in *lenP.  If there is an error (airOneLine returned -1, 
** something couldn't be allocated), *lenP is set to 0, and 
** we return 1.  HITTING EOF IS NOT ACTUALLY AN ERROR, see code
** below.  Otherwise we return 0.
**
** Does use biff
*/
int
_nrrdOneLine (int *lenP, NrrdIO *io, FILE *file) {
  char me[]="_nrrdOneLine", err[AIR_STRLEN_MED], **line;
  airArray *lineArr;
  int len, lineIdx;

  if (!( lenP && file && io )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (0 == io->lineLen) {
    /* io->line hasn't been allocated for anything */
    io->line = malloc(3);
    io->lineLen = 3;
  }
  len = airOneLine(file, io->line, io->lineLen);
  if (len <= io->lineLen) {
    if (-1 == len) {
      sprintf(err, "%s: invalid args to airOneLine()", me);
      biffAdd(NRRD, err); *lenP = 0; return 1;
    }
    /* otherwise we hit EOF before a newline, or the line (possibly empty)
       fit within the io->line, neither of which is an error here */
    *lenP = len;
    return 0;
  }
  /* else line didn't fit in buffer, so we have to increase line
     buffer size and put the line together in pieces */
  lineArr = airArrayNew((void**)(&line), NULL, sizeof(char *), 1);
  if (!lineArr) {
    sprintf(err, "%s: couldn't allocate airArray", me);
    biffAdd(NRRD, err); *lenP = 0; return 1;
  }
  airArrayPointerCB(lineArr, airNull, airFree);
  while (len == io->lineLen+1) {
    lineIdx = airArrayIncrLen(lineArr, 1);
    if (-1 == lineIdx) {
      sprintf(err, "%s: couldn't increment line buffer array", me);
      biffAdd(NRRD, err); *lenP = 0; return 1;
    }
    line[lineIdx] = io->line;
    io->lineLen *= 2;
    io->line = (char*)malloc(io->lineLen);
    if (!io->line) {
      sprintf(err, "%s: couldn't alloc %d-char line\n", me, io->lineLen);
      biffAdd(NRRD, err); *lenP = 0; return 1;
    }
    len = airOneLine(file, io->line, io->lineLen);
  }
  /* last part did fit in io->line buffer, also save this into line[] */
  lineIdx = airArrayIncrLen(lineArr, 1);
  if (-1 == lineIdx) {
    sprintf(err, "%s: couldn't increment line buffer array", me);
    biffAdd(NRRD, err); *lenP = 0; return 1;
  }
  line[lineIdx] = io->line;
  io->lineLen *= 3;  /* for good measure */
  io->line = (char*)malloc(io->lineLen);
  if (!io->line) {
    sprintf(err, "%s: couldn't alloc %d-char line\n", me, io->lineLen);
    biffAdd(NRRD, err); *lenP = 0; return 1;
  }
  /* now concatenate everything into a new io->line */
  strcpy(io->line, "");
  for (lineIdx=0; lineIdx<lineArr->len; lineIdx++) {
    strcat(io->line, line[lineIdx]);
  }
  airArrayNuke(lineArr);
  *lenP = strlen(io->line) + 1;
  return 0;
}

/*
** _nrrdValidHeader()
**
** consistency checks on relationship between fields of nrrd, (only)
** to be used after the headers is parsed, and before the data is read
**
*/
int
_nrrdValidHeader (Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdValidHeader", err[AIR_STRLEN_MED];
  int i;

  for (i=1; i<=NRRD_FIELD_MAX; i++) {
    if (!_nrrdFieldRequired[i])
      continue;
    if (!io->seen[i]) {
      sprintf(err, "%s: didn't see required field: %s",
	      me, airEnumStr(nrrdField, i));
      biffAdd(NRRD, err); return 0;
    }
  }
  if (nrrdTypeBlock == nrrd->type && 0 == nrrd->blockSize) {
    sprintf(err, "%s: type is %s, but missing field: %s", me,
	    airEnumStr(nrrdType, nrrdTypeBlock),
	    airEnumStr(nrrdField, nrrdField_block_size));
    biffAdd(NRRD, err); return 0;
  }
  /* this shouldn't actually be necessary ... */
  /* really? it saves all the data readers from doing it ... */
  if (!nrrdElementSize(nrrd)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 0;
  }
  /* _nrrdReadNrrdParse_sizes() checks axis[i].size, which completely
     determines the return of nrrdElementNumber() */
  if (airEndianUnknown == io->endian &&
      nrrdEncodingEndianMatters[io->encoding] &&
      1 != nrrdElementSize(nrrd)) {
    sprintf(err, "%s: type (%s) and encoding (%s) require %s info", me,
	    airEnumStr(nrrdType, nrrd->type),
	    airEnumStr(nrrdEncoding, io->encoding),
	    airEnumStr(nrrdField, nrrdField_endian));
    biffAdd(NRRD, err); return 0;    
  }

  /* we don't really try to enforce consistency with the
     min/max/center/size information on each axis, other than the
     value checking done by the _nrrdReadNrrdParse_* functions,
     because we only really care that we know each axis size.  Past
     that, if the user messes it up, its not really our problem ... */

  return 1;
}

/*
** _nrrdCalloc()
**
** allocates the data for the array.  Only to be called by data readers,
** since it assume the validity of size information, as enforced by
** _nrrdValidHeader().
*/
int
_nrrdCalloc (Nrrd *nrrd) {
  char me[]="_nrrdCalloc", err[AIR_STRLEN_MED];

  AIR_FREE(nrrd->data);
  nrrd->data = calloc(nrrdElementNumber(nrrd), nrrdElementSize(nrrd));
  if (!nrrd->data) {
    sprintf(err, "%s: couldn't calloc(" _AIR_SIZE_T_FMT
	    ", %d)", me, nrrdElementNumber(nrrd), nrrdElementSize(nrrd));
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

int
_nrrdReadDataRaw (Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdReadDataRaw", err[AIR_STRLEN_MED];
  size_t num, bsize, size, ret, dio;
  
  num = nrrdElementNumber(nrrd);
  bsize = num * nrrdElementSize(nrrd);
  size = bsize;
  if (num != bsize/nrrdElementSize(nrrd)) {
    fprintf(stderr,
	    "%s: PANIC: \"size_t\" can't represent byte-size of data.\n", me);
    exit(1);
  }

  if (_nrrdFormatUsesDIO[io->format]) {
    dio = airDioTest(size, io->dataFile, NULL);
  } else {
    dio = airNoDio_format;
  }
  if (airNoDio_okay == dio) {
    if (_nrrdFormatUsesDIO[io->format]) {
      if (3 <= nrrdStateVerboseIO) {
	fprintf(stderr, "with direct I/O ");
      }
      if (2 <= nrrdStateVerboseIO) {
	fprintf(stderr, "... ");
	fflush(stderr);
      }
    }
    /* airDioRead includes the memory allocation */
    ret = airDioRead(io->dataFile, &(nrrd->data), size);
    if (size != ret) {
      sprintf(err, "%s: airDioRead() failed", me);
      biffAdd(NRRD, err); return 1;
    }
  } else {
    if (_nrrdCalloc(nrrd)) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
    if (AIR_DIO && _nrrdFormatUsesDIO[io->format]) {
      if (3 <= nrrdStateVerboseIO) {
	fprintf(stderr, "with fread()");
	if (4 <= nrrdStateVerboseIO) {
	  fprintf(stderr, " (why no DIO: %s)", airNoDioErr(dio));
	}
      }
      if (2 <= nrrdStateVerboseIO) {
	fprintf(stderr, " ... ");
	fflush(stderr);
      }
    }
    ret = fread(nrrd->data, nrrdElementSize(nrrd), num, io->dataFile);
    if (ret != num) {
      sprintf(err, "%s: fread() got only " _AIR_SIZE_T_FMT " %d-byte things, "
	      "not " _AIR_SIZE_T_FMT ,
	      me, ret, nrrdElementSize(nrrd), num);
      biffAdd(NRRD, err); return 1;
    }
  }

  return 0;
}

/*
** -2: not allowed, error
** -1: whitespace
** [0,15]: values
*/
int
_nrrdReadHexTable[128] = {
/* 0   1   2   3   4   5   6   7   8   9 */
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -1,  /*   0 */
  -1, -1, -1, -1, -2, -2, -2, -2, -2, -2,  /*  10 */
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,  /*  20 */
  -2, -2, -1, -2, -2, -2, -2, -2, -2, -2,  /*  30 */
  -2, -2, -2, -2, -2, -2, -2, -2,  0,  1,  /*  40 */
   2,  3,  4,  5,  6,  7,  8,  9, -2, -2,  /*  50 */
  -2, -2, -2, -2, -2, 10, 11, 12, 13, 14,  /*  60 */
  15, -2, -2, -2, -2, -2, -2, -2, -2, -2,  /*  70 */
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,  /*  80 */
  -2, -2, -2, -2, -2, -2, -2, 10, 11, 12,  /*  90 */
  13, 14, 15, -2, -2, -2, -2, -2, -2, -2,  /* 100 */
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,  /* 110 */
  -2, -2, -2, -2, -2, -2, -2, -2           /* 120 */
};

int
_nrrdReadDataHex (Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdReadDataHex", err[AIR_STRLEN_MED];
  size_t nibIdx, nibNum;
  unsigned char *data;
  int car=0, nib;

  if (_nrrdCalloc(nrrd)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  data = nrrd->data;
  nibIdx = 0;
  nibNum = 2*nrrdElementNumber(nrrd)*nrrdElementSize(nrrd);
  if (nibNum/nrrdElementNumber(nrrd) != 2*nrrdElementSize(nrrd)) {
    sprintf(err, "%s: size_t can't hold 2*(#bytes in array)\n", me);
    biffAdd(NRRD, err); return 1;
  }
  while (nibIdx < nibNum) {
    car = fgetc(io->dataFile);
    if (EOF == car) break;
    nib = _nrrdReadHexTable[car & 127];
    if (-2 == nib) {
      /* not a valid hex character */
      break;
    }
    if (-1 == nib) {
      /* its white space */
      continue;
    }
    *data += nib << (4*(1-(nibIdx & 1)));
    data += nibIdx & 1;
    nibIdx++;
  }
  if (nibIdx != nibNum) {
    if (EOF == car) {
      sprintf(err, "%s: hit EOF getting "
	      "byte " _AIR_SIZE_T_FMT " of " _AIR_SIZE_T_FMT,
	      me, nibIdx/2, nibNum/2);
    } else {
      sprintf(err, "%s: hit invalid character ('%c') getting "
	      "byte " _AIR_SIZE_T_FMT " of " _AIR_SIZE_T_FMT,
	      me, car, nibIdx/2, nibNum/2);
    }
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}


int
_nrrdReadDataAscii (Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdReadDataAscii", err[AIR_STRLEN_MED],
    numbStr[AIR_STRLEN_HUGE];  /* HEY: fix this */
  size_t I, num;
  char *data;
  int size, tmp;
  
  if (nrrdTypeBlock == nrrd->type) {
    sprintf(err, "%s: can't read nrrd type %s from ascii", me,
	    airEnumStr(nrrdType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }
  num = nrrdElementNumber(nrrd);
  if (_nrrdCalloc(nrrd)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  data = nrrd->data;
  size = nrrdElementSize(nrrd);
  for (I=0; I<num; I++) {
    if (1 != fscanf(io->dataFile, "%s", numbStr)) {
      sprintf(err, "%s: couldn't parse element " _AIR_SIZE_T_FMT
	      " of " _AIR_SIZE_T_FMT, me, I+1, num);
      biffAdd(NRRD, err); return 1;
    }
    if (nrrd->type >= nrrdTypeInt) {
      /* sscanf supports putting value directly into this type */
      if (1 != airSingleSscanf(numbStr, nrrdTypeConv[nrrd->type], 
			       (void*)(data + I*size))) {
	sprintf(err, "%s: couln't parse %s " _AIR_SIZE_T_FMT
		" of " _AIR_SIZE_T_FMT " (\"%s\")", me,
		airEnumStr(nrrdType, nrrd->type),
		I+1, num, numbStr);
	biffAdd(NRRD, err); return 1;
      }
    } else {
      /* sscanf value into an int first */
      if (1 != airSingleSscanf(numbStr, "%d", &tmp)) {
	sprintf(err, "%s: couln't parse element " _AIR_SIZE_T_FMT
		" of " _AIR_SIZE_T_FMT " (\"%s\")",
		me, I+1, num, numbStr);
	biffAdd(NRRD, err); return 1;
      }
      nrrdIInsert[nrrd->type](data, I, tmp);
    }
  }
  
  return 0;
}

int
_nrrdReadDataGzip (Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdReadDataGzip", err[AIR_STRLEN_MED];
#if TEEM_ZLIB
  size_t num, bsize, size, total_read;
  int block_size, i, error=0;
  unsigned int read;
  char *data;
  gzFile gzfin;
  
  num = nrrdElementNumber(nrrd);
  bsize = num * nrrdElementSize(nrrd);
  size = bsize;
  if (num != bsize/nrrdElementSize(nrrd)) {
    fprintf(stderr,
	    "%s: PANIC: \"size_t\" can't represent byte-size of data.\n", me);
    exit(1);
  }

  /* Allocate memory for the incoming data. */
  if (_nrrdCalloc(nrrd)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }

  /* Create the gzFile for reading in the gzipped data. */
  if ((gzfin = _nrrdGzOpen(io->dataFile, "r")) == Z_NULL) {
    /* there was a problem */
    sprintf(err, "%s: error opening gzFile", me);
    biffAdd(NRRD, err);
    return 1;
  }

  /* Here is where we do the byte skipping. */
  for(i = 0; i < io->byteSkip; i++) {
    unsigned char b;
    /* Check to see if a single byte was able to be read. */
    if (_nrrdGzRead(gzfin, &b, 1, &read) != 0 || read != 1) {
      sprintf(err, "%s: hit an error skipping byte %d of %d",
	      me, i, io->byteSkip);
      biffAdd(NRRD, err);
      return 1;
    }
  }
  
  /* zlib can handle data sizes up to UINT_MAX, so we can't just 
     pass in the size, because it might be too large for an 
     unsigned int.  Therefore it must be read in chunks 
     if the size is larger than UINT_MAX. */
  if (size <= UINT_MAX) {
    block_size = (unsigned int)size;
  } else {
    block_size = UINT_MAX;
  }

  /* This counter will help us to make sure that we read as much data
     as we think we should. */
  total_read = 0;
  /* Pointer to the blocks as we read them. */
  data = nrrd->data;
  
  /* Ok, now we can begin reading. */
  while ((error = _nrrdGzRead(gzfin, data, block_size, &read)) == 0 && read > 0) {
    /* Increment the data pointer to the next available spot. */
    data += read; 
    total_read += read;
    /* We only want to read as much data as we need, so we need to check
       to make sure that we don't request data that might be there but that
       we don't want.  This will reduce block_size when we get to the last
       block (which may be smaller than block_size).
    */
    if (size - total_read < block_size)
      block_size = (unsigned int)(size - total_read);
  }

  /* Check if we stopped because of an error. */
  if (error != 0)
  {
    sprintf(err, "%s: error reading from gzFile", me);
    biffAdd(NRRD, err);
    return 1;
  }

  /* Close the gzFile.  Since _nrrdGzClose does not close the FILE* we
     will not encounter problems when io->dataFile is closed later. */
  if (_nrrdGzClose(gzfin) != 0) {
    sprintf(err, "%s: error closing gzFile", me);
    biffAdd(NRRD, err);
    return 1;
  }
  
  /* Check to see if we got out as much as we thought we should. */
  if (total_read != size) {
    sprintf(err, "%s: expected " _AIR_SIZE_T_FMT " bytes and received "
	    _AIR_SIZE_T_FMT " bytes",
	    me, size, total_read);
    biffAdd(NRRD, err);
    return 1;
  }
  
  return 0;
#else
  sprintf(err, "%s: sorry, this nrrd not compiled with gzip enabled", me);
  biffAdd(NRRD, err); return 1;
#endif
}

int
_nrrdReadDataBzip2 (Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdReadDataBzip2", err[AIR_STRLEN_MED];
#if TEEM_BZIP2
  size_t num, bsize, size, total_read;
  int block_size, read, i, bzerror=BZ_OK;
  char *data;
  BZFILE* bzfin;
  
  num = nrrdElementNumber(nrrd);
  bsize = num * nrrdElementSize(nrrd);
  size = bsize;
  if (num != bsize/nrrdElementSize(nrrd)) {
    fprintf(stderr,
	    "%s: PANIC: \"size_t\" can't represent byte-size of data.\n", me);
    exit(1);
  }

  /* Allocate memory for the incoming data. */
  if (_nrrdCalloc(nrrd)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }

  /* Create the BZFILE* for reading in the gzipped data. */
  bzfin = BZ2_bzReadOpen(&bzerror, io->dataFile, 0, 0, NULL, 0);
  if (bzerror != BZ_OK) {
    /* there was a problem */
    sprintf(err, "%s: error opening BZFILE: %s", me, 
	    BZ2_bzerror(bzfin, &bzerror));
    biffAdd(NRRD, err);
    BZ2_bzReadClose(&bzerror, bzfin);
    return 1;
  }

  /* Here is where we do the byte skipping. */
  for(i = 0; i < io->byteSkip; i++) {
    unsigned char b;
    /* Check to see if a single byte was able to be read. */
    read = BZ2_bzRead(&bzerror, bzfin, &b, 1);
    if (read != 1 || bzerror != BZ_OK) {
      sprintf(err, "%s: hit an error skipping byte %d of %d: %s",
	      me, i, io->byteSkip, BZ2_bzerror(bzfin, &bzerror));
      biffAdd(NRRD, err);
      return 1;
    }
  }
  
  /* bzip2 can handle data sizes up to INT_MAX, so we can't just 
     pass in the size, because it might be too large for an int.
     Therefore it must be read in chunks if the size is larger 
     than INT_MAX. */
  if (size <= INT_MAX) {
    block_size = (int)size;
  } else {
    block_size = INT_MAX;
  }

  /* This counter will help us to make sure that we read as much data
     as we think we should. */
  total_read = 0;
  /* Pointer to the blocks as we read them. */
  data = nrrd->data;
  
  /* Ok, now we can begin reading. */
  bzerror = BZ_OK;
  while ((read = BZ2_bzRead(&bzerror, bzfin, data, block_size))
	  && (BZ_OK == bzerror || BZ_STREAM_END == bzerror) ) {
    /* Increment the data pointer to the next available spot. */
    data += read;
    total_read += read;
    /* We only want to read as much data as we need, so we need to check
       to make sure that we don't request data that might be there but that
       we don't want.  This will reduce block_size when we get to the last
       block (which may be smaller than block_size).
    */
    if (size - total_read < block_size)
      block_size = (int)(size - total_read);
  }
  
  if (!( BZ_OK == bzerror || BZ_STREAM_END == bzerror )) {
    sprintf(err, "%s: error reading from BZFILE: %s",
	    me, BZ2_bzerror(bzfin, &bzerror));
    biffAdd(NRRD, err);
    return 1;
  }

  /* Close the BZFILE. */
  BZ2_bzReadClose(&bzerror, bzfin);
  if (BZ_OK != bzerror) {
    sprintf(err, "%s: error closing BZFILE: %s", me,
	    BZ2_bzerror(bzfin, &bzerror));
    biffAdd(NRRD, err);
    return 1;
  }
  
  /* Check to see if we got out as much as we thought we should. */
  if (total_read != size) {
    sprintf(err, "%s: expected " _AIR_SIZE_T_FMT " bytes and received "
	    _AIR_SIZE_T_FMT " bytes",
	    me, size, total_read);
    biffAdd(NRRD, err);
    return 1;
  }
  
  return 0;
#else
  sprintf(err, "%s: sorry, this nrrd not compiled with bzip2 enabled", me);
  biffAdd(NRRD, err); return 1;
#endif
}

/*
** The data readers are responsible for memory allocation.
** This is necessitated by the memory restrictions of direct I/O
*/
int (*
nrrdReadData[NRRD_ENCODING_MAX+1])(Nrrd *, NrrdIO *) = {
  NULL,
  _nrrdReadDataRaw,
  _nrrdReadDataAscii,
  _nrrdReadDataHex,
  _nrrdReadDataGzip,
  _nrrdReadDataBzip2
};

int
nrrdLineSkip (NrrdIO *io) {
  int i, skipRet;
  char me[]="nrrdLineSkip", err[AIR_STRLEN_MED];

  /* For compressed data: If you don't actually have ascii headers on
     top of your gzipped data then you will potentially huge lines
     while _nrrdOneLine looks for line terminations.  Quoting Gordon:
     "Garbage in, Garbage out." */
  
  for (i=1; i<=io->lineSkip; i++) {
    if (_nrrdOneLine(&skipRet, io, io->dataFile)) {
      sprintf(err, "%s: error skipping line %d of %d", me, i, io->lineSkip);
      biffAdd(NRRD, err); return 1;
    }
    if (!skipRet) {
      sprintf(err, "%s: hit EOF skipping line %d of %d", me, i, io->lineSkip);
      biffAdd(NRRD, err); return 1;
    }
  }
  return 0;
}

int
nrrdByteSkip (Nrrd *nrrd, NrrdIO *io) {
  int i, skipRet;
  char me[]="nrrdByteSkip", err[AIR_STRLEN_MED];
  size_t numbytes;

  if (!( nrrd && io )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (-1 == io->byteSkip) {
    if (nrrdEncodingRaw != io->encoding) {
      sprintf(err, "%s: can't backwards byte skipping in %s encoding",
	      me, airEnumStr(nrrdEncoding, io->encoding));
      biffAdd(NRRD, err); return 1;
    }
    if (stdin == io->dataFile) {
      sprintf(err, "%s: can't fseek on stdin", me);
      biffAdd(NRRD, err); return 1;
    }
    numbytes = nrrdElementNumber(nrrd)*nrrdElementSize(nrrd);
    if (fseek(io->dataFile, -numbytes, SEEK_END)) {
      sprintf(err, "%s: failed to fseek(dataFile, " _AIR_SIZE_T_FMT
	      ", SEEK_END)", me, numbytes);
      biffAdd(NRRD, err); return 1;      
    }
  } else {
    for (i=1; i<=io->byteSkip; i++) {
      skipRet = fgetc(io->dataFile);
      if (EOF == skipRet) {
	sprintf(err, "%s: hit EOF skipping byte %d of %d",
		me, i, io->byteSkip);
	biffAdd(NRRD, err); return 1;
      }
    }
  }
  return 0;
}

int
_nrrdReadNrrd (FILE *file, Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdReadNrrd", *err=NULL;
  int ret, len;

  /* parse header lines */
  while (1) {
    io->pos = 0;
    if (_nrrdOneLine(&len, io, file)) {
      if ((err = (char*)malloc(AIR_STRLEN_MED))) {
	sprintf(err, "%s: trouble getting line of header", me);
	biffAdd(NRRD, err); free(err);
      }
      return 1;
    }
    if (len > 1) {
      ret = _nrrdReadNrrdParseField(nrrd, io, AIR_TRUE);
      if (!ret) {
	if ((err = (char*)malloc(AIR_STRLEN_MED + strlen(io->line)))) {
	  sprintf(err, "%s: trouble parsing field in \"%s\"", me, io->line);
	  biffAdd(NRRD, err); free(err);
	}
	return 1;
      }
      /* the comment is the one field allowed more than once */
      if (ret != nrrdField_comment && io->seen[ret]) {
	if ((err = (char*)malloc(AIR_STRLEN_MED))) {
	  sprintf(err, "%s: already set field %s", me, 
		  airEnumStr(nrrdField, ret));
	  biffAdd(NRRD, err); free(err);
	}
	return 1;
      }
      if (_nrrdReadNrrdParseInfo[ret](nrrd, io, AIR_TRUE)) {
	if ((err = (char*)malloc(AIR_STRLEN_MED))) {
	  sprintf(err, "%s: trouble parsing %s info \"%s\"", me,
		  airEnumStr(nrrdField, ret), io->line+io->pos);
	  biffAdd(NRRD, err); free(err);
	}
	return 1;
      }
      io->seen[ret] = AIR_TRUE;
    } else {
      /* len <= 1 */
      break;
    }
  }

  if (!len                        /* we're at EOF ... */
      && !io->seperateHeader) {   /* but there's supposed to be data here! */
    if ((err = (char*)malloc(AIR_STRLEN_MED))) {
      sprintf(err, "%s: hit end of header, but no \"%s\" given", me,
	      airEnumStr(nrrdField, nrrdField_data_file));
      biffAdd(NRRD, err); free(err);
    }
    return 1;
  }
  
  if (!_nrrdValidHeader(nrrd, io)) {
    if ((err = (char*)malloc(AIR_STRLEN_MED))) {
      sprintf(err, "%s: %s", me, 
	      (len ? "finished reading header, but there were problems"
	       : "hit EOF before seeing a complete valid header"));
      biffAdd(NRRD, err); free(err);
    }
    return 1;
  }
  
  /* we seemed to have read a valid header; now read the data */
  if (!io->seperateHeader) {
    io->dataFile = file;
  }
  if (2 <= nrrdStateVerboseIO) {
    fprintf(stderr, "(%s: reading %s data ", me, 
	    airEnumStr(nrrdEncoding, io->encoding));
    fflush(stderr);
  }

  if (nrrdLineSkip(io)) {
    if ((err = (char*)malloc(AIR_STRLEN_MED))) {
      sprintf(err, "%s: couldn't skip lines", me);
      biffAdd(NRRD, err); free(err);
    }
    return 1;
  }
  if (!nrrdEncodingIsCompression[io->encoding]) {
    if (nrrdByteSkip(nrrd, io)) {
      if ((err = (char*)malloc(AIR_STRLEN_MED))) {
	sprintf(err, "%s: couldn't skip bytes", me);
	biffAdd(NRRD, err); free(err);
      }
      return 1;
    }
  }

  if (!io->skipData) {
    if (nrrdReadData[io->encoding](nrrd, io)) {
      if (2 <= nrrdStateVerboseIO) {
	fprintf(stderr, "error!\n");
      }
      if ((err = (char*)malloc(AIR_STRLEN_MED))) {
	sprintf(err, "%s:", me);
	biffAdd(NRRD, err); free(err);
      }
      return 1;
    }
    if (airEndianUnknown != io->endian) {
      /* we positively know the endianness of data just read */
      if (1 < nrrdElementSize(nrrd)
	  && nrrdEncodingEndianMatters[io->encoding]
	  && io->endian != AIR_ENDIAN) {
	/* endianness exposed in encoding, and its wrong */
	if (2 <= nrrdStateVerboseIO) {
	  fprintf(stderr, "(%s: fixing endianness ... ", me);
	  fflush(stderr);
	}
	nrrdSwapEndian(nrrd);
	if (2 <= nrrdStateVerboseIO) {
	  fprintf(stderr, "done)");
	  fflush(stderr);
	}
      }
    }
  } else {
    nrrd->data = NULL;
  }
  if (2 <= nrrdStateVerboseIO) {
    fprintf(stderr, "done)\n");
  }
  if (io->seperateHeader && stdin != io->dataFile) {
    if (!io->keepSeperateDataFileOpen && !io->skipData) {
      io->dataFile = airFclose(io->dataFile);
    }
  } else {
    /* put things back the way we found them */
    io->dataFile = NULL;
  }
  
  return 0;
}

int
_nrrdReshapeUpGrayscale (Nrrd *nimg) {
  char me[]="_nrrdReshapeUpGrayscale", err[AIR_STRLEN_MED];
  int axmap[3] = {-1, 0, 1}, ret;
  Nrrd *ntmp;  /* just a holder for axis information */
  
  ntmp = nrrdNew();
  ntmp->dim = 2;
  if ( (ret = nrrdAxesCopy(ntmp, nimg, NULL, NRRD_AXESINFO_NONE)) ) {
    sprintf(err, "%s: nrrdAxesCopy() returned %d", me, ret);
    biffAdd(NRRD, err);
    nrrdNuke(ntmp); return 1;
  }
  nimg->dim = 3;
  if ( (ret = nrrdAxesCopy(nimg, ntmp, axmap, NRRD_AXESINFO_NONE)) ) {
    sprintf(err, "%s: nrrdAxesCopy() returned %d", me, ret);
    biffAdd(NRRD, err);
    nrrdNuke(ntmp); return 1;
  }
  nrrdNuke(ntmp);
  return 0;
}

int
_nrrdReadPNM (FILE *file, Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdReadPNM", err[AIR_STRLEN_MED];
  const char *fs;
  int i, color, got, want, len, ret, val[5], sx, sy, max;
  
  nrrd->type = nrrdTypeUChar;
  switch(io->magic) {
  case nrrdMagicP2:
    color = AIR_FALSE;
    io->encoding = nrrdEncodingAscii;
    nrrd->dim = 2;
    break;
  case nrrdMagicP3:
    color = AIR_TRUE;
    io->encoding = nrrdEncodingAscii;
    nrrd->dim = 3;
    break;
  case nrrdMagicP5:
    color = AIR_FALSE;
    io->encoding = nrrdEncodingRaw;
    nrrd->dim = 2;
    break;
  case nrrdMagicP6:
    color = AIR_TRUE;
    io->encoding = nrrdEncodingRaw;
    nrrd->dim = 3;
    break;
  default:
    sprintf(err, "%s: sorry, magic %d not implemented", me, io->magic);
    biffAdd(NRRD, err); return 1;
    break;
  }
  val[0] = val[1] = val[2] = 0;
  got = 0;
  want = 3;
  while (got < want) {
    io->pos = 0;
    if (_nrrdOneLine(&len, io, file)) {
      sprintf(err, "%s: failed to get line from PNM header", me);
      biffAdd(NRRD, err); return 1;
    }
    if (!(0 < len)) {
      sprintf(err, "%s: hit EOF in header with %d of %d ints parsed",
	      me, got, want);
      biffAdd(NRRD, err); return 1;
    }
    if ('#' == io->line[0]) {
      if (strncmp(io->line, NRRD_PNM_COMMENT, strlen(NRRD_PNM_COMMENT))) {
	/* this is a generic comment */
	ret = 0;
	goto plain;
      }
      /* else this PNM comment is trying to tell us something */
      io->pos = strlen(NRRD_PNM_COMMENT);
      io->pos += strspn(io->line + io->pos, _nrrdFieldSep);
      ret = _nrrdReadNrrdParseField(nrrd, io, AIR_FALSE);
      if (!ret) {
	if (nrrdStateVerboseIO) {
	  fprintf(stderr, "(%s: unparsable field \"%s\" --> plain comment)\n",
		  me, io->line);
	}
	goto plain;
      }
      if (nrrdField_comment == ret) {
	ret = 0;
	goto plain;
      }
      fs = airEnumStr(nrrdField, ret);
      if (!_nrrdFieldValidInImage[ret]) {
	if (nrrdStateVerboseIO) {
	  fprintf(stderr, "(%s: field \"%s\" (not allowed in PNM) "
		  "--> plain comment)\n", me, fs);
	}
	ret = 0;
	goto plain;
      }
      if (!io->seen[ret] 
	  && _nrrdReadNrrdParseInfo[ret](nrrd, io, AIR_FALSE)) {
	if (nrrdStateVerboseIO) {
	  fprintf(stderr,
		  "(%s: unparsable info for field \"%s\" --> plain comment)\n",
		  me, fs);
	}
	ret = 0;
	goto plain;
      }
      io->seen[ret] = AIR_TRUE;
    plain:
      if (!ret) {
	if (nrrdCommentAdd(nrrd, io->line+1)) {
	  sprintf(err, "%s: couldn't add comment", me);
	  biffAdd(NRRD, err); return 1;
	}
      }
      continue;
    }
    
    if (3 == sscanf(io->line, "%d%d%d", val+got+0, val+got+1, val+got+2)) {
      got += 3;
      continue;
    }
    if (2 == sscanf(io->line, "%d%d", val+got+0, val+got+1)) {
      got += 2;
      continue;
    }
    if (1 == sscanf(io->line, "%d", val+got+0)) {
      got += 1;
      continue;
    }

    /* else, we couldn't parse ANY numbers on this line, which is okay
       as long as the line contains nothing but white space */
    for (i=0; i<=strlen(io->line)-1 && isspace(io->line[i]); i++)
      ;
    if (i != strlen(io->line)) {
      sprintf(err, "%s: \"%s\" has no integers but isn't just whitespace", 
	      me, io->line);
      biffAdd(NRRD, err); return 1;
    }
  }
  /* this assumes 3 == want */
  sx = val[0];
  sy = val[1];
  max = val[2];
  if (!(sx > 0 && sy > 0 && max > 0)) {
    sprintf(err, "%s: sx,sy,max of %d,%d,%d has problem", me, sx, sy, max);
    biffAdd(NRRD, err); return 1;
  }
  if (255 != max) {
    sprintf(err, "%s: sorry, can only deal with max value 255 (not %d)", 
	    me, max);
    biffAdd(NRRD, err); return 1;
  }

  /* we know what we need in order to set nrrd fields and read data */
  if (color) {
    nrrdAxesSet(nrrd, nrrdAxesInfoSize, 3, sx, sy);
  } else {
    nrrdAxesSet(nrrd, nrrdAxesInfoSize, sx, sy);
  }
  io->dataFile = file;
  if (!io->skipData) {
    if (nrrdReadData[io->encoding](nrrd, io)) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
  } else {
    nrrd->data = NULL;
  }

  return 0;
}

#if TEEM_PNG
void
_nrrdReadErrorHandlerPNG (png_structp png, png_const_charp message)
{
  char me[]="_nrrdReadErrorHandlerPNG", err[AIR_STRLEN_MED];
  /* add PNG error message to biff */
  sprintf(err, "%s: PNG error: %s", me, message);
  biffAdd(NRRD, err);
  /* longjmp back to the setjmp, return 1 */
  longjmp(png->jmpbuf, 1);
}

void
_nrrdReadWarningHandlerPNG (png_structp png, png_const_charp message)
{
  char me[]="_nrrdReadWarningHandlerPNG", err[AIR_STRLEN_MED];
  /* add the png warning message to biff */
  sprintf(err, "%s: PNG warning: %s", me, message);
  biffAdd(NRRD, err);
  /* no longjump, execution continues */
}

/* we need to use the file I/O callbacks on windows
   to make sure we can mix VC6 libpng with VC7 teem */
#ifdef WIN32
static void
_nrrdReadDataPNG (png_structp png, png_bytep data, png_size_t len)
{
  png_size_t read;
  read = (png_size_t)fread(data, (png_size_t)1, len, (FILE*)png->io_ptr);
  if (read != len) png_error(png, "file read error");
}
#endif /* WIN32 */
#endif /* TEEM_PNG */

int
_nrrdReadPNG (FILE *file, Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdReadPNG", err[AIR_STRLEN_MED];
#if TEEM_PNG
  png_structp png;
  png_infop info;
  png_bytep *row;
  png_uint_32 width, height, rowsize;
  png_text* txt;
  int depth, type, i, channels, numtxt;
  int ntype, ndim, nsize[3];

  /* create png struct with the error handlers above */
  png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
			       _nrrdReadErrorHandlerPNG,
			       _nrrdReadWarningHandlerPNG);
  if (png == NULL) {
    sprintf(err, "%s: failed to create PNG read struct", me);
    biffAdd(NRRD, err); return 1;
  }
  /* create image info struct */
  info = png_create_info_struct(png);
  if (info == NULL) {
    png_destroy_read_struct(&png, NULL, NULL);
    sprintf(err, "%s: failed to create PNG image info struct", me);
    biffAdd(NRRD, err); return 1;
  }
  /* set up png style error handling */
  if (setjmp(png->jmpbuf)) {
    /* the error is reported inside the handler, 
       but we still need to clean up and return */
    png_destroy_read_struct(&png, &info, NULL);
    return 1;
  }
  /* initialize png I/O */
#ifdef WIN32
  png_set_read_fn(png, (png_voidp)file, _nrrdReadDataPNG);
#else
  png_init_io(png, file);
#endif
  /* if we are here, we have already read 6 bytes from the file */
  png_set_sig_bytes(png, 6);
  /* png_read_info() returns all information from the file 
     before the first data chunk */
  png_read_info(png, info);
  png_get_IHDR(png, info, &width, &height, &depth, &type, 
	       NULL, NULL, NULL);
  /* expand paletted colors into rgb triplets */
  if (type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);
  /* expand grayscale images to 8 bits from 1, 2, or 4 bits */
  if (type == PNG_COLOR_TYPE_GRAY && depth < 8)
    png_set_gray_1_2_4_to_8(png);
  /* expand paletted or rgb images with transparency to full alpha
     channels so the data will be available as rgba quartets */
  if (png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);
  /* fix endianness for 16 bit formats */
  if (depth > 8 && airMyEndian == airEndianLittle)
    png_set_swap(png);
#if 0
  /* set up gamma correction */
  /* NOTE: screen_gamma is platform dependent,
     it can hardwired or set from a parameter/environment variable */
  if (png_get_sRGB(png_ptr, info_ptr, &intent)) {
    /* if the image has sRGB info, 
       pass in standard nrrd file gamma 1.0 */
    png_set_gamma(png_ptr, screen_gamma, 1.0);
  } else {
    double gamma;
    /* set image gamma if present */
    if (png_get_gAMA(png, info, &gamma))
      png_set_gamma(png, screen_gamma, gamma);
    else
      png_set_gamma(png, screen_gamma, 1.0);
  }
#endif
  /* update reader */
  png_read_update_info(png, info);  
  /* allocate memory for the image data */
  ntype = depth > 8 ? nrrdTypeUShort : nrrdTypeUChar;
  switch (type) {
    case PNG_COLOR_TYPE_GRAY:
    ndim = 2; nsize[0] = width; nsize[1] = height;
    break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
    ndim = 3; nsize[0] = 2; nsize[1] = width; nsize[2] = height;
    break;
    case PNG_COLOR_TYPE_RGB:
    ndim = 3; nsize[0] = 3; nsize[1] = width; nsize[2] = height;
    break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
    ndim = 3; nsize[0] = 4; nsize[1] = width; nsize[2] = height;
    break;
    case PNG_COLOR_TYPE_PALETTE:
    /* TODO: merge this with the outer switch, needs to be tested */
    channels = png_get_channels(png, info);
    if (channels < 2) {
      ndim = 2; nsize[0] = width; nsize[1] = height;
    } else {
      ndim = 3; nsize[0] = channels; nsize[1] = width; nsize[2] = height;
    }
    break;
    default:
    png_destroy_read_struct(&png, &info, NULL);
    sprintf(err, "%s: unknown png type: %d", me, type);
    biffAdd(NRRD, err); return 1;
    break;
  }
  if (nrrdMaybeAlloc_nva(nrrd, ntype, ndim, nsize)) {
    png_destroy_read_struct(&png, &info, NULL);
    sprintf(err, "%s: failed to allocate nrrd", me);
    biffAdd(NRRD, err); return 1;
  }
  /* query row size */
  rowsize = png_get_rowbytes(png, info);
  /* check byte size */
  if (nrrdElementNumber(nrrd)*nrrdElementSize(nrrd) != height*rowsize) {
    png_destroy_read_struct(&png, &info, NULL);
    sprintf(err, "%s: size mismatch", me);
    biffAdd(NRRD, err); return 1;
  }
  /* set up row pointers */
  row = (png_bytep*)malloc(sizeof(png_bytep)*height);
  for (i=0; i<height; i++)
    row[i] = &((png_bytep)nrrd->data)[i*rowsize];
  /* read the entire image in one pass */
  png_read_image(png, row);
  /* read all text fields from the text chunk */
  numtxt = png_get_text(png, info, &txt, NULL);
  for (i=0; i<numtxt; i++) {
    if (!strcmp(txt[i].key, NRRD_PNG_KEY)) {
      int ret;
      io->pos = 0;
      /* Reading PNGs teaches Gordon that his scheme for parsing nrrd header
	 information is inappropriately specific to reading PNMs and NRRDs,
	 since in this case the text from which we parse a nrrd field
	 descriptor did NOT come from a line of text as read by
	 _nrrdOneLine */
      AIR_FREE(io->line);
      io->line = airStrdup(txt[i].text);
      ret = _nrrdReadNrrdParseField(nrrd, io, AIR_FALSE);
      if (ret) {
	const char* fs = airEnumStr(nrrdField, ret);
	if (nrrdField_comment == ret) {
	  ret = 0;
	  goto plain;
	}
	if (!_nrrdFieldValidInImage[ret]) {
	  if (nrrdStateVerboseIO) {
	    fprintf(stderr, "(%s: field \"%s\" (not allowed in PNG) "
		    "--> plain comment)\n", me, fs);
	  }
	  ret = 0;
	  goto plain;
	}
	if (!io->seen[ret] 
	    && _nrrdReadNrrdParseInfo[ret](nrrd, io, AIR_FALSE)) {
	  if (nrrdStateVerboseIO) {
	    fprintf(stderr, "(%s: unparsable info for field \"%s\" "
		    "--> plain comment)\n", me, fs);
	  }
	  ret = 0;
	  goto plain;
	}
	io->seen[ret] = AIR_TRUE;	
      plain:
	if (!ret) {
	  if (nrrdCommentAdd(nrrd, io->line)) {
	    sprintf(err, "%s: couldn't add comment", me);
	    biffAdd(NRRD, err); return 1;
	  }
	}
      }
    } else if (!strcmp(txt[i].key, NRRD_PNG_COMMENT_KEY)) {
      char *p, *c;
      c = airStrtok(txt[i].text, "\n", &p);
      while (c) {
	if (nrrdCommentAdd(nrrd, c)) {
	  sprintf(err, "%s: couldn't add comment", me);
	  biffAdd(NRRD, err); return 1;
	}
	c = airStrtok(NULL, "\n", &p);
      }
    }
  }
  /* finish reading */
  png_read_end(png, info);
  /* clean up */
  AIR_FREE(row);
  png_destroy_read_struct(&png, &info, NULL);

  return 0;
#else
  sprintf(err, "%s: sorry, this nrrd not compiled with PNG enabled", me);
  biffAdd(NRRD, err); return 1;
#endif
}

int
_nrrdReadVTK (FILE *file, Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdReadVTK", err[AIR_STRLEN_MED], *three[3];
  int len, sx, sy, sz, ret, N;
  double xm, ym, zm, xs, ys, zs;
  airArray *mop;

#define GETLINE(what) \
  do { \
    ret = _nrrdOneLine(&len, io, file); \
  } while (!ret && (1 == len)); \
  if (ret || !len) { \
    sprintf(err, "%s: couldn't get " #what " line", me); \
    biffAdd(NRRD, err); return 1; \
  }

  /* read in content */
  GETLINE(content);
  if (!(nrrd->content = airStrdup(io->line))) {
    sprintf(err, "%s: couldn't read or copy content string", me);
    biffAdd(NRRD, err); return 1;
  }
  GETLINE(encoding); airToUpper(io->line);
  if (!strcmp("ASCII", io->line)) {
    io->encoding = nrrdEncodingAscii;
  } else if (!strcmp("BINARY", io->line)) {
    io->encoding = nrrdEncodingRaw;
  } else {
    sprintf(err, "%s: encoding \"%s\" wasn't \"ASCII\" or \"BINARY\"",
	    me, io->line);
    biffAdd(NRRD, err); return 1;
  }
  GETLINE(DATASET); airToUpper(io->line);
  if (!strstr(io->line, "STRUCTURED_POINTS")) {
    sprintf(err, "%s: sorry, only STRUCTURED_POINTS data is nrrd-ready", me);
    biffAdd(NRRD, err); return 1;
  }
  GETLINE(DIMENSIONS); airToUpper(io->line);
  if (!strstr(io->line, "DIMENSIONS")
      || 3 != sscanf(io->line, "DIMENSIONS %d %d %d", &sx, &sy, &sz)) {
    sprintf(err, "%s: couldn't parse DIMENSIONS line (\"%s\")", me, io->line);
    biffAdd(NRRD, err); return 1;
  }
  GETLINE(ORIGIN); airToUpper(io->line);
  if (!strstr(io->line, "ORIGIN")
      || 3 != sscanf(io->line, "ORIGIN %lf %lf %lf", &xm, &ym, &zm)) {
    sprintf(err, "%s: couldn't parse  ORIGIN line (\"%s\")", me, io->line);
    biffAdd(NRRD, err); return 1;
  }
  GETLINE(SPACING/ASPECT); airToUpper(io->line);
  if (!( (strstr(io->line, "SPACING")
	  && 3 == sscanf(io->line, "SPACING %lf %lf %lf", &xs, &ys, &zs))
	 ||
	 (strstr(io->line, "ASPECT_RATIO")
	  && 3 == sscanf(io->line, "ASPECT_RATIO %lf %lf %lf", &xs, &ys, &zs))
	 )) {
    sprintf(err, "%s: couldn't parse SPACING/ASPECT_RATIO line (\"%s\")",
	    me, io->line);
    biffAdd(NRRD, err); return 1;
  }
  GETLINE(POINT_DATA); airToUpper(io->line);
  if (1 != sscanf(io->line, "POINT_DATA %d", &N)) {
    sprintf(err, "%s: couldn't parse POINT_DATA line (\"%s\")", me, io->line);
    biffAdd(NRRD, err); return 1;
  }
  if (N != sx*sy*sz) {
    sprintf(err, "%s: product of sizes (%d*%d*%d == %d) != # elements (%d)", 
	    me, sx, sy, sz, sx*sy*sz, N);
    biffAdd(NRRD, err); return 1;
  }
  GETLINE(attribute declaration);
  mop = airMopNew();
  if (3 != airParseStrS(three, io->line, AIR_WHITESPACE, 3, AIR_FALSE)) {
    sprintf(err, "%s: didn't see three words in attribute declaration \"%s\"",
	    me, io->line);
    biffAdd(NRRD, err); return 1;
  }
  airMopAdd(mop, three[0], airFree, airMopAlways);
  airMopAdd(mop, three[1], airFree, airMopAlways);
  airMopAdd(mop, three[2], airFree, airMopAlways);
  airToLower(three[2]);
  if (!strcmp(three[2], "bit")) {
    if (nrrdEncodingAscii == io->encoding) {
      fprintf(stderr, "%s: WARNING: \"bit\"-type data will be read in as "
	      "unsigned char\n", me);
      nrrd->type = nrrdTypeUChar;
    } else {
      sprintf(err, "%s: can't read in \"bit\"-type data as BINARY", me);
      biffAdd(NRRD, err); return 1;
    }
  } else if (!strcmp(three[2], "unsigned_char")) {
    nrrd->type = nrrdTypeUChar;
  } else if (!strcmp(three[2], "short")) {
    nrrd->type = nrrdTypeShort;
  } else if (!strcmp(three[2], "int")) {
    nrrd->type = nrrdTypeInt;
  } else if (!strcmp(three[2], "float")) {
    nrrd->type = nrrdTypeFloat;
  } else if (!strcmp(three[2], "double")) {
    nrrd->type = nrrdTypeDouble;
  } else {
    sprintf(err, "%s: type \"%s\" not recognized", me, three[2]);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  airToUpper(three[0]);
  if (!strncmp("SCALARS", three[0], strlen("SCALARS"))) {
    GETLINE(LOOKUP_TABLE); airToUpper(io->line);
    if (strcmp(io->line, "LOOKUP_TABLE DEFAULT")) {
      sprintf(err, "%s: sorry, can only deal with default LOOKUP_TABLE", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    nrrd->dim = 3;
    nrrdAxesSet(nrrd, nrrdAxesInfoSize, sx, sy, sz);
    nrrdAxesSet(nrrd, nrrdAxesInfoSpacing, xs, ys, zs);
    nrrdAxesSet(nrrd, nrrdAxesInfoMin, xm, ym, zm);
  } else if (!strncmp("VECTORS", three[0], strlen("VECTORS"))) {
    nrrd->dim = 4;
    nrrdAxesSet(nrrd, nrrdAxesInfoSize, 3, sx, sy, sz);
    nrrdAxesSet(nrrd, nrrdAxesInfoSpacing, AIR_NAN, xs, ys, zs);
    nrrdAxesSet(nrrd, nrrdAxesInfoMin, AIR_NAN, xm, ym, zm);
  } else if (!strncmp("TENSORS", three[0], strlen("TENSORS"))) {
    nrrd->dim = 4;
    nrrdAxesSet(nrrd, nrrdAxesInfoSize, 9, sx, sy, sz);
    nrrdAxesSet(nrrd, nrrdAxesInfoSpacing, AIR_NAN, xs, ys, zs);
    nrrdAxesSet(nrrd, nrrdAxesInfoMin, AIR_NAN, xm, ym, zm);
  } else {
    sprintf(err, "%s: sorry, can only deal with SCALARS, VECTORS, and TENSORS "
	    "currently, so couldn't parse attribute declaration \"%s\"",
	    me, io->line);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  io->dataFile = file;
  if (nrrdReadData[io->encoding](nrrd, io)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  if (1 < nrrdElementSize(nrrd)
      && nrrdEncodingEndianMatters[io->encoding]
      && airMyEndian != airEndianBig) {
    /* encoding exposes endianness, and its big, but we aren't */
    nrrdSwapEndian(nrrd);
  }
  airMopOkay(mop);
  return 0;
}


int
_nrrdReadTable (FILE *file, Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdReadTable", err[AIR_STRLEN_MED], *errS;
  const char *fs;
  int line, len, ret, sx, sy, settwo = 0, gotOnePerAxis = AIR_FALSE;
  /* fl: first line, al: all lines */
  airArray *flArr, *alArr;
  float *fl, *al, oneFloat;

  /* this goofiness is just to leave the nrrd as we found it
     (specifically, nrrd->dim) when we hit an error */
#define UNSETTWO if (settwo) nrrd->dim = settwo

  /* we only get here with the first line already in io->line */
  line = 1;
  len = strlen(io->line);
  
  if (0 == nrrd->dim) {
    settwo = nrrd->dim;
    nrrd->dim = 2;
  }
  /* first, we get through comments */
  while (NRRD_COMMENT_CHAR == io->line[0]) {
    io->pos = 1;
    io->pos += strspn(io->line + io->pos, _nrrdFieldSep);
    ret = _nrrdReadNrrdParseField(nrrd, io, AIR_FALSE);
    /* could we parse anything? */
    if (!ret) {
      /* being unable to parse a comment as a nrrd field is not 
	 any kind of error */
      goto plain;
    }
    if (nrrdField_comment == ret) {
      ret = 0;
      goto plain;
    }
    fs = airEnumStr(nrrdField, ret);
    if (!_nrrdFieldValidInTable[ret]) {
      if (nrrdStateVerboseIO) {
	fprintf(stderr, "(%s: field \"%s\" not allowed in table "
		"--> plain comment)\n", me, fs);
      }
      ret = 0;
      goto plain;
    }
    /* when reading a table, we simply ignore repetitions of a field */
    if (!io->seen[ret]
	&& _nrrdReadNrrdParseInfo[ret](nrrd, io, AIR_TRUE)) {
      errS = biffGetDone(NRRD);
      if (nrrdStateVerboseIO) {
	fprintf(stderr, "%s: %s", me, errS);
	fprintf(stderr, "(%s: malformed field \"%s\" --> plain comment)\n",
		me, fs);
      }
      if (nrrdField_dimension == ret) {
	/* "# dimension: 0" lead nrrd->dim being set to 0 */
	nrrd->dim = 2;
      }
      free(errS);
      ret = 0;
      goto plain;
    }
    if (nrrdField_dimension == ret) {
      if (!(1 == nrrd->dim || 2 == nrrd->dim)) {
	if (nrrdStateVerboseIO) {
	  fprintf(stderr, "(%s: table dimension can only be 1 or 2; "
		  "resetting to 2)\n", me);
	}
	nrrd->dim = 2;
      }
      if (1 == nrrd->dim && gotOnePerAxis) {
	fprintf(stderr, "(%s: already parsed per-axis field, can't reset "
		"dimension to 1; resetting to 2)\n", me);
	nrrd->dim = 2;
      }
    }
    if (_nrrdFieldOnePerAxis[ret]) 
      gotOnePerAxis = AIR_TRUE;
    io->seen[ret] = AIR_TRUE;
  plain:
    if (!ret) {
      if (nrrdCommentAdd(nrrd, io->line + 1)) {
	sprintf(err, "%s: couldn't add comment", me);
	biffAdd(NRRD, err); UNSETTWO; return 1;
      }
    }
    if (_nrrdOneLine(&len, io, file)) {
      sprintf(err, "%s: error getting a line", me);
      biffAdd(NRRD, err); UNSETTWO; return 1;
    }
    if (!len) {
      sprintf(err, "%s: hit EOF before any numbers parsed", me);
      biffAdd(NRRD, err); UNSETTWO; return 1;
    }
    line++;
  }

  /* we supposedly have a line of numbers, see how many there are */
  if (!airParseStrF(&oneFloat, io->line, _nrrdTableSep, 1)) {
    sprintf(err, "%s: couldn't parse a single number on line %d", me, line);
    biffAdd(NRRD, err); UNSETTWO; return 1;
  }
  flArr = airArrayNew((void**)&fl, NULL, sizeof(float), _NRRD_TABLE_INCR);
  if (!flArr) {
    sprintf(err, "%s: couldn't create array for first line values", me);
    biffAdd(NRRD, err); UNSETTWO; return 1;
  }
  for (sx=1; 1; sx++) {
    /* there is obviously a limit to the number of numbers that can 
       be parsed from a single finite line of input text */
    if (airArraySetLen(flArr, sx)) {
      sprintf(err, "%s: couldn't alloc space for %d values", me, sx);
      biffAdd(NRRD, err); UNSETTWO; return 1;
    }
    if (sx > airParseStrF(fl, io->line, _nrrdTableSep, sx)) {
      /* We asked for sx ints and got less.  We know that we successfully
	 got one value, so we did succeed in parsing sx-1 values */
      sx--;
      break;
    }
  }
  flArr = airArrayNuke(flArr);
  if (1 == nrrd->dim && 1 != sx) {
    sprintf(err, "%s: wanted 1-D nrrd, but got %d values on 1st line", me, sx);
    biffAdd(NRRD, err); UNSETTWO; return 1;
  }
  
  /* now see how many more lines there are */
  alArr = airArrayNew((void**)&al, NULL, sx*sizeof(float), _NRRD_TABLE_INCR);
  if (!alArr) {
    sprintf(err, "%s: couldn't create data buffer", me);
    biffAdd(NRRD, err); UNSETTWO; return 1;
  }
  sy = 0;
  while (len) {
    if (-1 == airArrayIncrLen(alArr, 1)) {
      sprintf(err, "%s: couldn't create scanline of %d values", me, sx);
      biffAdd(NRRD, err); UNSETTWO; return 1;
    }
    ret = airParseStrF(al + sy*sx, io->line, _nrrdTableSep, sx);
    if (sx > ret) {
      sprintf(err, "%s: could only parse %d values (not %d) on line %d",
	      me, ret, sx, line);
      biffAdd(NRRD, err); UNSETTWO; return 1;
    }
    sy++;
    line++;
    if (_nrrdOneLine(&len, io, file)) {
      sprintf(err, "%s: error getting a line", me);
      biffAdd(NRRD, err); UNSETTWO; return 1;
    }
  }
  /*
  fprintf(stderr, "%s: nrrd->dim = %d, sx = %d; sy = %d\n",
	  me, nrrd->dim, sx, sy);
  */
  
  switch (nrrd->dim) {
  case 2:
    if (nrrdMaybeAlloc(nrrd, nrrdTypeFloat, 2, sx, sy)) {
      sprintf(err, "%s: couldn't allocate table data", me);
      biffAdd(NRRD, err); UNSETTWO; return 1;
    }
    break;
  case 1:
    if (nrrdMaybeAlloc(nrrd, nrrdTypeFloat, 1, sy)) {
      sprintf(err, "%s: couldn't allocate table data", me);
      biffAdd(NRRD, err); UNSETTWO; return 1;
    }
    break;
  default:
    fprintf(stderr, "%s: PANIC about to save, but dim = %d\n", me, nrrd->dim);
    exit(1);
    break;
  }
  memcpy(nrrd->data, al, sx*sy*sizeof(float));
  
  alArr = airArrayNuke(alArr);
  
  return 0;
}

/*
******** nrrdRead()
**
** read in nrrd from a given file.  We don't yet know the format or
** anything else about how the data is written.  The only use for the
** NrrdIO struct "_io" here is to contain the base directory in which
** we'll look for a seperate data file, if that's desired.  Currently
** no other communication to us is done via the NrrdIO, so _io can be
** passed as NULL (if no header-relative datafiles are desired), and a
** NrrdIO will be created as needed.
*/
int
nrrdRead (Nrrd *nrrd, FILE *file, NrrdIO *_io) {
  char err[AIR_STRLEN_MED], me[] = "nrrdRead";
  int len;
  float oneFloat;
  NrrdIO *io;
  airArray *mop;

  if (!(file && nrrd)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  mop = airMopNew();
  if (_io) {
    io = _io;
  } else {
    io = nrrdIONew();
    if (!io) {
      sprintf(err, "%s: couldn't alloc I/O struct", me);
      biffAdd(NRRD, err); return 1;
    }
    airMopAdd(mop, io, (airMopper)nrrdIONix, airMopAlways);
  }

  /* clear out anything in the given nrrd */
  nrrdInit(nrrd);

  if (_nrrdOneLine(&len, io, file)) {
    sprintf(err, "%s: error getting magic line", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  if (!len) {
    sprintf(err, "%s: " _NRRD_IMM_EOF, me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  
  /* we have one line, see if there's magic, or comments, or numbers */
  io->magic = airEnumVal(nrrdMagic, io->line);
  switch (io->magic) {
  case nrrdMagicOldNRRD:
  case nrrdMagicNRRD0001:
    io->format = nrrdFormatNRRD;
    if (_nrrdReadNrrd(file, nrrd, io)) {
      sprintf(err, "%s: trouble reading NRRD file", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    break;
  case nrrdMagicP2:
  case nrrdMagicP3:
  case nrrdMagicP5:
  case nrrdMagicP6:
    io->format = nrrdFormatPNM;
    if (_nrrdReadPNM(file, nrrd, io)) {
      sprintf(err, "%s: trouble reading PNM image", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    break;
  case nrrdMagicPNG:
    io->format = nrrdFormatPNG;
    if (_nrrdReadPNG(file, nrrd, io)) {
      sprintf(err, "%s: trouble reading PNG image", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    break;
  case nrrdMagicVTK10:
  case nrrdMagicVTK20:
    io->format = nrrdFormatVTK;
    if (_nrrdReadVTK(file, nrrd, io)) {
      sprintf(err, "%s: trouble reading VTK file", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    break;
  default:
    /* see if line is a comment, or if we can parse one float from it,
       which implies its a table, */
    if (NRRD_COMMENT_CHAR == io->line[0] 
	|| airParseStrF(&oneFloat, io->line, _nrrdTableSep, 1)) {
      io->format = nrrdFormatTable;
      if (_nrrdReadTable(file, nrrd, io)) {
	sprintf(err, "%s: trouble reading table", me);
	biffAdd(NRRD, err); airMopError(mop); return 1;
      }
    } else {
      /* there's no hope */
      sprintf(err, "%s: couldn't parse \"%s\" as magic of any format",
	      me, io->line);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    break;
  }
  /* reshape up grayscale images, if desired */
  if (nrrdFormatIsImage[io->format] && 2 == nrrd->dim
      && nrrdStateGrayscaleImage3D) {
    if (_nrrdReshapeUpGrayscale(nrrd)) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  
  if (_io) {
    /* reset the given NrrdIO so that it can be used again */
    nrrdIOReset(_io);
  }

  airMopOkay(mop);
  return 0;
}

/*
** _nrrdSplitName()
**
** splits a file name into a path and a base filename.  The directory
** seperator is assumed to be '/'.  The division between the path
** and the base is the last '/' in the file name.  The path is
** everything prior to this, and base is everything after (so the
** base does NOT start with '/').  If there is not a '/' in the name,
** or if a '/' appears as the last character, then the path is set to
** ".", and the name is copied into base.
*/
int
_nrrdSplitName (char **dirP, char **baseP, const char *name) {
  int i, ret;
  
  i = strrchr(name, '/') - name;
  AIR_FREE(*dirP);
  AIR_FREE(*baseP);
  /* we found a valid break if the last directory character
     is somewhere in the string except the last character */
  if (i>=0 && i<strlen(name)-1) {
    *dirP = airStrdup(name);
    (*dirP)[i] = 0;
    *baseP = airStrdup(name + i + 1);
    ret = 1;
  } else {
    /* if the name had no slash, its in the current directory, which
       means that we need to explicitly store "." as the header
       directory in case we have header-relative data. */
    *dirP = airStrdup(".");
    *baseP = airStrdup(name);
    ret = 0;
  }
  return ret;
}

void
nrrdDirBaseSet (NrrdIO *io, const char *name) {
  
  if (io && name) {
    _nrrdSplitName(&(io->dir), &(io->base), name);
  }
}

/*
******** nrrdLoad()
**
** (documentation here)
**
** sneakiness: returns 2 if the reason for problem was a failed fopen().
** 
*/
int
nrrdLoad (Nrrd *nrrd, const char *filename) {
  char me[]="nrrdLoad", err[AIR_STRLEN_MED];
  NrrdIO *io;
  FILE *file;
  airArray *mop;


  if (!(nrrd && filename)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  io = nrrdIONew();
  if (!io) {
    sprintf(err, "%s: couldn't alloc I/O struct", me);
    biffAdd(NRRD, err); return 1;
  }
  mop = airMopNew();
  airMopAdd(mop, io, (airMopper)nrrdIONix, airMopAlways);

  /* we save the directory of the filename given to us so
     that if it turns out that its just a nhdr file, with
     a header-relative data file, then we will know how
     to find the data file */
  _nrrdSplitName(&(io->dir), &(io->base), filename);
  /* printf("!%s: |%s|%s|\n", me, io->dir, io->base); */

  if (!strcmp("-", filename)) {
    file = stdin;
#ifdef WIN32
    _setmode(_fileno(file), _O_BINARY);
#endif
  } else {
    file = fopen(filename, "rb");
    if (!file) {
      sprintf(err, "%s: fopen(\"%s\",\"rb\") failed: %s", 
	      me, filename, strerror(errno));
      biffAdd(NRRD, err); airMopError(mop); return 2;
    }
    airMopAdd(mop, file, (airMopper)airFclose, airMopAlways);
  }

  if (nrrdRead(nrrd, file, io)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}

