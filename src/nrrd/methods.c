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
** initnrrd
**
** initializes a nrrd to default state.  Mostly just sets values to 
** -1, NaN, "", NULL, or Unknown
*/
void
_nrrdinit(Nrrd *nrrd) {
  int i;

  nrrd->data = NULL;
  nrrd->num = -1;
  nrrd->type = nrrdTypeUnknown;
  nrrd->dim = -1;
  nrrd->encoding = nrrdEncodingUnknown;
  /* this is too presumptious-- it meant that nrrdRead() assumes
     raw data, which is basically unsafe; encoding needs to be
     specified in the nrrd header 
  nrrd->encoding = nrrdEncodingRaw;
  */
  for (i=0; i<=NRRD_MAX_DIM-1; i++) {
    nrrd->size[i] = -1;
    nrrd->spacing[i] = AIR_NAN;
    nrrd->axisMin[i] = nrrd->axisMax[i] = AIR_NAN;
    strcpy(nrrd->label[i], "");
  }
  strcpy(nrrd->content, "");
  nrrd->blockSize = -1;
  nrrd->min = nrrd->max = AIR_NAN;
  nrrd->oldMin = nrrd->oldMax = AIR_NAN;
  strcpy(nrrd->dir, "");
  strcpy(nrrd->name, "");
  nrrd->dataFile = NULL;
  nrrd->lineSkip = 0;    /* this is a reasonable default value */
  nrrd->byteSkip = 0;    /* this is a reasonable default value */
  nrrd->ptr = NULL;
  nrrd->fileEndian = airEndianUnknown;
  nrrdCommentClear(nrrd);
}

/*
******** nrrdNew()
**
** creates and initializes a Nrrd
**
** this does NOT use biff
*/
Nrrd *
nrrdNew(void) {
  Nrrd *nrrd;
  
  nrrd = (Nrrd*)(calloc(1, sizeof(Nrrd)));
  if (nrrd) {
    _nrrdinit(nrrd);
  }
  return nrrd;
}

/*
******** nrrdInit()
**
** puts nrrd back into state following initialization
**
** this does NOT use biff
*/
void
nrrdInit(Nrrd *nrrd) {

  if (nrrd) {
    _nrrdinit(nrrd);
  }
}

/*
******** nrrdNix()
**
** does nothing with the array, just does whatever is needed
** to free the nrrd itself
**
** this does NOT use biff
*/
Nrrd *
nrrdNix(Nrrd *nrrd) {
  
  if (nrrd) {
    /* because comments are the only dynamically allocated part
       of the nrrd struct itself */
    nrrdCommentClear(nrrd);
    nrrd = airFree(nrrd);
  }
  return NULL;
}

/*
******** nrrdUnwrap()
**
** calls nrrdNix
*/
Nrrd *
nrrdUnwrap(Nrrd *nrrd) {
  
  return nrrdNix(nrrd);
}

/*
******** nrrdEmpty()
**
** frees data inside nrrd AND resets all its state, so its the
** same as what comes from nrrdNew()
*/
Nrrd *
nrrdEmpty(Nrrd *nrrd) {
  
  if (nrrd) {
    if (nrrd->data) {
      nrrd->data = airFree(nrrd->data);
    }
    _nrrdinit(nrrd);
  }
  return nrrd;
}

/*
******** nrrdNuke()
**
** blows away the nrrd and everything inside
**
** always returns NULL
*/
Nrrd *
nrrdNuke(Nrrd *nrrd) {
  
  if (nrrd) {
    nrrdEmpty(nrrd);
    nrrdNix(nrrd);
  }
  return NULL;
}

/*
******** nrrdWrap()
**
** wraps a given Nrrd around a given array
*/
void
nrrdWrap(Nrrd *nrrd, void *data, NRRD_BIG_INT num, int type, int dim) {

  if (nrrd) {
    nrrd->data = data;
    nrrd->num = num;
    nrrd->type = type;
    nrrd->dim = dim;
  }
}

/*
******** nrrdNewWrap()
**
** wraps a new Nrrd around a given array
*/
Nrrd *
nrrdNewWrap(void *data, NRRD_BIG_INT num, int type, int dim) {
  char me[]="nrrdNewWrap", err[NRRD_MED_STRLEN];
  Nrrd *nrrd;

  if (!(nrrd = nrrdNew())) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return NULL;
  }
  nrrdWrap(nrrd, data, num, type, dim);
  return nrrd;
}

/*
******** nrrdSetInfo()
**
** does anyone really need this?
*/
void
nrrdSetInfo(Nrrd *nrrd, NRRD_BIG_INT num, int type, int dim) {

  if (nrrd) {
    nrrd->num = num;
    nrrd->type = type;
    nrrd->dim = dim;
  }
}

/*
******** nrrdNewSetInfo()
**
** does anyone really need this?
*/
Nrrd *
nrrdNewSetInfo(NRRD_BIG_INT num, int type, int dim) {
  char me[]="nrrdNewSetInfo", err[NRRD_MED_STRLEN];
  Nrrd *nrrd;

  if (!(nrrd = nrrdNew())) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return NULL;
  }
  nrrdSetInfo(nrrd, num, type, dim);
  return nrrd;
}

int
nrrdCommentCopy(Nrrd *nout, Nrrd *nin) {
  char me[]="nrrdCommentCopy", err[NRRD_MED_STRLEN];
  int numc, i, E;

  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  nrrdCommentClear(nout);
  numc = nin->numComments;
  E = 0;
  if (numc) {
    if (!E) nout->comment = calloc(numc+1, sizeof(char*));
    E |= !nout->comment;
    for (i=0; i<=numc-1; i++) {
      if (!E) nout->comment[i] = airStrdup(nin->comment[i]);
      E |= !nout->comment[i];
    }
    if (!E) nout->comment[numc] = NULL;
    if (E) {
      sprintf(err, "%s: failed to allocate or copy comments", me);
      biffSet(NRRD, err); return 1;
    }
  }
  return 0;
}

int
nrrdCopy(Nrrd *nout, Nrrd *nin) {
  Nrrd ntmp;
  char me[]="nrrdCopy", err[NRRD_MED_STRLEN];

  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (nout == nin) {
    /* I guess there's nothing to do! */
    return 0;
  }
  if (nrrdMaybeAlloc(nout, nin->num, nin->type, nin->dim)) {
    sprintf(err, "%s: couldn't allocate data", me);
    biffAdd(NRRD, err); return 1;
  }
  nrrdCommentClear(nout);
  memcpy(&ntmp, nout, sizeof(Nrrd));
  memcpy(nout, nin, sizeof(Nrrd));
  nout->data = ntmp.data;
  nout->numComments = 0;
  nout->comment = NULL;
  if (nrrdCommentCopy(nout, nin)) {
    sprintf(err, "%s: can't copy comments", me);
    biffAdd(NRRD, err); return 1;
  }
  memcpy(nout->data, nin->data, nin->num*nrrdTypeSize[nin->type]);
  return 0;
}

Nrrd *
nrrdNewCopy(Nrrd *nin) {
  char me[]="nrrdNewCopy", err[NRRD_MED_STRLEN];
  Nrrd *nout;

  if (!(nout = nrrdNew())) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return NULL;
  }
  if (nrrdCopy(nout, nin)) {
    sprintf(err, "%s: trouble copying nrrd", me);
    biffAdd(NRRD, err); return NULL;
  }
  return nout;
}

/*
******** nrrdAlloc()
**
** allocates data array and sets information
**
** This function will always allocate more memory (via calloc), but
** it will free() nrrd->data if it is non-NULL when passed in
**
** Note: This function DOES use biff
*/
int 
nrrdAlloc(Nrrd *nrrd, NRRD_BIG_INT num, int type, int dim) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdAlloc";

  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (!AIR_BETWEEN(nrrdTypeUnknown, type, nrrdTypeLast)) {
    sprintf(err, "%s: type (%d) is invalid", me, type);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == type) {
    sprintf(err, "%s: can't deal with nrrdTypeBlock, sorry", me);
    biffSet(NRRD, err); return 1;
  }
  nrrd->data = airFree(nrrd->data);
  nrrd->data = calloc(num, nrrdTypeSize[type]);
  if (!(nrrd->data)) {
    sprintf(err, "%s: calloc(" NRRD_BIG_INT_PRINTF ",%d) failed", 
	    me, num, nrrdTypeSize[type]);
    biffSet(NRRD, err); return 1 ;
  }
  nrrd->num = num;
  nrrd->type = type;
  nrrd->dim = dim;
  return 0;
}

int
nrrdMaybeAlloc(Nrrd *nrrd, NRRD_BIG_INT num, int type, int dim) {
  char err[NRRD_MED_STRLEN], me[]="nrrdMaybeAlloc";
  NRRD_BIG_INT sizeWant, sizeHave;
  int need;
  
  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (!AIR_BETWEEN(nrrdTypeUnknown, type, nrrdTypeLast)) {
    sprintf(err, "%s: type (%d) is invalid", me, type);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == type) {
    sprintf(err, "%s: can't deal with nrrdTypeBlock, sorry", me);
    biffSet(NRRD, err); return 1;
  }
  if (!(nrrd->data && nrrd->num > 0)) {
    need = 1;
  }
  else {
    sizeHave = nrrd->num * nrrdElementSize(nrrd);
    sizeWant = num * nrrdTypeSize[type];
    need = sizeHave != sizeWant;
  }
  if (need) {
    if (nrrdAlloc(nrrd, num, type, dim)) {
      sprintf(err, "%s: trouble", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  return 0;
}


/*
******** nrrdNewAlloc()
**
** creates the nrrd AND the array inside
**
** Note: This function DOES use biff
*/
Nrrd *
nrrdNewAlloc(NRRD_BIG_INT num, int type, int dim) {
  char me[]="nrrdNewAlloc", err[NRRD_MED_STRLEN];
  Nrrd *nrrd;
  
  if (!(nrrd = nrrdNew())) {
    sprintf(err, "%s: nrrdNew() failed", me);
    biffAdd(NRRD, err); return NULL;
  }
  if (nrrdAlloc(nrrd, num, type, dim)) {
    sprintf(err, "%s: nrrdAlloc() failed", me);
    biffAdd(NRRD, err); 
    nrrdNix(nrrd);
    return NULL;
  }    
  return nrrd;
}

/*
******** nrrdNewPPM()
**
** for making a nrrd suitable for holding PPM data
*/
Nrrd *
nrrdNewPPM(int sx, int sy) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdNewPPM";
  Nrrd *ppm;

  if (!(sx > 0 && sy > 0)) {
    sprintf(err, "%s: got invalid sizes (%d,%d)", me, sx, sy);
    biffSet(NRRD, err);
    return NULL;
  }
  if (!(ppm = nrrdNewAlloc(sx*sy*3, nrrdTypeUChar, 3))) {
    sprintf(err, "%s: couldn't allocate %d x %d 24-bit image", me, sx, sy);
    biffAdd(NRRD, err);
    return NULL;
  }
  ppm->size[0] = 3;
  ppm->size[1] = sx;
  ppm->size[2] = sy;
  return ppm;
}

/*
******** nrrdNewPGM()
**
** for making a nrrd suitable for holding PGM data
*/
Nrrd *
nrrdNewPGM(int sx, int sy) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdNewPGM";
  Nrrd *pgm;

  if (!(sx > 0 && sy > 0)) {
    sprintf(err, "%s: got invalid sizes (%d,%d)", me, sx, sy);
    biffSet(NRRD, err);
    return NULL;
  }
  if (!(pgm = nrrdNewAlloc(sx*sy, nrrdTypeUChar, 2))) {
    sprintf(err, "%s: couldn't allocate %d x %d 8-bit image", me, sx, sy);
    biffAdd(NRRD, err);
    return NULL;
  }
  pgm->size[0] = sx;
  pgm->size[1] = sy;
  return pgm;
}

/*
******** nrrdCommentAdd()
**
** adds a given string to the list of comments
*/
void
nrrdCommentAdd(Nrrd *nrrd, char *cmt) {
  char *newcmt, **newcmts;
  int i, len, num;
  
  if (nrrd && cmt) {
    newcmt = airStrdup(cmt);
    len = strlen(newcmt);
    /* clean out carraige returns that would screw up reader */
    for (i=0; i<=len-1; i++) {
      if ('\n' == newcmt[i]) {
	if (i < len-1) {
	  newcmt[i] = ' ';
	}
	else {
	  newcmt[i] = 0;
	}
      }
    }
    /* num is the length of the old comment array */
    num = nrrd->numComments+1;
    newcmts = malloc((num+1)*sizeof(char *));
    for (i=0; i<=nrrd->numComments-1; i++)
      newcmts[i] = nrrd->comment[i];
    newcmts[i] = newcmt;
    newcmts[i+1] = NULL;
    if (nrrd->comment) 
      nrrd->comment = airFree(nrrd->comment);
    nrrd->comment = newcmts;
    nrrd->numComments += 1;
  }
}

/*
******** nrrdCommentClear()
**
** blows away comments only
*/
void
nrrdCommentClear(Nrrd *nrrd) {
  int i;

  if (nrrd) {
    if (nrrd->comment) {
      i = 0;
      while (nrrd->comment[i]) {
	nrrd->comment[i] = airFree(nrrd->comment[i]);
	i++;
      }
      nrrd->comment = airFree(nrrd->comment);
    }
    nrrd->numComments = 0;
  }
}

/*
******** nrrdCommentScan()
**
** looks through comments of nrrd for something of the form
** " <key> : <val>"
** There can be whitespace anywhere it appears in the format above.
** Returns 1 on error, 0 if okay, BUT DOES NOT USE BIFF for errors.
** This is because an error here is probably not a big deal.
**
** The division between key and val is the first colon which appears.
** "*valP" is set to the first non-whitespace character after the colon
** No (non-temporary) string is allocated; *valP points into data 
** already pointed to by "nrrd"
**
** This function does not use biff.
*/
int
nrrdCommentScan(Nrrd *nrrd, char *key, char **valP) {
  /* char me[]="nrrdCommentScan";  */
  int i;
  char *cmt, *k, *c, *t;

  if (!(nrrd && key && strlen(key) && valP))
    return 1;

  *valP = NULL;
  if (!nrrd->comment) {
    /* no comments to scan */
    return 1;
  }
  if (strchr(key, ':')) {
    /* stupid key */
    return 1;
  }
  i = 0;
  while (nrrd->comment[i]) {
    cmt = nrrd->comment[i++];
    /* printf("%s: looking at comment \"%s\"\n", me, cmt); */
    if ((k = strstr(cmt, key)) && (c = strchr(cmt, ':'))) {
      /* is key before colon? */
      if (!( k+strlen(key) <= c ))
	goto nope;
      /* is there only whitespace before the key? */
      t = cmt;
      while (t < k) {
	if (!isspace(*t))
	  goto nope;
	t++;
      }
      /* is there only whitespace after the key? */
      t = k+strlen(key);
      while (t < c) {
	if (!isspace(*t))
	  goto nope;
	t++;
      }
      /* is there something after the colon? */
      t = c+1;
      while (isspace(*t)) {
	t++;
      }
      if (!*t)
	goto nope;
      /* t now points to beginning of "value" string; we're done */
      *valP = t;
      /* printf("%s: found \"%s\"\n", me, t); */
      break;
    }
  nope:
    *valP = NULL;
  }
  if (*valP)
    return 0;
  else 
    return 1;
}


/*
******** nrrdDescribe
** 
** writes verbose description of nrrd to given file
*/
void
nrrdDescribe(FILE *file, Nrrd *nrrd) {
  int i;
  char *cmt;

  if (file && nrrd) {
    fprintf(file, "Nrrd at 0x%lx:\n", (unsigned long)nrrd);
    fprintf(file, "Data at 0x%lx is " NRRD_BIG_INT_PRINTF 
	    " elements of type %s.\n",
	    (unsigned long)nrrd->data, nrrd->num, nrrdType2Str[nrrd->type]);
    if (nrrdTypeBlock == nrrd->type) 
      fprintf(file, "The blocks have size %d\n", nrrd->blockSize);
    if (strlen(nrrd->content))
      fprintf(file, "It is \"%s\"\n", nrrd->content);
    fprintf(file, "It is a %d-dimensional array, with axes:\n", nrrd->dim);
    for (i=0; i<=nrrd->dim-1; i++) {
      if (strlen(nrrd->label[i]))
	fprintf(file, "%d: (\"%s\") ", i, nrrd->label[i]);
      else
	fprintf(file, "%d: ", i);
      fprintf(file, "size=%d, spacing=%lg, \n    axis(Min,Max) = (%lg,%lg)\n",
	      nrrd->size[i], nrrd->spacing[i], 
	      nrrd->axisMin[i], nrrd->axisMax[i]);
    }
    fprintf(file, "The min and max values are %lg, %lg\n", 
	    nrrd->min, nrrd->max);
    if (nrrd->comment) {
      fprintf(file, "Comments:\n");
      i = 0;
      while (cmt = nrrd->comment[i]) {
	fprintf(file, "%s\n", cmt);
	i++;
      }
    }
    fprintf(file, "\n");
  }
}

int
nrrdCheck(Nrrd *nrrd) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdCheck";
  NRRD_BIG_INT mult;
  int i;

  if (!(nrrd->num >= 1)) {
    sprintf(err, "%s: number of elements is %d", me, (int)nrrd->num);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdTypeUnknown == nrrd->type) {
    sprintf(err, "%s: type of array is unknown", me);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == nrrd->type && -1 == nrrd->blockSize) {
    sprintf(err, "%s: type is \"block\" but no blocksize given", me);
    biffSet(NRRD, err); return 1;
  }
  if (!AIR_INSIDE(1, nrrd->dim, NRRD_MAX_DIM)) {
    sprintf(err, "%s: dimension %d is outside valid range [1,%d]",
	    me, nrrd->dim, NRRD_MAX_DIM);
    biffSet(NRRD, err); return 1;
  }
  mult = 1;
  for (i=0; i<=nrrd->dim-1; i++) {
    if (-1 == nrrd->size[i])
      mult = -1;
    else
      mult *= nrrd->size[i];
  }
  if (mult != nrrd->num) {
    sprintf(err, "%s: # elements (" NRRD_BIG_INT_PRINTF 
	    ") != product of axes sizes (" NRRD_BIG_INT_PRINTF ")", 
	    me, nrrd->num, mult);
    biffSet(NRRD, err); return 1;
  }
  return 0;
}

int
nrrdSameSize(Nrrd *n1, Nrrd *n2) {
  char me[]="nrrdSameSize", err[NRRD_MED_STRLEN];
  int i;

  if (!(n1 && n2)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 0;
  }
  if (n1->dim != n2->dim) {
    sprintf(err, "%s: n1->dim (%d) != n2->dim (%d)", me, n1->dim, n2->dim);
    biffSet(NRRD, err); return 0;
  }
  for (i=0; i<=n1->dim-1; i++) {
    if (n1->size[i] != n2->size[i]) {
      sprintf(err, "%s: n1->size[%d] (%d) != n2->size[%d] (%d)", me,
	      i, n1->size[i], i, n2->size[i]);
      biffSet(NRRD, err); return 0;
    }
  }
  return 1;
}

void
_nrrdInitResample(nrrdResampleInfo *info) {
  int i, d;

  for (d=0; d<=NRRD_MAX_DIM-1; d++) {
    info->kernel[d] = NULL;
    info->samples[d] = 0;
    info->param[d][0] = 1.0;
    for (i=1; i<=NRRD_MAX_KERNEL_PARAMS-1; i++)
      info->param[d][i] = AIR_NAN;
    info->min[d] = info->max[d] = AIR_NAN;
  }
  info->type = nrrdTypeUnknown;
  /* these may or may not be the best choices for default values */
  info->renormalize = AIR_TRUE;
  info->boundary = nrrdBoundaryBleed;
  info->padValue = 0.0;
}

nrrdResampleInfo *
nrrdResampleInfoNew(void) {
  nrrdResampleInfo *info;

  info = (nrrdResampleInfo*)(calloc(1, sizeof(nrrdResampleInfo)));
  if (info) {
    _nrrdInitResample(info);
  }
  return info;
}

nrrdResampleInfo *
nrrdResampleInfoNix(nrrdResampleInfo *info) {
  
  return airFree(info);
}

/*
******** nrrdElementSize()
**
** So just how many bytes long is one element in this nrrd?
** This is needed because some nrrds be of "block" type.
*/
int
nrrdElementSize(Nrrd *nrrd) {

  if (!nrrd ||
      !(nrrd->type > nrrdTypeUnknown &&
	nrrd->type < nrrdTypeLast)) {
    return -1;
  }
  if (nrrdTypeBlock != nrrd->type) {
    return nrrdTypeSize[nrrd->type];
  }
  else {
    return nrrd->blockSize;
  }
}

