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

/*
******** nrrdCommentAdd()
**
** Adds a given string to the list of comments
** Leading spaces (' ') and comment chars ('#') are not included.
*/
int /* Biff: nope */
nrrdCommentAdd(Nrrd *nrrd, const char *_str) {
  /* static const char me[] = "nrrdCommentAdd";*/
  char *str;
  unsigned int ii;

  if (!(nrrd && _str)) {
    /* got NULL pointer */
    return 1;
  }
  _str += strspn(_str, " #");
  if (!strlen(_str)) {
    /* we don't bother adding comments with no length */
    return 0;
  }
  if (!strcmp(_str, _nrrdFormatURLLine0) || !strcmp(_str, _nrrdFormatURLLine1)) {
    /* sneaky hack: don't store the format URL comment lines */
    return 0;
  }
  str = airStrdup(_str);
  if (!str) {
    /* couldn't strdup given string */
    return 1;
  }
  /* clean out carraige returns that would screw up reader */
  airOneLinify(str);
  ii = airArrayLenIncr(nrrd->cmtArr, 1);
  if (!nrrd->cmtArr->data) {
    /* couldn't lengthen comment array */
    return 1;
  }
  nrrd->cmt[ii] = str;
  return 0;
}

/*
******** nrrdCommentClear()
**
** blows away comments, but does not blow away the comment airArray
*/
void /* Biff: nope */
nrrdCommentClear(Nrrd *nrrd) {

  if (nrrd) {
    airArrayLenSet(nrrd->cmtArr, 0);
  }
}

/*
******** nrrdCommentCopy()
**
** copies comments from one nrrd to another
** Existing comments in nout are blown away
*/
int /* Biff: nope */
nrrdCommentCopy(Nrrd *nout, const Nrrd *nin) {
  /* static const char me[] = "nrrdCommentCopy"; */
  int E;
  unsigned int numc, ii;

  if (!(nout && nin)) {
    /* got NULL pointer */
    return 1;
  }
  if (nout == nin) {
    /* can't satisfy semantics of copying with nout==nin */
    return 2;
  }
  nrrdCommentClear(nout);
  numc = nin->cmtArr->len;
  E = 0;
  for (ii = 0; ii < numc; ii++) {
    if (!E) E |= nrrdCommentAdd(nout, nin->cmt[ii]);
  }
  if (E) {
    /* couldn't add all comments */
    return 3;
  }
  return 0;
}
