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


#include <air.h>
#include <hest.h>
#include <nrrd.h>
#include <gage.h>

int
qbertParseNrrd(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[] = "qbertParseNrrd", *nerr;
  Nrrd **nrrdP;
  airArray *mop;
  
  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  nrrdP = ptr;
  mop = airMopInit();
  *nrrdP = nrrdNew();
  airMopAdd(mop, *nrrdP, (airMopper)nrrdNuke, airMopOnError);
  if (nrrdLoad(*nrrdP, str)) {
    airMopAdd(mop, nerr = biffGetDone(NRRD), airFree, airMopOnError);
    strncpy(err, nerr, AIR_STRLEN_HUGE-1);
    airMopError(mop);
    return 1;
  }
  if (3 != (*nrrdP)->dim) {
    sprintf(err, "%s: need a 3-D nrrd (not %d)", me, (*nrrdP)->dim);
    airMopError(mop);
    return 1;
  }
  airMopOkay(mop);
  return 0;
}

hestCB qbertNrrdHestCB = {
  sizeof(Nrrd *),
  "nrrd",
  qbertParseNrrd,
  (airMopper)nrrdNuke
}; 

void
usage(char *me) {
  /*               0    1        2            3            4             5     (6) */
  fprintf(stderr, 
	  "usage: %s <nrrdIn> <border> <sX>x<sY>x<sZ> <pv>/<pg>/<ph> <baseName>\n", me);
  fprintf(stderr, "        nrrdIn: scalar nrrd volume to process.  Does not need to be 8-bit;\n");
  fprintf(stderr, "                in fact, results are cleaner and have less quantization noise\n");
  fprintf(stderr, "                if the input volume is 16-bit or float.\n");
  fprintf(stderr, "        border: number of samples on boundary of output volumes which will be\n");
  fprintf(stderr, "                all zeroes.\n");
  fprintf(stderr, "    <baseName>: base of output filenames: output VGH volume will be\n");
  fprintf(stderr, "                \"<baseName>-vgh.nrrd\", and output QN volume will be\n");
  fprintf(stderr, "                \"<baseName>-16qna1pb.nrrd\"\n");
  exit(1);
}

#define HIST_SIZE 1024     /* histogram size for v, g, and h */
#define PERC_IGNORE 0.005  /* _percentage_ of v, g, or h to ignore */

char info[]="Calculate volumes friendly to monkeys.  Each voxel in the "
	     "output volume contains 8-bit quantized values of the data "
	     "value, gradient magnitude, and 2nd directional derivative.";

int
main(int argc, char *argv[]) {
  unsigned char *vgh;
  char *me, *outS, cmt[512], *herr;
  Nrrd *nin, *npad, *npad2, *nrsmp, *nvghF, *nvgh;
  NrrdResampleInfo *rsmpInfo;
  int i, j, k, vi, gi, hi, n, ignore,
    E, need, sz[3], border, padMin[3], padMax[3],
    needRsmp, needPad,
    vhist[HIST_SIZE], ghist[HIST_SIZE], hhist[HIST_SIZE];
  float perc[3], *vghF, v, g, h, minv, maxv, ming, maxg, minh, maxh;
  double t0, t1;
  hestParm *hparm;
  hestOpt *opt = NULL;
  gageSclContext *ctx;
  double kparm[3];

  me = argv[0];
  hparm = hestParmNew();
  hparm->elideSingleOtherType = AIR_TRUE;

  hestOptAdd(&opt, "i", "nin", airTypeOther, 1, 1, &nin, NULL,
	     "input volume, in nrrd format",
	     NULL, NULL, &qbertNrrdHestCB);
  hestOptAdd(&opt, "d", "sx sy sz", airTypeInt, 3, 3, sz, NULL,
	     "dimensions of output volume");
  hestOptAdd(&opt, "p", "pv pg ph", airTypeFloat, 3, 3, perc, "0 2 4",
	     "percentiles along value, gradient, and 2nd derivative "
	     "axes to EXCLUDE in the quantized 8-bit ranges. "
	     "\"0 0 0\" will fit ALL values and derivatives into 8-bits, "
	     "with no clamping whatsoever. \"0 2 4\" will fit all the "
	     "values, but will clamp the extreme 2 percentiles in the "
	     "gradient magnitude, and 4 percentiles in the 2nd derivative. "
	     "Noisier datasets will require higher derivative exclusion "
	     "values so as to best use the 8-bits on the significant "
	     "range of derivative values.");
  hestOptAdd(&opt, "b", "border", airTypeInt, 1, 1, &border, "0",
	     "number of samples on boundary of output volumes which "
	     "will be all zeroes");
  hestOptAdd(&opt, "o", "output", airTypeString, 1, 1, &outS, NULL,
	     "output volume in nrrd format");
  if (hestOptCheck(opt, &herr)) { printf("%s\n", herr); exit(1); }

  if (1 == argc) {
    hestInfo(stderr, me, info, hparm);
    hestUsage(stderr, opt, me, hparm);
    hestGlossary(stderr, opt, hparm);
    hparm = hestParmFree(hparm);
    opt = hestOptFree(opt);
    exit(1);
  }
  if (hestParse(opt, argc-1, argv+1, &herr, hparm)) {
    fprintf(stderr, "ERROR: %s\n", herr); free(herr);
    hestUsage(stderr, opt, me, hparm);
    hestGlossary(stderr, opt, hparm);
    hparm = hestParmFree(hparm);
    opt = hestOptFree(opt);
    exit(1);
  }
  printf("output size = %d %d %d\n", sz[0], sz[1], sz[2]);

  /* padding; to get up to (sz[i] - 2*border)
     This padding is only needed if the input volume is smaller along
     any of the axes than the desired output volume (minus the border). */
  for (i=0; i<=2; i++) {
    need = sz[i] - 2*border - nin->axis[i].size;
    printf("%s: sz[%d] = %d -> need = %d --> ", 
	   me, i, nin->axis[i].size, need);
    need = AIR_MAX(0, need);
    printf("%d --> ", need);
    padMin[i] = 0 - (int)floor(need/2.0);
    padMax[i] = nin->axis[i].size - 1 + (int)ceil(need/2.0);
    printf("pad indices: [%d..%d]\n", padMin[i], padMax[i]);
  }
  printf("%s: padding (stage 1) ... ", me); fflush(stdout);
  npad = nrrdNew();
  if (nrrdPad(npad, nin, padMin, padMax, nrrdBoundaryPad, 0.0)) {
    fprintf(stderr, "%s: trouble padding:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  printf("done\n");
  /*
  sprintf(name, "%s-pad0.nrrd", baseStr);
  nrrdSave(name, npad, NULL);
  */

  /* resampling; to get down to (sz[i] - 2*border) */
  rsmpInfo = nrrdResampleInfoNew();
  rsmpInfo->boundary = nrrdBoundaryBleed;
  rsmpInfo->type = nrrdTypeFloat;
  rsmpInfo->renormalize = AIR_TRUE;
  needRsmp = 0;
  for (i=0; i<=2; i++) {
    if (npad->axis[i].size != sz[i] - 2*border) {
      needRsmp = 1;
      rsmpInfo->kernel[i] = nrrdKernelAQuartic;
      rsmpInfo->parm[i][1] = 0.0834;
      rsmpInfo->samples[i] = sz[i] - 2*border;
      rsmpInfo->min[i] = 0;
      rsmpInfo->max[i] = npad->axis[i].size-1;
      printf("%s: resampling axis %d from %d to %d\n", 
	     me, i, npad->axis[i].size, sz[i]);
    }
    else {
      rsmpInfo->kernel[i] = NULL;
    }
  }
  printf("%s: resampling ... ", me); fflush(stdout);
  if (needRsmp) {
    nrsmp = nrrdNew();
    if (nrrdSpatialResample(nrsmp, npad, rsmpInfo)) {
      fprintf(stderr, "%s: trouble resampling:\n%s\n", me, biffGet(NRRD));
      exit(1);
    }
    npad = nrrdNuke(npad);
  }
  else {
    nrsmp = npad;
  }
  printf("done\n");
  rsmpInfo = nrrdResampleInfoNix(rsmpInfo);
  /*
  sprintf(name, "%s-rsmp.nrrd", baseStr);
  nrrdSave(name, nrsmp, NULL);
  */
  
  /* padding; to get up to (sz[i]) */
  for (i=0; i<=2; i++) {
    padMin[i] = -border;
    padMax[i] = sz[i] - border - 1;
  }
  printf("%s: padding (stage 2) ... ", me); fflush(stdout);
  if (nrrdPad(npad=nrrdNew(), nrsmp, padMin, padMax, nrrdBoundaryPad, 0.0)) {
    fprintf(stderr, "%s: trouble padding:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  printf("done\n");
  /*
  sprintf(name, "%s-pad1.nrrd", baseStr);
  nrrdSave(name, nrsmp, NULL);
  */

  /* probing to make triple volume */
  ctx = gageSclContextNew();
  ctx->c.verbose = 1;   /* reset later */
  ctx->c.renormalize = AIR_TRUE;
  ctx->c.checkIntegrals = AIR_TRUE;
  kparm[0] = 1.0;
  kparm[1] = 1.0;
  kparm[2] = 0.0;
  E = 0;
  /* we have to use a slightly blurring kernel for the 2nd derivatives
     to work out (B-spline is kind of rotationally symmetric) */
  if (!E) E |= gageSclKernelSet(ctx, gageKernel00,
				nrrdKernelBCCubic, kparm);
  if (!E) E |= gageSclKernelSet(ctx, gageKernel11,
				nrrdKernelBCCubicD, kparm);
  if (!E) E |= gageSclKernelSet(ctx, gageKernel22,
				nrrdKernelBCCubicDD, kparm);
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(GAGE));
    exit(1);
  }
  needPad = gageSclNeedPadGet(ctx);
  if (nrrdSimplePad(npad2=nrrdNew(), npad, needPad, nrrdBoundaryBleed)) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  if (!E) E |= gageSclVolumeSet(ctx, needPad, npad2);
  if (!E) E |= gageSclQuerySet(ctx,
			       (1 << gageSclValue) | 
			       (1 << gageSclGradMag) |
			       (1 << gageScl2ndDD));
  if (!E) E |= gageSclUpdate(ctx);
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(GAGE));
    exit(1);
  }
  nvghF = nrrdNew();
  if (nrrdAlloc(nvghF, nrrdTypeFloat, 4, 3, sz[0], sz[1], sz[2])) {
    fprintf(stderr, "%s: couldn't create floating point VGH volume:\n%s\n",
	    me, biffGet(NRRD));
    exit(1);
  }
  vghF = nvghF->data;
  npad2->axis[0].spacing = (AIR_EXISTS(npad2->axis[0].spacing)
			    ? npad2->axis[0].spacing : 1.0);
  npad2->axis[1].spacing = (AIR_EXISTS(npad2->axis[1].spacing)
			    ? npad2->axis[1].spacing : 1.0);
  npad2->axis[2].spacing = (AIR_EXISTS(npad2->axis[2].spacing)
			    ? npad2->axis[2].spacing : 1.0);
  printf("%s: probing ... ", me); fflush(stdout);
  ctx->c.verbose = 0;
  t0 = airTime();
  for (k=0; k<=sz[2]-1; k++) {
    for (j=0; j<=sz[1]-1; j++) {
      for (i=0; i<=sz[0]-1; i++) {
	gageSclProbe(ctx, i, j, k);
	vghF[0 + 3*(i + sz[0]*(j + sz[1]*k))] = *(ctx->val);
	vghF[1 + 3*(i + sz[0]*(j + sz[1]*k))] = *(ctx->gmag);
	vghF[2 + 3*(i + sz[0]*(j + sz[1]*k))] = *(ctx->scnd);
	/*
	printf("%d,%d,%d: %g %g %g\n", i, j, k, ans[vo], ans[go], ans[ho]);
	*/
      }
    }
  }
  t1 = airTime();
  npad2 = nrrdNuke(npad2);
  printf("done (probe rate = %g/sec)\n", sz[0]*sz[1]*sz[2]/(t1-t0));
  /*
  sprintf(name, "%s-vghF.nrrd", baseStr);
  nrrdSave(name, nvghF);
  */


  /* make histograms to determine inclusion ranges */
  minv = maxv = vghF[0 + 3*0];
  ming = maxg = vghF[1 + 3*0];
  minh = maxh = vghF[2 + 3*0];
  for (i=0; i<=sz[0]*sz[1]*sz[2]-1; i++) {
    minv = AIR_MIN(minv, vghF[0 + 3*i]);
    maxv = AIR_MAX(maxv, vghF[0 + 3*i]);
    ming = AIR_MIN(ming, vghF[1 + 3*i]);
    maxg = AIR_MAX(maxg, vghF[1 + 3*i]);
    minh = AIR_MIN(minh, vghF[2 + 3*i]);
    maxh = AIR_MAX(maxh, vghF[2 + 3*i]);
  }
  printf("%s: value  range: %g -> %g\n", me, minv, maxv);
  printf("%s:  grad  range: %g -> %g\n", me, ming, maxg);
  printf("%s: 2nd DD range: %g -> %g\n", me, minh, maxh);
  printf("%s: !!!! dictating that min value = 0\n", me);
  minv = 0;
  printf("%s: !!!! dictating that min grad = 0\n", me);
  ming = 0;
  printf("%s: !!!! dictating that min h = -(max h)\n", me);
  if (maxh > -minh) 
    minh = -maxh;
  else
    maxh = -minh;
  printf("%s: value  range: %g -> %g\n", me, minv, maxv);
  printf("%s:  grad  range: %g -> %g\n", me, ming, maxg);
  printf("%s: 2nd DD range: %g -> %g\n", me, minh, maxh);
  memset(vhist, 0, HIST_SIZE*sizeof(int));
  memset(ghist, 0, HIST_SIZE*sizeof(int));
  memset(hhist, 0, HIST_SIZE*sizeof(int));
  for (k=0; k<=sz[2]-1; k++) {
    for (j=0; j<=sz[1]-1; j++) {
      for (i=0; i<=sz[0]-1; i++) {
	v = vghF[0 + 3*(i + sz[0]*(j + sz[1]*k))];
	g = vghF[1 + 3*(i + sz[0]*(j + sz[1]*k))];
	h = vghF[2 + 3*(i + sz[0]*(j + sz[1]*k))];
	AIR_INDEX(minv, v, maxv, HIST_SIZE, vi);
	AIR_INDEX(ming, g, maxg, HIST_SIZE, gi);
	AIR_INDEX(minh, h, maxh, HIST_SIZE, hi);
	vhist[vi]++;
	ghist[gi]++;
	hhist[hi]++;
      }
    }
  }
  
  /* determine inclusion ranges */
  ignore = PERC_IGNORE*sz[0]*sz[1]*sz[2]/100;
  printf("%s: ignoring %d hits for v, g, and h each\n", me, ignore);
  j = perc[0]*sz[0]*sz[1]*sz[2]/100;
  i = HIST_SIZE-1;
  while (j > 0) {
    j -= vhist[i];
    i--;
  }
  maxv = AIR_AFFINE(0, i, HIST_SIZE-1, minv, maxv);
  j = perc[1]*sz[0]*sz[1]*sz[2]/100;
  i = HIST_SIZE-1;
  while (j > 0) {
    j -= ghist[i];
    i--;
  }
  maxg = AIR_AFFINE(0, i, HIST_SIZE-1, ming, maxg);
  j = perc[2]*sz[0]*sz[1]*sz[2]/100;
  i = 0;
  while (j > 0) {
    j -= hhist[i] + hhist[HIST_SIZE-1-i];
    i++;
  }
  minh = AIR_AFFINE(0, i, HIST_SIZE-1, minh, maxh);
  maxh = -minh;
  printf("%s: new inclusion ranges:\n", me);
  printf("%s: value  range: %g -> %g\n", me, minv, maxv);
  printf("%s:  grad  range: %g -> %g\n", me, ming, maxg);
  printf("%s: 2nd DD range: %g -> %g\n", me, minh, maxh);
  
  nvgh = nrrdNew();
  if (nrrdAlloc(nvgh, nrrdTypeUChar, 4, 3, sz[0], sz[1], sz[2])) {
    fprintf(stderr, "%s: couldn't create unsigned char VGH volume:\n%s\n",
	    me, biffGet(NRRD));
    exit(1);
  }
  vgh = nvgh->data;
  printf("%s: putting 2nd deriv in range 1 to 169 (zero at 85)\n", me);
  for (i=0; i<=sz[0]*sz[1]*sz[2]-1; i++) {
    AIR_INDEX(minv, vghF[0 + 3*i], maxv, 254, n);
    /*
    if (n) {
      printf("%d: v: %g -> %d\n", i, vghF[0 + 3*i], n);
      exit(1);
    }
    */
    vgh[0 + 3*i] = AIR_CLAMP(1, n+1, 254);
    AIR_INDEX(ming, vghF[1 + 3*i], maxg, 254, n);
    vgh[1 + 3*i] = AIR_CLAMP(1, n+1, 254);
    AIR_INDEX(minh, vghF[2 + 3*i], maxh, 169, n);
    vgh[2 + 3*i] = AIR_CLAMP(1, n+1, 169);
  }
  airStrdup(nvgh->axis[0].label, "vgh");
  airStrdup(nvgh->axis[1].label, "x");
  airStrdup(nvgh->axis[2].label, "y");
  airStrdup(nvgh->axis[3].label, "z");
  nvgh->axis[1].spacing = npad->axis[0].spacing;
  nvgh->axis[2].spacing = npad->axis[1].spacing;
  nvgh->axis[3].spacing = npad->axis[2].spacing;
  sprintf(cmt, "exclusions (v/g/h): %g/%g/%g", perc[0], perc[1], perc[2]);
  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "minv: %g", minv);  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "maxv: %g", maxv);  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "ming: %g", ming);  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "maxg: %g", maxg);  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "minh: %g", minh);  nrrdCommentAdd(nvgh, cmt);
  sprintf(cmt, "maxh: %g", maxh);  nrrdCommentAdd(nvgh, cmt);
  nrrdSave(outS, nvgh, NULL);

  nrsmp =  nrrdNuke(nrsmp);
  nvgh =  nrrdNuke(nvgh);

  exit(0);
}
