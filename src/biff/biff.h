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


#ifndef BIFF_HAS_BEEN_INCLUDED
#define BIFF_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>

#include <air.h>

#define BIFF_MAXKEYLEN 128  /* maximum allowed key length (not counting 
			       the null termination) */

extern void biffSet(char *key, char *err);
extern void biffAdd(char *key, char *err);
extern void biffMaybeAdd(int useBiff, char *key, char *err);
extern int biffCheck(char *key);
extern void biffDone(char *key);
extern void biffMove(char *destKey, char *err, char *srcKey);
extern char *biffGet(char *key);
extern char *biffGetDone(char *key);

/* some common error messages */
#define BIFF_NULL "%s: got NULL pointer"
#define BIFF_NRRDNEW "%s: couldn't create output nrrd struct"
#define BIFF_NRRDALLOC "%s: couldn't allocate space for output nrrd data"

/* extern C */
#ifdef __cplusplus
}
#endif
#endif /* BIFF_HAS_BEEN_INCLUDED */
