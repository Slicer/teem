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


#ifndef NRRD_PRIVATE_HAS_BEEN_INCLUDED
#define NRRD_PRIVATE_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

#define _NRRD_COMMENT_CHAR '#'
#define _NRRD_TABLE_INCR 256

typedef union {
  char **CP;
  int *I;
  unsigned int *UI;
  double *D;
  void *P;
} _nrrdAxesInfoPtrs;

/* arrays.c */
extern int _nrrdFieldValidInPNM[NRRD_FIELD_MAX+1];
extern int _nrrdFieldValidInTable[NRRD_FIELD_MAX+1];
extern char _nrrdEnumFieldStr[NRRD_FIELD_MAX+1][NRRD_STRLEN_SMALL];
extern int _nrrdFieldRequired[NRRD_FIELD_MAX+1];
extern int _nrrdFormatUsesDIO[NRRD_FORMAT_MAX+1];

/* axes.c */
extern void _nrrdAxisInit(nrrdAxis *axis);

/* convert.c */
extern void (*_nrrdConv[][NRRD_TYPE_MAX+1])(void *,void *, nrrdBigInt);

/* accessors.c */
extern void   (*_nrrdMinMaxFind[NRRD_TYPE_MAX+1])(void *minP, void *maxP, 
						  nrrdBigInt, void *data);

/* read.c */
extern char _nrrdFieldStr[NRRD_FIELD_MAX+1][NRRD_STRLEN_SMALL];
extern char _nrrdRelDirFlag[];
extern char _nrrdFieldSep[];
extern char _nrrdTableSep[];
extern int _nrrdSplitName(char *dir, char *base, char *name);

/* parse.c */
extern int (*_nrrdReadNrrdParseInfo[NRRD_FIELD_MAX+1])(Nrrd *, nrrdIO *, int);
extern int _nrrdReadNrrdParseField(Nrrd *nrrd, nrrdIO *io, int useBiff);

/* extern C */
#ifdef __cplusplus
}
#endif
#endif /* NRRD_PRIVATE_HAS_BEEN_INCLUDED */
