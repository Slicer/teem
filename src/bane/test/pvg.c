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


#include "../bane.h"

char *me;

void
usage() {
  /*                      0      1        2     (3) */
  fprintf(stderr, "usage: %s <pos2DIn> <ppmOut>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  FILE *file;
  char *posStr, *ppmStr;
  Nrrd *pos, *ppm;
  float *posData, p, min, max, sml, cwght, cidxf;
  unsigned char *ppmData, *rgb;
  int v, g, sv, sg, idx, smlIdx, /* donLen = 23, */ cidx;
  unsigned char don[] = {0, 0, 0,       /* background: black */
			 /* 1 */ 0, 107, 255,   /* start: blue */
			 51, 104, 255,
			 103, 117, 255,
			 123, 124, 255,
			 141, 130, 255,
			 156, 132, 255,
			 166, 131, 245,
			 174, 131, 231,
			 181, 130, 216,
			 187, 130, 201,
			 /* 11 */ 255, 255, 255,       /* middle: white */
			 /* 12 */ 255, 255, 255,
			 187, 130, 201,
			 192, 129, 186,
			 197, 128, 172,
			 200, 128, 158,
			 204, 127, 142,
			 210, 126, 113,
			 212, 126, 98,
			 213, 126, 84,
			 216, 126, 49,
			 /* 22 */ 220, 133, 0};  /* end: orange */

  
  me = argv[0];
  if (3 != argc)
    usage();
  posStr = argv[1];
  ppmStr = argv[2];

  if (!(file = fopen(posStr, "r"))) {
    fprintf(stderr, "%s: couldn't open %s for reading\n", me, posStr);
    usage();
  }
  if (!(pos = nrrdNewRead(file))) {
    fprintf(stderr, "%s: couldn't read pos from %s:\n%s\n", me, posStr,
	    biffGet(NRRD));
    usage();
  }
  fclose(file);
  if (!baneValidPos2D(pos)) {
    fprintf(stderr, "%s: %s isn't a valid p(v,g) file:\n%s\n", me, posStr,
	    biffGet(BANE));
    usage();
  }
  sv = pos->size[0];
  sg = pos->size[1];
  posData = (float*)(pos->data);

  /* assert that min = -max; */
  min = max = airNanf();
  for (g=0; g<=sg-1; g++) {
    for (v=0; v<=sv-1; v++) {
      idx = v + sv*g;
      p = posData[idx];
      if (!AIR_EXISTS(p))
	continue;
      if (!AIR_EXISTS(min)) {
	min = max = p;
	sml = AIR_ABS(p);
      }
      min = AIR_MIN(p, min);
      max = AIR_MAX(p, max);
      if (AIR_ABS(p) < sml) {
	sml = AIR_ABS(p);
	smlIdx = idx;
      }
    }
  }
  printf("%s: pos range: [%g,%g,%g]\n", me, min, sml, max);
  posData[smlIdx] = 0;
  if (nrrdHistoEq(pos, NULL, 2048, 3)) {
    fprintf(stderr, "%s: trouble doing histeq on p(v,g):\n%s\n", me, 
	    biffGet(NRRD));
    exit(1);
  }
  if (!(ppm = nrrdNewPPM(sv, sg))) {
    fprintf(stderr, "%s: couldn't make %dx%d PPM:\n%s\n", me, sv, sg,
	    biffGet(NRRD));
    exit(1);
  }
  ppmData = (unsigned char *)(ppm->data);
  sml = posData[smlIdx];
  for (g=0; g<=sg-1; g++) {
    for (v=0; v<=sv-1; v++) {
      idx = v + sv*g;
      p = posData[idx];
      rgb = ppmData + 3*(v + sv*(sg-1-g));
      if (!AIR_EXISTS(p)) {
	rgb[0] = don[0];
	rgb[1] = don[1];
	rgb[2] = don[2];
	continue;
      }
      if (p > sml) {
	cidxf = AIR_AFFINE(sml, p, max, 11.5, 21.999);
	cidx = cidxf;
	cwght = cidxf - cidx;
	rgb[0] = AIR_AFFINE(0, cwght, 1, don[0+3*cidx], don[0+3*(cidx+1)]);
	rgb[1] = AIR_AFFINE(0, cwght, 1, don[1+3*cidx], don[1+3*(cidx+1)]);
	rgb[2] = AIR_AFFINE(0, cwght, 1, don[2+3*cidx], don[2+3*(cidx+1)]);
      }
      else {
	cidxf = AIR_AFFINE(min, p, sml, 1, 11.5);
	cidx = cidxf;
	cwght = cidxf - cidx;
	rgb[0] = AIR_AFFINE(0, cwght, 1, don[0+3*cidx], don[0+3*(cidx+1)]);
	rgb[1] = AIR_AFFINE(0, cwght, 1, don[1+3*cidx], don[1+3*(cidx+1)]);
	rgb[2] = AIR_AFFINE(0, cwght, 1, don[2+3*cidx], don[2+3*(cidx+1)]);
      }
    }
  }
  if (!(file = fopen(ppmStr, "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, ppmStr);
    exit(1);
  }
  ppm->encoding = nrrdEncodingRaw;
  if (nrrdWritePNM(file, ppm)) {
    fprintf(stderr, "%s: trouble writing ppm:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  fclose(file);

  exit(0);
}

