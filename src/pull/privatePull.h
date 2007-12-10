/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

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

/* infoPull.c */
extern unsigned int _pullInfoAnswerLen[PULL_INFO_MAX+1];
PULL_EXPORT void (*_pullInfoAnswerCopy[10])(double *, const double *);

/* methodsPull.c */
extern pullVolume *_pullVolumeCopy(pullVolume *pvol);

/* binningPull.c */
extern pullBin *_pullBinLocate(pullContext *pctx, double *pos);
extern void _pullBinPointAdd(pullContext *pctx,
                             pullBin *bin, pullPoint *point);

/* actionPull.c */
extern int _pullProbe(pullTask *task, pullPoint *point);

/* corePull.c */
extern int _pullContextCheck(pullContext *pctx);

/* setupPull.c */
extern int _pullInfoSetup(pullContext *pctx);
extern pullTask *_pullTaskNew(pullContext *pctx, int threadIdx);
extern pullTask *_pullTaskNix(pullTask *task);
extern int _pullTaskSetup(pullContext *pctx);
extern int _pullBinSetup(pullContext *pctx);
extern int _pullPointSetup(pullContext *pctx);

#ifdef __cplusplus
}
#endif
