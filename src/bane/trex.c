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


#include "bane.h"

/* learned:
** NEVER EVER EVER bypass your own damn pseudo-constructors!
** "npos" used to be a Nrrd (not a pointer), and Joe's 
** trex stuff was crashing because the if data free(data) in nrrdAlloc
** was freeing random stuff, but (and this is the weird part)
** only on some 1-D nrrds of 256 floats (pos1D info), and not others.
*/
Nrrd *npos;

#define TREX_LUTLEN 256

float _baneTesting[256];

float *
_baneTRexRead(char *fname) {
  char me[]="_baneTRexRead";
  FILE *file;
  
  file = fopen(fname, "r");
  if (!file) {
    fprintf(stderr, "%s: !!! couldn't open %s for reading\n", me, fname);
    return NULL;
  }
  if (!(npos = nrrdNewRead(file))) {
    fprintf(stderr, "%s: !!! trouble reading \"%s\":\n%s\n", me, 
	    fname, biffGet(NRRD));
    return NULL;
  }
  if (!_baneValidPos1D(npos)) {
    fprintf(stderr, "%s: !!! didn't get a valid p(x) file:\n%s\n", me, 
	    biffGet(BANE));
    return NULL;
  }
  if (TREX_LUTLEN != npos->size[0]) {
    fprintf(stderr, "%s: !!! need a length %d p(x) (not %d)\n", me, 
	    TREX_LUTLEN, npos->size[0]); 
    return NULL;
  }

  return npos->data;
}

void
_baneTRexDone() {

  nrrdNuke(npos); 
}
