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


#include <stdlib.h>
#include <sys/time.h>  /* for SGI */
#include <time.h>
#include <unistd.h>
#include <wash.h>
#include "../limn.h"

#include <nrrd.h>

float
risky(float m[9]) {
  float sum, dif1, dif2, dif3, q[4];

  sum = 1 + m[2] + m[3] + m[7];
  dif1 = m[0] - m[5];
  dif2 = m[1] - m[8];
  dif3 = m[4] - m[6];
  LINEAL_4SET(q,
	      sum + dif1 - dif2 + dif3,
	      sum + dif1 + dif2 - dif3,
	      sum - dif1 + dif2 + dif3,
	      sum - dif1 - dif2 - dif3);
  /* return(LINEAL_4LEN(q)); */
  return(sum-1);
}

float
safer(float m[9]) {
  float q1[4], q2[4], q3[4], q4[4], q[4];
  
  LINEAL_4SET(q1,
	      1+m[0]+m[4]+m[8],
	      m[7] - m[5],
	      m[2] - m[6],
	      m[3] - m[1]);
  if (q1[0] < 0)
    LINEAL_4SCALE(q1, q1, -1);
  LINEAL_4SET(q2,
	      m[7] - m[5],
	      1+m[0]-m[4]-m[8],
	      m[3] + m[1],
	      m[6] + m[2]);
  if (q2[0] < 0)
    LINEAL_4SCALE(q2, q2, -1);
  LINEAL_4SET(q3,
	      m[2] - m[6],
	      m[3] + m[1],
	      1-m[0]+m[4]-m[8],
	      m[7] + m[5]);
  if (q3[0] < 0)
    LINEAL_4SCALE(q3, q3, -1);
  LINEAL_4SET(q4,
	      m[3] - m[1],
	      m[6] + m[2],
	      m[7] + m[5],
	      1-m[0]-m[4]+m[8]);
  if (q4[0] < 0)
    LINEAL_4SCALE(q4, q4, -1);
  LINEAL_4SET(q, 
	      q1[0] + q2[0] + q3[0] + q4[0],
	      q1[1] + q2[1] + q3[1] + q4[1],
	      q1[2] + q2[2] + q3[2] + q4[2],
	      q1[3] + q2[3] + q3[3] + q4[3]);
  return(LINEAL_4LEN(q));
}

void
randvec(float v[3]) {

  LINEAL_3SET(v, drand48()-0.5, drand48()-0.5, drand48()-0.5);
  lineal3Norm(v);
}

struct timeval tv;
struct timezone tz;

int
main() {
  FILE *tmp;
  limnObj *o, *o1, *o2, *c;
  float th, th1, th2, t[16], s[16], l[9], u1[3], u2[3], 
    n[16], m[16], 
    r[16], r1[16], r2[16], rq[16],
    q[4], q1[4], q2[4], q3[4];
  int i;
  
  /*
  o = limnNewAxes(12, 0.1);
  o1 = limnObjCopy(o);
  o2 = limnObjCopy(o);
  c = limnNewCube();
  limnMatxScale(s, 0.2, 0.2, 0.2);
  limnMatxApply(c, s);
  limnObjMerge(o, c);

  gettimeofday(&tv, &tz);  
  srand48(tv.tv_usec);

  th1 = (drand48()-0.5)*M_PI;
  randvec(u1);
  washAAtoQ(q1, th1, u1);
  washQtoM4(r1, q1);
  th2 = (drand48()-0.5)*M_PI;
  randvec(u2);
  washAAtoQ(q2, th2, u2);
  washQtoM4(r2, q2);

  memcpy(r, r1, 16*sizeof(float));
  limnMatxMult(r, r2);
  washMult(q, q1, q2);
  washQtoM4(rq, q);
  
  
  limnMatxApply(o1, r);
  limnMatxTranslate(t, 0.5, 0.5, 0.5);
  limnMatxApply(o1, t);
  limnMatxApply(o2, rq);
  limnMatxTranslate(t, 0.6, 0.6, 0.6);
  limnMatxApply(o2, t);
  
  limnObjMerge(o, o1);
  limnObjMerge(o, o2);
  tmp = fopen("tmp.obj", "w");
  limnWriteAsOBJ(tmp, o);
  fclose(tmp);
  */
  
  /*
  for (i=0; i<=100; i++) {
    randvec(v);
    th = (drand48()-0.5)*M_PI;
    washAAtoQ(q, th, v);
    washQtoM3(l, q);
    washM3toQ(q2, l);
    washQtoAA(&th2, u, q);
    printf("% 15.10f % 15.10f % 15.10f ;; % 15.10f\n",
	   v[0], v[1], v[2], th);
    printf("% 15.10f % 15.10f % 15.10f ;; % 15.10f\n\n",
	   u[0], u[1], u[2], th2);
  }
  exit(0);
  */  

  /*
  LINEAL_3SET(u1, 0, 0, 0.2);
  LINEAL_3SET(u2, 0, 0, 2.3);
  o = limnNewArrow(20, 1, 0.1, 0.2, 2.0, u1, u2);
  LINEAL_3SET(u1, 0, 0.2, 0);
  LINEAL_3SET(u2, 0, 2, 0);
  o1 = limnNewArrow(20, 1, 0.1, 0.2, 2.0, u1, u2);
  LINEAL_3SET(u1, 0.2, 0, 0);
  LINEAL_3SET(u2, 1.8, 0, 0);
  o2 = limnNewArrow(20, 1, 0.1, 0.2, 2.0, u1, u2);

  limnObjMerge(o, o1);
  limnObjMerge(o, o2);
  tmp = fopen("tmp.obj", "w");
  limnWriteAsOBJ(tmp, o);
  fclose(tmp);
  */

  /*
  o = limnNewCube();
  limnMatxScale(n, 0.2, 0.2, 0.2);
  limnMatxApply(o, n);
  limnMatxTranslate(n, 0, 0, -1.2);
  limnMatxApply(o, n);

  LINEAL_3SET(u1, 0.5, 0, 0);
  LINEAL_3SET(u2, 0, 1, 0);
  o2 = limnNewArrow(20, 1, 0.1, 0.2, 2.0, u1, u2);
  limnMatxTranslate(m, 0, 0, 0);
  limnMatxApply(o2, m);

  limnObjMerge(o2, o);
  tmp = fopen("tmp.obj", "w");
  limnWriteAsOBJ(tmp, o2);
  fclose(tmp);
  */
  
  /*
  LINEAL_3SET(scale, 1, 1, 1);
  o = limnNewCylinder(100, scale);
  tmp = fopen("cylinder.obj", "w");
  limnWriteAsOBJ(tmp, o);
  fclose(tmp);

  LINEAL_3SET(scale, 1, 4, 9);
  o = limnNewSphere(40, 20, scale);
  tmp = fopen("sphere.obj", "w");
  limnWriteAsOBJ(tmp, o);
  fclose(tmp);

  LINEAL_3SET(scale, 1, 4, 9);
  o = limnNewCube(scale);
  tmp = fopen("cube.obj", "w");
  limnWriteAsOBJ(tmp, o);
  fclose(tmp);
  */
}


  /*
    used to evaluate different M -> Q routines

  int idx, res, xi, yi, zi;
  float val, len, x, y, z, q[4], *data, r[9];
  Nrrd *n;
  FILE *f;
  
  res = 129;
  idx = 0;
  data = calloc(res*res*res, sizeof(float));
  for (zi=0; zi<res; zi++) {
    z = AIR_AFFINE(0, zi, res-1, -1, 1);
    for (yi=0; yi<res; yi++) {
      y = AIR_AFFINE(0, yi, res-1, -1, 1);
      for (xi=0; xi<res; xi++) {
	x = AIR_AFFINE(0, xi, res-1, -1, 1);
	len = x*x + y*y + z*z;
	if (len > 1) {
	  val = -0.1;
	}
	else {
	  washSAtoQ(q, x, y, z);
	  washQtoM3(r, q);
	  val = risky(r);
	}
	data[idx] = val;
	idx++;
      }
    }
  }
  n = nrrdNewWrap(data, res*res*res, nrrdTypeFloat, 3);
  n->size[0] = n->size[1] = n->size[2] = res;
  f = fopen("risky.nrrd", "w");
  nrrdWrite(f, n);
  fclose(f);
  nrrdNuke(n);
  exit(0);
  */
