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
EchoLight##TYPE *                                                \
_echoLight##TYPE##_new(void) {                                   \
  EchoLight##TYPE *light;                                        \
                                                                 \
  light = (EchoLight##TYPE *)calloc(1, sizeof(EchoLight##TYPE)); \
  light->type = echoLight##TYPE;                                 \
  do { BODY } while (0);                                         \
  return light;                                                  \
}

NEW_TMPL(Ambient,)                /* _echoLightAmbient_new */
NEW_TMPL(Directional,)            /* _echoLightDirectional_new */
NEW_TMPL(Area,                    /* _echoLightArea_new */
	 light->obj = NULL;
	 );

EchoLight *(*
_echoLightNew[ECHO_LIGHT_MAX+1])(void) = {
  NULL,
  (EchoLight *(*)(void))_echoLightAmbient_new,
  (EchoLight *(*)(void))_echoLightDirectional_new,
  (EchoLight *(*)(void))_echoLightArea_new
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

EchoLight *(*
_echoLightNix[ECHO_LIGHT_MAX+1])(EchoLight *) = {
  NULL,
  (EchoLight *(*)(EchoLight *))airFree,
  (EchoLight *(*)(EchoLight *))airFree,
  (EchoLight *(*)(EchoLight *))_echoLightArea_nix
};

EchoLight *
echoLightNix(EchoLight *light) {

  return _echoLightNix[light->type](light);
}
