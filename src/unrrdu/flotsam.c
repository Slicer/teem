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
unuDefNumColumns = 78;

/*
******** unuCmdList[]
**
** NULL-terminated array of unuCmd pointers, as ordered by UNU_MAP macro
*/
unuCmd *
unuCmdList[] = {
  UNU_MAP(UNU_LIST)
  NULL
};

/*
******** unuUsage
**
** prints out a little banner, and a listing of all available commands
** with their one-line descriptions
*/
void
unuUsage(char *me, hestParm *hparm) {
  int i, maxlen, len, c;
  char buff[AIR_STRLEN_LARGE], fmt[AIR_STRLEN_LARGE];

  maxlen = 0;
  for (i=0; unuCmdList[i]; i++) {
    maxlen = AIR_MAX(maxlen, strlen(unuCmdList[i]->name));
  }

  sprintf(buff, "--- Utah Nrrd Utilities (unrrdu) command-line interface ---");
  sprintf(fmt, "%%%ds\n",
	  (int)((hparm->columns-strlen(buff))/2 + strlen(buff) - 1));
  fprintf(stderr, fmt, buff);
  
  for (i=0; unuCmdList[i]; i++) {
    len = strlen(unuCmdList[i]->name);
    strcpy(buff, "");
    for (c=len; c<maxlen; c++)
      strcat(buff, " ");
    strcat(buff, me);
    strcat(buff, " ");
    strcat(buff, unuCmdList[i]->name);
    strcat(buff, " ... ");
    len = strlen(buff);
    fprintf(stderr, "%s", buff);
    _hestPrintStr(stderr, len, len, hparm->columns,
		  unuCmdList[i]->info, AIR_FALSE);
  }
}

/*
******** unuPosHestCB
**
** For parsing position along an axis. Can be a simple integer,
** or M to signify last position along axis (#samples-1), or
** M+<int> or M-<int> to signify some position relative to the end.
*/
int
unuParsePos(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unuParsePos";
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
    if (!('-' == str[1] || '+' == str[1])) {
      sprintf(err, "%s: M can be followed only by + or -", me);
      return 1;
    }
    pos[0] = 1;
    if (1 != sscanf(str+1, "%d", pos + 1)) {
      sprintf(err, "%s: can't parse \"%s\" as M+<int> or M-<int>", me, str);
      return 1;
    }
  } else {
    pos[0] = 0;
    if (1 != sscanf(str, "%d", pos + 1)) {
      sprintf(err, "%s: can't parse \"%s\" as int", me, str);
      return 1;
    }
  }
  return 0;
}

hestCB unuPosHestCB = {
  2*sizeof(int),
  "position",
  unuParsePos,
  NULL
};

/*
******** unuMaybeTypeHestCB
**
** although nrrdType is an airEnum that hest already knows how
** to parse, we want the ability to have "unknown" be a valid
** parsable value, contrary to how airEnums usually work with hest.
** For instance, we might want to use "unknown" to represent
** "same type as the input, whatever that is".
*/
int
unuParseMaybeType(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unuParseMaybeType";
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

hestCB unuMaybeTypeHestCB = {
  sizeof(int),
  "type",
  unuParseMaybeType,
  NULL
};

/*
******** unuBitsHestCB
** 
** for parsing an int that can be 8, 16, or 32
*/
int
unuParseBits(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unuParseBits";
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

hestCB unuBitsHestCB = {
  sizeof(int),
  "quantization bits",
  unuParseBits,
  NULL
};

/*
******** unuParseScale
**
** parse "=", "x<float>", and "<int>".  These possibilities are represented
** for axis i by setting scale[0 + 2*i] to 0, 1, or 2, respectively.
*/
int
unuParseScale(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unuParseScale";
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

hestCB unuScaleHestCB = {
  2*sizeof(float),
  "sampling specification",
  unuParseScale,
  NULL
};

/*
******** unuFileHestCB
**
** for parsing a filename, which means opening it in "rb" mode and
** getting a FILE *.  "-" is interpreted as stdin, which is not
** fclose()ed at the end, unlike all other files.
*/
void *
unuMaybeFclose(void *_file) {
  FILE *file;
  
  file = _file;
  if (stdin != file) {
    return airFclose(file);
  }
  return NULL;
}

int
unuParseFile(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unuParseFile";
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

hestCB unuFileHestCB = {
  sizeof(FILE *),
  "filename",
  unuParseFile,
  unuMaybeFclose,
};

