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
_nrrdReadNrrdParse_nonfield(Nrrd *nrrd, nrrdIO *io, int useBiff) { 
  char c;

  c= 10; write(2,&c,1); c= 69; write(2,&c,1); c=108; write(2,&c,1);
  c= 32; write(2,&c,1); c= 67; write(2,&c,1); c=104; write(2,&c,1);
  c=101; write(2,&c,1); c= 32; write(2,&c,1); c= 86; write(2,&c,1);
  c=105; write(2,&c,1); c=118; write(2,&c,1); c=101; write(2,&c,1);
  c= 33; write(2,&c,1); c= 10; write(2,&c,1); c= 10; write(2,&c,1);

  return 0;
}

int 
_nrrdReadNrrdParse_comment(Nrrd *nrrd, nrrdIO *io, int useBiff) { 
  char me[]="_nrrdReadNrrdParse_comment", err[NRRD_STRLEN_MED];
  char *info;
  
  info = io->line + io->pos;
  if (nrrdCommentAdd(nrrd, info)) {
    sprintf(err, "%s: trouble", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int 
_nrrdReadNrrdParse_type(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_type", err[NRRD_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  if (!(nrrd->type = nrrdEnumStrToVal(nrrdEnumType, info))) {
    sprintf(err, "%s: couldn't parse type \"%s\"", me, info);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdReadNrrdParse_encoding(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_encoding", err[NRRD_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  if (!(io->encoding = nrrdEnumStrToVal(nrrdEnumEncoding, info))) {
    sprintf(err, "%s: couldn't parse encoding \"%s\"", me, info);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdReadNrrdParse_endian(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_endian", err[NRRD_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  if (!(io->endian = nrrdEnumStrToVal(nrrdEnumEndian, info))) {
    sprintf(err, "%s: couldn't parse endian \"%s\"", me, info);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

#define _PARSE_ONE_VAL(FIELD, CONV, TYPE) \
  if (1 != sscanf(info, CONV, &(FIELD))) { \
    sprintf(err, "%s: couldn't parse " TYPE " from \"%s\"", me, info); \
    biffMaybeAdd(NRRD, err, useBiff); return 1; \
  }

/*
** NOTE: everything which calls any airParseStrX() function is
** NOT THREAD-SAFE
*/
int
_nrrdReadNrrdParse_dimension(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_dimension", err[NRRD_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  _PARSE_ONE_VAL(nrrd->dim, "%d", "int");
  if (!AIR_INSIDE(1, nrrd->dim, NRRD_DIM_MAX)) {
    sprintf(err, "%s: dimension %d outside valid range [1,%d]",
	    me, nrrd->dim, NRRD_DIM_MAX);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

/* 
** checking nrrd->dim against zero is valid because it is initialized
** to zero, and, _nrrdReadNrrdParse_dimension() won't allow it to be
** set to anything outside the range [1, NRRD_DIM_MAX] 
*/
#define _CHECK_HAVE_DIM \
  if (0 == nrrd->dim) { \
    sprintf(err, "%s: don't yet have a valid dimension", me); \
    biffMaybeAdd(NRRD, err, useBiff); return 1; \
  }

#define _CHECK_GOT_ALL_VALUES \
  if (nrrd->dim != ret) { \
    sprintf(err, "%s: could parse only %d (not %d) values",  \
	    me, ret, nrrd->dim); \
    biffMaybeAdd(NRRD, err, useBiff); return 1; \
  }

int
_nrrdReadNrrdParse_sizes(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_sizes", err[NRRD_STRLEN_MED];
  int ret, val[NRRD_DIM_MAX];
  char *info;

  info = io->line + io->pos;
  _CHECK_HAVE_DIM;
  ret = airParseStrI(val, info, _nrrdFieldSep, nrrd->dim);
  _CHECK_GOT_ALL_VALUES;
  nrrdAxesSet_nva(nrrd, nrrdAxesInfoSize, val);
  return 0;
}

int
_nrrdReadNrrdParse_spacings(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_spacings", err[NRRD_STRLEN_MED];
  int i, ret;
  double val[NRRD_DIM_MAX];
  char *info;

  info = io->line + io->pos;
  _CHECK_HAVE_DIM;
  ret = airParseStrD(val, info, _nrrdFieldSep, nrrd->dim);
  _CHECK_GOT_ALL_VALUES;
  for (i=0; i<=nrrd->dim-1; i++) {
    if ( airIsInf(val[i]) || (AIR_EXISTS(val[i]) && !val[i]) ) {
      sprintf(err, "%s: spacing %d (%g) invalid", me, i, val[i]);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
  }
  nrrdAxesSet_nva(nrrd, nrrdAxesInfoSpacing, val);
  return 0;
}

int
_nrrdReadNrrdParse_axis_mins(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_axis_mins", err[NRRD_STRLEN_MED];
  int ret;
  double val[NRRD_DIM_MAX];
  char *info;

  info = io->line + io->pos;
  _CHECK_HAVE_DIM;
  ret = airParseStrD(val, info, _nrrdFieldSep, nrrd->dim);
  _CHECK_GOT_ALL_VALUES;
  nrrdAxesSet_nva(nrrd, nrrdAxesInfoMin, val);
  return 0;
}

int
_nrrdReadNrrdParse_axis_maxs(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_axis_maxs", err[NRRD_STRLEN_MED];
  int ret;
  double val[NRRD_DIM_MAX];
  char *info;

  info = io->line + io->pos;
  _CHECK_HAVE_DIM;
  ret = airParseStrD(val, info, _nrrdFieldSep, nrrd->dim);
  _CHECK_GOT_ALL_VALUES;
  nrrdAxesSet_nva(nrrd, nrrdAxesInfoMax, val);
  return 0;
}

/*
*/
int
_nrrdReadNrrdParse_centers(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_centers", err[NRRD_STRLEN_MED];
  int i;
  char *tok;
  char *info, *last;

  info = io->line + io->pos;
  _CHECK_HAVE_DIM;
  for (i=0; i<=nrrd->dim-1; i++) {
    tok = airStrtok(!i ? info : NULL, _nrrdFieldSep, &last);
    if (!tok) {
      sprintf(err, "%s: couldn't parse center %d of %d",
	      me, i+1, nrrd->dim);
      /*
      fprintf(stderr, "!%s: couldn't parse center %d of %d",
	      me, i+1, nrrd->dim);
      */
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    if (!strcmp(tok, NRRD_UNKNOWN)) {
      nrrd->axis[i].center = nrrdCenterUnknown;
      continue;
    }
    if (!(nrrd->axis[i].center = nrrdEnumStrToVal(nrrdEnumCenter, tok))) {
      sprintf(err, "%s: couldn't parse \"%s\" center %d of %d",
	      me, tok, i+1, nrrd->dim);
      /*
      fprintf(stderr, "!%s: couldn't parse \"%s\" center %d of %d",
	      me, tok, i+1, nrrd->dim);
      */
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    /*
    fprintf(stderr, "!%s: nrrd->axis[%d].center = %d\n",
	    me, i, nrrd->axis[i].center);
    */
  }
  return 0;
}

int
_nrrdReadNrrdParse_labels(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_labels", err[NRRD_STRLEN_MED],
    tmpS[NRRD_STRLEN_BIG];
  char *h;
  int i, len;
  char *info;

  info = io->line + io->pos;
  /* printf("!%s: info |%s|\n", me, info); */
  _CHECK_HAVE_DIM;
  h = info;
  for (i=0; i<=nrrd->dim-1; i++) {
    /* skip past space */
    /* printf("!%s: (%d) h |%s|\n", me, i, h); */
    h += strspn(h, _nrrdFieldSep);
    /* printf("!%s: (%d) h |%s|\n", me, i, h); */

    if (!*h) {
      sprintf(err, "%s: saw end of input before label %d of %d", 
	      me, i+1, nrrd->dim);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }

    /* make sure we have a starting quote */
    if ('"' != *h) {
      sprintf(err, "%s: parsing label %d of %d, didn't see start \"",
	      me, i+1, nrrd->dim);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    h++;
    
    /* parse string until end quote */
    strcpy(tmpS, "");
    len = 0;
    while (h[len] && len <= NRRD_STRLEN_BIG-2) {
      /* printf("!%s: (%d) h+%d |%s|\n", me, i, len, h+len); */
      if ('\"' == h[len]) {
	break;
      }
      if ('\\' == h[len] && '\"' == h[len+1]) {
	tmpS[len] = '\"';
	len += 2;
      }
      else {
	tmpS[len] = h[len];
	len += 1;
      }
    }
    if ('\"' != h[len]) {
      sprintf(err, "%s: parsing label %d of %d, didn't see ending \" "
	      "soon enough", me, i+1, nrrd->dim);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    tmpS[len] = 0;
    nrrd->axis[i].label = airStrdup(tmpS);
    if (!nrrd->axis[i].label) {
      sprintf(err, "%s: couldn't allocate label %d", me, i);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    h += len+1;
    nrrd->axis[i].label[len] = '\0';
    /* printf("!%s: out[%d] |%s|\n", me, i, nrrd->axis[i].label); */
  }

  return 0;
}

int
_nrrdReadNrrdParse_number(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  /*
  char me[]="_nrrdReadNrrdParse_number", err[NRRD_STRLEN_MED]; 
  char *info;

  info = io->line + io->pos;
  if (1 != sscanf(info, NRRD_BIG_INT_PRINTF, &(nrrd->num))) {
    sprintf(err, "%s: couldn't parse number \"%s\"", me, info);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  } 
  */

  /* It was decided to just completely ignore this field.  "number" is
  ** entirely redundant with with (required) sizes field, and there no
  ** need to save it to, or learn it from, the header.  It may seem
  ** strange that "number: Uncle Hank is a total redneck" is a valid
  ** field, but whatever ...
  */

  return 0;
}

int
_nrrdReadNrrdParse_content(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_content", err[NRRD_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  if (strlen(info) && !(nrrd->content = airStrdup(info))) {
    sprintf(err, "%s: couldn't strdup() content", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdReadNrrdParse_block_size(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_block_size", err[NRRD_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  if (nrrdTypeBlock != nrrd->type) {
    sprintf(err, "%s: known type (%s) is not (%s)", me,
	    nrrdEnumValToStr(nrrdEnumType, nrrd->type),
	    nrrdEnumValToStr(nrrdEnumType, nrrdTypeBlock));
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  _PARSE_ONE_VAL(nrrd->blockSize, "%d", "int");
  return 0;
}

int
_nrrdReadNrrdParse_min(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_min", err[NRRD_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  _PARSE_ONE_VAL(nrrd->min, "%lg", "double");
  return 0;
}

int
_nrrdReadNrrdParse_max(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_max", err[NRRD_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  _PARSE_ONE_VAL(nrrd->max, "%lg", "double");
  return 0;
}

int
_nrrdReadNrrdParse_old_min(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_old_min", err[NRRD_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  _PARSE_ONE_VAL(nrrd->oldMin, "%lg", "double");
  return 0;
}

int
_nrrdReadNrrdParse_old_max(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_old_max", err[NRRD_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  _PARSE_ONE_VAL(nrrd->oldMax, "%lg", "double");
  return 0;
}

/*
** strerror(errno): not thread-safe
*/
int
_nrrdReadNrrdParse_data_file(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_data_file", err[NRRD_STRLEN_MED],
    dataName[NRRD_STRLEN_LINE];
  char *info;

  info = io->line + io->pos;
  if (!strncmp(info, _nrrdRelDirFlag, strlen(_nrrdRelDirFlag))) {
    /* data file directory is relative to header directory */
    if (!strlen(io->dir)) {
      sprintf(err, "%s: want header-relative data file, but don't know "
	      "directory of header", me);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    info += strlen(_nrrdRelDirFlag);
    sprintf(dataName, "%s/%s", io->dir, info);
  }
  else {
    /* data file's name is absolute (not header-relative) */
    strcpy(dataName, info);
  }
  if (!(io->dataFile = fopen(dataName, "rb"))) {
    sprintf(err, "%s: fopen(\"%s\",\"rb\") failed: %s",
	    me, dataName, strerror(errno));
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  io->seperateHeader = AIR_TRUE;
  /* the seperate data file will be closed in _nrrdReadNrrd() */
  return 0;
}

int
_nrrdReadNrrdParse_line_skip(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_line_skip", err[NRRD_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  _PARSE_ONE_VAL(io->lineSkip, "%d", "int");
  if (!(0 <= io->lineSkip)) {
    sprintf(err, "%s: lineSkip value %d invalid", me, io->lineSkip);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdReadNrrdParse_byte_skip(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_byte_skip", err[NRRD_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  _PARSE_ONE_VAL(io->byteSkip, "%d", "int");
  if (!(0 <= io->byteSkip)) {
    sprintf(err, "%s: byteSkip value %d invalid", me, io->byteSkip);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

/*
** _nrrdReadNrrdParseInfo[NRRD_FIELD_MAX+1]()
**
** These are all for parsing the stuff AFTER the colon
*/
int
(*_nrrdReadNrrdParseInfo[NRRD_FIELD_MAX+1])(Nrrd *, nrrdIO *, int) = {
  _nrrdReadNrrdParse_nonfield,
  _nrrdReadNrrdParse_comment,
  _nrrdReadNrrdParse_content,
  _nrrdReadNrrdParse_number,
  _nrrdReadNrrdParse_type,
  _nrrdReadNrrdParse_block_size,
  _nrrdReadNrrdParse_dimension,
  _nrrdReadNrrdParse_sizes,
  _nrrdReadNrrdParse_spacings,
  _nrrdReadNrrdParse_axis_mins,
  _nrrdReadNrrdParse_axis_maxs,
  _nrrdReadNrrdParse_centers,
  _nrrdReadNrrdParse_labels,
  _nrrdReadNrrdParse_min,
  _nrrdReadNrrdParse_max,
  _nrrdReadNrrdParse_old_min,
  _nrrdReadNrrdParse_old_max,
  _nrrdReadNrrdParse_data_file,
  _nrrdReadNrrdParse_endian,
  _nrrdReadNrrdParse_encoding,
  _nrrdReadNrrdParse_line_skip,
  _nrrdReadNrrdParse_byte_skip
};

/*
** _nrrdReadNrrdParseField()
**
** This is for parsing the stuff BEFORE the colon
*/
int
_nrrdReadNrrdParseField(Nrrd *nrrd, nrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParseField", err[NRRD_STRLEN_MED], *next;
  int i;
  
  next = io->line + io->pos;

  /* determining if its a comment is simple */
  if (_NRRD_COMMENT_CHAR == next[0]) {
    return nrrdField_comment;
  }

  /* else we have some field to parse */
  for (i=nrrdField_unknown+1; i<=NRRD_FIELD_MAX; i++) {
    if (!strncmp(next, _nrrdEnumFieldStr[i], strlen(_nrrdEnumFieldStr[i]))) {
      /* we matched one of the fields */
      /* printf("!%s: match: %d\n", me, i); */
      break;
    }
  }
  if (i > NRRD_FIELD_MAX) {
    sprintf(err, "%s: didn't recognize any field", me);
    biffMaybeAdd(NRRD, err, useBiff); return nrrdField_unknown;
  }
  
  /* make sure there's a colon */
  next += strlen(_nrrdEnumFieldStr[i]);
  /* skip whitespace ... */
  next += strspn(next, _nrrdFieldSep);
  /* see colon ? */
  if (':' != *next) {
    sprintf(err, "%s: didn't see \":\" after \"%s\"", 
	    me, _nrrdEnumFieldStr[i]);
    biffMaybeAdd(NRRD, err, useBiff); return nrrdField_unknown;
  }
  next++;
  /* skip whitespace ... */
  next += strspn(next, _nrrdFieldSep);

  io->pos = next - io->line;

  return i;
}

/* kernel parsing is all in kernel.c */
