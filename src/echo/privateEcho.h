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

#ifndef ECHO_PRIVATE_HAS_BEEN_INCLUDED
#define ECHO_PRIVATE_HAS_BEEN_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define OBJECT(obj)    ((EchoObject*)obj)
#define SPLIT(obj)     ((EchoSplit*)obj)
#define LIST(obj)      ((EchoList*)obj)
#define SPHERE(obj)    ((EchoSphere*)obj)
#define RECTANGLE(obj) ((EchoRectangle*)obj)
#define AABBOX(obj)    ((EchoAABBox*)obj)
#define TRIMESH(obj)   ((EchoTriMesh*)obj)
#define TRIANGLE(obj)  ((EchoTriangle*)obj)
#define INSTANCE(obj)  ((EchoInstance*)obj)

#define ECHO_NEW(TYPE) \
  (EchoObject##TYPE *)echoNew(echoObject##Type)

/* color.c */
#define COLOR_ARGS echoCol_t *chan, EchoIntx *intx, int samp,       \
                   EchoRTParm *parm, EchoThreadState *tstate,       \
                   EchoObject *scene, airArray *lightArr

typedef void (*_echoIntxColor_t) (COLOR_ARGS);
extern _echoIntxColor_t _echoIntxColor[ECHO_MATTER_MAX+1];

extern int _echoRefract(echoPos_t T[3], echoPos_t V[3],
			echoPos_t N[3], echoCol_t index);

#ifdef __cplusplus
}
#endif

#endif /*  ECHO_PRIVATE_HAS_BEEN_INCLUDED */
