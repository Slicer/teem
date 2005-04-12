/*
  Teem: Gordon Kindlmann's research software
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

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

#ifndef PUSH_PRIVATE_HAS_BEEN_INCLUDED
#define PUSH_PRIVATE_HAS_BEEN_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* defaultsPush.c */
extern int _pushVerbose;

/* forces.c */
extern airEnum *pushForceEnum;

/* binning.c */
extern int _pushBinFind(pushContext *pctx, push_t *pos);
extern void _pushBinPointAdd(pushContext *pctx, int bi, int pi);
extern void _pushBinPointRemove(pushContext *pctx, int bi, int losePii);
extern void _pushBinPointsAllAdd(pushContext *pctx);
extern int _pushBinPointsRebin(pushContext *pctx);
extern int _pushBinNeighborhoodFind(pushContext *pctx, int *nei,
                                    int bin, int dimIn);

/* corePush.c */
extern void _pushProcessDummy(pushTask *task, int bin,
                              const push_t *parm);

/* action.c */
extern void _pushTenInv(pushContext *pctx, push_t *inv, push_t *ten);
extern int _pushBinPointsRebin(pushContext *pctx);
extern void _pushProbe(pushContext *pctx, gageContext *gctx, push_t *pos);
extern int _pushInputProcess(pushContext *pctx);
extern void _pushInitialize(pushContext *pctx);
extern void _pushRepel(pushTask *task, int bin, const push_t *parm);
extern void _pushUpdate(pushTask *task, int bin, const push_t *parm);

#ifdef __cplusplus
}
#endif

#endif /* PUSH_PRIVATE_HAS_BEEN_INCLUDED */
