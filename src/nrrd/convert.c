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
******** nrrdConvert()
**
** copies values from one type of nrrd to another, without
** any transformation, except what you get with a cast
**
** HEY: for the time being, this uses a "double" as the intermediate
** value holder, which may mean needless loss of precision
*/
int
nrrdConvert(Nrrd *nin, Nrrd *nout, int type) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdConvert";
  int d;
  NRRD_BIG_INT I;

  if (!(nin && nout &&
	type > nrrdTypeUnknown &&
	type < nrrdTypeLast)) {
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  if (nin->type == nrrdTypeBlock) {
    sprintf(err, "%s: can't deal with type block now, sorry", me);
    biffSet(NRRD, err); return 1;
  }

  /* if we're actually converting to the same type, just do a copy */
  if (type == nin->type) {
    if (nrrdCopy(nin, nout)) {
      sprintf(err, "%s: couldn't copy input to output", me);
      biffSet(NRRD, err); return 1;
    }
    return 0;
  }

  /* allocate space if necessary */
  if (!(nout->data)) {
    if (nrrdAlloc(nout, nin->num, type, nin->dim)) {
      sprintf(err, "%s: nrrdAlloc() failed to create slice", me);
      biffSet(NRRD, err); return 1;
    }
  }
  nout->type = type;
  
  /* copy (cast, really) the values */
  for (I=0; I<=nin->num-1; I++) {
    nrrdDInsert[nout->type](nout->data, I, 
			    nrrdDLookup[nin->type](nin->data, I));
  }

  /* set information in new volume */
  for (d=0; d<=nin->dim-1; d++) {
    nout->size[d] = nin->size[d];
    nout->spacing[d] = nin->spacing[d];
    nout->axisMin[d] = nin->axisMin[d];
    nout->axisMax[d] = nin->axisMax[d];
    strcpy(nout->label[d], nin->label[d]);
  }
  sprintf(nout->content, "(%s)(%s)", nrrdType2Str[nout->type], 
	  strlen(nin->content) ? nin->content : NRRD_NO_CONTENT);

  /* bye */
  return 0;
}

Nrrd *
nrrdNewConvert(Nrrd *nin, int type) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdNewConvert";
  Nrrd *nout;

  if (!(nout = nrrdNew())) {
    sprintf(err, "%s: nrrdNew() failed", me);
    biffAdd(NRRD, err); return NULL;
  }
  if (nrrdConvert(nin, nout, type)) {
    sprintf(err, "%s: nrrdConvert() failed", me);
    biffAdd(NRRD, err);
    nrrdNuke(nout);
    return NULL;
  }
  return nout;
}

/*
******** nrrdQuantize
**
** convert values to 8, 16, or 32 bit unsigned quantities
** by mapping the value range delimited by the nrrd's min
** and max to the representable range 
**
** NOTE: for the time being, this uses a "double" as the intermediate
** value holder, which may mean needless loss of precision
*/
int
nrrdQuantize(Nrrd *nin, Nrrd *nout, float min, float max, int bits) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdQuantize";
  double valIn;
  int valOut;
  unsigned long long valOutll;
  NRRD_BIG_INT I;
  int type, d;

  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (!( AIR_EXISTS(min) && AIR_EXISTS(max) )) {
    sprintf(err, "%s: wasn't given min,max values", me);
    biffSet(NRRD, err); return 1;
  }
  if (!(8 == bits || 16 == bits || 32 == bits)) {
    sprintf(err, "%s: bits has to be 8, 16, or 32 (not %d)", me, bits);
    biffSet(NRRD, err); return 1;
  }

  /* determine nrrd type from number of bits */
  switch (bits) {
  case 8:
    type = nrrdTypeUChar;
    break;
  case 16:
    type = nrrdTypeUShort;
    break;
  case 32:
    type = nrrdTypeUInt;
    break;
  }

  /* allocate space if necessary */
  if (!(nout->data)) {
    if (nrrdAlloc(nout, nin->num, type, nin->dim)) {
      sprintf(err, "%s: nrrdAlloc() failed to create slice", me);
      biffSet(NRRD, err); return 1;
    }
  }

  /* copy the values */
  for (I=0; I<=nin->num-1; I++) {
    valIn = nrrdDLookup[nin->type](nin->data, I);
    switch (bits) {
    case 8:
    case 16:
      NRRD_INDEX(min, valIn, max, 1 << bits, valOut);
      valOut = NRRD_CLAMP(0, valOut, (1 << bits)-1);
      nrrdDInsert[nout->type](nout->data, I, valOut);
      break;
    case 32:
      NRRD_INDEX(min, valIn, max, 1LLU << 32, valOutll);
      /* this line isn't compiling on my intel laptop */
      valOut = NRRD_CLAMP(0, valOut, (1LLU << 32)-1);
      nrrdDInsert[nout->type](nout->data, I, valOutll);
      break;
    }
  }

  /* set information in new volume */
  for (d=0; d<=nin->dim-1; d++) {
    nout->size[d] = nin->size[d];
    nout->spacing[d] = nin->spacing[d];
    nout->axisMin[d] = nin->axisMin[d];
    nout->axisMax[d] = nin->axisMax[d];
    strcpy(nout->label[d], nin->label[d]);
  }
  nout->oldMin = min;
  nout->oldMax = max;
  sprintf(nout->content, "quantize(%s,%d)", 
	  strlen(nin->content) ? nin->content : NRRD_NO_CONTENT, bits);

  /* bye */
  return(0);
}

Nrrd *
nrrdNewQuantize(Nrrd *nin, float min, float max, int bits) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdNewQuantize";
  Nrrd *nout;

  if (!(nout = nrrdNew())) {
    sprintf(err, "%s: nrrdNew() failed", me);
    biffAdd(NRRD, err); return NULL;
  }
  if (nrrdQuantize(nin, nout, min, max, bits)) {
    sprintf(err, "%s: nrrdQuantize() failed", me);
    biffAdd(NRRD, err);
    nrrdNuke(nout);
    return NULL;
  }
  return nout;
}
