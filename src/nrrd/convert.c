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
** making these typedefs here allows us to use one token for both
** constructing function names, and for specifying argument types
*/
typedef signed char CH;
typedef unsigned char UC;
typedef signed short SH;
typedef unsigned short US;
typedef signed int IN;
typedef unsigned int UI;
typedef signed long long int LL;
typedef unsigned long long int UL;
typedef float FL;
typedef double DB;
typedef nrrdBigInt BI;
/* typedef long double LD; */

/*
** I don't think that I can get out of defining this macro twice,
** because of the rules off C preprocessor macro expansion.  If
** you can figure out a way to not use two identical macros, then
** email me (gk_at_cs.utah.edu) and I'll send you money for dinner.
**
** >>> MAP1 and MAP2 need to be identical <<<
*/

#define MAP1(F, A) \
F(A, CH) \
F(A, UC) \
F(A, SH) \
F(A, US) \
F(A, IN) \
F(A, UI) \
F(A, LL) \
F(A, UL) \
F(A, FL) \
F(A, DB)
/* F(A, LD) */

#define MAP2(F, A) \
F(A, CH) \
F(A, UC) \
F(A, SH) \
F(A, US) \
F(A, IN) \
F(A, UI) \
F(A, LL) \
F(A, UL) \
F(A, FL) \
F(A, DB)
/* F(A, LD) */

/* 
** _nrrdConv<Ta><Tb>()
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
#define CONV_DEF(TA, TB) \
void _nrrdConv##TA##TB(TA *a, TB *b, BI N) { while (N--) a[N]=b[N]; }

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
typedef void (*CF)(void *, void *, BI);

/* 
** Define all 100 of the individual converters. 
*/
MAP1(MAP2, CONV_DEF)

/* 
** Initialize the whole converter array.
** 
** This generates an incredibly long line of text, which hopefully will not
** break a stupid compiler with assumptions about line-length...
*/
CF _nrrdConv[NRRD_TYPE_MAX+1][NRRD_TYPE_MAX+1] = {
{NULL}, 
MAP1(CONVTO_LIST, _dummy_)
{NULL}
};
