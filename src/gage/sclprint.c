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

#include "gage.h"
#include "private.h"

void
_gageSclPrint_query(unsigned int query) {
  unsigned int q;

  fprintf(stderr, "query = %u ...\n", query);
  q = GAGE_SCL_MAX+1;
  do {
    q--;
    if ((1<<q) & query) {
      fprintf(stderr, "    %3d: %s\n", q, airEnumStr(gageScl, q));
    }
  } while (q);
}

void
_gageSclIv3Print(gageContext *ctx, gagePerVolume *pvl) {
  gage_t *iv3;
  int i, fd;

  iv3 = pvl->iv3;
  fd = ctx->fd;
  fprintf(stderr, "iv3[]:\n");
  switch(fd) {
  case 2:
    fprintf(stderr, "% 10.4f   % 10.4f\n", (float)iv3[6], (float)iv3[7]);
    fprintf(stderr, "   % 10.4f   % 10.4f\n\n", (float)iv3[4], (float)iv3[5]);
    fprintf(stderr, "% 10.4f   % 10.4f\n", (float)iv3[2], (float)iv3[3]);
    fprintf(stderr, "   % 10.4f   % 10.4f\n", (float)iv3[0], (float)iv3[1]);
    break;
  case 4:
    for (i=3; i>=0; i--) {
      fprintf(stderr, "% 10.4f   % 10.4f   % 10.4f   % 10.4f\n", 
	      (float)iv3[12+16*i], (float)iv3[13+16*i], 
	      (float)iv3[14+16*i], (float)iv3[15+16*i]);
      fprintf(stderr, "   % 10.4f  %c% 10.4f   % 10.4f%c   % 10.4f\n", 
	      (float)iv3[ 8+16*i], (i==1||i==2)?'\\':' ',
	      (float)iv3[ 9+16*i], (float)iv3[10+16*i], (i==1||i==2)?'\\':' ',
	      (float)iv3[11+16*i]);
      fprintf(stderr, "      % 10.4f  %c% 10.4f   % 10.4f%c   % 10.4f\n", 
	      (float)iv3[ 4+16*i], (i==1||i==2)?'\\':' ',
	      (float)iv3[ 5+16*i], (float)iv3[ 6+16*i], (i==1||i==2)?'\\':' ',
	      (float)iv3[ 7+16*i]);
      fprintf(stderr, "         % 10.4f   % 10.4f   % 10.4f   % 10.4f\n", 
	      (float)iv3[ 0+16*i], (float)iv3[ 1+16*i],
	      (float)iv3[ 2+16*i], (float)iv3[ 3+16*i]);
      if (i) fprintf(stderr, "\n");
    }
    break;
  default:
    for (i=0; i<fd*fd*fd; i++) {
      fprintf(stderr, "  iv3[% 3d,% 3d,% 3d] = % 10.4f\n",
	      i%fd, (i/fd)%fd, i/(fd*fd), (float)iv3[i]);
    }
    break;
  }
  return;
}
