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


#include "ten.h"

/*
******** tenAnistropy
**
** given an array of three SORTED (descending) eigenvalues "e",
** calculates the anisotropy coefficients of Westin et al.,
** and stores them in the "c" array.
** 
** c[tenAnisoC_l]: c_l; linear 
** c[tenAnisoC_p]: c_p; planar
** c[tenAnisoC_a]: c_a: c_l + c_p
** c[tenAnisoC_s]: c_s: 1 - c_a
** c[tenAnisoC_t]: c_theta: Gordon's anisotropy type: 0:linear <-> 1:planar
**
** This used to clamp all the values from 0 to 1, but it was
** later decided that if this gets garbage eigenvalues 
** (for instance, some of them are negative), then its okay
** that this produces garbage anisotropy values.  The user
** has to be able to deal.
**
** This does NOT use biff.
*/
void
tenAnisotropy(float c[TEN_MAX_ANISO+1], float e[3]) {
  float sum, cl, cp, ca;
  
  sum = e[0] + e[1] + e[2];

  if (sum) {
    c[tenAnisoC_l] = cl = (e[0] - e[1])/sum;
    c[tenAnisoC_p] = cp = 2*(e[1] - e[2])/sum;
    c[tenAnisoC_a] = ca = cl + cp;
    c[tenAnisoC_s] = 1 - ca;
    c[tenAnisoC_t] = ca ? cp/ca : 0;
  }
  else {
    c[tenAnisoC_l] = 0.0;
    c[tenAnisoC_p] = 0.0;
    c[tenAnisoC_a] = 0.0;
    c[tenAnisoC_s] = 0.0;
    c[tenAnisoC_t] = 0.0;
  }
  return;
}

