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
_hestPrintStr(FILE *f, int indent, int already, int width, char *_str) {
  char *str, *ws, *last;
  int nw, w, pos, s;

  str = airStrdup(_str);
  nw = airStrntok(str, " ");
  pos = already;
  for (w=0; w<=nw-1; w++) {
    ws = airStrtok(!w ? str : NULL, " ", &last);
    airOneLinify(ws);
    if (pos + 1 + strlen(ws) <= width) {
      fprintf(f, "%s%s", !w ? "" : " ", ws);
      pos += 1 + strlen(ws);
    }
    else {
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
    _hestPrintStr(file, 0, strlen(argv0) + 2, parm->columns, info);
  }
  parm = !_parm ? hestParmNix(parm) : NULL;
}

void
hestUsage(FILE *f, hestOpt *opt, char *argv0, hestParm *_parm) {
  int i, numOpts;
  char buff[2*AIR_STRLEN_HUGE], tmpS[AIR_STRLEN_HUGE];
  hestParm *parm;

  parm = !_parm ? hestParmNew() : _parm;

  if (_hestPanic(opt, NULL, parm)) {
    /* we can't continue; the opt array is botched */
    parm = !_parm ? hestParmNix(parm) : NULL;
    return;
  }
    
  numOpts = _hestNumOpts(opt);
  fprintf(f, "\n");
  strcpy(buff, "Usage: ");
  strcat(buff, argv0 ? argv0 : "");
  if (parm && parm->respFileEnable) {
    sprintf(tmpS, " [%cfile ...]", parm->respFileFlag);
    strcat(buff, tmpS);
  }
  for (i=0; i<=numOpts-1; i++) {
    strcat(buff, " ");
    if (opt[i].flag)
      strcat(buff, "[");
    SETBUFF(buff, opt[i]);
    if (opt[i].flag)
      strcat(buff, "]");
  }

  _hestPrintStr(f, 3, 0, parm->columns, buff);

  parm = !_parm ? hestParmNix(parm) : NULL;
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
    parm = !_parm ? hestParmNix(parm) : NULL;
    return;
  }
    
  numOpts = _hestNumOpts(opt);

  maxlen = 0;
  if (numOpts)
    fprintf(f, "\n");
  for (i=0; i<=numOpts-1; i++) {
    strcpy(buff, "");
    SETBUFF(buff, opt[i]);
    maxlen = AIR_MAX(strlen(buff), maxlen);
  }
  for (i=0; i<=numOpts-1; i++) {
    strcpy(buff, "");
    SETBUFF(buff, opt[i]);
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
    _hestPrintStr(f, maxlen + 3, maxlen + 3, parm->columns, buff);
  }
  parm = !_parm ? hestParmNix(parm) : NULL;

  return;
}

