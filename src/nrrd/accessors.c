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

#include "nrrd.h"
#include "privateNrrd.h"
#include "float.h"

/* 
** making these typedefs here allows us to used one token for both
** constructing function names, and for specifying argument types
*/
typedef signed char CH;
typedef unsigned char UC;
typedef signed short SH;
typedef unsigned short US;
/* Microsoft apparently uses 'IN' as a keyword, so we changed 'IN' to 'JN'. */
typedef signed int JN;
typedef unsigned int UI;
typedef airLLong LL;
/* ui64 to double conversion is not implemented, sorry */
#if _MSC_VER < 1300
typedef airLLong UL;
#else
typedef airULLong UL;
#endif
typedef float FL;
typedef double DB;

#define MAP(F, A) \
F(A, CH) \
F(A, UC) \
F(A, SH) \
F(A, US) \
F(A, JN) \
F(A, UI) \
F(A, LL) \
F(A, UL) \
F(A, FL) \
F(A, DB)

/* 
** _nrrdLoad<TA><TB>(<TB> *v)
**
** Dereferences v as TB*, casts it to TA, returns it.
*/
#define LOAD_DEF(TA, TB)                    \
TA                                          \
_nrrdLoad##TA##TB(TB *v) {                  \
  return *v;                                \
}
#define LOAD_LIST(TA, TB)                   \
  (TA (*)(void *))_nrrdLoad##TA##TB,

MAP(LOAD_DEF, JN)
MAP(LOAD_DEF, FL)
MAP(LOAD_DEF, DB)

int (*
nrrdILoad[NRRD_TYPE_MAX+1])(void*) = {
  NULL, MAP(LOAD_LIST, JN) NULL
};
float (*
nrrdFLoad[NRRD_TYPE_MAX+1])(void*) = {
  NULL, MAP(LOAD_LIST, FL) NULL
};
double (*
nrrdDLoad[NRRD_TYPE_MAX+1])(void*) = {
  NULL, MAP(LOAD_LIST, DB) NULL
};


/*
** _nrrdStore<TA><TB>(<TB> *v, <TA> j)
**
** Takes a TA j, and stores it in *v, thereby implicitly casting it to TB.
** Returns the result of the assignment, which may not be the same as
** the value that was passed in.
*/
#define STORE_DEF(TA, TB)                   \
TA                                          \
_nrrdStore##TA##TB(TB *v, TA j) {           \
  return (*v = j);                          \
}
#define STORE_LIST(TA, TB)                  \
  (TA (*)(void *, TA))_nrrdStore##TA##TB,

MAP(STORE_DEF, JN)
MAP(STORE_DEF, FL)
MAP(STORE_DEF, DB)

int (*
nrrdIStore[NRRD_TYPE_MAX+1])(void *, int) = {
  NULL, MAP(STORE_LIST, JN) NULL
};
float (*
nrrdFStore[NRRD_TYPE_MAX+1])(void *, float) = {
  NULL, MAP(STORE_LIST, FL) NULL
};
double (*
nrrdDStore[NRRD_TYPE_MAX+1])(void *, double) = {
  NULL, MAP(STORE_LIST, DB) NULL
};


/*
** _nrrdLookup<TA><TB>(<TB> *v, size_t I)
**
** Looks up element I of TB array v, and returns it cast to a TA.
*/
#define LOOKUP_DEF(TA, TB)                    \
TA                                            \
_nrrdLookup##TA##TB(TB *v, size_t I) {        \
  return v[I];                                \
}
#define LOOKUP_LIST(TA, TB)                   \
  (TA (*)(void*, size_t))_nrrdLookup##TA##TB,

MAP(LOOKUP_DEF, JN)
MAP(LOOKUP_DEF, FL)
MAP(LOOKUP_DEF, DB)

int (*
nrrdILookup[NRRD_TYPE_MAX+1])(void *, size_t) = {
  NULL, MAP(LOOKUP_LIST, JN) NULL
};
float (*
nrrdFLookup[NRRD_TYPE_MAX+1])(void *, size_t) = {
  NULL, MAP(LOOKUP_LIST, FL) NULL
};
double (*
nrrdDLookup[NRRD_TYPE_MAX+1])(void *, size_t) = {
  NULL, MAP(LOOKUP_LIST, DB) NULL
};


/*
** _nrrdInsert<TA><TB>(<TB> *v, size_t I, <TA> j)
**
** Given TA j, stores it in v[i] (implicitly casting to TB).
** Returns the result of the assignment, which may not be the same as
** the value that was passed in.
*/
#define INSERT_DEF(TA, TB)                         \
TA                                                 \
_nrrdInsert##TA##TB(TB *v, size_t I, TA j) {       \
  return (v[I] = j);                               \
}
#define INSERT_LIST(TA, TB)                        \
  (TA (*)(void*, size_t, TA))_nrrdInsert##TA##TB,

MAP(INSERT_DEF, JN)
MAP(INSERT_DEF, FL)
MAP(INSERT_DEF, DB)

int (*
nrrdIInsert[NRRD_TYPE_MAX+1])(void *, size_t, int) = {
  NULL, MAP(INSERT_LIST, JN) NULL
};
float (*
nrrdFInsert[NRRD_TYPE_MAX+1])(void *, size_t, float) = {
  NULL, MAP(INSERT_LIST, FL) NULL
};
double (*
nrrdDInsert[NRRD_TYPE_MAX+1])(void *, size_t, double) = {
  NULL, MAP(INSERT_LIST, DB) NULL
};


/*
******** nrrdSprint
**
** Dereferences pointer v and sprintf()s that value into given string s,
** returns the result of sprintf()
*/
int _nrrdSprintCH(char *s, CH *v) { return sprintf(s, "%d", *v); }
int _nrrdSprintUC(char *s, UC *v) { return sprintf(s, "%u", *v); }
int _nrrdSprintSH(char *s, SH *v) { return sprintf(s, "%d", *v); }
int _nrrdSprintUS(char *s, US *v) { return sprintf(s, "%u", *v); }
int _nrrdSprintIN(char *s, JN *v) { return sprintf(s, "%d", *v); }
int _nrrdSprintUI(char *s, UI *v) { return sprintf(s, "%u", *v); }
int _nrrdSprintLL(char *s, LL *v) { return sprintf(s, AIR_LLONG_FMT, *v); }
int _nrrdSprintUL(char *s, UL *v) { return sprintf(s, AIR_ULLONG_FMT, *v); }
/* HEY: sizeof(float) and sizeof(double) assumed here, since we're 
   basing "8" and "17" on 6 == FLT_DIG and 15 == DBL_DIG, which are 
   digits of precision for floats and doubles, respectively */
int _nrrdSprintFL(char *s, FL *v) {
  return airSinglePrintf(NULL, s, "%.8g", (double)(*v)); }
int _nrrdSprintDB(char *s, DB *v) {
  return airSinglePrintf(NULL, s, "%.17g", *v); }
int (*
nrrdSprint[NRRD_TYPE_MAX+1])(char *, void *) = {
  NULL,
  (int (*)(char *, void *))_nrrdSprintCH,
  (int (*)(char *, void *))_nrrdSprintUC,
  (int (*)(char *, void *))_nrrdSprintSH,
  (int (*)(char *, void *))_nrrdSprintUS,
  (int (*)(char *, void *))_nrrdSprintIN,
  (int (*)(char *, void *))_nrrdSprintUI,
  (int (*)(char *, void *))_nrrdSprintLL,
  (int (*)(char *, void *))_nrrdSprintUL,
  (int (*)(char *, void *))_nrrdSprintFL,
  (int (*)(char *, void *))_nrrdSprintDB,
  NULL};

/*
******** nrrdFprint
**
** Dereferences pointer v and fprintf()s that value into given file f;
** returns the result of fprintf()
*/
int _nrrdFprintCH(FILE *f, CH *v) { return fprintf(f, "%d", *v); }
int _nrrdFprintUC(FILE *f, UC *v) { return fprintf(f, "%u", *v); }
int _nrrdFprintSH(FILE *f, SH *v) { return fprintf(f, "%d", *v); }
int _nrrdFprintUS(FILE *f, US *v) { return fprintf(f, "%u", *v); }
int _nrrdFprintIN(FILE *f, JN *v) { return fprintf(f, "%d", *v); }
int _nrrdFprintUI(FILE *f, UI *v) { return fprintf(f, "%u", *v); }
int _nrrdFprintLL(FILE *f, LL *v) { return fprintf(f, AIR_LLONG_FMT, *v); }
int _nrrdFprintUL(FILE *f, UL *v) { return fprintf(f, AIR_ULLONG_FMT, *v); }
int _nrrdFprintFL(FILE *f, FL *v) {
  return airSinglePrintf(f, NULL, "%.8g", (double)(*v)); }
int _nrrdFprintDB(FILE *f, DB *v) {
  return airSinglePrintf(f, NULL, "%.17g", *v); }
int (*
nrrdFprint[NRRD_TYPE_MAX+1])(FILE *, void *) = {
  NULL,
  (int (*)(FILE *, void *))_nrrdFprintCH,
  (int (*)(FILE *, void *))_nrrdFprintUC,
  (int (*)(FILE *, void *))_nrrdFprintSH,
  (int (*)(FILE *, void *))_nrrdFprintUS,
  (int (*)(FILE *, void *))_nrrdFprintIN,
  (int (*)(FILE *, void *))_nrrdFprintUI,
  (int (*)(FILE *, void *))_nrrdFprintLL,
  (int (*)(FILE *, void *))_nrrdFprintUL,
  (int (*)(FILE *, void *))_nrrdFprintFL,
  (int (*)(FILE *, void *))_nrrdFprintDB,
  NULL};

/*
******** nrrdFClamp
**
** for integral types, clamps a given float to the range representable
** by by that type; for floating point types we just return
** the given number, since every float must fit in a double.
*/
float _nrrdFClampCH(FL v) { return AIR_CLAMP(SCHAR_MIN, v, SCHAR_MAX);}
float _nrrdFClampUC(FL v) { return AIR_CLAMP(0, v, UCHAR_MAX);}
float _nrrdFClampSH(FL v) { return AIR_CLAMP(SHRT_MIN, v, SHRT_MAX);}
float _nrrdFClampUS(FL v) { return AIR_CLAMP(0, v, USHRT_MAX);}
float _nrrdFClampIN(FL v) { return AIR_CLAMP(INT_MIN, v, INT_MAX);}
float _nrrdFClampUI(FL v) { return AIR_CLAMP(0, v, UINT_MAX);}
float _nrrdFClampLL(FL v) { return AIR_CLAMP(NRRD_LLONG_MIN, v, 
					     NRRD_LLONG_MAX);}
float _nrrdFClampUL(FL v) { return AIR_CLAMP(0, v, NRRD_ULLONG_MAX);}
float _nrrdFClampFL(FL v) { return v; }
float _nrrdFClampDB(FL v) { return v; }
float (*
nrrdFClamp[NRRD_TYPE_MAX+1])(FL) = {
  NULL,
  _nrrdFClampCH,
  _nrrdFClampUC,
  _nrrdFClampSH,
  _nrrdFClampUS,
  _nrrdFClampIN,
  _nrrdFClampUI,
  _nrrdFClampLL,
  _nrrdFClampUL,
  _nrrdFClampFL,
  _nrrdFClampDB,
  NULL};

/*
******** nrrdDClamp
**
** same as nrrdDClamp, but for doubles.  One change: in the case of
** floats, doubles are clamped to the range -FLT_MAX to FLT_MAX.
*/
double _nrrdDClampCH(DB v) { return AIR_CLAMP(SCHAR_MIN, v, SCHAR_MAX);}
double _nrrdDClampUC(DB v) { return AIR_CLAMP(0, v, UCHAR_MAX);}
double _nrrdDClampSH(DB v) { return AIR_CLAMP(SHRT_MIN, v, SHRT_MAX);}
double _nrrdDClampUS(DB v) { return AIR_CLAMP(0, v, USHRT_MAX);}
double _nrrdDClampIN(DB v) { return AIR_CLAMP(INT_MIN, v, INT_MAX);}
double _nrrdDClampUI(DB v) { return AIR_CLAMP(0, v, UINT_MAX);}
double _nrrdDClampLL(DB v) { return AIR_CLAMP(NRRD_LLONG_MIN, v, 
					      NRRD_LLONG_MAX);}
double _nrrdDClampUL(DB v) { return AIR_CLAMP(0, v, NRRD_ULLONG_MAX);}
double _nrrdDClampFL(DB v) { return AIR_CLAMP(-FLT_MAX, v, FLT_MAX); }
double _nrrdDClampDB(DB v) { return v; }
double (*
nrrdDClamp[NRRD_TYPE_MAX+1])(DB) = {
  NULL,
  _nrrdDClampCH,
  _nrrdDClampUC,
  _nrrdDClampSH,
  _nrrdDClampUS,
  _nrrdDClampIN,
  _nrrdDClampUI,
  _nrrdDClampLL,
  _nrrdDClampUL,
  _nrrdDClampFL,
  _nrrdDClampDB,
  NULL};

/* about here is where Gordon admits he might have some use for C++ */

#define _MM_ARGS(type) type *minP, type *maxP, Nrrd *nrrd

#define _MM_FIXED(type)                                                  \
  size_t I, N;                                                           \
  type a, b, min, max, *v;                                               \
                                                                         \
  if (!(minP && maxP))                                                   \
    return;                                                              \
                                                                         \
  /* all fixed-point values exist */                                     \
  nrrd->hasNonExist = nrrdNonExistFalse;                                 \
                                                                         \
  /* set the local data pointer */                                       \
  v = (type*)(nrrd->data);                                               \
                                                                         \
  /* get initial values */                                               \
  N = nrrdElementNumber(nrrd);                                           \
  min = max = v[0];                                                      \
                                                                         \
  /* run through array in pairs; by doing a compare on successive        \
     elements, we can do three compares per pair instead of the naive    \
     four.  In one very unexhaustive test on irix6.64, this resulted     \
     in a 20% decrease in running time.  I learned this trick from       \
     Numerical Recipes in C, long time ago, but I can't find it          \
     anywhere in the book now ... */                                     \
  for (I=0; I<=N-2; I+=2) {                                              \
    a = v[0 + I];                                                        \
    b = v[1 + I];                                                        \
    if (a < b) {                                                         \
      min = AIR_MIN(a, min);                                             \
      max = AIR_MAX(b, max);                                             \
    } else {                                                             \
      max = AIR_MAX(a, max);                                             \
      min = AIR_MIN(b, min);                                             \
    }                                                                    \
  }                                                                      \
                                                                         \
  /* get the very last one (may be redundant) */                         \
  a = v[N-1];                                                            \
  min = AIR_MIN(a, min);                                                 \
  max = AIR_MAX(a, max);                                                 \
                                                                         \
  /* record results */                                                   \
  *minP = min;                                                           \
  *maxP = max;

#define _MM_FLOAT(type)                                                  \
  size_t I, N;                                                           \
  type a, min, max, *v;                                                  \
                                                                         \
  if (!(minP && maxP))                                                   \
    return;                                                              \
                                                                         \
  /* this may be over-written below */                                   \
  nrrd->hasNonExist = nrrdNonExistFalse;                                 \
                                                                         \
  /* set the local data pointer */                                       \
  N = nrrdElementNumber(nrrd);                                           \
  v = (type*)(nrrd->data);                                               \
                                                                         \
  /* we have to explicitly search for the first non-NaN value */         \
  max = min = AIR_NAN;                                                   \
  for (I=0; I<N; I++) {                                                  \
    a = v[I];                                                            \
    if (AIR_EXISTS(a)) {                                                 \
      min = max = a;                                                     \
      break;                                                             \
    } else {                                                             \
      nrrd->hasNonExist = nrrdNonExistTrue;                              \
    }                                                                    \
  }                                                                      \
  /* we continue searching knowing something to compare against, but     \
     still checking AIR_EXISTS at each value.  We don't want an          \
     infinity to corrupt min or max, in accordance with the stated       \
     behavior of nrrdMinMaxSet() */                                      \
  for (I=I+1; I<N; I++) {                                                \
    a = v[I];                                                            \
    if (AIR_EXISTS(a)) {                                                 \
      if (a < min) {                                                     \
        min = a;                                                         \
      } else {                                                           \
        if (a > max) {                                                   \
          max = a;                                                       \
        }                                                                \
      }                                                                  \
    } else {                                                             \
      nrrd->hasNonExist = nrrdNonExistTrue;                              \
    }                                                                    \
  }                                                                      \
                                                                         \
  *minP = min;                                                           \
  *maxP = max;                                                           \

void _nrrdMinMaxCH (_MM_ARGS(CH)) {_MM_FIXED(CH)}
void _nrrdMinMaxUC (_MM_ARGS(UC)) {_MM_FIXED(UC)}
void _nrrdMinMaxSH (_MM_ARGS(SH)) {_MM_FIXED(SH)}
void _nrrdMinMaxUS (_MM_ARGS(US)) {_MM_FIXED(US)}
void _nrrdMinMaxIN (_MM_ARGS(JN)) {_MM_FIXED(JN)}
void _nrrdMinMaxUI (_MM_ARGS(UI)) {_MM_FIXED(UI)}
void _nrrdMinMaxLL (_MM_ARGS(LL)) {_MM_FIXED(LL)}
void _nrrdMinMaxUL (_MM_ARGS(UL)) {_MM_FIXED(UL)}
void _nrrdMinMaxFL (_MM_ARGS(FL)) {_MM_FLOAT(FL)}
void _nrrdMinMaxDB (_MM_ARGS(DB)) {_MM_FLOAT(DB)}

/*
******** nrrdFindMinMax[]
**
** the role of these is to allow finding the EXACT min and max of a nrrd,
** so that one does not have to rely on the potentially lossy storage
** of the min and max values in nrrd->min and nrrd->max, which are doubles.
**
** As such, it is purposely NOT named nrrdMinMaxFind, since that would
** be too similar to nrrdMinMaxSet, which sets the nrrd->min, nrrd->max
** fields.
** 
** These functions have as a side-effect the setting of nrrd->hasNonExist
*/
void (*
nrrdFindMinMax[NRRD_TYPE_MAX+1])(void *, void *, Nrrd *) = {
  NULL,
  (void (*)(void *, void *, Nrrd *))_nrrdMinMaxCH,
  (void (*)(void *, void *, Nrrd *))_nrrdMinMaxUC,
  (void (*)(void *, void *, Nrrd *))_nrrdMinMaxSH,
  (void (*)(void *, void *, Nrrd *))_nrrdMinMaxUS,
  (void (*)(void *, void *, Nrrd *))_nrrdMinMaxIN,
  (void (*)(void *, void *, Nrrd *))_nrrdMinMaxUI,
  (void (*)(void *, void *, Nrrd *))_nrrdMinMaxLL,
  (void (*)(void *, void *, Nrrd *))_nrrdMinMaxUL,
  (void (*)(void *, void *, Nrrd *))_nrrdMinMaxFL,
  (void (*)(void *, void *, Nrrd *))_nrrdMinMaxDB,
  NULL
};

/*
******** nrrdValCompare[]
**
** the sort of compare you'd give to qsort() to sort in ascending order:
** return < 0 if A < B,
**          0 if A == B,
**        > 0 if A > B
** The non-trivial part of this is that for floating-point values, we
** dictate that all non-existent values are smaller than all existent
** values, regardless of their actual values (so +infinity < -42).  This
** is to make sure that we have comparison that won't confuse qsort(),
** which underlies _nrrdMeasureMedian(), and to make it easier to seperate
** existant from non-existant values.
*/
#define _VC_ARGS(type) const type *A, const type *B
#define _VC_FIXED (*A < *B ? -1 : (*A > *B ? 1 : 0))
#define _VC_FLOAT                                                        \
  int ex, ret;                                                           \
                                                                         \
  ex = AIR_EXISTS(*A) + AIR_EXISTS(*B);                                  \
  switch (ex) {                                                          \
  case 2: ret = _VC_FIXED; break;                                        \
  case 1: ret = AIR_EXISTS(*A) ? 1 : -1; break;                          \
  case 0: default: ret = 0;                                              \
  }                                                                      \
  return ret;

int _nrrdValCompareCH (_VC_ARGS(CH)) {return _VC_FIXED;}
int _nrrdValCompareUC (_VC_ARGS(UC)) {return _VC_FIXED;}
int _nrrdValCompareSH (_VC_ARGS(SH)) {return _VC_FIXED;}
int _nrrdValCompareUS (_VC_ARGS(US)) {return _VC_FIXED;}
int _nrrdValCompareIN (_VC_ARGS(JN)) {return _VC_FIXED;}
int _nrrdValCompareUI (_VC_ARGS(UI)) {return _VC_FIXED;}
int _nrrdValCompareLL (_VC_ARGS(LL)) {return _VC_FIXED;}
int _nrrdValCompareUL (_VC_ARGS(UL)) {return _VC_FIXED;}
int _nrrdValCompareFL (_VC_ARGS(FL)) {_VC_FLOAT}
int _nrrdValCompareDB (_VC_ARGS(DB)) {_VC_FLOAT}
int (*
nrrdValCompare[NRRD_TYPE_MAX+1])(const void *, const void *) = {
  NULL,
  (int (*)(const void *, const void *))_nrrdValCompareCH,
  (int (*)(const void *, const void *))_nrrdValCompareUC,
  (int (*)(const void *, const void *))_nrrdValCompareSH,
  (int (*)(const void *, const void *))_nrrdValCompareUS,
  (int (*)(const void *, const void *))_nrrdValCompareIN,
  (int (*)(const void *, const void *))_nrrdValCompareUI,
  (int (*)(const void *, const void *))_nrrdValCompareLL,
  (int (*)(const void *, const void *))_nrrdValCompareUL,
  (int (*)(const void *, const void *))_nrrdValCompareFL,
  (int (*)(const void *, const void *))_nrrdValCompareDB,
  NULL
};
