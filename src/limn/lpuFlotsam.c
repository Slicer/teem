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

#include "limn.h"

/*
******** limnPuCmdList[]
**
** NULL-terminated array of unrrduCmd pointers, as ordered by
** LIMN_MAP macro
*/
const unrrduCmd *const limnPuCmdList[] = {LIMN_MAP(LIMN_LIST) NULL};

/*
******** limnPuUsage
**
** prints out a little banner, and a listing of all available commands
** with their one-line descriptions
*/
void
limnPuUsage(const char *me, hestParm *hparm) {
  unsigned int i, maxlen, len, c;
  char buff[AIR_STRLEN_LARGE + 1], fmt[AIR_STRLEN_LARGE + 1];

  maxlen = 0;
  for (i = 0; limnPuCmdList[i]; i++) {
    maxlen = AIR_MAX(maxlen, AIR_UINT(strlen(limnPuCmdList[i]->name)));
  }

  sprintf(buff, "--- LimnPolyData Hacking ---");
  sprintf(fmt, "%%%us\n",
          AIR_UINT((hparm->columns - strlen(buff)) / 2 + strlen(buff) - 1));
  fprintf(stderr, fmt, buff);

  for (i = 0; limnPuCmdList[i]; i++) {
    len = AIR_UINT(strlen(limnPuCmdList[i]->name));
    strcpy(buff, "");
    for (c = len; c < maxlen; c++)
      strcat(buff, " ");
    strcat(buff, me);
    strcat(buff, " ");
    strcat(buff, limnPuCmdList[i]->name);
    strcat(buff, " ... ");
    len = AIR_UINT(strlen(buff));
    fprintf(stderr, "%s", buff);
    _hestPrintStr(stderr, len, len, hparm->columns, limnPuCmdList[i]->info, AIR_FALSE);
  }
}
