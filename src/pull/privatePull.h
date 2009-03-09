/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
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

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifdef __cplusplus
extern "C" {
#endif

/* this has to be big enough to do experiments where binning is turned off */
#define _PULL_NEIGH_MAXNUM 4096

/* used by pullBinsPointMaybeAdd; don't add a point of its (normalized)
   distance to an existing point is less than this */
#define _PULL_BINNING_MAYBE_ADD_THRESH 0.15

/* don't nix a point if this (or greater) fraction of its neighbors
   have already been nixed */
#define _PULL_FRAC_NIXED_THRESH 0.4

/* only try adding a point if the normalized neighbor offset sum is 
   greater than this (making this too small only wastes time, by descending
   and testing a point that can't help reduce energy */
#define _PULL_NEIGH_OFFSET_SUM_THRESH 0.1

/* how far to place new points from isolated points (as a fraction of
   radiusSpace), when not using cubic well energy */
#define _PULL_NEWPNT_DIST 0.5

/* scaling factor between point->neighDistMean and distance cap; higher
   values allow for more adventurous explorations... */
#define _PULL_DIST_CAP_RSNORM 2.0

/* travel distance limit in terms of voxelSizeSpace and voxelSizeScale */
#define _PULL_DIST_CAP_VOXEL 1.5

/* where along s axis to probe energySpecS to see if its attractive or
   repulsive along scale */
#define _PULL_TARGET_DIM_S_PROBE 0.05

/* tentative new points aren't allowed to move further than this (in 
   rs-normalized space) from the original old point */
#define _PULL_NEWPNT_STRAY_DIST 1.7

/* fraction of bboxMax[3]-bboxMin[3] to use as step along scale
   for discrete differencing needed to find the gradient of strength */
#define _PULL_STRENGTH_ENERGY_DELTA_SCALE 0.001

/* number of iterations we allow something to be continuously stuck
   before nixing it */
#define _PULL_STUCK_ITER_NUM_MAX 5

/* volumePull.c */
extern pullVolume *_pullVolumeCopy(const pullVolume *pvol);
extern int _pullVolumeSetup(pullContext *pctx);
extern int _pullInsideBBox(pullContext *pctx, double pos[4]);

/* infoPull.c */
extern unsigned int _pullInfoAnswerLen[PULL_INFO_MAX+1];
extern void (*_pullInfoAnswerCopy[10])(double *, const double *);
extern int _pullInfoSetup(pullContext *pctx);

/* contextPull.c */
extern int _pullContextCheck(pullContext *pctx);

/* taskPull.c */
extern pullTask *_pullTaskNew(pullContext *pctx, int threadIdx);
extern pullTask *_pullTaskNix(pullTask *task);
extern int _pullTaskSetup(pullContext *pctx);
extern void _pullTaskFinish(pullContext *pctx);

/* actionPull.c */
extern int _pullPraying;
extern double _pullPrayCorner[2][2][3];
extern size_t _pullPrayRes[2];
extern double _pullDistLimit(pullTask *task, pullPoint *point);
extern double _pullEnergyFromPoints(pullTask *task, pullBin *bin,
                                    pullPoint *point, 
                                    /* output */
                                    double egradSum[4]);
extern double _pullPointEnergyTotal(pullTask *task, pullBin *bin,
                                    pullPoint *point, int ignoreImage,
                                    double force[4]);
extern int _pullPointProcessDescent(pullTask *task, pullBin *bin,
                                    pullPoint *point, int ignoreImage);
extern double _pullEnergyInterParticle(pullTask *task,
                                       pullPoint *me, pullPoint *she, 
                                       double spaceDist, double scaleDist,
                                       double egrad[4]);

/* constraints.c */
extern int _pullConstraintSatisfy(pullTask *task, pullPoint *point,
                                  int *constrFailP);
extern void _pullConstraintTangent(pullTask *task, pullPoint *point, 
                                   double proj[9]);
extern double _pullConstraintDim(pullContext *pctx,
				 pullTask *task, pullPoint *point);

/* pointPull.c */
extern double _pullPointScalar(const pullContext *pctx,
                               const pullPoint *point, int sclInfo,
                               double grad[4], double hess[9]);
extern void _pullPointCopy(pullPoint *dst, const pullPoint *src,
                           unsigned int ilen);
extern void _pullPointHistInit(pullPoint *point);
extern void _pullPointHistAdd(pullPoint *point, int cond);
extern double _pullStepInterAverage(const pullContext *pctx);
extern double _pullStepConstrAverage(const pullContext *pctx);
extern double _pullEnergyTotal(const pullContext *pctx);
extern int _pullProbe(pullTask *task, pullPoint *point);
extern void _pullPointStepEnergyScale(pullContext *pctx, double scale);
extern int _pullPointSetup(pullContext *pctx);
extern void _pullPointFinish(pullContext *pctx);

/* popcntl.c */
extern int _pullPointProcessNeighLearn(pullTask *task, pullBin *bin,
                                       pullPoint *point);
extern int _pullPointProcessAdding(pullTask *task, pullBin *bin,
                                   pullPoint *point);
extern int _pullPointProcessNixing(pullTask *task, pullBin *bin,
                                   pullPoint *point);
extern int _pullIterFinishAdding(pullContext *pctx);
extern int _pullIterFinishNixing(pullContext *pctx);
extern void _pullNixTheNixed(pullContext *pctx);

/* binningPull.c */
extern void _pullBinInit(pullBin *bin, unsigned int incr);
extern void _pullBinDone(pullBin *bin);
extern pullBin *_pullBinLocate(pullContext *pctx, double *pos);
extern void _pullBinPointAdd(pullContext *pctx,
                             pullBin *bin, pullPoint *point);
extern void _pullBinPointRemove(pullContext *pctx, pullBin *bin, int loseIdx);
extern void _pullBinNeighborSet(pullBin *bin, pullBin **nei, unsigned int num);
extern int _pullBinSetup(pullContext *pctx);
extern int _pullIterFinishDescent(pullContext *pctx);
extern void _pullBinFinish(pullContext *pctx);

/* corePull.c */
extern int _pullProcess(pullTask *task);
extern void *_pullWorker(void *_task);

#ifdef __cplusplus
}
#endif
