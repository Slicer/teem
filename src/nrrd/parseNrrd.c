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

int 
_nrrdReadNrrdParse_nonfield(Nrrd *nrrd, NrrdIO *io, int useBiff) { 
  /*
  char c;

  c= 10; write(2,&c,1); c= 69; write(2,&c,1); c=108; write(2,&c,1);
  c= 32; write(2,&c,1); c= 67; write(2,&c,1); c=104; write(2,&c,1);
  c=101; write(2,&c,1); c= 32; write(2,&c,1); c= 86; write(2,&c,1);
  c=105; write(2,&c,1); c=118; write(2,&c,1); c=101; write(2,&c,1);
  c= 33; write(2,&c,1); c= 10; write(2,&c,1); c= 10; write(2,&c,1);
  */
  return 0;
}

int 
_nrrdReadNrrdParse_comment(Nrrd *nrrd, NrrdIO *io, int useBiff) { 
  char me[]="_nrrdReadNrrdParse_comment", err[AIR_STRLEN_MED];
  char *info;
  
  info = io->line + io->pos;
  /* this skips the '#' at io->line[io->pos] and any other ' ' and '#' */
  if (nrrdCommentAdd(nrrd, info)) {
    sprintf(err, "%s: trouble adding comment", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int 
_nrrdReadNrrdParse_type(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_type", err[AIR_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  if (!(nrrd->type = airEnumVal(nrrdType, info))) {
    sprintf(err, "%s: couldn't parse type \"%s\"", me, info);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdReadNrrdParse_encoding(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_encoding", err[AIR_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  if (!(io->encoding = airEnumVal(nrrdEncoding, info))) {
    sprintf(err, "%s: couldn't parse encoding \"%s\"", me, info);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdReadNrrdParse_endian(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_endian", err[AIR_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  if (!(io->endian = airEnumVal(airEndian, info))) {
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

int
_nrrdReadNrrdParse_dimension(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_dimension", err[AIR_STRLEN_MED];
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
    sprintf(err, "%s: parsed %d values, but dimension is %d",  \
	    me, ret, nrrd->dim); \
    biffMaybeAdd(NRRD, err, useBiff); return 1; \
  }

int
_nrrdReadNrrdParse_sizes(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_sizes", err[AIR_STRLEN_MED];
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
_nrrdReadNrrdParse_spacings(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_spacings", err[AIR_STRLEN_MED];
  int i, ret;
  double val[NRRD_DIM_MAX];
  char *info;

  info = io->line + io->pos;
  _CHECK_HAVE_DIM;
  ret = airParseStrD(val, info, _nrrdFieldSep, nrrd->dim);
  _CHECK_GOT_ALL_VALUES;
  for (i=0; i<=nrrd->dim-1; i++) {
    if (!( !airIsInf_d(val[i]) && (airIsNaN(val[i]) || (0 != val[i])) )) {
      sprintf(err, "%s: spacing %d (%g) invalid", me, i, val[i]);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
  }
  nrrdAxesSet_nva(nrrd, nrrdAxesInfoSpacing, val);
  return 0;
}

int
_nrrdReadNrrdParse_axis_mins(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_axis_mins", err[AIR_STRLEN_MED];
  int ret, i, sgn;
  double val[NRRD_DIM_MAX];
  char *info;

  info = io->line + io->pos;
  _CHECK_HAVE_DIM;
  ret = airParseStrD(val, info, _nrrdFieldSep, nrrd->dim);
  _CHECK_GOT_ALL_VALUES;
  for (i=0; i<=nrrd->dim-1; i++) {
    if ((sgn=airIsInf_d(val[i]))) {
      sprintf(err, "%s: axis min %d %sinf invalid", me, i, 1==sgn ? "+" : "-");
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
  }
  nrrdAxesSet_nva(nrrd, nrrdAxesInfoMin, val);
  return 0;
}

int
_nrrdReadNrrdParse_axis_maxs(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_axis_maxs", err[AIR_STRLEN_MED];
  int ret, i, sgn;
  double val[NRRD_DIM_MAX];
  char *info;

  info = io->line + io->pos;
  _CHECK_HAVE_DIM;
  ret = airParseStrD(val, info, _nrrdFieldSep, nrrd->dim);
  _CHECK_GOT_ALL_VALUES;
  for (i=0; i<=nrrd->dim-1; i++) {
    if ((sgn=airIsInf_d(val[i]))) {
      sprintf(err, "%s: axis max %d %sinf invalid", me, i, 1==sgn ? "+" : "-");
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
  }
  nrrdAxesSet_nva(nrrd, nrrdAxesInfoMax, val);
  return 0;
}

/*
*/
int
_nrrdReadNrrdParse_centers(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_centers", err[AIR_STRLEN_MED];
  int i;
  char *tok;
  char *info, *last;

  info = io->line + io->pos;
  _CHECK_HAVE_DIM;
  for (i=0; i<=nrrd->dim-1; i++) {
    tok = airStrtok(!i ? info : NULL, _nrrdFieldSep, &last);
    if (!tok) {
      sprintf(err, "%s: couldn't extract string for center %d of %d",
	      me, i+1, nrrd->dim);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    if (!strcmp(tok, NRRD_UNKNOWN)) {
      nrrd->axis[i].center = nrrdCenterUnknown;
      continue;
    }
    if (!(nrrd->axis[i].center = airEnumVal(nrrdCenter, tok))) {
      sprintf(err, "%s: couldn't parse \"%s\" center %d of %d",
	      me, tok, i+1, nrrd->dim);
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
_nrrdReadNrrdParse_labels(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_labels", err[AIR_STRLEN_MED], *tmp;
  char *h;  /* this is the "here" pointer which gradually progresses
	       through all the labels (for all axes) */
  int i, len;
  airArray *tmpArr;
  char *info;

  /* because we have to correctly interpret quote marks, we
     can't simply rely on airParseStrS */
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
    tmp = NULL;
    tmpArr = airArrayNew((void**)(&tmp), NULL, sizeof(char), 2);
    if (!tmpArr) {
      sprintf(err, "%s: couldn't create airArray for label %d of %d\n",
	      me, i+1, nrrd->dim);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    len = airArrayIncrLen(tmpArr, 1);  /* len should be 0 */
    while (h[len]) {
      /* printf("!%s: (%d) h+%d |%s|\n", me, i, len, h+len); */
      if ('\"' == h[len]) {
	break;
      }
      if ('\\' == h[len] && '\"' == h[len+1]) {
	h += 1;
      }
      tmp[len] = h[len];
      len = airArrayIncrLen(tmpArr, 1);
    }
    if ('\"' != h[len]) {
      sprintf(err, "%s: parsing label %d of %d, didn't see ending \" "
	      "soon enough", me, i+1, nrrd->dim);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    tmp[len] = 0;
    nrrd->axis[i].label = airStrdup(tmp);
    if (!nrrd->axis[i].label) {
      sprintf(err, "%s: couldn't allocate label %d", me, i);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    h += len+1;
    airArrayNuke(tmpArr);
    /* printf("!%s: out[%d] |%s|\n", me, i, nrrd->axis[i].label); */
  }

  return 0;
}

int
_nrrdReadNrrdParse_number(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  /*
  char me[]="_nrrdReadNrrdParse_number", err[AIR_STRLEN_MED]; 
  char *info;

  info = io->line + io->pos;
  if (1 != sscanf(info, NRRD_BIG_INT_PRINTF, &(nrrd->num))) {
    sprintf(err, "%s: couldn't parse number \"%s\"", me, info);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  } 
  */

  /* It was decided to just completely ignore this field.  "number" is
  ** entirely redundant with with (required) sizes field, and there no
  ** need to save it to, or learn it from, the header.  In fact the "num"
  ** field was eliminated from the Nrrd struct some time ago, in favor of
  ** the nrrdElementNumber() function.  It may seem odd or unfortunate that
  ** 
  **     number: Hank Hill sells propane and propane accessories
  **
  ** is a valid field specification, but whatever ...
  */

  return 0;
}

int
_nrrdReadNrrdParse_content(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_content", err[AIR_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  if (strlen(info) && !(nrrd->content = airStrdup(info))) {
    sprintf(err, "%s: couldn't strdup() content", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdReadNrrdParse_block_size(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_block_size", err[AIR_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  if (nrrdTypeBlock != nrrd->type) {
    sprintf(err, "%s: known type (%s) is not (%s)", me,
	    airEnumStr(nrrdType, nrrd->type),
	    airEnumStr(nrrdType, nrrdTypeBlock));
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  _PARSE_ONE_VAL(nrrd->blockSize, "%d", "int");
  return 0;
}

int
_nrrdReadNrrdParse_min(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_min", err[AIR_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  _PARSE_ONE_VAL(nrrd->min, "%lg", "double");
  return 0;
}

int
_nrrdReadNrrdParse_max(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_max", err[AIR_STRLEN_MED];
  char *info;

  info = io->line + io->pos;
  _PARSE_ONE_VAL(nrrd->max, "%lg", "double");
  return 0;
}

int
_nrrdReadNrrdParse_old_min(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_old_min", err[AIR_STRLEN_MED];
  char *info;
  int sgn;

  info = io->line + io->pos;
  _PARSE_ONE_VAL(nrrd->oldMin, "%lg", "double");
  if ((sgn=airIsInf_d(nrrd->oldMin))) {
    sprintf(err, "%s: old min %sinf invalid", me, 1==sgn ? "+" : "-");
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdReadNrrdParse_old_max(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_old_max", err[AIR_STRLEN_MED];
  char *info;
  int sgn;

  info = io->line + io->pos;
  _PARSE_ONE_VAL(nrrd->oldMax, "%lg", "double");
  if ((sgn=airIsInf_d(nrrd->oldMax))) {
    sprintf(err, "%s: old max %sinf invalid", me, 1==sgn ? "+" : "-");
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

/*
** strerror(errno): not thread-safe
*/
int
_nrrdReadNrrdParse_data_file(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_data_file", err[AIR_STRLEN_MED], *dataName;
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
    dataName = malloc(strlen(io->dir) + strlen(info) + 2);
    sprintf(dataName, "%s/%s", io->dir, info);
  } else {
    /* data file's name is absolute (not header-relative) */
    dataName = airStrdup(info);
  }
  if (!(io->dataFile = fopen(dataName, "rb"))) {
    sprintf(err, "%s: fopen(\"%s\",\"rb\") failed: %s",
	    me, dataName, strerror(errno));
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  io->seperateHeader = AIR_TRUE;
  /* the seperate data file will be closed in _nrrdReadNrrd() */
  free(dataName);
  return 0;
}

int
_nrrdReadNrrdParse_line_skip(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_line_skip", err[AIR_STRLEN_MED];
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
_nrrdReadNrrdParse_byte_skip(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParse_byte_skip", err[AIR_STRLEN_MED];
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
(*_nrrdReadNrrdParseInfo[NRRD_FIELD_MAX+1])(Nrrd *, NrrdIO *, int) = {
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
_nrrdReadNrrdParseField(Nrrd *nrrd, NrrdIO *io, int useBiff) {
  char me[]="_nrrdReadNrrdParseField", err[AIR_STRLEN_MED], *next;
  int i;
  
  next = io->line + io->pos;

  /* determining if its a comment is simple */
  if (NRRD_COMMENT_CHAR == next[0]) {
    return nrrdField_comment;
  }

  /* else we have some field to parse */
  for (i=nrrdField_unknown+1; i<=NRRD_FIELD_MAX; i++) {
    if (!strncmp(next, nrrdField->str[i], strlen(nrrdField->str[i]))) {
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
  next += strlen(nrrdField->str[i]);
  /* skip whitespace ... */
  next += strspn(next, _nrrdFieldSep);
  /* see colon ? */
  if (':' != *next) {
    sprintf(err, "%s: didn't see \":\" after \"%s\"", 
	    me, nrrdField->str[i]);
    biffMaybeAdd(NRRD, err, useBiff); return nrrdField_unknown;
  }
  next++;
  /* skip whitespace ... */
  next += strspn(next, _nrrdFieldSep);

  io->pos = next - io->line;

  return i;
}

/* kernel parsing is all in kernel.c */
