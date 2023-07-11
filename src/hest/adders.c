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

#include "hest.h"
#include "privateHest.h"

/*
Since r6184 2014-03-17, GLK has noted (in ../TODO.txt):
  (from tendGlyph.c): there needs to be an alternative API for hest
  that is not var-args based (as is hestOptAdd). You can't tell when
  you've passed multiple strings for the detailed usage information by
  accident.  GLK had accidentally inserted a comma into my multi-line
  string for the "info" arg, relying on the automatic string
  concatenation, and ended up passing total garbage to hestOptAdd for
  the airEnum pointer, causing him to think that the tenGlyphType airEnum
  was malformed, when it was in fact fine ...
This motivated the r7026 2023-07-06 addition of hestOptAdd_nva, which
would have caught the above error.

The underlying issue there, though, is the total lack of type-checking associated with
the var-args functions.  The functions in this file help do as much type-checking as
possible with hest.  These functions cover nearly all uses of hest within Teem (and in
GLK's SciVIs class), in a way that is specific to the type of the value storage pointer
valueP, which is still a void* even in hestOptAdd_nva.  Many of the possibilities here
are unlikely to be needed (an option for 4 booleans?), but are generated here for
completeness (an option for 4 floats or 4 doubles is great for R,G,B,A).

However with airTypeOther, in which case the caller passes a hestCB struct of callbacks
to parse arbitrary things from the command-line, there is still unfortunately a
type-checking black hole void* involved.  And, there is no away around that:
the non-NULL-ity of hestCB->destroy determines wether the thing being parsed is
merely space to be initialized (valueP is an array of structs), versus a
struct to be allocated (valueP is an array of pointers to structs), we want that to
determine the type of valueP.  But the struct itself as to be void type, and void** is
not a generic pointer to pointer type (like void* is the generic pointer type). Still
the _Other versions of the function are generated here to slightly simplify the
hestOptAdd call (no more NULL, NULL for sawP and enum).  Actually, there is around
a type-checking black hole: extreme attentiveness!
*/

/* --------------------------------------------------------------- 1 == kind */
unsigned int
hestOptAdd_Flag(hestOpt **hoptP, const char *flag, int *valueP, const char *info) {

  return hestOptAdd_nva(hoptP, flag, NULL /* name */, airTypeInt /* actually moot */,
                        0 /* min */, 0 /* max */, valueP, NULL /* default */, info, /* */
                        NULL, NULL, NULL);
}

/* --------------------------------------------------------------- 2 == kind */
unsigned int
hestOptAdd_1_Bool(hestOpt **hoptP, const char *flag, const char *name, /* */
                  int *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(hoptP, flag, name, airTypeBool, 1, 1, /* */
                        valueP, dflt, info,                   /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_Int(hestOpt **hoptP, const char *flag, const char *name, /* */
                 int *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(hoptP, flag, name, airTypeInt, 1, 1, /* */
                        valueP, dflt, info,                  /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_UInt(hestOpt **hoptP, const char *flag, const char *name, /* */
                  unsigned int *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(hoptP, flag, name, airTypeUInt, 1, 1, /* */
                        valueP, dflt, info,                   /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_LongInt(hestOpt **hoptP, const char *flag, const char *name, /* */
                     long int *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(hoptP, flag, name, airTypeLongInt, 1, 1, /* */
                        valueP, dflt, info,                      /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_ULongInt(hestOpt **hoptP, const char *flag, const char *name, /* */
                      unsigned long int *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(hoptP, flag, name, airTypeULongInt, 1, 1, /* */
                        valueP, dflt, info,                       /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_Size_t(hestOpt **hoptP, const char *flag, const char *name, /* */
                    size_t *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(hoptP, flag, name, airTypeSize_t, 1, 1, /* */
                        valueP, dflt, info,                     /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_Float(hestOpt **hoptP, const char *flag, const char *name, /* */
                   float *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(hoptP, flag, name, airTypeFloat, 1, 1, /* */
                        valueP, dflt, info,                    /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_Double(hestOpt **hoptP, const char *flag, const char *name, /* */
                    double *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(hoptP, flag, name, airTypeDouble, 1, 1, /* */
                        valueP, dflt, info,                     /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_Char(hestOpt **hoptP, const char *flag, const char *name, /* */
                  char *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(hoptP, flag, name, airTypeChar, 1, 1, /* */
                        valueP, dflt, info,                   /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_String(hestOpt **hoptP, const char *flag, const char *name, /* */
                    char **valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(hoptP, flag, name, airTypeString, 1, 1, /* */
                        valueP, dflt, info,                     /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_Enum(hestOpt **hoptP, const char *flag, const char *name, /* */
                  int *valueP, const char *dflt, const char *info,     /* */
                  const airEnum *enm) {

  return hestOptAdd_nva(hoptP, flag, name, airTypeEnum, 1, 1, /* */
                        valueP, dflt, info,                   /* */
                        NULL, enm, NULL);
}

unsigned int
hestOptAdd_1_Other(hestOpt **hoptP, const char *flag, const char *name, /* */
                   void *valueP, const char *dflt, const char *info,    /* */
                   const hestCB *CB) {
  return hestOptAdd_nva(hoptP, flag, name, airTypeOther, 1, 1, /* */
                        valueP, dflt, info,                    /* */
                        NULL, NULL, CB);
}

/* --------------------------------------------------------------- 2 == kind */

/* for some reason writing out code above (and their declarations in hest.h) by hand was
tolerated, but from here on out the coding is going to use a lot of macro tricks, with
these name conventions:

M = 2, 3, or 4 = fixed # of parameters
N = user-given fixed # of parameters
_S = simple scalar types
_E = airEnum
_O = Other

Some way of gracefully handling the 10 different simple types, plus the airEnum and
Other, with the context of the functional-ish MAP macros, is surely possible, but it
eludes GLK at this time */

/* _S: simple scalar types */
#define DCL_M_S(M, ATYP, CTYP)                                                          \
  unsigned int hestOptAdd_##M##_##ATYP(hestOpt **hoptP, const char *flag,               \
                                       const char *name, CTYP valueP[M],                \
                                       const char *dflt, const char *info)
#define BODY_M_S(M, ATYP, CTYP)                                                         \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airType##ATYP, M, M, valueP, dflt, info,   \
                          NULL, NULL, NULL);                                            \
  }
#define DEF_M_S(M, ATYP, CTYP) DCL_M_S(M, ATYP, CTYP) BODY_M_S(M, ATYP, CTYP)
#define DCL_N_S(ATYP, CTYP)                                                             \
  unsigned int hestOptAdd_N_##ATYP(hestOpt **hoptP, const char *flag, const char *name, \
                                   unsigned int N, CTYP *valueP, const char *dflt,      \
                                   const char *info)
#define BODY_N_S(ATYP, CTYP)                                                            \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airType##ATYP, N, N, valueP, dflt, info,   \
                          NULL, NULL, NULL);                                            \
  }
#define DEF_N_S(ATYP, CTYP) DCL_N_S(ATYP, CTYP) BODY_N_S(ATYP, CTYP)

/* _E: Enum */
#define DCL_M_E(M)                                                                      \
  unsigned int hestOptAdd_##M##_Enum(hestOpt **hoptP, const char *flag,                 \
                                     const char *name, int valueP[M], const char *dflt, \
                                     const char *info, const airEnum *enm)
#define BODY_M_E(M)                                                                     \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airTypeEnum, M, M, valueP, dflt, info,     \
                          NULL, enm, NULL);                                             \
  }
#define DEF_M_E(M) DCL_M_E(M) BODY_M_E(M)
#define DCL_N_E                                                                         \
  unsigned int hestOptAdd_N_Enum(hestOpt **hoptP, const char *flag, const char *name,   \
                                 unsigned int N, int *valueP, const char *dflt,         \
                                 const char *info, const airEnum *enm)
#define BODY_N_E                                                                        \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airTypeEnum, N, N, valueP, dflt, info,     \
                          NULL, enm, NULL);                                             \
  }
#define DEF_N_E DCL_N_E BODY_N_E

/* _O: Other */
#define DCL_M_O(M)                                                                      \
  unsigned int hestOptAdd_##M##_Other(hestOpt **hoptP, const char *flag,                \
                                      const char *name, void *valueP, const char *dflt, \
                                      const char *info, const hestCB *CB)
#define BODY_M_O(M)                                                                     \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airTypeOther, M, M, valueP, dflt, info,    \
                          NULL, NULL, CB);                                              \
  }
#define DEF_M_O(M) DCL_M_O(M) BODY_M_O(M)
#define DCL_N_O                                                                         \
  unsigned int hestOptAdd_N_Other(hestOpt **hoptP, const char *flag, const char *name,  \
                                  unsigned int N, void *valueP, const char *dflt,       \
                                  const char *info, const hestCB *CB)
#define BODY_N_O                                                                        \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airTypeOther, N, N, valueP, dflt, info,    \
                          NULL, NULL, CB);                                              \
  }
#define DEF_N_O DCL_N_O BODY_N_O

/* MAP_M_S takes a macro MMAC that (like DCL_M_S or DEF_M_S) takes three args
-- M, ATYP, CTYP -- and applies it to all the simple scalar types.
MAP_N_S takes a macro NMAC (like DCL_N_S or DEF_N_S) that takes just two args
-- ATYP, CTYPE -- and applies to the scalar types */
#define MAP_M_S(MMAC, M)                                                                \
  MMAC(M, Bool, int)                                                                    \
  MMAC(M, Int, int)                                                                     \
  MMAC(M, UInt, unsigned int)                                                           \
  MMAC(M, LongInt, long int)                                                            \
  MMAC(M, ULongInt, unsigned long int)                                                  \
  MMAC(M, Size_t, size_t)                                                               \
  MMAC(M, Float, float)                                                                 \
  MMAC(M, Double, double)                                                               \
  MMAC(M, Char, char)                                                                   \
  MMAC(M, String, char *)
/* (yes would be nicer to avoid copy-pasta, but how?) */
#define MAP_N_S(NMAC)                                                                   \
  NMAC(Bool, int)                                                                       \
  NMAC(Int, int)                                                                        \
  NMAC(UInt, unsigned int)                                                              \
  NMAC(LongInt, long int)                                                               \
  NMAC(ULongInt, unsigned long int)                                                     \
  NMAC(Size_t, size_t)                                                                  \
  NMAC(Float, float)                                                                    \
  NMAC(Double, double)                                                                  \
  NMAC(Char, char)                                                                      \
  NMAC(String, char *)

/* v.v.v.v.v.v.v.v.v   Actual code!   v.v.v.v.v.v.v.v.v */
MAP_M_S(DEF_M_S, 2)
DEF_M_E(2)
DEF_M_O(2)
MAP_M_S(DEF_M_S, 3)
DEF_M_E(3)
DEF_M_O(3)
MAP_M_S(DEF_M_S, 4)
DEF_M_E(4)
DEF_M_O(4)
MAP_N_S(DEF_N_S)
DEF_N_E
DEF_N_O
/* ^'^'^'^'^'^'^'^'^'^'^'^'^'^'^'^'^'^'^'^'^'^'^'^'^'^'^ */

/* Macro for making a string out of whatever something has been #define'd to, exactly,
   without chasing down a sequence of #includes.
   https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html */
#define __STR(name) #name
#define _STR(name)  __STR(name)

/* for generating body of hestOptAddDeclsPrint;
NOTE assuming the local FILE *ff */
#define PRINT_M_S(M, ATYP, CTYP)                                                        \
  fprintf(ff, "HEST_EXPORT " _STR(DCL_M_S(M, ATYP, CTYP)) ";\n");
#define PRINT_M_E(M) fprintf(ff, "HEST_EXPORT " _STR(DCL_M_E(M)) ";\n");
#define PRINT_M_O(M) fprintf(ff, "HEST_EXPORT " _STR(DCL_M_O(M)) ";\n");
#define PRINT_N_S(ATYP, CTYP)                                                           \
  fprintf(ff, "HEST_EXPORT " _STR(DCL_N_S(ATYP, CTYP)) ";\n");
#define PRINT_N_E fprintf(ff, "HEST_EXPORT " _STR(DCL_N_E) ";\n");
#define PRINT_N_O fprintf(ff, "HEST_EXPORT " _STR(DCL_N_O) ";\n");

/* prints declarations for everything defined by macro above, which
HEY does not include the hestOptAdd_Flag and hestOptAdd_1_* functions */
void
hestOptAddDeclsPrint(FILE *ff) {
  /* HEY copy-pasta from "Actual code" above */
  MAP_M_S(PRINT_M_S, 2)
  PRINT_M_E(2)
  PRINT_M_O(2)
  MAP_M_S(PRINT_M_S, 3)
  PRINT_M_E(3)
  PRINT_M_O(3)
  MAP_M_S(PRINT_M_S, 4)
  PRINT_M_E(4)
  PRINT_M_O(4)
  MAP_N_S(PRINT_N_S)
  PRINT_N_E
  PRINT_N_O
}

/*
hestOptSetXX(hestOpt *opt, )
1v<T>, Nv<T>  need sawP

<T>=
Bool, Int, UInt, LongInt, ULongInt, Size_t,
Float, Double, Char, String,
Enum,  need Enum
Other,  need CB
*/
