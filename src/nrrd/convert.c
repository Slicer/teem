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


#include "nrrd.h"

/* 
** making these typedefs here allows us to used one token for both
** constructing function names, and for specifying argument types
*/
typedef signed char Char;
typedef unsigned char UChar;
typedef signed short Short;
typedef unsigned short UShort;
typedef signed int Int;
typedef unsigned int UInt;
typedef signed long long int LLong;
typedef unsigned long long int ULLong;
typedef float Float;
typedef double Double;
typedef long double LDouble;

/*
** I don't think that I can get out of defining this macro twice.
**
** >>> MAP1 and MAP2 need to be identical <<<
*/

#define MAP1(F, A) \
F(A, Char)   \
F(A, UChar)  \
F(A, Short)  \
F(A, UShort) \
F(A, Int)    \
F(A, UInt)   \
F(A, LLong)  \
F(A, ULLong) \
F(A, Float)  \
F(A, Double) \
F(A, LDouble)

#define MAP2(F, A) \
F(A, Char)   \
F(A, UChar)  \
F(A, Short)  \
F(A, UShort) \
F(A, Int)    \
F(A, UInt)   \
F(A, LLong)  \
F(A, ULLong) \
F(A, Float)  \
F(A, Double) \
F(A, LDouble)

/* 
** _nrrdConvTaTb()
** 
** given two arrays, a and b, of different types (Ta and Tb) but equal
** size N, _nrrdConvTaTb(a, b, N) will copy all the values from b into
** a, thereby effecting the same type-conversion as one gets with a
** cast.  See K+R Appendix A6 (pg. 197) for the details of what that
** entails.  There are plenty of situations where the results are
** "undefined" (assigning -23 to an unsigned char); the point here is
** to make available on arrays all the same behavior you can get from
** scalars 
*/
#define CONV_DEF(TA, TB)                          \
void                                              \
_nrrdConv##TA##TB(TA *a, TB *b, NRRD_BIG_INT N) { \
  for (N--;N>=0;N--)                              \
    a[N]=b[N];                                    \
}

/* 
** the individual converter's appearance in the array initialization,
** using the cast to the "CF" typedef defined below
*/
#define CONV_LIST(TA, TB) (CF)_nrrdConv##TA##TB,

/* 
** the brace-delimited list of all converters _to_ type TYPE 
*/
#define CONVTO_LIST(_dummy_, TYPE) {NULL, MAP2(CONV_LIST, TYPE) NULL},



/*
** This is where the actual emitted code starts ...
*/



/*
** This typedef makes the definition of _nrrdConv[][] shorter
*/
typedef void (*CF)(void *, void *, NRRD_BIG_INT);

/* 
** Define all 121 of the individual converters. 
*/
MAP1(MAP2, CONV_DEF)

/* 
** Initialize the whole converter array.
*/
CF _nrrdConv[NRRD_MAX_TYPE+1][NRRD_MAX_TYPE+1] = {
{NULL}, 
MAP1(CONVTO_LIST, _dummy_)
{NULL}
};
