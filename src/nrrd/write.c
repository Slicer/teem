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
  #include <sys/types.h>
  #include <unistd.h>
*/

int
_nrrdFieldInteresting(Nrrd *nrrd, NrrdIO *io, int field) {
  int d, ret;
  
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
       from the header.  Should this judgement later be found in
       error, this is the one place where the policy change can be
       implemented */
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
    ret = !!(io->lineSkip);
    break;
  case nrrdField_byte_skip:
    ret = !!(io->byteSkip);
    break;
  }

  return ret;
}

int
_nrrdWriteDataRaw(Nrrd *nrrd, NrrdIO *io) {
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
      sprintf(err, "%s: fwrite() returned " AIR_SIZE_T_FMT
	      " (not " AIR_SIZE_T_FMT ")", me,
	      ret, nrrdElementNumber(nrrd));
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
_nrrdWriteDataAscii(Nrrd *nrrd, NrrdIO *io) {
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

int (*
nrrdWriteData[NRRD_ENCODING_MAX+1])(Nrrd *, NrrdIO *) = {
  NULL,
  _nrrdWriteDataRaw,
  _nrrdWriteDataAscii
};

/*
** _nrrdSprintFieldInfo
**
** this prints "<field>: <info>" into given string "str", in a form
** suitable to be written to NRRD or PNM headers.  This will always
** print something (for valid inputs), even stupid <info>s like
** "(unknown endian)".  It is up to the caller to decide which fields
** are worth writing.
*/
void
_nrrdSprintFieldInfo(char *str, Nrrd *nrrd, NrrdIO *io, int field) {
  char buff[AIR_STRLEN_MED];
  const char *fs;
  int i, D;

  if (!( str 
	 && nrrd 
	 && AIR_INSIDE(1, nrrd->dim, NRRD_DIM_MAX)
	 && AIR_BETWEEN(nrrdField_unknown, field, nrrdField_last) )) {
    return;
  }
  
  D = nrrd->dim;
  fs = airEnumStr(nrrdField, field);
  switch (field) {
  case nrrdField_comment:
    /* this is handled differently */
    strcpy(str, "");
    break;
  case nrrdField_type:
    sprintf(str, "%s: %s", fs,
	    airEnumStr(nrrdType, nrrd->type));
    break;
  case nrrdField_encoding:
    sprintf(str, "%s: %s", fs,
	    airEnumStr(nrrdEncoding, io->encoding));
    break;
  case nrrdField_endian:
    /* note we record our current architecture's endian */
    sprintf(str, "%s: %s", fs,
	    airEnumStr(airEndian, AIR_ENDIAN));
    break;
  case nrrdField_dimension:
    sprintf(str, "%s: %d", fs, nrrd->dim);
    break;
    /* ---- begin per-axis fields ---- */
  case nrrdField_sizes:
    sprintf(str, "%s:", fs);
    for (i=0; i<D; i++) {
      sprintf(buff, " %d", nrrd->axis[i].size);
      strcat(str, buff);
    }
    break;
  case nrrdField_spacings:
    sprintf(str, "%s:", fs);
    for (i=0; i<D; i++) {
      airSinglePrintf(NULL, buff, " %lg", nrrd->axis[i].spacing);
      strcat(str, buff);
    }
    break;
  case nrrdField_axis_mins:
    sprintf(str, "%s:", fs);
    for (i=0; i<D; i++) {
      airSinglePrintf(NULL, buff, " %lg", nrrd->axis[i].min);
      strcat(str, buff);
    }
    break;
  case nrrdField_axis_maxs:
    sprintf(str, "%s:", fs);
    for (i=0; i<D; i++) {
      airSinglePrintf(NULL, buff, " %lg", nrrd->axis[i].max);
      strcat(str, buff);
    }
    break;
  case nrrdField_centers:
    sprintf(str, "%s:", fs);
    for (i=0; i<D; i++) {
      sprintf(buff, " %s",
	      (nrrd->axis[i].center 
	       ? airEnumStr(nrrdCenter, nrrd->axis[i].center)
	       : NRRD_UNKNOWN));
      strcat(str, buff);
    }
    break;
  case nrrdField_labels:
    sprintf(str, "%s:", fs);
    for (i=0; i<D; i++) {
      sprintf(buff, " \"%s\"", 
	      airStrlen(nrrd->axis[i].label) ? nrrd->axis[i].label : "");
      strcat(str, buff);
    }
    break;
    /* ---- end per-axis fields ---- */
  case nrrdField_number:
    sprintf(str, "%s: " AIR_SIZE_T_FMT, fs, nrrdElementNumber(nrrd));
    break;
  case nrrdField_content:
    sprintf(str, "%s: %s", fs, airOneLinify(nrrd->content));
    break;
  case nrrdField_block_size:
    sprintf(str, "%s: %d", fs, nrrd->blockSize);
    break;
  case nrrdField_min:
    sprintf(str, "%s: ", fs);
    airSinglePrintf(NULL, buff, "%lg", nrrd->min);
    strcat(str, buff);
    break;
  case nrrdField_max:
    sprintf(str, "%s: ", fs);
    airSinglePrintf(NULL, buff, "%lg", nrrd->max);
    strcat(str, buff);
    break;
  case nrrdField_old_min:
    sprintf(str, "%s: ", fs);
    airSinglePrintf(NULL, buff, "%lg", nrrd->oldMin);
    strcat(str, buff);
    break;
  case nrrdField_old_max:
    sprintf(str, "%s: ", fs);
    airSinglePrintf(NULL, buff, "%lg", nrrd->oldMax);
    strcat(str, buff);
    break;
  case nrrdField_data_file:
    if (airStrlen(io->dataFN)) {
      /* for "unu make -h" */
      sprintf(str, "%s: %s", fs, io->dataFN);
    } else {
      sprintf(str, "%s: ./%s", fs, io->base);
    }
    break;
  case nrrdField_line_skip:
    sprintf(str, "%s: %d", fs, io->lineSkip);
    break;
  case nrrdField_byte_skip:
    sprintf(str, "%s: %d", fs, io->byteSkip);
    break;
  }

  return;
}

#define _PRINT_FIELD(prefix, field) \
if (_nrrdFieldInteresting(nrrd, io, field)) { \
  _nrrdSprintFieldInfo(line, nrrd, io, field), \
  fprintf(file, "%s%s\n", prefix, line); \
}

int
_nrrdWriteNrrd(FILE *file, Nrrd *nrrd, NrrdIO *io, int writeData) {
  char me[]="_nrrdWriteNrrd", err[AIR_STRLEN_MED], 
    tmpName[NRRD_STRLEN_LINE],  line[NRRD_STRLEN_LINE];
  int i;
  
  if (writeData) {
    if (io->seperateHeader) {
      sprintf(tmpName, "%s/%s", io->dir, io->base);
      io->dataFile = fopen(tmpName, "wb");
      if (!io->dataFile) {
	sprintf(err, "%s: couldn't fopen(\"%s\",\"wb\"): %s",
		me, tmpName, strerror(errno));
	biffAdd(NRRD, err); return 1;
      }
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
    fprintf(file, "%c %s\n", _NRRD_COMMENT_CHAR, nrrd->cmt[i]);
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
_nrrdWritePNM(FILE *file, Nrrd *nrrd, NrrdIO *io) {
  char me[]="_nrrdWritePNM", err[AIR_STRLEN_MED];
  char line[NRRD_STRLEN_LINE];
  int i, color, sx, sy, magic;
  
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
    if (_nrrdFieldValidInPNM[i]) { 
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

int
_nrrdWriteTable(FILE *file, Nrrd *nrrd, NrrdIO *io) {
  char cmt[AIR_STRLEN_SMALL], line[NRRD_STRLEN_LINE],
    buff[AIR_STRLEN_SMALL];
  size_t I;
  int i, x, y, sx, sy;
  void *data;
  float val;

  sprintf(cmt, "%c ", _NRRD_COMMENT_CHAR);
  /* If dimension is 1, we always print it. Dimension of 2 is 
     otherwise assumed. */
  if (1 == nrrd->dim) {
    _PRINT_FIELD(cmt, nrrdField_dimension);
  }
  if (!io->bareTable) {
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
** HEY: this is where the filename of a seperate datafile is determined
*/
void
_nrrdGuessFormat(NrrdIO *io, const char *filename) {
  int strpos;

  /* currently, we play the detached header game whenever the filename
     ends in NRRD_EXT_HEADER, and when we play this game, the data file
     is ALWAYS header relative. */
  /* we assume that that io->encoding is valid at this point: the
     seperate datafile will have an extension determined by 
     the encoding (and having extension "(unknown encoding)" would
     be a real drag). */
  strpos = strlen(io->base) - strlen(NRRD_EXT_HEADER);
  if (airEndsWith(filename, NRRD_EXT_HEADER)) {
    io->base[strpos++] = '.';
    strcpy(io->base + strpos,
	   airEnumStr(nrrdEncoding, io->encoding));
    io->seperateHeader = AIR_TRUE;
    io->format = nrrdFormatNRRD;
  } else if (airEndsWith(filename, NRRD_EXT_PGM) 
	     || airEndsWith(filename, NRRD_EXT_PPM)) {
    io->format = nrrdFormatPNM;
  } else if (airEndsWith(filename, NRRD_EXT_TABLE)) {
    io->format = nrrdFormatTable;
  } else {
    /* nothing obvious */
    io->format = nrrdFormatUnknown;
  }
}

void
_nrrdFixFormat(NrrdIO *io, Nrrd *nrrd) {
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
    fits = nrrdFitsInFormat(nrrd, nrrdFormatPNM, AIR_FALSE);
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
  case nrrdFormatTable:
    if (!nrrdFitsInFormat(nrrd, nrrdFormatTable, AIR_FALSE)) {
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
nrrdWrite(FILE *file, Nrrd *nrrd, NrrdIO *io) {
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
  if (!AIR_BETWEEN(nrrdFormatUnknown, io->format, nrrdFormatLast)) {
    sprintf(err, "%s: invalid format %d", me, io->format);
    biffAdd(NRRD, err); return 1;
  }
  if (!nrrdFitsInFormat(nrrd, io->format, AIR_TRUE)) {
    sprintf(err, "%s: doesn't fit in %s format", me,
	    airEnumStr(nrrdFormat, io->format));
    biffAdd(NRRD, err); return 1;
  }

  switch (io->format) {
  case nrrdFormatNRRD:
    ret = _nrrdWriteNrrd(file, nrrd, io, AIR_TRUE);
    break;
  case nrrdFormatPNM:
    ret = _nrrdWritePNM(file, nrrd, io);
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
nrrdSave(const char *filename, Nrrd *nrrd, NrrdIO *io) {
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
    _nrrdSplitName(io->dir, io->base, filename);
    _nrrdGuessFormat(io, filename);
    _nrrdFixFormat(io, nrrd);
  }
  if (!( AIR_INSIDE(nrrdFormatUnknown, io->format, nrrdFormatLast) )) {
    sprintf(err, "%s: invalid format %d\n", me, io->format);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }

  if (!strcmp("-", filename)) {
    file = stdout;
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

