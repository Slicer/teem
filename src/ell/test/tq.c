/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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


#include <stdio.h>
#include "../ell.h"

char *me;

int
main(int argc, char *argv[]) {
  float angle_f, axis_f[3], q_f[4], tm3A_f[9], tm3B_f[9], tm3C_f[9], 
    tm4A_f[9], tm4B_f[9], tm4C_f[9], 
    rmat3_f[9], rmat4_f[16], mat3_f[9], mat4_f[16];
  double angle_d, axis_d[3], q_d[4], tm3A_d[9], tm3B_d[9], tm3C_d[9], 
    tm4A_d[9], tm4B_d[9], tm4C_d[9], 
    rmat3_d[9], rmat4_d[16], mat3_d[9], mat4_d[16];

  int I, N;
  double tmp, det, frob;

  me = argv[0];
  N = 100;

  for (I=0; I<N; I++) {
    ELL_3V_SET(axis_f, 2*airRand()-1, 2*airRand()-1, 2*airRand()-1);
    ELL_3V_NORM(axis_f, axis_f, tmp); /* yea, not uniform, so what */
    angle_f = M_PI*(2*airRand()-1);

    ell_aa_to_3m_f(rmat3_f, angle_f, axis_f);
    det = ELL_3M_DET(rmat3_f);
    frob = ELL_3M_FROB(rmat3_f);
    ELL_3M_TRAN(tm3A_f, rmat3_f);
    ell_3m_inv_f(tm3B_f, rmat3_f);
    ELL_3M_SUB(tm3C_f, tm3A_f, tm3B_f);
    printf("%04d: det = %g; size = %g; err = %g\n", I, det, frob*frob/3,
	   1 + ELL_3M_FROB(tm3C_f));
  }

  exit(0);
}
