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
      strcat(B, "\t...");
    strcat(B, ">");
  }
  if (!O->min && max)
    strcat(B, "]");
}

/* early version of _hestSetBuff() function */
#define SETBUFF(B, O) \
  strcat(B, O.flag ? "-" : ""), \
  strcat(B, O.flag ? O.flag : ""), \
  strcat(B, O.flag && (O.min || _hestMax(O.max)) ? "\t" : ""), \
  strcat(B, !O.min && _hestMax(O.max) ? "[" : ""), \
  strcat(B, O.min || _hestMax(O.max) ? "<" : ""), \
  strcat(B, O.min || _hestMax(O.max) ? O.name : ""), \
  strcat(B, (O.min < _hestMax(O.max) && (_hestMax(O.max) > 1)) ? " ...": ""), \
  strcat(B, O.min || _hestMax(O.max) ? ">" : ""), \
  strcat(B, !O.min && _hestMax(O.max) ? "]" : "");

/*
** _hestPrintStr()
**
** not a useful function.  Do not use.
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
    /* we used airStrtok() to delimit words on spaces ... */
    ws = airStrtok(!wrd ? str : NULL, " ", &last);
    /* ... but then convert tabs to spaces */
    airStrtrans(ws, '\t', ' ');
    if (pos + 1 + strlen(ws) <= width - !!bslash) {
      /* if this word would still fit on the current line */
      if (wrd) fprintf(f, " ");
      fprintf(f, "%s", ws);
      pos += 1 + strlen(ws);
    }
    else {
      /* else we start a new line and print the indent */
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
    /* if the last character of the word was a newline, then indent */
    if ('\n' == ws[strlen(ws)-1]) {
      /* we indent one character less than before, because the stupid
	 if (wrd) fprintf(f, " "); line above will print a space for us */
      for (s=0; s<=indent-2; s++) {
	fprintf(f, " ");
      }
      pos = indent;
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
    sprintf(tmpS, " [%cfile\t...]", parm->respFileFlag);
    strcat(buff, tmpS);
  }
  for (i=0; i<=numOpts-1; i++) {
    strcat(buff, " ");
    if (1 == opt[i].kind || (opt[i].flag && opt[i].dflt))
      strcat(buff, "[");
    _hestSetBuff(buff, opt + i, parm, AIR_FALSE);
    if (1 == opt[i].kind || (opt[i].flag && opt[i].dflt))
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
    _hestSetBuff(buff, opt + i, parm, AIR_TRUE);
    maxlen = AIR_MAX(strlen(buff), maxlen);
  }
  if (parm && parm->respFileEnable) {
    sprintf(buff, "%cfile\t...", parm->respFileFlag);
    len = strlen(buff);
    for (j=len; j<=maxlen-1; j++)
      fprintf(f, " ");
    fprintf(f, "%s = ", buff);
    strcpy(buff, "response file(s) containing command-line arguments");
    _hestPrintStr(f, maxlen + 3, maxlen + 3, parm->columns, buff, AIR_FALSE);
  }
  for (i=0; i<=numOpts-1; i++) {
    strcpy(buff, "");
    _hestSetBuff(buff, opt + i, parm, AIR_TRUE);
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
    if ((opt[i].min || _hestMax(opt[i].max))
	&& (!( 2 == opt[i].kind
	       && airTypeEnum == opt[i].type 
	       && parm->elideSingleEnumType )) 
	&& (!( 2 == opt[i].kind
	       && airTypeOther == opt[i].type 
	       && parm->elideSingleOtherType )) 
	) {
      /* if there are newlines in the info, then we want to clarify the
         type by printing it on its own line */
      if (opt[i].info && strchr(opt[i].info, '\n')) {
	strcat(buff, "\n ");
      }
      else {
	strcat(buff, " ");
      }
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
      sprintf(tmpS, "%s%s", 
	      (airTypeEnum == opt[i].type
	       ? opt[i].enm->name
	       : (airTypeOther == opt[i].type
		  ? opt[i].CB->type
		  : airTypeStr[opt[i].type])),
	      _hestMax(opt[i].max) > 1 ? "s" : "");
      strcat(buff, tmpS);
      strcat(buff, ")");
    }
    /*
    fprintf(stderr, "!%s: parm->elideSingleOtherDefault = %d\n",
	    "hestGlossary", parm->elideSingleOtherDefault);
    */
    if (opt[i].dflt 
	&& (opt[i].min || _hestMax(opt[i].max))
	&& (!( 2 == opt[i].kind
	       && (airTypeFloat == opt[i].type || airTypeDouble == opt[i].type)
	       && !AIR_EXISTS(airAtod(opt[i].dflt)) 
	       && parm->elideSingleNonExistFloatDefault ))
	&& (!( 2 == opt[i].kind
	       && airTypeOther == opt[i].type
	       && parm->elideSingleOtherDefault ))
	) {
      /* if there are newlines in the info, then we want to clarify the
	 default by printing it on its own line */
      if (opt[i].info && strchr(opt[i].info, '\n')) {
	strcat(buff, "\n ");
      }
      else {
	strcat(buff, "; ");
      }
      strcat(buff, "default:\t");
      strcpy(tmpS, opt[i].dflt);
      airStrtrans(tmpS, ' ', '\t');
      strcat(buff, "\"");
      strcat(buff, tmpS);
      strcat(buff, "\"");
    }
    _hestPrintStr(f, maxlen + 3, maxlen + 3, parm->columns, buff, AIR_FALSE);
  }
  parm = !_parm ? hestParmFree(parm) : NULL;

  return;
}

