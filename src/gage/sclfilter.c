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
_gageScl3PFilter2(GT *iv3, GT *iv2, GT *iv1,
		  GT *fw0, GT *fw1, GT *fw2,
		  GT *val, GT *gvec, GT *hess,
		  int doD1, int doD2) {

#define D2(a, b) ((a)[0]*(b)[0] + (a)[1]*(b)[1])

  /* fw? + 2*?
       |     |  
       |     +- along which axis (0:x, 1:y, 2:z)
       |
       + what information (0:value, 1:1st deriv, 2:2nd deriv)

     iv3: 3D cube cache of original volume values
     iv2: 2D square cache of intermediate filter results
     iv1: 1D linear cache of intermediate filter results
  */

  /* x0 */
  iv2[0] = D2(fw0 + 2*0, iv3 + 2*0);
  iv2[1] = D2(fw0 + 2*0, iv3 + 2*1);
  iv2[2] = D2(fw0 + 2*0, iv3 + 2*2);
  iv2[3] = D2(fw0 + 2*0, iv3 + 2*3);
  /* x0y0 */
  iv1[0] = D2(fw0 + 2*1, iv2 + 2*0);           
  iv1[1] = D2(fw0 + 2*1, iv2 + 2*1);
  *val = D2(fw0 + 2*2, iv1);                   /* f */
  if (doD1) {
    gvec[2] = D2(fw1 + 2*2, iv1);              /* g_z */
    if (doD2) {
      hess[8] = D2(fw2 + 2*2, iv1);            /* h_zz */
    }
    /* x0y1 */
    iv1[0] = D2(fw1 + 2*1, iv2 + 2*0);           
    iv1[1] = D2(fw1 + 2*1, iv2 + 2*1);   
    gvec[1] = D2(fw0 + 2*2, iv1);              /* g_y */
    if (doD2) {
      hess[5] = hess[7] = D2(fw1 + 2*2, iv1);  /* h_yz */
      /* x0y2 */
      iv1[0] = D2(fw2 + 2*1, iv2 + 2*0);           
      iv1[1] = D2(fw2 + 2*1, iv2 + 2*1);
      hess[4] = D2(fw0 + 2*2, iv1);            /* h_yy */
    }
    /* x1 */
    iv2[0] = D2(fw1 + 2*0, iv3 + 2*0);
    iv2[1] = D2(fw1 + 2*0, iv3 + 2*1);
    iv2[2] = D2(fw1 + 2*0, iv3 + 2*2);
    iv2[3] = D2(fw1 + 2*0, iv3 + 2*3);
    /* x1y0 */
    iv1[0] = D2(fw0 + 2*1, iv2 + 2*0);           
    iv1[1] = D2(fw0 + 2*1, iv2 + 2*1);
    gvec[0] = D2(fw0 + 2*2, iv1);              /* g_x */
    if (doD2) {
      hess[2] = hess[6] = D2(fw1 + 2*2, iv1);  /* h_xz */
      /* x1y1 */
      iv1[0] = D2(fw1 + 2*1, iv2 + 2*0);           
      iv1[1] = D2(fw1 + 2*1, iv2 + 2*1);
      hess[1] = hess[3] = D2(fw0 + 2*2, iv1);  /* h_xy */
      /* x2 */
      iv2[0] = D2(fw2 + 2*0, iv3 + 2*0);
      iv2[1] = D2(fw2 + 2*0, iv3 + 2*1);
      iv2[2] = D2(fw2 + 2*0, iv3 + 2*2);
      iv2[3] = D2(fw2 + 2*0, iv3 + 2*3);
      /* x2y0 */
      iv1[0] = D2(fw0 + 2*1, iv2 + 2*0);           
      iv1[1] = D2(fw0 + 2*1, iv2 + 2*1);
      hess[0] = D2(fw0 + 2*2, iv1);            /* h_xx */
    }
  }
  return;
}

void
_gageScl3PFilter4(GT *iv3, GT *iv2, GT *iv1,
		  GT *fw0, GT *fw1, GT *fw2,
		  GT *val, GT *gvec, GT *hess,
		  int doD1, int doD2) {
#define D4(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2]+(a)[3]*(b)[3])
  /* x0 */
  iv2[ 0] = D4(fw0 + 4*0, iv3 + 4* 0);
  iv2[ 1] = D4(fw0 + 4*0, iv3 + 4* 1);
  iv2[ 2] = D4(fw0 + 4*0, iv3 + 4* 2);
  iv2[ 3] = D4(fw0 + 4*0, iv3 + 4* 3);
  iv2[ 4] = D4(fw0 + 4*0, iv3 + 4* 4);
  iv2[ 5] = D4(fw0 + 4*0, iv3 + 4* 5);
  iv2[ 6] = D4(fw0 + 4*0, iv3 + 4* 6);
  iv2[ 7] = D4(fw0 + 4*0, iv3 + 4* 7);
  iv2[ 8] = D4(fw0 + 4*0, iv3 + 4* 8);
  iv2[ 9] = D4(fw0 + 4*0, iv3 + 4* 9);
  iv2[10] = D4(fw0 + 4*0, iv3 + 4*10);
  iv2[11] = D4(fw0 + 4*0, iv3 + 4*11);
  iv2[12] = D4(fw0 + 4*0, iv3 + 4*12);
  iv2[13] = D4(fw0 + 4*0, iv3 + 4*13);
  iv2[14] = D4(fw0 + 4*0, iv3 + 4*14);
  iv2[15] = D4(fw0 + 4*0, iv3 + 4*15);
  /* x0y0 */
  iv1[0] = D4(fw0 + 4*1, iv2 + 4*0);
  iv1[1] = D4(fw0 + 4*1, iv2 + 4*1);
  iv1[2] = D4(fw0 + 4*1, iv2 + 4*2);
  iv1[3] = D4(fw0 + 4*1, iv2 + 4*3);
  *val = D4(fw0 + 4*2, iv1);                   /* f */
  if (doD1) {
    gvec[2] = D4(fw1 + 4*2, iv1);              /* g_z */
    if (doD2) {
      hess[8] = D4(fw2 + 4*2, iv1);            /* h_zz */
    }
    /* x0y1 */
    iv1[0] = D4(fw1 + 4*1, iv2 + 4*0);
    iv1[1] = D4(fw1 + 4*1, iv2 + 4*1);
    iv1[2] = D4(fw1 + 4*1, iv2 + 4*2);
    iv1[3] = D4(fw1 + 4*1, iv2 + 4*3);
    gvec[1] = D4(fw0 + 4*2, iv1);              /* g_y */
    if (doD2) {
      hess[5] = hess[7] = D4(fw1 + 4*2, iv1);  /* h_yz */
      /* x0y2 */
      iv1[0] = D4(fw2 + 4*1, iv2 + 4*0);
      iv1[1] = D4(fw2 + 4*1, iv2 + 4*1);
      iv1[2] = D4(fw2 + 4*1, iv2 + 4*2);
      iv1[3] = D4(fw2 + 4*1, iv2 + 4*3);
      hess[4] = D4(fw0 + 4*2, iv1);            /* h_yy */
    }
    /* x1 */
    iv2[ 0] = D4(fw1 + 4*0, iv3 + 4* 0);
    iv2[ 1] = D4(fw1 + 4*0, iv3 + 4* 1);
    iv2[ 2] = D4(fw1 + 4*0, iv3 + 4* 2);
    iv2[ 3] = D4(fw1 + 4*0, iv3 + 4* 3);
    iv2[ 4] = D4(fw1 + 4*0, iv3 + 4* 4);
    iv2[ 5] = D4(fw1 + 4*0, iv3 + 4* 5);
    iv2[ 6] = D4(fw1 + 4*0, iv3 + 4* 6);
    iv2[ 7] = D4(fw1 + 4*0, iv3 + 4* 7);
    iv2[ 8] = D4(fw1 + 4*0, iv3 + 4* 8);
    iv2[ 9] = D4(fw1 + 4*0, iv3 + 4* 9);
    iv2[10] = D4(fw1 + 4*0, iv3 + 4*10);
    iv2[11] = D4(fw1 + 4*0, iv3 + 4*11);
    iv2[12] = D4(fw1 + 4*0, iv3 + 4*12);
    iv2[13] = D4(fw1 + 4*0, iv3 + 4*13);
    iv2[14] = D4(fw1 + 4*0, iv3 + 4*14);
    iv2[15] = D4(fw1 + 4*0, iv3 + 4*15);
    /* x1y0 */
    iv1[ 0] = D4(fw0 + 4*1, iv2 + 4*0);
    iv1[ 1] = D4(fw0 + 4*1, iv2 + 4*1);
    iv1[ 2] = D4(fw0 + 4*1, iv2 + 4*2);
    iv1[ 3] = D4(fw0 + 4*1, iv2 + 4*3);
    gvec[0] = D4(fw0 + 4*2, iv1);              /* g_x */
    if (doD2) {
      hess[2] = hess[6] = D4(fw1 + 4*2, iv1);  /* h_xz */
      /* x1y1 */
      iv1[ 0] = D4(fw1 + 4*1, iv2 + 4*0);
      iv1[ 1] = D4(fw1 + 4*1, iv2 + 4*1);
      iv1[ 2] = D4(fw1 + 4*1, iv2 + 4*2);
      iv1[ 3] = D4(fw1 + 4*1, iv2 + 4*3);
      hess[1] = hess[3] = D4(fw0 + 4*2, iv1);  /* h_xy */
      /* x2 (damn h_xx) */
      iv2[ 0] = D4(fw2 + 4*0, iv3 + 4* 0);
      iv2[ 1] = D4(fw2 + 4*0, iv3 + 4* 1);
      iv2[ 2] = D4(fw2 + 4*0, iv3 + 4* 2);
      iv2[ 3] = D4(fw2 + 4*0, iv3 + 4* 3);
      iv2[ 4] = D4(fw2 + 4*0, iv3 + 4* 4);
      iv2[ 5] = D4(fw2 + 4*0, iv3 + 4* 5);
      iv2[ 6] = D4(fw2 + 4*0, iv3 + 4* 6);
      iv2[ 7] = D4(fw2 + 4*0, iv3 + 4* 7);
      iv2[ 8] = D4(fw2 + 4*0, iv3 + 4* 8);
      iv2[ 9] = D4(fw2 + 4*0, iv3 + 4* 9);
      iv2[10] = D4(fw2 + 4*0, iv3 + 4*10);
      iv2[11] = D4(fw2 + 4*0, iv3 + 4*11);
      iv2[12] = D4(fw2 + 4*0, iv3 + 4*12);
      iv2[13] = D4(fw2 + 4*0, iv3 + 4*13);
      iv2[14] = D4(fw2 + 4*0, iv3 + 4*14);
      iv2[15] = D4(fw2 + 4*0, iv3 + 4*15);
      /* x2y0 */
      iv1[ 0] = D4(fw0 + 4*1, iv2 + 4*0);
      iv1[ 1] = D4(fw0 + 4*1, iv2 + 4*1);
      iv1[ 2] = D4(fw0 + 4*1, iv2 + 4*2);
      iv1[ 3] = D4(fw0 + 4*1, iv2 + 4*3);
      hess[0] = D4(fw0 + 4*2, iv1);            /* h_xx */
    }
  }
  return;
}

void
_gageScl3PFilterN(int fd,
		  GT *iv3, GT *iv2, GT *iv1,
		  GT *fw0, GT *fw1, GT *fw2,
		  GT *val, GT *gvec, GT *hess,
		  int doD1, int doD2) {
  int i, j, k;
  double F;

  /* x0 */
  for (k=0; k<fd; k++)
    for (j=0; j<fd; j++) {
      for (F=i=0; i<fd; i++) F += fw0[i + fd*0]*iv3[i + fd*(j + fd*k)];
      iv2[j + fd*k] = F;
    }
  /* x0y0 */
  for (k=0; k<fd; k++) {
    for (F=j=0; j<fd; j++) F += fw0[j + fd*1]*iv2[j + fd*k];
    iv1[k] = F;
  }
  /* f */
  for (F=i=0; i<fd; i++) F += fw0[i + fd*2]*iv1[i];
  *val = F;
  if (doD1) {
    /* g_z */
    for (F=i=0; i<fd; i++) F += fw1[i + fd*2]*iv1[i];
    gvec[2] = F;
    if (doD2) {
      /* h_zz */
      for (F=i=0; i<fd; i++) F += fw2[i + fd*2]*iv1[i];
      hess[8] = F;
    }
    /* x0y1 */
    for (k=0; k<fd; k++) {
      for (F=j=0; j<fd; j++) F += fw1[j + fd*1]*iv2[j + fd*k];
      iv1[k] = F;
    }
    /* g_y */
    for (F=i=0; i<fd; i++) F += fw0[i + fd*2]*iv1[i];
    gvec[1] = F;
    if (doD2) {
      /* h_yz */
      for (F=i=0; i<fd; i++) F += fw1[i + fd*2]*iv1[i];
      hess[7] = hess[5] = F;
      /* x0y2 */
      for (k=0; k<fd; k++) {
	for (F=j=0; j<fd; j++) F += fw2[j + fd*1]*iv2[j + fd*k];
	iv1[k] = F;
      }
      /* h_yy */
      for (F=i=0; i<fd; i++) F += fw0[i + fd*2]*iv1[i];
      hess[4] = F;
    }
    /* x1 */
    for (k=0; k<fd; k++)
      for (j=0; j<fd; j++) {
	for (F=i=0; i<fd; i++) F += fw1[i + fd*0]*iv3[i + fd*(j + fd*k)];
	iv2[j + fd*k] = F;
      }
    /* x1y0 */
    for (k=0; k<fd; k++) {
      for (F=j=0; j<fd; j++) F += fw0[j + fd*1]*iv2[j + fd*k];
      iv1[k] = F;
    }
    /* g_x */
    for (F=i=0; i<fd; i++) F += fw0[i + fd*2]*iv1[i];
    gvec[0] = F;
    if (doD2) {
      /* h_xz */
      for (F=i=0; i<fd; i++) F += fw1[i + fd*2]*iv1[i];
      hess[2] = hess[6] = F;
      /* x1y1 */
      for (k=0; k<fd; k++) {
	for (F=j=0; j<fd; j++) F += fw1[j + fd*1]*iv2[j + fd*k];
	iv1[k] = F;
      }
      /* h_xy */
      for (F=i=0; i<fd; i++) F += fw0[i + fd*2]*iv1[i];
      hess[1] = hess[3] = F;
      /* x2 (damn h_xx) */
      for (k=0; k<fd; k++)
	for (j=0; j<fd; j++) {
	  for (F=i=0; i<fd; i++) F += fw2[i + fd*0]*iv3[i + fd*(j + fd*k)];
	  iv2[j + fd*k] = F;
	}
      /* x2y0 */
      for (k=0; k<fd; k++) {
	for (F=j=0; j<fd; j++) F += fw0[j + fd*1]*iv2[j + fd*k];
	iv1[k] = F;
      }
      /* h_xx */
      for (F=i=0; i<fd; i++) F += fw0[i + fd*2]*iv1[i];
      hess[0] = F;
    }
  }
  return;
}
