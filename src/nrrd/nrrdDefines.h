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
