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
  ((v)[0] = (a), (v)[1] = (b), (v)[2] = (c), (v))

#define ELL_3V_GET(a, b, c, v) \
  ((a) = (v)[0], (b) = (v)[1], (c) = (v)[2])

#define ELL_3V_COPY(v2, v1) \
  ((v2)[0] = (v1)[0], (v2)[1] = (v1)[1], (v2)[2] = (v1)[2], (v2))

#define ELL_3V_ADD(v3, v1, v2)  \
  ((v3)[0] = (v1)[0] + (v2)[0], \
   (v3)[1] = (v1)[1] + (v2)[1], \
   (v3)[2] = (v1)[2] + (v2)[2], (v3))

#define ELL_3V_ADD3(v4, v1, v2, v3)       \
  ((v4)[0] = (v1)[0] + (v2)[0] + (v3)[0], \
   (v4)[1] = (v1)[1] + (v2)[1] + (v3)[1], \
   (v4)[2] = (v1)[2] + (v2)[2] + (v3)[2], (v4))

#define ELL_3V_SUB(v3, v1, v2)  \
  ((v3)[0] = (v1)[0] - (v2)[0], \
   (v3)[1] = (v1)[1] - (v2)[1], \
   (v3)[2] = (v1)[2] - (v2)[2], (v3))

#define ELL_3V_DOT(v1, v2) \
  ((v1)[0]*(v2)[0] + (v1)[1]*(v2)[1] + (v1)[2]*(v2)[2])

#define ELL_3V_SCALE(v2, v1, a) \
  ((v2)[0] = (v1)[0]*a, (v2)[1] = (v1)[1]*a, (v2)[2] = (v1)[2]*a, (v2))

#define ELL_3V_LEN(v) \
  (sqrt((v)[0]*(v)[0] + (v)[1]*(v)[1] + (v)[2]*(v)[2]))

#define ELL_3V_NORM(v2, v1, norm) \
  (norm = 1.0/ELL_3V_LEN(v1), ELL_3V_SCALE(v2, v1, norm))

#define ELL_3V_CROSS(v3, v1, v2) \
  ((v3)[0] = (v1)[1]*(v2)[2] - (v1)[2]*(v2)[1], \
   (v3)[1] = (v1)[2]*(v2)[0] - (v1)[0]*(v2)[2], \
   (v3)[2] = (v1)[0]*(v2)[1] - (v1)[1]*(v2)[0], (v3))

#define ELL_3M_COPY(m2, m1) \
  (ELL_3V_COPY((m2)+0, (m1)+0), \
   ELL_3V_COPY((m2)+3, (m1)+3), \
   ELL_3V_COPY((m2)+6, (m1)+6), (m2))

#define ELL_3M_SETDIAG(m, a, b, c) \
  ((m)[0] = (a), (m)[4] = (b), (m)[8] = (c), (m))

#define ELL_3M_TRANSP(m2, m1, t)                    \
  ((t) = (m1)[1], (m2)[1] = (m1)[3], (m2)[3] = (t), \
   (t) = (m1)[2], (m2)[2] = (m1)[6], (m2)[6] = (t), \
   (t) = (m1)[5], (m2)[5] = (m1)[7], (m2)[7] = (t), (m2))

#define ELL_3MV_MUL(v2, m, v1) \
  ((v2)[0] = (m)[0]*(v1)[0] + (m)[3]*(v1)[1] + (m)[6]*(v1)[2], \
   (v2)[1] = (m)[1]*(v1)[0] + (m)[4]*(v1)[1] + (m)[7]*(v1)[2], \
   (v2)[2] = (m)[2]*(v1)[0] + (m)[5]*(v1)[1] + (m)[8]*(v1)[2], (v2))

#define ELL_3MV_TMUL(v2, m, v1) \
  ((v2)[0] = (m)[0]*(v1)[0] + (m)[1]*(v1)[1] + (m)[2]*(v1)[2], \
   (v2)[1] = (m)[3]*(v1)[0] + (m)[4]*(v1)[1] + (m)[5]*(v1)[2], \
   (v2)[2] = (m)[6]*(v1)[0] + (m)[7]*(v1)[1] + (m)[8]*(v1)[2], (v2))

#define ELL_3MM_MUL(m3, m1, m2)                                   \
  ((m3)[0] = (m1)[0]*(m2)[0] + (m1)[3]*(m2)[1] + (m1)[6]*(m2)[2], \
   (m3)[1] = (m1)[1]*(m2)[0] + (m1)[4]*(m2)[1] + (m1)[7]*(m2)[2], \
   (m3)[2] = (m1)[2]*(m2)[0] + (m1)[5]*(m2)[1] + (m1)[8]*(m2)[2], \
                                                                  \
   (m3)[3] = (m1)[0]*(m2)[3] + (m1)[3]*(m2)[4] + (m1)[6]*(m2)[5], \
   (m3)[4] = (m1)[1]*(m2)[3] + (m1)[4]*(m2)[4] + (m1)[7]*(m2)[5], \
   (m3)[5] = (m1)[2]*(m2)[3] + (m1)[5]*(m2)[4] + (m1)[8]*(m2)[5], \
                                                                  \
   (m3)[6] = (m1)[0]*(m2)[6] + (m1)[3]*(m2)[7] + (m1)[6]*(m2)[8], \
   (m3)[7] = (m1)[1]*(m2)[6] + (m1)[4]*(m2)[7] + (m1)[7]*(m2)[8], \
   (m3)[8] = (m1)[2]*(m2)[6] + (m1)[5]*(m2)[7] + (m1)[8]*(m2)[8], (m3))

/*
** the 4x4 matrix-related macros assume that the matrix indexing is:
**
** 0   4   8  12
** 1   5   9  13
** 2   6  10  14
** 3   7  11  15
*/

#define ELL_4V_SET(v, a, b, c, d) \
  ((v)[0] = (a), (v)[1] = (b), (v)[2] = (c), (v)[3] = (d), (v))

#define ELL_4V_GET(a, b, c, d, v) \
  ((a) = (v)[0], (b) = (v)[1], (c) = (v)[2], (d) = (v)[3])

#define ELL_4V_COPY(v2, v1) \
  ((v2)[0] = (v1)[0],       \
   (v2)[1] = (v1)[1],       \
   (v2)[2] = (v1)[2],       \
   (v2)[3] = (v1)[3], (v2))

#define ELL_4V_ADD(v3, v1, v2)  \
  ((v3)[0] = (v1)[0] + (v2)[0], \
   (v3)[1] = (v1)[1] + (v2)[1], \
   (v3)[2] = (v1)[2] + (v2)[2], \
   (v3)[3] = (v1)[3] + (v2)[3], (v3))

#define ELL_4V_SUB(v3, v1, v2)  \
  ((v3)[0] = (v1)[0] - (v2)[0], \
   (v3)[1] = (v1)[1] - (v2)[1], \
   (v3)[2] = (v1)[2] - (v2)[2], \
   (v3)[3] = (v1)[3] - (v2)[3], (v3))

#define ELL_4V_DOT(v1, v2) \
  ((v1)[0]*(v2)[0] + (v1)[1]*(v2)[1] + (v1)[2]*(v2)[2] + (v1)[3]*(v2)[3])

#define ELL_4V_SCALE(v2, v1, a) \
  ((v2)[0] = (v1)[0]*a, (v2)[1] = (v1)[1]*a, \
   (v2)[2] = (v1)[2]*a, (v2)[3] = (v1)[3]*a, (v2))

#define ELL_4V_LEN(v) \
  (sqrt((v)[0]*(v)[0] + (v)[1]*(v)[1] + (v)[2]*(v)[2] + (v)[3]*(v)[3]))

#define ELL_4M_COPY(m2, m1)     \
  (ELL_4V_COPY((m2)+ 0, (m1)+ 0), \
   ELL_4V_COPY((m2)+ 4, (m1)+ 4), \
   ELL_4V_COPY((m2)+ 8, (m1)+ 8), \
   ELL_4V_COPY((m2)+12, (m1)+12), (m2))

#define ELL_4MV_MUL(v2, m, v1)                                              \
  ((v2)[0]=(m)[ 0]*(v1)[0]+(m)[ 4]*(v1)[1]+(m)[ 8]*(v1)[2]+(m)[12]*(v1)[3], \
   (v2)[1]=(m)[ 1]*(v1)[0]+(m)[ 5]*(v1)[1]+(m)[ 9]*(v1)[2]+(m)[13]*(v1)[3], \
   (v2)[2]=(m)[ 2]*(v1)[0]+(m)[ 6]*(v1)[1]+(m)[10]*(v1)[2]+(m)[14]*(v1)[3], \
   (v2)[3]=(m)[ 3]*(v1)[0]+(m)[ 7]*(v1)[1]+(m)[11]*(v1)[2]+(m)[15]*(v1)[3], \
   (v2))

#define ELL_4MV_TMUL(v2, m, v1)                                             \
  ((v2)[0]=(m)[ 0]*(v1)[0]+(m)[ 1]*(v1)[1]+(m)[ 2]*(v1)[2]+(m)[ 3]*(v1)[3], \
   (v2)[1]=(m)[ 4]*(v1)[0]+(m)[ 5]*(v1)[1]+(m)[ 6]*(v1)[2]+(m)[ 7]*(v1)[3], \
   (v2)[2]=(m)[ 8]*(v1)[0]+(m)[ 9]*(v1)[1]+(m)[10]*(v1)[2]+(m)[11]*(v1)[3], \
   (v2)[3]=(m)[12]*(v1)[0]+(m)[13]*(v1)[1]+(m)[14]*(v1)[2]+(m)[15]*(v1)[3], \
   (v2))

/*
** the ELL_4M_SET... macros are setting the matrix one _column_
** at a time- so the matrix components appear below in transpose
*/

#define ELL_4M_SET_COLS(m, a, b, c, d)  \
  (ELL_4V_COPY((m)+ 0, a),              \
   ELL_4V_COPY((m)+ 4, b),              \
   ELL_4V_COPY((m)+ 8, c),              \
   ELL_4V_COPY((m)+12, d), (m))

#define ELL_4M_SET_ROWS(m, a, b, c, d)                 \
  (ELL_4V_SET((m)+ 0, (a)[0], (b)[0], (c)[0], (d)[0]), \
   ELL_4V_SET((m)+ 4, (a)[1], (b)[1], (c)[1], (d)[1]), \
   ELL_4V_SET((m)+ 8, (a)[2], (b)[2], (c)[2], (d)[2]), \
   ELL_4V_SET((m)+12, (a)[3], (b)[3], (c)[3], (d)[3]), (m))

#define ELL_4M_SET_IDENT(m) \
  (ELL_4V_SET((m)+ 0,  1 ,  0 ,  0 , 0), \
   ELL_4V_SET((m)+ 4,  0 ,  1 ,  0 , 0), \
   ELL_4V_SET((m)+ 8,  0 ,  0 ,  1 , 0), \
   ELL_4V_SET((m)+12,  0 ,  0 ,  0 , 1), (m))

#define ELL_4M_SET_SCALE(m, x, y, z)     \
  (ELL_4V_SET((m)+ 0, (x),  0 ,  0 , 0), \
   ELL_4V_SET((m)+ 4,  0 , (y),  0 , 0), \
   ELL_4V_SET((m)+ 8,  0 ,  0 , (z), 0), \
   ELL_4V_SET((m)+12,  0 ,  0 ,  0 , 1), (m))

#define ELL_4M_SET_TRANSLATE(m, x, y, z) \
  (ELL_4V_SET((m)+ 0,  1 ,  0 ,  0 , 0), \
   ELL_4V_SET((m)+ 4,  0 ,  1 ,  0 , 0), \
   ELL_4V_SET((m)+ 8,  0 ,  0 ,  1 , 0), \
   ELL_4V_SET((m)+12, (x), (y), (z), 1), (m))

#define ELL_4M_SET_ROTATE_X(m, th)                   \
  (ELL_4V_SET((m)+ 0,  1 ,     0    ,     0    , 0), \
   ELL_4V_SET((m)+ 4,  0 ,  cos(th) , +sin(th) , 0), \
   ELL_4V_SET((m)+ 8,  0 , -sin(th) ,  cos(th) , 0), \
   ELL_4V_SET((m)+12,  0 ,     0    ,     0    , 1), (m))

#define ELL_4M_SET_ROTATE_Y(m, th)                   \
  (ELL_4V_SET((m)+ 0,  cos(th) ,  0 , -sin(th) , 0), \
   ELL_4V_SET((m)+ 4,     0    ,  1 ,     0    , 0), \
   ELL_4V_SET((m)+ 8, +sin(th) ,  0 ,  cos(th) , 0), \
   ELL_4V_SET((m)+12,     0    ,  0 ,     0    , 1), (m))

#define ELL_4M_SET_ROTATE_Z(m, th)                   \
  (ELL_4V_SET((m)+ 0,  cos(th) , +sin(th) ,  0 , 0), \
   ELL_4V_SET((m)+ 4, -sin(th) ,  cos(th) ,  0 , 0), \
   ELL_4V_SET((m)+ 8,     0    ,     0    ,  1 , 0), \
   ELL_4V_SET((m)+12,     0    ,     0    ,  0 , 1), (m))

#define ELL_4MM_MUL(n, l, m)                                                \
  ((n)[ 0]=(l)[ 0]*(m)[ 0]+(l)[ 4]*(m)[ 1]+(l)[ 8]*(m)[ 2]+(l)[12]*(m)[ 3], \
   (n)[ 1]=(l)[ 1]*(m)[ 0]+(l)[ 5]*(m)[ 1]+(l)[ 9]*(m)[ 2]+(l)[13]*(m)[ 3], \
   (n)[ 2]=(l)[ 2]*(m)[ 0]+(l)[ 6]*(m)[ 1]+(l)[10]*(m)[ 2]+(l)[14]*(m)[ 3], \
   (n)[ 3]=(l)[ 3]*(m)[ 0]+(l)[ 7]*(m)[ 1]+(l)[11]*(m)[ 2]+(l)[15]*(m)[ 3], \
                                                                            \
   (n)[ 4]=(l)[ 0]*(m)[ 4]+(l)[ 4]*(m)[ 5]+(l)[ 8]*(m)[ 6]+(l)[12]*(m)[ 7], \
   (n)[ 5]=(l)[ 1]*(m)[ 4]+(l)[ 5]*(m)[ 5]+(l)[ 9]*(m)[ 6]+(l)[13]*(m)[ 7], \
   (n)[ 6]=(l)[ 2]*(m)[ 4]+(l)[ 6]*(m)[ 5]+(l)[10]*(m)[ 6]+(l)[14]*(m)[ 7], \
   (n)[ 7]=(l)[ 3]*(m)[ 4]+(l)[ 7]*(m)[ 5]+(l)[11]*(m)[ 6]+(l)[15]*(m)[ 7], \
                                                                            \
   (n)[ 8]=(l)[ 0]*(m)[ 8]+(l)[ 4]*(m)[ 9]+(l)[ 8]*(m)[10]+(l)[12]*(m)[11], \
   (n)[ 9]=(l)[ 1]*(m)[ 8]+(l)[ 5]*(m)[ 9]+(l)[ 9]*(m)[10]+(l)[13]*(m)[11], \
   (n)[10]=(l)[ 2]*(m)[ 8]+(l)[ 6]*(m)[ 9]+(l)[10]*(m)[10]+(l)[14]*(m)[11], \
   (n)[11]=(l)[ 3]*(m)[ 8]+(l)[ 7]*(m)[ 9]+(l)[11]*(m)[10]+(l)[15]*(m)[11], \
                                                                            \
   (n)[12]=(l)[ 0]*(m)[12]+(l)[ 4]*(m)[13]+(l)[ 8]*(m)[14]+(l)[12]*(m)[15], \
   (n)[13]=(l)[ 1]*(m)[12]+(l)[ 5]*(m)[13]+(l)[ 9]*(m)[14]+(l)[13]*(m)[15], \
   (n)[14]=(l)[ 2]*(m)[12]+(l)[ 6]*(m)[13]+(l)[10]*(m)[14]+(l)[14]*(m)[15], \
   (n)[15]=(l)[ 3]*(m)[12]+(l)[ 7]*(m)[13]+(l)[11]*(m)[14]+(l)[15]*(m)[15], \
   (n))

#define ELL_34M_EXTRACT(m, l) \
  ((m)[0] = (l)[ 0], (m)[1] = (l)[ 1], (m)[2] = (l)[ 2], \
   (m)[3] = (l)[ 4], (m)[4] = (l)[ 5], (m)[5] = (l)[ 6], \
   (m)[6] = (l)[ 8], (m)[7] = (l)[ 9], (m)[8] = (l)[10], (m))

#define ELL_43M_INSET(l, m) \
  ((l)[ 0] = (m)[0], (l)[ 1] = (m)[1], (l)[ 2] = (m)[2], (l)[ 3] = 0, \
   (l)[ 4] = (m)[3], (l)[ 5] = (m)[4], (l)[ 6] = (m)[5], (l)[ 7] = 0, \
   (l)[ 8] = (m)[6], (l)[ 9] = (m)[7], (l)[10] = (m)[8], (l)[11] = 0, \
   (l)[12] =   0   , (l)[13] =   0   , (l)[14] =   0   , (l)[15] = 1, (l))

#ifdef __cplusplus
}
#endif
#endif /* ELLMACROS_HAS_BEEN_INCLUDED */
