/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/


#include "limn.h"

/*  homogenous coordinates matrix transform layout (indices):

   0   4   8   12
   1   5   9   13
   2   6   10  14
   3   7   11  15

   0   1   2   3
   4   5   6   7
   8   9   10  11
   12  13  14  15

*/

void
limnMatxScale(float *m, float x, float y, float z) {

  LINEAL_4SET(m + 0,   x,  0,  0,  0);
  LINEAL_4SET(m + 4,   0,  y,  0,  0);
  LINEAL_4SET(m + 8,   0,  0,  z,  0);
  LINEAL_4SET(m + 12,  0,  0,  0,  1);
}

void
limnMatxTranslate(float *m, float x, float y, float z) {

  LINEAL_4SET(m + 0,   1,  0,  0,  x);
  LINEAL_4SET(m + 4,   0,  1,  0,  y);
  LINEAL_4SET(m + 8,   0,  0,  1,  z);
  LINEAL_4SET(m + 12,  0,  0,  0,  1);
}

void
limnMatxRotateX(float *m, float th) {
  float c, s;

  c = cos(th);
  s = sin(th);
  LINEAL_4SET(m + 0,   1,  0,  0,  0);
  LINEAL_4SET(m + 4,   0,  c, -s,  0);
  LINEAL_4SET(m + 8,   0, +s,  c,  0);
  LINEAL_4SET(m + 12,  0,  0,  0,  1);
}

void
limnMatxRotateY(float *m, float th) {
  float c, s;

  c = cos(th);
  s = sin(th);
  LINEAL_4SET(m + 0,   c,  0, +s,  0);
  LINEAL_4SET(m + 4,   0,  1,  0,  0);
  LINEAL_4SET(m + 8,  -s,  0,  c,  0);
  LINEAL_4SET(m + 12,  0,  0,  0,  1);
}

void
limnMatxRotateZ(float *m, float th) {
  float c, s;

  c = cos(th);
  s = sin(th);
  LINEAL_4SET(m + 0,   c, -s,  0,  0);
  LINEAL_4SET(m + 4,  +s,  c,  0,  0);
  LINEAL_4SET(m + 8,   0,  0,  1,  0);
  LINEAL_4SET(m + 12,  0,  0,  0,  1);
}

int
limnMatxApply(limnObj *o, float *m) {
  char me[] = "limnMatxApply", err[128];
  int i;
  float *w, t[4];

  if (!(o && m)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(LIMN, err); return 1;
  }
  
  for (i=0; i<=o->numP-1; i++) {
    w = o->p[i].w;
    t[0] = LINEAL_4DOT(w, m + 0);
    t[1] = LINEAL_4DOT(w, m + 4);
    t[2] = LINEAL_4DOT(w, m + 8);
    t[3] = LINEAL_4DOT(w, m +12);
    LINEAL_4COPY(w, t);
  }
  return 0;
}

void
limnMatxMult(float *m, float *_n) {
  float n[16], t[16];
  int r, c, i;
  
  /* transpose _n into n */ 
  n[ 0] = _n[ 0];  n[ 1] = _n[ 4];  n[ 2] = _n[ 8];  n[ 3] = _n[12];
  n[ 4] = _n[ 1];  n[ 5] = _n[ 5];  n[ 6] = _n[ 9];  n[ 7] = _n[13];
  n[ 8] = _n[ 2];  n[ 9] = _n[ 6];  n[10] = _n[10];  n[11] = _n[14];
  n[12] = _n[ 3];  n[13] = _n[ 7];  n[14] = _n[11];  n[15] = _n[15];
  
  /* if columns change faster in matrix layout 
     (that is, m[0..3] is first row), then c
     should be inner loop */
  i = 0;
  for (r=0; r<=12; r+=4) {
    for (c=0; c<=12; c+=4) {
      t[i] = LINEAL_4DOT(m + r, n + c);
      i++;
    }
  }

  memcpy(m, t, 16*sizeof(float));
}

int
limnNormHC(limnObj *o) {
  char me[] = "limnNormHC", err[512];
  int i;
  float *w, s;
  
  if (!o) {
    sprintf(err, "%s: got a NULL pointer\n", me);
    biffSet(LIMN, err); return 1;
  }

  for (i=0; i<=o->numP-1; i++) {
    w = o->p[i].w;
    s = 1/w[3];
    LINEAL_4SET(w, w[0]*s, w[1]*s, w[2]*s, 1);
  }
  return 0;
}

void
limnMatxRotateUtoV(float *m, float *_u, float *_v) {
  double len, u[3], v[3], n[3], U[3], V[3],
    dot, eps = 0.0001;
  float x[16];

  /* actually, we rotate from the (u,v,n) frame to 
     (e1, e2, e3), and from (e1, e2, e3) to (U,V,n) */

  LINEAL_3COPY(u, _u);
  lineal3NormD(u);
  LINEAL_3COPY(U, _v);
  lineal3NormD(U);
  LINEAL_3CROSS(n, u, U);
  len = LINEAL_3LEN(n);
  if (AIR_ABS(len) < eps) {
    /* they're nearly co-linear */
    dot = LINEAL_3DOT(u, U);
    if (dot > 0) {
      /* u and U point in nearly the same direction,
	 set m to identity; we're done */
      limnMatxScale(m, 1, 1, 1);
      return;
    }
    else {
      /* u and U point in nearly opposite directions- 
	 the solution is ill-defined, so we guess */
      lineal3PerpD(n, u);
    }
  }
  lineal3NormD(n);
  LINEAL_3CROSS(v, n, u);
  LINEAL_3CROSS(V, n, U);
  LINEAL_4SET(x + 0, u[0], u[1], u[2], 0);
  LINEAL_4SET(x + 4, v[0], v[1], v[2], 0);
  LINEAL_4SET(x + 8, n[0], n[1], n[2], 0);
  LINEAL_4SET(x +12,    0,    0,    0, 1);
  LINEAL_4SET(m + 0, U[0], V[0], n[0], 0);
  LINEAL_4SET(m + 4, U[1], V[1], n[1], 0);
  LINEAL_4SET(m + 8, U[2], V[2], n[2], 0);
  LINEAL_4SET(m +12,    0,    0,    0, 1);
  limnMatxMult(m, x);
}

void
limnMatx9to16(float s[16], float n[9]) {

  s[ 0] = n[0];  s[ 4] = n[3];  s[ 8] = n[6];  s[12] = 0;
  s[ 1] = n[1];  s[ 5] = n[4];  s[ 9] = n[7];  s[13] = 0;
  s[ 2] = n[2];  s[ 6] = n[5];  s[10] = n[8];  s[14] = 0;
  s[ 3] =    0;  s[ 7] =    0;  s[11] =    0;  s[15] = 1;
}

void
limnMatx16to9(float n[9], float s[16]) {

  n[0] = s[ 0];  n[3] = s[ 4];  n[6] = s[ 8];
  n[1] = s[ 1];  n[4] = s[ 5];  n[7] = s[ 9];
  n[2] = s[ 2];  n[5] = s[ 6];  n[8] = s[10];
}

void
limnMatx9Print(FILE *f, float n[9]) {

  fprintf(f, " % 15.10f   % 15.10f   % 15.10f\n", n[0], n[3], n[6]);
  fprintf(f, " % 15.10f   % 15.10f   % 15.10f\n", n[1], n[4], n[7]);
  fprintf(f, " % 15.10f   % 15.10f   % 15.10f\n", n[2], n[5], n[8]);
}

void
limnMatx16Print(FILE *f, float s[16]) {

  fprintf(f, " % 15.10f   % 15.10f   % 15.10f   % 15.10f\n", 
	  s[0], s[4], s[8], s[12]);
  fprintf(f, " % 15.10f   % 15.10f   % 15.10f   % 15.10f\n", 
	  s[1], s[5], s[9], s[13]);
  fprintf(f, " % 15.10f   % 15.10f   % 15.10f   % 15.10f\n", 
	  s[2], s[6], s[10], s[14]);
  fprintf(f, " % 15.10f   % 15.10f   % 15.10f   % 15.10f\n", 
	  s[3], s[7], s[11], s[15]);
}
