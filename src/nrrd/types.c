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

/*
******** nrrdStr2Type()
**
** takes a given string and returns the integral type
*/
int
nrrdStr2Type(char *str) {

  /* >>>> obviously, the next two arrays      <<<<
     >>>> have to be in sync with each other, <<<<
     >>>> and the second has be in sync with  <<<<
     >>>> the nrrdType enum in nrrd.h         <<<< */

#define NUMVARIANTS 24

  char typStr[NUMVARIANTS][NRRD_SMALL_STRLEN]  = {
    "char", "signed char",
    "uchar", "unsigned char",
    "short", "short int", "signed short", "signed short int",
    "ushort", "unsigned short", "unsigned short int",
    "int", "signed int",
    "unsigned int",
    "long long", "long long int", "signed long long", "signed long long int",
    "unsigned long long", "unsigned long long int",
    "float",
    "double",
    "long double",
    "block"};
  int numStr[NUMVARIANTS] = {
    1, 1,
    2, 2,
    3, 3, 3, 3,
    4, 4, 4,
    5, 5,
    6,
    7, 7, 7, 7,
    8, 8,
    9,
    10,
    11,
    12};

  int i;

  for (i=0; i<=NUMVARIANTS-1; i++) {
    if (!(strcmp(typStr[i], str))) {
      return(numStr[i]);
    }
  }
  return(nrrdTypeUnknown);
}


