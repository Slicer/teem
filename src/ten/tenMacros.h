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

#ifndef TENMACROS_HAS_BEEN_INCLUDED
#define TENMACROS_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

/* 
******** TEN_LIST2MAT, TEN_MAT2LIST
**
** for going between 7-element list and 9-element matrix
** representations of a symmetric tensor
**
** the ordering of the tensor elements is assumed to be:
**
** threshold        0
** Dxx Dxy Dxz      1   2   3
** Dxy Dyy Dyz  =  (2)  4   5
** Dxz Dyz Dzz     (3) (5)  6 
**
** As in ell, the matrix ordering is given by:
**
**   0  3  6
**   1  4  7
**   2  5  8
**
** Note that TEN_MAT2LIST does NOT set the threshold element (index 0)
*/

#define TEN_LIST2MAT(m, l) ( \
   (m)[0] = (l)[1],          \
   (m)[1] = (l)[2],          \
   (m)[2] = (l)[3],          \
   (m)[3] = (l)[2],          \
   (m)[4] = (l)[4],          \
   (m)[5] = (l)[5],          \
   (m)[6] = (l)[3],          \
   (m)[7] = (l)[5],          \
   (m)[8] = (l)[6] )

#define TEN_MAT2LIST(l, m) ( \
   (l)[1] = (m)[0],          \
   (l)[2] = (m)[3],          \
   (l)[3] = (m)[6],          \
   (l)[4] = (m)[4],          \
   (l)[5] = (m)[7],          \
   (l)[6] = (m)[8] )



#ifdef __cplusplus
}
#endif
#endif /* TENMACROS_HAS_BEEN_INCLUDED */
