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

#include "unrrdu.h"

/* number of columns that hest will used */
int
unrrduDefNumColumns = 78;

/*
******** unrrduCmdList[]
**
** NULL-terminated array of unrrduCmd pointers, as ordered by UNRRDU_MAP macro
*/
unrrduCmd *
unrrduCmdList[] = {
  UNRRDU_MAP(UNRRDU_LIST)
  NULL
};

/*
******** unrrduUsage
**
** prints out a little banner, and a listing of all available commands
** with their one-line descriptions
*/
void
unrrduUsage(char *me, hestParm *hparm) {
  int i, maxlen, len, c;
  char buff[AIR_STRLEN_LARGE], fmt[AIR_STRLEN_LARGE];

  maxlen = 0;
  for (i=0; unrrduCmdList[i]; i++) {
    maxlen = AIR_MAX(maxlen, strlen(unrrduCmdList[i]->name));
  }

  sprintf(buff, "--- Utah Nrrd Utilities (unrrdu) command-line interface ---");
  sprintf(fmt, "%%%ds\n",
	  (int)((hparm->columns-strlen(buff))/2 + strlen(buff) - 1));
  fprintf(stderr, fmt, buff);
  
  for (i=0; unrrduCmdList[i]; i++) {
    len = strlen(unrrduCmdList[i]->name);
    strcpy(buff, "");
    for (c=len; c<maxlen; c++)
      strcat(buff, " ");
    strcat(buff, me);
    strcat(buff, " ");
    strcat(buff, unrrduCmdList[i]->name);
    strcat(buff, " ... ");
    len = strlen(buff);
    fprintf(stderr, "%s", buff);
    _hestPrintStr(stderr, len, len, hparm->columns,
		  unrrduCmdList[i]->info, AIR_FALSE);
  }
}

/*
******** unrrduPosHestCB
**
** For parsing position along an axis. Can be a simple integer,
** or M to signify last position along axis (#samples-1), or
** M+<int> or M-<int> to signify some position relative to the end.
**
** It can also be m+<int> to signify some position relative to some
** "minimum", assuming that a minimum position is being specified
** at the same time as this one.  Obviously, there has to be some 
** error handling to make sure that no one is trying to define a
** minimum position with respect to itself.  And, the ability to
** specify a position as "m+<int>" shouldn't be advertised in situations
** (unu slice) where you only have one position, rather than an interval
** between two positions (unu crop and unu pad).
**
** This information is represented with two integers, pos[0] and pos[1]:
** pos[0] ==  0: pos[1] gives the absolute position
** pos[0] ==  1: pos[1] gives the position relative to the last index
** pos[0] == -1: pos[1] gives the position relative to a "minimum" position
*/
int
unrrduParsePos(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unrrduParsePos";
  int *pos;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  pos = ptr;
  if (!strcmp("M", str)) {
    pos[0] = 1;
    pos[1] = 0;
    return 0;
  }
  if ('M' == str[0]) {
    if (!( '-' == str[1] || '+' == str[1] )) {
      sprintf(err, "%s: \'M\' can be followed only by \'+\' or \'-\'", me);
      return 1;
    }
    pos[0] = 1;
    if (1 != sscanf(str+1, "%d", &(pos[1]))) {
      sprintf(err, "%s: can't parse \"%s\" as M+<int> or M-<int>", me, str);
      return 1;
    }
    return 0;
  }
  if ('m' == str[0]) {
    if ('+' != str[1]) {
      sprintf(err, "%s: \'m\' can only be followed by \'+\'", me);
      return 1;
    }
    pos[0] = -1;
    if (1 != sscanf(str+1, "%d", &(pos[1]))) {
      sprintf(err, "%s: can't parse \"%s\" as m+<int>", me, str);
      return 1;
    }
    if (pos[1] < 0 ) {
      sprintf(err, "%s: int in m+<int> must be non-negative (not %d)",
	      me, pos[1]);
      return 1;
    }
    return 0;
  }
  /* else its just a plain unadorned integer */
  pos[0] = 0;
  if (1 != sscanf(str, "%d", &(pos[1]))) {
    sprintf(err, "%s: can't parse \"%s\" as int", me, str);
    return 1;
  }
  return 0;
}

hestCB unrrduPosHestCB = {
  2*sizeof(int),
  "position",
  unrrduParsePos,
  NULL
};

/*
******** unrrduMaybeTypeHestCB
**
** although nrrdType is an airEnum that hest already knows how
** to parse, we want the ability to have "unknown" be a valid
** parsable value, contrary to how airEnums usually work with hest.
** For instance, we might want to use "unknown" to represent
** "same type as the input, whatever that is".
*/
int
unrrduParseMaybeType(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unrrduParseMaybeType";
  int *typeP;

  /* fprintf(stderr, "!%s: str = \"%s\"\n", me, str); */
  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  typeP = ptr;
  if (!strcmp("unknown", str)) {
    *typeP = nrrdTypeUnknown;
  } else {
    *typeP = airEnumVal(nrrdType, str);
    if (nrrdTypeUnknown == *typeP) {
      sprintf(err, "%s: can't parse \"%s\" as type", me, str);
      return 1;
    }
  }
  /* fprintf(stderr, "!%s: *typeP = %d\n", me, *typeP); */
  return 0;
}

hestCB unrrduMaybeTypeHestCB = {
  sizeof(int),
  "type",
  unrrduParseMaybeType,
  NULL
};

/*
******** unrrduBitsHestCB
** 
** for parsing an int that can be 8, 16, or 32
*/
int
unrrduParseBits(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unrrduParseBits";
  int *bitsP;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  bitsP = ptr;
  if (1 != sscanf(str, "%d", bitsP)) {
    sprintf(err, "%s: can't parse \"%s\" as int", me, str);
    return 1;
  }
  if (!( 8 == *bitsP || 16 == *bitsP || 32 == *bitsP )) {
    sprintf(err, "%s: bits (%d) not 8, 16, or 32", me, *bitsP);
    return 1;
  }
  return 0;
}

hestCB unrrduBitsHestCB = {
  sizeof(int),
  "quantization bits",
  unrrduParseBits,
  NULL
};

/*
******** unrrduParseScale
**
** parse "=", "x<float>", and "<int>".  These possibilities are represented
** for axis i by setting scale[0 + 2*i] to 0, 1, or 2, respectively.
*/
int
unrrduParseScale(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unrrduParseScale";
  float *scale;
  int num;
  
  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  scale = ptr;
  if (!strcmp("=", str)) {
    scale[0] = 0.0;
    scale[1] = 0.0;
    return 0;
  }

  /* else */
  if ('x' == str[0]) {
    if (1 != sscanf(str+1, "%f", scale+1)) {
      sprintf(err, "%s: can't parse \"%s\" as x<float>", me, str);
      return 1;
    }
    scale[0] = 1.0;
  }
  else {
    if (1 != sscanf(str, "%d", &num)) {
      sprintf(err, "%s: can't parse \"%s\" as int", me, str);
      return 1;
    }
    scale[0] = 2.0;
    scale[1] = num;
  }
  return 0;
}

hestCB unrrduScaleHestCB = {
  2*sizeof(float),
  "sampling specification",
  unrrduParseScale,
  NULL
};

/*
******** unrrduFileHestCB
**
** for parsing a filename, which means opening it in "rb" mode and
** getting a FILE *.  "-" is interpreted as stdin, which is not
** fclose()ed at the end, unlike all other files.
*/
void *
unrrduMaybeFclose(void *_file) {
  FILE *file;
  
  file = _file;
  if (stdin != file) {
    return airFclose(file);
  }
  return NULL;
}

int
unrrduParseFile(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unrrduParseFile";
  FILE **fileP;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  fileP = ptr;
  if (!strcmp("-", str)) {
    *fileP = stdin;
  }
  else {
    *fileP = fopen(str, "rb");
    if (!*fileP) {
      sprintf(err, "%s: fopen(\"%s\",\"rb\") failed: %s",
	      me, str, strerror(errno));
      return 1;
    }
  }
  return 0;
}

hestCB unrrduFileHestCB = {
  sizeof(FILE *),
  "filename",
  unrrduParseFile,
  unrrduMaybeFclose,
};

