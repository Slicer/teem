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


#ifndef LIMN_HAS_BEEN_INCLUDED
#define LIMN_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <air.h>
#include <biff.h>
#include <math.h>

#define LIMN "limn"

/* qn.c */
extern void limn16QNtoV(float *vec, unsigned short qn, int doNorm);
extern unsigned short limnVto16QN(float *vec);
extern void limn16QN1PBtoV(float *vec, unsigned short qn, int doNorm);
extern unsigned short limnVto16QN1PB(float *vec);
extern void limn15QNtoV(float *vec, unsigned short qn, int doNorm);
extern unsigned short limnVto15QN(float *vec);

#ifdef __cplusplus
}
#endif
#endif /* LIMN_HAS_BEEN_INCLUDED */
