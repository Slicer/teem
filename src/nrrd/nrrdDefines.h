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


#ifndef NRRD_DEFINES_HAS_BEEN_INCLUDED
#define NRRD_DEFINES_HAS_BEEN_INCLUDED

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NRRD_MAX_DIM 12         /* The maximum dimension which we can handle */

#define NRRD_BIG_INT long long  /* biggest integral type allowed on system; 
				   used to hold number of items in array 
				   (MUST be a signed type, even though as a 
				   store for # of things, could be unsigned)
				   using "int" would give a maximum pow-of-2
				   array size of 1024x1024x1024 
				   (really 1024x1024x2048-1) */
#define NRRD_BIG_INT_PRINTF "%lld"

#define NRRD_SMALL_STRLEN  129  /* single words, types and labels */
#define NRRD_MED_STRLEN    257  /* phrases, single line of error message */
#define NRRD_BIG_STRLEN    513  /* lines, ascii header lines from file */
#define NRRD_HUGE_STRLEN  1025  /* sentences */
#define NRRD_ERR_STRLEN   2049  /* error message accumulation */

#define NRRD_MAX_KERNEL_PARAMS 5  /* max # arguments to a resampling kernel */

#define NRRD_NO_CONTENT "?"     /* to fill in for an unset "content" field */

/* for the 64-bit integer types (not standard except in C9X), we try to
   use the names for the _MIN and _MAX values which are used in C9X
   (as well as gcc) such as LLONG_MAX; if these aren't defined, we try
   the ones used on SGI such as LONGLONG_MAX, if still not defined, we
   go wild and define something ourselves, with total disregard to
   what the architecture and compiler actually support */

#ifdef LLONG_MAX
#  define NRRD_LLONG_MAX LLONG_MAX
#else
#  ifdef LONGLONG_MAX
#    define NRRD_LLONG_MAX LONGLONG_MAX
#  else
#    define NRRD_LLONG_MAX 9223372036854775807LL
#  endif
#endif

#ifdef LLONG_MIN
#  define NRRD_LLONG_MIN LLONG_MIN
#else
#  ifdef LONGLONG_MIN
#    define NRRD_LLONG_MIN LONGLONG_MIN
#  else
#    define NRRD_LLONG_MIN (-NRRD_LLONG_MAX-1LL)
#  endif
#endif

#ifdef ULLONG_MAX
#  define NRRD_ULLONG_MAX ULLONG_MAX
#else
#  ifdef ULONGLONG_MAX
#    define NRRD_ULLONG_MAX ULONGLONG_MAX
#  else
#    define NRRD_ULLONG_MAX 18446744073709551615LLU
#  endif
#endif

/* -------------------------------------------------------- */
/* ----------- END of user-alterable defines -------------- */
/* -------------------------------------------------------- */

#define NRRD_HEADER "NRRD00.01"
#define NRRD_COMMENT_CHAR '#'

/* extern C */
#ifdef __cplusplus
}
#endif

#endif /* NRRD_DEFINES_HAS_BEEN_INCLUDED */
