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

#include "hest.h"
#include "private.h"

/*
** don't ask
*/
void
_hestSetBuff(char *B, hestOpt *O, hestParm *P, int showlong) {
  char copy[AIR_STRLEN_HUGE], *sep;
  int max, len;

  max = _hestMax(O->max);
  if (O->flag) {
    strcpy(copy, O->flag);
    if ((sep = strchr(copy, P->multiFlagSep))) {
      *sep = 0;
      strcat(B, "-");
      strcat(B, copy);
      if (showlong) {
	len = strlen(B);
	B[len] = P->multiFlagSep;
	B[len+1] = '\0';
	strcat(B, "--");
	strcat(B, sep+1);
      }
    }
    else {
      strcat(B, "-");
      strcat(B, O->flag);
    }
    if (O->min || max)
      strcat(B, "\t");
  }
  if (!O->min && max)
    strcat(B, "[");
  if (O->min || max) {
    strcat(B, "<");
    strcat(B, O->name);
    if (O->min < max && max > 1)
      strcat(B, "...");
    strcat(B, ">");
  }
  if (!O->min && max)
    strcat(B, "]");
}

#define SETBUFF(B, O) \
  strcat(B, O.flag ? "-" : ""), \
  strcat(B, O.flag ? O.flag : ""), \
  strcat(B, O.flag && (O.min || _hestMax(O.max)) ? "\t" : ""), \
  strcat(B, !O.min && _hestMax(O.max) ? "[" : ""), \
  strcat(B, O.min || _hestMax(O.max) ? "<" : ""), \
  strcat(B, O.min || _hestMax(O.max) ? O.name : ""), \
  strcat(B, (O.min < _hestMax(O.max) && (_hestMax(O.max) > 1)) ? "..." : ""), \
  strcat(B, O.min || _hestMax(O.max) ? ">" : ""), \
  strcat(B, !O.min && _hestMax(O.max) ? "]" : "");

/*
** _hestPrintStr()
**
** not a generally useful function.  Assumes that "str" has already
** been run through airOneLinify, except for tabs ('\t') which should
** be treated like a "&nbsp;" in HTML (rendered as space but not
** delimiting the word in which it appears), and then prints out the
** strings to FILE *f, breaking on word boundaries, but with left 
** indenting of "indent" spaces, except for the first line, which
** starts not with a new line but with "already" chars printed.
*/
void
_hestPrintStr(FILE *f, int indent, int already, int width, char *_str,
	      int bslash) {
  char *str, *ws, *last;
  int nwrd, wrd, pos, s;

  str = airStrdup(_str);
  nwrd = airStrntok(str, " ");
  pos = already;
  for (wrd=0; wrd<=nwrd-1; wrd++) {
    ws = airStrtok(!wrd ? str : NULL, " ", &last);
    airOneLinify(ws);
    if (pos + 1 + strlen(ws) <= width) {
      fprintf(f, "%s%s", !wrd ? "" : " ", ws);
      pos += 1 + strlen(ws);
    }
    else {
      if (bslash) {
	fprintf(f, " \\");
      }
      fprintf(f, "\n");
      for (s=0; s<=indent-1; s++) {
	fprintf(f, " ");
      }
      fprintf(f, "%s", ws); 
      pos = indent + strlen(ws);
    }
  }
  fprintf(f, "\n");
  free(str);
}

void
hestInfo(FILE *file, char *argv0, char *info, hestParm *_parm) {
  hestParm *parm;

  parm = !_parm ? hestParmNew() : _parm;
  if (info) {
    fprintf(file, "\n%s: ", argv0);
    _hestPrintStr(file, 0, strlen(argv0) + 2, parm->columns, info, AIR_FALSE);
  }
  parm = !_parm ? hestParmFree(parm) : NULL;
}

void
hestUsage(FILE *f, hestOpt *opt, char *argv0, hestParm *_parm) {
  int i, numOpts;
  char buff[2*AIR_STRLEN_HUGE], tmpS[AIR_STRLEN_HUGE];
  hestParm *parm;

  parm = !_parm ? hestParmNew() : _parm;

  if (_hestPanic(opt, NULL, parm)) {
    /* we can't continue; the opt array is botched */
    parm = !_parm ? hestParmFree(parm) : NULL;
    return;
  }
    
  numOpts = _hestNumOpts(opt);
  fprintf(f, "\n");
  strcpy(buff, "Usage: ");
  strcat(buff, argv0 ? argv0 : "");
  if (parm && parm->respFileEnable) {
    sprintf(tmpS, " [%cfile...]", parm->respFileFlag);
    strcat(buff, tmpS);
  }
  for (i=0; i<=numOpts-1; i++) {
    strcat(buff, " ");
    if (opt[i].flag && opt[i].dflt)
      strcat(buff, "[");
    _hestSetBuff(buff, opt + i, _parm, AIR_FALSE);
    if (opt[i].flag && opt[i].dflt)
      strcat(buff, "]");
  }

  _hestPrintStr(f, strlen("Usage: "), 0, parm->columns, buff, AIR_TRUE);

  parm = !_parm ? hestParmFree(parm) : NULL;
  return;
}

void
hestGlossary(FILE *f, hestOpt *opt, hestParm *_parm) {
  int i, j, len, maxlen, numOpts;
  char buff[2*AIR_STRLEN_HUGE], tmpS[AIR_STRLEN_HUGE];
  hestParm *parm;

  parm = !_parm ? hestParmNew() : _parm;

  if (_hestPanic(opt, NULL, parm)) {
    /* we can't continue; the opt array is botched */
    parm = !_parm ? hestParmFree(parm) : NULL;
    return;
  }
    
  numOpts = _hestNumOpts(opt);

  maxlen = 0;
  if (numOpts)
    fprintf(f, "\n");
  for (i=0; i<=numOpts-1; i++) {
    strcpy(buff, "");
    _hestSetBuff(buff, opt + i, _parm, AIR_TRUE);
    maxlen = AIR_MAX(strlen(buff), maxlen);
  }
  if (parm && parm->respFileEnable) {
    sprintf(buff, "%cfile...", parm->respFileFlag);
    len = strlen(buff);
    for (j=len; j<=maxlen-1; j++)
      fprintf(f, " ");
    fprintf(f, "%s = ", buff);
    strcpy(buff, "response file(s) containing command-line arguments");
    _hestPrintStr(f, maxlen + 3, maxlen + 3, parm->columns, buff, AIR_FALSE);
  }
  for (i=0; i<=numOpts-1; i++) {
    strcpy(buff, "");
    _hestSetBuff(buff, opt + i, _parm, AIR_TRUE);
    airOneLinify(buff);
    len = strlen(buff);
    for (j=len; j<=maxlen-1; j++)
      fprintf(f, " ");
    fprintf(f, "%s = ", buff);
    /* we've now printed some spaces, and the flag, and " = ", all of which
       should bring us to position   */
    strcpy(buff, "");
    if (opt[i].info)
      strcat(buff, opt[i].info);
    if (opt[i].min || _hestMax(opt[i].max)) {
      if (opt[i].info)
	strcat(buff, " ");
      strcat(buff, "(");
      if (opt[i].min == 0 && _hestMax(opt[i].max) == 1) {
	strcat(buff, "optional\t");
      }
      else {
	if (opt[i].min == _hestMax(opt[i].max) && _hestMax(opt[i].max) > 1) {
	  sprintf(tmpS, "%d\t", _hestMax(opt[i].max));
	  strcat(buff, tmpS);
	}
	else if (opt[i].min < _hestMax(opt[i].max)) {
	  if (-1 == opt[i].max) {
	    sprintf(tmpS, "%d\tor\tmore\t", opt[i].min);
	  }
	  else {
	    sprintf(tmpS, "%d..%d\t", opt[i].min, _hestMax(opt[i].max));
	  }
	  strcat(buff, tmpS);
	}
      }
      sprintf(tmpS, "%s%s", airTypeStr[opt[i].type],
	      _hestMax(opt[i].max) > 1 ? "s" : "");
      strcat(buff, tmpS);
      strcat(buff, ")");
    }
    if (opt[i].dflt && (opt[i].min || _hestMax(opt[i].max))) {
      strcat(buff, "; default\t");
      strcpy(tmpS, opt[i].dflt);
      airStrtrans(tmpS, ' ', '\t');
      strcat(buff, tmpS);
    }
    _hestPrintStr(f, maxlen + 3, maxlen + 3, parm->columns, buff, AIR_FALSE);
  }
  parm = !_parm ? hestParmFree(parm) : NULL;

  return;
}

