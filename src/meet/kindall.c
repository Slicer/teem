/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "meet.h"

int
_meetGageKindParse(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[] = "_meetGageKindParse";
  gageKind **kindP;
  
  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  kindP = (gageKind **)ptr;
  airToLower(str);
  if (!strcmp(gageKindScl->name, str)) {
    *kindP = gageKindScl;
  } else if (!strcmp(gageKindVec->name, str)) {
    *kindP = gageKindVec;
  } else if (!strcmp(tenGageKind->name, str)) {
    *kindP = tenGageKind;
  } else if (!strcmp(TEN_DWI_GAGE_KIND_NAME, str)) {
    *kindP = tenDwiGageKindNew();
  } else {
    sprintf(err, "%s: not \"%s\", \"%s\", \"%s\", or \"%s\"", me,
            gageKindScl->name, gageKindVec->name,
            tenGageKind->name, TEN_DWI_GAGE_KIND_NAME);
    return 1;
  }

  return 0;
}

void *
_meetGageKindDestroy(void *ptr) {
  gageKind *kind;
  
  if (ptr) {
    kind = AIR_CAST(gageKind *, ptr);
    if (!strcmp(TEN_DWI_GAGE_KIND_NAME, kind->name)) {
      tenDwiGageKindNix(kind);
    }
  }
  return NULL;
}

static hestCB
_meetHestGageKind = {
  sizeof(gageKind *),
  "gageKind",
  _meetGageKindParse,
  _meetGageKindDestroy
}; 

/*
******** meetHestGageKind
**
** This provides a uniform way to parse gageKinds from the command-line
*/
hestCB *
meetHestGageKind = &_meetHestGageKind;
