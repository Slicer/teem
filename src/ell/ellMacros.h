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

#ifndef ELLMACROS_HAS_BEEN_INCLUDED
#define ELLMACROS_HAS_BEEN_INCLUDED


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
******** ELL_MAX3_IDX
**
** returns 0, 1, 2, to indicate which of the three arguments is largest
*/
#define ELL_MAX3_IDX(v0, v1, v2) \
  (v0 > v1                       \
   ? (v1 > v2                    \
      ? 0                        \
      : (v0 > v2                 \
	 ? 0                     \
	 : 2))                   \
   : (v2 > v1                    \
      ? 2                        \
      : 1))

/*
******** ELL_MIN3_IDX
**
** returns 0, 1, 2, to indicate which of the three arguments is smallest
*/
#define ELL_MIN3_IDX(v0, v1, v2) \
  (v0 < v1                       \
   ? (v1 < v2                    \
      ? 0                        \
      : (v0 < v2                 \
	 ? 0                     \
	 : 2))                   \
   : (v2 < v1                    \
      ? 2                        \
      : 1))

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

#define ELL_3V_SCALE(v2, a, v1) \
  ((v2)[0] = (a)*(v1)[0],       \
   (v2)[1] = (a)*(v1)[1],       \
   (v2)[2] = (a)*(v1)[2])

#define ELL_3V_SCALEADD(v2, s0, v0, s1, v1) \
  ((v2)[0] = (s0)*(v0)[0] + (s1)*(v1)[0],   \
   (v2)[1] = (s0)*(v0)[1] + (s1)*(v1)[1],   \
   (v2)[2] = (s0)*(v0)[2] + (s1)*(v1)[2])

#define ELL_3V_SCALEADD3(v3, s0, v0, s1, v1, s2, v2)        \
  ((v3)[0] = (s0)*(v0)[0] + (s1)*(v1)[0] + (s2)*(v2)[0],    \
   (v3)[1] = (s0)*(v0)[1] + (s1)*(v1)[1] + (s2)*(v2)[1],    \
   (v3)[2] = (s0)*(v0)[2] + (s1)*(v1)[2] + (s2)*(v2)[2])

#define ELL_3V_LEN(v) \
  (sqrt((v)[0]*(v)[0] + (v)[1]*(v)[1] + (v)[2]*(v)[2]))

#define ELL_3V_NORM(v2, v1, norm) \
  (norm = 1.0/ELL_3V_LEN(v1), ELL_3V_SCALE(v2, norm, v1))

#define ELL_3V_CROSS(v3, v1, v2) \
  ((v3)[0] = (v1)[1]*(v2)[2] - (v1)[2]*(v2)[1], \
   (v3)[1] = (v1)[2]*(v2)[0] - (v1)[0]*(v2)[2], \
   (v3)[2] = (v1)[0]*(v2)[1] - (v1)[1]*(v2)[0])

#define ELL_3V_MIN(v3,v1,v2) (         \
  (v3)[0] = AIR_MIN((v1)[0], (v2)[0]), \
  (v3)[1] = AIR_MIN((v1)[1], (v2)[1]), \
  (v3)[2] = AIR_MIN((v1)[2], (v2)[2]))

#define ELL_3V_MAX(v3,v1,v2) (         \
  (v3)[0] = AIR_MAX((v1)[0], (v2)[0]), \
  (v3)[1] = AIR_MAX((v1)[1], (v2)[1]), \
  (v3)[2] = AIR_MAX((v1)[2], (v2)[2]))

#define ELL_3M_SCALE(m2, m1, s) \
  (ELL_3V_SCALE((m2)+0, (m1)+0, (s)), \
   ELL_3V_SCALE((m2)+3, (m1)+3, (s)), \
   ELL_3V_SCALE((m2)+6, (m1)+6, (s)))

#define ELL_3M_ADD(m3, m1, m2) \
  ((m3)[0] = (m1)[0] + (m2)[0],  \
   (m3)[1] = (m1)[1] + (m2)[1],  \
   (m3)[2] = (m1)[2] + (m2)[2],  \
   (m3)[3] = (m1)[3] + (m2)[3],  \
   (m3)[4] = (m1)[4] + (m2)[4],  \
   (m3)[5] = (m1)[5] + (m2)[5],  \
   (m3)[6] = (m1)[6] + (m2)[6],  \
   (m3)[7] = (m1)[7] + (m2)[7],  \
   (m3)[8] = (m1)[8] + (m2)[8])

#define ELL_3M_SUB(m3, m1, m2) \
  ((m3)[0] = (m1)[0] - (m2)[0],  \
   (m3)[1] = (m1)[1] - (m2)[1],  \
   (m3)[2] = (m1)[2] - (m2)[2],  \
   (m3)[3] = (m1)[3] - (m2)[3],  \
   (m3)[4] = (m1)[4] - (m2)[4],  \
   (m3)[5] = (m1)[5] - (m2)[5],  \
   (m3)[6] = (m1)[6] - (m2)[6],  \
   (m3)[7] = (m1)[7] - (m2)[7],  \
   (m3)[8] = (m1)[8] - (m2)[8])

#define ELL_3M_COPY(m2, m1) \
  (ELL_3V_COPY((m2)+0, (m1)+0), \
   ELL_3V_COPY((m2)+3, (m1)+3), \
   ELL_3V_COPY((m2)+6, (m1)+6))

#define ELL_3M_SET_IDENT(m) \
  (ELL_3V_SET((m)+0,  1 ,  0 ,  0), \
   ELL_3V_SET((m)+3,  0 ,  1 ,  0), \
   ELL_3V_SET((m)+9,  0 ,  0 ,  1))

#define ELL_3M_SET_DIAG(m, a, b, c) \
  ((m)[0] = (a), (m)[4] = (b), (m)[8] = (c))

#define ELL_3M_TRANSPOSE(m2, m1) \
  ((m2)[0] = (m1)[0],            \
   (m2)[1] = (m1)[3],            \
   (m2)[2] = (m1)[6],            \
   (m2)[3] = (m1)[1],            \
   (m2)[4] = (m1)[4],            \
   (m2)[5] = (m1)[7],            \
   (m2)[6] = (m1)[2],            \
   (m2)[7] = (m1)[5],            \
   (m2)[8] = (m1)[8])

#define ELL_3M_TRACE(m) ((m)[0] + (m)[4] + (m)[8])

#define ELL_3M_L2NORM(m) \
  (sqrt((m)[0]*(m)[0] + (m)[1]*(m)[1] + (m)[2]*(m)[2] + \
        (m)[3]*(m)[3] + (m)[4]*(m)[4] + (m)[5]*(m)[5] + \
        (m)[6]*(m)[6] + (m)[7]*(m)[7] + (m)[8]*(m)[8]))

#define ELL_3MV_GET_COL0(v, m) \
  (ELL_3V_SET((v), (m)[0], (m)[1], (m)[2]))

#define ELL_3MV_GET_COL1(v, m) \
  (ELL_3V_SET((v), (m)[3], (m)[4], (m)[5]))

#define ELL_3MV_GET_COL2(v, m) \
  (ELL_3V_SET((v), (m)[6], (m)[7], (m)[8]))

#define ELL_3MV_GET_ROW0(v, m) \
  (ELL_3V_SET((v), (m)[0], (m)[3], (m)[6]))

#define ELL_3MV_GET_ROW1(v, m) \
  (ELL_3V_SET((v), (m)[1], (m)[4], (m)[7]))

#define ELL_3MV_GET_ROW2(v, m) \
  (ELL_3V_SET((v), (m)[2], (m)[5], (m)[8]))

#define ELL_3MV_SET_COL0(m, v) \
  (ELL_3V_GET((m)[0], (m)[1], (m)[2], (v)))

#define ELL_3MV_SET_COL1(m, v) \
  (ELL_3V_GET((m)[3], (m)[4], (m)[5], (v)))

#define ELL_3MV_SET_COL2(m, v) \
  (ELL_3V_GET((m)[6], (m)[7], (m)[8], (v)))

#define ELL_3MV_SET_ROW0(m, v) \
  (ELL_3V_GET((m)[0], (m)[3], (m)[6], (v)))

#define ELL_3MV_SET_ROW1(m, v) \
  (ELL_3V_GET((m)[1], (m)[4], (m)[7], (v)))

#define ELL_3MV_SET_ROW2(m, v) \
  (ELL_3V_GET((m)[2], (m)[5], (m)[8], (v)))

#define ELL_3MV_OUTER(m, v1, v2) \
  (ELL_3V_SCALE((m)+0, (v1), (v2)[0]), \
   ELL_3V_SCALE((m)+3, (v1), (v2)[1]), \
   ELL_3V_SCALE((m)+6, (v1), (v2)[2]))

#define ELL_3MV_MUL(v2, m, v1) \
  ((v2)[0] = (m)[0]*(v1)[0] + (m)[3]*(v1)[1] + (m)[6]*(v1)[2], \
   (v2)[1] = (m)[1]*(v1)[0] + (m)[4]*(v1)[1] + (m)[7]*(v1)[2], \
   (v2)[2] = (m)[2]*(v1)[0] + (m)[5]*(v1)[1] + (m)[8]*(v1)[2])

#define ELL_3MV_TMUL(v2, m, v1) \
  ((v2)[0] = (m)[0]*(v1)[0] + (m)[1]*(v1)[1] + (m)[2]*(v1)[2], \
   (v2)[1] = (m)[3]*(v1)[0] + (m)[4]*(v1)[1] + (m)[5]*(v1)[2], \
   (v2)[2] = (m)[6]*(v1)[0] + (m)[7]*(v1)[1] + (m)[8]*(v1)[2])

#define ELL_3M_MUL(m3, m1, m2)                                    \
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
   (m3)[8] = (m1)[2]*(m2)[6] + (m1)[5]*(m2)[7] + (m1)[8]*(m2)[8])


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
  ((v2)[0] = (v1)[0],       \
   (v2)[1] = (v1)[1],       \
   (v2)[2] = (v1)[2],       \
   (v2)[3] = (v1)[3])

#define ELL_4V_ADD(v3, v1, v2)  \
  ((v3)[0] = (v1)[0] + (v2)[0], \
   (v3)[1] = (v1)[1] + (v2)[1], \
   (v3)[2] = (v1)[2] + (v2)[2], \
   (v3)[3] = (v1)[3] + (v2)[3])

#define ELL_4V_SUB(v3, v1, v2)  \
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

#define ELL_4M_COPY(m2, m1)     \
  (ELL_4V_COPY((m2)+ 0, (m1)+ 0), \
   ELL_4V_COPY((m2)+ 4, (m1)+ 4), \
   ELL_4V_COPY((m2)+ 8, (m1)+ 8), \
   ELL_4V_COPY((m2)+12, (m1)+12))

#define ELL_4M_TRANSPOSE(m2, m1) \
  ((m2)[ 0] = (m2)[ 0],          \
   (m2)[ 1] = (m2)[ 4],          \
   (m2)[ 2] = (m2)[ 8],          \
   (m2)[ 3] = (m2)[12],          \
   (m2)[ 4] = (m2)[ 1],          \
   (m2)[ 5] = (m2)[ 5],          \
   (m2)[ 6] = (m2)[ 9],          \
   (m2)[ 7] = (m2)[13],          \
   (m2)[ 8] = (m2)[ 2],          \
   (m2)[ 9] = (m2)[ 6],          \
   (m2)[10] = (m2)[10],          \
   (m2)[11] = (m2)[14],          \
   (m2)[12] = (m2)[ 3],          \
   (m2)[13] = (m2)[ 7],          \
   (m2)[14] = (m2)[11],          \
   (m2)[15] = (m2)[15])

#define ELL_4MV_GET_COL0(v, m) \
  (ELL_4V_SET((v), (m)[ 0], (m)[ 1], (m)[ 2], (m)[ 3]))

#define ELL_4MV_GET_COL1(v, m) \
  (ELL_4V_SET((v), (m)[ 4], (m)[ 5], (m)[ 6], (m)[ 7]))

#define ELL_4MV_GET_COL2(v, m) \
  (ELL_4V_SET((v), (m)[ 8], (m)[ 9], (m)[10], (m)[11])

#define ELL_4MV_GET_COL3(v, m) \
  (ELL_4V_SET((v), (m)[12], (m)[13], (m)[14], (m)[15]))

#define ELL_4MV_GET_ROW0(v, m) \
  (ELL_4V_SET((v), (m)[ 0], (m)[ 4], (m)[ 8], (m)[12]))

#define ELL_4MV_GET_ROW1(v, m) \
  (ELL_4V_SET((v), (m)[ 1], (m)[ 5], (m)[ 9], (m)[13]))

#define ELL_4MV_GET_ROW2(v, m) \
  (ELL_4V_SET((v), (m)[ 2], (m)[ 6], (m)[10], (m)[14]))

#define ELL_4MV_GET_ROW3(v, m) \
  (ELL_4V_SET((v), (m)[ 3], (m)[ 7], (m)[11], (m)[15]))

#define ELL_4MV_SET_COL0(m, v) \
  (ELL_4V_GET((m)[ 0], (m)[ 1], (m)[ 2], (m)[ 3], (v)))

#define ELL_4MV_SET_COL1(m, v) \
  (ELL_4V_GET((m)[ 4], (m)[ 5], (m)[ 6], (m)[ 7], (v)))

#define ELL_4MV_SET_COL2(m, v) \
  (ELL_4V_GET((m)[ 8], (m)[ 9], (m)[10], (m)[11], (v)))

#define ELL_4MV_SET_COL3(m, v) \
  (ELL_4V_GET((m)[12], (m)[13], (m)[14], (m)[15], (v)))

#define ELL_4MV_SET_ROW0(m, v) \
  (ELL_4V_GET((m)[ 0], (m)[ 4], (m)[ 8], (m)[12], (v)))

#define ELL_4MV_SET_ROW1(m, v) \
  (ELL_4V_GET((m)[ 1], (m)[ 5], (m)[ 9], (m)[13], (v)))

#define ELL_4MV_SET_ROW2(m, v) \
  (ELL_4V_GET((m)[ 2], (m)[ 6], (m)[10], (m)[14], (v)))

#define ELL_4MV_SET_ROW3(m, v) \
  (ELL_4V_GET((m)[ 3], (m)[ 7], (m)[11], (m)[15], (v)))

#define ELL_4MV_MUL(v2, m, v1)                                              \
  ((v2)[0]=(m)[ 0]*(v1)[0]+(m)[ 4]*(v1)[1]+(m)[ 8]*(v1)[2]+(m)[12]*(v1)[3], \
   (v2)[1]=(m)[ 1]*(v1)[0]+(m)[ 5]*(v1)[1]+(m)[ 9]*(v1)[2]+(m)[13]*(v1)[3], \
   (v2)[2]=(m)[ 2]*(v1)[0]+(m)[ 6]*(v1)[1]+(m)[10]*(v1)[2]+(m)[14]*(v1)[3], \
   (v2)[3]=(m)[ 3]*(v1)[0]+(m)[ 7]*(v1)[1]+(m)[11]*(v1)[2]+(m)[15]*(v1)[3])

#define ELL_4MV_TMUL(v2, m, v1)                                             \
  ((v2)[0]=(m)[ 0]*(v1)[0]+(m)[ 1]*(v1)[1]+(m)[ 2]*(v1)[2]+(m)[ 3]*(v1)[3], \
   (v2)[1]=(m)[ 4]*(v1)[0]+(m)[ 5]*(v1)[1]+(m)[ 6]*(v1)[2]+(m)[ 7]*(v1)[3], \
   (v2)[2]=(m)[ 8]*(v1)[0]+(m)[ 9]*(v1)[1]+(m)[10]*(v1)[2]+(m)[11]*(v1)[3], \
   (v2)[3]=(m)[12]*(v1)[0]+(m)[13]*(v1)[1]+(m)[14]*(v1)[2]+(m)[15]*(v1)[3])

#define ELL_34V_HOMOG(v2, v1) \
  ((v2)[0] = (v1)[0]/(v1)[3], \
   (v2)[1] = (v1)[1]/(v1)[3], \
   (v2)[2] = (v1)[2]/(v1)[3])

#define ELL_4V_HOMOG(v2, v1)  \
  ((v2)[0] = (v1)[0]/(v1)[3], \
   (v2)[1] = (v1)[1]/(v1)[3], \
   (v2)[2] = (v1)[2]/(v1)[3], \
   (v2)[3] = 1.0)

/*
** the ELL_4M_SET... macros are setting the matrix one _column_
** at a time- so the matrix components appear below in transpose
**
** These macros are intended to be used as aids with homogeneous transforms
*/

#define ELL_4M_SET_COLS(m, a, b, c, d)  \
  (ELL_4V_COPY((m)+ 0, a),              \
   ELL_4V_COPY((m)+ 4, b),              \
   ELL_4V_COPY((m)+ 8, c),              \
   ELL_4V_COPY((m)+12, d))

#define ELL_4M_SET_ROWS(m, a, b, c, d)                 \
  (ELL_4V_SET((m)+ 0, (a)[0], (b)[0], (c)[0], (d)[0]), \
   ELL_4V_SET((m)+ 4, (a)[1], (b)[1], (c)[1], (d)[1]), \
   ELL_4V_SET((m)+ 8, (a)[2], (b)[2], (c)[2], (d)[2]), \
   ELL_4V_SET((m)+12, (a)[3], (b)[3], (c)[3], (d)[3]))

#define ELL_4M_SET_IDENT(m) \
  (ELL_4V_SET((m)+ 0,  1 ,  0 ,  0 , 0), \
   ELL_4V_SET((m)+ 4,  0 ,  1 ,  0 , 0), \
   ELL_4V_SET((m)+ 8,  0 ,  0 ,  1 , 0), \
   ELL_4V_SET((m)+12,  0 ,  0 ,  0 , 1))

#define ELL_4M_SET_SCALE(m, x, y, z)     \
  (ELL_4V_SET((m)+ 0, (x),  0 ,  0 , 0), \
   ELL_4V_SET((m)+ 4,  0 , (y),  0 , 0), \
   ELL_4V_SET((m)+ 8,  0 ,  0 , (z), 0), \
   ELL_4V_SET((m)+12,  0 ,  0 ,  0 , 1))

#define ELL_4M_SET_TRANSLATE(m, x, y, z) \
  (ELL_4V_SET((m)+ 0,  1 ,  0 ,  0 , 0), \
   ELL_4V_SET((m)+ 4,  0 ,  1 ,  0 , 0), \
   ELL_4V_SET((m)+ 8,  0 ,  0 ,  1 , 0), \
   ELL_4V_SET((m)+12, (x), (y), (z), 1))

#define ELL_4M_SET_ROTATE_X(m, th)                   \
  (ELL_4V_SET((m)+ 0,  1 ,     0    ,     0    , 0), \
   ELL_4V_SET((m)+ 4,  0 ,  cos(th) , +sin(th) , 0), \
   ELL_4V_SET((m)+ 8,  0 , -sin(th) ,  cos(th) , 0), \
   ELL_4V_SET((m)+12,  0 ,     0    ,     0    , 1))

#define ELL_4M_SET_ROTATE_Y(m, th)                   \
  (ELL_4V_SET((m)+ 0,  cos(th) ,  0 , -sin(th) , 0), \
   ELL_4V_SET((m)+ 4,     0    ,  1 ,     0    , 0), \
   ELL_4V_SET((m)+ 8, +sin(th) ,  0 ,  cos(th) , 0), \
   ELL_4V_SET((m)+12,     0    ,  0 ,     0    , 1))

#define ELL_4M_SET_ROTATE_Z(m, th)                   \
  (ELL_4V_SET((m)+ 0,  cos(th) , +sin(th) ,  0 , 0), \
   ELL_4V_SET((m)+ 4, -sin(th) ,  cos(th) ,  0 , 0), \
   ELL_4V_SET((m)+ 8,     0    ,     0    ,  1 , 0), \
   ELL_4V_SET((m)+12,     0    ,     0    ,  0 , 1))

#define ELL_4M_MUL(n, l, m)                                                 \
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
   (n)[15]=(l)[ 3]*(m)[12]+(l)[ 7]*(m)[13]+(l)[11]*(m)[14]+(l)[15]*(m)[15])

#define ELL_34M_EXTRACT(m, l) \
  ((m)[0] = (l)[ 0], (m)[1] = (l)[ 1], (m)[2] = (l)[ 2], \
   (m)[3] = (l)[ 4], (m)[4] = (l)[ 5], (m)[5] = (l)[ 6], \
   (m)[6] = (l)[ 8], (m)[7] = (l)[ 9], (m)[8] = (l)[10])

#define ELL_43M_INSET(l, m) \
  ((l)[ 0] = (m)[0], (l)[ 1] = (m)[1], (l)[ 2] = (m)[2], (l)[ 3] = 0, \
   (l)[ 4] = (m)[3], (l)[ 5] = (m)[4], (l)[ 6] = (m)[5], (l)[ 7] = 0, \
   (l)[ 8] = (m)[6], (l)[ 9] = (m)[7], (l)[10] = (m)[8], (l)[11] = 0, \
   (l)[12] =   0   , (l)[13] =   0   , (l)[14] =   0   , (l)[15] = 1)

#define ELL_5V_SET(v, a, b, c, d, e) \
  ((v)[0]=(a), (v)[1]=(b), (v)[2]=(c), (v)[3]=(d), (v)[4]=(e))

#endif /* ELLMACROS_HAS_BEEN_INCLUDED */
