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

  /*
    x =  [ 0.5  0.5 ] u
    y    [ 0.5 -0.5 ] v
  */

  /*
    u =  [ 1.0  1.0 ] x
    v    [ 1.0 -1.0 ] y
  */

  /* xor of low bits == 0 --> z<0 ; xor == 1 --> z>0 */

/* ----------------------------------------------------------------  */

void
limn16QNtoV(float *vec, unsigned short qn, int doNorm) {
  int ui, vi;
  float u, v, x, y, z, n;
  
  ui = qn & 0xFF;
  vi = qn >> 8;
  u = AIR_AFFINE(-0.5, ui, 255.5, -0.5, 0.5);
  v = AIR_AFFINE(-0.5, vi, 255.5, -0.5, 0.5);
  x =  u + v;
  y =  u - v;
  z = 1 - AIR_ABS(x) - AIR_ABS(y);
  z *= (((ui ^ vi) & 0x01) << 1) - 1;
  if (doNorm) {
    n = 1.0/sqrt(x*x + y*y + z*z);
    vec[0] = x*n; 
    vec[1] = y*n; 
    vec[2] = z*n;
  }
  else {
    vec[0] = x;
    vec[1] = y;
    vec[2] = z;
  }
}

unsigned short
limnVto16QN(float *vec) {
  float L, u, v, x, y, z;
  int ui, vi, zi;
  
  x = vec[0];
  y = vec[1];
  z = vec[2];
  L = AIR_ABS(x) + AIR_ABS(y) + AIR_ABS(z);
  if (L) {
    x /= L;
    y /= L;
  }
  u = x + y;
  v = x - y;
  AIR_INDEX(-1, u, 1, 256, ui);
  AIR_INDEX(-1, v, 1, 256, vi);
  zi = (ui ^ vi) & 0x01;
  if (!zi && z > 1.0/128.0) {
    ui -= (((ui >> 7) & 0x01) << 1) - 1;
  } 
  else if (zi && z < -1.0/128.0) {
    vi -= (((vi >> 7) & 0x01) << 1) - 1;
  }
  zi = (ui ^ vi) & 0x01;
  if (!zi && z > 1.0/128.0) {
    printf("panic01\n");
  } 
  else if (zi && z < -1.0/128.0) {
    printf("panic02\n");
  }
  return (vi << 8) | ui;
}

/* ----------------------------------------------------------------  */

void
limn16QN1PBtoV(float *vec, unsigned short qn, int doNorm) {
  int ui, vi;
  float u, v, x, y, z, n;
  
  ui = qn & 0xFF;
  vi = qn >> 8;
  u = AIR_AFFINE(0.5, ui, 254.5, -0.5, 0.5);
  v = AIR_AFFINE(0.5, vi, 254.5, -0.5, 0.5);
  x =  u + v;
  y =  u - v;
  z = 1 - AIR_ABS(x) - AIR_ABS(y);
  z *= (((ui ^ vi) & 0x01) << 1) - 1;
  if (doNorm) {
    n = 1.0/sqrt(x*x + y*y + z*z);
    vec[0] = x*n; 
    vec[1] = y*n; 
    vec[2] = z*n;
  }
  else {
    vec[0] = x;
    vec[1] = y;
    vec[2] = z;
  }
}

unsigned short
limnVto16QN1PB(float *vec) {
  float L, u, v, x, y, z;
  int ui, vi, zi;
  
  x = vec[0];
  y = vec[1];
  z = vec[2];
  L = AIR_ABS(x) + AIR_ABS(y) + AIR_ABS(z);
  if (L) {
    x /= L;
    y /= L;
  }
  u = x + y;
  v = x - y;
  AIR_INDEX(-1, u, 1, 254, ui); ui++;
  AIR_INDEX(-1, v, 1, 254, vi); vi++;
  zi = (ui ^ vi) & 0x01;
  if (!zi && z > 1.0/127.0) {
    ui -= (((ui >> 7) & 0x01) << 1) - 1;
  } 
  else if (zi && z < -1.0/127.0) {
    vi -= (((vi >> 7) & 0x01) << 1) - 1;
  }
  zi = (ui ^ vi) & 0x01;
  if (!zi && z > 1.0/127.0) {
    printf("panic01\n");
  } 
  else if (zi && z < -1.0/127.0) {
    printf("panic02\n");
  }
  return (vi << 8) | ui;
}

/* ----------------------------------------------------------------  */

void
limn15QNtoV(float *vec, unsigned short qn, int doNorm) {
  int ui, vi, zi;
  float u, v, x, y, z, n;
  
  if (qn) {
    ui = qn & 0x7F;
    vi = (qn >> 7) & 0x7F;
    zi = (qn >> 14) & 0x01;
    u = AIR_AFFINE(-0.5, ui, 127.5, -0.5, 0.5);
    v = AIR_AFFINE(-0.5, vi, 127.5, -0.5, 0.5);
    x =  u + v;
    y =  u - v;
    z = 1 - AIR_ABS(x) - AIR_ABS(y);
    z *= (zi << 1) - 1;
    if (doNorm) {
      n = 1.0/sqrt(x*x + y*y + z*z);
      vec[0] = x*n; 
      vec[1] = y*n; 
      vec[2] = z*n;
    }
    else {
      vec[0] = x;
      vec[1] = y;
      vec[2] = z;
    }
  }
  else {
    vec[0] = vec[1] = vec[2] = 0.0;
  }
}

unsigned short
limnVto15QN(float *vec) {
  float L, u, v, x, y, z;
  int ui, vi, zi;
  unsigned short ret;
  
  x = vec[0];
  y = vec[1];
  z = vec[2];
  L = AIR_ABS(x) + AIR_ABS(y) + AIR_ABS(z);
  if (L) {
    x /= L;
    y /= L;
    u = x + y;
    v = x - y;
    AIR_INDEX(-1, u, 1, 128, ui);
    AIR_INDEX(-1, v, 1, 128, vi);
    zi = (z > 0);
    ret = (zi << 14) | (vi << 7) | ui;
    ret += !ret;
    return ret;
  }
  else {
    return 0;
  }
}



