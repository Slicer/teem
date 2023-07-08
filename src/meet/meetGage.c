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

#include "meet.h"

static gageKind *
_meetGageKindParse(const char *_str, int constOnly) {
  char *str;
  gageKind *ret;

  if (!_str) {
    return NULL;
  }
  str = airToLower(airStrdup(_str));
  if (!str) {
    return NULL;
  }
  if (!strcmp(gageKindScl->name, str)) {
    ret = gageKindScl;
  } else if (!strcmp(gageKind2Vec->name, str)) {
    ret = gageKind2Vec;
  } else if (!strcmp(gageKindVec->name, str)) {
    ret = gageKindVec;
  } else if (!strcmp(tenGageKind->name, str)) {
    ret = tenGageKind;
  } else if (!constOnly && !strcmp(TEN_DWI_GAGE_KIND_NAME, str)) {
    ret = tenDwiGageKindNew();
  } else {
    ret = NULL;
  }
  airFree(str);
  return ret;
}

gageKind * /* Biff: nope */
meetGageKindParse(const char *_str) {

  return _meetGageKindParse(_str, AIR_FALSE);
}

const gageKind * /* Biff: nope */
meetConstGageKindParse(const char *_str) {

  return _meetGageKindParse(_str, AIR_TRUE);
}

/*
** same as _meetHestGageKindParse below but without the DWI kind,
** which isn't const
*/
static int
_meetHestConstGageKindParse(void *ptr, const char *str, char err[AIR_STRLEN_HUGE + 1]) {
  static const char me[] = "_meetHestGageConstKindParse";
  const gageKind **kindP;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  /* of course, the const correctness goes out the window with all
     the casting that's necessary with hest ... */
  kindP = (const gageKind **)ptr;
  *kindP = meetConstGageKindParse(str);
  if (!*kindP) {
    sprintf(err, "%s: \"%s\" not \"%s\", \"%s\", \"%s\", or \"%s\"", me, str,
            gageKindScl->name, gageKind2Vec->name, gageKindVec->name, tenGageKind->name);
    return 1;
  }

  return 0;
}

static int
_meetHestGageKindParse(void *ptr, const char *str, char err[AIR_STRLEN_HUGE + 1]) {
  static const char me[] = "_meetHestGageKindParse";
  gageKind **kindP;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  kindP = (gageKind **)ptr;
  *kindP = meetGageKindParse(str);
  if (!*kindP) {
    sprintf(err, "%s: \"%s\" not \"%s\", \"%s\", \"%s\", \"%s\", or \"%s\"", me, str,
            gageKindScl->name, gageKind2Vec->name, gageKindVec->name, tenGageKind->name,
            TEN_DWI_GAGE_KIND_NAME);
    return 1;
  }

  return 0;
}

static void *
_meetHestGageKindDestroy(void *ptr) {
  gageKind *kind;

  if (ptr) {
    kind = AIR_CAST(gageKind *, ptr);
    if (!strcmp(TEN_DWI_GAGE_KIND_NAME, kind->name)) {
      tenDwiGageKindNix(kind);
    }
  }
  return NULL;
}

static const hestCB _meetHestGageKind
  = {sizeof(gageKind *), "gageKind", _meetHestGageKindParse, _meetHestGageKindDestroy};

static const hestCB _meetHestConstGageKind = {sizeof(gageKind *), "gageKind",
                                              _meetHestConstGageKindParse, NULL};

/*
******** meetHestGageKind
**
** This provides a uniform way to parse gageKinds from the command-line
*/
const hestCB *const meetHestGageKind = &_meetHestGageKind;
const hestCB *const meetHestConstGageKind = &_meetHestConstGageKind;
