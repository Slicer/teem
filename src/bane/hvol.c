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


#include "bane.h"

int
_maxMarg(baneHVolParm *hvp) {
  int marg;

  marg = 0;
  marg = AIR_MAX(marg, baneMeasrMargin[hvp->axp[0].measr]);
  marg = AIR_MAX(marg, baneMeasrMargin[hvp->axp[1].measr]);
  marg = AIR_MAX(marg, baneMeasrMargin[hvp->axp[2].measr]);
  return marg;
}

int
_baneValidMeasrParm(baneMeasrParm *mp) {
  char me[]="_baneValidMeasrParm", err[128];
  int i;
  
  if (!(mp->res >= 2)) {
    sprintf(err, "%s: need resolution at least 2 (not %d)", me, mp->res);
    biffSet(BANE, err); return 0;
  }
  if (!( AIR_INSIDE(baneMeasrUnknown+1, mp->measr, baneMeasrLast-1) )) {
    sprintf(err, "%s: measurement index %d outside range [%d,%d]", me,
	    mp->measr, baneMeasrUnknown+1, baneMeasrLast-1);
    biffSet(BANE, err); return 0;
  }
  if (!( AIR_INSIDE(baneIncUnknown+1, mp->inc, baneIncLast-1) )) {
    sprintf(err, "%s: inclusion index %d outside range [%d,%d]", me,
	    mp->inc, baneIncUnknown+1, baneIncLast-1);
    biffSet(BANE, err); return 0;
  }
  for (i=0; i<=baneIncNumParm[mp->inc]-1; i++) {
    if (!AIR_EXISTS(mp->incParm[i]))
      break;
  }
  if (i != baneIncNumParm[mp->inc]) {
    sprintf(err, "%s: got only %d parms for %s inclusion (not %d)", me, i,
	    baneIncStr[mp->inc], baneIncNumParm[mp->inc]);
    biffSet(BANE, err); return 0;
  }
  if ( (baneIncStdv == mp->inc || baneIncPercentile == mp->inc)
       && !(2 <= mp->incParm[0]) ) {
    sprintf(err, "%s: can't make histogram size %d for %s inclusion", me,
	    (int)mp->incParm[0], baneIncStr[mp->inc]);
    biffSet(BANE, err); return 0;
  }
  return 1;
}

int
_baneValidInput(Nrrd *nin, baneHVolParm *hvp) {
  char me[]="_baneValidInput", err[128];
  int i, marg;

  if (3 != nin->dim) {
    sprintf(err, "%s: need a 3-dimensional nrrd (not %d)", me, nin->dim);
    biffSet(BANE, err); return 0;
  }
  if (!( AIR_OPINSIDE(nrrdTypeUnknown, nin->type, nrrdTypeLast) &&
	 nin->type != nrrdTypeBlock )) {
    sprintf(err, "%s: must have a scalar type nrrd", me);
    biffSet(BANE, err); return 0;
  }
  if (!( AIR_EXISTS(nin->spacing[0]) && nin->spacing[0] > 0 &&
	 AIR_EXISTS(nin->spacing[1]) && nin->spacing[1] > 0 &&
	 AIR_EXISTS(nin->spacing[2]) && nin->spacing[2] > 0 )) {
    sprintf(err, "%s: must have positive spacing for all three axes", me);
    biffSet(BANE, err); return 0;
  }
  marg = _maxMarg(hvp);
  if (!( nin->size[0]-2*marg > 0 &&
	 nin->size[1]-2*marg > 0 &&
	 nin->size[2]-2*marg > 0 )) {
    sprintf(err, "%s: can't have measurement margin %d with sizes %d,%d,%d",
	    me, marg, nin->size[0], nin->size[1], nin->size[2]);
    biffSet(BANE, err); return 0;
  }
  for (i=0; i<=2; i++) {
    if (!_baneValidMeasrParm(hvp->axp + i)) {
      sprintf(err, "%s: trouble with measurement parm on axis %d", me, i);
      biffAdd(BANE, err); return 0;
    }
  }
  if (!( AIR_INSIDE(baneClipUnknown+1, hvp->clip, baneClipLast-1) )) {
    sprintf(err, "%s: clip method index %d outside range [%d,%d]",
	    me, hvp->clip, baneClipUnknown+1, baneClipLast-1);
    biffSet(BANE, err); return 0;
  }
  for (i=0; i<=baneClipNumParm[hvp->clip]-1; i++) {
    if (!AIR_EXISTS(hvp->clipParm[i]))
      break;
  }
  if (i != baneClipNumParm[hvp->clip]) {
    sprintf(err, "%s: got only %d parms for %s clipping (not %d)", me, i,
	    baneClipStr[hvp->clip], baneClipNumParm[hvp->clip]);
    biffSet(BANE, err); return 0;
  }
  return 1;
}

int
_baneFindInclusion(double min[3], double max[3], 
		   Nrrd *nin, baneHVolParm *hvp) {
  char me[]="_baneFindInclusion", err[128], prog[13];
  int marg, sx, sy, sz, x, y, z, incIdx0, incIdx1, incIdx2;
  double *incParm0, *incParm1, *incParm2;
  baneIncInitType init0, init1, init2;
  baneMeasrType msr0, msr1, msr2;
  Nrrd *n0, *n1, *n2;
  NRRD_BIG_INT idx;

  marg = _maxMarg(hvp);
  sx = nin->size[0];
  sy = nin->size[1];
  sz = nin->size[2];
  incIdx0 = hvp->axp[0].inc;
  incIdx1 = hvp->axp[1].inc;
  incIdx2 = hvp->axp[2].inc;
  /* printf("%s: HEY %d %d %d\n", me, incIdx0, incIdx1, incIdx2); */
  incParm0 = hvp->axp[0].incParm;
  incParm1 = hvp->axp[1].incParm;
  incParm2 = hvp->axp[2].incParm;
  msr0 = baneMeasr[hvp->axp[0].measr];
  msr1 = baneMeasr[hvp->axp[1].measr];
  msr2 = baneMeasr[hvp->axp[2].measr];
  n0 = baneIncNrrd[incIdx0](incParm0);
  n1 = baneIncNrrd[incIdx1](incParm1);
  n2 = baneIncNrrd[incIdx2](incParm2);
  if (!(n0 && n1 && n2)) {
    sprintf(err, "%s: trouble getting Nrrds for axis inclusion methods", me);
    biffAdd(BANE, err); return 1;
  }

  /* Determining the inclusion ranges for the histogram volume takes 
     some work- either finding the min and max values of some measure,
     and/or making a histogram of them.  Needed work for the three
     measures should done simultaneously during a given pass through
     the volume, so we break up the work into three stages- initA,
     initB, and then the final determination of ranges.  Here we start
     with initA.  If the chosen inclusion methods don't have anything
     to do at this stage (the callback is NULL), we don't do anything */
  if (hvp->verb) {
    fprintf(stderr, "%s: pass A of inclusion initialization ...       ", me);
    fflush(stderr);
  }
  init0 = baneIncInitA[incIdx0];
  init1 = baneIncInitA[incIdx1];
  init2 = baneIncInitA[incIdx2];
  if (init0 || init1 || init2) {
    for (z=marg; z<=sz-marg-1; z++) {
      for (y=marg; y<=sy-marg-1; y++) {
	if (hvp->verb && !((y+sy*z)%100)) {
	  fprintf(stderr, "%s", airDoneStr(0, y+sy*z, sy*sz, prog));
	}
	for (x=marg; x<=sx-marg-1; x++) {
	  idx = x + sx*(y + sy*z);
	  if (init0)
	    init0(n0, msr0(nin, idx));
	  if (init1)
	    init1(n1, msr1(nin, idx));
	  if (init2)
	    init2(n2, msr2(nin, idx));
	}
      }
    }
  }
  if (hvp->verb)
    fprintf(stderr, "\b\b\b\b\b\b  done\n");
  if (hvp->verb > 1) {
    printf("%s: after initA; ranges: [%g,%g] [%g,%g] [%g,%g]\n", me,
	   n0->axisMin[0], n0->axisMax[0], 
	   n1->axisMin[0], n1->axisMax[0], 
	   n2->axisMin[0], n2->axisMax[0]);
  }

  /* second stage of initialization, includes creating histograms */
  if (hvp->verb) {
    fprintf(stderr, "%s: pass B of inclusion initialization ...       ", me);
    fflush(stderr);
  }
  init0 = baneIncInitB[incIdx0];
  init1 = baneIncInitB[incIdx1];
  init2 = baneIncInitB[incIdx2];
  if (init0 || init1 || init2) {
    for (z=marg; z<=sz-marg-1; z++) {
      for (y=marg; y<=sy-marg-1; y++) {
	if (hvp->verb && !((y+sy*z)%100)) {
	  fprintf(stderr, "%s", airDoneStr(0, y+sy*z, sy*sz, prog));
	}
	for (x=marg; x<=sx-marg-1; x++) {
	  idx = x + sx*(y + sy*z);
	  /* printf("%s: ----- (%d,%d,%d) -----\n", me, x, y, z); */
	  if (init0)
	    init0(n0, msr0(nin, idx));
	  if (init1)
	    init1(n1, msr1(nin, idx));
	  if (init2)
	    init2(n2, msr2(nin, idx));
	}
      }
    }
  }
  if (hvp->verb)
    fprintf(stderr, "\b\b\b\b\b\b  done\n");
  if (hvp->verb > 1) {
    printf("%s: after initB; ranges: [%g,%g] [%g,%g] [%g,%g]\n", me,
	   n0->axisMin[0], n0->axisMax[0], 
	   n1->axisMin[0], n1->axisMax[0], 
	   n2->axisMin[0], n2->axisMax[0]);
  }

  /* now the real work of determining the inclusion */
  if (hvp->verb) {
    fprintf(stderr, "%s: determining inclusion ... ", me);
    fflush(stderr);
  }
  baneInc[incIdx0](min+0, max+0, n0, incParm0, 
		   baneMeasrRange[hvp->axp[0].measr]);
  baneInc[incIdx1](min+1, max+1, n1, incParm1, 
		   baneMeasrRange[hvp->axp[1].measr]);
  baneInc[incIdx2](min+2, max+2, n2, incParm2, 
		   baneMeasrRange[hvp->axp[2].measr]);
  if (hvp->verb)
    fprintf(stderr, "done\n");
  /*
  if (!(AIR_EXISTS(min[0]) && AIR_EXISTS(min[1]) && AIR_EXISTS(min[2]) && 
	AIR_EXISTS(max[0]) && AIR_EXISTS(max[1]) && AIR_EXISTS(max[2]))) {
    sprintf(err, "%s: failed to find ranges ([%g,%g], [%g,%g], [%g,%g])\n",
	    me, min[0], max[0], min[1], max[1], min[2], max[2]);
    biffSet(BANE, err); return 1;
  }
  */

  /* can safely nuke NULL */
  nrrdNuke(n0);
  nrrdNuke(n1);
  nrrdNuke(n2);
  return 0;
}

int
baneMakeHVol(Nrrd *hvol, Nrrd *nin, baneHVolParm *hvp) {
  char me[]="baneMakeHVol", err[128], prog[13];
  int sx, sy, sz, shx, shy, shz, x, y, z, hx, hy, hz, marg, *rhvdata, 
    clipVal, hval;
  NRRD_BIG_INT idx, hidx, included;
  double val0, val1, val2, min[3], max[3];
  baneMeasrType msr0, msr1, msr2;
  float fracIncluded;
  unsigned char *nhvdata;
  Nrrd *rawhvol;

  /* printf("%s: HEY %g %g\n", me, 
     hvp->axp[1].incParm[0], hvp->axp[1].incParm[1]); */
  if (!(hvol && nin && hvp)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(BANE, err); return 1;
  }
  if (!_baneValidInput(nin, hvp)) {
    sprintf(err, "%s: something wrong with input", me);
    biffAdd(BANE, err); return 1;
  }
  sx = nin->size[0];
  sy = nin->size[1];
  sz = nin->size[2];
  marg = _maxMarg(hvp);
  msr0 = baneMeasr[hvp->axp[0].measr];
  msr1 = baneMeasr[hvp->axp[1].measr];
  msr2 = baneMeasr[hvp->axp[2].measr];

  if (_baneFindInclusion(min, max, nin, hvp)) {
    sprintf(err, "%s: trouble finding inclusion ranges", me);
    biffAdd(BANE, err); return 1;
  }
  if (max[0] == min[0]) {
    max[0] += 1;
    if (hvp->verb)
      sprintf(err, "%s: fixing range 0 [%g,%g] --> [%g,%g]\n",
	      me, min[0], min[0], min[0], max[0]);
  }
  if (max[1] == min[1]) {
    max[1] += 1;
    if (hvp->verb)
      sprintf(err, "%s: fixing range 1 [%g,%g] --> [%g,%g]\n",
	      me, min[1], min[1], min[1], max[1]);
  }
  if (max[2] == min[2]) {
    max[2] += 1;
    if (hvp->verb)
      sprintf(err, "%s: fixing range 2 [%g,%g] --> [%g,%g]\n",
	      me, min[2], min[2], min[2], max[2]);
  }
  if (hvp->verb)
    printf("%s: inclusion: 0:[%g,%g], 1:[%g,%g], 2:[%g,%g]\n", me,
	   min[0], max[0], min[1], max[1], min[2], max[2]);
  
  /* construct the "raw" (un-clipped) histogram volume */
  if (hvp->verb) {
    fprintf(stderr, "%s: creating raw histogram volume ...       ", me);
    fflush(stderr);
  }
  shx = hvp->axp[0].res;
  shy = hvp->axp[1].res;
  shz = hvp->axp[2].res;
  rawhvol = nrrdNewAlloc(shx*shy*shz, nrrdTypeInt, 3);
  if (!rawhvol) {
    sprintf(err, "%s: couldn't allocate raw histovol (%dx%dx%d)", me,
	    shx, shy, shz);
    biffMove(BANE, err, NRRD); return 1;
  }
  rawhvol->size[0] = shx;
  rawhvol->size[1] = shy;
  rawhvol->size[2] = shz;
  rhvdata = rawhvol->data;
  included = 0;
  
  for (z=marg; z<=sz-marg-1; z++) {
    for (y=marg; y<=sy-marg-1; y++) {
      if (hvp->verb && !((y+sy*z)%100)) {
	fprintf(stderr, "%s", airDoneStr(0, y+sy*z, sy*sz, prog));
      }
      for (x=marg; x<=sx-marg-1; x++) {
	idx = x + sx*(y + sy*z);
	val0 = msr0(nin, idx);
	val1 = msr1(nin, idx);
	val2 = msr2(nin, idx);
	if (!( AIR_INSIDE(min[0], val0, max[0]) &&
	       AIR_INSIDE(min[1], val1, max[1]) &&
	       AIR_INSIDE(min[2], val2, max[2]) )) {
	  continue;
	}
	/* else this voxel will contribute to the histovol */
	AIR_INDEX(min[0], val0, max[0], shx, hx);
	AIR_INDEX(min[1], val1, max[1], shy, hy);
	AIR_INDEX(min[2], val2, max[2], shz, hz);
	hidx = hx + shx*(hy + shy*hz);
	if (rhvdata[hidx] < INT_MAX)
	  ++rhvdata[hidx];
	++included;
      }
    }
  }
  fracIncluded = (float)included/((sz-2*marg)*(sy-2*marg)*(sx-2*marg));
  if (fracIncluded < hvp->incLimit) {
    sprintf(err, "%s: included only %g%% of data, wanted at least %g%%",
	    me, 100*fracIncluded, 100*hvp->incLimit);
    biffSet(BANE, err); return 1;
  }
  if (hvp->verb) {
    fprintf(stderr, "\b\b\b\b\b\b  done\n");
    fprintf(stderr, "%s: included %g%% of original voxels\n", me, 
	    fracIncluded*100);
  }
  
  /* determine the clipping value and produce the final histogram volume */
  clipVal = baneClip[hvp->clip](rawhvol, hvp->clipParm);
  if (hvp->verb)
    fprintf(stderr, "%s: will clip at %d\n", me, clipVal);
  if (-1 == clipVal) {
    sprintf(err, "%s: trouble determining clip value", me);
    biffAdd(BANE, err); return 1;
  }
  if (hvp->verb) {
    fprintf(stderr, "%s: creating 8-bit histogram volume ...       ", me);
    fflush(stderr);
  }
  if (nrrdAlloc(hvol, shx*shy*shz, nrrdTypeUChar, 3)) {
    sprintf(err, "%s: couldn't alloc finished histovol", me);
    biffMove(BANE, err, NRRD); return 1;
  }
  hvol->size[0] = shx;
  hvol->size[1] = shy;
  hvol->size[2] = shz;
  hvol->axisMin[0] = min[0];
  hvol->axisMin[1] = min[1];
  hvol->axisMin[2] = min[2];
  hvol->axisMax[0] = max[0];
  hvol->axisMax[1] = max[1];
  hvol->axisMax[2] = max[2];
  strcpy(hvol->label[0], baneMeasrStr[hvp->axp[0].measr]);
  strcpy(hvol->label[1], baneMeasrStr[hvp->axp[1].measr]);
  strcpy(hvol->label[2], baneMeasrStr[hvp->axp[2].measr]);
  nhvdata = hvol->data;
  for (hz=0; hz<=shz-1; hz++) {
    for (hy=0; hy<=shy-1; hy++) {
      if (hvp->verb && !((hy+shy*hz)%100)) {
	fprintf(stderr, "%s", airDoneStr(0, hy+shy*hz, shy*shz, prog));
      }
      for (hx=0; hx<=shx-1; hx++) {
	hidx = hx + shx*(hy + shy*hz);
	AIR_INDEX(0, rhvdata[hidx], clipVal, 256, hval);
	hval = AIR_CLAMP(0, hval, 255);
	nhvdata[hidx] = hval;
      }
    }
  }
  if (hvp->verb)
    fprintf(stderr, "\b\b\b\b\b\b  done\n");
  nrrdNuke(rawhvol);

  return 0;
}

Nrrd *
baneGKMSHVol(Nrrd *nin, float perc) {
  char me[]="baneGKMSHVol", err[128];
  baneHVolParm *hvp;
  Nrrd *hvol;
  
  if (!(hvp = baneHVolParmNew())) {
    sprintf(err, "%s: couldn't get hvol parm struct", me);
    biffAdd(BANE, err); return NULL;
  }
  baneHVolParmGKMSInit(hvp);
  hvp->axp[0].incParm[1] = perc;
  hvp->axp[1].incParm[1] = perc;
  hvol = nrrdNew();
  if (baneMakeHVol(hvol, nin, hvp)) {
    sprintf(err, "%s: trouble making GKMS histogram volume", me);
    biffAdd(BANE, err); free(hvp); return NULL;
  }
  baneHVolParmNix(hvp);
  return hvol;
}

int
baneApplyMeasr(Nrrd *nout, Nrrd *nin, int measr) {
  char me[]="baneApplyMeasr", err[128];
  int sx, sy, sz, x, y, z, marg;
  baneMeasrType msr;
  NRRD_BIG_INT idx;
  float (*insert)(void *, NRRD_BIG_INT, float);
  
  if (3 != nin->dim) {
    sprintf(err, "%s: need a 3-dimensional nrrd (not %d)", me, nin->dim);
    biffSet(BANE, err); return 1;
  }
  if (!( AIR_OPINSIDE(nrrdTypeUnknown, nin->type, nrrdTypeLast) &&
	 nin->type != nrrdTypeBlock )) {
    sprintf(err, "%s: must have a scalar type nrrd", me);
    biffSet(BANE, err); return 1;
  }
  if (!( AIR_EXISTS(nin->spacing[0]) && nin->spacing[0] > 0 &&
	 AIR_EXISTS(nin->spacing[1]) && nin->spacing[1] > 0 &&
	 AIR_EXISTS(nin->spacing[2]) && nin->spacing[2] > 0 )) {
    sprintf(err, "%s: must have positive spacing for all three axes", me);
    biffSet(BANE, err); return 1;
  }

  sx = nin->size[0];
  sy = nin->size[1];
  sz = nin->size[2];
  marg = baneMeasrMargin[measr];
  msr = baneMeasr[measr];
  insert = nrrdFInsert[nrrdTypeFloat];

  if (nrrdAlloc(nout, sx*sy*sz, nrrdTypeFloat, 3)) {
    sprintf(err, "%s: couldn't alloc output nrrd", me);
    biffMove(BANE, err, NRRD); return 1;
  }
  nout->size[0] = sx;
  nout->size[1] = sy;
  nout->size[2] = sz;
  nout->spacing[0] = nin->spacing[0];
  nout->spacing[1] = nin->spacing[1];
  nout->spacing[2] = nin->spacing[2];
  insert = nrrdFInsert[nrrdTypeFloat];
  for (z=marg; z<=sz-marg-1; z++) {
    for (y=marg; y<=sy-marg-1; y++) {
      for (x=marg; x<=sx-marg-1; x++) {
	idx = x + sx*(y + sy*z);
	insert(nout->data, idx, msr(nin, idx));
      }
    }
  }
  return 0;
}
