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

#ifdef __cplusplus
extern "C" {
#endif

/*
** This private header was created because the following two "VL" functions are used
** only within in the biff sources. They take a va_list, which is unusual, and
** (currently) used for no other public functions in Teem.
**
** Furthermore, pre-1.13 release it became apparent that nothing else in Teem (outside
** of biff) was using any biffMsg anything, so these were also all moved to here,
** though out of laziness no _ prefix was added (as is expected of "private" text
** symbols in the library)
*/

/* biffmsg.c */
extern void _biffMsgAddVL(biffMsg *msg, const char *errfmt, va_list args);
extern void _biffMsgMoveVL(biffMsg *dest, biffMsg *src, const char *errfmt,
                           va_list args);

extern biffMsg *biffMsgNew(const char *key);
extern biffMsg *biffMsgNix(biffMsg *msg);
extern void biffMsgAdd(biffMsg *msg, const char *err);
extern void biffMsgClear(biffMsg *msg);
extern void biffMsgMove(biffMsg *dest, biffMsg *src, const char *err);
/* ---- BEGIN non-NrrdIO */
extern void biffMsgAddf(biffMsg *msg, const char *errfmt, ...)
#ifdef __GNUC__
  __attribute__((format(printf, 2, 3)))
#endif
  ;
extern void biffMsgMovef(biffMsg *dest, biffMsg *src, const char *errfmt, ...)
#ifdef __GNUC__
  __attribute__((format(printf, 3, 4)))
#endif
  ;
/* ---- END non-NrrdIO */
extern unsigned int biffMsgErrNum(const biffMsg *msg);
extern unsigned int biffMsgStrlen(const biffMsg *msg);
extern void biffMsgStrSet(char *ret, const biffMsg *msg);
/* ---- BEGIN non-NrrdIO */
extern char *biffMsgStrGet(const biffMsg *msg);
/* ---- END non-NrrdIO */

#ifdef __cplusplus
}
#endif
