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


#include "nrrd.h"
#include "private.h"

/* 
** making these typedefs here allows us to used one token for both
** constructing function names, and for specifying argument types
*/
typedef signed char CH;
typedef unsigned char UC;
typedef signed short SH;
typedef unsigned short US;
typedef signed int IN;
typedef unsigned int UI;
typedef signed long long int LL;
typedef unsigned long long int UL;
typedef float FL;
typedef double DB;
typedef nrrdBigInt BI;
/* typedef long double LD; */

#define MAP(F, A) \
F(A, CH) \
F(A, UC) \
F(A, SH) \
F(A, US) \
F(A, IN) \
F(A, UI) \
F(A, LL) \
F(A, UL) \
F(A, FL) \
F(A, DB)
/* F(A, LD) */

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

MAP(LOAD_DEF, IN)
MAP(LOAD_DEF, FL)
MAP(LOAD_DEF, DB)

int (*nrrdILoad[NRRD_TYPE_MAX+1])(void*) = {
  NULL, MAP(LOAD_LIST, IN) NULL
};
float (*nrrdFLoad[NRRD_TYPE_MAX+1])(void*) = {
  NULL, MAP(LOAD_LIST, FL) NULL
};
double (*nrrdDLoad[NRRD_TYPE_MAX+1])(void*) = {
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

MAP(STORE_DEF, IN)
MAP(STORE_DEF, FL)
MAP(STORE_DEF, DB)

int (*nrrdIStore[NRRD_TYPE_MAX+1])(void *, int) = {
  NULL, MAP(STORE_LIST, IN) NULL
};
float (*nrrdFStore[NRRD_TYPE_MAX+1])(void *, float) = {
  NULL, MAP(STORE_LIST, FL) NULL
};
double (*nrrdDStore[NRRD_TYPE_MAX+1])(void *, double) = {
  NULL, MAP(STORE_LIST, DB) NULL
};



/*
** _nrrdLookup<TA><TB>(<TB> *v, nrrdBigInt I)
**
** Looks up element I of TB array v, and returns it cast to a TA.
*/
#define LOOKUP_DEF(TA, TB)                    \
TA                                            \
_nrrdLookup##TA##TB(TB *v, nrrdBigInt I) {    \
  return v[I];                                \
}
#define LOOKUP_LIST(TA, TB)                   \
  (TA (*)(void*, nrrdBigInt))_nrrdLookup##TA##TB,

MAP(LOOKUP_DEF, IN)
MAP(LOOKUP_DEF, FL)
MAP(LOOKUP_DEF, DB)

int (*nrrdILookup[NRRD_TYPE_MAX+1])(void *, nrrdBigInt) = {
  NULL, MAP(LOOKUP_LIST, IN) NULL
};
float (*nrrdFLookup[NRRD_TYPE_MAX+1])(void *, nrrdBigInt) = {
  NULL, MAP(LOOKUP_LIST, FL) NULL
};
double (*nrrdDLookup[NRRD_TYPE_MAX+1])(void *, nrrdBigInt) = {
  NULL, MAP(LOOKUP_LIST, DB) NULL
};



/*
** _nrrdInsert<TA><TB>(<TB> *v, nrrdBigInt I, <TA> j)
**
** Given TA j, stores it in v[i] (implicitly casting to TB).
** Returns the result of the assignment, which may not be the same as
** the value that was passed in.
*/
#define INSERT_DEF(TA, TB)                         \
TA                                                 \
_nrrdInsert##TA##TB(TB *v, nrrdBigInt I, TA j) {   \
  return (v[I] = j);                               \
}
#define INSERT_LIST(TA, TB)                        \
  (TA (*)(void*, nrrdBigInt, TA))_nrrdInsert##TA##TB,

MAP(INSERT_DEF, IN)
MAP(INSERT_DEF, FL)
MAP(INSERT_DEF, DB)

int (*nrrdIInsert[NRRD_TYPE_MAX+1])(void *, nrrdBigInt, int) = {
  NULL, MAP(INSERT_LIST, IN) NULL
};
float (*nrrdFInsert[NRRD_TYPE_MAX+1])(void *, nrrdBigInt, float) = {
  NULL, MAP(INSERT_LIST, FL) NULL
};
double (*nrrdDInsert[NRRD_TYPE_MAX+1])(void *, nrrdBigInt, double) = {
  NULL, MAP(INSERT_LIST, DB) NULL
};



/*
******** nrrdSprint
**
** Dereferences pointer v and sprintf()s that value into given string s,
** returns the result of sprintf()
*/
int _nrrdSprintC(char *s, char *v) {
  return(sprintf(s, "%d", *v)); }
int _nrrdSprintUC(char *s, unsigned char *v) {
  return(sprintf(s, "%u", *v)); }
int _nrrdSprintS(char *s, short *v) {
  return(sprintf(s, "%d", *v)); }
int _nrrdSprintUS(char *s, unsigned short *v) {
  return(sprintf(s, "%u", *v)); }
int _nrrdSprintI(char *s, int *v) {
  return(sprintf(s, "%d", *v)); }
int _nrrdSprintUI(char *s, unsigned int *v) {
  return(sprintf(s, "%u", *v)); }
int _nrrdSprintLLI(char *s, long long int *v) {
  return(sprintf(s, "%lld", *v)); }
int _nrrdSprintULLI(char *s, unsigned long long *v) {
  return(sprintf(s, "%llu", *v)); }
/* HEY: sizeof(float) and sizeof(double) assumed here, since we're 
   basing "8" and "17" on 6 == FLT_DIG and 15 == DBL_DIG, which are 
   digits of precision for floats and doubles, respectively */
int _nrrdSprintF(char *s, float *v) {
  return(airSinglePrintf(NULL, s, "%.8g", *v)); }
int _nrrdSprintD(char *s, double *v) {
  return(airSinglePrintf(NULL, s, "%.17lg", *v)); }
/*
int _nrrdSprintLD(char *s, long double *v) {
  return(sprintf(s, "%Lf", *v)); }
*/
int (*nrrdSprint[NRRD_TYPE_MAX+1])(char *, void *) = {
  NULL,
  (int (*)(char *, void *))_nrrdSprintC,
  (int (*)(char *, void *))_nrrdSprintUC,
  (int (*)(char *, void *))_nrrdSprintS,
  (int (*)(char *, void *))_nrrdSprintUS,
  (int (*)(char *, void *))_nrrdSprintI,
  (int (*)(char *, void *))_nrrdSprintUI,
  (int (*)(char *, void *))_nrrdSprintLLI,
  (int (*)(char *, void *))_nrrdSprintULLI,
  (int (*)(char *, void *))_nrrdSprintF,
  (int (*)(char *, void *))_nrrdSprintD,
  /* (int (*)(char *, void *))_nrrdSprintLD, */
  NULL};

/*
******** nrrdFprint
**
** Dereferences pointer v and fprintf()s that value into given file f;
** returns the result of fprintf()
*/
int _nrrdFprintC(FILE *f, char *v) {
  return(fprintf(f, "%d", *v)); }
int _nrrdFprintUC(FILE *f, unsigned char *v) {
  return(fprintf(f, "%u", *v)); }
int _nrrdFprintS(FILE *f, short *v) {
  return(fprintf(f, "%d", *v)); }
int _nrrdFprintUS(FILE *f, unsigned short *v) {
  return(fprintf(f, "%u", *v)); }
int _nrrdFprintI(FILE *f, int *v) {
  return(fprintf(f, "%d", *v)); }
int _nrrdFprintUI(FILE *f, unsigned int *v) {
  return(fprintf(f, "%u", *v)); }
int _nrrdFprintLLI(FILE *f, long long int *v) {
  return(fprintf(f, "%lld", *v)); }
int _nrrdFprintULLI(FILE *f, unsigned long long *v) {
  return(fprintf(f, "%llu", *v)); }
int _nrrdFprintF(FILE *f, float *v) {
  return(airSinglePrintf(f, NULL, "%.8g", *v)); }
int _nrrdFprintD(FILE *f, double *v) {
  return(airSinglePrintf(f, NULL, "%.17lg", *v)); }
/* int _nrrdFprintLD(FILE *f, long double *v) {
   return(fprintf(f, "%Lf", *v)); } */
int (*nrrdFprint[NRRD_TYPE_MAX+1])(FILE *, void *) = {
  NULL,
  (int (*)(FILE *, void *))_nrrdFprintC,
  (int (*)(FILE *, void *))_nrrdFprintUC,
  (int (*)(FILE *, void *))_nrrdFprintS,
  (int (*)(FILE *, void *))_nrrdFprintUS,
  (int (*)(FILE *, void *))_nrrdFprintI,
  (int (*)(FILE *, void *))_nrrdFprintUI,
  (int (*)(FILE *, void *))_nrrdFprintLLI,
  (int (*)(FILE *, void *))_nrrdFprintULLI,
  (int (*)(FILE *, void *))_nrrdFprintF,
  (int (*)(FILE *, void *))_nrrdFprintD,
  /*   (int (*)(FILE *, void *))_nrrdFprintLD, */
  NULL};

/*
******** nrrdFClamp
**
** clamps a given float to the range to the range representable by the
** given integral type; for floating point types we just return the
** given number, even if that number is infinity or nan or something
** else which creates random nonsense when assigned to the integral
** type.  This is the only defensibly policy.
*/
float _nrrdFClampC(float v) {
  if (v < SCHAR_MIN) return SCHAR_MIN;
  if (v > SCHAR_MAX) return SCHAR_MAX;
  return v;
}
float _nrrdFClampUC(float v) {
  if (v < 0) return 0;
  if (v > UCHAR_MAX) return UCHAR_MAX;
  return v;
}
float _nrrdFClampS(float v) {
  if (v < SHRT_MIN) return SHRT_MIN;
  if (v > SHRT_MAX) return SHRT_MAX;
  return v;
}
float _nrrdFClampUS(float v) {
  if (v < 0) return 0;
  if (v > USHRT_MAX) return USHRT_MAX;
  return v;
}
float _nrrdFClampI(float v) {
  if (v < INT_MIN) return INT_MIN;
  if (v > INT_MAX) return INT_MAX;
  return v;
}
float _nrrdFClampUI(float v) {
  if (v < 0) return 0;
  if (v > UINT_MAX) return UINT_MAX;
  return v;
}
float _nrrdFClampLLI(float v) {
  if (v < NRRD_LLONG_MIN) return NRRD_LLONG_MIN;
  if (v > NRRD_LLONG_MAX) return NRRD_LLONG_MAX;
  return v;
}
float _nrrdFClampULLI(float v) {
  if (v < 0) return 0;
  if (v > NRRD_ULLONG_MAX) return NRRD_ULLONG_MAX;
  return v;
}
float _nrrdFClampF(float v) { return v; }
float _nrrdFClampD(float v) { return v; }
/* float _nrrdFClampLD(float v) { return v; } */
float (*nrrdFClamp[NRRD_TYPE_MAX+1])(float) = {
  NULL,
  _nrrdFClampC,
  _nrrdFClampUC,
  _nrrdFClampS,
  _nrrdFClampUS,
  _nrrdFClampI,
  _nrrdFClampUI,
  _nrrdFClampLLI,
  _nrrdFClampULLI,
  _nrrdFClampF,
  _nrrdFClampD,
  /*   _nrrdFClampLD, */
  NULL};

/*
******** nrrdDClamp
**
** same as nrrdDClamp, but for doubles
*/
double _nrrdDClampC(double v) {
  if (v < SCHAR_MIN) return SCHAR_MIN;
  if (v > SCHAR_MAX) return SCHAR_MAX;
  return v;
}
double _nrrdDClampUC(double v) {
  if (v < 0) return 0;
  if (v > UCHAR_MAX) return UCHAR_MAX;
  return v;
}
double _nrrdDClampS(double v) {
  if (v < SHRT_MIN) return SHRT_MIN;
  if (v > SHRT_MAX) return SHRT_MAX;
  return v;
}
double _nrrdDClampUS(double v) {
  if (v < 0) return 0;
  if (v > USHRT_MAX) return USHRT_MAX;
  return v;
}
double _nrrdDClampI(double v) {
  if (v < INT_MIN) return INT_MIN;
  if (v > INT_MAX) return INT_MAX;
  return v;
}
double _nrrdDClampUI(double v) {
  if (v < 0) return 0;
  if (v > UINT_MAX) return UINT_MAX;
  return v;
}
double _nrrdDClampLLI(double v) {
  if (v < NRRD_LLONG_MIN) return NRRD_LLONG_MIN;
  if (v > NRRD_LLONG_MAX) return NRRD_LLONG_MAX;
  return v;
}
double _nrrdDClampULLI(double v) {
  if (v < 0) return 0;
  if (v > NRRD_ULLONG_MAX) return NRRD_ULLONG_MAX;
  return v;
}
double _nrrdDClampF(double v) { return v; }
double _nrrdDClampD(double v) { return v; }
/* double _nrrdDClampLD(double v) { return v; } */
double (*nrrdDClamp[NRRD_TYPE_MAX+1])(double) = {
  NULL,
  _nrrdDClampC,
  _nrrdDClampUC,
  _nrrdDClampS,
  _nrrdDClampUS,
  _nrrdDClampI,
  _nrrdDClampUI,
  _nrrdDClampLLI,
  _nrrdDClampULLI,
  _nrrdDClampF,
  _nrrdDClampD,
  /*   _nrrdDClampLD, */
  NULL};

/* about here is where Gordon admits he might have some use for C++ */

#define _MM_ARGS(type) type *minP, type *maxP, nrrdBigInt N, type *v

#define _MM_FIXED(type)                                                  \
  nrrdBigInt I, T;                                                       \
  type a, b, min, max;                                                   \
                                                                         \
  if (!(minP && maxP))                                                   \
    return;                                                              \
                                                                         \
  /* get initial values */                                               \
  min = max = v[0];                                                      \
                                                                         \
  /* run through array in pairs; by doing a compare on successive        \
     elements, we can do three compares per pair instead of the naive    \
     four.  In one very unexhaustive test on irix6.64, this resulted     \
     in a 20% decrease in running time.  I learned this trick from       \
     Numerical Recipes in C, long time ago, but I can't find it          \
     anywhere in the book now ... */                                     \
  T = N/2;                                                               \
  for (I=0; I<=T; I++) {                                                 \
    a = v[0 + 2*I];                                                      \
    b = v[1 + 2*I];                                                      \
    if (a < b) {                                                         \
      min = AIR_MIN(a, min);                                             \
      max = AIR_MAX(b, max);                                             \
    }                                                                    \
    else {                                                               \
      max = AIR_MAX(a, max);                                             \
      min = AIR_MIN(b, min);                                             \
    }                                                                    \
  }                                                                      \
                                                                         \
  /* get the very last one (may be redundant) */                         \
  a = v[N-1];                                                            \
  if (a < min) {                                                         \
    min = a;                                                             \
  }                                                                      \
  else {                                                                 \
    if (a > max) {                                                       \
      max = a;                                                           \
    }                                                                    \
  }                                                                      \
                                                                         \
  /* record results */                                                   \
  *minP = min;                                                           \
  *maxP = max;

#define _MM_FLOAT(type)                                                  \
  nrrdBigInt I;                                                          \
  type a, min, max;                                                      \
                                                                         \
  if (!(minP && maxP))                                                   \
    return;                                                              \
                                                                         \
  /* we have to explicitly search for the first non-NaN value */         \
  min = min = AIR_NAN;                                                   \
  for (I=0; I<N; I++) {                                                  \
    a = v[I];                                                            \
    if (AIR_EXISTS(a)) {                                                 \
      min = max = a;                                                     \
      break;                                                             \
    }                                                                    \
    else {                                                               \
      nrrdHackHasNonExist = AIR_TRUE;                                    \
    }                                                                    \
  }                                                                      \
  /* we continue searching knowing something to compare against, but     \
     still checking AIR_EXISTS at each value.  We don't want an          \
     infinity to corrupt min or max, since this is the stated behavior   \
     of nrrdMinMaxFind() */                                              \
  for (I=I+1; I<N; I++) {                                                \
    a = v[I];                                                            \
    if (AIR_EXISTS(a)) {                                                 \
      if (a < min) {                                                     \
        min = a;                                                         \
      }                                                                  \
      else {                                                             \
        if (a > max) {                                                   \
          max = a;                                                       \
        }                                                                \
      }                                                                  \
    }                                                                    \
    else {                                                               \
      nrrdHackHasNonExist = AIR_TRUE;                                    \
    }                                                                    \
  }                                                                      \
                                                                         \
  *minP = min;                                                           \
  *maxP = max;                                                           \

void _nrrdMinMaxC   (_MM_ARGS(char))           { _MM_FIXED(char) }
void _nrrdMinMaxUC  (_MM_ARGS(unsigned char))  { _MM_FIXED(unsigned char) }
void _nrrdMinMaxS   (_MM_ARGS(short))          { _MM_FIXED(short) }
void _nrrdMinMaxUS  (_MM_ARGS(unsigned short)) { _MM_FIXED(unsigned short) }
void _nrrdMinMaxI   (_MM_ARGS(int))            { _MM_FIXED(int) }
void _nrrdMinMaxUI  (_MM_ARGS(unsigned int))   { _MM_FIXED(unsigned int) }
void _nrrdMinMaxLLI (_MM_ARGS(long long int))  { _MM_FIXED(long long int) }
void _nrrdMinMaxULLI(_MM_ARGS(unsigned long long int))   { 
  _MM_FIXED(unsigned long long int) }
void _nrrdMinMaxF   (_MM_ARGS(float))          { _MM_FLOAT(float) }
void _nrrdMinMaxD   (_MM_ARGS(double))         { _MM_FLOAT(double) }
/* void _nrrdMinMaxLD  (_MM_ARGS(long double))    { _MM_FLOAT(long double) } */
void (*_nrrdMinMaxFind[NRRD_TYPE_MAX+1])(void *, void *, 
					 nrrdBigInt, void *) = {
  NULL,
  (void (*)(void *, void *, nrrdBigInt, void *))_nrrdMinMaxC,
  (void (*)(void *, void *, nrrdBigInt, void *))_nrrdMinMaxUC,
  (void (*)(void *, void *, nrrdBigInt, void *))_nrrdMinMaxS,
  (void (*)(void *, void *, nrrdBigInt, void *))_nrrdMinMaxUS,
  (void (*)(void *, void *, nrrdBigInt, void *))_nrrdMinMaxI,
  (void (*)(void *, void *, nrrdBigInt, void *))_nrrdMinMaxUI,
  (void (*)(void *, void *, nrrdBigInt, void *))_nrrdMinMaxLLI,
  (void (*)(void *, void *, nrrdBigInt, void *))_nrrdMinMaxULLI,
  (void (*)(void *, void *, nrrdBigInt, void *))_nrrdMinMaxF,
  (void (*)(void *, void *, nrrdBigInt, void *))_nrrdMinMaxD,
  /* (void (*)(void *, void *, nrrdBigInt, void *))_nrrdMinMaxLD, */
  NULL
};
