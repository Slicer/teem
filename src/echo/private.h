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

#define SPLIT(obj)  ((EchoObjectSplit*)obj)
#define LIST(obj)   ((EchoObjectList*)obj)
#define SPHERE(obj) ((EchoObjectSphere*)obj)
#define AABBOX(obj) ((EchoObjectAABBox*)obj)

/* intx.c */
#define INTX_ARGS(TYPE) EchoIntx *intx, EchoRay *ray,               \
                        EchoParam *param, EchoObject##TYPE *obj

typedef int (*_echoRayIntx_t)(INTX_ARGS( ));
extern _echoRayIntx_t _echoRayIntx[ECHO_OBJECT_MAX+1];

typedef void (*_echoRayIntxUV_t)(EchoIntx *intx, EchoRay *ray);
extern _echoRayIntxUV_t _echoRayIntxUV[ECHO_OBJECT_MAX+1];

/* color.c */
#define COLOR_ARGS echoCol_t *chan, EchoIntx *intx, int samp,       \
                   EchoParam *param, EchoThreadState *tstate,       \
                   EchoObject *scene, airArray *lightArr

typedef void (*_echoIntxColor_t) (COLOR_ARGS);
extern _echoIntxColor_t _echoIntxColor[ECHO_MATTER_MAX+1];

extern int _echoRefract(echoPos_t T[3], echoPos_t V[3],
			echoPos_t N[3], echoCol_t index);

/* object.c */

#define BNDS_ARGS(TYPE) echoPos_t lo[3], echoPos_t hi[3], \
                        EchoObject##TYPE *obj

typedef void (*_echoObjectBounds_t)(BNDS_ARGS( ));
extern _echoObjectBounds_t _echoObjectBounds[ECHO_OBJECT_MAX+1];


#endif /*  ECHO_PRIVATE_HAS_BEEN_INCLUDED */

