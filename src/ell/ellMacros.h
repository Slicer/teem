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

#ifndef ELLMACROS_HAS_BEEN_INCLUDED
#define ELLMACROS_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif


/*
******** ELL_SWAP2, ELL_SWAP3
**
** used to interchange 2 or 3 values, using the given temp variable
*/
#define ELL_SWAP2(a, b, t)    (t)=(a);(a)=(b);(b)=(t)
#define ELL_SWAP3(a, b, c, t) (t)=(a);(a)=(b);(b)=(c);(c)=(t)

/*
******** ELL_SORT3
**
** sorts v0, v1, v2 in descending order, using given temp variable t,
*/
#define ELL_SORT3(v0, v1, v2, t)             \
  if (v0 > v1) {                             \
    if (v1 < v2) {                           \
      if (v0 > v2) { ELL_SWAP2(v1, v2, t); } \
      else { ELL_SWAP3(v0, v2, v1, t); }     \
    }                                        \
  }                                          \
  else {                                     \
    if (v1 > v2) {                           \
      if (v0 > v2) { ELL_SWAP2(v0, v1, t); } \
      else { ELL_SWAP3(v0, v1, v2, t); }     \
    }                                        \
    else {                                   \
      ELL_SWAP2(v0, v2, t);                  \
    }                                        \
  }


/*
** the 3x3 matrix-related macros assume that the matrix indexing is:
** 0  3  6
** 1  4  7
** 2  5  8
*/

#define ELL_3V_SET(v, a, b, c) \
  ((v)[0] = (a), (v)[1] = (b), (v)[2] = (c))

#define ELL_3V_GET(a, b, c, v) \
  ((a) = (v)[0], (b) = (v)[1], (c) = (v)[2])

#define ELL_3V_COPY(v2, v1) \
  ((v2)[0] = (v1)[0], (v2)[1] = (v1)[1], (v2)[2] = (v1)[2])

#define ELL_3V_ADD(v3, v1, v2)  \
  ((v3)[0] = (v1)[0] + (v2)[0], \
   (v3)[1] = (v1)[1] + (v2)[1], \
   (v3)[2] = (v1)[2] + (v2)[2])

#define ELL_3V_ADD3(v4, v1, v2, v3)       \
  ((v4)[0] = (v1)[0] + (v2)[0] + (v3)[0], \
   (v4)[1] = (v1)[1] + (v2)[1] + (v3)[1], \
   (v4)[2] = (v1)[2] + (v2)[2] + (v3)[2])

#define ELL_3V_SUB(v3, v1, v2)  \
  ((v3)[0] = (v1)[0] - (v2)[0], \
   (v3)[1] = (v1)[1] - (v2)[1], \
   (v3)[2] = (v1)[2] - (v2)[2])

#define ELL_3V_DOT(v1, v2) \
  ((v1)[0]*(v2)[0] + (v1)[1]*(v2)[1] + (v1)[2]*(v2)[2])

#define ELL_3V_SCALE(v2, v1, a) \
  ((v2)[0] = (v1)[0]*a, (v2)[1] = (v1)[1]*a, (v2)[2] = (v1)[2]*a)

#define ELL_3V_LEN(v) \
  (sqrt((v)[0]*(v)[0] + (v)[1]*(v)[1] + (v)[2]*(v)[2]))

#define ELL_3V_NORM(v, norm) \
  (norm = 1.0/ELL_3V_LEN(v), ELL_3V_SCALE(v, v, norm))

#define ELL_3V_CROSS(v3, v1, v2) \
  ((v3)[0] = (v1)[1]*(v2)[2] - (v1)[2]*(v2)[1], \
   (v3)[1] = (v1)[2]*(v2)[0] - (v1)[0]*(v2)[2], \
   (v3)[2] = (v1)[0]*(v2)[1] - (v1)[1]*(v2)[0])

#define ELL_3M_COPY(m2, m1) \
  ((m2)[0] = (m1)[0], (m2)[1] = (m1)[1], (m2)[2] = (m1)[2], \
   (m2)[3] = (m1)[3], (m2)[4] = (m1)[4], (m2)[5] = (m1)[5], \
   (m2)[6] = (m1)[6], (m2)[7] = (m1)[7], (m2)[8] = (m1)[8])

#define ELL_3M_SETDIAG(m, a, b, c) \
  ((m)[0] = (a), (m)[4] = (b), (m)[8] = (c))

#define ELL_3MV_MUL(v2, m, v1) \
  ((v2)[0] = (m)[0]*(v1)[0] + (m)[3]*(v1)[1] + (m)[6]*(v1)[2], \
   (v2)[1] = (m)[1]*(v1)[0] + (m)[4]*(v1)[1] + (m)[7]*(v1)[2], \
   (v2)[2] = (m)[2]*(v1)[0] + (m)[5]*(v1)[1] + (m)[8]*(v1)[2])

#define ELL_3MV_TMUL(v2, m, v1) \
  ((v2)[0] = (m)[0]*(v1)[0] + (m)[1]*(v1)[1] + (m)[2]*(v1)[2], \
   (v2)[1] = (m)[3]*(v1)[0] + (m)[4]*(v1)[1] + (m)[5]*(v1)[2], \
   (v2)[2] = (m)[6]*(v1)[0] + (m)[7]*(v1)[1] + (m)[8]*(v1)[2])

/*
** the 4x4 matrix-related macros assume that the matrix indexing is:
**
** 0   4   8  12
** 1   5   9  13
** 2   6  10  14
** 3   7  11  15
*/

#define ELL_4V_SET(v, a, b, c, d) \
  ((v)[0] = (a), (v)[1] = (b), (v)[2] = (c), (v)[3] = (d))

#define ELL_4V_GET(a, b, c, d, v) \
  ((a) = (v)[0], (b) = (v)[1], (c) = (v)[2], (d) = (v)[3])

#define ELL_4V_COPY(v2, v1) \
  ((v2)[0] = (v1)[0], (v2)[1] = (v1)[1], (v2)[2] = (v1)[2], (v2)[3] = (v1)[3])

#define ELL_4V_ADD(v3, v1, v2)    \
  ((v3)[0] = (v1)[0] + (v2)[0], \
   (v3)[1] = (v1)[1] + (v2)[1], \
   (v3)[2] = (v1)[2] + (v2)[2], \
   (v3)[3] = (v1)[3] + (v2)[3])

#define ELL_4V_SUB(v3, v1, v2)    \
  ((v3)[0] = (v1)[0] - (v2)[0], \
   (v3)[1] = (v1)[1] - (v2)[1], \
   (v3)[2] = (v1)[2] - (v2)[2], \
   (v3)[3] = (v1)[3] - (v2)[3])

#define ELL_4V_DOT(v1, v2) \
  ((v1)[0]*(v2)[0] + (v1)[1]*(v2)[1] + (v1)[2]*(v2)[2] + (v1)[3]*(v2)[3])

#define ELL_4V_SCALE(v2, v1, a) \
  ((v2)[0] = (v1)[0]*a, (v2)[1] = (v1)[1]*a, \
   (v2)[2] = (v1)[2]*a, (v2)[3] = (v1)[3]*a)

#define ELL_4V_LEN(v) \
  (sqrt((v)[0]*(v)[0] + (v)[1]*(v)[1] + (v)[2]*(v)[2] + (v)[3]*(v)[3]))



#ifdef __cplusplus
}
#endif
#endif /* ELLMACROS_HAS_BEEN_INCLUDED */
