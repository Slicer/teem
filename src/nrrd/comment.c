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

/*
******** nrrdCommentAdd()
**
** Adds a given string to the list of comments
** Leading spaces (' ') and comment chars ('#') are not included.
*/
int
nrrdCommentAdd(Nrrd *nrrd, char *_str, int useBiff) {
  char me[]="nrrdCommentAdd", err[512], *str;
  int i, len;
  
  if (!(nrrd && _str)) {
    if (useBiff) {
      sprintf(err, "%s: got NULL pointer", me);
      biffSet(NRRD, err);
    }
    return 1;
  }
  _str += strspn(_str, " #");
  if (!strlen(_str)) {
    /* we don't bother adding comments with no length */
    return 0;
  }
  str = airStrdup(_str);
  if (!str) {
    if (useBiff) {
      sprintf(err, "%s: couldn't strdup given string", me);
      biffSet(NRRD, err);
    }
    return 1;
  }
  len = strlen(str);
  if (len >= NRRD_STRLEN_COMMENT) {
    if (useBiff) {
      sprintf(err, "%s: cmt's length (%d) exceeds max (%d)", 
	      me, len, NRRD_STRLEN_COMMENT);
      biffSet(NRRD, err);
    }
    return 1;
  }
  /* clean out carraige returns that would screw up reader */
  airOneLinify(str);
  i = airArrayIncrLen(nrrd->cmtArr, 1);
  if (-1 == i) {
    if (useBiff) {
      sprintf(err, "%s: couldn't lengthen comment array", me);
      biffSet(NRRD, err);
    }
    return 1;
  }
  nrrd->cmt[i] = str;
  return 0;
}

/*
******** nrrdCommentClear()
**
** blows away comments, but does not blow away the comment airArray
*/
void
nrrdCommentClear(Nrrd *nrrd) {

  if (nrrd) {
    airArraySetLen(nrrd->cmtArr, 0);
  }
}

/*
******** nrrdCommentScan()
**
** looks through comments of nrrd for something of the form
** " <key> : <val>"
** There can be whitespace anywhere it appears in the format above.
** Returns 1 on error, 0 if okay, BUT DOES NOT USE BIFF for errors.
** This is because an error here is probably not a big deal.
**
** The division between key and val is the first colon which appears.
** "*valP" is set to the first non-whitespace character after the colon
** No (non-temporary) string is allocated; *valP points into data 
** already pointed to by "nrrd"
**
** This function does not use biff.
*/
int
nrrdCommentScan(Nrrd *nrrd, char *key, char **valP) {
  /* char me[]="nrrdCommentScan";  */
  int i;
  char *cmt, *k, *c, *t;

  if (!(nrrd && airStrlen(key) && valP))
    return 1;

  *valP = NULL;
  if (!nrrd->cmt) {
    /* no comments to scan */
    return 1;
  }
  if (strchr(key, ':')) {
    /* key contains colon- would confuse later steps */
    return 1;
  }
  for (i=0; i<=nrrd->cmtArr->len-1; i++) {
    cmt = nrrd->cmt[i];
    /* printf("%s: looking at comment \"%s\"\n", me, cmt); */
    if ((k = strstr(cmt, key)) && (c = strchr(cmt, ':'))) {
      /* is key before colon? */
      if (!( k+strlen(key) <= c ))
	goto nope;
      /* is there only whitespace before the key? */
      t = cmt;
      while (t < k) {
	if (!isspace(*t))
	  goto nope;
	t++;
      }
      /* is there only whitespace after the key? */
      t = k+strlen(key);
      while (t < c) {
	if (!isspace(*t))
	  goto nope;
	t++;
      }
      /* is there something after the colon? */
      t = c+1;
      while (isspace(*t)) {
	t++;
      }
      if (!*t)
	goto nope;
      /* t now points to beginning of "value" string; we're done */
      *valP = t;
      /* printf("%s: found \"%s\"\n", me, t); */
      break;
    }
  nope:
    *valP = NULL;
  }
  if (*valP)
    return 0;
  else 
    return 1;
}

int
nrrdCommentCopy(Nrrd *nout, Nrrd *nin, int useBiff) {
  char me[]="nrrdCommentCopy", err[512];
  int numc, i, E;

  if (!(nout && nin)) {
    if (useBiff) {
      sprintf(err, "%s: got NULL pointer", me);
      biffSet(NRRD, err);
    }
    return 1;
  }
  nrrdCommentClear(nout);
  numc = nin->cmtArr->len;
  E = 0;
  for (i=0; i<=numc-1; i++) {
    if (!E) E |= nrrdCommentAdd(nout, nin->cmt[i], useBiff);
  }
  if (E) {
    if (useBiff) {
      sprintf(err, "%s: couldn't add all comments", me);
      biffSet(NRRD, err);
    }
    return 1;
  }
  return 0;
}


