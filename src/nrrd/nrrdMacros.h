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


#ifndef NRRD_MACROS_HAS_BEEN_INCLUDED
#define NRRD_MACROS_HAS_BEEN_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/*
******** NRRD_ROUND(x)
**
** rounds argument to nearest integer
** (using NRRD_BIGINT to make sure value isn't truncated unnecessarily)
*/
#define NRRD_ROUND(x) (NRRD_BIG_INT)((x) + 0.5)

/*
******** NRRD_LERP(f,x,y)
**
** linear interpolation between x and y, weighted by f.
** Returns x when f = 0;
*/
#define NRRD_LERP(f,x,y) ((x) + (f)*((y) - (x)))

/*
******** NRRD_CLAMP_INDEX(i,x,I,L,t)
**
** variant of NRRD_INDEX which does bounds checking, and is thereby slower
*/
#define NRRD_CLAMP_INDEX(i,x,I,L,t) (t=((L)*((double)(NRRDCLAMP(i,x,I))-(i)) \
					 /((double)(I)-(i))), \
				     t -= (t == L))

/* extern C */
#ifdef __cplusplus
}
#endif

#endif /* NRRD_MACROS_HAS_BEEN_INCLUDED */
