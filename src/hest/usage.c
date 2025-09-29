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
  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, see <https://www.gnu.org/licenses/>.
*/

#include "hest.h"
#include "privateHest.h"

/*
** don't ask
*/
static void
_hestSetBuff(char *B, const hestOpt *O, const hestParm *P, int showshort, int showlong) {
  char copy[AIR_STRLEN_HUGE + 1], *sep;
  unsigned int len;
  AIR_UNUSED(P); // formerly for P->multiFlagSep
  int max = _hestMax(O->max);
  if (O->flag) {
    strcpy(copy, O->flag);
    if ((sep = strchr(copy, MULTI_FLAG_SEP))) {
      *sep = 0;
      if (showshort) {
        strcat(B, "-");
        strcat(B, copy);
      }
      if (showlong) {
        if (showshort) {
          len = AIR_UINT(strlen(B));
          B[len] = MULTI_FLAG_SEP;
          B[len + 1] = '\0';
        }
        strcat(B, "--");
        strcat(B, sep + 1);
      }
    } else {
      strcat(B, "-");
      strcat(B, O->flag);
    }
    if (O->min || max) {
      strcat(B, "\t");
    }
  }
  if (!O->min && max) {
    strcat(B, "[");
  }
  if (O->min || max) {
    strcat(B, "<");
    strcat(B, O->name);
    if ((int)(O->min) < max && max > 1) { /* HEY scrutinize casts */
      strcat(B, "\t...");
    }
    strcat(B, ">");
  }
  if (!O->min && max) {
    strcat(B, "]");
  }
}

/* early version of _hestSetBuff() function */
#define SETBUFF(B, O)                                                                   \
  strcat(B, O.flag ? "-" : ""), strcat(B, O.flag ? O.flag : ""),                        \
    strcat(B, O.flag && (O.min || _hestMax(O.max)) ? "\t" : ""),                        \
    strcat(B, !O.min && _hestMax(O.max) ? "[" : ""),                                    \
    strcat(B, O.min || _hestMax(O.max) ? "<" : ""),                                     \
    strcat(B, O.min || _hestMax(O.max) ? O.name : ""),                                  \
    strcat(B, (O.min < _hestMax(O.max) && (_hestMax(O.max) > 1)) ? " ..." : ""),        \
    strcat(B, O.min || _hestMax(O.max) ? ">" : ""),                                     \
    strcat(B, !O.min && _hestMax(O.max) ? "]" : "");

/*
** _hestPrintStr()
**
** not a useful function.  Do not use.
*/
void
_hestPrintStr(FILE *f, unsigned int indent, unsigned int already, unsigned int width,
              const char *_str, int bslash) {
  char *str, *ws, *last;
  int newed = AIR_FALSE;
  unsigned int wrd, nwrd, ii, pos;

  str = airStrdup(_str);
  nwrd = airStrntok(str, " ");
  pos = already;
  for (wrd = 0; wrd < nwrd; wrd++) {
    /* we used airStrtok() to delimit words on spaces ... */
    ws = airStrtok(!wrd ? str : NULL, " ", &last);
    /* ... but then convert tabs to spaces */
    airStrtrans(ws, '\t', ' ');
    if (pos + 1 + AIR_UINT(strlen(ws)) <= width - !!bslash) {
      /* if this word would still fit on the current line */
      if (wrd && !newed) fprintf(f, " ");
      fprintf(f, "%s", ws);
      pos += 1 + AIR_UINT(strlen(ws));
      newed = AIR_FALSE;
    } else {
      /* else we start a new line and print the indent */
      if (bslash) {
        fprintf(f, " \\");
      }
      fprintf(f, "\n");
      for (ii = 0; ii < indent; ii++) {
        fprintf(f, " ");
      }
      fprintf(f, "%s", ws);
      pos = indent + AIR_UINT(strlen(ws));
    }
    /* if the last character of the word was a newline, then indent */
    if ('\n' == ws[strlen(ws) - 1]) {
      for (ii = 0; ii < indent; ii++) {
        fprintf(f, " ");
      }
      pos = indent;
      newed = AIR_TRUE;
    } else {
      newed = AIR_FALSE;
    }
  }
  fprintf(f, "\n");
  free(str);
}

#define HPARM (_hparm ? _hparm : hparm)

void
hestInfo(FILE *file, const char *argv0, const char *info, const hestParm *_hparm) {
  hestParm *hparm;

  hparm = _hparm ? NULL : hestParmNew();
  /* how to const-correctly use hparm or _hparm in an expression */
  if (info) {
    if (argv0) {
      fprintf(file, "\n%s: ", argv0);
      _hestPrintStr(file, 0, AIR_UINT(strlen(argv0)) + 2, HPARM->columns, info,
                    AIR_FALSE);
      if (HPARM->noBlankLineBeforeUsage) {
        /* we still want a blank line to separate info and usage */
        fprintf(file, "\n");
      }
    } else {
      fprintf(file, "ERROR: hestInfo got NULL argv0\n");
    }
  }
  if (hparm) {
    hestParmFree(hparm);
  }
}

// error string song and dance
#define DO_ERR                                                                          \
  char *err = biffGetDone(HEST);                                                        \
  fprintf(stderr, "%s: problem with given hestOpt array\n%s", __func__, err);           \
  free(err)

void
hestUsage(FILE *f, const hestOpt *hopt, const char *argv0, const hestParm *_hparm) {
  int i, numOpts;
  /* with a very large number of options, it is possible to overflow buff[].
  Previous to the 2023 revisit, it was for max lenth 2*AIR_STRLEN_HUGE, but
  test/ex6.c blew past that.  May have to increment again in the future :) */
  char buff[64 * AIR_STRLEN_HUGE + 1], tmpS[AIR_STRLEN_SMALL + 1];
  hestParm *hparm = _hparm ? NULL : hestParmNew();

  if (_hestOPCheck(hopt, HPARM)) {
    /* we can't continue; the opt array is botched */
    DO_ERR;
    if (hparm) {
      hestParmFree(hparm);
    }
    return;
  }

  numOpts = hestOptNum(hopt);
  if (!(HPARM->noBlankLineBeforeUsage)) {
    fprintf(f, "\n");
  }
  strcpy(buff, "Usage: ");
  strcat(buff, argv0 ? argv0 : "");
  if (HPARM->responseFileEnable) {
    sprintf(tmpS, " [%cfile\t...]", RESPONSE_FILE_FLAG);
    strcat(buff, tmpS);
  }
  for (i = 0; i < numOpts; i++) {
    strcat(buff, " ");
    if (1 == hopt[i].kind || (hopt[i].flag && hopt[i].dflt)) strcat(buff, "[");
    _hestSetBuff(buff, hopt + i, HPARM, AIR_TRUE, AIR_TRUE);
    if (1 == hopt[i].kind || (hopt[i].flag && hopt[i].dflt)) strcat(buff, "]");
  }

  _hestPrintStr(f, AIR_UINT(strlen("Usage: ")), 0, HPARM->columns, buff, AIR_TRUE);
  if (hparm) {
    hestParmFree(hparm);
  }
  return;
}

void
hestGlossary(FILE *f, const hestOpt *hopt, const hestParm *_hparm) {
  int i, j, maxlen, numOpts;
  unsigned int len;
  /* See note above about overflowing buff[] */
  char buff[64 * AIR_STRLEN_HUGE + 1], tmpS[AIR_STRLEN_HUGE + 1];
  hestParm *hparm = _hparm ? NULL : hestParmNew();

  if (_hestOPCheck(hopt, HPARM)) {
    /* we can't continue; the opt array is botched */
    DO_ERR;
    if (hparm) {
      hestParmFree(hparm);
    }
    return;
  }

  numOpts = hestOptNum(hopt);

  maxlen = 0;
  if (numOpts) {
    fprintf(f, "\n");
  }
  for (i = 0; i < numOpts; i++) {
    strcpy(buff, "");
    _hestSetBuff(buff, hopt + i, HPARM, AIR_TRUE, AIR_FALSE);
    maxlen = AIR_MAX((int)strlen(buff), maxlen);
  }
  if (HPARM->responseFileEnable) {
    sprintf(buff, "%cfile ...", RESPONSE_FILE_FLAG);
    len = AIR_UINT(strlen(buff));
    for (j = len; j < maxlen; j++) {
      fprintf(f, " ");
    }
    fprintf(f, "%s = ", buff);
    strcpy(buff, "response file(s) containing command-line arguments");
    _hestPrintStr(f, maxlen + 3, maxlen + 3, HPARM->columns, buff, AIR_FALSE);
  }
  for (i = 0; i < numOpts; i++) {
    strcpy(buff, "");
    _hestSetBuff(buff, hopt + i, HPARM, AIR_TRUE, AIR_FALSE);
    airOneLinify(buff);
    len = AIR_UINT(strlen(buff));
    for (j = len; j < maxlen; j++) {
      fprintf(f, " ");
    }
    fprintf(f, "%s", buff);
    strcpy(buff, "");
#if 1
    if (hopt[i].flag && strchr(hopt[i].flag, MULTI_FLAG_SEP)) {
      /* there is a long-form flag as well as short */
      _hestSetBuff(buff, hopt + i, HPARM, AIR_FALSE, AIR_TRUE);
      strcat(buff, " = ");
      fprintf(f, " , ");
    } else {
      /* there is only a short-form flag */
      fprintf(f, " = ");
    }
#else
    fprintf(f, " = ");
#endif
    if (hopt[i].info) {
      strcat(buff, hopt[i].info);
    }
    if ((hopt[i].min || _hestMax(hopt[i].max))
        && (!(2 == hopt[i].kind && airTypeEnum == hopt[i].type
              && HPARM->elideSingleEnumType))
        && (!(2 == hopt[i].kind && airTypeOther == hopt[i].type
              && HPARM->elideSingleOtherType))) {
      /* if there are newlines in the info, then we want to clarify the
         type by printing it on its own line */
      if (hopt[i].info && strchr(hopt[i].info, '\n')) {
        strcat(buff, "\n ");
      } else {
        strcat(buff, " ");
      }
      strcat(buff, "(");
      if (hopt[i].min == 0 && _hestMax(hopt[i].max) == 1) {
        strcat(buff, "optional\t");
      } else {
        if ((int)hopt[i].min == _hestMax(hopt[i].max)
            && _hestMax(hopt[i].max) > 1) { /* HEY scrutinize casts */
          sprintf(tmpS, "%d\t", _hestMax(hopt[i].max));
          strcat(buff, tmpS);
        } else if ((int)hopt[i].min < _hestMax(hopt[i].max)) { /* HEY scrutinize casts */
          if (-1 == hopt[i].max) {
            sprintf(tmpS, "%d\tor\tmore\t", hopt[i].min);
          } else {
            sprintf(tmpS, "%d..%d\t", hopt[i].min, _hestMax(hopt[i].max));
          }
          strcat(buff, tmpS);
        }
      }
      sprintf(tmpS, "%s%s",
              (airTypeEnum == hopt[i].type
                 ? hopt[i].enm->name
                 : (airTypeOther == hopt[i].type ? hopt[i].CB->type
                                                 : _hestTypeStr[hopt[i].type])),
              (_hestMax(hopt[i].max) > 1
                 ? (airTypeOther == hopt[i].type
                        && 'y' == hopt[i].CB->type[airStrlen(hopt[i].CB->type) - 1]
                        && HPARM->cleverPluralizeOtherY
                      ? "\bies"
                      : "s")
                 : ""));
      strcat(buff, tmpS);
      strcat(buff, ")");
    }
    /*
    fprintf(stderr, "!%s: HPARM->elideSingleOtherDefault = %d\n",
            "hestGlossary", HPARM->elideSingleOtherDefault);
    */
    if (hopt[i].dflt && (hopt[i].min || _hestMax(hopt[i].max))
        && (!(2 == hopt[i].kind
              && (airTypeFloat == hopt[i].type || airTypeDouble == hopt[i].type)
              && !AIR_EXISTS(airAtod(hopt[i].dflt))
              && HPARM->elideSingleNonExistFloatDefault))
        && (!((3 == hopt[i].kind || 5 == hopt[i].kind)
              && (airTypeFloat == hopt[i].type || airTypeDouble == hopt[i].type)
              && !AIR_EXISTS(airAtod(hopt[i].dflt))
              && HPARM->elideMultipleNonExistFloatDefault))
        && (!(2 == hopt[i].kind && airTypeOther == hopt[i].type
              && HPARM->elideSingleOtherDefault))
        && (!(2 == hopt[i].kind && airTypeString == hopt[i].type
              && HPARM->elideSingleEmptyStringDefault && 0 == airStrlen(hopt[i].dflt)))
        && (!((3 == hopt[i].kind || 5 == hopt[i].kind) && airTypeString == hopt[i].type
              && HPARM->elideMultipleEmptyStringDefault
              && 0 == airStrlen(hopt[i].dflt)))) {
      /* if there are newlines in the info, then we want to clarify the
         default by printing it on its own line */
      if (hopt[i].info && strchr(hopt[i].info, '\n')) {
        strcat(buff, "\n ");
      } else {
        strcat(buff, "; ");
      }
      strcat(buff, "default:\t");
      strcpy(tmpS, hopt[i].dflt);
      airStrtrans(tmpS, ' ', '\t');
      strcat(buff, "\"");
      strcat(buff, tmpS);
      strcat(buff, "\"");
    }
    _hestPrintStr(f, maxlen + 3, maxlen + 3, HPARM->columns, buff, AIR_FALSE);
  }
  if (hparm) {
    hestParmFree(hparm);
  }

  return;
}

#undef HPARM
