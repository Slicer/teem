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
cb(float rgb[3], float vec[3], void *blah) {
  float r, g, b;
  
  ELL_3V_GET(r, g, b, vec);
  r = AIR_MAX(0, r);
  g = AIR_MAX(0, g);
  b = AIR_MAX(0, b);
  ELL_3V_SET(rgb, r, g, b);
  return;
}

char *me;

int
main(int argc, char *argv[]) {
  Nrrd *map, *ppm;

  me = argv[0];

  if (limnEnvMapFill(map=nrrdNew(), cb, NULL, limnQN_16checker)) {
    fprintf(stderr, "%s: trouble:\n%s", me, biffGet(LIMN));
    exit(1);
  }
  map->min = 0;
  map->max = 1;
  if (nrrdQuantize(ppm=nrrdNew(), map, 8)) {
    fprintf(stderr, "%s: trouble:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  if (nrrdSave("map.ppm", ppm, NULL)) {
    fprintf(stderr, "%s: trouble:\n%s", me, biffGet(NRRD));
    exit(1);
  }

  nrrdNuke(map);
  nrrdNuke(ppm);
  exit(0);
}
