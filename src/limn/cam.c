#include "limn.h"

/*
******** limnSetUVN()
**
** sets uvn in cam, as well as eNear, eFar, and eDist
*/
int
limnSetUVN(limnCam *cam) {
  char me[] = "limnSetUVN", err[512];
  float len, u[3], v[3], n[3];

  if (!cam) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(LIMN, err); return(1);
  }
  
  LINEAL_3SUB(n, cam->at, cam->from);
  len = LINEAL_3LEN(n);
  if (!len) {
    sprintf(err, "%s: cam->at (%g,%g,%g) == cam->from (%g,%g,%g)!", me,
	    cam->at[0], cam->at[1], cam->at[2], 
	    cam->from[0], cam->from[1], cam->from[2]);
    biffSet(LIMN, err); return(1);
  }
  if (cam->eyeRel) {
    cam->eNear = cam->near;
    cam->eDist = cam->dist;
    cam->eFar = cam->far;
  }
  else {
    /* ctx->cam->{near,dist} are "at" relative */
    cam->eNear = cam->near + len;
    cam->eDist = cam->dist + len;
    cam->eFar = cam->far + len;
  }
  if (cam->eNear < 0 ||
      cam->eDist < 0 ||
      cam->eFar < 0) {
    sprintf(err, "%s: eye-relative near (%g), dist (%g), or far (%g) < 0",
	    me, cam->eNear, cam->eDist, cam->eFar);
    biffSet(LIMN, err); return(1);
  }
  if (cam->eNear > cam->eFar) {
    sprintf(err, "%s: eye-relative near (%g) further than far (%g)",
	    me, cam->eNear, cam->eFar);
    biffSet(LIMN, err); return(1);
  }
  LINEAL_3SCALE(n, n, 1/len);
  LINEAL_3CROSS(u, n, cam->up);
  len = LINEAL_3LEN(u);
  if (!len) {
    sprintf(err, "%s: cam->up is co-linear with view direction", me);
    biffSet(LIMN, err); return(1);
  }
  LINEAL_3SCALE(u, u, 1/len);
  LINEAL_3CROSS(v, u, n);

  LINEAL_3COPY(cam->uvn+0, u);
  LINEAL_3COPY(cam->uvn+3, v);
  LINEAL_3COPY(cam->uvn+6, n);

  return(0);
}

