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
_nrrdWriteNrrd(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdWriteNrrd", err[NRRD_MED_STRLEN], 
    tmpName[NRRD_LINEBUFF_STRLEN];
  int i, D, doit;
  FILE *dataFile;
  
  if (io->seperateHeader) {
    sprintf(tmpName, "%s/%s", io->dir, io->base);
    dataFile = fopen(tmpName, "wb");
    if (!dataFile) {
      sprintf(err, "%s: couldn't fopen(\"%s\",\"wb\"): %s",
	      me, tmpName, strerror(errno));
      biffAdd(NRRD, err); return 1;
    }
  }
  else {
    dataFile = file;
  }

  fprintf(file, "%s\n", NRRD_HEADER);

  if (airStrlen(nrrd->content))
    fprintf(file, "content: %s\n", airOneLinify(nrrd->content));

  fprintf(file, "type: %s\n", nrrdEnumValToStr(nrrdEnumType, nrrd->type));
  if (nrrdTypeBlock == nrrd->type) {
    fprintf(file, "block size: %d\n", nrrd->blockSize);
  }

  D = nrrd->dim;
  fprintf(file, "dimension: %d\n", D);

  fprintf(file, "sizes:");
  for (i=0; i<=D-1; i++)
    fprintf(file, " %d", nrrd->axis[i].size);
  fprintf(file, "\n");

  doit = 0;
  for (i=0; i<=D-1; i++) doit |= AIR_EXISTS(nrrd->axis[i].spacing);
  if (doit) {
    fprintf(file, "spacings:");
    for (i=0; i<=D-1; i++)
      fprintf(file, " %lg", nrrd->axis[i].spacing);
    fprintf(file, "\n");
  }
  doit = 0;
  for (i=0; i<=D-1; i++) doit |= AIR_EXISTS(nrrd->axis[i].min);
  if (doit) {
    fprintf(file, "axis mins:");
    for (i=0; i<=D-1; i++)
      fprintf(file, " %lg", nrrd->axis[i].min);
    fprintf(file, "\n");
  }
  doit = 0;
  for (i=0; i<=D-1; i++) doit |= AIR_EXISTS(nrrd->axis[i].max);
  if (doit) {
    fprintf(file, "axis maxs:");
    for (i=0; i<=D-1; i++)
      fprintf(file, " %lg", nrrd->axis[i].max);
    fprintf(file, "\n");
  }
  doit = 0;
  for (i=0; i<=D-1; i++) doit |= (nrrdCenterUnknown != nrrd->axis[i].center);
  if (doit) {
    fprintf(file, "centers:");
    for (i=0; i<=D-1; i++)
      fprintf(file, " %s", nrrdEnumValToStr(nrrdEnumCenter, 
					    nrrd->axis[i].center));
    fprintf(file, "\n");
  }
  doit = 0;
  for (i=0; i<=D-1; i++) doit |= airStrlen(nrrd->axis[i].label);
  if (doit) {
    fprintf(file, "labels:");
    for (i=0; i<=D-1; i++)
      fprintf(file, " \"%s\"", (airStrlen(nrrd->axis[i].label)
				? nrrd->axis[i].label
				: ""));
    fprintf(file, "\n");
  }
  
  if (AIR_EXISTS(nrrd->min))
    fprintf(file, "min: %lg\n", nrrd->min);
  if (AIR_EXISTS(nrrd->max))
    fprintf(file, "max: %lg\n", nrrd->max);
  if (AIR_EXISTS(nrrd->oldMin)) 
    fprintf(file, "old min: %lg\n", nrrd->oldMin);
  if (AIR_EXISTS(nrrd->oldMax))
    fprintf(file, "old max: %lg\n", nrrd->oldMax);
  
  if (io->seperateHeader) {
    fprintf(file, "data file: ./%s\n", io->base);
  }
  if (io->lineSkip)
    fprintf(file, "line skip: %d\n", io->lineSkip);
  if (io->byteSkip)
    fprintf(file, "byte skip: %d\n", io->byteSkip);
  
  for (i=0; i<=nrrd->cmtNum-1; i++) {
    fprintf(file, "# %s\n", nrrd->cmt[i]);
  }

  if (!io->seperateHeader) {
    fprintf(file, "\n");
  }

  /* write data */

  if (io->seperateHeader) {
    fclose(dataFile);
  }

  return 0;
}

int
_nrrdWritePNM(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  /*
  char me[]="_nrrdWritePNM", err[NRRD_MED_STRLEN];
  */
  
  return 0;
}

int
_nrrdWriteTable(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  /*
  char me[]="_nrrdWriteTable", err[NRRD_MED_STRLEN];
  */

  return 0;
}

int
nrrdWrite(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  char me[]="nrrdWrite", err[NRRD_MED_STRLEN];
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
nrrdSave(Nrrd *nrrd, char *filename, int encoding) {
  char me[]="nrrdSave", err[NRRD_MED_STRLEN];
  nrrdIO *io;
  int fits;
  FILE *file;

  if (!(nrrd && filename)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_BETWEEN(nrrdEncodingUnknown, encoding, nrrdEncodingLast)) {
    sprintf(err, "%s: given encoding %d invalid", me, encoding);
    biffAdd(NRRD, err); return 1;
  }
  io = nrrdIONew();
  if (!io) {
    sprintf(err, "%s: couldn't alloc something", me);
    biffAdd(NRRD, err); return 1;
  }
  io->encoding = encoding;
  
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
  printf("!%s: |%s|%s|\n", me, io->dir, io->base);

  /* currently, we play the detached header game whenever the filename
     ends in NRRD_EXT_HEADER, and when we play this game, the data file
     is ALWAYS header relative. */
  if (1 <= airEndsWith(filename, NRRD_EXT_HEADER)) {
    strcpy(io->base + strlen(io->base) - strlen(NRRD_EXT_HEADER),
	   NRRD_EXT_RAW);
    io->seperateHeader = AIR_TRUE;
    io->format = nrrdFormatNRRD;
  }
  else if (1 <= airEndsWith(filename, NRRD_EXT_PGM)
	   || 1 <= airEndsWith(filename, NRRD_EXT_PPM)) {
    fits = nrrdFitsInFormat(nrrd, nrrdFormatPNM, AIR_FALSE);
    if (!fits) {
      fprintf(stderr, 
	      "(%s: given nrrd can't be a PNM image, saving as NRRD)\n", me); 
      io->format = nrrdFormatNRRD;
    }
    else {
      if (2 == fits && airEndsWith(filename, NRRD_EXT_PPM)) {
	fprintf(stderr,
		"(%s: given image is grayscale, saving as PGM)\n", me); 
      }
      if (3 == fits && airEndsWith(filename, NRRD_EXT_PGM)) {
	fprintf(stderr,
		"(%s: given image is color, saving as PPM)\n", me); 
      }
      io->format = nrrdFormatPNM;
    }
  }
  else if (1 <= airEndsWith(filename, NRRD_EXT_TABLE)) {
    if (!nrrdFitsInFormat(nrrd, nrrdFormatTable, AIR_FALSE)) {
      fprintf(stderr,
	      "(%s: given nrrd can't be a table, saving as NRRD)\n", me);
      io->format = nrrdFormatNRRD;
    }
    else {
      io->format = nrrdFormatTable;
    }
  }
  else {
    /* filename does not suggest any particular format */
    io->format = nrrdFormatNRRD;
  }
  
  if (nrrdWrite(file, nrrd, io)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  fclose(file);
  io = nrrdIONix(io);
  return 0;
}

/* Ernesto "Che" Guevara  */

