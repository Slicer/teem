#include "limn.h"

void
limn16QNtoV(float *vec, unsigned short qn, int doNorm) {
  int ui, vi;
  float u, v, x, y, z, n;
  
  ui = qn & 0xFF;
  vi = qn >> 8;
  /*
    x =  [ 0.5  0.5 ] u
    y    [ 0.5 -0.5 ] v
  */
  u = AIR_AFFINE(-0.5, ui, 255.5, -0.5, 0.5);
  v = AIR_AFFINE(-0.5, vi, 255.5, -0.5, 0.5);
  x =  u + v;
  y =  u - v;
  z = 1 - AIR_ABS(x) - AIR_ABS(y);
  /* xor of low bits == 0 --> z<0 ; xor == 1 --> z>0 */
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
  x /= L;
  y /= L;
  /*
    u =  [ 1.0  1.0 ] x
    v    [ 1.0 -1.0 ] y
  */
  u = x + y;
  v = x - y;
  AIR_INDEX(-1, u, 1, 256, ui);
  AIR_INDEX(-1, v, 1, 256, vi);
  /* xor of low bits == 0 --> z<0 ; xor == 1 --> z>0 */
  zi = (ui ^ vi) & 0x01;
  if (!zi && z > 1.0/128.0) {
    ui -= (((ui >> 7) & 0x01) << 1) - 1;
  } 
  else if (zi && z < -1.0/128.0) {
    vi -= (((vi >> 7) & 0x01) << 1) - 1;
  }
  zi = (ui ^ vi) & 0x01;
  if (!zi && z > 1.0/128.0) {
    printf("fuck01\n");
  } 
  else if (zi && z < -1.0/128.0) {
    printf("fuck02\n");
  }
  return (vi << 8) & ui;
}
