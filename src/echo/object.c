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

#define NEW_TMPL(TYPE, BODY)                                     \
EchoObject##TYPE *                                               \
_echoObject##TYPE##_new(void) {                                  \
  EchoObject##TYPE *obj;                                         \
                                                                 \
  obj = (EchoObject##TYPE *)calloc(1, sizeof(EchoObject##TYPE)); \
  obj->type = echoObject##TYPE;                                  \
  do { BODY } while (0);                                         \
  return obj;                                                    \
}

/*
  echoObjectSphere,
  echoObjectCube,
  echoObjectTriangle,
  echoObjectRectangle,
  echoObjectMesh,
  echoObjectIsosurface,
  echoObjectAABox,
*/

NEW_TMPL(Sphere,)
NEW_TMPL(Cube,)
NEW_TMPL(Triangle,)
NEW_TMPL(Rectangle,)
NEW_TMPL(Mesh, /* ??? */ )
NEW_TMPL(Isosurface, /* ??? */)
NEW_TMPL(AABox,                                                             \
	 obj->obj = NULL;                                                   \
	 obj->objArr = airArrayNew((void**)&(obj->obj), NULL,               \
				   sizeof(EchoObject *), ECHO_OBJECT_INCR); \
	 /* register callbacks ... */                                       \
	 )

typedef EchoObject *(*echoObjectNew_t)(void);

EchoObject *(*
_echoObjectNew[ECHO_OBJECT_MAX+1])(void) = {
  NULL,
  (echoObjectNew_t)_echoObjectSphere_new,
  (echoObjectNew_t)_echoObjectCube_new,
  (echoObjectNew_t)_echoObjectTriangle_new,
  (echoObjectNew_t)_echoObjectRectangle_new,
  (echoObjectNew_t)_echoObjectMesh_new,
  (echoObjectNew_t)_echoObjectIsosurface_new,
  (echoObjectNew_t)_echoObjectAABox_new
};

EchoObject *
echoObjectNew(int type) {
  
  return _echoObjectNew[type]();
}


/* ---------------------------------------------------- */

#define NIX_TMPL(TYPE, BODY)                                     \
EchoObject##TYPE *                                               \
_echoObject##TYPE##_nix(EchoObject##TYPE *obj) {                 \
                                                                 \
  do { BODY } while (0);                                         \
  free(obj);                                                     \
  return NULL;                                                   \
}

NIX_TMPL(Mesh, /* ??? */)
NIX_TMPL(AABox,                                                  \
	 obj->objArr = airArrayNuke(obj->objArr);                \
	 )

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
