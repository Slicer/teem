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


#include <stdio.h>
#include <limn.h>

void
main() {
  FILE *f1, *f2, *f3, *f4;
  int xi, yi, ui, vi, ri, gi, bi, zi;
  unsigned short qn;
  float r, v[3];
  unsigned char rgb[256*256][3];
  
  f1 = fopen("out1.ppm", "w");
  fprintf(f1, "P6\n");
  fprintf(f1, "256 256\n");
  fprintf(f1, "255\n");
  for (vi=0; vi<=255; vi++) {
    for (ui=0; ui<=255; ui++) {
      qn = (vi << 8) | ui;
      limn16QNtoV(v, qn, AIR_TRUE);
      v[0] = AIR_MAX(0, v[0]);
      v[1] = AIR_MAX(0, v[1]);
      v[2] = AIR_MAX(0, v[2]);
      AIR_INDEX(0, v[0], 1, 256, ri);
      AIR_INDEX(0, v[1], 1, 256, gi);
      AIR_INDEX(0, v[2], 1, 256, bi);
      fputc(ri, f1);
      fputc(gi, f1);
      fputc(bi, f1);
      rgb[qn][0] = ri;
      rgb[qn][1] = gi;
      rgb[qn][2] = bi;
    }
  }
  fclose(f1);

  f2 = fopen("out2.ppm", "w");
  fprintf(f1, "P6\n");
  fprintf(f1, "256 512\n");
  fprintf(f1, "255\n");
  for (yi=0; yi<=255; yi++) {
    v[1] = AIR_AFFINE(0, yi, 255, -1, 1);
    for (xi=0; xi<=255; xi++) {
      v[0] = AIR_AFFINE(0, xi, 255, -1, 1);
      r = v[0]*v[0] + v[1]*v[1];
      if (r <= 1) {
	v[2] = sqrt(1-r);
	qn = limnVto16QN(v);
	fputc(rgb[qn][0], f2);
	fputc(rgb[qn][1], f2);
	fputc(rgb[qn][2], f2);
      }
      else {
	fputc(128, f2);	fputc(128, f2);	fputc(128, f2);
      }
    }
  }
  for (yi=0; yi<=255; yi++) {
    v[1] = AIR_AFFINE(0, yi, 255, -1, 1);
    for (xi=0; xi<=255; xi++) {
      v[0] = AIR_AFFINE(0, xi, 255, -1, 1);
      r = v[0]*v[0] + v[1]*v[1];
      if (r <= 1) {
	v[2] = -sqrt(1-r);
	qn = limnVto16QN(v);
	fputc(rgb[qn][0], f2);
	fputc(rgb[qn][1], f2);
	fputc(rgb[qn][2], f2);
      }
      else {
	fputc(128, f2);	fputc(128, f2);	fputc(128, f2);
      }
    }
  }
  fclose(f2);

  f3 = fopen("out3.ppm", "w");
  fprintf(f3, "P6\n");
  fprintf(f3, "128 256\n");
  fprintf(f3, "255\n");
  zi = 1;
  for (vi=0; vi<=127; vi++) {
    for (ui=0; ui<=127; ui++) {
      qn = (zi << 14) | (vi << 7) | ui;
      limn15QNtoV(v, qn, AIR_TRUE);
      v[0] = AIR_MAX(0, v[0]);
      v[1] = AIR_MAX(0, v[1]);
      v[2] = AIR_MAX(0, v[2]);
      AIR_INDEX(0, v[0], 1, 256, ri);
      AIR_INDEX(0, v[1], 1, 256, gi);
      AIR_INDEX(0, v[2], 1, 256, bi);
      fputc(ri, f3);
      fputc(gi, f3);
      fputc(bi, f3);
      rgb[qn][0] = ri;
      rgb[qn][1] = gi;
      rgb[qn][2] = bi;
    }
  }
  zi = 0;
  for (vi=0; vi<=127; vi++) {
    for (ui=0; ui<=127; ui++) {
      qn = (zi << 14) | (vi << 7) | ui;
      limn15QNtoV(v, qn, AIR_TRUE);
      v[0] = AIR_MAX(0, v[0]);
      v[1] = AIR_MAX(0, v[1]);
      v[2] = AIR_MAX(0, v[2]);
      AIR_INDEX(0, v[0], 1, 256, ri);
      AIR_INDEX(0, v[1], 1, 256, gi);
      AIR_INDEX(0, v[2], 1, 256, bi);
      fputc(ri, f3);
      fputc(gi, f3);
      fputc(bi, f3);
      rgb[qn][0] = ri;
      rgb[qn][1] = gi;
      rgb[qn][2] = bi;
    }
  }
  fclose(f3);

  f4 = fopen("out4.ppm", "w");
  fprintf(f1, "P6\n");
  fprintf(f1, "256 512\n");
  fprintf(f1, "255\n");
  for (yi=0; yi<=255; yi++) {
    v[1] = AIR_AFFINE(0, yi, 255, -1, 1);
    for (xi=0; xi<=255; xi++) {
      v[0] = AIR_AFFINE(0, xi, 255, -1, 1);
      r = v[0]*v[0] + v[1]*v[1];
      if (r <= 1) {
	v[2] = sqrt(1-r);
	qn = limnVto15QN(v);
	fputc(rgb[qn][0], f4);
	fputc(rgb[qn][1], f4);
	fputc(rgb[qn][2], f4);
      }
      else {
	fputc(128, f4);	fputc(128, f4);	fputc(128, f4);
      }
    }
  }
  for (yi=0; yi<=255; yi++) {
    v[1] = AIR_AFFINE(0, yi, 255, -1, 1);
    for (xi=0; xi<=255; xi++) {
      v[0] = AIR_AFFINE(0, xi, 255, -1, 1);
      r = v[0]*v[0] + v[1]*v[1];
      if (r <= 1) {
	v[2] = -sqrt(1-r);
	qn = limnVto15QN(v);
	fputc(rgb[qn][0], f4);
	fputc(rgb[qn][1], f4);
	fputc(rgb[qn][2], f4);
      }
      else {
	fputc(128, f4);	fputc(128, f4);	fputc(128, f4);
      }
    }
  }
  fclose(f4);


}
