/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "echo.h"

#define SIMPLE_NEW(TYPE)                                        \
EchoObject##TYPE *                                              \
_echoObject##TYPE##_new(void) {                                 \
  EchoObject##TYPE *ret;                                        \
                                                                \
 ret = (EchoObject##TYPE *)calloc(1, sizeof(EchoObject##TYPE)); \
 ret->type = echoObject##TYPE;                                  \
 return ret;                                                    \
}

SIMPLE_NEW(Sphere)        /* _echoObjectSphere_new */
SIMPLE_NEW(Cube)          /* _echoObjectCube_new */
SIMPLE_NEW(Triangle)      /* _echoObjectTriangle_new */
SIMPLE_NEW(Rectangle)     /* _echoObjectRectangle_new */

EchoObjectMesh *
_echoObjectMesh_new(void) {
  EchoObjectMesh *ret;

  ret = (EchoObjectMesh *)calloc(1, sizeof(EchoObjectMesh *));
  ret->type = echoObjectMesh;
  /* ??? */
  return ret;
}

EchoObjectAABox *
_echoObjectAABox_new(void) {
  EchoObjectAABox *ret;

  ret = (EchoObjectAABox *)calloc(1, sizeof(EchoObjectAABox *));
  ret->type = echoObjectAABox;
  ret->objArr = airArrayNew((void**)(&(ret->obj)), NULL, 
			    sizeof(EchoObject *), ECHO_OBJECT_INCR);
  /* register callbacks ... */
  return ret;
}

typedef EchoObject *(*echoObjectNew_t)(void);

EchoObject *(*
_echoObjectNew[ECHO_OBJECT_MAX+1])(void) = {
  NULL,
  (echoObjectNew_t)_echoObjectSphere_new,
  (echoObjectNew_t)_echoObjectCube_new,
  (echoObjectNew_t)_echoObjectTriangle_new,
  (echoObjectNew_t)_echoObjectRectangle_new,
  (echoObjectNew_t)_echoObjectMesh_new,
  (echoObjectNew_t)_echoObjectAABox_new
};

EchoObject *
echoObjectNew(int type) {
  
  return _echoObjectNew[type]();
}


/* ---------------------------------------------------- */

EchoObjectMesh *
_echoObjectMesh_nix(EchoObjectMesh *mesh) {

  /* ??? */
  free(mesh);
  return NULL;
}

EchoObjectAABox *
_echoObjectAABox_nix(EchoObjectAABox *box) {
  
  box->objArr = airArrayNuke(box->objArr);
  free(box);
  return NULL;
}

typedef EchoObject *(*echoObjectNix_t)(EchoObject *);

EchoObject *(*
_echoObjectNix[ECHO_OBJECT_MAX+1])(EchoObject *) = {
  NULL,
  (echoObjectNix_t)airFree,
  (echoObjectNix_t)airFree,
  (echoObjectNix_t)airFree,
  (echoObjectNix_t)airFree,
  (echoObjectNix_t)_echoObjectMesh_nix,
  (echoObjectNix_t)_echoObjectAABox_nix
};

EchoObject *
echoObjectNix(EchoObject *obj) {

  return _echoObjectNix[obj->type](obj);
}
