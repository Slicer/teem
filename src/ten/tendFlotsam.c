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

#include "ten.h"
#include "tenPrivate.h"

/*
******** tendCmdList[]
**
** NULL-terminated array of unrrduCmd pointers, as ordered by
** TEN_MAP macro
*/
unrrduCmd *
tendCmdList[] = {
  TEND_MAP(TEND_LIST)
  NULL
};

/*
******** tendUsage
**
** prints out a little banner, and a listing of all available commands
** with their one-line descriptions
*/
void
tendUsage(char *me, hestParm *hparm) {
  int i, maxlen, len, c;
  char buff[AIR_STRLEN_LARGE], fmt[AIR_STRLEN_LARGE];

  maxlen = 0;
  for (i=0; tendCmdList[i]; i++) {
    maxlen = AIR_MAX(maxlen, strlen(tendCmdList[i]->name));
  }

  sprintf(buff, "--- Diffusion Tensor Processing and Analysis ---");
  sprintf(fmt, "%%%ds\n",
	  (int)((hparm->columns-strlen(buff))/2 + strlen(buff) - 1));
  fprintf(stderr, fmt, buff);
  
  for (i=0; tendCmdList[i]; i++) {
    len = strlen(tendCmdList[i]->name);
    strcpy(buff, "");
    for (c=len; c<maxlen; c++)
      strcat(buff, " ");
    strcat(buff, me);
    strcat(buff, " ");
    strcat(buff, tendCmdList[i]->name);
    strcat(buff, " ... ");
    len = strlen(buff);
    fprintf(stderr, "%s", buff);
    _hestPrintStr(stderr, len, len, hparm->columns,
		  tendCmdList[i]->info, AIR_FALSE);
  }
}

