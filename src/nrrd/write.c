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
#include <teem32bit.h>

#if TEEM_BZIP2
#include <bzlib.h>
#endif
#if TEEM_PNG
#include <png.h>
#endif

/*
  #include <sys/types.h>
  #include <unistd.h>
*/

int
_nrrdFieldInteresting (Nrrd *nrrd, NrrdIO *io, int field) {
  int d, ret=0;
  
  if (!( nrrd
	 && AIR_INSIDE(1, nrrd->dim, NRRD_DIM_MAX)
	 && io
	 && AIR_BETWEEN(nrrdEncodingUnknown, io->encoding, nrrdEncodingLast)
	 && AIR_BETWEEN(nrrdField_unknown, field, nrrdField_last) )) {
    return 0;
  }
  
  switch (field) {
  case nrrdField_comment:
    /* comments are always handled differently (by being printed
       explicity), so they are never "interesting" */
    ret = 0;
    break;
  case nrrdField_number:
    /* "number" is entirely redundant with "sizes", which is a
       required field.  Absolutely nothing is lost in eliding "number"
       from the header, so "number" is NEVER interesting.  Should this
       judgement later be found in error, this is the one place where
       the policy change can be implemented */
    ret = 0;
    break;
  case nrrdField_type:
  case nrrdField_encoding:
  case nrrdField_dimension:
  case nrrdField_sizes:
    ret = 1;
    break;
  case nrrdField_block_size:
    ret = (nrrdTypeBlock == nrrd->type);
    break;
  case nrrdField_spacings:
    ret = 0;
    for (d=0; d<nrrd->dim; d++) {
      ret |= AIR_EXISTS(nrrd->axis[d].spacing);
    }
    break;
  case nrrdField_axis_mins:
    ret = 0;
    for (d=0; d<nrrd->dim; d++) {
      ret |= AIR_EXISTS(nrrd->axis[d].min);
    }
    break;
  case nrrdField_axis_maxs:
    ret = 0;
    for (d=0; d<nrrd->dim; d++) {
      ret |= AIR_EXISTS(nrrd->axis[d].max);
    }
    break;
  case nrrdField_centers:
    ret = 0;
    for (d=0; d<nrrd->dim; d++) {
      ret |= (nrrdCenterUnknown != nrrd->axis[d].center);
    }
    break;
  case nrrdField_labels:
    ret = 0;
    for (d=0; d<nrrd->dim; d++) {
      ret |= !!(airStrlen(nrrd->axis[d].label));
    }
    break;
  case nrrdField_units:
    ret = 0;
    for (d=0; d<nrrd->dim; d++) {
      ret |= !!(airStrlen(nrrd->axis[d].unit));
    }
    break;
  case nrrdField_endian:
    ret = (nrrdEncodingEndianMatters[io->encoding]
	   && 1 < nrrdElementSize(nrrd));
    break;
  case nrrdField_content:
    ret = !!(airStrlen(nrrd->content));
    break;
  case nrrdField_min:
    ret = AIR_EXISTS(nrrd->min);
    break;
  case nrrdField_max:
    ret = AIR_EXISTS(nrrd->max);
    break;
  case nrrdField_old_min:
    ret = AIR_EXISTS(nrrd->oldMin);
    break;
  case nrrdField_old_max:
    ret = AIR_EXISTS(nrrd->oldMax);
    break;
  case nrrdField_data_file:
    ret = io->seperateHeader;
    break;
  case nrrdField_line_skip:
    ret = io->lineSkip > 0;
    break;
  case nrrdField_byte_skip:
    ret = io->byteSkip > 0;
    break;
  }

  return ret;
}

int
_nrrdWriteDataRaw (Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdWriteDataRaw", err[AIR_STRLEN_MED];
  size_t size, ret, dio;
  
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nrrd)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }
  size = nrrdElementNumber(nrrd) * nrrdElementSize(nrrd);
  if (nrrdElementNumber(nrrd) != size/nrrdElementSize(nrrd)) {
    sprintf(err, "%s: \"size_t\" can't represent byte-size of data.", me);
    biffAdd(NRRD, err); return 1;
  }

  if (_nrrdFormatUsesDIO[io->format]) {
    dio = airDioTest(size, io->dataFile, nrrd->data);
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
    ret = airDioWrite(io->dataFile, nrrd->data, size);
    if (size != ret) {
      sprintf(err, "%s: airDioWrite failed", me);
      biffAdd(NRRD, err); return 1;
    }
  } else {
    if (AIR_DIO && _nrrdFormatUsesDIO[io->format]) {
      if (3 <= nrrdStateVerboseIO) {
	fprintf(stderr, "with fwrite()");
	if (4 <= nrrdStateVerboseIO) {
	  fprintf(stderr, " (why no DIO: %s)", airNoDioErr(dio));
	}
      }
      if (2 <= nrrdStateVerboseIO) {
	fprintf(stderr, " ... ");
	fflush(stderr);
      }
    }
    ret = fwrite(nrrd->data, nrrdElementSize(nrrd),
		 nrrdElementNumber(nrrd), io->dataFile);
    if (ret != nrrdElementNumber(nrrd)) {
      sprintf(err, "%s: fwrite() wrote only " _AIR_SIZE_T_FMT 
	      " %d-byte things, not " _AIR_SIZE_T_FMT ,
	      me, ret, nrrdElementSize(nrrd), nrrdElementNumber(nrrd));
      biffAdd(NRRD, err); return 1;
    }
    fflush(io->dataFile);
    /*
    if (ferror(io->dataFile)) {
      sprintf(err, "%s: ferror returned non-zero", me);
      biffAdd(NRRD, err); return 1;
    }
    */
  }
  return 0;
}

int
_nrrdWriteHexTable[16] = {
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

int
_nrrdWriteDataHex (Nrrd *nrrd, NrrdIO *io) {
  /* char me[]="_nrrdWriteDataAscii", err[AIR_STRLEN_MED]; */
  unsigned char *data;
  size_t byteIdx, byteNum;

  data = nrrd->data;
  byteNum = nrrdElementNumber(nrrd)*nrrdElementSize(nrrd);
  for (byteIdx=0; byteIdx<byteNum; byteIdx++) {
    fprintf(io->dataFile, "%c%c",
	    _nrrdWriteHexTable[(*data)>>4],
	    _nrrdWriteHexTable[(*data)&15]);
    if (34 == byteIdx%35)
      fprintf(io->dataFile, "\n");
    data++;
  }
  
  return 0;
}


int
_nrrdWriteDataAscii (Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdWriteDataAscii", err[AIR_STRLEN_MED], 
    buff[AIR_STRLEN_MED];
  int size, bufflen, linelen;
  char *data;
  size_t I, num;
  
  if (nrrdTypeBlock == nrrd->type) {
    sprintf(err, "%s: can't write nrrd type %s to ascii", me,
	    airEnumStr(nrrdType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nrrd)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }
  data = nrrd->data;
  size = nrrdElementSize(nrrd);
  num = nrrdElementNumber(nrrd);
  linelen = 0;
  for (I=0; I<num; I++) {
    nrrdSprint[nrrd->type](buff, data);
    if (1 == nrrd->dim) {
      fprintf(io->dataFile, "%s\n", buff);
    } else if (nrrd->dim == 2 
	       && nrrd->axis[0].size <= io->valsPerLine) {
      fprintf(io->dataFile, "%s%c", buff,
	      (I+1)%(nrrd->axis[0].size) ? ' ' : '\n');
    } else {
      bufflen = strlen(buff);
      if (linelen+bufflen+1 <= io->charsPerLine) {
	fprintf(io->dataFile, "%s%s", I ? " " : "", buff);
	linelen += (I ? 1 : 0) + bufflen;
      } else {
	fprintf(io->dataFile, "\n%s", buff);
	linelen = bufflen;
      }
    }
    data += size;
  }
  /* just to be sure, we always end with a carraige return */
  fprintf(io->dataFile, "\n");
  
  return 0;
}

int
_nrrdWriteDataGzip (Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdWriteDataGzip", err[AIR_STRLEN_MED];
#if TEEM_ZLIB
  size_t num, bsize, size, total_written;
  int block_size, fmt_i=0, error=0;
  char *data, fmt[4];
  gzFile gzfout;
  unsigned int wrote;
  
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nrrd)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }
  num = nrrdElementNumber(nrrd);
  if (!num) {
    sprintf(err, "%s: calculated number of elements to be zero!", me);
    biffAdd(NRRD, err); return 1;
  }
  bsize = num * nrrdElementSize(nrrd);
  size = bsize;
  if (num != bsize/nrrdElementSize(nrrd)) {
    fprintf(stderr,
	    "%s: PANIC: \"size_t\" can't represent byte-size of data.\n", me);
    exit(1);
  }

  /* Set format string based on the NrrdIO parameters. */
  fmt[fmt_i++] = 'w';
  if (0 <= io->zlibLevel && io->zlibLevel <= 9)
    fmt[fmt_i++] = '0' + io->zlibLevel;
  switch (io->zlibStrategy) {
  case nrrdZlibStrategyHuffman:
    fmt[fmt_i++] = 'h';
    break;
  case nrrdZlibStrategyFiltered:
    fmt[fmt_i++] = 'f';
    break;
  case nrrdZlibStrategyDefault:
  default:
    break;
  }
  fmt[fmt_i] = 0;

  /* Create the gzFile for writing in the gzipped data. */
  if ((gzfout = _nrrdGzOpen(io->dataFile, fmt)) == Z_NULL) {
    /* there was a problem */
    sprintf(err, "%s: error opening gzFile", me);
    biffAdd(NRRD, err);
    return 1;
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

  /* This counter will help us to make sure that we write as much data
     as we think we should. */
  total_written = 0;
  /* Pointer to the blocks as we write them. */
  data = nrrd->data;
  
  /* Ok, now we can begin writing. */
  while ((error = _nrrdGzWrite(gzfout, data, block_size, &wrote)) == 0 
	 && wrote > 0) {
    /* Increment the data pointer to the next available spot. */
    data += wrote;
    total_written += wrote;
    /* We only want to write as much data as we need, so we need to check
       to make sure that we don't write more data than is there.  This
       will reduce block_size when we get to the last block (which may
       be smaller than block_size).
    */
    if (size - total_written < block_size)
      block_size = (unsigned int)(size - total_written);
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
  if (_nrrdGzClose(gzfout) != 0) {
    sprintf(err, "%s: error closing gzFile", me);
    biffAdd(NRRD, err);
    return 1;
  }
  
  /* Check to see if we got out as much as we thought we should. */
  if (total_written != size) {
    sprintf(err, "%s: expected to write " _AIR_SIZE_T_FMT " bytes, but only "
	    "wrote " _AIR_SIZE_T_FMT,
	    me, size, total_written);
    biffAdd(NRRD, err);
    return 1;
  }
  
  return 0;
#else
  sprintf(err, "%s: sorry, this nrrd not compiled with zlib "
	  "(needed for gzip) enabled", me);
  biffAdd(NRRD, err); return 1;
#endif
}

int
_nrrdWriteDataBzip2 (Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdWriteDataBzip2", err[AIR_STRLEN_MED];
#if TEEM_BZIP2
  size_t num, bsize, size, total_written;
  int block_size, bs, bzerror=BZ_OK;
  char *data;
  BZFILE* bzfout;

  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nrrd)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }
  num = nrrdElementNumber(nrrd);
  if (!num) {
    sprintf(err, "%s: calculated number of elements to be zero!", me);
    biffAdd(NRRD, err); return 1;
  }
  bsize = num * nrrdElementSize(nrrd);
  size = bsize;
  if (num != bsize/nrrdElementSize(nrrd)) {
    fprintf(stderr,
	    "%s: PANIC: \"size_t\" can't represent byte-size of data.\n", me);
    exit(1);
  }

  /* Set compression block size. */
  if (1 <= io->bzip2BlockSize && io->bzip2BlockSize <= 9) {
    bs = io->bzip2BlockSize;
  } else {
    bs = 9;
  }
  /* Open bzfile for writing. Verbosity and work factor are set
     to default values. */
  bzfout = BZ2_bzWriteOpen(&bzerror, io->dataFile, bs, 0, 0);
  if (BZ_OK != bzerror) {
    sprintf(err, "%s: error opening BZFILE: %s", me, 
	    BZ2_bzerror(bzfout, &bzerror));
    biffAdd(NRRD, err);
    BZ2_bzWriteClose(&bzerror, bzfout, 0, NULL, NULL);
    return 1;
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

  /* This counter will help us to make sure that we write as much data
     as we think we should. */
  total_written = 0;
  /* Pointer to the blocks as we write them. */
  data = nrrd->data;
  
  /* Ok, now we can begin writing. */
  bzerror = BZ_OK;
  while (size - total_written > block_size) {
    BZ2_bzWrite(&bzerror, bzfout, data, block_size);
    if (BZ_OK != bzerror) break;
    /* Increment the data pointer to the next available spot. */
    data += block_size; 
    total_written += block_size;
  }
  /* write the last (possibly smaller) block when its humungous data;
     write the whole data when its small */
  if (BZ_OK == bzerror) {
    block_size = (int)(size - total_written);
    BZ2_bzWrite(&bzerror, bzfout, data, block_size);
    total_written += block_size;
  }

  if (BZ_OK != bzerror) {
    sprintf(err, "%s: error writing to BZFILE: %s",
	    me, BZ2_bzerror(bzfout, &bzerror));
    biffAdd(NRRD, err);
    return 1;
  }

  /* Close the BZFILE. */
  BZ2_bzWriteClose(&bzerror, bzfout, 0, NULL, NULL);
  if (BZ_OK != bzerror) {
    sprintf(err, "%s: error closing BZFILE: %s", me,
	    BZ2_bzerror(bzfout, &bzerror));
    biffAdd(NRRD, err);
    return 1;
  }
  
  /* Check to see if we got out as much as we thought we should. */
  if (total_written != size) {
    sprintf(err, "%s: expected to write " _AIR_SIZE_T_FMT " bytes, but only "
	    "wrote " _AIR_SIZE_T_FMT,
	    me, size, total_written);
    biffAdd(NRRD, err);
    return 1;
  }
  
  return 0;
#else
  sprintf(err, "%s: sorry, this nrrd not compiled with bzip2 enabled", me);
  biffAdd(NRRD, err); return 1;
#endif
}


int (*
nrrdWriteData[NRRD_ENCODING_MAX+1])(Nrrd *, NrrdIO *) = {
  NULL,
  _nrrdWriteDataRaw,
  _nrrdWriteDataAscii,
  _nrrdWriteDataHex,
  _nrrdWriteDataGzip,
  _nrrdWriteDataBzip2
};

/*
** _nrrdSprintFieldInfo
**
** this prints "<field>: <info>" into *strP (after allocating it for
** big enough, usually with a stupidly big margin of error), in a form
** suitable to be written to NRRD or PNM headers.  This will always
** print something (for valid inputs), even stupid <info>s like
** "(unknown endian)".  It is up to the caller to decide which fields
** are worth writing.
**
** HEY: this function is an utter mess.  re-write it pronto.
*/
void
_nrrdSprintFieldInfo (char **strP, Nrrd *nrrd, NrrdIO *io, int field) {
  char buff[AIR_STRLEN_MED];
  const char *fs;
  int i, D, fslen, fdlen, endi;

  if (!( strP
	 && nrrd 
	 && AIR_INSIDE(1, nrrd->dim, NRRD_DIM_MAX)
	 && AIR_BETWEEN(nrrdField_unknown, field, nrrdField_last) )) {
    return;
  }
  
  D = nrrd->dim;
  fs = airEnumStr(nrrdField, field);
  fslen = strlen(fs) + 50;  /* HEY: the real problem with this is that
			       we have to anticipate the "prefix" that 
			       will be used with the _PRINT_FIELD macro,
			       and at this point we have no access to
			       that information */
  switch (field) {
  case nrrdField_comment:
    /* this is handled differently */
    *strP = airStrdup("");
    break;
  case nrrdField_type:
    *strP = malloc(fslen + strlen(airEnumStr(nrrdType, nrrd->type)));
    sprintf(*strP, "%s: %s", fs, airEnumStr(nrrdType, nrrd->type));
    break;
  case nrrdField_encoding:
    *strP = malloc(fslen + strlen(airEnumStr(nrrdEncoding, io->encoding)));
    sprintf(*strP, "%s: %s", fs, airEnumStr(nrrdEncoding, io->encoding));
    break;
  case nrrdField_endian:
    if (airEndianUnknown != io->endian) {
      /* we know a specific endianness because either it was recorded as
	 part of "unu make -h", or it was set (and data was possibly
	 altered) as part of "unu save" */
      endi = io->endian;
    } else {
      /* we record our current architecture's endian because we're
	 going to writing out data */
      endi = AIR_ENDIAN;
    }
    *strP = malloc(fslen + strlen(airEnumStr(airEndian, endi)));
    sprintf(*strP, "%s: %s", fs, airEnumStr(airEndian, endi));
    break;
  case nrrdField_dimension:
    *strP = malloc(fslen + 10);
    sprintf(*strP, "%s: %d", fs, nrrd->dim);
    break;
    /* ---- begin per-axis fields ---- */
  case nrrdField_sizes:
    *strP = malloc(fslen + D*10);
    sprintf(*strP, "%s:", fs);
    for (i=0; i<D; i++) {
      sprintf(buff, " %d", nrrd->axis[i].size);
      strcat(*strP, buff);
    }
    break;
  case nrrdField_spacings:
    *strP = malloc(fslen + D*30);
    sprintf(*strP, "%s:", fs);
    for (i=0; i<D; i++) {
      airSinglePrintf(NULL, buff, " %lg", nrrd->axis[i].spacing);
      strcat(*strP, buff);
    }
    break;
  case nrrdField_axis_mins:
    *strP = malloc(fslen + D*30);
    sprintf(*strP, "%s:", fs);
    for (i=0; i<D; i++) {
      airSinglePrintf(NULL, buff, " %lg", nrrd->axis[i].min);
      strcat(*strP, buff);
    }
    break;
  case nrrdField_axis_maxs:
    *strP = malloc(fslen + D*30);
    sprintf(*strP, "%s:", fs);
    for (i=0; i<D; i++) {
      airSinglePrintf(NULL, buff, " %lg", nrrd->axis[i].max);
      strcat(*strP, buff);
    }
    break;
  case nrrdField_centers:
    *strP = malloc(fslen + D*10);
    sprintf(*strP, "%s:", fs);
    for (i=0; i<D; i++) {
      sprintf(buff, " %s",
	      (nrrd->axis[i].center 
	       ? airEnumStr(nrrdCenter, nrrd->axis[i].center)
	       : NRRD_UNKNOWN));
      strcat(*strP, buff);
    }
    break;
  case nrrdField_labels:
    fdlen = 0;
    for (i=0; i<D; i++) {
      fdlen += airStrlen(nrrd->axis[i].label) + 4;
    }
    *strP = malloc(fslen + fdlen);
    sprintf(*strP, "%s:", fs);
    for (i=0; i<D; i++) {
      strcat(*strP, " \"");
      if (airStrlen(nrrd->axis[i].label)) {
	strcat(*strP, nrrd->axis[i].label);
      }
      strcat(*strP, "\"");
    }
    break;
  case nrrdField_units:
    fdlen = 0;
    for (i=0; i<D; i++) {
      fdlen += airStrlen(nrrd->axis[i].unit) + 4;
    }
    *strP = malloc(fslen + fdlen);
    sprintf(*strP, "%s:", fs);
    for (i=0; i<D; i++) {
      strcat(*strP, " \"");
      if (airStrlen(nrrd->axis[i].unit)) {
	strcat(*strP, nrrd->axis[i].unit);
      }
      strcat(*strP, "\"");
    }
    break;
    /* ---- end per-axis fields ---- */
  case nrrdField_number:
    *strP = malloc(fslen + 30);
    sprintf(*strP, "%s: " _AIR_SIZE_T_FMT, fs, nrrdElementNumber(nrrd));
    break;
  case nrrdField_content:
    airOneLinify(nrrd->content);
    *strP = malloc(fslen + strlen(nrrd->content));
    sprintf(*strP, "%s: %s", fs, nrrd->content);
    break;
  case nrrdField_block_size:
    *strP = malloc(fslen + 20);
    sprintf(*strP, "%s: %d", fs, nrrd->blockSize);
    break;
  case nrrdField_min:
    *strP = malloc(fslen + 30);
    sprintf(*strP, "%s: ", fs);
    airSinglePrintf(NULL, buff, "%lg", nrrd->min);
    strcat(*strP, buff);
    break;
  case nrrdField_max:
    *strP = malloc(fslen + 30);
    sprintf(*strP, "%s: ", fs);
    airSinglePrintf(NULL, buff, "%lg", nrrd->max);
    strcat(*strP, buff);
    break;
  case nrrdField_old_min:
    *strP = malloc(fslen + 30);
    sprintf(*strP, "%s: ", fs);
    airSinglePrintf(NULL, buff, "%lg", nrrd->oldMin);
    strcat(*strP, buff);
    break;
  case nrrdField_old_max:
    *strP = malloc(fslen + 30);
    sprintf(*strP, "%s: ", fs);
    airSinglePrintf(NULL, buff, "%lg", nrrd->oldMax);
    strcat(*strP, buff);
    break;
  case nrrdField_data_file:
    if (airStrlen(io->dataFN)) {
      *strP = malloc(fslen + strlen(io->dataFN));
      /* for "unu make -h" */
      sprintf(*strP, "%s: %s", fs, io->dataFN);
    } else {
      *strP = malloc(fslen + strlen(io->base));
      sprintf(*strP, "%s: ./%s", fs, io->base);
    }
    break;
  case nrrdField_line_skip:
    *strP = malloc(fslen + 20);
    sprintf(*strP, "%s: %d", fs, io->lineSkip);
    break;
  case nrrdField_byte_skip:
    *strP = malloc(fslen + 20);
    sprintf(*strP, "%s: %d", fs, io->byteSkip);
    break;
  }

  return;
}

#define _PRINT_FIELD(prefix, field) \
if (_nrrdFieldInteresting(nrrd, io, field)) { \
  _nrrdSprintFieldInfo(&line, nrrd, io, field); \
  fprintf(file, "%s%s\n", prefix, line); \
  free(line); \
}

int
_nrrdWriteNrrd (FILE *file, Nrrd *nrrd, NrrdIO *io, int writeData) {
  char me[]="_nrrdWriteNrrd", err[AIR_STRLEN_MED], *tmpName, *line;
  int i;
  
  if (writeData) {
    if (io->seperateHeader) {
      tmpName = malloc(strlen(io->dir) + strlen(io->base) + 2);
      sprintf(tmpName, "%s/%s", io->dir, io->base);
      io->dataFile = fopen(tmpName, "wb");
      if (!io->dataFile) {
	sprintf(err, "%s: couldn't fopen(\"%s\",\"wb\"): %s",
		me, tmpName, strerror(errno));
	biffAdd(NRRD, err); return 1;
      }
      free(tmpName);
    } else {
      io->dataFile = file;
    }
  }

  fprintf(file, "%s\n", airEnumStr(nrrdMagic, nrrdMagicNRRD0001));
  /* fprintf(file, "%s\n", airEnumStr(nrrdMagic, nrrdMagicOldNRRD)); */

  /* this is where the majority of the header printing happens */
  for (i=1; i<=NRRD_FIELD_MAX; i++) {
    _PRINT_FIELD("", i);
  }

  for (i=0; i<nrrd->cmtArr->len; i++) {
    fprintf(file, "%c %s\n", NRRD_COMMENT_CHAR, nrrd->cmt[i]);
  }

  if (!io->seperateHeader) {
    fprintf(file, "\n");
  }

  if (writeData) {
    if (2 <= nrrdStateVerboseIO) {
      fprintf(stderr, "(%s: writing %s data ", me, 
	      airEnumStr(nrrdEncoding, io->encoding));
      fflush(stderr);
    }
    if (nrrdWriteData[io->encoding](nrrd, io)) {
      if (2 <= nrrdStateVerboseIO) {
	fprintf(stderr, "error!\n");
      }
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
    if (2 <= nrrdStateVerboseIO) {
      fprintf(stderr, "done)\n");
    }

    if (io->seperateHeader) {
      io->dataFile = airFclose(io->dataFile);
    } else {
      io->dataFile = NULL;
    }
  }

  return 0;
}

int
_nrrdReshapeDownGrayscale (Nrrd *nimg) {
  char me[]="_nrrdReshapeDownGrayscale", err[AIR_STRLEN_MED];
  int axmap[2] = {1, 2};
  Nrrd *ntmp;  /* just a holder for axis information */
  
  ntmp = nrrdNew();
  if (nrrdAxesCopy(ntmp, nimg, NULL, NRRD_AXESINFO_NONE)
      || nrrdAxesCopy(nimg, ntmp, axmap, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s: ", me); biffAdd(NRRD, err); return 1;
  }
  return 0;
}

int
_nrrdWritePNM (FILE *file, Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdWritePNM", err[AIR_STRLEN_MED], *line;
  int i, color, sx, sy, magic;
  
  if (3 == nrrd->dim && 1 == nrrd->axis[0].size) {
    if (_nrrdReshapeDownGrayscale(nrrd)) {
      sprintf(err, "%s: trouble reshaping grayscale image", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  color = (3 == nrrd->dim);
  if (!color) {
    magic = (nrrdEncodingAscii == io->encoding
	     ? nrrdMagicP2
	     : nrrdMagicP5);
    sx = nrrd->axis[0].size;
    sy = nrrd->axis[1].size;
  } else {
    magic = (nrrdEncodingAscii == io->encoding
	     ? nrrdMagicP3
	     : nrrdMagicP6);
    sx = nrrd->axis[1].size;
    sy = nrrd->axis[2].size;
  }
  
  fprintf(file, "%s\n", airEnumStr(nrrdMagic, magic));
  fprintf(file, "%d %d\n", sx, sy);
  for (i=1; i<=NRRD_FIELD_MAX; i++) {
    if (_nrrdFieldValidInImage[i]) { 
      _PRINT_FIELD(NRRD_PNM_COMMENT, i); 
    }
  }
  for (i=0; i<nrrd->cmtArr->len; i++) {
    fprintf(file, "# %s\n", nrrd->cmt[i]);
  }
  fprintf(file, "255\n");

  io->dataFile = file;
  if (nrrdWriteData[io->encoding](nrrd, io)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  
  return 0;
}

#if TEEM_PNG
void
_nrrdWriteErrorHandlerPNG (png_structp png, png_const_charp message)
{
  char me[]="_nrrdWriteErrorHandlerPNG", err[AIR_STRLEN_MED];
  /* add PNG error message to biff */
  sprintf(err, "%s: PNG error: %s", me, message);
  biffAdd(NRRD, err);
  /* longjmp back to the setjmp, return 1 */
  longjmp(png->jmpbuf, 1);
}

void
_nrrdWriteWarningHandlerPNG (png_structp png, png_const_charp message)
{
  char me[]="_nrrdWriteWarningHandlerPNG", err[AIR_STRLEN_MED];
  /* add the png warning message to biff */
  sprintf(err, "%s: PNG warning: %s", me, message);
  biffAdd(NRRD, err);
  /* no longjump, execution continues */
}

/* we need to use the file I/O callbacks on windows
   to make sure we can mix VC6 libpng with VC7 teem */
#ifdef WIN32
static void
_nrrdWriteDataPNG (png_structp png, png_bytep data, png_size_t len)
{
  png_size_t written;
  written = fwrite(data, 1, len, (FILE*)(png->io_ptr));
  if (written != len) png_error(png, "file write error");
}

static void
_nrrdFlushDataPNG (png_structp png)
{
   FILE *io_ptr = (FILE*)(png->io_ptr);
   if (io_ptr != NULL) fflush(io_ptr);
}
#endif /* WIN32 */
#endif /* TEEM_PNG */

int
_nrrdWritePNG (FILE *file, Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdWritePNG", err[AIR_STRLEN_MED];
#if TEEM_PNG
  int depth, type, i, numtxt, csize;
  png_structp png;
  png_infop info;
  png_bytep *row;
  png_uint_32 width, height, rowsize;
  png_text txt[NRRD_FIELD_MAX+1];

  /* no need to check type and format, done in FitsInFormat */
  /* create png struct with the error handlers above */
  png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
				_nrrdWriteErrorHandlerPNG,
				_nrrdWriteWarningHandlerPNG);
  if (png == NULL) {
    sprintf(err, "%s: failed to create PNG write struct", me);
    biffAdd(NRRD, err); return 1;
  }
  /* create image info struct */
  info = png_create_info_struct(png);
  if (info == NULL) {
    png_destroy_write_struct(&png, NULL);
    sprintf(err, "%s: failed to create PNG image info struct", me);
    biffAdd(NRRD, err); return 1;
  }
  /* set up error png style error handling */
  if (setjmp(png->jmpbuf))
  {
    /* the error is reported inside the error handler, 
       but we still need to clean up an return with an error */
    png_destroy_read_struct(&png, &info, NULL);
    return 1;
  }
  /* initialize png I/O */
#ifdef WIN32
  png_set_write_fn(png, file, _nrrdWriteDataPNG, _nrrdFlushDataPNG);
#else
  png_init_io(png, file);        
#endif
  /* calculate depth, width, height, and row size */
  depth = nrrd->type == nrrdTypeUChar ? 8 : 16;
  switch (nrrd->dim) {
    case 2: /* g only */
    width = nrrd->axis[0].size;
    height = nrrd->axis[1].size;
    type = PNG_COLOR_TYPE_GRAY;
    rowsize = width*nrrdElementSize(nrrd);
    break;
    case 3: /* g, ga, rgb, rgba */
    width = nrrd->axis[1].size;
    height = nrrd->axis[2].size;
    rowsize = width*nrrd->axis[0].size*nrrdElementSize(nrrd);
    switch (nrrd->axis[0].size) {
      case 1:
      type = PNG_COLOR_TYPE_GRAY;
      break;
      case 2:
      type = PNG_COLOR_TYPE_GRAY_ALPHA;
      break;
      case 3:
      type = PNG_COLOR_TYPE_RGB;
      break;
      case 4:
      type = PNG_COLOR_TYPE_RGB_ALPHA;
      break;
      default:
      sprintf(err, "%s: nrrd->axis[0].size (%d) not compatible with PNG",
	      me, nrrd->axis[0].size);
      biffAdd(NRRD, err); return 1;
      break;
    }
    break;
    default:
    sprintf(err, "%s: dimension (%d) not compatible with PNG", me, nrrd->dim);
    biffAdd(NRRD, err); return 1;
    break;
  }
  /* set image header info */
  png_set_IHDR(png, info, width, height, depth, type,
	       PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
	       PNG_FILTER_TYPE_BASE);
  /* add nrrd fields to the text chunk */
  numtxt = 0;
  csize = 0;
  for (i=1; i<=NRRD_FIELD_MAX; i++) {
    if (_nrrdFieldValidInImage[i] && _nrrdFieldInteresting(nrrd, io, i)) { 
      txt[numtxt].key = NRRD_PNG_KEY;
      txt[numtxt].compression = PNG_TEXT_COMPRESSION_NONE;
      _nrrdSprintFieldInfo(&(txt[numtxt].text), nrrd, io, i);      
      numtxt++;
    }
  }
  /* add nrrd comments as a single text field */
  if (nrrd->cmtArr->len > 0)
  {
    txt[numtxt].key = NRRD_PNG_COMMENT_KEY;
    txt[numtxt].compression = PNG_TEXT_COMPRESSION_NONE;
    for (i=0; i<nrrd->cmtArr->len; i++)
      csize += airStrlen(nrrd->cmt[i]) + 1;
    txt[numtxt].text = (png_charp)malloc(csize + 1);
    txt[numtxt].text[0] = 0;
    for (i=0; i<nrrd->cmtArr->len; i++) {
      strcat(txt[numtxt].text, nrrd->cmt[i]);
      strcat(txt[numtxt].text, "\n");
    }
    numtxt++;
  }
  if (numtxt > 0)
    png_set_text(png, info, txt, numtxt);
  /* write header */
  png_write_info(png, info);
  /* fix endianness for 16 bit formats */
  if (depth > 8 && airMyEndian == airEndianLittle)
    png_set_swap(png);
  /* set up row pointers */
  row = (png_bytep*)malloc(sizeof(png_bytep)*height);
  for (i=0; i<height; i++)
    row[i] = &((png_bytep)nrrd->data)[i*rowsize];
  png_set_rows(png, info, row);
  /* write the entire image in one pass */
  png_write_image(png, row);
  /* finish writing */
  png_write_end(png, info);
  /* clean up */
  for (i=0; i<numtxt; i++) airFree(txt[i].text);    
  airFree(row);
  png_destroy_write_struct(&png, &info);

  return 0;
#else
  sprintf(err, "%s: sorry, this nrrd not compiled with PNG enabled", me);
  biffAdd(NRRD, err); return 1;
#endif
}

int
_nrrdWriteTable (FILE *file, Nrrd *nrrd, NrrdIO *io) {
  char cmt[AIR_STRLEN_SMALL], *line, buff[AIR_STRLEN_SMALL];
  size_t I;
  int i, x, y, sx, sy;
  void *data;
  float val;

  sprintf(cmt, "%c ", NRRD_COMMENT_CHAR);
  if (!io->bareTable) {
    if (1 == nrrd->dim) {
      _PRINT_FIELD(cmt, nrrdField_dimension);
    }
    for (i=1; i<=NRRD_FIELD_MAX; i++) {
      /* dimension is handled above */
      if (_nrrdFieldValidInTable[i] && nrrdField_dimension != i) {
	_PRINT_FIELD(cmt, i);
      }
    }
  }

  if (1 == nrrd->dim) {
    sx = 1;
    sy = nrrd->axis[0].size;
  }
  else {
    sx = nrrd->axis[0].size;
    sy = nrrd->axis[1].size;
  }
  data = nrrd->data;
  I = 0;
  for (y=0; y<sy; y++) {
    for (x=0; x<sx; x++) {
      val = nrrdFLookup[nrrd->type](data, I);
      nrrdSprint[nrrdTypeFloat](buff, &val);
      if (x) fprintf(file, " ");
      fprintf(file, "%s", buff);
      I++;
    }
    fprintf(file, "\n");
  }
  
  return 0;
}

/*
** HEY: this (of all places) is where the filename of a seperate
** datafile is determined
*/
void
_nrrdGuessFormat (NrrdIO *io, const char *filename) {
  int strpos;
  char suffix[AIR_STRLEN_SMALL];

  /* currently, we play the detached header game whenever the filename
     ends in NRRD_EXT_HEADER, and when we play this game, the data file
     is ALWAYS header relative. */
  /* we assume that that io->encoding is valid at this point: the
     seperate datafile will have an extension determined by 
     the encoding (and having extension "(unknown encoding)" would
     be a real drag). */
  strpos = strlen(io->base) - strlen(NRRD_EXT_HEADER);
  if (airEndsWith(filename, NRRD_EXT_HEADER)) {
    /* HEY: isn't this an out-of-allocated-bounds error? */
    io->base[strpos++] = '.';
    if (nrrdEncodingIsCompression[io->encoding]) {
      sprintf(suffix, "raw.%s", airEnumStr(nrrdEncoding, io->encoding));
    } else {
      strcpy(suffix, airEnumStr(nrrdEncoding, io->encoding));
    }
    strcpy(io->base + strpos, suffix);
    io->seperateHeader = AIR_TRUE;
    io->format = nrrdFormatNRRD;
  } else if (airEndsWith(filename, NRRD_EXT_PGM) 
	     || airEndsWith(filename, NRRD_EXT_PPM)) {
    io->format = nrrdFormatPNM;
  } else if (airEndsWith(filename, NRRD_EXT_PNG)) {
    io->format = nrrdFormatPNG;
  } else if (airEndsWith(filename, NRRD_EXT_TABLE)) {
    io->format = nrrdFormatTable;
  } else {
    /* nothing obvious */
    io->format = nrrdFormatUnknown;
  }
}

void
_nrrdFixFormat (NrrdIO *io, Nrrd *nrrd) {
  char me[]="_nrrdFixFormat";
  int fits;

  switch(io->format) {
  case nrrdFormatUnknown:
    /* if they still don't know what format to use, then enforce NRRD.
       There is no point in having a "default" format (which would be
       called nrrdDefWrtFormat) because the point of nrrd is that it is
       the mother of all nearly raw raster data file formats */
    io->format = nrrdFormatNRRD;
    break;
  case nrrdFormatNRRD:
    /* everything fits in a nrrd */
    /* Actually, invalid nrrds can't fit in a nrrd file format, but we can't
       do error reporting here because we have a void return; besides, 
       invalid nrrds will get caught sooner or later ... */
    break;
  case nrrdFormatPNM:
    fits = nrrdFitsInFormat(nrrd, io->encoding, nrrdFormatPNM, AIR_FALSE);
    if (!fits) {
      if (nrrdStateVerboseIO) {
	fprintf(stderr, "(%s: Can't be a PNM image -> saving as NRRD)\n", me); 
      }
      io->format = nrrdFormatNRRD;
    } else {
      if (2 == fits && airEndsWith(io->base, NRRD_EXT_PPM)) {
	if (nrrdStateVerboseIO) {
	  fprintf(stderr, "(%s: Image is grayscale; saving as PGM)\n", me); 
	}
      }
      if (3 == fits && airEndsWith(io->base, NRRD_EXT_PGM)) {
	if (nrrdStateVerboseIO) {
	  fprintf(stderr, "(%s: Image is color -> saving as PPM)\n", me); 
	}
      }
      /* nrrdFormatPNM == io->format is okay, we did only a warning */
    }
    break;
  case nrrdFormatPNG:
    fits = nrrdFitsInFormat(nrrd, io->encoding, nrrdFormatPNG, AIR_FALSE);
    if (!fits) {
      if (nrrdStateVerboseIO) {
	fprintf(stderr, "(%s: Can't be a PNG -> saving as NRRD)\n", me);
      }
      io->format = nrrdFormatNRRD;
    }
    break;
  case nrrdFormatTable:
    if (!nrrdFitsInFormat(nrrd, io->encoding, nrrdFormatTable, AIR_FALSE)) {
      if (nrrdStateVerboseIO) {
	fprintf(stderr, "(%s: Can't be a table -> saving as NRRD)\n", me);
      }
      io->format = nrrdFormatNRRD;
    }
    break;
  default:
    fprintf(stderr, "%s: PANIC: don't know about format %d\n", me, io->format);
    exit(1);
    break;
  }
  
  return;
}

int
nrrdWrite (FILE *file, Nrrd *nrrd, NrrdIO *io) {
  char me[]="nrrdWrite", err[AIR_STRLEN_MED];
  int ret=0;

  if (!(file && nrrd && io)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!nrrdValid(nrrd)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_BETWEEN(nrrdEncodingUnknown, io->encoding, nrrdEncodingLast)) {
    sprintf(err, "%s: invalid encoding %d", me, io->encoding);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdEncodingUnknown == io->encoding) {
    io->encoding = nrrdDefWrtEncoding;
  } else if (!AIR_BETWEEN(nrrdEncodingUnknown, io->encoding, 
			  nrrdEncodingLast)) {
    sprintf(err, "%s: invalid encoding %d\n", me, io->encoding);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_BETWEEN(nrrdFormatUnknown, io->format, nrrdFormatLast)) {
    sprintf(err, "%s: invalid format %d", me, io->format);
    biffAdd(NRRD, err); return 1;
  }
  if (!nrrdFitsInFormat(nrrd, io->encoding, io->format, AIR_TRUE)) {
    sprintf(err, "%s: doesn't fit in %s format", me,
	    airEnumStr(nrrdFormat, io->format));
    biffAdd(NRRD, err); return 1;
  }
  if (io->byteSkip || io->lineSkip) {
    sprintf(err, "%s: can't generate line or byte skips on data write", me);
    biffAdd(NRRD, err); return 1;
  }

  switch (io->format) {
  case nrrdFormatNRRD:
    ret = _nrrdWriteNrrd(file, nrrd, io, AIR_TRUE);
    break;
  case nrrdFormatPNM:
    ret = _nrrdWritePNM(file, nrrd, io);
    break;
  case nrrdFormatPNG:
    ret = _nrrdWritePNG(file, nrrd, io);
    break;
  case nrrdFormatTable:
    ret = _nrrdWriteTable(file, nrrd, io);
    break;
  }
  if (ret) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  
  /* reset the NrrdIO so that it can be used again */
  nrrdIOReset(io);

  return 0;
}

int
nrrdSave (const char *filename, Nrrd *nrrd, NrrdIO *io) {
  char me[]="nrrdSave", err[AIR_STRLEN_MED];
  FILE *file;
  airArray *mop;

  if (!(nrrd && filename)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  mop = airMopInit();
  if (!io) {
    io = nrrdIONew();
    if (!io) {
      sprintf(err, "%s: couldn't alloc something", me);
      biffAdd(NRRD, err); return 1;
    }
    airMopAdd(mop, io, (airMopper)nrrdIONix, airMopAlways);
  }
  if (nrrdEncodingUnknown == io->encoding) {
    io->encoding = nrrdDefWrtEncoding;
  } else if (!AIR_BETWEEN(nrrdEncodingUnknown, io->encoding, 
			  nrrdEncodingLast)) {
    sprintf(err, "%s: invalid encoding %d\n", me, io->encoding);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }

  if (nrrdFormatUnknown == io->format) {
    _nrrdSplitName(&(io->dir), &(io->base), filename);
    _nrrdGuessFormat(io, filename);
    _nrrdFixFormat(io, nrrd);
  }
  if (!( AIR_INSIDE(nrrdFormatUnknown, io->format, nrrdFormatLast) )) {
    sprintf(err, "%s: invalid format %d\n", me, io->format);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }

  if (!strcmp("-", filename)) {
    file = stdout;
#ifdef WIN32
    _setmode(_fileno(file), _O_BINARY);
#endif
  } else {
    if (!(file = fopen(filename, "wb"))) {
      sprintf(err, "%s: couldn't fopen(\"%s\",\"wb\"): %s", 
	      me, filename, strerror(errno));
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    airMopAdd(mop, file, (airMopper)airFclose, airMopAlways);
  }

  if (nrrdWrite(file, nrrd, io)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}

/* Ernesto "Che" Guevara  */
