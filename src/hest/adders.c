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

/* --------------------------------------------------------------- 1 == kind */
unsigned int
hestOptAdd_Flag(hestOpt **optP, const char *flag, int *valueP, const char *info) {

  return hestOptAdd_nva(optP, flag, NULL /* name */, airTypeInt /* actually moot */,
                        0 /* min */, 0 /* max */, valueP, NULL /* default */, info, /* */
                        NULL, NULL, NULL);
}

/* --------------------------------------------------------------- 2 == kind */
unsigned int
hestOptAdd_1_Bool(hestOpt **optP, const char *flag, const char *name, /* */
                  int *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(optP, flag, name, airTypeBool, 1, 1, /* */
                        valueP, dflt, info,                  /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_Int(hestOpt **optP, const char *flag, const char *name, /* */
                 int *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(optP, flag, name, airTypeInt, 1, 1, /* */
                        valueP, dflt, info,                 /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_UInt(hestOpt **optP, const char *flag, const char *name, /* */
                  unsigned int *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(optP, flag, name, airTypeUInt, 1, 1, /* */
                        valueP, dflt, info,                  /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_LongInt(hestOpt **optP, const char *flag, const char *name, /* */
                     long int *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(optP, flag, name, airTypeLongInt, 1, 1, /* */
                        valueP, dflt, info,                     /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_ULongInt(hestOpt **optP, const char *flag, const char *name, /* */
                      unsigned long int *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(optP, flag, name, airTypeULongInt, 1, 1, /* */
                        valueP, dflt, info,                      /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_Size_t(hestOpt **optP, const char *flag, const char *name, /* */
                    size_t *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(optP, flag, name, airTypeSize_t, 1, 1, /* */
                        valueP, dflt, info,                    /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_Float(hestOpt **optP, const char *flag, const char *name, /* */
                   float *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(optP, flag, name, airTypeFloat, 1, 1, /* */
                        valueP, dflt, info,                   /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_Double(hestOpt **optP, const char *flag, const char *name, /* */
                    double *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(optP, flag, name, airTypeDouble, 1, 1, /* */
                        valueP, dflt, info,                    /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_Char(hestOpt **optP, const char *flag, const char *name, /* */
                  char *valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(optP, flag, name, airTypeChar, 1, 1, /* */
                        valueP, dflt, info,                  /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_String(hestOpt **optP, const char *flag, const char *name, /* */
                    char **valueP, const char *dflt, const char *info) {
  return hestOptAdd_nva(optP, flag, name, airTypeString, 1, 1, /* */
                        valueP, dflt, info,                    /* */
                        NULL, NULL, NULL);
}

unsigned int
hestOptAdd_1_Enum(hestOpt **optP, const char *flag, const char *name, /* */
                  int *valueP, const char *dflt, const char *info,    /* */
                  const airEnum *enm) {
  return hestOptAdd_nva(optP, flag, name, airTypeEnum, 1, 1, /* */
                        valueP, dflt, info,                  /* */
                        NULL, enm, NULL);
}

unsigned int
hestOptAdd_1_Other(hestOpt **optP, const char *flag, const char *name, /* */
                   void *valueP, const char *dflt, const char *info,   /* */
                   const hestCB *CB) {
  return hestOptAdd_nva(optP, flag, name, airTypeOther, 1, 1, /* */
                        valueP, dflt, info,                   /* */
                        NULL, NULL, CB);
}

/*
hestOptSetXX(hestOpt *opt, )
1<T>, 2<T>, 3<T>, 4<T>, N<T>
1v<T>, Nv<T>  need sawP

<T>=
Bool, Int, UInt, LongInt, ULongInt, Size_t,
Float, Double, Char, String,
Enum,  need Enum
Other,  need CB
*/
