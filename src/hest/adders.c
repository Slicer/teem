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
This motivated the r7026 2023-07-06 addition of non-var-args hestOptAdd_nva, which
would have caught the above error.

The underlying issue there, though, is the total lack of type-checking associated with
the var-args functions. Even without var-args, the "void*" type of the value storage
pointer is still a problem. Therefore, the functions in this file help do as much
type-checking as possible with hest.  These functions cover nearly all uses of hest
within Teem (and in GLK's SciVis class), in a way that is specific to the type of the
value storage pointer valueP, which is still a void* even in hestOptAdd_nva.  Many of the
possibilities here are unlikely to be needed (an option for 4 booleans?), but are
generated here for completeness (an option for 4 floats or 4 doubles is great for
R,G,B,A values).

However for airTypeOther (i.e. when the caller passes a hestCB struct of callbacks to
parse arbitrary things from the command-line) there is still unfortunately a
type-checking black hole void* involved.  And, there is no away around that: either
valueP is an array of structs (when hestCB->destroy is NULL) or an array of pointers to
structs (hestCB->destroy is non-NULL), for which the most specific type for valueP would
be either void* or void**, respectively. But void** is not a generic pointer to pointer
type (like void* is the generic pointer type), and, we're not doing compile-time checks
on the non-NULL-ity of hestCB->destroy. So it all devolves back to plain void*. Still,
the hestOptAdd_*_Other function are generated here to slightly simplify the hestOptAdd
call, since there is no more NULL and NULL for sawP and enum.  The only way around this
particular type-checking black hole is still extreme attentiveness.

Two other design points of note:

(1) If we're moving to more type checking, why not make the default values be something
other than a mere string?  Why isn't the default typed in a way analogous to the newly
typed valueP?  This is a great idea, and it was actually briefly tried, but then
abandoned. One of the original good ideas that made hest work (when it was created) was
the recognition that if the point is to get values out of argv strings collected from the
command-line, then you are absolutely unavoidably in the business of parsing values from
strings, at which point you are losing zero expressivity to have the default also
expressed as a string, and, as a happy side-effect, the type of the string was the same
for all cases. So nothing in hest ever learned to *copy* values from a given default
value array, it *only* *parses* values from strings. So couldn't all the functions
generated below take typed default values and generate the default string? Yes, but the
code is annoying, and it seems fishy to have a string-->value route for user-supplied
options, but a value-->string-->value route for options using their default, and it risks
confusion to have the hestGlossary-generated usage info show a default string (and it in
that context it really does need to be a string) that doesn't seem to match the
compiled-time value array.  Also, the cases where the default is not known at compile
(and is instead learned only at run-time) are so rare that the cost of converting that
default value to a string is acceptable. If hest (in Teem v3) learns to copy values from
a given default value array, this may be revisited.

(2) Why the underscores in the names? Teem is pretty consistent about usingCamelCase for
everything, no?  Yes, except when it gets in the way of legibility, like the airInsane_*
enum values, the airFP_* enum values, and nrrdWrap_nva, nrrdAlloc_nva ... (and the
snake_case ell and tijk libraries). The "hestOptAdd" prefix needs to stay the same (we're
not going to have both hestOptAdd() and hest_opt_add_*() in one library, when other
necessary functions like hestOptFree() are not going to change. So then the question is:
which is more reader-friendly: hestOptAddNvUInt or hestOptAdd_Nv_UInt?  Obviously the
second, and that matters more that superficial consistency.
*/

/* --------------------------------------------------------------- 1 == kind */
/* (there is only one kind of kind==1 option) */
unsigned int
hestOptAdd_Flag(hestOpt **hoptP, const char *flag, int *valueP, const char *info) {

  return hestOptAdd_nva(hoptP, flag, NULL /* name */, airTypeInt /* actually moot */,
                        0 /* min */, 0 /* max */, valueP, NULL /* default */, info, /* */
                        NULL, NULL, NULL);
}

/* The number of very similar functions to define here justifies macro tricks,
which follow these naming conventions:

DCL_ = declaration (for the header file)
IMP- = implementation (for this file)
DEF_ = definition = declaration + implementation
_T_ = simple scalar types
_E_ = airEnum
_O_ = Other (needs hestCB)
_1 = single parameter, either fixed (kind 2) or variable (kind 4)
_M = 2, 3, or 4 = COMPILE-TIME fixed # of parameters (kind 3)
     (these exist as a convenience, covering many common hest uses)
_N = RUN_TIME user-given fixed # of parameters (still kind 3)
_V = RUN_TIME variable # of parameters (kind 5)

*/

/* utility concatenators, used to form the name of macros to expand further */
#define CONC(A, B)     A##_##B
#define CONC3(A, B, C) A##_##B##_##C

/*
_1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1
_1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1
_1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1 _1
*/

/* "_1v_" functions (declared via _DCL_T_0) are for a single variable parm opt (kind 4),
    "_1_" functions (declared via _DCL_T_1) are for a single fixed parm opt (kind 2)
*/
#define _DCL_T_0(ATYP, CTYP)                                                            \
  unsigned int hestOptAdd_1v_##ATYP(hestOpt **hoptP, const char *flag,                  \
                                    const char *name, CTYP *valueP, const char *dflt,   \
                                    const char *info)
#define _DCL_T_1(ATYP, CTYP)                                                            \
  unsigned int hestOptAdd_1_##ATYP(hestOpt **hoptP, const char *flag, /* */             \
                                   const char *name, CTYP *valueP, const char *dflt,    \
                                   const char *info)
/* DCL_T_1(M) chooses between _DCL_T_0 and _DCL_T_1 */
#define DCL_T_1(M, ATYP, CTYP) CONC(_DCL_T, M)(ATYP, CTYP)
/* IMP_T_1(M) passes M as minimum # parms;
   M=0 --> kind 4
   M=1 --> kind 2; max # parms is 1 for both  */
#define IMP_T_1(M, ATYP, CTYP)                                                          \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airType##ATYP, M, 1, /* */                 \
                          valueP, dflt, info,                     /* */                 \
                          NULL, NULL, NULL);                                            \
  }
#define DEF_T_1(M, ATYP, CTYP) DCL_T_1(M, ATYP, CTYP) IMP_T_1(M, ATYP, CTYP)
/* copy-pasta for Enum -------------- */
#define _DCL_E_0                                                                        \
  unsigned int hestOptAdd_1v_Enum(hestOpt **hoptP, const char *flag, /* */              \
                                  const char *name, int *valueP, const char *dflt,      \
                                  const char *info, const airEnum *enm)
#define _DCL_E_1                                                                        \
  unsigned int hestOptAdd_1_Enum(hestOpt **hoptP, const char *flag, /* */               \
                                 const char *name, int *valueP, const char *dflt,       \
                                 const char *info, const airEnum *enm)
#define DCL_E_1(M) CONC(_DCL_E, M)
#define IMP_E_1(M)                                                                      \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airTypeEnum, M, 1, /* */                   \
                          valueP, dflt, info,                   /* */                   \
                          NULL, enm, NULL);                                             \
  }
#define DEF_E_1(M) DCL_E_1(M) IMP_E_1(M)
/* copy-pasta for Other -------------- */
#define _DCL_O_0                                                                        \
  unsigned int hestOptAdd_1v_Other(hestOpt **hoptP, const char *flag, /* */             \
                                   const char *name, void *valueP, const char *dflt,    \
                                   const char *info, const hestCB *CB)
#define _DCL_O_1                                                                        \
  unsigned int hestOptAdd_1_Other(hestOpt **hoptP, const char *flag, /* */              \
                                  const char *name, void *valueP, const char *dflt,     \
                                  const char *info, const hestCB *CB)
#define DCL_O_1(M) CONC(_DCL_O, M)
#define IMP_O_1(M)                                                                      \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airTypeOther, M, 1, /* */                  \
                          valueP, dflt, info,                    /* */                  \
                          NULL, NULL, CB);                                              \
  }
#define DEF_O_1(M) DCL_O_1(M) IMP_O_1(M)

/*
_M and _N    _M and _N    _M and _N    _M and _N    _M and _N    _M and _N    _M and _N
_M and _N    _M and _N    _M and _N    _M and _N    _M and _N    _M and _N    _M and _N
_M and _N    _M and _N    _M and _N    _M and _N    _M and _N    _M and _N    _M and _N
*/

/* copy-pasta for _T scalar types for (compile-time) M or (run-time) N fixed parms */
#define DCL_T_M(M, ATYP, CTYP)                                                          \
  unsigned int hestOptAdd_##M##_##ATYP(hestOpt **hoptP, const char *flag,               \
                                       const char *name, CTYP valueP[M],                \
                                       const char *dflt, const char *info)
#define IMP_T_M(M, ATYP, CTYP)                                                          \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airType##ATYP, M, M, valueP, dflt, info,   \
                          NULL, NULL, NULL);                                            \
  }
#define DEF_T_M(M, ATYP, CTYP) DCL_T_M(M, ATYP, CTYP) IMP_T_M(M, ATYP, CTYP)
#define DCL_T_N(_, ATYP, CTYP)                                                          \
  unsigned int hestOptAdd_N_##ATYP(hestOpt **hoptP, const char *flag, const char *name, \
                                   unsigned int N, CTYP *valueP, const char *dflt,      \
                                   const char *info)
#define IMP_T_N(_, ATYP, CTYP)                                                          \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airType##ATYP, N, N, valueP, dflt, info,   \
                          NULL, NULL, NULL);                                            \
  }
#define DEF_T_N(_, ATYP, CTYP) DCL_T_N(_, ATYP, CTYP) IMP_T_N(_, ATYP, CTYP)
/* copy-pasta for _E Enums */
#define DCL_E_M(M)                                                                      \
  unsigned int hestOptAdd_##M##_Enum(hestOpt **hoptP, const char *flag,                 \
                                     const char *name, int valueP[M], const char *dflt, \
                                     const char *info, const airEnum *enm)
#define IMP_E_M(M)                                                                      \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airTypeEnum, M, M, valueP, dflt, info,     \
                          NULL, enm, NULL);                                             \
  }
#define DEF_E_M(M) DCL_E_M(M) IMP_E_M(M)
#define DCL_E_N(_)                                                                      \
  unsigned int hestOptAdd_N_Enum(hestOpt **hoptP, const char *flag, const char *name,   \
                                 unsigned int N, int *valueP, const char *dflt,         \
                                 const char *info, const airEnum *enm)
#define IMP_E_N(_)                                                                      \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airTypeEnum, N, N, valueP, dflt, info,     \
                          NULL, enm, NULL);                                             \
  }
#define DEF_E_N(_) DCL_E_N(_) IMP_E_N(_)
/* copy-pasta for _O Other */
#define DCL_O_M(M)                                                                      \
  unsigned int hestOptAdd_##M##_Other(hestOpt **hoptP, const char *flag,                \
                                      const char *name, void *valueP, const char *dflt, \
                                      const char *info, const hestCB *CB)
#define IMP_O_M(M)                                                                      \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airTypeOther, M, M, valueP, dflt, info,    \
                          NULL, NULL, CB);                                              \
  }
#define DEF_O_M(M) DCL_O_M(M) IMP_O_M(M)
#define DCL_O_N(_)                                                                      \
  unsigned int hestOptAdd_N_Other(hestOpt **hoptP, const char *flag, const char *name,  \
                                  unsigned int N, void *valueP, const char *dflt,       \
                                  const char *info, const hestCB *CB)
#define IMP_O_N(_)                                                                      \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airTypeOther, N, N, valueP, dflt, info,    \
                          NULL, NULL, CB);                                              \
  }
#define DEF_O_N(_) DCL_O_N(_) IMP_O_N(_)

/*
_V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V
_V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V
_V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V _V
*/

#define DCL_T_V(_, ATYP, CTYP)                                                          \
  unsigned int hestOptAdd_Nv_##ATYP(hestOpt **hoptP, const char *flag,                  \
                                    const char *name, unsigned int min, int max,        \
                                    CTYP **valueP, const char *dflt, const char *info,  \
                                    unsigned int *sawP)
#define IMP_T_V(_, ATYP, CTYP)                                                          \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airType##ATYP, min, max, /* */             \
                          valueP, dflt, info,                         /* */             \
                          sawP, NULL, NULL);                                            \
  }
#define DEF_T_V(M, ATYP, CTYP) DCL_T_V(M, ATYP, CTYP) IMP_T_V(M, ATYP, CTYP)
/* copy-pasta for Enum -------------- */
#define DCL_E_V(_)                                                                      \
  unsigned int hestOptAdd_Nv_Enum(hestOpt **hoptP, const char *flag, /* */              \
                                  const char *name, unsigned int min, int max,          \
                                  int **valueP, const char *dflt, const char *info,     \
                                  unsigned int *sawP, const airEnum *enm)
#define IMP_E_V(_)                                                                      \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airTypeEnum, min, max, /* */               \
                          valueP, dflt, info,                       /* */               \
                          sawP, enm, NULL);                                             \
  }
#define DEF_E_V(_) DCL_E_V(_) IMP_E_V(_)
/* copy-pasta for Other -------------- */
#define DCL_O_V(_)                                                                      \
  unsigned int hestOptAdd_Nv_Other(hestOpt **hoptP, const char *flag, /* */             \
                                   const char *name, unsigned int min, int max,         \
                                   void *valueP, const char *dflt, const char *info,    \
                                   unsigned int *sawP, const hestCB *CB)
#define IMP_O_V(_)                                                                      \
  {                                                                                     \
    return hestOptAdd_nva(hoptP, flag, name, airTypeOther, min, max, /* */              \
                          valueP, dflt, info,                        /* */              \
                          sawP, NULL, CB);                                              \
  }
#define DEF_O_V(_) DCL_O_V(_) IMP_O_V(_)

/* MAP_T takes a macro MAC that (like DCL_T_M or DEF_T_M) takes three args
(M, ATYP, CTYP) and applies it to all the simple scalar types */
#define MAP_T(MAC, M)                                                                   \
  MAC(M, Bool, int)                                                                     \
  MAC(M, Int, int)                                                                      \
  MAC(M, UInt, unsigned int)                                                            \
  MAC(M, LongInt, long int)                                                             \
  MAC(M, ULongInt, unsigned long int)                                                   \
  MAC(M, Size_t, size_t)                                                                \
  MAC(M, Float, float)                                                                  \
  MAC(M, Double, double)                                                                \
  MAC(M, Char, char)                                                                    \
  MAC(M, String, char *)

/* MAP(BSN, X, M) takes a macro basename BSN (like DEF) and
expands it to the _T_, _E_, and _O_ cases for _X. E.g. "MAP(DEF, 1, 0)" expands to:
MAP_T(DEF_T_1, 0)
DEF_E_1(0)
DEF_O_1(0)
*/
#define MAP(BSN, X, M) MAP_T(CONC3(BSN, T, X), M) CONC3(BSN, E, X)(M) CONC3(BSN, O, X)(M)

/* This does expansion over all possibilities of BSN */
#define LETS_GET_THIS_PARTY_STARTED(BSN)                                                \
  MAP(BSN, 1, 0)                                                                        \
  MAP(BSN, 1, 1)                                                                        \
  MAP(BSN, M, 2)                                                                        \
  MAP(BSN, M, 3)                                                                        \
  MAP(BSN, M, 4)                                                                        \
  MAP(BSN, N, _)                                                                        \
  MAP(BSN, V, _)

/* !!! HERE IS THE ACTUAL CODE FOR ALL THE hestOptAdd_*_* CASES !!! */
LETS_GET_THIS_PARTY_STARTED(DEF)

/* Macro for making a string out of whatever something has been #define'd to
   https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html */
#define __STR(name) #name
#define _STR(name)  __STR(name)

/* for generating body of hestOptAddDeclsPrint;
NOTE assuming the local variable FILE *ff */
#define PRINT_T_1(M, ATYP, CTYP)                                                        \
  fprintf(ff, "HEST_EXPORT " _STR(DCL_T_1(M, ATYP, CTYP)) ";\n");
#define PRINT_E_1(M) fprintf(ff, "HEST_EXPORT " _STR(DCL_E_1(M)) ";\n");
#define PRINT_O_1(M) fprintf(ff, "HEST_EXPORT " _STR(DCL_O_1(M)) ";\n");
#define PRINT_T_M(M, ATYP, CTYP)                                                        \
  fprintf(ff, "HEST_EXPORT " _STR(DCL_T_M(M, ATYP, CTYP)) ";\n");
#define PRINT_E_M(M) fprintf(ff, "HEST_EXPORT " _STR(DCL_E_M(M)) ";\n");
#define PRINT_O_M(M) fprintf(ff, "HEST_EXPORT " _STR(DCL_O_M(M)) ";\n");
#define PRINT_T_N(_, ATYP, CTYP)                                                        \
  fprintf(ff, "HEST_EXPORT " _STR(DCL_T_N(_, ATYP, CTYP)) ";\n");
#define PRINT_E_N(_) fprintf(ff, "HEST_EXPORT " _STR(DCL_E_N(_)) ";\n");
#define PRINT_O_N(_) fprintf(ff, "HEST_EXPORT " _STR(DCL_O_N(_)) ";\n");
#define PRINT_T_V(_, ATYP, CTYP)                                                        \
  fprintf(ff, "HEST_EXPORT " _STR(DCL_T_V(_, ATYP, CTYP)) ";\n");
#define PRINT_E_V(_) fprintf(ff, "HEST_EXPORT " _STR(DCL_E_V(_)) ";\n");
#define PRINT_O_V(_) fprintf(ff, "HEST_EXPORT " _STR(DCL_O_V(_)) ";\n");

/* prints declarations (for hest.h) for everything defined by macros above */
void
hestOptAddDeclsPrint(FILE *ff) {
  /* the flag is the one case not handled by macro expansion */
  fprintf(ff, "HEST_EXPORT unsigned int hestOptAdd_Flag(hestOpt **optP, "
              "const char *flag, int *valueP, const char *info);\n");
  /* declarations for all other cases */
  LETS_GET_THIS_PARTY_STARTED(PRINT)
}
