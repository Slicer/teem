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

#ifdef __cplusplus
extern "C" {
#endif


#ifndef NRRD_PRIVATE_HAS_BEEN_INCLUDED
#define NRRD_PRIVATE_HAS_BEEN_INCLUDED

#if NRRD_RESAMPLE_FLOAT
#  define nrrdResample_nrrdType nrrdTypeFloat
#  define EVALN evalN_f               /* NrrdKernel method */
#else
#  define nrrdResample_nrrdType nrrdTypeDouble
#  define EVALN evalN_d               /* NrrdKernel method */
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
extern int _nrrdFieldOnePerAxis[NRRD_FIELD_MAX+1];
extern char _nrrdEnumFieldStr[NRRD_FIELD_MAX+1][AIR_STRLEN_SMALL];
extern int _nrrdFieldRequired[NRRD_FIELD_MAX+1];
extern int _nrrdFormatUsesDIO[NRRD_FORMAT_MAX+1];

/* simple.c */
extern int _nrrdContentSet_nva(Nrrd *nout, const char *func,
			       char *content, const char *format,
			       va_list arg);
extern int _nrrdContentSet(Nrrd *nout, const char *func,
			   char *content, const char *format, ...);


/* axes.c */
extern void _nrrdAxisInit(NrrdAxis *axis);
extern int _nrrdCenter(int center);

/* convert.c */
extern void (*_nrrdConv[][NRRD_TYPE_MAX+1])(void *,void *, size_t);

/* map.c */
extern int _nrrdMinMaxSet(Nrrd *nrrd);

/* read.c */
extern char _nrrdFieldStr[NRRD_FIELD_MAX+1][AIR_STRLEN_SMALL];
extern char _nrrdRelDirFlag[];
extern char _nrrdFieldSep[];
extern char _nrrdTableSep[];
extern int _nrrdSplitName(char *dir, char *base, const char *name);

/* parse.c */
extern int (*_nrrdReadNrrdParseInfo[NRRD_FIELD_MAX+1])(Nrrd *, NrrdIO *, int);
extern int _nrrdReadNrrdParseField(Nrrd *nrrd, NrrdIO *io, int useBiff);

/* methods.c */
extern int _nrrdSizeValid(int dim, int *size);
extern void _nrrdTraverse(Nrrd *nrrd);

#endif /* NRRD_PRIVATE_HAS_BEEN_INCLUDED */

#ifdef __cplusplus
}
#endif
