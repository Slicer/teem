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

#include "mite.h"

int 
miteThreadBegin(miteThreadInfo **mttP, miteRenderInfo *mrr,
		miteUserInfo *muu, int whichThread) {

  (*mttP) = mrr->tt[whichThread];
  if (!whichThread) {
    /* this is the first thread- it just points to the parent gageContext */
    (*mttP)->gtx = mrr->gtx0;
  } else {
    /* we have to generate a new gageContext */
    (*mttP)->gtx = gageContextCopy(mrr->gtx0);
  }
  (*mttP)->san = (*mttP)->gtx->pvl[0]->ans;

  (*mttP)->thrid = whichThread;
  return 0;
}

int 
miteThreadEnd(miteThreadInfo *mtt, miteRenderInfo *mrr,
	      miteUserInfo *muu) {

  return 0;
}

