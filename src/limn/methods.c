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



