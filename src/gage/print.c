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

#define PRINT2(N,C)                                     \
   fw = fw##N##C;                                       \
   printf("   -" #N "->% 15.7f   % 15.7f\n",            \
	  (float)fw[0], (float)fw[1])
#define PRINT4(N,C)                                              \
   fw = fw##N##C;                                                \
   printf("   -" #N "->% 15.7f   % 15.7f   % 15.7f   % 15.7f\n", \
	  (float)fw[0], (float)fw[1], (float)fw[2], (float)fw[3])
#define PRINTALL(HOW,C)                                 \
   if (fw000) { HOW(00,C); }                            \
   if (fw100) { HOW(10,C); }                            \
   if (fw110) { HOW(11,C); }                            \
   if (fw200) { HOW(20,C); }                            \
   if (fw210) { HOW(21,C); }                            \
   if (fw220) { HOW(22,C); }

void
_gagePrint_fslw(gageContext *ctx, int doD1, int doD2) {
  int i, fd;
  gage_t *fslx, *fsly, *fslz, *fw,
    *fw000, *fw001, *fw002, 
    *fw100, *fw101, *fw102, 
    *fw110, *fw111, *fw112, 
    *fw200, *fw201, *fw202, 
    *fw210, *fw211, *fw212, 
    *fw220, *fw221, *fw222;

  /* float *p; */

  fd = ctx->fd;
  fslx = ctx->fsl + fd*0;
  fsly = ctx->fsl + fd*1;
  fslz = ctx->fsl + fd*2;
  fw000 = ctx->fw[gageKernel00] + fd*0;
  fw001 = ctx->fw[gageKernel00] + fd*1;
  fw002 = ctx->fw[gageKernel00] + fd*2;
  fw100 = ctx->fw[gageKernel10] + fd*0;
  fw101 = ctx->fw[gageKernel10] + fd*1;
  fw102 = ctx->fw[gageKernel10] + fd*2;
  fw110 = ctx->fw[gageKernel11] + fd*0;
  fw111 = ctx->fw[gageKernel11] + fd*1;
  fw112 = ctx->fw[gageKernel11] + fd*2;
  fw200 = ctx->fw[gageKernel20] + fd*0;
  fw201 = ctx->fw[gageKernel20] + fd*1;
  fw202 = ctx->fw[gageKernel20] + fd*2;
  fw210 = ctx->fw[gageKernel21] + fd*0;
  fw211 = ctx->fw[gageKernel21] + fd*1;
  fw212 = ctx->fw[gageKernel21] + fd*2;
  fw220 = ctx->fw[gageKernel22] + fd*0;
  fw221 = ctx->fw[gageKernel22] + fd*1;
  fw222 = ctx->fw[gageKernel22] + fd*2;

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
    printf("x[]:\n");
    for (i=0; i<fd; i++) {
      printf("     % 5d : % 15.7f -0,1,2-> % 15.7f, % 15.7f, % 15.7f\n",
	     i, (float)fslx[i],
	     (float)fw000[i], (float)fw110[i], (float)fw220[i]);
    }
    printf("y[]:\n");
    for (i=0; i<fd; i++) {
      printf("     % 5d : % 15.7f -0,1,2-> % 15.7f, % 15.7f, % 15.7f\n",
	     i, (float)fsly[i],
	     (float)fw001[i], (float)fw111[i], (float)fw221[i]);
    }
    printf("z[]:\n");
    for (i=0; i<fd; i++) {
      printf("     % 5d : % 15.7f -0,1,2-> % 15.7f, % 15.7f, % 15.7f\n",
	     i, (float)fslz[i],
	     (float)fw002[i], (float)fw112[i], (float)fw222[i]);
    }
    break;
  }
  return;
}

