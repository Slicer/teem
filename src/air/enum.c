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

#include "air.h"

int
airEnumUnknown(airEnum *enm) {

  if (enm->val) {
    return enm->val[0];
  }
  else {
    return 0;
  }
}

int
airEnumValidVal(airEnum *enm, int val) {
  int i, valid;

  valid = AIR_FALSE;
  if (enm->val) {
    /* we need to check against the given values */
    for (i=1; i<=enm->M; i++) {
      if (val == enm->val[i]) {
	valid = AIR_TRUE;
	break;
      }
    }
  }
  else {
    valid = AIR_INSIDE(1, val, enm->M);
  }
  return valid;
}

char *
airEnumStr(airEnum *enm, int val) {
  int i;
  char *ret = NULL;

  if (!airEnumValidVal(enm, val))
    return enm->str[0];

  if (enm->val) {
    /* we need to check against the given values */
    for (i=0; i<=enm->M; i++) {
      if (val == enm->val[i]) {
	ret = enm->str[i];
	break;
      }
    }
    ret = ret ? ret : enm->str[0];
  }
  else {
    /* no value list, simply lookup string */
    ret = enm->str[val];
  }
  return ret;
}


int 
airEnumVal(airEnum *enm, const char *str) {
  char *strCpy, test[AIR_STRLEN_SMALL];
  int i;

  if (!str)
    return airEnumUnknown(enm);
  
  strCpy = airStrdup(str);
  if (!enm->sense) {
    airToLower(strCpy);
  }

  if (enm->strEqv) {
    for (i=0; strlen(enm->strEqv[i]); i++) {
      strncpy(test, enm->strEqv[i], AIR_STRLEN_SMALL);
      test[AIR_STRLEN_SMALL-1] = '\0';
      if (!enm->sense)
	airToLower(test);
      if (!strcmp(test, strCpy)) {
	free(strCpy);
	return enm->valEqv[i];
      }
    }
  }
  else {
    /* enm->strEqv NULL */
    for (i=1; i<=enm->M; i++) {
      strncpy(test, enm->str[i], AIR_STRLEN_SMALL);
      test[AIR_STRLEN_SMALL-1] = '\0';
      if (!enm->sense)
	airToLower(test);
      if (!strcmp(test, strCpy)) {
	free(strCpy);
	return enm->val ? enm->val[i] : i;
      }      
    }
  }

  /* else we never matched a string */
  free(strCpy);
  return airEnumUnknown(enm);
}
