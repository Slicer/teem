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

/*
** learned: don't ever count on interleaved printfs to stdout
** and stderr to appear in the right order
*/

#include "bane.h"
#include "private.h"

int
_baneValidAxis(baneAxis *ax) {
  char me[]="_baneValidAxis", err[AIR_STRLEN_MED];
  int i;
  
  if (!(ax->res >= 2)) {
    sprintf(err, "%s: need resolution at least 2 (not %d)", me, ax->res);
    biffAdd(BANE, err); return 0;
  }
  if (!ax->measr) {
    sprintf(err, "%s: have NULL baneMeasr", me);
    biffAdd(BANE, err); return 0;
  }
  for (i=0; i<ax->measr->numParm; i++) {
    if (!AIR_EXISTS(ax->measrParm[i])) {
      sprintf(err, "%s: didn't get %d parms for %s measurement",
	      me, ax->measr->numParm, ax->measr->name);
      biffAdd(BANE, err); return 0;
    }
  }
  if (!ax->inc) {
    sprintf(err, "%s: have NULL baneInc", me);
    biffAdd(BANE, err); return 0;
  }
  for (i=0; i<ax->inc->numParm; i++) {
    if (!AIR_EXISTS(ax->incParm[i])) {
      sprintf(err, "%s: didn't get %d parms for %s inclusion",
	      me, ax->inc->numParm, ax->inc->name);
      biffAdd(BANE, err); return 0;
    }
  }
  if (_baneInc_HistNew == ax->inc->histNew) {
    /* a histogram is needed for inclusion */
    if (!( 3 < ax->incParm[0] )) {
      sprintf(err, "%s: won't make a size-%d histogram for %s inclusion",
	      me, (int)(ax->incParm[0]), ax->inc->name);
    }
  }

  /* all okay */
  return 1;
}

int
_baneFindInclusion(double min[3], double max[3], 
		   Nrrd *nin, baneHVolParm *hvp, gageContext *ctx) {
  char me[]="_baneFindInclusion", err[AIR_STRLEN_MED], prog[13];
  int sx, sy, sz, x, y, z;
  baneInc *inc[3];
  double *incParm[3], *measrParm[3];
  baneMeasr *measr[3];
  Nrrd *hist[3];
  baneIncPass *incPass[3];
  gageSclAnswer *san;
  
  /* conveniance copies */
  sx = nin->axis[0].size;
  sy = nin->axis[1].size;
  sz = nin->axis[2].size;
  inc[0] = hvp->ax[0].inc;
  inc[1] = hvp->ax[1].inc;
  inc[2] = hvp->ax[2].inc;
  incParm[0] = hvp->ax[0].incParm;
  incParm[1] = hvp->ax[1].incParm;
  incParm[2] = hvp->ax[2].incParm;
  measr[0] = hvp->ax[0].measr;
  measr[1] = hvp->ax[1].measr;
  measr[2] = hvp->ax[2].measr;
  measrParm[0] = hvp->ax[0].measrParm;
  measrParm[1] = hvp->ax[1].measrParm;
  measrParm[2] = hvp->ax[2].measrParm;
  san = (gageSclAnswer *)(ctx->pvl[0]->ansStruct);
  if (hvp->verbose) {
    fprintf(stderr, "%s: inclusions: %s %s %s\n", me,
	    inc[0]->name, inc[1]->name, inc[2]->name);
    fprintf(stderr, "%s: measures: %s %s %s\n", me,
	    measr[0]->name, measr[1]->name, measr[2]->name);
    fprintf(stderr, "%s: gage query:\n", me);
    ctx->pvl[0]->kind->queryPrint(stderr, ctx->pvl[0]->query);
  }

  hist[0] = inc[0]->histNew(incParm[0]);
  hist[1] = inc[1]->histNew(incParm[1]);
  hist[2] = inc[2]->histNew(incParm[2]);
  if (!(hist[0] && hist[1] && hist[2])) {
    sprintf(err, "%s: trouble getting Nrrds for axis inclusions", me);
    biffAdd(BANE, err); return 1;
  }

  /* Determining the inclusion ranges for the histogram volume takes
     some work- either finding the min and max values of some measure,
     and/or making a histogram of them.  The needed work for the three
     measures should done simultaneously during a given pass through
     the volume, so we break up the work into three stages- "passA",
     "passB", and then the final determination of ranges, "ans".  Here
     we start with passA.  If the chosen inclusion methods don't have
     anything to do at this stage (the callback is NULL), we don't do
     anything */
  if (hvp->verbose) {
    fprintf(stderr, "%s: pass A of inclusion initialization ...       ", me);
    fflush(stderr);
  }
  incPass[0] = inc[0]->passA;
  incPass[1] = inc[1]->passA;
  incPass[2] = inc[2]->passA;
  if (incPass[0] || incPass[1] || incPass[2]) {
    /*
    fprintf(stderr, "%s: inclusion pass CBs = %p %p %p \n", me, 
	    incPass[0], incPass[1], incPass[2]);
    */
    for (z=0; z<sz; z++) {
      for (y=0; y<sy; y++) {
	if (hvp->verbose && !((y+sy*z)%100)) {
	  fprintf(stderr, "%s", airDoneStr(0, y+sy*z, sy*sz, prog));
	  fflush(stderr);
	}
	for (x=0; x<sx; x++) {
	  gageProbe(ctx, x, y, z);
	  /*
	  fprintf(stderr, "## _baneFindInclusion: (%d,%d,%d) -> (%g,%g,%g)\n",
		  x,y,z, measr[0]->ans(san, measrParm[0]),
		  measr[1]->ans(san, measrParm[1]),
		  measr[2]->ans(san, measrParm[2]));
	  */
	  if (incPass[0])
	    incPass[0](hist[0], measr[0]->ans(san, measrParm[0]), incParm[0]);
	  if (incPass[1])
	    incPass[1](hist[1], measr[1]->ans(san, measrParm[1]), incParm[1]);
	  if (incPass[2])
	    incPass[2](hist[2], measr[2]->ans(san, measrParm[2]), incParm[2]);
	}
      }
    }
  }
  if (hvp->verbose)
    fprintf(stderr, "\b\b\b\b\b\b  done\n");
  if (hvp->verbose > 1) {
    fprintf(stderr, "%s: after pass A; ranges: [%g,%g] [%g,%g] [%g,%g]\n", me,
	    hist[0]->axis[0].min, hist[0]->axis[0].max, 
	    hist[1]->axis[0].min, hist[1]->axis[0].max, 
	    hist[2]->axis[0].min, hist[2]->axis[0].max);
  }

  /* second stage of initialization, includes creating histograms */
  if (hvp->verbose) {
    fprintf(stderr, "%s: pass B of inclusion initialization ...       ", me);
    fflush(stderr);
  }
  incPass[0] = inc[0]->passB;
  incPass[1] = inc[1]->passB;
  incPass[2] = inc[2]->passB;
  if (incPass[0] || incPass[1] || incPass[2]) {
    for (z=0; z<sz; z++) {
      for (y=0; y<sy; y++) {
	if (hvp->verbose && !((y+sy*z)%100)) {
	  fprintf(stderr, "%s", airDoneStr(0, y+sy*z, sy*sz, prog));
	  fflush(stderr);
	}
	for (x=0; x<sx; x++) {
	  gageProbe(ctx, x, y, z);
	  if (incPass[0])
	    incPass[0](hist[0], measr[0]->ans(san, measrParm[0]), incParm[0]);
	  if (incPass[1])
	    incPass[1](hist[1], measr[1]->ans(san, measrParm[1]), incParm[1]);
	  if (incPass[2])
	    incPass[2](hist[2], measr[2]->ans(san, measrParm[2]), incParm[2]);
	}
      }
    }
  }
  if (hvp->verbose)
    fprintf(stderr, "\b\b\b\b\b\b  done\n");
  if (hvp->verbose > 1) {
    fprintf(stderr, "%s: after pass B; ranges: [%g,%g] [%g,%g] [%g,%g]\n", me,
	    hist[0]->axis[0].min, hist[0]->axis[0].max, 
	    hist[1]->axis[0].min, hist[1]->axis[0].max, 
	    hist[2]->axis[0].min, hist[2]->axis[0].max);
  }

  /* now the real work of determining the inclusion */
  if (hvp->verbose) {
    fprintf(stderr, "%s: determining inclusion ... ", me);
    fflush(stderr);
  }
  inc[0]->ans(0 + min, 0 + max, hist[0], incParm[0], measr[0]->range);
  inc[1]->ans(1 + min, 1 + max, hist[1], incParm[1], measr[1]->range);
  inc[2]->ans(2 + min, 2 + max, hist[2], incParm[2], measr[2]->range);
  if (hvp->verbose)
    fprintf(stderr, "done\n");

  nrrdNuke(hist[0]);
  nrrdNuke(hist[1]);
  nrrdNuke(hist[2]);
  return 0;
}

int
baneMakeHVol(Nrrd *hvol, Nrrd *nin, baneHVolParm *hvp) {
  char me[]="baneMakeHVol", err[AIR_STRLEN_MED], prog[13];
  gageContext *ctx;
  gagePerVolume *pvl;
  gageSclAnswer *san;
  int E, sx, sy, sz, shx, shy, shz, x, y, z, hx, hy, hz,
    *rhvdata, clipVal, hval;
  /* these are doubles because ultimately the inclusion functions
     use doubles, because I wanted the most generality */
  double val[3], min[3], max[3], *measrParm[3];
  baneMeasr *measr[3];
  size_t hidx, included;
  float fracIncluded;
  unsigned char *nhvdata;
  Nrrd *rawhvol;

  /* printf("%s: HEY %g %g\n", me, 
     hvp->axp[1].incParm[0], hvp->axp[1].incParm[1]); */
  if (!(hvol && nin && hvp)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(BANE, err); return 1;
  }
  if (!baneValidInput(nin, hvp)) {
    sprintf(err, "%s: something wrong with input volume", me);
    biffAdd(BANE, err); return 1;
  }

  /* set up */
  sx = nin->axis[0].size;
  sy = nin->axis[1].size;
  sz = nin->axis[2].size;
  measr[0] = hvp->ax[0].measr;
  measr[1] = hvp->ax[1].measr;
  measr[2] = hvp->ax[2].measr;
  measrParm[0] = hvp->ax[0].measrParm;
  measrParm[1] = hvp->ax[1].measrParm;
  measrParm[2] = hvp->ax[2].measrParm;

  /* create the gageSimple and initialize it */
  ctx = gageContextNew();
  pvl = gagePerVolumeNew(gageKindScl);
  gageSet(ctx, gageVerbose, 0);
  gageSet(ctx, gageRenormalize, hvp->renormalize);
  gageSet(ctx, gageCheckIntegrals, AIR_TRUE);
  gageSet(ctx, gageK3Pack, AIR_TRUE);
  E = 0;
  if (!E) E |= gagePerVolumeAttach(ctx, pvl);
  if (!E) E |= gageKernelSet(ctx, gageKernel00, hvp->k[gageKernel00],
			     hvp->kparm[gageKernel00]);
  if (!E) E |= gageKernelSet(ctx, gageKernel11, hvp->k[gageKernel11],
			     hvp->kparm[gageKernel11]);
  if (!E) E |= gageKernelSet(ctx, gageKernel22, hvp->k[gageKernel22],
			     hvp->kparm[gageKernel22]);
  if (!E) E |= gageVolumeSet(ctx, pvl, nin);
  if (!E) E |= gageQuerySet(ctx, pvl, (hvp->ax[0].measr->query |
				       hvp->ax[1].measr->query |
				       hvp->ax[2].measr->query));
  if (!E) E |= gageUpdate(ctx);
  if (E) {
    sprintf(err, "%s: trouble setting up gage", me);
    biffMove(BANE, err, GAGE); return 1;
  }
  san = (gageSclAnswer *)(ctx->pvl[0]->ansStruct);
  
  fprintf(stderr, "##%s: calling _baneFindInclusion ...\n", me);
  if (_baneFindInclusion(min, max, nin, hvp, ctx)) {
    sprintf(err, "%s: trouble finding inclusion ranges", me);
    biffAdd(BANE, err); return 1;
  }
  fprintf(stderr, "##%s: _baneFindInclusion done\n", me);
  if (max[0] == min[0]) {
    max[0] += 1;
    if (hvp->verbose)
      sprintf(err, "%s: fixing range 0 [%g,%g] --> [%g,%g]\n",
	      me, min[0], min[0], min[0], max[0]);
  }
  if (max[1] == min[1]) {
    max[1] += 1;
    if (hvp->verbose)
      sprintf(err, "%s: fixing range 1 [%g,%g] --> [%g,%g]\n",
	      me, min[1], min[1], min[1], max[1]);
  }
  if (max[2] == min[2]) {
    max[2] += 1;
    if (hvp->verbose)
      sprintf(err, "%s: fixing range 2 [%g,%g] --> [%g,%g]\n",
	      me, min[2], min[2], min[2], max[2]);
  }
  if (hvp->verbose)
    fprintf(stderr, "%s: inclusion: 0:[%g,%g], 1:[%g,%g], 2:[%g,%g]\n", me,
	    min[0], max[0], min[1], max[1], min[2], max[2]);
  
  /* construct the "raw" (un-clipped) histogram volume */
  if (hvp->verbose) {
    fprintf(stderr, "%s: creating raw histogram volume ...       ", me);
    fflush(stderr);
  }
  shx = hvp->ax[0].res;
  shy = hvp->ax[1].res;
  shz = hvp->ax[2].res;
  if (nrrdAlloc(rawhvol=nrrdNew(), nrrdTypeInt, 3, shx, shy, shz)) {
    sprintf(err, "%s: couldn't allocate raw histovol (%dx%dx%d)", me,
	    shx, shy, shz);
    biffMove(BANE, err, NRRD); return 1;
  }
  rhvdata = rawhvol->data;
  included = 0;
  
  for (z=0; z<sz; z++) {
    for (y=0; y<sy; y++) {
      if (hvp->verbose && !((y+sy*z)%100)) {
	fprintf(stderr, "%s", airDoneStr(0, y+sy*z, sy*sz, prog));
	fflush(stderr);
      }
      for (x=0; x<sx; x++) {
	gageProbe(ctx, x, y, z);
	val[0] = measr[0]->ans(san, measrParm[0]);
	val[1] = measr[1]->ans(san, measrParm[1]);
	val[2] = measr[2]->ans(san, measrParm[2]);
	if (!( AIR_INSIDE(min[0], val[0], max[0]) &&
	       AIR_INSIDE(min[1], val[1], max[1]) &&
	       AIR_INSIDE(min[2], val[2], max[2]) )) {
	  continue;
	}
	/* else this voxel will contribute to the histovol */
	AIR_INDEX(min[0], val[0], max[0], shx, hx);
	AIR_INDEX(min[1], val[1], max[1], shy, hy);
	AIR_INDEX(min[2], val[2], max[2], shz, hz);
	hidx = hx + shx*(hy + shy*hz);
	if (rhvdata[hidx] < INT_MAX)
	  ++rhvdata[hidx];
	++included;
      }
    }
  }
  fracIncluded = (float)included/(sz*sy*sx);
  if (fracIncluded < hvp->incLimit) {
    sprintf(err, "%s: included only %g%% of data, wanted at least %g%%",
	    me, 100*fracIncluded, 100*hvp->incLimit);
    biffAdd(BANE, err); return 1;
  }
  if (hvp->verbose) {
    fprintf(stderr, "\b\b\b\b\b\b  done\n");
    fprintf(stderr, "%s: included %g%% of original voxels\n", me, 
	    fracIncluded*100);
  }
  
  /* determine the clipping value and produce the final histogram volume */
  clipVal = hvp->clip->ans(rawhvol, hvp->clipParm);
  if (-1 == clipVal) {
    sprintf(err, "%s: trouble determining clip value", me);
    biffAdd(BANE, err); return 1;
  }
  if (hvp->verbose)
    fprintf(stderr, "%s: will clip at %d\n", me, clipVal);
  if (hvp->verbose) {
    fprintf(stderr, "%s: creating 8-bit histogram volume ...       ", me);
    fflush(stderr);
  }
  if (nrrdAlloc(hvol, nrrdTypeUChar, 3, shx, shy, shz)) {
    sprintf(err, "%s: couldn't alloc finished histovol", me);
    biffMove(BANE, err, NRRD); return 1;
  }
  hvol->axis[0].min = min[0];
  hvol->axis[1].min = min[1];
  hvol->axis[2].min = min[2];
  hvol->axis[0].max = max[0];
  hvol->axis[1].max = max[1];
  hvol->axis[2].max = max[2];
  hvol->axis[0].label = airStrdup(hvp->ax[0].measr->name);
  hvol->axis[1].label = airStrdup(hvp->ax[1].measr->name);
  hvol->axis[2].label = airStrdup(hvp->ax[2].measr->name);
  hvol->axis[0].center = nrrdCenterCell;
  hvol->axis[1].center = nrrdCenterCell;
  hvol->axis[2].center = nrrdCenterCell;
  nhvdata = hvol->data;
  for (hz=0; hz<shz; hz++) {
    for (hy=0; hy<shy; hy++) {
      if (hvp->verbose && !((hy+shy*hz)%100)) {
	fprintf(stderr, "%s", airDoneStr(0, hy+shy*hz, shy*shz, prog));
	fflush(stderr);
      }
      for (hx=0; hx<shx; hx++) {
	hidx = hx + shx*(hy + shy*hz);
	AIR_INDEX(0, rhvdata[hidx], clipVal, 256, hval);
	hval = AIR_CLAMP(0, hval, 255);
	nhvdata[hidx] = hval;
      }
    }
  }
  if (hvp->verbose)
    fprintf(stderr, "\b\b\b\b\b\b  done\n");
  nrrdNuke(rawhvol);

  return 0;
}

Nrrd *
baneGKMSHVol(Nrrd *nin, float gradPerc, float hessPerc) {
  char me[]="baneGKMSHVol", err[AIR_STRLEN_MED];
  baneHVolParm *hvp;
  Nrrd *hvol;
  
  if (!(hvp = baneHVolParmNew())) {
    sprintf(err, "%s: couldn't get hvol parm struct", me);
    biffAdd(BANE, err); return NULL;
  }
  baneHVolParmGKMSInit(hvp);
  hvp->ax[0].incParm[1] = gradPerc;
  hvp->ax[1].incParm[1] = hessPerc;
  hvol = nrrdNew();
  if (baneMakeHVol(hvol, nin, hvp)) {
    sprintf(err, "%s: trouble making GKMS histogram volume", me);
    biffAdd(BANE, err); free(hvp); return NULL;
  }
  baneHVolParmNix(hvp);
  return hvol;
}

/*
int
baneApplyMeasr(Nrrd *nout, Nrrd *nin, int measr) {
  char me[]="baneApplyMeasr", err[AIR_STRLEN_MED];
  int sx, sy, sz, x, y, z, marg;
  baneMeasrType msr;
  nrrdBigInt idx;
  float (*insert)(void *, nrrdBigInt, float);
  
  if (3 != nin->dim) {
    sprintf(err, "%s: need a 3-dimensional nrrd (not %d)", me, nin->dim);
    biffAdd(BANE, err); return 1;
  }
  if (!( AIR_BETWEEN(nrrdTypeUnknown, nin->type, nrrdTypeLast) &&
	 nin->type != nrrdTypeBlock )) {
    sprintf(err, "%s: must have a scalar type nrrd", me);
    biffAdd(BANE, err); return 1;
  }
  if (!( AIR_EXISTS(nin->axis[0].spacing) && nin->axis[0].spacing > 0 &&
	 AIR_EXISTS(nin->axis[1].spacing) && nin->axis[1].spacing > 0 &&
	 AIR_EXISTS(nin->axis[2].spacing) && nin->axis[2].spacing > 0 )) {
    sprintf(err, "%s: must have positive spacing for all three axes", me);
    biffAdd(BANE, err); return 1;
  }

  sx = nin->axis[0].size;
  sy = nin->axis[1].size;
  sz = nin->axis[2].size;
  marg = baneMeasrMargin[measr];
  msr = baneMeasr[measr];

  if (nrrdAlloc(nout, nrrdTypeFloat, 3, sx, sy, sz)) {
    sprintf(err, "%s: couldn't alloc output nrrd", me);
    biffMove(BANE, err, NRRD); return 1;
  }
  nout->axis[0].spacing = nin->axis[0].spacing;
  nout->axis[1].spacing = nin->axis[1].spacing;
  nout->axis[2].spacing = nin->axis[2].spacing;
  insert = nrrdFInsert[nrrdTypeFloat];
  for (z=marg; z<sz-marg; z++) {
    for (y=marg; y<sy-marg; y++) {
      for (x=marg; x<sx-marg; x++) {
	idx = x + sx*(y + sy*z);
	insert(nout->data, idx, msr(nin, idx));
      }
    }
  }
  return 0;
}
*/
