/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef UNRRDU_HAS_BEEN_INCLUDED
#define UNRRDU_HAS_BEEN_INCLUDED

#include <teem/air.h>
#include <teem/biff.h>
#include <teem/hest.h>
#include <teem/nrrd.h>
/* moss.h is included where it is needed, in ilk.c */

#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(TEEM_STATIC)
#  if defined(TEEM_BUILD) || defined(unrrdu_EXPORTS) || defined(teem_EXPORTS)
#    define UNRRDU_EXPORT extern __declspec(dllexport)
#  else
#    define UNRRDU_EXPORT extern __declspec(dllimport)
#  endif
#else /* TEEM_STATIC || UNIX */
#  define UNRRDU_EXPORT extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define UNRRDU unrrduBiffKey

#define UNRRDU_COLUMNS 78 /* how many chars per line do we allow hest */

/*
******** unrrduCmd
**
** How we associate the one word for the unu command ("name"),
** the one-line info string ("info"), and the single function we
** which implements the command ("main").
*/
typedef struct {
  const char *name, *info;
  int (*main)(int argc, const char **argv, const char *me, hestParm *hparm);
  int hidden;
} unrrduCmd;

/*
** used with unrrduHestScaleCB to identify for "unu resample -s"
** whether to resample along an axis, and if so, how the number of
** samples on an axis should be scaled (though now its more general
** than just scaling) */
enum {
  unrrduScaleUnknown,       /* 0: */
  unrrduScaleNothing,       /* 1: "=" */
  unrrduScaleMultiply,      /* 2: e.g. "x2" */
  unrrduScaleDivide,        /* 3: e.g. "/2" */
  unrrduScaleAdd,           /* 4: e.g. "+2" */
  unrrduScaleSubtract,      /* 5: e.g. "-2" */
  unrrduScaleAspectRatio,   /* 6: "a" */
  unrrduScaleExact,         /* 7: e.g. "128" */
  unrrduScaleSpacingTarget, /* 8: e.g. "s0.89" */
  unrrduScaleLast
};

/* flotsam.c */
UNRRDU_EXPORT const int unrrduPresent;
UNRRDU_EXPORT const char *const unrrduBiffKey;
UNRRDU_EXPORT unsigned int unrrduDefNumColumns;
/* unrrduCmdList[] gives addresses of unrrdu_fooCmd for all commands "unu foo".
   The list of all commands is maintained in privateUnrrdu.h */
UNRRDU_EXPORT const unrrduCmd *const unrrduCmdList[];
UNRRDU_EXPORT int unrrduCmdMain(int argc, const char **argv, const char *cmd,
                                const char *title, const unrrduCmd *const *cmdList,
                                hestParm *hparm, FILE *fusage);
UNRRDU_EXPORT void unrrduUsageUnu(const char *me, hestParm *hparm, int alsoHidden);
UNRRDU_EXPORT int unrrduUsage(const char *me, hestParm *hparm, const char *title,
                              const unrrduCmd *const *cmdList);
UNRRDU_EXPORT const hestCB unrrduHestPosCB;
UNRRDU_EXPORT const hestCB unrrduHestMaybeTypeCB;
UNRRDU_EXPORT const hestCB unrrduHestScaleCB;
UNRRDU_EXPORT const hestCB unrrduHestBitsCB;
UNRRDU_EXPORT const hestCB unrrduHestFileCB;
UNRRDU_EXPORT const hestCB unrrduHestEncodingCB;
UNRRDU_EXPORT const hestCB unrrduHestFormatCB;

#ifdef __cplusplus
}
#endif

#endif /* UNRRDU_HAS_BEEN_INCLUDED */
