/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

/*
** The contents of this file were derived from the *rand48*.c files
** from glibc-2.3/stdlib, which is distributed under the LGPL that
** governs the distribution of Teem.
*/

#include "air.h"
#include "privateAir.h"

airDrand48State
_airDrand48StateGlobal = {
  AIR_ULLONG(0x5deece66d),  /* a */
  0xb,                      /* c */
  0x330e, 0, 0              /* x0, x1, x2 */
};
airDrand48State *
airDrand48StateGlobal = &_airDrand48StateGlobal;

void
_airDrand48Iterate(airDrand48State *state) {
  airULLong X, result;

  X = ((airULLong)state->x2 << 32 
       | (unsigned)state->x1 << 16 
       | state->x0);

  result = X*state->a + state->c;

  state->x0 = result & 0xffff;
  state->x1 = (result >> 16) & 0xffff;
  state->x2 = (result >> 32) & 0xffff;

  return;
}

void
airSrand48_r(airDrand48State *state, int seed) {

  state->x0 = 0x330e;
  state->x1 = seed & 0xffff;
  state->x2 = seed >> 16;
  state->c = 0xb;
  state->a = AIR_ULLONG(0x5deece66d);

  return;
}

void
airSrand48(int seed) {

  airSrand48_r(airDrand48StateGlobal, seed);
  return;
}

double
airDrand48_r(airDrand48State *state) {
  _airDouble temp;
  
  _airDrand48Iterate(state);
  temp.v = 1.0;
  temp.c2.frac1 = (state->x2 << 4) | (state->x1 >> 12);
  temp.c2.frac0 = ((state->x1 & 0xfff) << 20) | (state->x0 << 4);
  return temp.v - 1.0;
}

double
airDrand48() {

  return airDrand48_r(airDrand48StateGlobal);
}
