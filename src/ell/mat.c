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


#include "ell.h"

#define _3INV \
  det = ELL_3M_DET(m); \
  i[0] =  _ELL_2_DET((m)[4],(m)[5],(m)[7],(m)[8])/det; \
  i[1] = -_ELL_2_DET((m)[1],(m)[2],(m)[7],(m)[8])/det; \
  i[2] =  _ELL_2_DET((m)[1],(m)[2],(m)[4],(m)[5])/det; \
  i[3] = -_ELL_2_DET((m)[3],(m)[5],(m)[6],(m)[8])/det; \
  i[4] =  _ELL_2_DET((m)[0],(m)[2],(m)[6],(m)[8])/det; \
  i[5] = -_ELL_2_DET((m)[0],(m)[2],(m)[3],(m)[5])/det; \
  i[6] =  _ELL_2_DET((m)[3],(m)[4],(m)[6],(m)[7])/det; \
  i[7] = -_ELL_2_DET((m)[0],(m)[1],(m)[6],(m)[7])/det; \
  i[8] =  _ELL_2_DET((m)[0],(m)[1],(m)[3],(m)[4])/det

void
ell3mInvert_f(float i[9], float m[9]) {
  float det;
  
  _3INV;
}

void
ell3mInvert_d(double i[9], double m[9]) {
  double det;

  _3INV;
}

#define _4INV \
  det = ELL_4M_DET(m); \
  i[ 0] =  _ELL_3_DET((m)[ 5],(m)[ 6],(m)[ 7], \
		      (m)[ 9],(m)[10],(m)[11], \
		      (m)[13],(m)[14],(m)[15])/det; \
  i[ 1] = -_ELL_3_DET((m)[ 1],(m)[ 2],(m)[ 3], \
 		      (m)[ 9],(m)[10],(m)[11], \
		      (m)[13],(m)[14],(m)[15])/det; \
  i[ 2] =  _ELL_3_DET((m)[ 1],(m)[ 2],(m)[ 3], \
		      (m)[ 5],(m)[ 6],(m)[ 7], \
		      (m)[13],(m)[14],(m)[15])/det; \
  i[ 3] = -_ELL_3_DET((m)[ 1],(m)[ 2],(m)[ 3], \
		      (m)[ 5],(m)[ 6],(m)[ 7], \
		      (m)[ 9],(m)[10],(m)[11])/det; \
  i[ 4] = -_ELL_3_DET((m)[ 4],(m)[ 6],(m)[ 7], \
		      (m)[ 8],(m)[10],(m)[11], \
		      (m)[12],(m)[14],(m)[15])/det; \
  i[ 5] =  _ELL_3_DET((m)[ 0],(m)[ 2],(m)[ 3], \
		      (m)[ 8],(m)[10],(m)[11], \
		      (m)[12],(m)[14],(m)[15])/det; \
  i[ 6] = -_ELL_3_DET((m)[ 0],(m)[ 2],(m)[ 3], \
		      (m)[ 4],(m)[ 6],(m)[ 7], \
		      (m)[12],(m)[14],(m)[15])/det; \
  i[ 7] =  _ELL_3_DET((m)[ 0],(m)[ 2],(m)[ 3], \
		      (m)[ 4],(m)[ 6],(m)[ 7], \
		      (m)[ 8],(m)[10],(m)[11])/det; \
  i[ 8] =  _ELL_3_DET((m)[ 4],(m)[ 5],(m)[ 7], \
		      (m)[ 8],(m)[ 9],(m)[11], \
		      (m)[12],(m)[13],(m)[15])/det; \
  i[ 9] = -_ELL_3_DET((m)[ 0],(m)[ 1],(m)[ 3], \
		      (m)[ 8],(m)[ 9],(m)[11], \
		      (m)[12],(m)[13],(m)[15])/det; \
  i[10] =  _ELL_3_DET((m)[ 0],(m)[ 1],(m)[ 3], \
		      (m)[ 4],(m)[ 5],(m)[ 7], \
		      (m)[12],(m)[13],(m)[15])/det; \
  i[11] = -_ELL_3_DET((m)[ 0],(m)[ 1],(m)[ 3], \
		      (m)[ 4],(m)[ 5],(m)[ 7], \
		      (m)[ 8],(m)[ 9],(m)[11])/det; \
  i[12] = -_ELL_3_DET((m)[ 4],(m)[ 5],(m)[ 6], \
		      (m)[ 8],(m)[ 9],(m)[10], \
		      (m)[12],(m)[13],(m)[14])/det; \
  i[13] =  _ELL_3_DET((m)[ 0],(m)[ 1],(m)[ 2], \
		      (m)[ 8],(m)[ 9],(m)[10], \
		      (m)[12],(m)[13],(m)[14])/det; \
  i[14] = -_ELL_3_DET((m)[ 0],(m)[ 1],(m)[ 2], \
		      (m)[ 4],(m)[ 5],(m)[ 6], \
		      (m)[12],(m)[13],(m)[14])/det; \
  i[15] =  _ELL_3_DET((m)[ 0],(m)[ 1],(m)[ 2], \
		      (m)[ 4],(m)[ 5],(m)[ 6], \
		      (m)[ 8],(m)[ 9],(m)[10])/det

void
ell4mInvert_f(float i[16], float m[16]) {
  float det;

  _4INV;
}

void
ell4mInvert_d(double i[16], double m[16]) {
  double det;

  _4INV;
}

