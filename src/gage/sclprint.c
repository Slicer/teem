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
_gageSclPrint_iv3(gageSclContext *ctx) {
  GT *iv3;
  int i;

  iv3 = ctx->iv3;
  printf("iv3[]:\n");
  switch(ctx->fd) {
  case 2:
    printf("% 10.4f   % 10.4f\n", (float)iv3[6], (float)iv3[7]);
    printf("   % 10.4f   % 10.4f\n\n", (float)iv3[4], (float)iv3[5]);
    printf("% 10.4f   % 10.4f\n", (float)iv3[2], (float)iv3[3]);
    printf("   % 10.4f   % 10.4f\n", (float)iv3[0], (float)iv3[1]);
    break;
  case 4:
    for (i=3; i>=0; i--) {
      printf("% 10.4f   % 10.4f   % 10.4f   % 10.4f\n", 
	     (float)iv3[12+16*i], (float)iv3[13+16*i], 
	     (float)iv3[14+16*i], (float)iv3[15+16*i]);
      printf("   % 10.4f  %c% 10.4f   % 10.4f%c   % 10.4f\n", 
	     (float)iv3[ 8+16*i], (i==1||i==2)?'\\':' ',
	     (float)iv3[ 9+16*i], (float)iv3[10+16*i], (i==1||i==2)?'\\':' ',
	     (float)iv3[11+16*i]);
      printf("      % 10.4f  %c% 10.4f   % 10.4f%c   % 10.4f\n", 
	     (float)iv3[ 4+16*i], (i==1||i==2)?'\\':' ',
	     (float)iv3[ 5+16*i], (float)iv3[ 6+16*i], (i==1||i==2)?'\\':' ',
	     (float)iv3[ 7+16*i]);
      printf("         % 10.4f   % 10.4f   % 10.4f   % 10.4f\n", 
	     (float)iv3[ 0+16*i], (float)iv3[ 1+16*i],
	     (float)iv3[ 2+16*i], (float)iv3[ 3+16*i]);
      if (i) printf("\n");
    }
    break;
  default:
    for (i=0; i<ctx->fd*ctx->fd*ctx->fd; i++) {
      printf("  iv3[% 5d] = % 10.4f\n", i, (float)iv3[i]);
    }
    break;
  }
  return;
}

#define PRINT2(N,C)                                     \
   fw = fw##N##C;                                       \
   printf("   -" #N "->% 15.7f   % 15.7f\n",            \
	  (float)fw[0], (float)fw[1])
#define PRINT4(N,C)                                              \
   fw = fw##N##C;                                                \
   printf("   -" #N "->% 15.7f   % 15.7f   % 15.7f   % 15.7f\n", \
	  (float)fw[0], (float)fw[1], (float)fw[2], (float)fw[3])
#define PRINTALL(HOW,C)                                 \
   HOW(00,C);                                           \
   if (doD1 && six) {HOW(10,C);}                        \
   if (doD1) {HOW(11,C);}                               \
   if (doD2 && six) {HOW(20,C);}                        \
   if (doD2 && six) {HOW(21,C);}                        \
   if (doD2) {HOW(22,C);}

void
_gageSclPrint_fslw(gageSclContext *ctx, int doD1, int doD2) {
  int fd, six;
  GT *fslx, *fsly, *fslz, *fw,
    *fw000, *fw001, *fw002, 
    *fw100, *fw101, *fw102, 
    *fw110, *fw111, *fw112, 
    *fw200, *fw201, *fw202, 
    *fw210, *fw211, *fw212, 
    *fw220, *fw221, *fw222;

  /* float *p; */

  fd = ctx->fd;
  fslx = ctx->fsl + fd*0;  fsly = ctx->fsl + fd*1;  fslz = ctx->fsl + fd*2;
  fw000 = ctx->fw00 + fd*0; fw001 = ctx->fw00 + fd*1; fw002 = ctx->fw00 + fd*2;
  fw100 = ctx->fw10 + fd*0; fw101 = ctx->fw10 + fd*1; fw102 = ctx->fw10 + fd*2;
  fw110 = ctx->fw11 + fd*0; fw111 = ctx->fw11 + fd*1; fw112 = ctx->fw11 + fd*2;
  fw200 = ctx->fw20 + fd*0; fw201 = ctx->fw20 + fd*1; fw202 = ctx->fw20 + fd*2;
  fw210 = ctx->fw21 + fd*0; fw211 = ctx->fw21 + fd*1; fw212 = ctx->fw21 + fd*2;
  fw220 = ctx->fw22 + fd*0; fw221 = ctx->fw22 + fd*1; fw222 = ctx->fw22 + fd*2;
  six = !ctx->k3pack;

  printf("fsl -> fw: \n");
  switch(fd) {
  case 2:
    printf("x[]: % 15.7f   % 15.7f\n",
	   (float)fslx[0], (float)fslx[1]);
    PRINTALL(PRINT2, 0);
    printf("y[]: % 15.7f   % 15.7f\n",
	   (float)fsly[0], (float)fsly[1]);
    PRINTALL(PRINT2, 1);
    printf("z[]: % 15.7f   % 15.7f\n",
	   (float)fslz[0], (float)fslz[1]);
    PRINTALL(PRINT2, 2);
    break;
  case 4:
    /*
      printf(" ----------------- \n");
      p = ctx->fw00 + 4*0;
      printf("% 15.7f|% 15.7f|% 15.7f|% 15.7f\n", p[0], p[1], p[2], p[3]);
      p = ctx->fw00 + 4*1;
      printf("% 15.7f|% 15.7f|% 15.7f|% 15.7f\n", p[0], p[1], p[2], p[3]);
      p = ctx->fw00 + 4*2;
      printf("% 15.7f|% 15.7f|% 15.7f|% 15.7f\n", p[0], p[1], p[2], p[3]);
      printf(" ----------------- \n");
      p = fw000;
      printf("% 15.7f|% 15.7f|% 15.7f|% 15.7f\n", p[0], p[1], p[2], p[3]);
      p = fw001;
      printf("% 15.7f|% 15.7f|% 15.7f|% 15.7f\n", p[0], p[1], p[2], p[3]);
      p = fw002;
      printf("% 15.7f|% 15.7f|% 15.7f|% 15.7f\n", p[0], p[1], p[2], p[3]);
    */
    printf("x[]: % 15.7f  % 15.7f  % 15.7f  % 15.7f\n", 
	   (float)fslx[0], (float)fslx[1], (float)fslx[2], (float)fslx[3]);
    PRINTALL(PRINT4, 0);
    printf("y[]: % 15.7f  % 15.7f  % 15.7f  % 15.7f\n", 
	   (float)fsly[0], (float)fsly[1], (float)fsly[2], (float)fsly[3]);
    PRINTALL(PRINT4, 1);
    printf("z[]: % 15.7f  % 15.7f  % 15.7f  % 15.7f\n", 
	   (float)fslz[0], (float)fslz[1], (float)fslz[2], (float)fslz[3]);
    PRINTALL(PRINT4, 2);
    break;
  default:
    /*
    printf("x[]:\n");
    for (i=0; i<=fd-1; i++) {
      printf("     % 5d : % 15.7f -1,2,3-> % 15.7f, % 15.7f, % 15.7f\n",
	     i, (float)fslx[i],
	     (float)fw0x[i], (float)fw1x[i], (float)fw2x[i]);
    }
    printf("y[]:\n");
    for (i=0; i<=ctx->fd-1; i++) {
      printf("     % 5d : % 15.7f -1,2,3-> % 15.7f, % 15.7f, % 15.7f\n",
	     i, (float)fsly[i],
	     (float)fw0y[i], (float)fw1y[i], (float)fw2y[i]);
    }
    printf("z[]:\n");
    for (i=0; i<=ctx->fd-1; i++) {
      printf("     % 5d : % 15.7f -1,2,3-> % 15.7f, % 15.7f, % 15.7f\n",
	     i, (float)fslz[i],
	     (float)fw0z[i], (float)fw1z[i], (float)fw2z[i]);
    }
    */
    break;
  }
  return;
}

