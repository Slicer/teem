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


#include "nrrd.h"
#include "private.h"

/*
  (this was written before airMopSub ... )
learned: if you start using airMop stuff, and you register a free, but
then you free the memory yourself, YOU HAVE GOT TO register a NULL in
place of the original free.  The next malloc may end up at the same
address as what you just freed, and if you want this memory to NOT be
mopped up, then you'll be confused with the original registered free
goes into effect and mops it up for you, even though YOU NEVER
REGISTERED a free for the second malloc.  If you want simple stupid
tools, you have to treat them accordingly (be extremely careful with
fire).  */

int
nrrdSimpleResample(Nrrd *nout, Nrrd *nin,
		   nrrdKernel *kernel, double *param,
		   int *samples, double *scalings) {
  char me[]="nrrdSimpleResample", err[AIR_STRLEN_MED];
  nrrdResampleInfo *info;
  int d, p, np, center;

  if (!(nout && nin && kernel && (samples || scalings))) {
    sprintf(err, "%s: not NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(info = nrrdResampleInfoNew())) {
    sprintf(err, "%s: can't allocate resample info struct", me);
    biffAdd(NRRD, err); return 1;
  }

  np = kernel->numParam;
  for (d=0; d<=nin->dim-1; d++) {
    info->kernel[d] = kernel;
    if (samples) {
      info->samples[d] = samples[d];
    }
    else {
      center = _nrrdCenter(nin->axis[d].center);
      if (nrrdCenterCell == center)
	info->samples[d] = nin->axis[d].size*scalings[d];
      else
	info->samples[d] = (nin->axis[d].size - 1)*scalings[d] + 1;
    }
    for (p=0; p<=np-1; p++)
      info->param[d][p] = param[p];
    /* set the min/max for this axis if not already set to something */
    if (!( AIR_EXISTS(nin->axis[d].min) && AIR_EXISTS(nin->axis[d].max) ))
      nrrdAxisSetMinMax(nin, d);
    info->min[d] = nin->axis[d].min;
    info->max[d] = nin->axis[d].max;
  }
  /* we go with the defaults (enstated by _nrrdResampleInfoInit())
     for all the remaining fields */

  if (nrrdSpatialResample(nout, nin, info)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }

  info = nrrdResampleInfoNix(info);
  return 0;
}

/*
** _nrrdResampleCheckInfo()
**
** checks validity of given nrrdResampleInfo *info: 
** - all required parameters exist
** - both min[d] and max[d] for all axes d
*/
int
_nrrdResampleCheckInfo(Nrrd *nin, nrrdResampleInfo *info) {
  char me[] = "_nrrdResampleCheckInfo", err[AIR_STRLEN_MED];
  nrrdKernel *k;
  int center, p, d, np, minsmp;

  if (nrrdTypeBlock == nin->type || nrrdTypeBlock == info->type) {
    sprintf(err, "%s: can't resample to or from type %s", me,
	    airEnumStr(nrrdType, nrrdTypeBlock));
  }
  if (nrrdBoundaryUnknown == info->boundary) {
    sprintf(err, "%s: didn't set boundary behavior\n", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdBoundaryPad == info->boundary && !AIR_EXISTS(info->padValue)) {
    sprintf(err, "%s: asked for boundary padding, but no pad value set\n", me);
    biffAdd(NRRD, err); return 1;
  }
  for (d=0; d<=nin->dim-1; d++) {
    k = info->kernel[d];
    /* we only care about the axes being resampled */
    if (!k)
      continue;
    if (!(info->samples[d] > 0)) {
      sprintf(err, "%s: axis %d # samples (%d) invalid", 
	      me, d, info->samples[d]);
      biffAdd(NRRD, err); return 1;
    }
    if (!( AIR_EXISTS(nin->axis[d].min) && AIR_EXISTS(nin->axis[d].max) )) {
      sprintf(err, "%s: input nrrd's axis %d min,max have not both been set",
	      me, d);
      biffAdd(NRRD, err); return 1;
    }
    if (!( AIR_EXISTS(info->min[d]) && AIR_EXISTS(info->max[d]) )) {
      sprintf(err, "%s: info's axis %d min,max not both set", me, d);
      biffAdd(NRRD, err); return 1;
    }
    np = k->numParam;
    for (p=0; p<=np-1; p++) {
      if (!AIR_EXISTS(info->param[d][p])) {
	sprintf(err, "%s: didn't set parameter %d (of %d) for axis %d\n",
		me, p, np, d);
	biffAdd(NRRD, err); return 1;
      }
    }
    center = _nrrdCenter(nin->axis[d].center);
    minsmp = nrrdCenterCell == center ? 1 : 2;
    if (!( nin->axis[d].size >= minsmp && info->samples[d] >= minsmp )) {
      sprintf(err, "%s: axis %d # input samples (%d) or output samples (%d) "
	      " invalid for %s centering",
	      me, d, nin->axis[d].size, info->samples[d],
	      airEnumStr(nrrdCenter, center));
      biffAdd(NRRD, err); return 1;
    }
  }
  return 0;
}

/*
** _nrrdResampleComputePermute()
**
** figures out information related to how the axes in a nrrd are
** permuted during resampling: permute, topRax, botRax, passes, ax[][], sz[][]
*/
void
_nrrdResampleComputePermute(int permute[], 
			    int ax[NRRD_DIM_MAX][NRRD_DIM_MAX], 
			    int sz[NRRD_DIM_MAX][NRRD_DIM_MAX], 
			    int *topRax, int *botRax, int *passes,
			    Nrrd *nin,
			    nrrdResampleInfo *info) {
  /* char me[]="_nrrdResampleComputePermute"; */
  int a, p, d, dim;
  
  dim = nin->dim;
  
  /* what are the first (top) and last (bottom) axes being resampled? */
  *topRax = *botRax = -1;
  for (d=0; d<=dim-1; d++) {
    if (info->kernel[d]) {
      if (*topRax < 0) {
	*topRax = d;
      }
      *botRax = d;
    }
  }

  /* figure out total number of passes needed, and construct the
     permute[] array.  permute[i] = j means that the axis in position
     i of the old array will be in position j of the new one
     (permute[] answers "where do I put this", not "what do I put here").
  */
  *passes = a = 0;
  for (d=0; d<=dim-1; d++) {
    if (info->kernel[d]) {
      do {
	a = AIR_MOD(a+1, dim);
      } while (!info->kernel[a]);
      permute[a] = d;
      *passes += 1;
    }
    else {
      permute[d] = d;
      a += a == d;
    }
  }
  permute[dim] = dim;
  if (!*passes) {
    /* none of the kernels was non-NULL */
    return;
  }
  
  /*
  printf("%s: permute:\n", me);
  for (d=0; d<=dim-1; d++) {
    printf("   permute[%d] = %d\n", d, permute[d]);
  }
  */

  /* create array of how the axes will be arranged in each pass ("ax"), 
     and create array of how big each axes is in each pass ("sz").
     The input to pass i will have axis layout described in ax[i] and
     axis sizes described in sz[i] */
  for (d=0; d<=dim-1; d++) {
    ax[0][d] = d;
    sz[0][d] = nin->axis[d].size;
  }
  for (p=0; p<=*passes-1; p++) {
    for (d=0; d<=dim-1; d++) {
      ax[p+1][permute[d]] = ax[p][d];
      if (d == *topRax) {
	/* this is the axis which is getting resampled, 
	   so the number of samples is potentially changing */
	sz[p+1][permute[d]] = (info->kernel[ax[p][d]]
			       ? info->samples[ax[p][d]]
			       : sz[p][d]);
      }
      else {
	/* this axis is just a shuffled version of the
	   previous axis; no resampling this pass.
	   Note: this case also includes axes which aren't 
	   getting resampled whatsoever */
	sz[p+1][permute[d]] = sz[p][d];
      }
    }
  }
  /*
  printf("%s: axis arrangements for %d passes:\n", me, *passes);
  for (p=0; p<=*passes; p++) {
    printf("%s: %d:", me, p);
    for (d=0; d<=dim-1; d++) {
      printf("\t%d(%d)", ax[p][d], sz[p][d]); 
    }
    printf("\n");
  }
  */
  return;
}

/*
** _nrrdResampleMakeWeightIndex()
**
** _allocate_ and fill the arrays of indices and weights that are
** needed to process all the scanlines along a given axis; also
** be so kind as to return the sampling ratio (<1: downsampling,
** new sample spacing larger, >1: upsampling, new sample spacing smaller)
**
** returns "dotLen", the number of input samples which are required
** for resampling this axis, or 0 if there was an error.  Uses biff.
*/
int
_nrrdResampleMakeWeightIndex(float **weightP, int **indexP, float *ratioP,
			     Nrrd *nin, nrrdResampleInfo *info, int d) {
  char me[]="_nrrdResampleMakeWeightIndex", err[AIR_STRLEN_MED];
  int sizeIn, sizeOut, center, dotLen, halfLen, *index, base, idx;
  double minIn, maxIn, minOut, maxOut, spcIn, spcOut, param0=0.0;
  float ratio, support, integral, *weight, pos, idxF, wght;

  int e, i;

  if (!(info->kernel[d])) {
    sprintf(err, "%s: don't see a kernel for dimension %d", me, d);
    biffAdd(NRRD, err); *weightP = NULL; *indexP = NULL; return 0;
  }

  center = _nrrdCenter(nin->axis[d].center);
  sizeIn = nin->axis[d].size;
  sizeOut = info->samples[d];
  minIn = nin->axis[d].min;
  maxIn = nin->axis[d].max;
  minOut = info->min[d];
  maxOut = info->max[d];
  spcIn = NRRD_AXIS_SPACING(center, minIn, maxIn, sizeIn);
  spcOut = NRRD_AXIS_SPACING(center, minOut, maxOut, sizeOut);
  *ratioP = ratio = spcIn/spcOut;
  support = info->kernel[d]->support(info->param[d]);
  integral = info->kernel[d]->integral(info->param[d]);
  /*
  fprintf(stderr, 
	  "%s(%d): size{In,Out} = %d, %d, support = %f; ratio = %f\n", 
	  me, d, sizeIn, sizeOut, support, ratio);
  */
  if (ratio > 1) {
    /* if upsampling, we need only as many samples as needed for
       interpolation with the given kernel */
    dotLen = 2*ceil(support);
  }
  else {
    /* if downsampling, we need to use all the samples covered by
       the stretched out version of the kernel */
    dotLen = 2*ceil(support/ratio);
  }
  /*
  fprintf(stderr, "%s(%d): dotLen = %d\n", me, d, dotLen);
  */

  weight = calloc(sizeOut*dotLen, sizeof(float));
  index = calloc(sizeOut*dotLen, sizeof(int));
  if (!(weight && index)) {
    sprintf(err, "%s: can't allocate weight and index arrays", me);
    biffAdd(NRRD, err); *weightP = NULL; *indexP = NULL; return 0;
  }

  /* calculate sample locations and do first pass on indices */
  halfLen = dotLen/2;
  for (i=0; i<=sizeOut-1; i++) {
    pos = NRRD_AXIS_POS(center, minOut, maxOut, sizeOut, i);
    idxF = NRRD_AXIS_IDX(center, minIn, maxIn, sizeIn, pos);
    base = floor(idxF) - halfLen + 1;
    for (e=0; e<=dotLen-1; e++) {
      index[e + dotLen*i] = base + e;
      weight[e + dotLen*i] = idxF - index[e + dotLen*i];
    }
    /*
    if (!i)
      printf("%s: sample locations:\n", me);
    printf("%s: %d\n        ", me, i);
    for (e=0; e<=dotLen-1; e++)
      printf("%d/%g ", index[e + dotLen*i], weight[e + dotLen*i]);
    printf("\n");
    */
  }

  /*
  nrrdBoundaryPad,      1: fill with some user-specified value
  nrrdBoundaryBleed,    2: copy the last/first value out as needed
  nrrdBoundaryWrap,     3: wrap-around
  nrrdBoundaryWeight,   4: normalize the weighting on the existing samples;
			ONLY sensible for a strictly positive kernel
			which integrates to unity (as in blurring)
  */

  /* figure out what to do with the out-of-range indices */
  for (i=0; i<=dotLen*sizeOut-1; i++) {
    idx = index[i];
    if (!AIR_INSIDE(0, idx, sizeIn-1)) {
      switch(info->boundary) {
      case nrrdBoundaryPad:
      case nrrdBoundaryWeight:  /* this will be further handled later */
	idx = sizeIn;
	break;
      case nrrdBoundaryBleed:
	idx = AIR_CLAMP(0, idx, sizeIn-1);
	break;
      case nrrdBoundaryWrap:
	idx = AIR_MOD(idx, sizeIn);
	break;
      default:
	sprintf(err, "%s: boundary behavior %d unknown/unimplemented", 
		me, info->boundary);
	biffAdd(NRRD, err); *weightP = NULL; *indexP = NULL; return 0;
      }
      index[i] = idx;
    }
  }

  /* run the sample locations through the chosen kernel.  We play a 
     sneaky trick on the kernel parameter 0 in case of downsampling. */
  if (ratio < 1) {
    param0 = info->param[d][0];
    info->param[d][0] = param0/ratio;
  }
  info->kernel[d]->evalN_f(weight, weight, dotLen*sizeOut, info->param[d]);
  if (ratio < 1) {
    info->param[d][0] = param0;
  }

  /*
    if (nrrdBoundaryWeight == info->boundary) {
      if (integral) {
      }
    }
    else {
  */

  if (nrrdBoundaryWeight == info->boundary) {
    if (integral) {
      /* above, we set to sizeIn all the indices that were out of 
	 range.  We now use that to determine the sum of the weights
	 for the indices that were in-range */
      for (i=0; i<=sizeOut-1; i++) {
	wght = 0;
	for (e=0; e<=dotLen-1; e++) {
	  if (sizeIn != index[e + dotLen*i]) {
	    wght += weight[e + dotLen*i];
	  }
	}
	for (e=0; e<=dotLen-1; e++) {
	  idx = index[e + dotLen*i];
	  if (sizeIn != idx) {
	    weight[e + dotLen*i] *= integral/wght;
	  }
	  else {
	    weight[e + dotLen*i] = 0;
	  }
	}
      }
    }
  }
  else {
    /* try to remove ripple/grating on downsampling */
    if (ratio < 1 && info->renormalize && integral) {
      for (i=0; i<=sizeOut-1; i++) {
	wght = 0;
	for (e=0; e<=dotLen-1; e++) {
	  wght += weight[e + dotLen*i];
	}
	if (wght) {
	  for (e=0; e<=dotLen-1; e++) {
	    weight[e + dotLen*i] *= integral/wght;
	  }
	}
      }
    }
  }
  /*
  printf("%s: sample weights:\n", me);
  for (i=0; i<=sizeOut-1; i++) {
    printf("%s: %d\n        ", me, i);
    wght = 0;
    for (e=0; e<=dotLen-1; e++) {
      printf("%d/%g ", index[e + dotLen*i], weight[e + dotLen*i]);
      wght += weight[e + dotLen*i];
    }
    printf(" (sum = %g)\n", wght);
  }
  */

  *weightP = weight;
  *indexP = index;
  /*
  fprintf(stderr, "%s: dotLen = %d\n", me, dotLen);
  */
  return dotLen;
}

/*
******** nrrdSpatialResample()
**
** general-purpose array-resampler: resamples a nrrd of any type
** (except block) and any dimension along any or all of its axes, with
** any combination of up- or down-sampling along the axes, with any
** kernel (specified by callback), with potentially a different kernel
** for each axis.  Whether or not to resample along axis d is
** controlled by the non-NULL-ity of info->kernel[d].  Where to sample
** on the axis is controlled by info->min[d] and info->max[d]; these
** specify a range of "positions" aka "world space" positions, as 
** determined by the per-axis min and max of the input nrrd, which must
** be set for every resampled axis.
** 
** we cyclically permute those axes being resampled, and never touch
** the position (in axis ordering) of axes along which we are not
** resampling.  This strategy is certainly not the most intelligent
** one possible, but it does mean that the axis along which we're
** currently resampling-- the one along which we'll have to look at
** multiple adjecent samples-- is that resampling axis which is
** currently most contiguous in memory.  It may make sense to precede
** the resampling with an axis permutation which bubbles all the
** resampled axes to the front (most contiguous) end of the axis list,
** and then puts them back in place afterwards, depending on the cost
** of such axis permutation overhead.
*/
int
nrrdSpatialResample(Nrrd *nout, Nrrd *nin, nrrdResampleInfo *info) {
  char me[]="nrrdSpatialResample", err[AIR_STRLEN_MED];
  float *arr[NRRD_DIM_MAX],   /* intermediate copies of the input data
				 undergoing resampling; we don't need a full-
				 fledged nrrd for these.  Only about two of
				 these arrays will be allocated at a time;
				 intermediate results will be free()d when not
				 needed */
    *_in,                     /* current input vector being resampled;
				 not necessarily contiguous in memory
				 (if strideIn != 1) */
    *in,                      /* buffer for input vector; contiguous */
    *_out,                    /* output vector in context of volume;
				 never contiguous */
    ratio,                 
    ratios[NRRD_DIM_MAX],     /* record of "ratio" for all resampled axes,
				 used to compute new spacing in output */
    tmpF;           
  Nrrd *floatNin;             /* if the input nrrd is not of type float,
				 then we make a copy here */
  int i, s, d, e,
    pass,                     /* current pass */
    topLax,
    topRax,                   /* the lowest index of an axis which is
				 resampled.  If all axes are being resampled,
				 then this is 0.  If for some reason the
				 "x" axis (fastest stride) is not being
				 resampled, but "y" is, then topRax is 1 */
    botRax,                   /* index of highest axis being resampled */
    dim,                      /* dimension of thing we're resampling */
    typeIn, typeOut,          /* types of input and output of resampling */
    passes,                   /* # of passes needed to resample all axes */
    permute[NRRD_DIM_MAX],    /* how to permute axes of last pass to get
				 axes for current pass */
    ax[NRRD_DIM_MAX+1][NRRD_DIM_MAX],  /* axis ordering on each pass */
    sz[NRRD_DIM_MAX+1][NRRD_DIM_MAX];  /* how many samples along each
					  axis, changing on each pass */

  /* all these variables have to do with the spacing of elements in
     memory for the current pass of resampling, and they (except
     strideIn) are re-set at the beginning of each pass */
  float
    *weight;                  /* sample weights */
  int 
    ci[NRRD_DIM_MAX+1],
    co[NRRD_DIM_MAX+1],
    sizeIn, sizeOut,          /* lengths of input and output vectors */
    dotLen,                   /* # input samples to dot with weights to get
				 one output sample */
    *index;                   /* dotLen*sizeOut 2D array of input indices */
  nrrdBigInt 
    I,                        /* swiss-army int */
    strideIn,                 /* the stride between samples in the input
				 "scanline" being resampled */
    strideOut,                /* stride between samples in output 
				 "scanline" from resampling */
    L, LI, LO, numLines,      /* top secret */
    numOut;                   /* # of _samples_, total, in output volume;
				 this is for allocating the output */
  airArray *mop;              /* for cleaning up */
  
  if (!(nout && nin && info)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdBoundaryUnknown == info->boundary) {
    sprintf(err, "%s: need to specify a boundary behavior", me);
    biffAdd(NRRD, err); return 1;
  }

  dim = nin->dim;
  typeIn = nin->type;
  typeOut = nrrdTypeUnknown == info->type ? typeIn : info->type;

  if (_nrrdResampleCheckInfo(nin, info)) {
    sprintf(err, "%s: problem with arguments", me);
    biffAdd(NRRD, err); return 1;
  }

  _nrrdResampleComputePermute(permute, ax, sz,
			      &topRax, &botRax, &passes,
			      nin, info);
  topLax = topRax ? 0 : 1;

  if (0 == passes) {
    /* actually, no resampling was desired.  Copy input to output,
       but with the clamping that we normally do at the end of resampling */
    nrrdAxesGet_nva(nin, nrrdAxesInfoSize, sz[0]);
    if (nrrdMaybeAlloc_nva(nout, typeOut, nin->dim, sz[0])) {
      sprintf(err, "%s: couldn't allocate output", me);
      biffAdd(NRRD, err); return 1;
    }
    numOut = nrrdElementNumber(nout);
    for (I=0; I<=numOut-1; I++) {
      tmpF = nrrdFLookup[nin->type](nin->data, I);
      tmpF = nrrdFClamp[typeOut](tmpF);
      nrrdFInsert[typeOut](nout->data, I, tmpF);
    }
    return 0;
  }

  mop = airMopInit();
  /* convert input nrrd to float if necessary */
  if (nrrdTypeFloat != typeIn) {
    if (nrrdConvert(floatNin = nrrdNew(), nin, nrrdTypeFloat)) {
      sprintf(err, "%s: couldn't create float copy of input", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    arr[0] = floatNin->data;
    airMopAdd(mop, floatNin, (airMopper)nrrdNuke, airMopAlways);
  }
  else {
    floatNin = NULL;
    arr[0] = nin->data;
  }
  
  /* compute strideIn; this is actually the same for every pass
     because (strictly speaking) in every pass we are resampling
     the same axis, and axes with lower indices are constant length */
  strideIn = 1;
  for (d=0; d<topRax; d++) {
    strideIn *= nin->axis[d].size;
  }
  /*
  printf("%s: strideIn = "NRRD_BIG_INT_PRINTF"\n", me, strideIn);
  */

  /* go! */
  for (pass=0; pass<=passes-1; pass++) {
    /*
    printf("%s: --- pass %d --- \n", me, pass);
    */
    numLines = strideOut = 1;
    for (d=0; d<=dim-1; d++) {
      if (d < botRax)
	strideOut *= sz[pass+1][d];
      if (d != topRax)
	numLines *= sz[pass][d];
    }
    sizeIn = sz[pass][topRax];
    sizeOut = sz[pass+1][botRax];
    numOut = numLines*sizeOut;
    /* for the rest of the loop body, d is the original "dimension"
       for the axis being resampled */
    d = ax[pass][topRax];
    /*
    printf("%s(%d): numOut = "NRRD_BIG_INT_PRINTF"\n", me, pass, numOut);
    printf("%s(%d): numLines = "NRRD_BIG_INT_PRINTF"\n", me, pass, numLines);
    printf("%s(%d): stride: In=%d, Out=%d\n", me, pass, 
	   (int)strideIn, (int)strideOut);
    printf("%s(%d): sizeIn = %d\n", me, pass, sizeIn);
    printf("%s(%d): sizeOut = %d\n", me, pass, sizeOut);
    */

    /* we can free the input to the previous pass 
       (if its not the given data) */
    if (pass > 0) {
      if (pass == 1) {
	if (arr[0] != nin->data) {
	  airMopSub(mop, floatNin, (airMopper)nrrdNuke);
	  floatNin = nrrdNuke(floatNin);
	  arr[0] = NULL;
	  /*
	  printf("%s: pass %d: freeing arr[0]\n", me, pass);
	  */
	}
      }
      else {
	airMopSub(mop, arr[pass-1], airFree);
	arr[pass-1] = airFree(arr[pass-1]);
	/*
	printf("%s: pass %d: freeing arr[%d]\n", me, pass, pass-1);
	*/
      }
    }

    /* allocate output volume */
    arr[pass+1] = (float*)calloc(numOut, sizeof(float));
    if (!arr[pass+1]) {
      sprintf(err, "%s: couldn't create array of "NRRD_BIG_INT_PRINTF" floats"
	      " for output of pass %d", me, numOut, pass);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    airMopAdd(mop, arr[pass+1], airFree, airMopAlways);
    /*
    printf("%s: allocated arr[%d]\n", me, pass+1);
    */

    /* allocate contiguous input scanline buffer, we alloc one more
       than needed to provide a place for the pad value.  That is, in
       fact, the over-riding reason to copy a scanline to a local
       array: so that there is a simple consistent (non-branchy) way
       to incorporate the pad values */
    in = (float *)calloc(sizeIn+1, sizeof(float));
    airMopAdd(mop, in, airFree, airMopAlways);
    in[sizeIn] = info->padValue;

    dotLen = _nrrdResampleMakeWeightIndex(&weight, &index, &ratio,
					  nin, info, d);
    if (!dotLen) {
      sprintf(err, "%s: trouble creating weight and index vector arrays", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    ratios[d] = ratio;
    airMopAdd(mop, weight, airFree, airMopAlways);
    airMopAdd(mop, index, airFree, airMopAlways);

    /* the skinny: resample all the scanlines */
    _in = arr[pass];
    _out = arr[pass+1];
    memset(ci, 0, (NRRD_DIM_MAX+1)*sizeof(int));
    memset(co, 0, (NRRD_DIM_MAX+1)*sizeof(int));
    for (L=0; L<numLines; L++) {
      /* calculate the index to get to input and output scanlines,
	 according the coordinates of the start of the scanline */
      NRRD_COORD_INDEX(LI, ci, sz[pass], dim);
      NRRD_COORD_INDEX(LO, co, sz[pass+1], dim);
      _in = arr[pass] + LI;
      _out = arr[pass+1] + LO;
      
      /* read input scanline into contiguous array */
      for (i=0; i<sizeIn; i++) {
	in[i] = _in[i*strideIn];
      }

      /* do the weighting */
      for (i=0; i<sizeOut; i++) {
	tmpF = 0.0;
	for (s=0; s<dotLen; s++)
	  tmpF += in[index[s + dotLen*i]]*weight[s + dotLen*i];
	_out[i*strideOut] = tmpF;
      }
 
      /* update the coordinates for the scanline starts.  We don't
	 use the usual NRRD_COORD macros because we're subject to
	 the unusual constraint that ci[topRax] and co[permute[topRax]]
	 must stay exactly zero */
      e = topLax;
      ci[e]++; 
      co[permute[e]]++;
      while (L < numLines-1 && ci[e] == sz[pass][e]) {
	ci[e] = co[permute[e]] = 0;
	e++;
	e += e == topRax;
	ci[e]++; 
	co[permute[e]]++;
      }
    }

    /* pass-specific clean up */
    airMopSub(mop, weight, airFree);
    airMopSub(mop, index, airFree);
    airMopSub(mop, in, airFree);
    weight = airFree(weight);
    index = airFree(index);
    in = airFree(in);
  }

  /* clean up second-to-last array and scanline buffers */
  if (passes > 1) {
    airMopSub(mop, arr[passes-1], airFree);
    arr[passes-1] = airFree(arr[passes-1]);
    /*
    printf("%s: now freeing arr[%d]\n", me, passes-1);
    */
  }
  else if (arr[passes-1] != nin->data) {
    airMopSub(mop, floatNin, (airMopper)nrrdNuke);
    floatNin = nrrdNuke(floatNin);
  }
  arr[passes-1] = NULL;
  
  /* create output nrrd and set axis info */
  if (nrrdMaybeAlloc_nva(nout, typeOut, dim, sz[passes])) {
    sprintf(err, "%s: couldn't allocate final output nrrd", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  /*
  printf("!%s: nout: dim = %d; sz[] = %d %d %d\n", me,
	 dim, sz[passes][0],  sz[passes][1],  sz[passes][2]);
  */
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopOnError);
  nrrdAxesCopy(nout, nin, NULL, 
	       (NRRD_AXESINFO_SIZE
		| NRRD_AXESINFO_AMINMAX
		| NRRD_AXESINFO_SPACING));
  for (d=0; d<=dim-1; d++) {
    if (info->kernel[d]) {
      nout->axis[d].min = info->min[d];
      nout->axis[d].max = info->max[d];
      nout->axis[d].spacing = nin->axis[d].spacing/ratios[d];
    }
    else {
      nout->axis[d].min = nin->axis[d].min;
      nout->axis[d].max = nin->axis[d].max;
      nout->axis[d].spacing = nin->axis[d].spacing;
    }
  }

  /* maybe copy the resampling final result into the output nrrd,
     clamping as we go to insure that fixed point results don't have
     unexpected wrap-around.  */
  numOut = nrrdElementNumber(nout);
  if (info->clamp) {
    for (I=0; I<=numOut-1; I++) {
      tmpF = nrrdFClamp[typeOut](arr[passes][I]);
      nrrdFInsert[typeOut](nout->data, I, tmpF);
    }
  }
  else {
    for (I=0; I<=numOut-1; I++) {
      nrrdFInsert[typeOut](nout->data, I, arr[passes][I]);
    }
  }

  /* enough already */
  airMopOkay(mop);
  return 0;
}
