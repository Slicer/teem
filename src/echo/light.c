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

#define SIMPLE_NEW(TYPE)                                      \
EchoLight##TYPE *                                             \
_echoLight##TYPE##_new(void) {                                \
  EchoLight##TYPE *ret;                                       \
                                                              \
 ret = (EchoLight##TYPE *)calloc(1, sizeof(EchoLight##TYPE)); \
 ret->type = echoLight##TYPE;                                 \
 return ret;                                                  \
}

SIMPLE_NEW(Ambient)      /* _echoLightAmbient_new */
SIMPLE_NEW(Directional)  /* _echoLightDirectional_new */

EchoLightArea *
_echoLightArea_new(void) {
  EchoLightArea *ret;

  ret = (EchoLightArea *)calloc(1, sizeof(EchoLightArea));
  ret->type = echoLightArea;
  /* ??? */
  return ret;
}


typedef EchoLight *(*echoLightNew_t)(void);

EchoLight *(*
_echoLightNew[ECHO_LIGHT_MAX+1])(void) = {
  NULL,
  (echoLightNew_t)_echoLightAmbient_new,
  (echoLightNew_t)_echoLightDirectional_new,
  (echoLightNew_t)_echoLightArea_new
};

EchoLight *
echoLightNew(int type) {
  
  return _echoLightNew[type]();
}

/* ---------------------------------------------------- */

EchoLightArea *
_echoLightArea_nix(EchoLightArea *area) {
  
  /* ??? */
  free(area);
  return NULL;
}

typedef EchoLight *(*echoLightNix_t)(EchoLight *);

EchoLight *(*
_echoLightNix[ECHO_LIGHT_MAX+1])(EchoLight *) = {
  NULL,
  (echoLightNix_t)airFree,
  (echoLightNix_t)airFree,
  (echoLightNix_t)_echoLightArea_nix
};

EchoLight *
echoLightNix(EchoLight *light) {

  return _echoLightNix[light->type](light);
}
