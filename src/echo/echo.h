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

#ifndef ECHO_HAS_BEEN_INCLUDED
#define ECHO_HAS_BEEN_INCLUDED

#define ECHO "echo"

#include <stdio.h>
#include <math.h>

#include <air.h>
#include <biff.h>
#include <ell.h>
#include <limn.h>
#include <nrrd.h>
#include <dye.h>

typedef float echoPos_t;
typedef float echoCol_t;

#define ECHO_EPS 0.0001
#define ECHO_OBJECT_INCR 100

/* ray ---------------------------------------- */

typedef struct {
  echoPos_t pos[3];
  echoPos_t dir[3];
} EchoRay;

/* object ---------------------------------------- */

enum {
  echoObjectUnknown,
  echoObjectSphere,
  echoObjectCube,
  echoObjectTriangle,
  echoObjectRectangle,
  echoObjectMesh,
  echoObjectAABox,
  echoObjectLast
};
#define ECHO_OBJECT_MAX    6

/* function: me, k, intx --> lit color */

#define ECHO_OBJECT_COMMON \
  int type

typedef struct {
  ECHO_OBJECT_COMMON;
} EchoObject;

typedef struct {
  ECHO_OBJECT_COMMON;
  echoPos_t pos[3];
  echoPos_t rad;
} EchoObjectSphere;

typedef struct {
  ECHO_OBJECT_COMMON;
  /* ??? */
} EchoObjectCube;

typedef struct {
  ECHO_OBJECT_COMMON;
  echoPos_t vert[3][3];
} EchoObjectTriangle;

typedef struct {
  ECHO_OBJECT_COMMON;
  echoPos_t origin[3], edge0[3], edge1[3];
} EchoObjectRectangle;

typedef struct {
  ECHO_OBJECT_COMMON;
  /* ??? */
} EchoObjectMesh;

typedef struct {
  ECHO_OBJECT_COMMON;
  echoPos_t min[3], max[3];
  EchoObject **obj;
  airArray *objArr;
} EchoObjectAABox;

extern EchoObject *echoObjectNew(int type);
extern EchoObject *echoObjectNix(EchoObject *obj);

/* light ---------------------------------------- */

enum {
  echoLightUnknown,
  echoLightAmbient,     /* 1 */
  echoLightDirectional, /* 2 */
  echoLightArea,        /* 3 */
  echoLightLast
};
#define ECHO_LIGHT_MAX     3

#define ECHO_LIGHT_COMMON \
  int type;               \
  echoCol_t col[3]

typedef struct {
  ECHO_LIGHT_COMMON;
} EchoLight;

typedef struct {
  ECHO_LIGHT_COMMON;
} EchoLightAmbient;

typedef struct {
  ECHO_LIGHT_COMMON;
  echoPos_t dir[3];
} EchoLightDirectional;

typedef struct {
  ECHO_LIGHT_COMMON;
  echoPos_t origin[3], edge0[3], edge1[3];
  /* ??? */
} EchoLightArea;

extern EchoLight *echoLightNew(int type);
extern EchoLight *echoLightNix(EchoLight *light);

#endif /* ECHO_HAS_BEEN_INCLUDED */
