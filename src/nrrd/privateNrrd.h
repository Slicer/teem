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

#ifdef _WIN32
#  include <io.h>
#  include <fcntl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define _NRRD_TEXT_INCR      1024
#define _NRRD_LLONG_MAX_HELP AIR_LLONG(2305843009213693951)
#define _NRRD_LLONG_MIN_HELP AIR_LLONG(-2305843009213693952)

#define _NRRD_WHITESPACE_NOTAB " \n\r\v\f" /* K+R pg. 157 */

/* ---- BEGIN non-NrrdIO */

#if NRRD_RESAMPLE_FLOAT
#  define nrrdResample_nrrdType nrrdTypeFloat
#  define EVALN                 evalN_f /* NrrdKernel method */
#else
#  define nrrdResample_nrrdType nrrdTypeDouble
#  define EVALN                 evalN_d /* NrrdKernel method */
#endif

/* to access whatever nrrd there may be in in a NrrdIter */
#define _NRRD_ITER_NRRD(iter) ((iter)->nrrd ? (iter)->nrrd : (iter)->ownNrrd)

/* ---- END non-NrrdIO */

/*
** _NRRD_SPACING
**
** returns nrrdDefSpacing if the argument doesn't exist, otherwise
** returns the argument
*/
#define _NRRD_SPACING(spc) (AIR_EXISTS(spc) ? spc : nrrdDefSpacing)

typedef union {
  char **CP;
  int *I;
  unsigned int *UI;
  size_t *ST;
  double *D;
  const void *P;
  double (*V)[NRRD_SPACE_DIM_MAX];
} _nrrdAxisInfoSetPtrs;

typedef union {
  char **CP;
  int *I;
  unsigned int *UI;
  size_t *ST;
  double *D;
  void *P;
  double (*V)[NRRD_SPACE_DIM_MAX];
} _nrrdAxisInfoGetPtrs;

/* defaultsNrrd.c */
extern airLLong _nrrdLLongMaxHelp(airLLong val);
extern airLLong _nrrdLLongMinHelp(airLLong val);
extern airULLong _nrrdULLongMaxHelp(airULLong val);

/* keyvalue.c */
extern void _nrrdWriteEscaped(FILE *file, char *dst, const char *str,
                              const char *toescape, const char *tospace);
extern int _nrrdKeyValueWrite(FILE *file, char **stringP, const char *prefix,
                              const char *key, const char *value);

/* formatXXX.c */
extern const char *const _nrrdFormatURLLine0;
extern const char *const _nrrdFormatURLLine1;
extern const NrrdFormat _nrrdFormatNRRD;
extern const NrrdFormat _nrrdFormatPNM;
extern const NrrdFormat _nrrdFormatPNG;
extern const NrrdFormat _nrrdFormatVTK;
extern const NrrdFormat _nrrdFormatText;
extern const NrrdFormat _nrrdFormatEPS;
extern int _nrrdHeaderCheck(Nrrd *nrrd, NrrdIoState *nio, int checkSeen);
extern int _nrrdFormatNRRD_whichVersion(const Nrrd *nrrd, NrrdIoState *nio);

/* encodingXXX.c */
extern const NrrdEncoding _nrrdEncodingRaw;
extern const NrrdEncoding _nrrdEncodingAscii;
extern const NrrdEncoding _nrrdEncodingHex;
extern const NrrdEncoding _nrrdEncodingGzip;
extern const NrrdEncoding _nrrdEncodingBzip2;
extern const NrrdEncoding _nrrdEncodingZRL;

/* arrays.c */
extern const int _nrrdFieldValidInImage[NRRD_FIELD_MAX + 1];
extern const int _nrrdFieldValidInText[NRRD_FIELD_MAX + 1];
extern const int _nrrdFieldOnePerAxis[NRRD_FIELD_MAX + 1];
extern const int _nrrdFieldRequired[NRRD_FIELD_MAX + 1];

/* simple.c */
extern char *_nrrdContentGet(const Nrrd *nin);
extern int _nrrdContentSet_nva(Nrrd *nout, const char *func, char *content,
                               const char *format, va_list arg);
extern int _nrrdContentSet_va(Nrrd *nout, const char *func, char *content,
                              const char *format, ...);
extern int (*const _nrrdFieldCheck[NRRD_FIELD_MAX + 1])(const Nrrd *nrrd, int useBiff);
extern void _nrrdSplitSizes(size_t *pieceSize, size_t *pieceNum, Nrrd *nrrd,
                            unsigned int listDim);

/* axis.c */
extern int _nrrdKindAltered(int kindIn, int resampling);
extern void _nrrdAxisInfoCopy(NrrdAxisInfo *dest, const NrrdAxisInfo *src, int bitflag);
extern void _nrrdAxisInfoInit(NrrdAxisInfo *axis);
extern void _nrrdAxisInfoNewInit(NrrdAxisInfo *axis);
extern int _nrrdCenter(int center);
extern int _nrrdCenter2(int center, int def);
/* ---- BEGIN non-NrrdIO */
extern int _nrrdDblcmp(double aa, double bb);

/* convertNrrd.c */
extern void (*const _nrrdConv[][NRRD_TYPE_MAX + 1])(void *, const void *, size_t);
extern void (*const _nrrdClampConv[][NRRD_TYPE_MAX + 1])(void *, const void *, size_t);
extern void (*const _nrrdCastClampRound[][NRRD_TYPE_MAX + 1])(void *, const void *,
                                                              size_t, int doClamp,
                                                              int roundd);
/* ---- END non-NrrdIO */

/* read.c */
extern const char *const _nrrdFieldSep;
extern const char *const _nrrdTextSep;
extern const char *const _nrrdNoSpaceVector;
extern int _nrrdByteSkipSkip(FILE *dataFile, Nrrd *nrrd, NrrdIoState *nio,
                             long int byteSkip);
extern int _nrrdCalloc(Nrrd *nrrd, NrrdIoState *nio, FILE *file);
extern void _nrrdSplitName(char **dirP, char **baseP, const char *name);

/* write.c */
extern int _nrrdFieldInteresting(const Nrrd *nrrd, NrrdIoState *nio, int field);
extern void _nrrdSprintFieldInfo(char **strP, const char *prefix, const Nrrd *nrrd,
                                 NrrdIoState *nio, int field, int dropAxis0);
extern void _nrrdFprintFieldInfo(FILE *file, const char *prefix, const Nrrd *nrrd,
                                 NrrdIoState *nio, int field, int dropAxis0);

/* parseNrrd.c */
extern int _nrrdReadNrrdParseField(NrrdIoState *nio, int useBiff);

/* methodsNrrd.c */
extern int _nrrdCopy(Nrrd *nout, const Nrrd *nin, int bitflag);
extern int _nrrdSizeCheck(const size_t *size, unsigned int dim, int useBiff);
extern int _nrrdMaybeAllocMaybeZero_nva(Nrrd *nrrd, int type, unsigned int dim,
                                        const size_t *size, int zeroWhenNoAlloc);

#if TEEM_ZLIB
#  if TEEM_VTK_MANGLE
#    include "vtk_zlib_mangle.h"
#  endif
#  include <zlib.h> /* NrrdIO-hack-004 */

/* gzio.c */
extern gzFile _nrrdGzOpen(FILE *fd, const char *mode);
extern int _nrrdGzClose(gzFile file);
extern int _nrrdGzRead(gzFile file, void *buf, unsigned int len, unsigned int *read);
extern int _nrrdGzWrite(gzFile file, const void *buf, unsigned int len,
                        unsigned int *written);
#else
extern int _nrrdGzDummySymbol(void);
#endif

/* ---- BEGIN non-NrrdIO */
/* apply1D.c */
extern double _nrrdApplyDomainMin(const Nrrd *nmap, int ramps, int mapAxis);
extern double _nrrdApplyDomainMax(const Nrrd *nmap, int ramps, int mapAxis);

/* ---- END non-NrrdIO */

#ifdef __cplusplus
}
#endif
