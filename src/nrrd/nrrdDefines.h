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
#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>

#define NRRD_DIM_MAX 10            /* Maximum dimension which we can handle */

#define NRRD_EXT_HEADER ".nhdr"
#define NRRD_EXT_PGM    ".pgm"
#define NRRD_EXT_PPM    ".ppm"
#define NRRD_EXT_TABLE  ".txt"

#define NRRD_BIG_INT_PRINTF "%llu"

#define NRRD_STRLEN_SMALL   65
#define NRRD_STRLEN_MED    129
#define NRRD_STRLEN_BIG    257
#define NRRD_STRLEN_HUGE   513

#define NRRD_STRLEN_LINE 8*1024+1  /* length of lines from a file.  Needs to
				      be big because of possibility of bare
				      ascii tables with lots of data */
#define NRRD_STRLEN_COMMENT 129    /* longest comment strings allowed */

#define NRRD_KERNEL_PARAMS_MAX 5   /* max # arguments to a kernel */


/* 
** For the 64-bit integer types (not standard except in C9X), we try
** to use the names for the _MIN and _MAX values which are used in C9X
** (as well as gcc) such as LLONG_MAX.
** 
** If these aren't defined, we try the ones used on SGI such as
** LONGLONG_MAX.
**
** If these aren't defined either, we go wild and define something
** ourselves (which just happen to be the values defined in C9X), with
** total disregard to what the architecture and compiler actually
** support.
*/

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

/*
** Chances are, you shouldn't mess with these
*/

#define NRRD_BIGGEST_TYPE double    /* this should be a basic C type which
				       requires for storage the maximum size
				       of all the basic C types */
#define NRRD_COMMENT_INCR 16
#define NRRD_PNM_COMMENT "# NRRD>"  /* this is designed to be robust against
				       the mungling that xv does, but no
				       promises for any other image programs */
#define NRRD_UNKNOWN  "???"         /* how to represent something unknown in
				       a field of the nrrd header, when it
				       being unknown is not an error */

/* extern C */
#ifdef __cplusplus
}
#endif
#endif /* NRRD_DEFINES_HAS_BEEN_INCLUDED */
