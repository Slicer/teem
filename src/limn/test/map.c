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


#include <limn.h>

void
cb(unsigned char rgb[3], float vec[3]) {
  float r, g, b;
  int rI, gI, bI;
  
  ELL_3V_GET(r, g, b, vec);
  r = AIR_ABS(r);
  g = AIR_ABS(g);
  b = AIR_ABS(b);
  AIR_INDEX(0.0, r, 1.0, 256, rI);
  AIR_INDEX(0.0, g, 1.0, 256, gI);
  AIR_INDEX(0.0, b, 1.0, 256, bI);
  ELL_3V_SET(rgb, rI, gI, bI);
  return;
}

char *me;

int
main(int argc, char *argv[]) {
  Nrrd *ppm;

  me = argv[0];

  if (limnEnvMapFill(ppm=nrrdNew(), cb, limnQN16)) {
    fprintf(stderr, "%s: trouble:\n%s", me, biffGet(LIMN));
    exit(1);
  }
  if (nrrdSave("map.ppm", ppm)) {
    fprintf(stderr, "%s: trouble:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  
  exit(0);
}
