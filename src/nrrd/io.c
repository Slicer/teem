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

char _nrrdDirChars[]="/\\";
char _nrrdRelDirFlag[]=".";
char _nrrdHdrExt[]=".nhdr";
char _nrrdRawExt[]=".raw";
char _nrrdPGMExt[]=".pgm";
char _nrrdPPMExt[]=".ppm";

/* Ernesto "Che" Guevara  */

int _nrrdNumFields = 19;   /* number of field identifiers recognized */
char _nrrdMagicstr[NRRD_BIG_STRLEN];  /* hack: holder for magic string */

/* field identifier string */
char fieldStr[][NRRD_SMALL_STRLEN] = {  
  "content",
  "number",
  "type",
  "dimension",
  "encoding",
  "sizes",
  "spacings",
  "axis mins",
  "axis maxs",
  "labels",
  "min",
  "max",
  "blocksize",
  "data file",
  "line skip",
  "byte skip",
  "old min",
  "old max",
  "endian"
};

/* conversion character for info in the line */
char fieldConv[][NRRD_SMALL_STRLEN] = {  
  "%s",
  NRRD_BIG_INT_PRINTF,
  "%s",
  "%d",
  "%s",
  "%d",
  "%lf",
  "%lf",
  "%lf",
  "%s",
  "%lf",
  "%lf",
  "%d",
  "%s",
  "%d",
  "%d",
  "%lf",
  "%lf",
  "%s"
};

/* whether or not we parse as many items as the dimension */
int fieldMultiple[] = { 
  0,
  0,
  0,
  0,
  0,
  1,
  1,
  1,
  1,
  1,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0
};

/*
******** nrrdOneLine()
** 
** gets one line from "file", putting it into an array if given size.
** "size" must be the size of line buffer "line".  Always null-terminates
** the contents of the array (except if the arguments are invalid).
**
** -1: if arguments are invalid
** 0: if saw EOF before seeing a newline
** 1: if line was a single newline
** n, where n <= size: if line was n-1 characters followed by newline
** size+1: if didn't see a newline within size-1 characters
**
** So except for returns of -1 and size+1, the return is the number of
** characters comprising the line, including the newline character.
**
** This function does not use biff.
*/
int
nrrdOneLine(FILE *file, char *line, int size) {
  int c, i;
  
  if (!(size >= 2 && line && file))
    return -1;
  line[0] = 0;
  for (i=0; (i <= size-2 && 
	     EOF != (c=getc(file)) && 
	     c != '\n'); ++i)
    line[i] = c;
  if (EOF == c)
    return 0;
  if (0 == i) {
    /* !EOF ==> we stopped because of newline */
    return 1;
  }
  if (i == size-1) {
    line[size-1] = 0;
    if ('\n' != c) {
      /* we got to the end of the array and still no newline */
      return size+1;
    }
    else {
      /* we got to end of array just as we saw a newline, all's well */
      return size;
    }
  }
  /* i < size-1 && EOF != c ==> '\n' == c */
  line[i] = 0;
  return i+1;
}

/*
** _nrrdSscanfDouble
**
** the only point of having this is to have recognition of "nan"
** built into the parsing of strings into numbers
*/
int
_nrrdSscanfDouble(char *str, char *fmt, double *ptr) {
  int ret;
  char *c, *tmp;

  ret = -1;
  if (tmp = strdup(str)) {
    c = tmp;
    while (*c) {
      *c = tolower(*c);
      c++;
    }
    if (strstr(tmp, "nan")) {
      *ptr = airNand();
      ret = 1;
    }
    else {
      ret = sscanf(str, fmt, ptr);
      
    }
    free(tmp);
  }
  return ret;
}


/*
** _nrrdGetnums()
**
** give me a string, tell me how many numbers to parse, I'll put them
** in the given array
**
** This function uses biff.
*/
int
_nrrdGetnums(char *data, double *array, int num) {
  char err[NRRD_MED_STRLEN], me[] = "_nrrdGetnums";
  int i;

  if (!(num > 0))
    return 0;

  if (1 != _nrrdSscanfDouble(data, "%lg", &(array[0]))) {
    sprintf(err, "%s: couldn't get first num of %s", me, data);
    biffSet(NRRD, err); return 1;
  }
  i = 1;
  while (i < num) {
    if (!(data = strstr(data, " "))) {
      sprintf(err, "%s: didn't see space after number %d of %d", me, i, num);
      biffSet(NRRD, err); return 1;
    }
    data = &(data[1]);
    if (1 != _nrrdSscanfDouble(data, "%lg", &(array[i]))) {
      sprintf(err, "%s: couldn't parse %s for num %d", me, data, i+1);
      biffSet(NRRD, err); return 1;
    }
    i++;
  }
  return 0;
}

/*
** _nrrdGetstring()
**
** parse out one "" delimited from data, put it into str, but don't
** exceed size characters
**
** This function uses biff
*/
char *
_nrrdGetstring(char *data, char *str, int size) {
  char err[NRRD_MED_STRLEN], me[] = "_nrrdGetstring";
  int i;

  if (!(data && str && size >= 2)) {
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return NULL;
  }
  if (!('"' == data[0])) {
    sprintf(err, "%s: \"%s\" doesn't start with '\"'", me, data);
    biffSet(NRRD, err); return NULL;
  }
  data += 1;
  for (i=0; (i<=size-2 && 
	     !('"' == data[i] && '\\' != data[i-1])); i++)
    str[i] = data[i];
  if (i == size-1) {
    sprintf(err, "%s: didn't see end of string \"%s\" soon enough", me, data);
    biffSet(NRRD, err); return NULL;
  }
  str[i] = 0;
  return data+i;
}

void
_nrrdJoinFilename(char *fullname, char *dir, char *base) {

  /* do we have an idea of the header file? */  
  if (!strlen(dir))
    goto normal;

  /* does the file name start with the flag? */
  if (strncmp(base, _nrrdRelDirFlag, strlen(_nrrdRelDirFlag)))
    goto normal;

  /* is the next character a directory seperator? */
  if (!strchr(_nrrdDirChars, base[strlen(_nrrdRelDirFlag)]))
    goto normal;

  /* yes, yes, yes: we add "dir" to "base" */
  sprintf(fullname, "%s%s", dir, base+strlen(_nrrdRelDirFlag)+1);
  return;

 normal:
  strcpy(fullname, base);
  return;
}

/*
** _nrrdParseLine()
**
** does most of the work for parsing a nrrd header
*/ 
int
_nrrdParseLine(Nrrd *nrrd, char *line) {
  int f, i, type, endian, *ivals;
  char *next, *tmp, fullname[NRRD_BIG_STRLEN], str[NRRD_MED_STRLEN], 
    err[NRRD_MED_STRLEN], me[] = "_nrrdParseLine";
  void *field[18];
  double *dvals, invals[NRRD_MAX_DIM];

  field[0] = &nrrd->content;
  field[1] = &nrrd->num;
  field[2] = &nrrd->type;      /* code below assumes field 2 is type */
  field[3] = &nrrd->dim;
  field[4] = &nrrd->encoding;  /* code below assumes field 4 is encoding */
  field[5] = &nrrd->size;
  field[6] = &nrrd->spacing;
  field[7] = &nrrd->axisMin;
  field[8] = &nrrd->axisMax;
  field[9] = &nrrd->label;
  field[10] = &nrrd->min;
  field[11] = &nrrd->max;
  field[12] = &nrrd->blockSize;
  field[13] = NULL;           /* there is no actual field in the nrrd for
				 the string holding the name of the datafile,
				 we're just using this element of the local
				 "field" array for this purpose */
  field[14] = &nrrd->lineSkip;
  field[15] = &nrrd->byteSkip;
  field[16] = &nrrd->oldMin;
  field[17] = &nrrd->oldMax;
  field[18] = &nrrd->fileEndian;

  for (f=0; f<=_nrrdNumFields-1; f++) {
    if (!(strncmp(line, fieldStr[f], strlen(fieldStr[f])))) {
      next = line + strlen(fieldStr[f]);
      if (!(':' == next[0] && ' ' == next[1])) {
	/* HEY!!: if there's a space missing between the colon and the
	   rest, it doesn't matter that they got the field name right,
	   we just barf on the whole line; which is too bad */
	sprintf(err, "%s: didn't see \": \" after %s", me, fieldStr[f]);
	biffSet(NRRD, err); return 1;
      }
      next += 2;
      break;
    }
  }
  if (AIR_INSIDE(0, f, _nrrdNumFields-1)) {
    /*
    printf("_nrrdParseLine: saw field %d head, to parse \"%s\"", f, next);
    */
    if (!(fieldMultiple[f])) {
      if (!(strcmp(fieldConv[f], "%s"))) {
	if (4 == f) {
	  /* need to interpret encoding string */
	  for (i=0; i<=nrrdEncodingLast-1; i++) {
	    if (!(strcmp(next, nrrdEncoding2Str[i]))) {
	      nrrd->encoding = i;
	      break;
	    }
	  }
	  if (nrrdEncodingLast == i) {
	    sprintf(err, "%s: didn't recognize encoding \"%s\"", me, next);
	    biffSet(NRRD, err); return 1;
	  }
	  /*
	  printf("got encoding %d\n", nrrd->encoding);
	  */
	}
	else if (2 == f) {
	  /* need to interpret type string */
	  if (nrrdTypeUnknown == (type = nrrdStr2Type(next))) {
	    sprintf(err, "%s: didn't recognize type \"%s\"", me, next);
	    biffSet(NRRD, err); return 1;
	  }
	  nrrd->type = type;
	  /*
	  printf("got type %d\n", nrrd->type);
	  */
	}
	else if (13 == f || 14 == f) {
	  /* have to see if we can open the indicated data file */
	  _nrrdJoinFilename(fullname, nrrd->dir, next);
	  if (!(nrrd->dataFile = fopen(fullname, "rb"))) {
	    sprintf(err, "%s: fopen(\"%s\",\"rb\") failed: %s",
		    me, fullname, strerror(errno));
	    biffSet(NRRD, err); return 1;
	  }
	}
	else if (18 == f) {
	  /* have to parse the endianness */
	  if (nrrdEndianUnknown == (endian = nrrdStr2Endian(next))) {
	    sprintf(err, "%s: didn't recognize endianness \"%s\"", me, next);
	    biffSet(NRRD, err); return 1;
	  }
	  nrrd->fileEndian = endian;
	  /*
	  printf("got endian %d\n", nrrd->fileEndian);
	  */
	}
	else {
	  /* we don't have to act on the string other than copy it 
	     into the nrrd struct */
	  strcpy(field[f], next);
	}
      }
      else {
	/* else it is not a string that we need to parse */
	if (1 != (sscanf(next, fieldConv[f], field[f]))) {
	  sprintf(err, "%s: couldn't sscanf %s \"%s\" as %s",
		  me, fieldStr[f], next, fieldConv[f]);
	  biffSet(NRRD, err);
	  return 1;
	}
      }
    }
    else {
      /* we've got multiple items to parse */
      if (-1 == nrrd->dim) {
	sprintf(err, "%s: can't parse \"%s\" until dimension is known",
		me, fieldStr[f]);
	biffSet(NRRD, err); return 1;
      }
      if (!AIR_INSIDE(1, nrrd->dim, NRRD_MAX_DIM)) {
	sprintf(err, "%s: dimension %d is outside valid range [1,%d]",
		me, nrrd->dim, NRRD_MAX_DIM);
	biffSet(NRRD, err); return(1);
      }
      if (!(strcmp(fieldConv[f], "%s"))) {
	/* we have multiple strings to parse */
	for (i=0; i<=nrrd->dim-1; i++) {
	  if (!(tmp = _nrrdGetstring(next, str, NRRD_MED_STRLEN))) {
	    sprintf(err, "%s: couldn't get 1st string from \"%s\"", 
		    me, next);
	    biffAdd(NRRD, err); return 1;
	  }
	  strcpy(nrrd->label[i], str);
	  /* value for "next" set on last iteration is never dereferenced */
	  next = tmp + 2;  
	}
      }
      else {
	/* we have multiple numbers to parse */
	if (_nrrdGetnums(next, invals, nrrd->dim)) {
	  sprintf(err, "%s: couldn't parse %d numbers in \"%s\"",
		  me, nrrd->dim, next);
	  biffAdd(NRRD, err); return 1;
	}
	for (i=0; i<=nrrd->dim-1; i++)
	ivals = field[f];
	dvals = field[f];
	for (i=0; i<=nrrd->dim-1; i++) {
	  if (!(strcmp(fieldConv[f], "%lf"))) {
	    dvals[i] = invals[i];
	    /*
	    printf("got %d: %lf\n", i, dvals[i]);
	    */
	  }
	  else {
	    ivals[i] = invals[i];
	    /*
	    printf("got %d: %d\n", i, ivals[i]);
	    */
	  }
	}
      }
    }
  }
  else {
    /* didn't recognize any fields at the beginning of the line */
    sprintf(err, "%s: no recognized field identifiers in \"%s\"", me, line);
    biffSet(NRRD, err);
    return 1;
  }
  return 0;
}

int
_nrrdNonCmtLine(FILE *file, char *line, int size, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[] = "_nrrdNonCmtLine";
  int len;

  do {
    if (!(len = nrrdOneLine(file, line, size))) {
      sprintf(err, "%s: hit EOF", me); 
      biffSet(NRRD, err); 
      return 0;
    }
    if (NRRD_COMMENT_CHAR == line[0]) {
      nrrdAddComment(nrrd, line+1);
    }
  } while (NRRD_COMMENT_CHAR == line[0]);
  return len;
}

int
nrrdReadMagic(FILE *file) {
  char line[NRRD_BIG_STRLEN], err[NRRD_MED_STRLEN], 
    me[] = "nrrdReadMagic";
  int i, len;

  _nrrdMagicstr[0] = 0;
  do {
    if (!(len = nrrdOneLine(file, line, NRRD_BIG_STRLEN))) {
      sprintf(err, "%s: initial _nrrdNonCmtLine() hit EOF", me);
      biffSet(NRRD, err); return nrrdMagicUnknown;
    }
  } while (1 == len);

  /* --- got to first non-trivial line */
  strcpy(_nrrdMagicstr, line);
  for (i=nrrdMagicUnknown+1; i<nrrdMagicLast; i++) {
    if (!strcmp(line, nrrdMagic2Str[i])) {
      break;
    }
  }
  if (i < nrrdMagicLast) {
    return i;
  }
  else {
    return nrrdMagicUnknown;
  }
}

int
nrrdReadHeader(FILE *file, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdReadHeader";
  int magic;

  magic = nrrdReadMagic(file);
  switch (magic) {
  case nrrdMagicUnknown:
    sprintf(err, "%s: unknown magic \"%s\"", me, _nrrdMagicstr);
    biffSet(NRRD, err);
    return 1;
  case nrrdMagicNrrd0001:
    if (nrrdReadNrrdHeader(file, nrrd)) {
      sprintf(err, "%s: nrrdReadNrrdHeader failed", me);
      biffAdd(NRRD, err); return 1;
    }
    break;
  case nrrdMagicP1:
  case nrrdMagicP2:
  case nrrdMagicP3:
  case nrrdMagicP4:
  case nrrdMagicP5:
  case nrrdMagicP6:
    if (nrrdReadPNMHeader(file, nrrd, magic)) {
      sprintf(err, "%s: nrrdReadPNMHeader failed", me);
      biffAdd(NRRD, err); return 1;
    }
    break;
  }
  return 0;
}  
  
int
nrrdReadNrrdHeader(FILE *file, Nrrd *nrrd) {
  char line[NRRD_BIG_STRLEN], err[NRRD_MED_STRLEN], 
    me[] = "nrrdReadNrrdHeader";
  int d, len;

  do {
    if (!(len = _nrrdNonCmtLine(file, line, NRRD_BIG_STRLEN, nrrd))) {
      /* _nrrdNonCmtLine returned 0, meaning it hit EOF.  This is an
	 error in most cases, but in the case that there's a seperate
	 data file (nrrd->dataFile != NULL), we gracefully permit
	 the situation and pretend that we just hit the single newline
	 signalling the end of the header */
      if (nrrd->dataFile) {
	len = 1;
      }
      else {
	sprintf(err, "%s: hit EOF parsing header", me);
	biffAdd(NRRD, err); return 1;
      }
    }
    if (len > 1) {
      if (_nrrdParseLine(nrrd, line)) {
	sprintf(err, "%s: _nrrdParseLine(\"%s\") failed", me, line);
	biffAdd(NRRD, err); return 1;
      }
    }
  } while (len > 1);

  /* we've hit a newline-only line; header is done.  Is nrrd finished? */
  /* as a courtesy, compute num from the size array if it wasn't given,
     but only if we were told the dimension */
  if (nrrd->num < 1 && nrrd->dim > -1) {
    nrrd->num = 1;
    for (d=0; d<=nrrd->dim-1; d++) {
      nrrd->num *= nrrd->size[d];
    }
  }
  if (nrrdCheck(nrrd)) {
    sprintf(err, "%s: nrrdCheck() failed", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdEncodingUnknown == nrrd->encoding) {
    sprintf(err, "%s: Encoding method has not been set.", me);
    biffSet(NRRD, err); return 1;
  }
  return 0;
}

int
nrrdReadData(FILE *file, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdReadData", line[NRRD_BIG_STRLEN];
  NrrdReadDataType fptr;
  FILE *dataFile;
  int ret, i;
  
  if (!(file && nrrd && nrrd->num > 0)){
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  if (!(AIR_INSIDE(nrrdEncodingUnknown+1,
		   nrrd->encoding,
		   nrrdEncodingLast-1))) {
    sprintf(err, "%s: invalid encoding: %d", me, nrrd->encoding);
    biffSet(NRRD, err); return 1;
  }
  if (!(fptr = nrrdReadDataFptr[nrrd->encoding])) {
    sprintf(err, "%s: NULL function pointer for %s reader", 
	    me, nrrdEncoding2Str[nrrd->encoding]);
    biffSet(NRRD, err); return 1;
  }
  if (nrrd->dataFile) {
    /* we have a seperate data file, and we may need to skip
       some of the lines and/or bytes */
    dataFile = nrrd->dataFile;
    if (nrrd->lineSkip) {
      for (i=0; i<=nrrd->lineSkip-1; i++) {
	ret = nrrdOneLine(dataFile, line, NRRD_BIG_STRLEN);
	if (!(ret > 0 && ret <= NRRD_BIG_STRLEN)) {
	  sprintf(err, "%s: nrrdOneLine returned %d trying to skip line %d "
		  "(of %d) in seperate data file",
		  me, ret, i+1, nrrd->lineSkip);
	  biffSet(NRRD, err); return 1;
	}
      }
    }
    if (nrrd->byteSkip) {
      if (fseek(dataFile, nrrd->byteSkip, SEEK_CUR)) {
	sprintf(err, "%s: failed to fseek(%d, SEEK_CUR) on datafile",
		me, nrrd->byteSkip);
	biffSet(NRRD, err); return 1;
      }
    }
  }
  else {
    /* there is no seperate data file, we're going to read from the
       present (given) file */
    dataFile = file;
  }
  fprintf(stderr, 
	  "(%s: reading %s data ... ", me, nrrdEncoding2Str[nrrd->encoding]);
  fflush(stdout);
  if ((*fptr)(dataFile, nrrd)) {
    fprintf(stderr, "ERROR)\n");
    sprintf(err, "%s: data reader for %s encoding failed.", 
	    me, nrrdEncoding2Str[nrrd->encoding]);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrd->dataFile) {
    /* if there was a seperate data file, close it now */
    fclose(dataFile);
    nrrd->dataFile = NULL;
  }
  fprintf(stderr, "done)\n");

  /* now fix endianness if file endian doesn't match our current endian,
     but only if this was an encoding for which it mattered */
  if (nrrdEncodingEndianMatters[nrrd->encoding] 
      && nrrd->fileEndian != nrrdEndianUnknown
      && nrrd->fileEndian != nrrdMyEndian()) {
    fprintf(stderr, "(%s: fixing endianness ... ", me); fflush(stdout);
    nrrdSwapEndian(nrrd);
    fprintf(stderr, "done)\n");
  }

  return 0;
}

int
nrrdReadDataRaw(FILE *file, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdReadDataRaw";
  int num, size;
  
  if (!(file && nrrd && nrrd->num > 0)){
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  num = nrrd->num;
  size = nrrdTypeSize[nrrd->type];
  if (!nrrd->data) {
    if (nrrdAlloc(nrrd, nrrd->num, nrrd->type, nrrd->dim)) {
      /* admittedly weird to be calling this since we already have num, 
	 type, dim set-- just a way to get error reporting on calloc */
      sprintf(err, "%s: nrrdAlloc() failed", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  if (num != fread(nrrd->data, size, num, file)) {
    sprintf(err, "%s: unable to read %d objects of size %d", me, num, size);
    biffSet(NRRD, err); return 1;
  }
  return 0;
}

int
nrrdReadDataZlib(FILE *file, Nrrd *nrrd) {

  fprintf(stderr, "nrrdReadDataZlib: NOT IMPLEMENTED\n");
  return 0;
}

int
nrrdReadDataAscii(FILE *file, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdReadDataAscii", 
    numstr[NRRD_MED_STRLEN];
  NRRD_BIG_INT i;
  int type, tmp, size;
  char *data;
  unsigned char *udata;

  if (!(file && nrrd)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  type = nrrd->type;
  data = nrrd->data;
  if (!(AIR_INSIDE(nrrdTypeUnknown+1, type, nrrdTypeLast-1))) {
    sprintf(err, "%s: got bogus type %d", me, type);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == type) {
    sprintf(err, "%s: can't read into blocks from ascii", me);
    biffAdd(NRRD, err); return 1;
  }
  size = nrrdTypeSize[type];
  for (i=0; i<=nrrd->num-1; i++) {
    if (1 != fscanf(file, "%s", numstr)) {
      sprintf(err, 
	      "%s: couldn't get element "NRRD_BIG_INT_PRINTF"", me, i);
      biffAdd(NRRD, err); return 1;
    }
    if (type > nrrdTypeShort) {
      /* sscanf supports putting value directly into this type */
      if (1 != sscanf(numstr, nrrdType2Conv[type], (void*)data)) {
	sprintf(err, 
		"%s: couldn't parse element "NRRD_BIG_INT_PRINTF"", me, i);
	biffAdd(NRRD, err); return 1;
      }
    }
    else {
      /* type is nrrdTypeChar or nrrdTypeUChar: sscanf into int first */
      if (1 != sscanf(numstr, nrrdType2Conv[type], &tmp)) {
	sprintf(err, 
		"%s: couldn't parse element "NRRD_BIG_INT_PRINTF"", me, i);
	biffAdd(NRRD, err); return 1;
      }
      if (nrrdTypeChar == type) {
	*data = tmp;
      }
      if (nrrdTypeUChar == type) {
	udata = (unsigned char *)data;
	*udata = tmp;
      }
    }
    data += size;
  }
  return 0;
}

int
nrrdReadDataHex(FILE *file, Nrrd *nrrd) {

  fprintf(stderr, "nrrdReadDataHex: NOT IMPLEMENTED\n");
  return 0;
}

int
nrrdReadDataBase85(FILE *file, Nrrd *nrrd) {

  fprintf(stderr, "nrrdReadDataBase85: NOT IMPLEMENTED\n");
  return 0;
}

/*
** _nrrdSplitFilename()
**
** sees if the "name" string can be split into a directory part and a
** filename part.  If so, the directory part (including the directory
** delimiter ("/")) is copied into "dir", and the rest is copied into
** "base" (these strings are assumed to be allocated for sufficient
** length).  Otherwise, nothing is written into "dir" or "base".  
**
** return value is 1 if the split was possible, 0 if not.
*/
int
_nrrdSplitFilename(char *name, char *dir, char *base) {
  int i, j, len, numsep;
  
  numsep = strlen(_nrrdDirChars);
  len = strlen(name);
  for (i=len-1; i>=0; i--) {
    for (j=0; j<=numsep-1; j++) {
      if (name[i] == _nrrdDirChars[j])
	break;
    }
    if (j<=numsep-1)
      break;
  }
  /* we found a valid break if the last directory character
     is somewhere in the string except the last character */
  if (i>=0 && i<strlen(name)-1) {
    strcpy(dir, name);
    strcpy(base, dir+i+1);
    dir[i+1] = 0;
    return 1;
  }
  return 0;
}

int
nrrdLoad(char *name, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[]="nrrdLoad", *dir, *base;
  FILE *file;
  
  if (!(name && nrrd)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  /* this is just to allocate dir and base with some useful size */
  dir = strdup(name);
  base = strdup(name);
  if (_nrrdSplitFilename(name, dir, base)) {
    /* the file name starts with a directory; remember it */
    strcpy(nrrd->dir, dir);
  }
  else {
    /* the file name didn't start with a directory */
    strcpy(nrrd->dir, "");
  }
  if (!strcmp("-", name)) {
    file = stdin;
  }
  else {
    file = fopen(name, "rb");
  }
  if (!file) {
    sprintf(err, "%s: fopen(\"%s\",\"rb\") failed: %s", 
	    me, name, strerror(errno));
    biffSet(NRRD, err); return 1;
  }
  if (nrrdRead(file, nrrd)) {
    sprintf(err, "%s: nrrdRead() failed", me);
    biffAdd(NRRD, err); return 1;
  }
  if (file != stdin) 
    fclose(file);
  free(dir);
  free(base);
  return 0;
}

Nrrd *
nrrdNewLoad(char *name) {
  char err[NRRD_MED_STRLEN], me[]="nrrdNewLoad";
  Nrrd *nrrd;
  
  if (!name) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return NULL;
  }
  if (!(nrrd = nrrdNew())) {
    sprintf(err, "%s: nrrdNew() failed", me);
    biffAdd(NRRD, err); return NULL;
  }
  if (nrrdLoad(name, nrrd)) {
    sprintf(err, "%s: nrrdLoad() failed", me);
    nrrdNuke(nrrd);
    biffAdd(NRRD, err); return NULL;
  }
  return nrrd;
}

int 
nrrdSave(char *name, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[]="nrrdSave", *ext, *dir, *base, *rawfn;
  FILE *file;
  int (*writer)(FILE *, Nrrd *);
  
  if (!(name && nrrd)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (!strcmp("-", name)) {
    file = stdout;
  }
  else {
    file = fopen(name, "wb");
  }
  if (!file) {
    sprintf(err, "%s: fopen(\"%s\",\"wb\") failed: %s", 
	    me, name, strerror(errno));
    biffSet(NRRD, err); return 1;
  }
  writer = nrrdWrite;
  ext = strstr(name, _nrrdHdrExt);
  if (ext && ext == name + strlen(name) - strlen(_nrrdHdrExt)) {
    /* the given name ends with _nrrdHdrExt, so we play games */
    rawfn = strdup(name);
    strcpy(rawfn + strlen(rawfn) - strlen(_nrrdHdrExt), _nrrdRawExt);
    if (!(nrrd->dataFile = fopen(rawfn, "wb"))) {
      sprintf(err, "%s: fopen(\"%s\",\"wb\") failed: %s", me, rawfn,
	      strerror(errno));
      biffSet(NRRD, err); return 1;
    }
    dir = strdup(rawfn);
    base = strdup(rawfn);
    _nrrdSplitFilename(rawfn, dir, base);
    sprintf(nrrd->name, "%s%c%s", _nrrdRelDirFlag, _nrrdDirChars[0], base);
    free(rawfn);
    free(dir);
    free(base);
    goto write;
  }
  ext = strstr(name, _nrrdPGMExt);
  if (ext && ext == name + strlen(name) - strlen(_nrrdPGMExt)) {
    if (1 == nrrdValidPNM(nrrd, AIR_FALSE)) {
      writer = nrrdWritePNM;
      goto write;
    }
    fprintf(stderr, "(%s: type or shape can't be PGM, saving as NRRD)\n", me);
  }
  ext = strstr(name, _nrrdPPMExt);
  if (ext && ext == name + strlen(name) - strlen(_nrrdPPMExt)) {
    if (2 == nrrdValidPNM(nrrd, AIR_FALSE)) {
      writer = nrrdWritePNM;
      goto write;
    }
    fprintf(stderr, "(%s: type or shape can't be PPM, saving as NRRD)\n", me);
  }
 write:
  if (writer(file, nrrd)) {
    sprintf(err, "%s: write failed", me);
    biffAdd(NRRD, err); return 1;
  }
  if (file != stdout)
    fclose(file);
  return 0;
}

int
nrrdRead(FILE *file, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdRead";

  if (nrrdReadHeader(file, nrrd)) {
    sprintf(err, "%s: nrrdReadHeader() failed", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!nrrd->data) {
    if (nrrdAlloc(nrrd, nrrd->num, nrrd->type, nrrd->dim)) {
      sprintf(err, "%s: nrrdAlloc() failed", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  if (nrrdReadData(file, nrrd)) {
    sprintf(err, "%s: nrrdReadData() failed", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

Nrrd *
nrrdNewRead(FILE *file) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdNewRead";
  Nrrd *nrrd;

  if (!file) {
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return NULL;
  }
  if (!(nrrd = nrrdNew())) {
    sprintf(err, "%s: nrrdNew() failed", me);
    biffAdd(NRRD, err); return NULL;
  }
  if (nrrdRead(file, nrrd)) {
    sprintf(err, "%s: nrrdRead() failed", me);
    nrrdNuke(nrrd);
    biffAdd(NRRD, err); return NULL;
  }
  return nrrd;
}

int
nrrdWrite(FILE *file, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdWrite";

  if (!(file && nrrd && nrrd->data)){
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  /* this is for the sake of laziness, but it may be a bad idea */
  if (nrrdEncodingUnknown == nrrd->encoding) {
    nrrd->encoding = nrrdEncodingRaw;
  }
  if (nrrdWriteHeader(file, nrrd)) {
    sprintf(err, "%s: nrrdWriteHeader() failed", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdWriteData(file, nrrd)) {
    sprintf(err, "%s: nrrdWriteData() failed", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

int
nrrdWriteHeader(FILE *file, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdWriteHeader", *cmt;
  int i, doit;

  if (!(file && nrrd)){
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdCheck(nrrd)) {
    sprintf(err, "%s: nrrdCheck() failed", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(AIR_INSIDE(nrrdEncodingUnknown+1,
		   nrrd->encoding,
		   nrrdEncodingLast-1))) {
    sprintf(err, "%s: invalid encoding: %d", me, nrrd->encoding);
    biffSet(NRRD, err); return 1;
  }
  fprintf(file, "%s\n", NRRD_HEADER);
  if (strlen(nrrd->content))
    fprintf(file, "content: %s\n", nrrd->content);
  fprintf(file, "number: " NRRD_BIG_INT_PRINTF "\n", nrrd->num);
  fprintf(file, "type: %s\n", nrrdType2Str[nrrd->type]);
  fprintf(file, "dimension: %d\n", nrrd->dim);
  if (AIR_EXISTS(nrrd->oldMin))
    fprintf(file, "old min: %lf\n", nrrd->oldMin);
  if (AIR_EXISTS(nrrd->oldMax))
    fprintf(file, "old max: %lf\n", nrrd->oldMax);
  fprintf(file, "encoding: %s\n", nrrdEncoding2Str[nrrd->encoding]);
  /* if chosen encoding exposes endianness, then we need to
     record endianness in the header */
  nrrd->fileEndian = nrrdMyEndian();
  printf("%s: element size = %d\n", me, nrrdElementSize(nrrd));
  if (nrrdEncodingEndianMatters[nrrd->encoding]
      && 1 != nrrdElementSize(nrrd)) {
    fprintf(file, "endian: %s\n", nrrdEndian2Str[nrrd->fileEndian]);
  }
  if (nrrdEncodingUser == nrrd->encoding)
    fprintf(file, "blocksize: %d\n", nrrd->blockSize);
  fprintf(file, "sizes:");
  for (i=0; i<=nrrd->dim-1; i++)
    fprintf(file, " %d", nrrd->size[i]);
  fprintf(file, "\n");
  doit = 0;
  for (i=0; i<=nrrd->dim-1; i++)
    doit |= AIR_EXISTS(nrrd->spacing[i]);
  if (doit) {
    fprintf(file, "spacings:");
    for (i=0; i<=nrrd->dim-1; i++)
      fprintf(file, " %lf", nrrd->spacing[i]);
    fprintf(file, "\n");
  }
  doit = 0;
  for (i=0; i<=nrrd->dim-1; i++)
    doit |= AIR_EXISTS(nrrd->axisMin[i]);
  if (doit) {
    fprintf(file, "axis mins:");
    for (i=0; i<=nrrd->dim-1; i++)
      fprintf(file, " %lf", nrrd->axisMin[i]);
    fprintf(file, "\n");
  }
  doit = 0;
  for (i=0; i<=nrrd->dim-1; i++)
    doit |= AIR_EXISTS(nrrd->axisMax[i]);
  if (doit) {
    fprintf(file, "axis maxs:");
    for (i=0; i<=nrrd->dim-1; i++)
      fprintf(file, " %lf", nrrd->axisMax[i]);
    fprintf(file, "\n");
  }
  doit = 0;
  for (i=0; i<=nrrd->dim-1; i++)
    doit |= strlen(nrrd->label[i]);
  if (doit) {
    fprintf(file, "labels:");
    for (i=0; i<=nrrd->dim-1; i++)
      fprintf(file, " \"%s\"", nrrd->label[i]);
    fprintf(file, "\n");
  }
  if (nrrd->comment) {
    i = 0;
    while (cmt = nrrd->comment[i]) {
      fprintf(file, "#%s\n", cmt);
      i++;
    }
  }
  if (nrrd->dataFile) {
    fprintf(file, "data file: %s\n", nrrd->name);
  }
  else {
    fprintf(file, "\n");
  }
  return 0;
}

int
nrrdWriteData(FILE *file, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdWriteData";
  NrrdWriteDataType fptr;
  FILE *dataFile;
  
  if (!(file && nrrd && nrrd->num > 0)){
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  if (!(AIR_INSIDE(nrrdEncodingUnknown+1,
		   nrrd->encoding,
		   nrrdEncodingLast-1))) {
    sprintf(err, "%s: invalid encoding: %d", me, nrrd->encoding);
    biffSet(NRRD, err); return 1;
  }
  if (!(fptr = nrrdWriteDataFptr[nrrd->encoding])) {
    sprintf(err, "%s: NULL function pointer for %s writer", 
	    me, nrrdEncoding2Str[nrrd->encoding]);
    biffSet(NRRD, err); return 1;
  }
  dataFile = nrrd->dataFile ? nrrd->dataFile : file;
  if ((*fptr)(dataFile, nrrd)) {
    sprintf(err, "%s: data writer for %s encoding failed.", 
	    me, nrrdEncoding2Str[nrrd->encoding]);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrd->dataFile) {
    fclose(nrrd->dataFile);
    nrrd->dataFile = NULL;
  }
  return 0;
}

int
nrrdWriteDataRaw(FILE *file, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdWriteDataRaw";
  int num, size;
  
  if (!(file && nrrd && nrrd->data && nrrd->num > 0)){
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  num = nrrd->num;
  size = nrrdTypeSize[nrrd->type];
  if (num != fwrite(nrrd->data, size, num, file)) {
    sprintf(err, 
	    "%s: unable to write %d objects of size %d (data,file=%lu,%lu)", 
	    me, num, size, (unsigned long)(nrrd->data), (unsigned long)(file));
    biffSet(NRRD, err); 
    if (ferror(file)) {
      sprintf(err, "%s: ferror returned non-zero", me);
      biffAdd(NRRD, err);
    }
    return 1;
  }
  return 0;
}

int
nrrdWriteDataZlib(FILE *file, Nrrd *nrrd) {

  fprintf(stderr, "nrrdWriteDataZlib: NOT IMPLEMENTED\n");
  return 0;
}

int
nrrdWriteDataAscii(FILE *file, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdWriteDataAscii";
  NRRD_BIG_INT i;
  int type, size;
  char *data;
  
  if (!(file && nrrd)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  type = nrrd->type;
  data = nrrd->data;
  if (!(AIR_INSIDE(nrrdTypeUnknown+1, type, nrrdTypeLast-1))) {
    sprintf(err, "%s: got bogus type %d", me, type);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == type) {
    sprintf(err, "%s: can't write blocks to ascii", me);
    biffAdd(NRRD, err); return 1;
  }
  size = nrrdTypeSize[type];
  for (i=0; i<=nrrd->num-1; i++) {
    nrrdFprint[type](file, data);
    if ( (nrrd->dim == 1)
	 ||
	 (nrrd->dim == 2 &&
	  nrrd->size[0] <= 6 && 
	  !((i+1)%(nrrd->size[0])))
	 ||
	 (nrrd->dim != 2 && !((i+1)%6))
	 ) {
      fprintf(file, "\n");
    }
    else {
      fprintf(file, " ");
    }
    data += size;
  }  
  return 0;
}

int
nrrdWriteDataHex(FILE *file, Nrrd *nrrd) {

  fprintf(stderr, "nrrdWriteDataHex: NOT IMPLEMENTED\n");
  return 0;
}

int
nrrdWriteDataBase85(FILE *file, Nrrd *nrrd) {

  fprintf(stderr, "nrrdWriteDataBase85: NOT IMPLEMENTED\n");
  return 0;
}

int
nrrdReadPNMHeader(FILE *file, Nrrd *nrrd, int magic) {
  char err[NRRD_MED_STRLEN], line[NRRD_BIG_STRLEN], 
    me[] = "nrrdReadPNMHeader";
  int ascii, color, bitmap, 
    size, sx, sy, max, *num[5], want, got, dumb;

  if (!(file && nrrd)) {
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  size = NRRD_BIG_STRLEN;
  switch(magic) {
  case nrrdMagicP6:
    color = 1; ascii = 0; bitmap = 0;
    break;
  case nrrdMagicP5:
    color = 0; ascii = 0; bitmap = 0;
    break;
  case nrrdMagicP4:
    color = 0; ascii = 0; bitmap = 1;
    break;
  case nrrdMagicP3:
    color = 1; ascii = 1; bitmap = 0;
    break;
  case nrrdMagicP2:
    color = 0; ascii = 1; bitmap = 0;
    break;
  case nrrdMagicP1:
    color = 0; ascii = 1; bitmap = 1;
    break;
  default:
    sprintf(err, "%s: sorry, (%d) unsupported PNM format file", me, magic);
    biffSet(NRRD, err); return 1;
  }
  sx = sy = max = 0;
  num[0] = &sx;
  num[1] = &sy;
  num[2] = &max;
  num[3] = num[4] = &dumb;
  got = 0;
  want = bitmap ? 2 : 3;
  while (got < want) {
    /* eventually, at worst, this will go to the end of the file */
    if (!(0 < nrrdOneLine(file, line, size))) {
      sprintf(err, "%s: line read failed", me);
      biffSet(NRRD, err); return 1;
    }
    /* printf("%s: got line: |%s|\n", me, line); */
    if ('#' == line[0]) {
      nrrdAddComment(nrrd, line+1);
      continue;
    }
    if (3 == sscanf(line, "%d%d%d", num[got], num[got+1], num[got+2])) {
      /* printf("%s: got 3\n", me); */
      got += 3;
      continue;
    }
    if (2 == sscanf(line, "%d%d", num[got], num[got+1])) {
      /* printf("%s: got 2\n", me); */
      got += 2;
      continue;
    }
    if (1 == sscanf(line, "%d", num[got])) {
      /* printf("%s: got 1\n", me); */
      got += 1;
      continue;
    }
  }
  sx = AIR_MAX(0, sx);
  sy = AIR_MAX(0, sy);
  max = AIR_MAX(0, max);
  /* printf("%s: image is %dx%d, maxval=%d\n", me, sx, sy, max); */
  nrrd->num = (color ? 3 : 1)*sx*sy;
  /* we do not support binary bit arrays; a binary PGM will get
     put into an arrays of uchars */
  nrrd->type = ascii && max > 255 ? nrrdTypeUInt : nrrdTypeUChar;
  nrrd->dim = color ? 3 : 2;
  if (color) {
    nrrd->size[0] = 3;
    nrrd->size[1] = sx;
    nrrd->size[2] = sy;
  }
  else {
    nrrd->size[0] = sx;
    nrrd->size[1] = sy;
  }
  nrrd->encoding = (ascii 
		    ? nrrdEncodingAscii
		    : nrrdEncodingRaw);
  /* printf("%s: ascii = %d, encoding = %d\n", me, ascii, nrrd->encoding); */
  return 0;
}

/*
******** nrrdValidPNM
**
** odd little function for telling if a given nrrd could be a pgm or ppm
** returns 0: if couldn't be a pgm or ppm
**         1: if it looks like a pgm (2-D 8-bit grayscale)
**         2: if it looks like a ppm (3-D 8-bit color, rgb interleaved)
**
** HEY!! should probably allow something larger that 8-bit values
**     if the user wants ASCII encoding; PNM does support that
**
** whether or not this function uses biff is determined by the useBiff arg
*/
int
nrrdValidPNM(Nrrd *pnm, int useBiff) {
  char me[]="nrrdValidPPM", err[NRRD_MED_STRLEN];
  int color;

  if (!pnm) {
    if (useBiff) {
      sprintf(err, "%s: got NULL pointer", me);
      biffSet(NRRD, err);
    }
    return 0;
  }
  if (nrrdTypeUChar != pnm->type) {
    if (useBiff) {
      sprintf(err, "%s: isn't of type unsigned char", me);
      biffSet(NRRD, err);
    }
    return 0;
  }
  if (!( 2 == pnm->dim || 3 == pnm->dim )) {
    if (useBiff) {
      sprintf(err, "%s: dimension is %d, not 2 or 3", me, pnm->dim);
      biffSet(NRRD, err);
    }
    return 0;
  }
  color = (3 == pnm->dim);
  if (color && 3 != pnm->size[0]) {
    if (useBiff) {
      sprintf(err, "%s: is 3-D, but first axis size is %d, not 3", 
	      me, pnm->size[0]);
      biffSet(NRRD, err);
    }
    return 0;
  }
  return color ? 2 : 1;
}

int
nrrdWritePNM(FILE *file, Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdWritePNM", *cmt;
  int i, ascii, color, imgtype;
  
  if (!(file && nrrd)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  imgtype = nrrdValidPNM(nrrd, AIR_TRUE);
  if (!imgtype) {
    sprintf(err, "%s: didn't get a PNM", me);
    biffAdd(NRRD, err); return 1;
  }
  /* this is for the sake of laziness, but it may be a bad idea */
  if (nrrdEncodingUnknown == nrrd->encoding) {
    nrrd->encoding = nrrdEncodingRaw;
  }
  color = imgtype == 2;
  ascii = nrrdEncodingAscii == nrrd->encoding;
  if (!( ascii || nrrdEncodingRaw == nrrd->encoding )) {
    sprintf(err, "%s: PNM only supports %s or %s encoding", me,
	    nrrdEncoding2Str[nrrdEncodingRaw], 
	    nrrdEncoding2Str[nrrdEncodingAscii]);
    biffSet(NRRD, err); return 1;
  }
  fprintf(file, "P%c\n", (color 
			  ? (ascii ? '3' : '6')
			  : (ascii ? '2' : '5')));
  if (nrrd->comment) {
    i = 0;
    while (cmt = nrrd->comment[i]) {
      fprintf(file, "#%s\n", cmt);
      i++;
    }
  }
  fprintf(file, "%d %d\n255\n", 
	  color ? nrrd->size[1] : nrrd->size[0], 
	  color ? nrrd->size[2] : nrrd->size[1]);
  if (nrrdWriteData(file, nrrd)) {
    sprintf(err, "%s: nrrdWriteData() failed", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

NrrdReadDataType 
nrrdReadDataFptr[] = {
  NULL,
  nrrdReadDataRaw,
  nrrdReadDataZlib,
  nrrdReadDataAscii,
  nrrdReadDataHex,
  nrrdReadDataBase85,
  NULL
};

NrrdWriteDataType 
nrrdWriteDataFptr[] = {
  NULL,
  nrrdWriteDataRaw,
  nrrdWriteDataZlib,
  nrrdWriteDataAscii,
  nrrdWriteDataHex,
  nrrdWriteDataBase85,
  NULL
};
