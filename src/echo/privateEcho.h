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

#define OBJECT(obj)    ((echoObject*)obj)
#define SPLIT(obj)     ((echoSplit*)obj)
#define LIST(obj)      ((echoList*)obj)
#define SPHERE(obj)    ((echoSphere*)obj)
#define RECTANGLE(obj) ((echoRectangle*)obj)
#define AABBOX(obj)    ((echoAABBox*)obj)
#define TRIMESH(obj)   ((echoTriMesh*)obj)
#define TRIANGLE(obj)  ((echoTriangle*)obj)
#define INSTANCE(obj)  ((echoInstance*)obj)

#define ECHO_NEW(TYPE) \
  (echoObject##TYPE *)echoNew(echoObject##Type)

#ifdef __cplusplus
}
#endif

#endif /*  ECHO_PRIVATE_HAS_BEEN_INCLUDED */
