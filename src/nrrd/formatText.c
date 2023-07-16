/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "nrrd.h"
#include "privateNrrd.h"

static int
_nrrdFormatText_available(void) {

  return AIR_TRUE;
}

static int
_nrrdFormatText_nameLooksLike(const char *fname) {

  return (airEndsWith(fname, NRRD_EXT_TEXT) || airEndsWith(fname, ".text")
          || airEndsWith(fname, ".ascii"));
}

static int /* Biff: maybe:3:AIR_FALSE */
_nrrdFormatText_fitsInto(const Nrrd *nrrd, const NrrdEncoding *encoding, int useBiff) {
  static const char me[] = "_nrrdFormatText_fitsInto";

  AIR_UNUSED(encoding);
  /* encoding ignored- always ascii */
  if (!(1 == nrrd->dim || 2 == nrrd->dim)) {
    biffMaybeAddf(useBiff, NRRD, "%s: dimension is %d, not 1 or 2", me, nrrd->dim);
    return AIR_FALSE;
  }
  if (nrrdTypeBlock == nrrd->type) {
    biffMaybeAddf(useBiff, NRRD, "%s: can't save blocks to plain text", me);
    return AIR_FALSE;
  }
  /* NOTE: type of array not guaranteed to survive */
  return AIR_TRUE;
}

static int
_nrrdFormatText_contentStartsLike(NrrdIoState *nio) {
  float oneFloat;

  return (NRRD_COMMENT_CHAR == nio->line[0]
          || airParseStrF(&oneFloat, nio->line, _nrrdTextSep, 1));
}

static int /* Biff: 1 */
_nrrdFormatText_read(FILE *file, Nrrd *nrrd, NrrdIoState *nio) {
  static const char me[] = "_nrrdFormatText_read";
  const char *fs;
  char *errS;
  unsigned int llen;
  size_t line, plen, sx, sy, elsz, size[NRRD_DIM_MAX];
  int nret, fidx, settwo = 0, gotOnePerAxis = AIR_FALSE;
  /* fl: first line (of floats) */
  airArray *flArr, *dataArr;
  float oneFloat;
  char *data;
  size_t (*parser)(void *, const char *, const char *, size_t);
  airPtrPtrUnion appu;

  if (!_nrrdFormatText_contentStartsLike(nio)) {
    biffAddf(NRRD, "%s: this doesn't look like a %s file", me, nrrdFormatText->name);
    return 1;
  }

  /* this goofiness is just to leave the nrrd as we found it
     (specifically, nrrd->dim) when we hit an error */
#define UNSETTWO                                                                        \
  if (settwo) nrrd->dim = settwo

  /* we only get here with the first line already in nio->line */
  line = 1;
  llen = AIR_UINT(strlen(nio->line));

  if (0 == nrrd->dim) {
    settwo = nrrd->dim;
    nrrd->dim = 2;
  }
  /* first, we get through comments */
  while (NRRD_COMMENT_CHAR == nio->line[0]) {
    nio->pos = 1;
    nio->pos += AIR_INT(strspn(nio->line + nio->pos, _nrrdFieldSep));
    fidx = _nrrdReadNrrdParseField(nio, AIR_FALSE);
    /* could we parse anything? */
    if (!fidx) {
      /* being unable to parse a comment as a nrrd field is not
         any kind of error */
      goto plain;
    }
    if (nrrdField_comment == fidx) {
      fidx = 0;
      goto plain;
    }
    fs = airEnumStr(nrrdField, fidx);
    if (!_nrrdFieldValidInText[fidx]) {
      if (1 <= nrrdStateVerboseIO) {
        fprintf(stderr,
                "(%s: field \"%s\" not allowed in plain text "
                "--> plain comment)\n",
                me, fs);
      }
      fidx = 0;
      goto plain;
    }
    /* when reading plain text, we simply ignore repetitions of a field */
    if ((nrrdField_keyvalue == fidx || !nio->seen[fidx])
        && nrrdFieldInfoParse[fidx](file, nrrd, nio, AIR_TRUE)) {
      errS = biffGetDone(NRRD);
      if (1 <= nrrdStateVerboseIO) {
        fprintf(stderr, "%s: %s", me, errS);
        fprintf(stderr, "(%s: malformed field \"%s\" --> plain comment)\n", me, fs);
      }
      if (nrrdField_dimension == fidx) {
        /* "# dimension: 0" lead nrrd->dim being set to 0 */
        /* HEY reconcile this with 7 lines later! */
        nrrd->dim = 2;
      }
      free(errS);
      fidx = 0;
      goto plain;
    }
    if (nrrdField_dimension == fidx) {
      if (!(1 == nrrd->dim || 2 == nrrd->dim)) {
        if (1 <= nrrdStateVerboseIO) {
          fprintf(stderr,
                  "(%s: plain text dimension can only be 1 or 2; "
                  "resetting to 2)\n",
                  me);
        }
        nrrd->dim = 2;
      }
      if (1 == nrrd->dim && gotOnePerAxis) {
        fprintf(stderr,
                "(%s: already parsed per-axis field, can't reset "
                "dimension to 1; resetting to 2)\n",
                me);
        nrrd->dim = 2;
      }
    }
    if (_nrrdFieldOnePerAxis[fidx]) {
      gotOnePerAxis = AIR_TRUE;
    }
    nio->seen[fidx] = AIR_TRUE;
  plain:
    if (!fidx) {
      if (nrrdCommentAdd(nrrd, nio->line + 1)) {
        biffAddf(NRRD, "%s: couldn't add comment", me);
        UNSETTWO;
        return 1;
      }
    }
    if (nrrdOneLine(&llen, nio, file)) {
      biffAddf(NRRD, "%s: error getting a line", me);
      UNSETTWO;
      return 1;
    }
    if (!llen) {
      biffAddf(NRRD, "%s: hit EOF before any numbers parsed", me);
      UNSETTWO;
      return 1;
    }
    line++;
  }

  /* we supposedly have a line of numbers, now see how many there are.
     For the specific purpose of counting numbers, we assume float type;
     but we aren't going to remember these values. */
  if (!airParseStrF(&oneFloat, nio->line, _nrrdTextSep, 1)) {
    char stmp[AIR_STRLEN_SMALL + 1];
    biffAddf(NRRD, "%s: couldn't parse a single number on line %s", me,
             airSprintSize_t(stmp, line));
    UNSETTWO;
    return 1;
  }
  flArr = airArrayNew(NULL, NULL, sizeof(float), _NRRD_TEXT_INCR);
  if (!flArr) {
    biffAddf(NRRD, "%s: couldn't create array for first line values", me);
    UNSETTWO;
    return 1;
  }
  for (sx = 1; 1; sx++) {
    /* there is obviously a limit to the number of numbers that can
       be parsed from a single finite line of input text */
    airArrayLenSet(flArr, AIR_UINT(sx));
    if (!flArr->data) {
      char stmp[AIR_STRLEN_SMALL + 1];
      biffAddf(NRRD, "%s: couldn't alloc space for %s values", me,
               airSprintSize_t(stmp, sx));
      UNSETTWO;
      return 1;
    }
    if (sx
        > airParseStrF((float *)(flArr->data), nio->line, _nrrdTextSep, AIR_UINT(sx))) {
      /* We asked for sx ints and got less.  We know that we successfully
         got one value, so we did succeed in parsing sx-1 values */
      sx--;
      break;
    }
  }
  flArr = airArrayNuke(flArr); /* forget about values parsed on 1st line */
  if (1 == nrrd->dim && 1 != sx) {
    char stmp[AIR_STRLEN_SMALL + 1];
    biffAddf(NRRD, "%s: wanted 1-D nrrd, but got %s values on 1st line", me,
             airSprintSize_t(stmp, sx));
    UNSETTWO;
    return 1;
  }
  /* else sx == 1 when nrrd->dim == 1 */

  if (nrrdTypeBlock == nrrd->type) {
    biffAddf(NRRD, "%s: can't read %s type data from text", me,
             airEnumStr(nrrdType, nrrdTypeBlock));
    UNSETTWO;
    return 1;
  }
  /* If nrrd->type is non-zero (something other than nrrdTypeUnknown), that
     means the value type in the text file has been explicitly documented via
     a "type:" field (thanks to nio->moreThanFloatInText). We learned sx
     above by counting how many *floats* could be parsed, but next we have to
     actually learn values of type only known at run-time.

     Else (nrrd->type is zero): nio->moreThanFloatInText describes the
     capability of plain text file on *write*, but there isn't currently a
     way of saying "when reading plain text that doesn't say otherwise, we
     should understand it as of type THIS", so we'll stick with the type that
     this library has long associated with plain text: float */
  if (!nrrd->type) {
    nrrd->type = nrrdTypeFloat;
  }
  elsz = nrrdTypeSize[nrrd->type];
  parser = nrrdStringValsParse[nrrd->type];

  /* now see how many more lines there are */
  appu.c = &data;
  dataArr = airArrayNew(appu.v, NULL, sx * elsz, _NRRD_TEXT_INCR);
  if (!dataArr) {
    biffAddf(NRRD, "%s: couldn't create data buffer", me);
    UNSETTWO;
    return 1;
  }
  sy = 0;
  while (llen) {
    airArrayLenIncr(dataArr, 1);
    if (!dataArr->data) {
      char stmp[AIR_STRLEN_SMALL + 1];
      biffAddf(NRRD, "%s: couldn't create scanline of %s values", me,
               airSprintSize_t(stmp, sx));
      UNSETTWO;
      return 1;
    }
    plen = parser(data + sy * sx * elsz, nio->line, _nrrdTextSep, sx);
    if (sx > plen) {
      char stmp[3][AIR_STRLEN_SMALL + 1];
      biffAddf(NRRD, "%s: could only parse %s values (not %s) on line %s", me,
               airSprintSize_t(stmp[0], plen), airSprintSize_t(stmp[1], sx),
               airSprintSize_t(stmp[2], line));
      UNSETTWO;
      return 1;
    }
    sy++;
    line++;
    if (nrrdOneLine(&llen, nio, file)) {
      biffAddf(NRRD, "%s: error getting a line", me);
      UNSETTWO;
      return 1;
    }
  }
  /*
  fprintf(stderr, "%s: nrrd->dim = %d, sx = %d; sy = %d\n",
          me, nrrd->dim, sx, sy);
  */

  if (!(1 == nrrd->dim || 2 == nrrd->dim)) {
    fprintf(stderr, "%s: PANIC about to save, but dim = %d\n", me, nrrd->dim);
    exit(1);
  }
  if (1 == nrrd->dim) {
    size[0] = sy;
  } else {
    size[0] = sx;
    size[1] = sy;
  }

  if (nio->oldData && nio->oldDataSize == sx * sy * elsz) {
    nret = nrrdWrap_nva(nrrd, nio->oldData, nrrd->type, nrrd->dim, size);
  } else {
    nret = nrrdMaybeAlloc_nva(nrrd, nrrd->type, nrrd->dim, size);
  }
  if (nret) {
    biffAddf(NRRD, "%s: couldn't create nrrd for plain text data", me);
    UNSETTWO;
    return 1;
  }
  memcpy(nrrd->data, data, sx * sy * elsz);

  dataArr = airArrayNuke(dataArr);
  return 0;
}

static int
_nrrdFormatText_write(FILE *file, const Nrrd *nrrd, NrrdIoState *nio) {
  char cmt[AIR_STRLEN_SMALL + 1], buff[AIR_STRLEN_SMALL + 1];
  size_t I, dsz;
  int i, x, y, sx, sy; /* HEY unsigned? */
  const void *data;
  const char *cdata;
  float val;
  int moreThanFloat;

  /* should we exercise new functionality to save more than just
     float in a text file */
  moreThanFloat = (nrrdTypeFloat != nrrd->type && !nio->bareText
                   && nio->moreThanFloatInText);
  sprintf(cmt, "%c ", NRRD_COMMENT_CHAR);
  if (!nio->bareText) {
    if (1 == nrrd->dim) {
      _nrrdFprintFieldInfo(file, cmt, nrrd, nio, nrrdField_dimension, AIR_FALSE);
    }
    for (i = 1; i <= NRRD_FIELD_MAX; i++) {
      if (_nrrdFieldValidInText[i]    /* (nrrdType is now valid) */
          && nrrdField_dimension != i /* dimension is handled above */
          && _nrrdFieldInteresting(nrrd, nio, i)
          && (nrrdField_type != i  /* either not type */
              || moreThanFloat)) { /* or is type, and we should
                                      record non-float type */
        _nrrdFprintFieldInfo(file, cmt, nrrd, nio, i, AIR_FALSE);
      }
    }
    if (nrrdKeyValueSize(nrrd)) {
      unsigned int kvi;
      for (kvi = 0; kvi < nrrd->kvpArr->len; kvi++) {
        _nrrdKeyValueWrite(file, NULL, "#", nrrd->kvp[0 + 2 * kvi],
                           nrrd->kvp[1 + 2 * kvi]);
      }
    }
  }

  if (1 == nrrd->dim) {
    sx = 1;
    sy = AIR_INT(nrrd->axis[0].size);
  } else {
    sx = AIR_INT(nrrd->axis[0].size);
    sy = AIR_INT(nrrd->axis[1].size);
  }
  data = nrrd->data;
  cdata = (const char *)nrrd->data;
  dsz = nrrdTypeSize[nrrd->type];
  I = 0;
  for (y = 0; y < sy; y++) {
    for (x = 0; x < sx; x++) {
      if (moreThanFloat) {
        nrrdSprint[nrrd->type](buff, cdata + I * dsz);
      } else {
        val = nrrdFLookup[nrrd->type](data, I);
        nrrdSprint[nrrdTypeFloat](buff, &val);
      }
      if (x) fprintf(file, " ");
      fprintf(file, "%s", buff);
      I++;
    }
    fprintf(file, "\n");
  }

  return 0;
}

const NrrdFormat _nrrdFormatText = {"text",
                                    AIR_FALSE, /* isImage */
                                    AIR_TRUE,  /* readable */
                                    AIR_FALSE, /* usesDIO */
                                    _nrrdFormatText_available,
                                    _nrrdFormatText_nameLooksLike,
                                    _nrrdFormatText_fitsInto,
                                    _nrrdFormatText_contentStartsLike,
                                    _nrrdFormatText_read,
                                    _nrrdFormatText_write};

const NrrdFormat *const nrrdFormatText = &_nrrdFormatText;
