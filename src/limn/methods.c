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

limnLight *
limnNewLight() {
  limnLight *lit;

  lit = (limnLight *)calloc(1, sizeof(limnLight));
  return(lit);
}

void
limnNixLight(limnLight *lit) {

  if (lit) {
    free(lit);
  }
}

limnCam *
limnNewCam() {
  limnCam *cam;

  cam = (limnCam *)calloc(1, sizeof(limnCam));
  cam->eyeRel = 1;
  return(cam);
}

void
limnNixCam(limnCam *cam) {

  if (cam) {
    free(cam);
  }
}



