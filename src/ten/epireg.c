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
_tenEpiRegisterCheck(Nrrd *nout, Nrrd **nin, int ninLen, int reference,
		  float bw, float thresh, float soft,
		  NrrdKernel *kern, double *kparm) {
  char me[]="_tenEpiRegisterCheck", err[AIR_STRLEN_MED];
  int ni;

  if (!( nout && nin && kern && kparm )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( ninLen >= 3 )) {
    sprintf(err, "%s: given ninLen (%d) not >= 3", me, ninLen);
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
  if (!( AIR_IN_CL(1, reference, ninLen-1) )) {
    sprintf(err, "%s: reference index %d not in valid range [1,%d]", 
	    me, reference, ninLen-1);
    biffAdd(TEN, err); return 1;
  }
  if (!( AIR_EXISTS(bw) && AIR_EXISTS(thresh) && AIR_EXISTS(soft) )) {
    sprintf(err, "%s: not all bw, thresh, and soft exist", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( bw >= 0 && soft >= soft )) {
    sprintf(err, "%s: bw (%g) and soft (%g) are not both non-negative",
	    me, bw, soft);
    biffAdd(TEN, err); return 1;
  }
  return 0;
}

/*
** this assumes that all nblur[i] are valid nrrds, and does nothing
** to manage them
*/
int
_tenEpiRegisterBlur(Nrrd **nblur, Nrrd **nin, int ninLen, float bw, int verb) {
  char me[]="_tenEpiRegisterBlur", err[AIR_STRLEN_MED];
  NrrdResampleInfo *rinfo;
  airArray *mop;
  int ni, sx, sy, sz;
  double min, max;

  if (!bw) {
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
  ELL_3V_SET(rinfo->kernel, NULL, nrrdKernelGaussian, NULL);
  ELL_3V_SET(rinfo->samples, sx, sy, sz);
  rinfo->parm[1][0] = bw;
  rinfo->parm[1][1] = 3.0; /* how many stnd devs do we cut-off at */
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
    min = nin[ni]->axis[1].min;
    max = nin[ni]->axis[1].max;
    nin[ni]->axis[1].min = 0;
    nin[ni]->axis[1].max = sy-1;
    if (nrrdSpatialResample(nblur[ni], nin[ni], rinfo)) {
      sprintf(err, "%s: trouble blurring nin[%d]", me, ni);
      biffMove(TEN, err, NRRD); return 1;
    }
    nin[ni]->axis[1].min = min;
    nin[ni]->axis[1].max = max;
  }
  if (verb) {
    fprintf(stderr, "done\n");
  }
  airMopOkay(mop);
  return 0;
}

int
_tenEpiRegisterThreshold(Nrrd **nthresh, Nrrd **nblur, int ninLen,
			 float b0thr, float dwithr, float soft, int verb) {
  char me[]="_tenEpiRegisterThreshold", err[AIR_STRLEN_MED];
  airArray *mop;
  int I, sx, sy, sz, ni;
  float val, *thr;
  
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
    if (nrrdMaybeAlloc(nthresh[ni], nrrdTypeFloat, 3, sx, sy, sz)) {
      sprintf(err, "%s: trouble allocating threshold %d", me, ni);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    thr = (float*)(nthresh[ni]->data);
    for (I=0; I<sx*sy*sz; I++) {
      val = nrrdFLookup[nblur[ni]->type](nblur[ni]->data, I);
      val -= ni ? dwithr : b0thr;
      val = airErf(val/(soft + 0.00000001));
      thr[I] = AIR_AFFINE(-1.0, val, 1.0, 0.0, 1.0);
    }
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
** _tenEpiRegisterMoments()
**
** the moments are stored in (of course) a nrrd, one scanline per slice,
** with each scanline containing:
**
**       0       1       2       3       4
**   mean(x)  mean(y)  M_02    M_11    M_20
*/
int
_tenEpiRegisterMoments(Nrrd **nmom, Nrrd **nthresh, int ninLen, int verb) {
  char me[]="_tenEpiRegisterMoments", err[AIR_STRLEN_MED];
  int sx, sy, sz, xi, yi, zi, ni;
  double N, mx, my, x, y, M02, M11, M20, *mom;
  float *thr, val;

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
    thr = (float *)(nthresh[ni]->data);
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
		"threshold too high?", me, ni);
	biffAdd(TEN, err); return 1;
      }
      mx /= N;
      my /= N;
      /* ------ find M02, M11, M20 */
      M02 = M11 = M20 = 0.0;
      for (yi=0; yi<sy; yi++) {
	for (xi=0; xi<sx; xi++) {
	  val = thr[xi + sx*yi];
	  x = xi - mx;
	  y = yi - my;
	  M02 += y*y*val;
	  M11 += x*y*val;
	  M20 += x*x*val;
	}
      }
      M02 /= N;
      M11 /= N;
      M20 /= N;
      /* ------ set output */
      mom[0] = mx;
      mom[1] = my;
      mom[2] = M02;
      mom[3] = M11;
      mom[4] = M20;
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
** _tenEpiRegisterPairXforms
**
** uses moment information to compute all pair-wise transforms, which are
** stored in the 3 x ninLen x ninLen x sizeZ output.  If xfr = npxfr->data,
** xfr[0 + 3*(zi + sz*(A + ninLen*B))] is shear,
** xfr[1 +              "            ] is scale, and 
** xfr[2 +              "            ] is translate in the transform
** that maps slice zi from volume A to volume B.
*/
int
_tenEpiRegisterPairXforms(Nrrd *npxfr, Nrrd **nmom, int ninLen) {
  char me[]="_tenEpiRegisterPairXforms", err[AIR_STRLEN_MED];
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

int
_tenEpiRegisterDoit(Nrrd **ndone, Nrrd *npxfr, Nrrd **nin, int ninLen,
		    int fixb0, int ref, NrrdKernel *kern, double *kparm,
		    int verb) {
  char me[]="_tenEpiRegisterDoit", err[AIR_STRLEN_MED];
  gageContext *gtx;
  gagePerVolume *pvl=NULL;
  airArray *mop;
  int E, ni, xi, yi, zi, sx, sy, sz;
  gage_t *val;
  double *xfr, mx, my, hh, ss, tt;

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
  for (ni=0; ni<ninLen; ni++) {
    if (verb) {
      fprintf(stderr, "% 2d ", ni); fflush(stderr);
    }
    if (nrrdCopy(ndone[ni], nin[ni])) {
      sprintf(err, "%s: couldn't do initial copy of nin[%d]", me, ni);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    if (!ni && fixb0) {
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
      xfr = (double*)(npxfr->data) + 5*(zi + sz*(ref + ninLen*ni));
      mx = xfr[0];
      my = xfr[1];
      hh = xfr[2];
      ss = xfr[3];
      tt = xfr[4];
      tt += (1-ss)*my - hh*mx;
      for (yi=0; yi<sy; yi++) {
	for (xi=0; xi<sx; xi++) {
	  gageProbe(gtx, xi, hh*xi + ss*yi + tt, zi);
	  nrrdDInsert[ndone[ni]->type](ndone[ni]->data, xi + sx*(yi + sy*zi),
				       nrrdDClamp[ndone[ni]->type](*val));
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

int
tenEpiRegister(Nrrd *nout, Nrrd **nin, int ninLen, int reference,
	       float bw, float thresh, float soft,
	       NrrdKernel *kern, double *kparm,
	       int progress, int verbose) {
  char me[]="tenEpiRegister", err[AIR_STRLEN_MED];
  airArray *mop;
  Nrrd **nbuffA, **nbuffB, *npxfr, *nprog;
  int i;

  mop = airMopNew();
  if (_tenEpiRegisterCheck(nout, nin, ninLen, reference,
			bw, thresh, soft,
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
    nbuffA[i] = nrrdNew();
    airMopAdd(mop, nbuffA[i], (airMopper)nrrdNuke, airMopAlways);
    nbuffB[i] = nrrdNew();
    airMopAdd(mop, nbuffB[i], (airMopper)nrrdNuke, airMopAlways);
  }
  npxfr = nrrdNew();
  airMopAdd(mop, npxfr, (airMopper)nrrdNuke, airMopAlways);
  nprog = nrrdNew();
  airMopAdd(mop, nprog, (airMopper)nrrdNuke, airMopAlways);

  /* ------ blur */
  if (_tenEpiRegisterBlur(nbuffA, nin, ninLen, bw, verbose)) {
    sprintf(err, "%s: trouble %s", me, bw ? "blurring" : "copying");
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
  if (_tenEpiRegisterThreshold(nbuffB, nbuffA, ninLen, 
			       0, thresh, soft, verbose)) {
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

  /* ------ moments */
  if (_tenEpiRegisterMoments(nbuffA, nbuffB, ninLen, verbose)) {
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
  if (_tenEpiRegisterPairXforms(npxfr, nbuffA, ninLen)) {
    sprintf(err, "%s: trouble calculating transforms", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (progress) {
    if (nrrdSave("pxfr.nrrd", npxfr, NULL)) {
      sprintf(err, "%s: trouble saving intermediate: pair-wise xforms", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    fprintf(stderr, "%s: save pxfr.nrrd\n", me);
  }

  /* ------ doit */
  /* filter/regularize transforms? */
  if (_tenEpiRegisterDoit(nbuffB, npxfr, nin, ninLen,
			  AIR_TRUE, reference, kern, kparm, verbose)) {
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
  return 0;
}
