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

#include "ten.h"
#include "tenPrivate.h"

int
_tenEpiRegCheck(Nrrd *nout, Nrrd **nin, int ninLen, Nrrd *ngrad,
		int reference,
		float bwX, float bwY, float B0thr, float DWthr,
		NrrdKernel *kern, double *kparm) {
  char me[]="_tenEpiRegCheck", err[AIR_STRLEN_MED];
  int ni;

  if (!( nout && nin && ngrad && kern && kparm )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  /* what is this for? */
  if (!( ninLen >= 3 )) {
    sprintf(err, "%s: given ninLen (%d) not >= 3", me, ninLen);
    biffAdd(TEN, err); return 1;
  }
  if (tenGradCheck(ngrad)) {
    sprintf(err, "%s: problem with given gradient list", me);
    biffAdd(TEN, err); return 1;
  }
  if (ninLen-1 != ngrad->axis[1].size) {
    sprintf(err, "%s: got %d DWIs, but %d gradient directions", me,
	    ninLen-1, ngrad->axis[1].size);
    biffAdd(TEN, err); return 1;
  }
  for (ni=0; ni<ninLen; ni++) {
    if (nrrdCheck(nin[ni])) {
      sprintf(err, "%s: basic nrrd validity failed on nin[%d]", me, ni);
      biffMove(TEN, err, NRRD); return 1;
    }
    if (!nrrdSameSize(nin[0], nin[ni], AIR_TRUE)) {
      sprintf(err, "%s: nin[%d] is different from nin[0]", me, ni);
      biffMove(TEN, err, NRRD); return 1;
    }
  }
  if (!( 3 == nin[0]->dim )) {
    sprintf(err, "%s: didn't get a set of 3-D arrays (got %d-D)", me,
	    nin[0]->dim);
    biffAdd(TEN, err); return 1;
  }
  if (!( AIR_IN_CL(0, reference, ninLen-1) )) {
    sprintf(err, "%s: reference index %d not in valid range [0,%d]", 
	    me, reference, ninLen-1);
    biffAdd(TEN, err); return 1;
  }
  if (!( AIR_EXISTS(bwX) && AIR_EXISTS(bwY) )) {
    sprintf(err, "%s: bwX, bwY don't both exist", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( bwX >= 0 && bwY >= 0 )) {
    sprintf(err, "%s: bwX (%g) and bwY (%g) are not both non-negative",
	    me, bwX, bwY);
    biffAdd(TEN, err); return 1;
  }
  return 0;
}

/*
** this assumes that all nblur[i] are valid nrrds, and does nothing
** to manage them
*/
int
_tenEpiRegBlur(Nrrd **nblur, Nrrd **nin, int ninLen,
	       float bwX, float bwY, int verb) {
  char me[]="_tenEpiRegBlur", err[AIR_STRLEN_MED];
  NrrdResampleInfo *rinfo;
  airArray *mop;
  int ni, sx, sy, sz;
  double savemin[2], savemax[2];

  if (!( bwX || bwY )) {
    if (verb) {
      fprintf(stderr, "%s:\n            ", me); fflush(stderr);
    }
    for (ni=0; ni<ninLen; ni++) {
      if (verb) {
	fprintf(stderr, "% 2d ", ni); fflush(stderr);
      }
      if (nrrdCopy(nblur[ni], nin[ni])) {
	sprintf(err, "%s: trouble copying nin[%d]", me, ni);
	biffMove(TEN, err, NRRD); return 1;
      }
    }
    if (verb) {
      fprintf(stderr, "done\n");
    }
    return 0;
  }
  /* else we need to blur */
  sx = nin[0]->axis[0].size;
  sy = nin[0]->axis[1].size;
  sz = nin[0]->axis[2].size;
  mop = airMopNew();
  rinfo = nrrdResampleInfoNew();
  airMopAdd(mop, rinfo, (airMopper)nrrdResampleInfoNix, airMopAlways);
  if (bwX) {
    rinfo->kernel[0] = nrrdKernelGaussian;
    rinfo->parm[0][0] = bwX;
    rinfo->parm[0][1] = 3.0; /* how many stnd devs do we cut-off at */
  }
  if (bwY) {
    rinfo->kernel[1] = nrrdKernelGaussian;
    rinfo->parm[1][0] = bwY;
    rinfo->parm[1][1] = 3.0; /* how many stnd devs do we cut-off at */
  }
  rinfo->kernel[2] = NULL;
  ELL_3V_SET(rinfo->samples, sx, sy, sz);
  ELL_3V_SET(rinfo->min, 0, 0, 0);
  ELL_3V_SET(rinfo->max, sx-1, sy-1, sz-1);
  rinfo->boundary = nrrdBoundaryBleed;
  rinfo->type = nrrdTypeUnknown;
  rinfo->renormalize = AIR_TRUE;
  rinfo->clamp = AIR_TRUE;
  if (verb) {
    fprintf(stderr, "%s:\n            ", me); fflush(stderr);
  }
  for (ni=0; ni<ninLen; ni++) {
    if (verb) {
      fprintf(stderr, "% 2d ", ni); fflush(stderr);
    }
    savemin[0] = nin[ni]->axis[0].min; savemax[0] = nin[ni]->axis[0].max; 
    savemin[1] = nin[ni]->axis[1].min; savemax[1] = nin[ni]->axis[1].max;
    nin[ni]->axis[0].min = 0; nin[ni]->axis[0].max = sx-1;
    nin[ni]->axis[1].min = 0; nin[ni]->axis[1].max = sy-1;
    if (nrrdSpatialResample(nblur[ni], nin[ni], rinfo)) {
      sprintf(err, "%s: trouble blurring nin[%d]", me, ni);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    nin[ni]->axis[0].min = savemin[0]; nin[ni]->axis[0].max = savemax[0]; 
    nin[ni]->axis[1].min = savemin[1]; nin[ni]->axis[1].max = savemax[1];
  }
  if (verb) {
    fprintf(stderr, "done\n");
  }
  airMopOkay(mop);
  return 0;
}

int
_tenEpiRegFindValley(int *valIdxP, Nrrd *nhist) {
  char me[]="_tenEpiRegFindValley", err[AIR_STRLEN_MED];
  double gparm[NRRD_KERNEL_PARMS_NUM], dparm[NRRD_KERNEL_PARMS_NUM];
  Nrrd *ntmpA, *ntmpB, *nhistD, *nhistDD;
  float *histD, *histDD;
  airArray *mop;
  int bb, bins;

  mop = airMopNew();
  airMopAdd(mop, ntmpA=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ntmpB=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nhistD=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nhistDD=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);

  bins = nhist->axis[0].size;
  gparm[0] = bins/50;  /* wacky heuristic for gaussian stdev */
  gparm[1] = 3;        /* how many stdevs to cut-off at */
  dparm[0] = 1.0;      /* unit spacing */
  dparm[1] = 1.0;      /* B-Spline kernel */
  dparm[2] = 0.0;
  if (nrrdCheapMedian(ntmpA, nhist, AIR_FALSE, 2, 1.0, 1024)
      || nrrdSimpleResample(ntmpB, ntmpA,
			    nrrdKernelGaussian, gparm, &bins, NULL)
      || nrrdSimpleResample(nhistD, ntmpB,
			    nrrdKernelBCCubicD, dparm, &bins, NULL)
      || nrrdSimpleResample(nhistDD, ntmpB,
			    nrrdKernelBCCubicDD, dparm, &bins, NULL)) {
    sprintf(err, "%s: trouble processing histogram", me);
    biffMove(TEN, err, NRRD), airMopError(mop); return 1;
  }
  histD = (float*)(nhistD->data);
  histDD = (float*)(nhistDD->data);
  for (bb=0; bb<bins-1; bb++) {
    if (histD[bb]*histD[bb+1] < 0 && histDD[bb] > 0) {
      /* zero-crossing in 1st deriv, positive 2nd deriv */
      break;
    }
  }
  if (bb == bins-1) {
    sprintf(err, "%s: never saw a satisfactory zero crossing", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }

  *valIdxP = bb;
  airMopOkay(mop);
  return 0;
}

int
_tenEpiRegFindThresh(float *B0thrP, float *DWthrP, Nrrd **nin, int ninLen) {
  char me[]="_tenEpiRegFindThresh", err[AIR_STRLEN_MED];
  Nrrd *nhist, *ntmp;
  airArray *mop;
  int ni, val, bins, E;
  double min=0, max=0;

  mop = airMopNew();
  airMopAdd(mop, nhist=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ntmp=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);

  nrrdMinMaxSet(nin[0]);
  bins = AIR_MIN(1024, nin[0]->max - nin[0]->min + 1);
  if (nrrdHisto(nhist, nin[0], NULL, bins, nrrdTypeFloat)) {
    sprintf(err, "%s: problem forming B0 histogram", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }
  if (_tenEpiRegFindValley(&val, nhist)) {
    sprintf(err, "%s: problem finding B0 histogram valley", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  *B0thrP = nrrdAxisPos(nhist, 0, 0.85*val); /* another wacky hack */
  fprintf(stderr, "%s: using %g for B0 threshold\n", me, *B0thrP);

  for (ni=1; ni<ninLen; ni++) {
    nrrdMinMaxSet(nin[ni]);
    if (1 == ni) {
      min = nin[ni]->min;
      max = nin[ni]->max;
    } else {
      min = AIR_MIN(min, nin[ni]->min);
      max = AIR_MAX(max, nin[ni]->max);
    }
  }
  bins = AIR_MIN(1024, max - min + 1);
  ntmp->axis[0].min = min;
  ntmp->axis[0].max = max;
  for (ni=1; ni<ninLen; ni++) {
    if (nrrdHisto(ntmp, nin[ni], NULL, bins, nrrdTypeFloat)) {
      sprintf(err, "%s: problem forming histogram of DWI %d", me, ni);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    if (1 == ni) {
      E = nrrdCopy(nhist, ntmp);
    } else {
      E = nrrdArithBinaryOp(nhist, nrrdBinaryOpAdd, nhist, ntmp);
    }
    if (E) {
      sprintf(err, "%s: problem updating histogram sum on DWI %d", me, ni);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
  }
  if (_tenEpiRegFindValley(&val, nhist)) {
    sprintf(err, "%s: problem finding DWI histogram valley", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  *DWthrP = nrrdAxisPos(nhist, 0, 0.85*val); /* another wacky hack */
  fprintf(stderr, "%s: using %g for DWI threshold\n", me, *DWthrP);
  
  airMopOkay(mop);
  return 0;
}

int
_tenEpiRegThreshold(Nrrd **nthresh, Nrrd **nblur, int ninLen,
		    float B0thr, float DWthr, int verb) {
  char me[]="_tenEpiRegThreshold", err[AIR_STRLEN_MED];
  airArray *mop;
  int I, sx, sy, sz, ni;
  float val;
  unsigned char *thr;

  if (!( AIR_EXISTS(B0thr) && AIR_EXISTS(DWthr) )) {
    if (_tenEpiRegFindThresh(&B0thr, &DWthr, nblur, ninLen)) {
      sprintf(err, "%s: trouble with automatic threshold determination", me);
      biffAdd(TEN, err); return 1;
    }
  }
  
  mop = airMopNew();
  if (verb) {
    fprintf(stderr, "%s:\n            ", me); fflush(stderr);
  }
  sx = nblur[0]->axis[0].size;
  sy = nblur[0]->axis[1].size;
  sz = nblur[0]->axis[2].size;
  for (ni=0; ni<ninLen; ni++) {
    if (verb) {
      fprintf(stderr, "% 2d ", ni); fflush(stderr);
    }
    if (nrrdMaybeAlloc(nthresh[ni], nrrdTypeUChar, 3, sx, sy, sz)) {
      sprintf(err, "%s: trouble allocating threshold %d", me, ni);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    thr = (unsigned char *)(nthresh[ni]->data);
    for (I=0; I<sx*sy*sz; I++) {
      val = nrrdFLookup[nblur[ni]->type](nblur[ni]->data, I);
      val -= !ni ? B0thr : DWthr;
      thr[I] = (val >= 0 ? 1 : 0);
    }
  }
  if (verb) {
    fprintf(stderr, "done\n");
  }
  
  airMopOkay(mop); 
  return 0;
}

/*
** _tenEpiRegBB: find the biggest bright CC
*/
int
_tenEpiRegBB(Nrrd *nval, Nrrd *nsize) {
  unsigned char *val;
  int ci, *size, big;

  val = (unsigned char *)(nval->data);
  size = (int *)(nsize->data);
  big = 0;
  for (ci=0; ci<nsize->axis[0].size; ci++) {
    big = val[ci] ? AIR_MAX(big, size[ci]) : big;
  }
  return big;
}

int
_tenEpiRegCC(Nrrd **nthr, int ninLen, int conny, int verb) {
  char me[]="_tenEpiRegCC", err[AIR_STRLEN_MED];
  Nrrd *nslc, *ncc, *nval, *nsize;
  airArray *mop;
  int ni, z, sz, big, E;

  if (verb) {
    fprintf(stderr, "%s:\n            ", me); fflush(stderr);
  }
  mop = airMopNew();
  airMopAdd(mop, nslc=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nval=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ncc=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nsize=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  sz = nthr[0]->axis[2].size;
  for (ni=0; ni<ninLen; ni++) {
    if (verb) {
      fprintf(stderr, "% 2d ", ni); fflush(stderr);
    }
#if 0
    /* within each slice, we merge to dark little pieces (smaller than
       brightSize), pieces, and merge to bright big pieces (smaller then
       darkSize) .  The problem with this is when a 3-D connected piece
       of brain is seen as detached in a slice, as happens at the top of
       the cortex, and sometimes in the temporal lobes */
    for (z=0; z<sz; z++) {
      if ( nrrdSlice(nslc, nthr[ni], 2, z)
	   || nrrdCCFind(ncc, &nval, nslc, nrrdTypeUnknown, conny)
	   || nrrdCCMerge(ncc, ncc, nval, 1, darkSize, 0, conny)
	   || nrrdCCMerge(ncc, ncc, nval, -1, brightSize, 0, conny)
	   || nrrdCCRevalue(nslc, ncc, nval)
	   || nrrdSplice(nthr[ni], nthr[ni], nslc, 2, z) ) {
	sprintf(err, "%s: trouble processing slice %d of nthr[%d]", me, z, ni);
	biffMove(TEN, err, NRRD); return 1;
      }
      num = nrrdCCNum(ncc);
      if (2 != num) {
	fprintf(stderr, "%s: slice %d of nthr[%d] has %d CCs\n",
		me, z, ni, num);
      }
    }
#else
    /* for each volume, we find the biggest bright 3-D CC, and merge
       down (to dark) all smaller bright pieces.  Then, within each
       slice, we do 2-D CCs, find the biggest bright CC (size == big),
       and merge up (to bright) all small dark pieces, where
       (currently) small is big/2 */
    E = 0;
    if (!E) E |= nrrdCCFind(ncc, &nval, nthr[ni], nrrdTypeUnknown, conny);
    if (!E) E |= nrrdCCSize(nsize, ncc);
    if (!E) big = _tenEpiRegBB(nval, nsize);
    if (!E) E |= nrrdCCMerge(ncc, ncc, nval, -1, big-1, 0, conny);
    if (!E) E |= nrrdCCRevalue(nthr[ni], ncc, nval);
    if (E) {
      sprintf(err, "%s: trouble processing nthr[%d]", me, ni);
      biffMove(TEN, err, NRRD); return 1;
    }
    for (z=0; z<sz; z++) {
      if ( nrrdSlice(nslc, nthr[ni], 2, z)
	   || nrrdCCFind(ncc, &nval, nslc, nrrdTypeUnknown, conny)
	   || nrrdCCSize(nsize, ncc)
	   || !(big = _tenEpiRegBB(nval, nsize))
	   || nrrdCCMerge(ncc, ncc, nval, 1, big/2, 0, conny)
	   || nrrdCCRevalue(nslc, ncc, nval)
	   || nrrdSplice(nthr[ni], nthr[ni], nslc, 2, z) ) {
	sprintf(err, "%s: trouble processing slice %d of nthr[%d]", me, z, ni);
	biffMove(TEN, err, NRRD); return 1;
      }
    }
#endif
  }
  if (verb) {
    fprintf(stderr, "done\n");
  }

  airMopOkay(mop);
  return 0;
}

#define MEAN_X 0 
#define MEAN_Y 1
#define M_02   2
#define M_11   3
#define M_20   4

/*
** _tenEpiRegMoments()
**
** the moments are stored in (of course) a nrrd, one scanline per slice,
** with each scanline containing:
**
**       0       1       2       3       4
**   mean(x)  mean(y)  M_02    M_11    M_20
*/
int
_tenEpiRegMoments(Nrrd **nmom, Nrrd **nthresh, int ninLen, int verb) {
  char me[]="_tenEpiRegMoments", err[AIR_STRLEN_MED];
  int sx, sy, sz, xi, yi, zi, ni;
  double N, mx, my, cx, cy, x, y, M02, M11, M20, *mom;
  float val;
  unsigned char *thr;

  sx = nthresh[0]->axis[0].size;
  sy = nthresh[0]->axis[1].size;
  sz = nthresh[0]->axis[2].size;
  if (verb) {
    fprintf(stderr, "%s:\n            ", me); fflush(stderr);
  }
  for (ni=0; ni<ninLen; ni++) {
    if (verb) {
      fprintf(stderr, "% 2d ", ni); fflush(stderr);
    }
    if (nrrdMaybeAlloc(nmom[ni], nrrdTypeDouble, 2, 5, sz)) {
      sprintf(err, "%s: couldn't allocate nmom[%d]", me, ni);
      biffMove(TEN, err, NRRD); return 1;
    }
    nrrdAxesSet(nmom[ni], nrrdAxesInfoLabel, "mx,my,h,s,t", "z");
    thr = (unsigned char *)(nthresh[ni]->data);
    mom = (double *)(nmom[ni]->data);
    for (zi=0; zi<sz; zi++) {
      /* ------ find mx, my */
      N = 0;
      mx = my = 0.0;
      for (yi=0; yi<sy; yi++) {
	for (xi=0; xi<sx; xi++) {
	  val = thr[xi + sx*yi];
	  N += val;
	  mx += xi*val;
	  my += yi*val;
	}
      }
      if (!N) {
	sprintf(err, "%s: saw no non-zero pixels in nthresh[%d]; "
		"%s threshold too high?", me, ni, !ni ? "B0" : "DW");
	biffAdd(TEN, err); return 1;
      }
      if (N == sx*sy) {
	sprintf(err, "%s: saw only non-zero pixels in nthresh[%d]; "
		"%s threshold too low?", me, ni, !ni ? "B0" : "DW");
	biffAdd(TEN, err); return 1;
      }
      mx /= N;
      my /= N;
      cx = sx/2.0;
      cy = sy/2.0;
      /* ------ find M02, M11, M20 */
      M02 = M11 = M20 = 0.0;
      for (yi=0; yi<sy; yi++) {
	for (xi=0; xi<sx; xi++) {
	  val = thr[xi + sx*yi];
	  x = xi - cx;
	  y = yi - cy;
	  M02 += y*y*val;
	  M11 += x*y*val;
	  M20 += x*x*val;
	}
      }
      M02 /= N;
      M11 /= N;
      M20 /= N;
      /* ------ set output */
      mom[MEAN_X] = mx;
      mom[MEAN_Y] = my;
      mom[M_02] = M02;
      mom[M_11] = M11;
      mom[M_20] = M20;
      thr += sx*sy;
      mom += 5;
    }
  }
  if (verb) {
    fprintf(stderr, "done\n");
  }

  return 0;
}

/*
** _tenEpiRegPairXforms
**
** uses moment information to compute all pair-wise transforms, which are
** stored in the 3 x ninLen x ninLen x sizeZ output.  If xfr = npxfr->data,
** xfr[0 + 3*(zi + sz*(A + ninLen*B))] is shear,
** xfr[1 +              "            ] is scale, and 
** xfr[2 +              "            ] is translate in the transform
** that maps slice zi from volume A to volume B.
*/
int
_tenEpiRegPairXforms(Nrrd *npxfr, Nrrd **nmom, int ninLen) {
  char me[]="_tenEpiRegPairXforms", err[AIR_STRLEN_MED];
  double *xfr, *A, *B, hh, ss, tt;
  int ai, bi, zi, sz;
  
  sz = nmom[0]->axis[1].size;
  if (nrrdMaybeAlloc(npxfr, nrrdTypeDouble, 4,
		     5, sz, ninLen, ninLen)) {
    sprintf(err, "%s: couldn't allocate transform nrrd", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  nrrdAxesSet(npxfr, nrrdAxesInfoLabel, "mx,my,h,s,t", "zi", "orig", "txfd");
  xfr = (double *)(npxfr->data);
  for (bi=0; bi<ninLen; bi++) {
    for (ai=0; ai<ninLen; ai++) {
      for (zi=0; zi<sz; zi++) {
	A = (double*)(nmom[ai]->data) + 5*zi;
	B = (double*)(nmom[bi]->data) + 5*zi;
	ss = sqrt((A[M_20]*B[M_02] - B[M_11]*B[M_11]) /
		  (A[M_20]*A[M_02] - A[M_11]*A[M_11]));
	hh = (B[M_11] - ss*A[M_11])/A[M_20];
	tt = B[MEAN_Y] - A[MEAN_Y];
	ELL_5V_SET(xfr + 5*(zi + sz*(ai + ninLen*bi)),
		   A[MEAN_X], A[MEAN_Y], hh, ss, tt);
      }
    }
  }
  return 0;
}

#define SHEAR  2
#define MAG    3
#define TRAN   4

int
_tenEpiRegEstimHST(Nrrd *nhst, Nrrd *npxfr, int ninLen, Nrrd *ngrad) {
  char me[]="_tenEpiRegEstimHST", err[AIR_STRLEN_MED];
  double *hst, *grad, *mat1, *mat2, *vec, *ans, *pxfr, *gA, *gB;
  int z, sz, A, B, npairs, ri;
  Nrrd *nmat1, *nmat2, *nvec, *ninv, *nans;
  airArray *mop;

  mop = airMopNew();
  airMopAdd(mop, nmat1=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nmat2=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ninv=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nvec=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nans=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  npairs = (ninLen-1)*(ninLen-2);
  sz = npxfr->axis[1].size;
  if (nrrdMaybeAlloc(nhst, nrrdTypeDouble, 2, 9, sz)
      || nrrdMaybeAlloc(nmat1, nrrdTypeDouble, 2, 3, npairs)
      || nrrdMaybeAlloc(nmat2, nrrdTypeDouble, 2, 5, npairs)
      || nrrdMaybeAlloc(nvec, nrrdTypeDouble, 2, 1, npairs)) {
    sprintf(err, "%s: couldn't allocate HST nrrd", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }
  nrrdAxesSet(nhst, nrrdAxesInfoLabel, "Hx,Hy,Hz,Sx,Sy,Sz,Tx,Ty,Tz", "z");
  grad = (double *)(ngrad->data);
  mat1 = (double *)(nmat1->data);
  mat2 = (double *)(nmat2->data);
  vec = (double *)(nvec->data);

  /* ------ find Mx, My, Mz per slice */
  for (z=0; z<sz; z++) {
    hst = (double *)(nhst->data) + 0 + 9*z;
    ri = 0;
    for (A=1; A<ninLen; A++) {
      for (B=1; B<ninLen; B++) {
	if (A == B) continue;
	pxfr = (double *)(npxfr->data) + 0 + 5*(z + sz*(A + ninLen*B));
	gA = grad + 0 + 3*(A-1);
	gB = grad + 0 + 3*(B-1);
	ELL_3V_SET(mat1 + 3*ri,
		   pxfr[MAG]*gA[0] - gB[0],
		   pxfr[MAG]*gA[1] - gB[1],
		   pxfr[MAG]*gA[2] - gB[2]);
	vec[ri] = 1 - pxfr[MAG];
	ri += 1;
      }
    }
    ellNmPseudoInverse(ninv, nmat1);
    ellNmMultiply(nans, ninv, nvec);
    ans = (double *)(nans->data);
    hst[3] = ans[0];
    hst[4] = ans[1];
    hst[5] = ans[2];
  }

  /* ------ find Sx, Sy, Sz per slice */
  for (z=0; z<sz; z++) {
    hst = (double *)(nhst->data) + 0 + 9*z;
    ri = 0;
    for (A=1; A<ninLen; A++) {
      for (B=1; B<ninLen; B++) {
	if (A == B) continue;
	pxfr = (double *)(npxfr->data) + 0 + 5*(z + sz*(A + ninLen*B));
	gA = grad + 0 + 3*(A-1);
	gB = grad + 0 + 3*(B-1);
	ELL_3V_SET(mat1 + 3*ri,
		   gB[0] - pxfr[MAG]*gA[0],
		   gB[1] - pxfr[MAG]*gA[1],
		   gB[2] - pxfr[MAG]*gA[2]);
	vec[ri] = pxfr[SHEAR];
	ri += 1;
      }
    }
    ellNmPseudoInverse(ninv, nmat1);
    ellNmMultiply(nans, ninv, nvec);
    ans = (double *)(nans->data);
    hst[0] = ans[0];
    hst[1] = ans[1];
    hst[2] = ans[2];
  }

  /* ------ find Tx, Ty, Tz per slice */
  for (z=0; z<sz; z++) {
    hst = (double *)(nhst->data) + 0 + 9*z;
    ri = 0;
    for (A=1; A<ninLen; A++) {
      for (B=1; B<ninLen; B++) {
	if (A == B) continue;
	pxfr = (double *)(npxfr->data) + 0 + 5*(z + sz*(A + ninLen*B));
	gA = grad + 0 + 3*(A-1);
	gB = grad + 0 + 3*(B-1);
	ELL_3V_SET(mat1 + 3*ri,
		   gB[0] - pxfr[MAG]*gA[0],
		   gB[1] - pxfr[MAG]*gA[1],
		   gB[2] - pxfr[MAG]*gA[2]);
	vec[ri] = pxfr[TRAN];
	ri += 1;
      }
    }
    ellNmPseudoInverse(ninv, nmat1);
    ellNmMultiply(nans, ninv, nvec);
    ans = (double *)(nans->data);
    hst[6] = ans[0];
    hst[7] = ans[1];
    hst[8] = ans[2];
  }

  airMopOkay(mop);
  return 0;
}

int
_tenEpiRegSmoothHST(Nrrd *nhst, float bwP) {
  char me[]="_tenEpiRegSmoothHST", err[AIR_STRLEN_MED];
  NrrdResampleInfo *rinfo;
  airArray *mop;
  int sz, sp;

  if (nhst->axis[1].size > 1) {
    sp = nhst->axis[0].size;
    sz = nhst->axis[1].size;
    mop = airMopNew();
    rinfo = nrrdResampleInfoNew();
    airMopAdd(mop, rinfo, (airMopper)nrrdResampleInfoNix, airMopAlways);
    rinfo->kernel[1] = nrrdKernelGaussian;
    rinfo->parm[1][0] = bwP;
    rinfo->parm[1][1] = 3.0; /* how many stnd devs do we cut-off at */
    ELL_2V_SET(rinfo->samples, sp, sz);
    ELL_2V_SET(rinfo->min, 0, 0);
    ELL_2V_SET(rinfo->max, sp-1, sz-1);
    rinfo->boundary = nrrdBoundaryBleed;
    rinfo->type = nrrdTypeUnknown;
    rinfo->renormalize = AIR_TRUE;
    rinfo->clamp = AIR_TRUE;
    
    nhst->axis[1].min = 0;
    nhst->axis[1].max = sz-1;
    if (nrrdSpatialResample(nhst, nhst, rinfo)) {
      sprintf(err, "%s: trouble blurring nhst", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    airMopOkay(mop);
  }
  return 0;
}

int
_tenEpiRegGetHST(double *hhP, double *ssP, double *ttP,
		 int ref, int ni, int zi,
		 Nrrd *npxfr, Nrrd *nhst, Nrrd *ngrad) {
  double *xfr, *hst, *grad, zero[3]={0,0,0};
  int sz, ninLen;

  /* these could also have been passed to us, but we can also discover them */
  sz = npxfr->axis[1].size;
  ninLen = npxfr->axis[2].size;

  if (ref) {
    /* we register against a specific DWI */
    xfr = (double*)(npxfr->data) + 0 + 5*(zi + sz*(ref + ninLen*ni));
    *hhP = xfr[2];
    *ssP = xfr[3];
    *ttP = xfr[4];
  } else {
    /* we use the estimated S,M,T vectors to determine distortion
       as a function of gradient direction, and then invert this */
    hst = (double*)(nhst->data) + 0 + 9*zi;
    if (ni) {
      grad = (double*)(ngrad->data) + 0 + 3*(ni-1);
    } else {
      grad = zero;
    }
    *hhP = ELL_3V_DOT(grad, hst + 0*3);
    *ssP = 1 + ELL_3V_DOT(grad, hst + 1*3);
    *ttP = ELL_3V_DOT(grad, hst + 2*3);
  }
  return 0;
}

int
_tenEpiRegDoit1(Nrrd **ndone, Nrrd *npxfr, Nrrd *nhst, Nrrd *ngrad,
		Nrrd **nin, int ninLen,
		int ref, NrrdKernel *kern, double *kparm,
		int verb) {
  char me[]="_tenEpiRegDoit1", err[AIR_STRLEN_MED];
  gageContext *gtx;
  gagePerVolume *pvl=NULL;
  airArray *mop;
  int E, ni, xi, yi, zi, sx, sy, sz;
  gage_t *val;
  double cx, cy, hh, ss, tt;

  mop = airMopNew();
  gtx = gageContextNew();
  airMopAdd(mop, gtx, (airMopper)gageContextNix, airMopAlways);

  /* gageSet(gtx, gageParmRenormalize, AIR_TRUE); */
  gageSet(gtx, gageParmCheckIntegrals, AIR_TRUE);
  gageSet(gtx, gageParmRequireAllSpacings, AIR_FALSE);
  gageSet(gtx, gageParmRequireEqualCenters, AIR_FALSE);
  gageSet(gtx, gageParmDefaultCenter, nrrdCenterCell);

  if (verb) {
    fprintf(stderr, "%s:\n            ", me); fflush(stderr);
  }
  sx = nin[0]->axis[0].size;
  sy = nin[0]->axis[1].size;
  sz = nin[0]->axis[2].size;
  cx = sx/2.0;
  cy = sy/2.0;
  for (ni=0; ni<ninLen; ni++) {
    if (verb) {
      fprintf(stderr, "% 2d ", ni); fflush(stderr);
    }
    if (nrrdCopy(ndone[ni], nin[ni])) {
      sprintf(err, "%s: couldn't do initial copy of nin[%d]", me, ni);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    if (!ni) {
      /* we've just copied the B=0 image, and that's all we're supposed 
	 to do, so we're done.  All successive dwi volumes will have to
	 deal with gage */
      continue;
    }
    E = 0;
    if (pvl) {
      if (!E) E |= gagePerVolumeDetach(gtx, pvl);
      if (!E) gagePerVolumeNix(pvl);
    }
    if (!E) E |= !(pvl = gagePerVolumeNew(gtx, nin[ni], gageKindScl));
    if (!E) E |= gageQuerySet(gtx, pvl, 1 << gageSclValue);
    /* this next one is needlessly repeated */
    if (!E) E |= gageKernelSet(gtx, gageKernel00, kern, kparm);
    if (!E) E |= gagePerVolumeAttach(gtx, pvl);
    if (!E) E |= gageUpdate(gtx);
    if (E) {
      sprintf(err, "%s: trouble with gage on nin[%d]", me, ni);
      biffMove(TEN, err, GAGE); airMopError(mop); return 1;
    }
    val = gageAnswerPointer(gtx, pvl, gageSclValue);
    if (gageProbe(gtx, 0, 0, 0)) {
      sprintf(err, "%s: gage failed initial probe: %s", me, gageErrStr);
      biffAdd(TEN, err); airMopError(mop); return 1;
    }
    for (zi=0; zi<sz; zi++) {
      _tenEpiRegGetHST(&hh, &ss, &tt, ref, ni, zi, npxfr, nhst, ngrad);
      tt += (1-ss)*cy - hh*cx;
      for (yi=0; yi<sy; yi++) {
	for (xi=0; xi<sx; xi++) {
	  gageProbe(gtx, xi, hh*xi + ss*yi + tt, zi);
	  nrrdDInsert[ndone[ni]->type](ndone[ni]->data, xi + sx*(yi + sy*zi),
				       nrrdDClamp[ndone[ni]->type](ss*(*val)));
	}
      }
    }
  }
  if (verb) {
    fprintf(stderr, "done\n");
  }

  airMopOkay(mop);
  return 0;
}

/*
** _tenEpiRegSliceWarp
**
** Apply [hh,ss,tt] transform to nin, putting results in nout, but with
** some trickiness:
** - nwght and nidx are already allocated to the the weights (type float)
**   and indices for resampling nin with "kern" and "kparm"
** - nout is already allocated to the correct size and type
** - nin is type float, but output must be type nout->type
** - nin is been transposed to have the resampled axis fastest in memory,
**   but nout output will not be transposed
*/
int
_tenEpiRegSliceWarp(Nrrd *nout, Nrrd *nin, Nrrd *nwght, Nrrd *nidx, 
		    NrrdKernel *kern, double *kparm,
		    double hh, double ss, double tt, double cx, double cy) {
  float *wght, *in, pp, pf, tmp;
  int *idx, supp, sx, sy, xi, yi, pb, pi;
  double (*ins)(void *, size_t, double), (*clamp)(double);

  
  sy = nin->axis[0].size;
  sx = nin->axis[1].size;
  supp = kern->support(kparm);
  ins = nrrdDInsert[nout->type];
  clamp = nrrdDClamp[nout->type];

  in = (float*)(nin->data);
  for (xi=0; xi<sx; xi++) {
    idx = (int*)(nidx->data);
    wght = (float*)(nwght->data);
    for (yi=0; yi<sy; yi++) {
      pp = hh*(xi - cx) + ss*(yi - cy) + tt + cy;
      pb = floor(pp);
      pf = pp - pb;
      for (pi=-(supp-1); pi<=supp; pi++) {
	idx[pi+(supp-1)] = AIR_IN_CL(0, pb + pi, sy-1) ? pb + pi : -1;
	wght[pi+(supp-1)] = pi - pf;
      }
      idx += 2*supp;
      wght += 2*supp;
    }
    idx = (int*)(nidx->data);
    wght = (float*)(nwght->data);
    kern->evalN_f(wght, wght, 2*supp*sy, kparm);
    for (yi=0; yi<sy; yi++) {
      tmp = 0;
      for (pi=0; pi<2*supp; pi++) {
	tmp += idx[pi] >= 0 ? in[idx[pi]]*wght[pi] : 0;
      }
      ins(nout->data, xi + sx*yi, clamp(ss*tmp));
      idx += 2*supp;
      wght += 2*supp;
    }
    in += sy;
  }

  return 0;
}

/*
** _tenEpiRegDoit2()
**
** an optimized version of _tenEpiRegDoit1(), which is MUCH faster because
** it doesn't use gage, which is appropriate, since the resampling is really
** only happening along one dimension.
*/
int
_tenEpiRegDoit2(Nrrd **ndone, Nrrd *npxfr, Nrrd *nhst, Nrrd *ngrad,
		Nrrd **nin, int ninLen,
		int ref, NrrdKernel *kern, double *kparm,
		int verb) {
  char me[]="_tenEpiRegDoit2", err[AIR_STRLEN_MED];
  Nrrd *ntmp, *nfin, *nslcA, *nslcB, *nwght, *nidx;
  airArray *mop;
  int sx, sy, sz, ni, zi, supp;
  double hh, ss, tt, cx, cy;

  mop = airMopNew();
  airMopAdd(mop, ntmp=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nfin=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nslcA=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nslcB=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nwght=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nidx=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);

  if (verb) {
    fprintf(stderr, "%s:\n            ", me); fflush(stderr);
  }
  sx = nin[0]->axis[0].size;
  sy = nin[0]->axis[1].size;
  sz = nin[0]->axis[2].size;
  cx = sx/2.0;
  cy = sy/2.0;
  supp = kern->support(kparm);
  if (nrrdMaybeAlloc(nwght, nrrdTypeFloat, 2, 2*supp, sy)
      || nrrdMaybeAlloc(nidx, nrrdTypeInt, 2, 2*supp, sy)) {
    sprintf(err, "%s: trouble allocating buffers", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }
  if (nrrdCopy(ndone[0], nin[0])) {
    sprintf(err, "%s: trouble copying T2 ref image", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }
  for (ni=1; ni<ninLen; ni++) {
    if (verb) {
      fprintf(stderr, "% 2d ", ni); fflush(stderr);
    }
    if (nrrdCopy(ndone[ni], nin[ni])
	|| ((1==ni) && nrrdSlice(nslcB, ndone[ni], 2, 0)) /* slice at 1==ni */
	|| nrrdAxesSwap(ntmp, nin[ni], 0, 1)
	|| nrrdConvert(nfin, ntmp, nrrdTypeFloat)) {
      sprintf(err, "%s: trouble prepping at ni=%d", me, ni);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    for (zi=0; zi<sz; zi++) {
      if (_tenEpiRegGetHST(&hh, &ss, &tt, ref, ni, zi, npxfr, nhst, ngrad)
	  || nrrdSlice(nslcA, nfin, 2, zi)
	  || _tenEpiRegSliceWarp(nslcB, nslcA, nwght, nidx, kern, kparm,
				 hh, ss, tt, cx, cy)
	  || nrrdSplice(ndone[ni], ndone[ni], nslcB, 2, zi)) {
	sprintf(err, "%s: trouble on slice %d if ni=%d", me, zi, ni);
	/* because the _tenEpiReg calls above don't use biff */
	biffMove(TEN, err, NRRD); airMopError(mop); return 1;
      }
    }
  }
  if (verb) {
    fprintf(stderr, "done\n");
  }

  airMopOkay(mop);
  return 0;
}

int
tenEpiRegister(Nrrd *nout, Nrrd **nin, int ninLen, Nrrd *_ngrad,
	       int reference,
	       float bwX, float bwY, float bwP,
	       float B0thr, float DWthr, int doCC,
	       NrrdKernel *kern, double *kparm,
	       int progress, int verbose) {
  char me[]="tenEpiRegister", err[AIR_STRLEN_MED];
  airArray *mop;
  Nrrd **nbuffA, **nbuffB, *npxfr, *nprog, *nhst, *ngrad;
  int i, hack1, hack2;

  hack1 = nrrdStateAlwaysSetContent;
  hack2 = nrrdStateDisableContent;
  nrrdStateAlwaysSetContent = AIR_FALSE;
  nrrdStateDisableContent = AIR_TRUE;

  mop = airMopNew();
  if (_tenEpiRegCheck(nout, nin, ninLen, _ngrad, reference,
		      bwX, bwY, B0thr, DWthr,
		      kern, kparm)) {
    sprintf(err, "%s: trouble with input", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }

  nbuffA = (Nrrd **)calloc(ninLen, sizeof(Nrrd*));
  nbuffB = (Nrrd **)calloc(ninLen, sizeof(Nrrd*));
  if (!( nbuffA && nbuffB )) {
    sprintf(err, "%s: couldn't allocate tmp nrrd pointer arrays", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, nbuffA, airFree, airMopAlways);
  airMopAdd(mop, nbuffB, airFree, airMopAlways);
  for (i=0; i<ninLen; i++) {
    airMopAdd(mop, nbuffA[i] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
    airMopAdd(mop, nbuffB[i] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  }
  airMopAdd(mop, npxfr = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nprog = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nhst = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ngrad = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  if (tenGradNormalize(ngrad, _ngrad)) {
    sprintf(err, "%s: trouble normalizing/converting gradients", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }

  /* ------ blur */
  if (_tenEpiRegBlur(nbuffA, nin, ninLen, bwX, bwY, verbose)) {
    sprintf(err, "%s: trouble %s", me, (bwX || bwY) ? "blurring" : "copying");
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (progress) {
    if (nrrdJoin(nprog, nbuffA, ninLen, 0, AIR_TRUE)
	|| nrrdSave("blur.nrrd", nprog, NULL)) {
      sprintf(err, "%s: trouble saving intermediate: blurred", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    fprintf(stderr, "%s: saved blur.nrrd\n", me);
  }

  /* ------ threshold */
  if (_tenEpiRegThreshold(nbuffB, nbuffA, ninLen, 
			  B0thr, DWthr, verbose)) {
    sprintf(err, "%s: trouble thresholding", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (progress) {
    if (nrrdJoin(nprog, nbuffB, ninLen, 0, AIR_TRUE)
	|| nrrdSave("thresh.nrrd", nprog, NULL)) {
      sprintf(err, "%s: trouble saving intermediate: tresholded", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    fprintf(stderr, "%s: saved thresh.nrrd\n", me);
  }

  /* ------ connected components */
  if (doCC) {
    if (_tenEpiRegCC(nbuffB, ninLen, 1, verbose)) {
      sprintf(err, "%s: trouble doing connected components", me);
      biffAdd(TEN, err); airMopError(mop); return 1;
    }
    if (progress) {
      if (nrrdJoin(nprog, nbuffB, ninLen, 0, AIR_TRUE)
	  || nrrdSave("ccs.nrrd", nprog, NULL)) {
	sprintf(err, "%s: trouble saving intermediate: "
		"connected components ", me);
	biffMove(TEN, err, NRRD); airMopError(mop); return 1;
      }
      fprintf(stderr, "%s: saved ccs.nrrd\n", me);
    }
  }

  /* ------ moments */
  if (_tenEpiRegMoments(nbuffA, nbuffB, ninLen, verbose)) {
    sprintf(err, "%s: trouble finding moments", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (progress) {
    if (nrrdJoin(nprog, nbuffA, ninLen, 0, AIR_TRUE)
	|| nrrdSave("mom.nrrd", nprog, NULL)) {
      sprintf(err, "%s: trouble saving intermediate: moments", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    fprintf(stderr, "%s: saved mom.nrrd\n", me);
  }

  /* ------ transforms */
  if (_tenEpiRegPairXforms(npxfr, nbuffA, ninLen)) {
    sprintf(err, "%s: trouble calculating transforms", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (progress) {
    if (nrrdSave("pxfr.nrrd", npxfr, NULL)) {
      sprintf(err, "%s: trouble saving intermediate: pair-wise xforms", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    fprintf(stderr, "%s: saved pxfr.nrrd\n", me);
  }

  /* ------ HST estimation */
  if (_tenEpiRegEstimHST(nhst, npxfr, ninLen, ngrad)) {
    sprintf(err, "%s: trouble estimating HST", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (progress) {
    if (nrrdSave("hst.nrrd", nhst, NULL)) {
      sprintf(err, "%s: trouble saving intermediate: HST estimates", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    fprintf(stderr, "%s: saved hst.nrrd\n", me);
  }

  /* ------ HST smoothing/fitting */
  if (_tenEpiRegSmoothHST(nhst, bwP)) {
    sprintf(err, "%s: trouble smoothing/fitting HST", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (progress) {
    if (nrrdSave("smhst.nrrd", nhst, NULL)) {
      sprintf(err, "%s: trouble saving intermediate: smoothed HST", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    fprintf(stderr, "%s: saved smhst.nrrd\n", me);
  }

  /* ------ doit */
  if (_tenEpiRegDoit2(nbuffB, npxfr, nhst, ngrad, nin, ninLen,
		      reference, kern, kparm, verbose)) {
    sprintf(err, "%s: trouble performing final registration", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  
  if (verbose) {
    fprintf(stderr, "%s: creating final output ... ", me); fflush(stderr);
  }
  if (nrrdJoin(nout, nbuffB, ninLen, 0, AIR_TRUE)) {
    sprintf(err, "%s: trouble creating final output", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }
  if (verbose) {
    fprintf(stderr, "done\n");
  }
  
  airMopOkay(mop);
  nrrdStateAlwaysSetContent = hack1;
  nrrdStateDisableContent = hack2;
  return 0;
}
