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
#include "private.h"

int
_nrrdFieldInteresting(Nrrd *nrrd, nrrdIO *io, int field) {
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
    /* comments are always handled differently */
    ret = 0;
    break;
  case nrrdField_number:
    /* This may be somewhat surprising.  The truth is, "number" is
       entirely redundant with "sizes", which is a required field.
       Absolutely nothing is lost in eliding "number" from the header.
       Should this judgement later be found in error, this is the one
       place where the policy change can be implemented */
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
    for (ret=d=0; d<=nrrd->dim-1; d++) {
      ret |= AIR_EXISTS(nrrd->axis[d].spacing);
    }
    break;
  case nrrdField_axis_mins:
    for (ret=d=0; d<=nrrd->dim-1; d++) {
      ret |= AIR_EXISTS(nrrd->axis[d].min);
    }
    break;
  case nrrdField_axis_maxs:
    for (ret=d=0; d<=nrrd->dim-1; d++) {
      ret |= AIR_EXISTS(nrrd->axis[d].max);
    }
    break;
  case nrrdField_centers:
    for (ret=d=0; d<=nrrd->dim-1; d++) {
      ret |= !!(nrrd->axis[d].center);
    }
    break;
  case nrrdField_labels:
    for (ret=d=0; d<=nrrd->dim-1; d++) {
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
_nrrdWriteDataRaw(Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdWriteDataRaw", err[NRRD_STRLEN_MED];
  nrrdBigInt bsize;
  size_t size, ret, dio;
  
  bsize = nrrd->num * nrrdElementSize(nrrd);
  size = bsize;
  if (size != bsize) {
    fprintf(stderr, "%s: PANIC, sorry", me);
    exit(1);
  }

  dio = airDioTest(size, io->dataFile, nrrd->data);
  if (airNoDio_okay == dio) {
    if (nrrdFormatNRRD == io->format) {
      fprintf(stderr, "using direct I/O ... "); fflush(stderr);
    }
    ret = airDioWrite(io->dataFile, nrrd->data, size);
    if (size != ret) {
      sprintf(err, "%s: airDioWrite failed", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  else {
    if (AIR_DIO && nrrdFormatNRRD == io->format) {
      fprintf(stderr, "using fwrite() ... "); fflush(stderr);
    }
    ret = fwrite(nrrd->data, nrrdElementSize(nrrd), nrrd->num, io->dataFile);
    if (ret != nrrd->num) {
      sprintf(err, "%s: unable to complete fwrite()", me);
      biffAdd(NRRD, err); return 1;
    }
    /*
    if (ferror(io->dataFile)) {
      sprintf(err, "%s: ferror returned non-zero", me);
      biffAdd(NRRD, err); return 1;
    }
    */
  }
  return 0;
}

#define _NRRD_VALS_PER_LINE 6
#define _NRRD_CHARS_PER_LINE 73

int
_nrrdWriteDataAscii(Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdWriteDataAscii", err[NRRD_STRLEN_MED], 
    buff[NRRD_STRLEN_MED];
  int size, bufflen, linelen;
  char *data;
  nrrdBigInt I;
  
  if (nrrdTypeBlock == nrrd->type) {
    sprintf(err, "%s: can't write blocks to ascii", me);
    biffAdd(NRRD, err); return 1;
  }
  data = nrrd->data;
  size = nrrdElementSize(nrrd);
  linelen = 0;
  for (I=0; I<=nrrd->num-1; I++) {
    nrrdSprint[nrrd->type](buff, data);
    if (1 == nrrd->dim) {
      fprintf(io->dataFile, "%s\n", buff);
    }
    else if (nrrd->dim == 2 && nrrd->axis[0].size <= _NRRD_VALS_PER_LINE) {
      fprintf(io->dataFile, "%s%c", buff,
	      (I+1)%(nrrd->axis[0].size) ? ' ' : '\n');
    }
    else {
      bufflen = strlen(buff);
      if (linelen < _NRRD_CHARS_PER_LINE
	  && linelen+bufflen+1 >= _NRRD_CHARS_PER_LINE) {
	fprintf(io->dataFile, "%s\n", buff);
	linelen = 0;
      }
      else {
	fprintf(io->dataFile, "%s ", buff);
	linelen += bufflen + 1;
      }
    }
    data += size;
  }
  
  return 0;
}

int
(*_nrrdWriteData[NRRD_ENCODING_MAX+1])(Nrrd *, nrrdIO *) = {
  NULL,
  _nrrdWriteDataRaw,
  _nrrdWriteDataAscii
};

/*
** _nrrdSprintFieldInfo
**
** this prints "<field>: <info>" into given string "str", in a form
** suitable to be written to NRRD or PNM headers.  This will always
** print something, even stupid <info>s like "(unknown endian)".
** It is up to the caller to decide which fields are worth writing.
*/
void
_nrrdSprintFieldInfo(char *str, Nrrd *nrrd, nrrdIO *io, int field) {
  char buff[NRRD_STRLEN_MED], *fs;
  int i, D;

  if (!( str 
	 && nrrd 
	 && AIR_INSIDE(1, nrrd->dim, NRRD_DIM_MAX)
	 && AIR_BETWEEN(nrrdField_unknown, field, nrrdField_last) )) {
    return;
  }

  D = nrrd->dim;
  fs = nrrdEnumValToStr(nrrdEnumField, field);
  switch (field) {
  case nrrdField_comment:
    /* this is handled differently */
    strcpy(str, "");
    break;
  case nrrdField_type:
    sprintf(str, "%s: %s", fs,
	    nrrdEnumValToStr(nrrdEnumType, nrrd->type));
    break;
  case nrrdField_encoding:
    sprintf(str, "%s: %s", fs,
	    nrrdEnumValToStr(nrrdEnumEncoding, io->encoding));
    break;
  case nrrdField_endian:
    /* note we record our current architecture's endian */
    sprintf(str, "%s: %s", fs,
	    nrrdEnumValToStr(nrrdEnumEndian, AIR_ENDIAN));
    break;
  case nrrdField_dimension:
    sprintf(str, "%s: %d", fs, nrrd->dim);
    break;
    /* ---- begin per-axis fields ---- */
  case nrrdField_sizes:
    sprintf(str, "%s:", fs);
    for (i=0; i<=D-1; i++) {
      sprintf(buff, " %d", nrrd->axis[i].size);
      strcat(str, buff);
    }
    break;
  case nrrdField_spacings:
    sprintf(str, "%s:", fs);
    for (i=0; i<=D-1; i++) {
      airSinglePrintf(NULL, buff, " %lg", nrrd->axis[i].spacing);
      strcat(str, buff);
    }
    break;
  case nrrdField_axis_mins:
    sprintf(str, "%s:", fs);
    for (i=0; i<=D-1; i++) {
      airSinglePrintf(NULL, buff, " %lg", nrrd->axis[i].min);
      strcat(str, buff);
    }
    break;
  case nrrdField_axis_maxs:
    sprintf(str, "%s:", fs);
    for (i=0; i<=D-1; i++) {
      airSinglePrintf(NULL, str, " %lg", nrrd->axis[i].max);
      strcat(str, buff);
    }
    break;
  case nrrdField_centers:
    sprintf(str, "%s:", fs);
    for (i=0; i<=D-1; i++) {
      sprintf(buff, " %s", 
	      nrrdEnumValToStr(nrrdEnumCenter, nrrd->axis[i].center));
      strcat(str, buff);
    }
    break;
  case nrrdField_labels:
    sprintf(str, "%s:", fs);
    for (i=0; i<=D-1; i++) {
      sprintf(buff, " \"%s\"", 
	      airStrlen(nrrd->axis[i].label) ? nrrd->axis[i].label : "");
      strcat(str, buff);
    }
    break;
    /* ---- end per-axis fields ---- */
  case nrrdField_number:
    sprintf(str, "%s: " NRRD_BIG_INT_PRINTF, fs, nrrd->num);
    break;
  case nrrdField_content:
    sprintf(str, "%s: %s", fs, airOneLinify(nrrd->content));
    break;
  case nrrdField_block_size:
    sprintf(str, "%s: %d", fs, nrrd->blockSize);
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
    airSinglePrintf(NULL, buff, "%lg", nrrd->oldMin);
    strcat(str, buff);
    break;
  case nrrdField_data_file:
    sprintf(str, "%s: ./%s", fs, io->base);
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
_nrrdWriteNrrd(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdWriteNrrd", err[NRRD_STRLEN_MED], 
    tmpName[NRRD_STRLEN_LINE],  line[NRRD_STRLEN_LINE];
  int i;
  
  if (io->seperateHeader) {
    sprintf(tmpName, "%s/%s", io->dir, io->base);
    io->dataFile = fopen(tmpName, "wb");
    if (!io->dataFile) {
      sprintf(err, "%s: couldn't fopen(\"%s\",\"wb\"): %s",
	      me, tmpName, strerror(errno));
      biffAdd(NRRD, err); return 1;
    }
  }
  else {
    io->dataFile = file;
  }

  fprintf(file, "%s\n", NRRD_HEADER);

  for (i=1; i<=NRRD_FIELD_MAX; i++)
    _PRINT_FIELD("", i);

  for (i=0; i<=nrrd->cmtArr->len-1; i++) {
    fprintf(file, "%c %s\n", _NRRD_COMMENT_CHAR, nrrd->cmt[i]);
  }

  if (!io->seperateHeader) {
    fprintf(file, "\n");
  }

  fprintf(stderr, "(%s: writing %s data ... ", me, 
	  nrrdEnumValToStr(nrrdEnumEncoding, io->encoding));
  fflush(stderr);
  if (_nrrdWriteData[io->encoding](nrrd, io)) {
    fprintf(stderr, "error!\n");
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  fprintf(stderr, "done)\n");

  if (io->seperateHeader) {
    io->dataFile = airFclose(io->dataFile);
  }

  return 0;
}

int
_nrrdWritePNM(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdWritePNM", err[NRRD_STRLEN_MED];
  char line[NRRD_STRLEN_LINE];
  int i, color, sx, sy, magic;
  
  color = (3 == nrrd->dim);
  if (!color) {
    magic = (nrrdEncodingAscii == io->encoding
	     ? nrrdMagicP2
	     : nrrdMagicP5);
    sx = nrrd->axis[0].size;
    sy = nrrd->axis[1].size;
  }
  else {
    magic = (nrrdEncodingAscii == io->encoding
	     ? nrrdMagicP3
	     : nrrdMagicP6);
    sx = nrrd->axis[1].size;
    sy = nrrd->axis[2].size;
  }
  
  fprintf(file, "%s\n", nrrdEnumValToStr(nrrdEnumMagic, magic));
  fprintf(file, "%d %d\n", sx, sy);
  for (i=1; i<=NRRD_FIELD_MAX; i++) {
    if (_nrrdFieldValidInPNM[i]) {
      _PRINT_FIELD(NRRD_PNM_COMMENT, i);
    }
  }
  for (i=0; i<=nrrd->cmtArr->len-1; i++) {
    fprintf(file, "# %s\n", nrrd->cmt[i]);
  }
  fprintf(file, "255\n");

  io->dataFile = file;
  if (_nrrdWriteData[io->encoding](nrrd, io)) {
    fprintf(stderr, "error!\n");
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  
  return 0;
}

int
_nrrdWriteTable(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  char cmt[NRRD_STRLEN_SMALL], line[NRRD_STRLEN_LINE];
  nrrdBigInt I;
  int i, x, y, sx, sy;
  void *data;
  
  sprintf(cmt, "%c ", _NRRD_COMMENT_CHAR);
  if (!io->bareTable) {
    for (i=1; i<=NRRD_FIELD_MAX; i++) {
      if (_nrrdFieldValidInTable[i]) {
	_PRINT_FIELD(cmt, i);
      }
    }
  }

  sx = nrrd->axis[0].size;
  sy = nrrd->axis[1].size;
  data = nrrd->data;
  I = 0;
  for (y=0; y<=sy-1; y++) {
    for (x=0; x<=sx-1; x++) {
      if (x) fprintf(file, " ");
      airSinglePrintf(file, NULL, "%g", nrrdFLookup[nrrd->type](data, I));
      I++;
    }
    fprintf(file, "\n");
  }
  
  return 0;
}

void
_nrrdGuessFormat(char *filename, Nrrd *nrrd, nrrdIO *io) {

  /* currently, we play the detached header game whenever the filename
     ends in NRRD_EXT_HEADER, and when we play this game, the data file
     is ALWAYS header relative. */
  if (2 <= airEndsWith(filename, NRRD_EXT_HEADER)) {
    strcpy(io->base + strlen(io->base) - strlen(NRRD_EXT_HEADER),
	   NRRD_EXT_RAW);
    io->seperateHeader = AIR_TRUE;
    io->format = nrrdFormatNRRD;
  }
  else if (2 <= airEndsWith(filename, NRRD_EXT_PGM)
	   || 1 <= airEndsWith(filename, NRRD_EXT_PPM)) {
    io->format = nrrdFormatPNM;
  }
  else if (2 <= airEndsWith(filename, NRRD_EXT_TABLE)) {
    io->format = nrrdFormatTable;
  }
  else {
    /* filename does not suggest any particular format */
    io->format = NRRD_FORMAT_DEFAULT;
  }
}

void
_nrrdFixFormat(Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdFixFormat";
  int fits;

  switch(io->format) {
  case nrrdFormatNRRD:
    /* everything fits in a nrrd */
    break;
  case nrrdFormatPNM:
    fits = nrrdFitsInFormat(nrrd, nrrdFormatPNM, AIR_FALSE);
    if (!fits) {
      fprintf(stderr, "(%s: Can't be a PNM image; saving as NRRD)\n", me); 
      io->format = nrrdFormatNRRD;
    }
    else {
      if (2 == fits && airEndsWith(io->base, NRRD_EXT_PPM)) {
	fprintf(stderr, "(%s: Image is grayscale; saving as PGM)\n", me); 
      }
      if (3 == fits && airEndsWith(io->base, NRRD_EXT_PGM)) {
	fprintf(stderr,	"(%s: Image is color; saving as PPM)\n", me); 
      }
    }
    break;
  case nrrdFormatTable:
    if (!nrrdFitsInFormat(nrrd, nrrdFormatTable, AIR_FALSE)) {
      fprintf(stderr, "(%s: Can't be a table; saving as NRRD)\n", me);
      io->format = nrrdFormatNRRD;
    }
    break;
  }
  
  return;
}

int
nrrdWrite(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  char me[]="nrrdWrite", err[NRRD_STRLEN_MED];
  int ret;

  if (!(file && nrrd && io)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!nrrdValid(nrrd)) {
    sprintf(err, "%s: trouble", me);
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
	    nrrdEnumValToStr(nrrdEnumFormat, io->format));
    biffAdd(NRRD, err); return 1;
  }

  switch (io->format) {
  case nrrdFormatNRRD:
    ret = _nrrdWriteNrrd(file, nrrd, io);
    break;
  case nrrdFormatPNM:
    ret = _nrrdWritePNM(file, nrrd, io);
    break;
  case nrrdFormatTable:
    ret = _nrrdWriteTable(file, nrrd, io);
    break;
  }
  if (ret) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

int
nrrdSave(char *filename, Nrrd *nrrd, nrrdIO *io) {
  char me[]="nrrdSave", err[NRRD_STRLEN_MED];
  FILE *file;
  int wantedFormat;

  if (!(nrrd && filename)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!io) {
    io = nrrdIONew();
    if (!io) {
      sprintf(err, "%s: couldn't alloc something", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  else {
    
  }
  if (nrrdEncodingUnknown == io->encoding) {
    io->encoding = NRRD_ENCODING_DEFAULT;
  }
  else if (!AIR_BETWEEN(nrrdEncodingUnknown, io->encoding, 
			nrrdEncodingLast)) {
    sprintf(err, "%s: invalid encoding %d\n", me, io->encoding);
    biffAdd(NRRD, err); return 1;
  }

  if (!strcmp("-", filename)) {
    file = stdout;
  }
  else {
    if (!(file = fopen(filename, "wb"))) {
      sprintf(err, "%s: couldn't fopen(\"%s\",\"wb\"): %s", 
	      me, filename, strerror(errno));
      biffAdd(NRRD, err); return 1;
    }
  }

  _nrrdSplitName(io->dir, io->base, filename);
  _nrrdGuessFormat(filename, nrrd, io);
  _nrrdFixFormat(nrrd, io);
  
  if (nrrdWrite(file, nrrd, io)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  fclose(file);
  io = nrrdIONix(io);
  return 0;
}

/* Ernesto "Che" Guevara  */

