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

/* field identifier string */
char
_nrrdFieldStr[_NRRD_NUMFIELDS][NRRD_SMALL_STRLEN] = {

  "type",       /*  0: required, anywhere */
  "encoding",   /*  1: required, anywhere */
  "endian",     /*  2: required for some types, anywhere */

  "dimension",  /*  3: required, must precede all the "_s" axis fields */

  "sizes",      /*  4: required, must follow dimension */
  "spacings",   /*  5: not required, must follow dimension */
  "axis mins",  /*  6: " */
  "axis maxs",  /*  7:  " */
  "centers",    /*  8: " */
  "labels",     /*  9: " */

  "number",     /* 10: not required, anywhere */
  "content",    /* 11: " */
  "block size", /* 12: " */
  "min",        /* 13: " */
  "max",        /* 14: " */
  "old min",    /* 15: " */
  "old max",    /* 16: " */

  "data file",  /* 17: not required, anywhere */
  "line skip",  /* 18: " */
  "byte skip",  /* 19: " */

};

char _nrrdRelDirFlag[] = "./";
char _nrrdMultiFieldSep[] = " \t";
char _nrrdTableSep[] = " ,\t";

/*
** _nrrdGotValidHeader
**
** consistency checks on relationship between fields of nrrd
** 
** ALSO, it is here that we do the courtesy setting of the
** "num" field (to the product of the axes' sizes)
*/
int
_nrrdGotValidHeader(Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdGotValidHeader", err[NRRD_MED_STRLEN];
  int i;
  NRRD_BIG_INT num;
  
  if (nrrdTypeUnknown == nrrd->type) {
    sprintf(err, "%s: didn't see required field: type", me);
    biffAdd(NRRD, err); return 0;
  }
  if (nrrdTypeBlock == nrrd->type && -1 == nrrd->blockSize) {
    sprintf(err, "%s: type is block, but missing field: block size", me);
    biffAdd(NRRD, err); return 0;
  }
  if (nrrdEncodingUnknown == io->encoding) {
    sprintf(err, "%s: didn't see required field: encoding", me);
    biffAdd(NRRD, err); return 0;
  }
  if (airEndianUnknown == io->endian &&
      nrrdEncodingEndianMatters[io->encoding] &&
      1 != nrrdElementSize(nrrd)) {
    sprintf(err, "%s: type (%s) and encoding (%s) require field: endian",
	    me, nrrdEnumValToStr(nrrdEnumType, nrrd->type),
	    nrrdEnumValToStr(nrrdEnumEncoding, io->encoding));
    biffAdd(NRRD, err); return 0;    
  }
  if (-1 == nrrd->dim) {
    sprintf(err, "%s: didn't see required field: dimension", me);
    biffAdd(NRRD, err); return 0;
  }

  /* HEY: should we check on validity of min/max/center/size combination */
  
  num = 1;
  for (i=0; i<=nrrd->dim-1; i++) {
    num *= nrrd->axis[i].size;
  }
  if (-1 != nrrd->num) {
    if (num != nrrd->num) {
      sprintf(err, "%s: given \"number\" disagrees with product of \"sizes\"",
	      me);
      biffAdd(NRRD, err); return 0;
    }
  }
  else {
    nrrd->num = num;
  }
  
  return 1;
}

int
_nrrdReadDataRaw(Nrrd *nrrd, nrrdIO *io) {
  
  return 0;
}

int
_nrrdReadDataAscii(Nrrd *nrrd, nrrdIO *io) {

  return 0;
}

int
(*_nrrdReadData[NRRD_ENCODING_MAX+1])(Nrrd *, nrrdIO *) = {
  NULL,
  _nrrdReadDataRaw,
  _nrrdReadDataAscii
};

int
_nrrdReadNrrd(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdReadNrrd", err[NRRD_LINEBUFF_STRLEN+NRRD_MED_STRLEN];
  int len;

  /* parse header lines */
  while (1) {
    len = airOneLine(file, io->line, NRRD_LINEBUFF_STRLEN);
    if (len > 1) {
      if (_nrrdReadNrrdParseLine(nrrd, io)) {
	sprintf(err, "%s: trouble on line \"%s\"\n", me, io->line);
	biffAdd(NRRD, err); return 1;
      }
    }
    else {
      break;
    }
  }

  /* though not actually necessary, do this error check here as a courtesy */
  if (!len) {
    if (!io->file) {
      sprintf(err, "%s: hit end of header, but no \"data file\" given", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  
  if (!_nrrdGotValidHeader(nrrd, io)) {
    sprintf(err, "%s: %s", me, 
	    (len ? "finished reading header, but there were problems"
	     : "hit EOF before seeing a complete header"));
    biffAdd(NRRD, err); return 1;
  }
  
  /* we seemed to have read a valid header; now read the data */
  if (!io->file) {
    io->file = file;
  }
  if (_nrrdReadData[io->encoding](nrrd, io)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  
  return 0;
}

int
_nrrdReadPNM(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  /*
  char me[]="_nrrdReadPNM", err[NRRD_MED_STRLEN];
  */

  return 0;
}

int
_nrrdReadTable(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  /*
  char me[]="_nrrdReadTable", err[NRRD_MED_STRLEN];
  */
  
  return 0;
}

/*
** if we have a file name for the header file ("file") being read,
** it must be placed in io->dir, with no trailing "/"
*/
int
nrrdRead(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdRead";
  int len, magic;
  float onefloat;

  printf("!%s: hello\n", me);
  if (!(file && nrrd && io)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  nrrdInit(nrrd);
  len = airOneLine(file, io->line, NRRD_LINEBUFF_STRLEN);
  printf("!%s: got |%s|\n", me, io->line);
  if (!len) {
    sprintf(err, "%s: immediately hit EOF", me);
    biffAdd(NRRD, err); return 1;
  }
  
  /* we have one line, see if there's magic, or comments, or numbers */
  magic = nrrdEnumStrToVal(nrrdEnumMagic, io->line);
  printf("!%s: magic = %d\n", me, magic);
  switch (magic) {
  case nrrdMagicNRRD0001:
    if (_nrrdReadNrrd(file, nrrd, io)) {
      sprintf(err, "%s: trouble reading NRRD", me);
      biffAdd(NRRD, err); return 1;
    }
    io->format = nrrdFormatNRRD;
    break;
  case nrrdMagicP2:
  case nrrdMagicP3:
  case nrrdMagicP5:
  case nrrdMagicP6:
    if (_nrrdReadPNM(file, nrrd, io)) {
      sprintf(err, "%s: trouble reading PNM", me);
      biffAdd(NRRD, err); return 1;
    }
    io->format = nrrdFormatPNM;
    break;
  default:
    /* see if line is a comment, which implies its a table */
    if (NRRD_COMMENT_CHAR == io->line[0]) {
      if (nrrdCommentAdd(nrrd, io->line+1, AIR_TRUE)) {
	sprintf(err, "%s: couldn't add initial comment", me);
	biffAdd(NRRD, err); return 1;
      }
      strcpy(io->line, "");
      if (_nrrdReadTable(file, nrrd, io)) {
	sprintf(err, "%s: trouble reading table (a)", me);
	biffAdd(NRRD, err); return 1;
      }
    }
    else {
      /* it still might be a table- see if we can parse one float */
      if (1 != airParseStrF(&onefloat, io->line, _nrrdTableSep, 1)) {
	sprintf(err, "%s: couldn't parse as NRRD, PNM, or table", me);
	biffAdd(NRRD, err); return 1;
      }
      /* we could parse one float; try to read it as a table */
      if (_nrrdReadTable(file, nrrd, io)) {
	sprintf(err, "%s: trouble reading table (b)", me);
	biffAdd(NRRD, err); return 1;
      }
      io->format = nrrdFormatTable;
    }
    break;
  }
  return 0;
}

/*
** _nrrdSplitName()
*/
int
_nrrdSplitName(char *dir, char *base, char *name) {
  int i, ret;
  
  i = strrchr(name, '/') - name;
  /* we found a valid break if the last directory character
     is somewhere in the string except the last character */
  if (i>=0 && i<strlen(name)-1) {
    strcpy(dir, name);
    dir[i] = 0;
    strcpy(base, name + i);
    ret = 1;
  }
  else {
    strcpy(dir, ".");
    strcpy(base, name);
    ret = 0;
  }
  return ret;
}

int
nrrdLoad(Nrrd *nrrd, char *filename) {
  char me[]="nrrdLoad", err[NRRD_MED_STRLEN];
  nrrdIO *io;
  FILE *file;

  printf("!%s: hello\n", me);
  if (!(nrrd && filename)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }

  /* we save the directory of the filename given to us so
     that if it turns out that its just a nhdr file, with
     a header-relative data file, then we will know how
     to find the data file */
  io = nrrdIONew();
  if (!io) {
    sprintf(err, "%s: couldn't alloc something", me);
    biffAdd(NRRD, err); return 1;
  }
  _nrrdSplitName(io->dir, io->base, filename);
  printf("!%s: |%s|%s|\n", me, io->dir, io->base);

  if (!strcmp("-", filename)) {
    file = stdin;
  }
  else {
    file = fopen(filename, "rb");
    if (!file) {
      sprintf(err, "%s: fopen(\"%s\",\"rb\") failed: %s", 
	      me, filename, strerror(errno));
      biffAdd(NRRD, err); return 1;
    }
  }
  if (nrrdRead(file, nrrd, io)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }

  if (file != stdin)
    fclose(file);
  io = nrrdIONix(io);
  return 0;
}

