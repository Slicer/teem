/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/


#include "ell.h"

/*
******** ell3vPerp_f()
**
** Given a 3-vector, produce one which is perpendicular.
** Output length won't be same as input length, but it will always
** be non-zero, if input length is non-zero.   This will NOT produce
** a unit-length vector if the input is unit length.
*/
void
ell3vPerp_f(float a[3], float b[3]) {
  int idx;

  idx = 0;
  if (AIR_ABS(b[0]) < AIR_ABS(b[1]))
    idx = 1;
  if (AIR_ABS(b[idx]) < AIR_ABS(b[2]))
    idx = 2;
  switch (idx) {
  case 0:
    ELL_3V_SET(a, b[1] - b[2], -b[0], b[0]);
    break;
  case 1:
    ELL_3V_SET(a, -b[1], b[0] - b[2], b[1]);
    break;
  case 2:
    ELL_3V_SET(a, -b[2], b[2], b[0] - b[1]);
    break;
  }
}

/*
******** ell3vPerp_d()
**
** same as above, but for doubles
*/
void
ell3vPerp_d(double a[3], double b[3]) {
  int idx;

  idx = 0;
  if (AIR_ABS(b[0]) < AIR_ABS(b[1]))
    idx = 1;
  if (AIR_ABS(b[idx]) < AIR_ABS(b[2]))
    idx = 2;
  switch (idx) {
  case 0:
    ELL_3V_SET(a, b[1] - b[2], -b[0], b[0]);
    break;
  case 1:
    ELL_3V_SET(a, -b[1], b[0] - b[2], b[1]);
    break;
  case 2:
    ELL_3V_SET(a, -b[2], b[2], b[0] - b[1]);
    break;
  }
}

