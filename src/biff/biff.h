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

#ifdef __cplusplus
extern "C" {
#endif


#ifndef BIFF_HAS_BEEN_INCLUDED
#define BIFF_HAS_BEEN_INCLUDED

#include <stdio.h>
#include <string.h>

#include <air.h>

#define BIFF_MAXKEYLEN 128  /* maximum allowed key length (not counting 
			       the null termination) */

extern void biffSet(char *key, char *err);
extern void biffAdd(char *key, char *err);
extern void biffMaybeAdd(char *key, char *err, int useBiff);
extern int biffCheck(char *key);
extern void biffDone(char *key);
extern void biffMove(char *destKey, char *err, char *srcKey);
extern char *biffGet(char *key);
extern char *biffGetDone(char *key);

#endif /* BIFF_HAS_BEEN_INCLUDED */

#ifdef __cplusplus
}
#endif
