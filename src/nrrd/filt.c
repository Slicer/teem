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


#include "nrrd.h"

int
_median(unsigned char *hist, int half) {
  int sum = 0;
  unsigned char *hpt;
  
  hpt = hist;
  
  while(sum < half)
    sum += *hpt++;
  
  return(hpt - 1 - hist);
}

int
_index(Nrrd *nin, NRRD_BIG_INT I, int bins) {
  double val;
  int idx;
  
  val = nrrdDLookup[nin->type](nin->data, I);
  AIR_INDEX(nin->min, val, nin->max, bins, idx);
  return(idx);
}

void
_printhist(unsigned char *hist, int bins, char *desc) {
  int i;

  printf("%s:\n", desc);
  for (i=0; i<=bins-1; i++) {
    if (hist[i]) {
      printf("   %d: %d\n", i, hist[i]);
    }
  }
}

void
_nrrdMedian1D(Nrrd *nout, Nrrd *nin, int radius, 
	      int bins, unsigned char *hist) {
  /* char me[] = "_nrrdMedian1D"; */
  NRRD_BIG_INT X;
  int idx, diam, half;
  double val;

  diam = 2*radius + 1;
  half = diam/2 + 1;
  /* initialize histogram */
  memset(hist, 0, bins*sizeof(unsigned char));
  for (X=0; X<=diam-1; X++) {
    hist[_index(nin, X, bins)]++;
  }
  /* _printhist(hist, bins, "after init"); */
  /* find median at each point using existing histogram */
  for (X=radius; X<=nin->num-radius-1; X++) {
    idx = _median(hist, half);
    val = AIR_AFFINE(0, idx, bins-1, nin->min, nin->max);
    nrrdDInsert[nout->type](nout->data, X, val);
    /* probably update histogram for next iteration */
    if (X < nin->num-radius-1) {
      hist[_index(nin, X+radius+1, bins)]++;
      hist[_index(nin, X-radius, bins)]--;
    }
  }
}

void
_nrrdMedian2D(Nrrd *nout, Nrrd *nin, int radius, 
	      int bins, unsigned char *hist) {
  /* char me[] = "_nrrdMedian2D"; */
  NRRD_BIG_INT X, Y, I, J;
  int sx, sy, idx, diam, half;
  double val;

  diam = 2*radius + 1;
  half = diam*diam/2 + 1;
  sx = nin->size[0];
  sy = nin->size[1];
  for (Y=radius; Y<=sy-radius-1; Y++) {
    /* initialize histogram */
    memset(hist, 0, bins*sizeof(unsigned char));
    for (J=-radius; J<=radius; J++) {
      for (I=0; I<=diam-1; I++) {
	hist[_index(nin, I + sx*(J+Y), bins)]++;
      }
    }
    /* find median at each point using existing histogram */
    for (X=radius; X<=sx-radius-1; X++) {
      idx = _median(hist, half);
      val = AIR_AFFINE(0, idx, bins-1, nin->min, nin->max);
      nrrdDInsert[nout->type](nout->data, X + sx*Y, val);
      /* probably update histogram for next iteration */
      if (X < sx-radius-1) {
	for (J=-radius; J<=radius; J++) {
	  hist[_index(nin, X+radius+1 + sx*(J+Y), bins)]++;
	  hist[_index(nin, X-radius + sx*(J+Y), bins)]--;
	}
      }
    }
  }
}

void
_nrrdMedian3D(Nrrd *nout, Nrrd *nin, int radius, 
	      int bins, unsigned char *hist) {
  /* char me[] = "_nrrdMedian3D"; */
  NRRD_BIG_INT X, Y, Z, I, J, K;
  int sx, sy, sz, idx, diam, half;
  double val;

  diam = 2*radius + 1;
  half = diam*diam*diam/2 + 1;
  sx = nin->size[0];
  sy = nin->size[1];
  sz = nin->size[2];
  for (Z=radius; Z<=sz-radius-1; Z++) {
    for (Y=radius; Y<=sy-radius-1; Y++) {
      /* initialize histogram */
      memset(hist, 0, bins*sizeof(unsigned char));
      for (K=-radius; K<=radius; K++) {
	for (J=-radius; J<=radius; J++) {
	  for (I=0; I<=diam-1; I++) {
	    hist[_index(nin, I + sx*(J+Y + sy*(K+Z)), bins)]++;
	  }
	}
      }
      /* find median at each point using existing histogram */
      for (X=radius; X<=sx-radius-1; X++) {
	idx = _median(hist, half);
	val = AIR_AFFINE(0, idx, bins-1, nin->min, nin->max);
	nrrdDInsert[nout->type](nout->data, X + sx*(Y + sy*Z), val);
	/* probably update histogram for next iteration */
	if (X < sx-radius-1) {
	  for (K=-radius; K<=radius; K++) {
	    for (J=-radius; J<=radius; J++) {
	      hist[_index(nin, X+radius+1 + sx*(J+Y + sy*(K+Z)), bins)]++;
	      hist[_index(nin, X-radius + sx*(J+Y + sy*(K+Z)), bins)]--;
	    }
	  }
	}
      }
    }
  }
}


int
nrrdMedian(Nrrd *nout, Nrrd *nin, int radius, int bins) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdMedian";
  unsigned char *hist;
  int d;

  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (!(radius >= 1)) {
    sprintf(err, "%s: need radius >= 1 (got %d)", me, radius);
    biffSet(NRRD, err); return 1;
  }
  if (!(bins >= 1)) {
    sprintf(err, "%s: need bins >= 1 (got %d)", me, bins);
    biffSet(NRRD, err); return 1;
  }
  if (!(AIR_INSIDE(1, nin->dim, 3))) {
    sprintf(err, "%s: can only handle dim 1, 2, 3 (not %d)", me, nin->dim);
    biffSet(NRRD, err); return 1;    
  }
  if (!(nout->data)) {
    if (nrrdAlloc(nout, nin->num, nin->type, nin->dim)) {
      sprintf(err, "%s: nrrdAlloc() failed to create slice", me);
      biffSet(NRRD, err); return 1;
    }
  }
  if (!(AIR_EXISTS(nin->min) && AIR_EXISTS(nin->max))) {
    if (nrrdRange(&nin->min, &nin->max, nin)) {
      sprintf(err, "%s: couldn't learn value range", me);
      biffSet(NRRD, err); return 1;
    }
  }
  if (!(hist = calloc(bins, sizeof(unsigned char)))) {
    sprintf(err, "%s: couldn't allocate histogram (%d bins)", me, bins);
    biffSet(NRRD, err); return 1;
  }
  switch (nin->dim) {
  case 1:
    _nrrdMedian1D(nout, nin, radius, bins, hist);
    break;
  case 2:
    _nrrdMedian2D(nout, nin, radius, bins, hist);
    break;
  case 3:
    _nrrdMedian3D(nout, nin, radius, bins, hist);
    break;
  default:
    sprintf(err, "%s: can't handle dimensions %d", me, nin->dim);
    biffSet(NRRD, err); return 1;
  }

  for (d=0; d<=nin->dim-1; d++) {
    nout->size[d] = nin->size[d];
    nout->spacing[d] = nin->spacing[d];
    nout->axisMin[d] = nin->axisMin[d];
    nout->axisMax[d] = nin->axisMax[d];
    strcpy(nout->label[d], nin->label[d]);
  }
  sprintf(nout->content, "median(%s,%d,%d)", nin->content, radius, bins);
  free(hist);
  return(0);
}

int
_nrrdResampleCheckInfo(Nrrd *nin, nrrdResampleInfo *info) {
  char me[] = "_nrrdResampleCheckInfo", err[NRRD_BIG_STRLEN];
  nrrdKernel *k;
  int p, d, np;

  for (d=0; d<=nin->dim-1; d++) {
    k = info->kernel[d];
    /* we only care about the axes being resampled */
    if (!k)
      continue;
    np = k->numParam();
    for (p=0; p<=np-1; p++) {
      if (!AIR_EXISTS(info->param[d][p])) {
	sprintf(err, "%s: didn't set parameter %d for axis %d\n", me, p, d);
	biffSet(NRRD, err); return 1;
      }
    }
    if (!(AIR_EXISTS(info->min[d]) && AIR_EXISTS(info->max[d]))) {
      sprintf(err, "%s: didn't set min and max domain limits for axis %d",
	      me, d);
      biffSet(NRRD, err); return 1;
    }
    if (!(info->min[d] != info->max[d])) {
      sprintf(err, "%s: need to have axis %d min (%g) and max (%g) distinct",
	      me, d, info->min[d], info->max[d]);
      biffSet(NRRD, err); return 1;
    }
    if (!(info->samples[d] > 1)) {
      sprintf(err, "%s: axis %d # samples (%d) invalid", 
	      me, d, info->samples[d]);
      biffSet(NRRD, err); return 1;
    }
  }
  if (nrrdBoundaryUnknown == info->boundary) {
    sprintf(err, "%s: didn't set boundary behavior\n", me);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdBoundaryPad == info->boundary && !AIR_EXISTS(info->padValue)) {
    sprintf(err, "%s: asked for boundary padding, but no pad value set\n", me);
    biffSet(NRRD, err); return 1;
  }
  return 0;
}


/*
** _nrrdResampleComputePermute()
**
** figures out information related to how the axes in a nrrd are
** permuted during resampling: topRax, botRax, passes, ax[][], sz[][]
*/
void
_nrrdResampleComputePermute(int permute[], 
			    int ax[NRRD_MAX_DIM][NRRD_MAX_DIM], 
			    int sz[NRRD_MAX_DIM][NRRD_MAX_DIM], 
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
     (permute answers "where do I put this", not "what do I put here").
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
  if (!*passes)
    return;
  
  /*
  printf("%s: permute:\n", me);
  for (d=0; d<=dim-1; d++) {
    printf("   permute[%d] = %d\n", d, permute[d]);
  }
  */

  /* create array of how the axes will be arranged in each pass ("ax"), 
     and create array of how big each axes is in each pass ("sz").
     The input pass i will have axis layout described in ax[i] and
     axis sizes described in sz[i] */
  for (d=0; d<=dim-1; d++) {
    ax[0][d] = d;
    sz[0][d] = nin->size[d];
  }
  for (p=0; p<=*passes-1; p++) {
    for (d=0; d<=dim-1; d++) {
      ax[p+1][permute[d]] = ax[p][d];
      if (d == *topRax) {
	/* this is the axis which is getting resampled, 
	   so the number of samples is changing.
	   Or not. */
	sz[p+1][permute[d]] = (info->kernel[ax[p][d]]
			       ? info->samples[ax[p][d]]
			       : sz[p][d]);
      }
      else {
	/* this axis is just a shuffled version of the
	   previous axis; no resampling this pass */
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
** _nrrdResampleFillSmpIndex()
**
** allocate and fill the arrays of indices and weights that are
** needed to process all the scanlines along a given axis; also
** be so kind as to return the sampling ratio (<1: downsampling,
** result has fewer samples, >1: upsampling, result has more samples)
*/
int
_nrrdResampleFillSmpIndex(float **smpP, int **indexP, float *smpRatioP,
			  Nrrd *nin, nrrdResampleInfo *info, int d) {
  char me[]="_nrrdResampleFillSmpIndex", err[NRRD_BIG_STRLEN];
  float smpRatio, suppF, *smp, tmpF, p0, integral;
  int e, i, ind, lengthIn, lengthOut, dotLen, *index;
  
  if (!(info->kernel[d])) {
    sprintf(err, "%s: don't see a kernel for dimension %d", me, d);
    biffAdd(NRRD, err); return 0;
  }

  lengthIn = nin->size[d];
  lengthOut = info->samples[d];
  smpRatio = (lengthOut-1)/(info->max[d] - info->min[d]);
  suppF = info->kernel[d]->support(info->param[d]);
  integral = info->kernel[d]->integral(info->param[d]);
  /*
  fprintf(stderr, "%s(%d): suppF = %g; smpRatio = %g\n", 
	  me, d, suppF, smpRatio);
  */
  if (smpRatio > 1) {
    /* if upsampling, we need only as many samples as needed for
       interpolation with the given kernel */
    dotLen = 2*AIR_ROUNDUP(suppF);
  }
  else {
    /* if downsampling, we need to use all the samples covered by
       the stretched out version of the kernel */
    dotLen = 2*AIR_ROUNDUP(suppF/smpRatio);
  }
  smp = (float *)calloc(lengthOut*dotLen, sizeof(float));
  index = (int *)calloc(lengthOut*dotLen, sizeof(int));
  if (!(smp && index)) {
    sprintf(err, "%s: can't alloc smp and index", me);
    biffAdd(NRRD, err); return 0;
  }

  /*
  nrrdBoundaryPad,      1: fill with some user-specified value
  nrrdBoundaryBleed,    2: copy the last/first value out as needed
  nrrdBoundaryWrap,     3: wrap-around
  nrrdBoundaryWeight,   4: normalize the weighting on the existing samples;
			ONLY sensible for a strictly positive kernel
			which integrates to unity (as in blurring)
  */

  /* calculate sample locations and do first pass on indices */
  for (i=0; i<=lengthOut-1; i++) {
    tmpF = AIR_AFFINE(0, i, lengthOut-1, info->min[d], info->max[d]);
    for (e=0; e<=dotLen-1; e++) {
      index[e + dotLen*i] = e + (int)floor(tmpF) - dotLen/2 + 1;
      smp[e + dotLen*i] = tmpF - index[e + dotLen*i];
    }
    /*
    if (!i)
      printf("%s: sample locations:\n", me);
    printf("%s: %d\n        ", me, i);
    for (e=0; e<=dotLen-1; e++)
      printf("%d/%g ", index[e + dotLen*i], smp[e + dotLen*i]);
    printf("\n");
    */
  }

  /* figure out what to do with the out-of-range indices */
  for (i=0; i<=dotLen*lengthOut-1; i++) {
    ind = index[i];
    if (ind < 0 || ind > lengthIn-1) {
      switch(info->boundary) {
      case nrrdBoundaryPad:
	ind = lengthIn;
	break;
      case nrrdBoundaryBleed:
	ind = AIR_CLAMP(0, ind, lengthIn-1);
	break;
      case nrrdBoundaryWrap:
	ind = AIR_MOD(ind, lengthIn);
	break;
      default:
	sprintf(err, "%s: boundary behavior %d unknown/unimplemented", 
		me, info->boundary);
	biffAdd(NRRD, err); return 0;
      }
      index[i] = ind;
    }
  }

  /* run the sample locations through the chosen kernel */
  if (smpRatio < 1) {
    p0 = info->param[d][0];
    info->param[d][0] = p0/smpRatio;
  }
  info->kernel[d]->evalVec(smp, smp, dotLen*lengthOut, info->param[d]);
  if (smpRatio < 1) {
    info->param[d][0] = p0;
  }

  /* try to remove ripple/grating on downsampling */
  if (smpRatio < 1 && AIR_TRUE == info->renormalize && integral) {
    for (i=0; i<=lengthOut-1; i++) {
      tmpF = 0;
      for (e=0; e<=dotLen-1; e++) {
	tmpF += smp[e + dotLen*i];
      }
      if (tmpF) {
	for (e=0; e<=dotLen-1; e++) {
	  smp[e + dotLen*i] *= integral/tmpF;
	}
      }
    }
  }

  /*
  printf("%s: sample weights:\n", me);
  for (i=0; i<=lengthOut-1; i++) {
    printf("%s: %d\n        ", me, i);
    tmpF = 0;
    for (e=0; e<=dotLen-1; e++) {
      printf("%d/%g ", index[e + dotLen*i], smp[e + dotLen*i]);
      tmpF += smp[e + dotLen*i];
    }
    printf(" (sum = %g)\n", tmpF);
  }
  */

  *smpP = smp;
  *indexP = index;
  *smpRatioP = smpRatio;
  return dotLen;
}

/*
******** nrrdSpatialResample()
**
** general-purpose array-resampler: resamples a nrrd of any type and
** any dimension along any or all of its axes, with any combination of
** up- or down-sampling along the axes, with any kernel (specified by
** callback), with potentially a different kernel for each axis.
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
**
** The above paragraph does not pertain to reality.  Ignore it.
** The above sentence is only partly correct.  This situation needs fixing.
**
** on error, this often leaks memory like a sieve.  Fixing this will
** have to wait until I implement the "mop" library
*/
int
nrrdSpatialResample(Nrrd *nout, Nrrd *nin, nrrdResampleInfo *info) {
  char me[]="nrrdSpatialResample", err[NRRD_BIG_STRLEN];
  float *arr[NRRD_MAX_DIM],   /* intermediate copies of the array; we don't
				 need a full-fledged nrrd for these.  Only
				 about two of these arrays will be allocated
				 at a time; intermediate results will be
				 free()d when not needed */
    *_in,                     /* current input vector being resampled;
				 not necessarily contiguous in memory
				 (if strideIn != 1) */
    *in,                      /* buffer for input vector; contiguous */
    *_out,                    /* output vector in context of volume;
				 never contiguous */
    smpRatio,                 
    smpRatios[NRRD_MAX_DIM],  /* record of smpRatios for all resampled axes */
    tmpF;           
  Nrrd *floatNin = NULL;      /* if the input nrrd is not of type float,
				 then we make a copy here */
  int i, s, d, e, p,
    topLax,
    topRax,                   /* the lowest index of an axis which is
				 resampled.  If all axes are being resampled,
				 that this is 0.  If for some reason the
				 "x" axis (fastest stride) is not being
				 resampled, but "y" is, then topRax is 1 */
    botRax,                   /* index of highest axis being resampled */
    dim,                      /* dimension of thing we're resampling */
    typeIn, typeOut,          /* types of input and output of resampling */
    passes,                   /* # of passes needed to resample all axes */
    permute[NRRD_MAX_DIM],    /* how to permute axes of last pass to get
				 axes for current pass */
    ax[NRRD_MAX_DIM][NRRD_MAX_DIM],  /* axis ordering on each pass */
    sz[NRRD_MAX_DIM][NRRD_MAX_DIM];  /* how many samples along each
					axis, changing on each pass */

  /* all these variables have to do with the spacing of elements in
     memory for the current pass of resampling, and they (except
     strideIn) are re-set at the beginning of each pass */
  float
    *smp;                     /* initially, sample locations (in kernel space),
				 then overwritten with sample weights */
  int 
    ci[NRRD_MAX_DIM+1],
    co[NRRD_MAX_DIM+1],
    lengthIn, lengthOut,      /* lengths of input and output vectors */
    dotLen,                   /* # input samples to dot with weights to get
				 one output sample */
    *index;                   /* dotLen*lengthOut 2D array of input indices */
  NRRD_BIG_INT 
    I,                        /* swiss-army int */
    strideIn,                 /* the stride between samples in the input
				 "scanline" being resampled */
    strideOut,                /* stride between samples in output 
				 "scanline" from resampling */
    L, LI, LO, numLines,      /* top secret */
    strideBIn, strideBOut,
    numOut;                   /* # of _samples_, total, in output volume;
				 this is for allocating the output */
  if (!(nout && nin && info)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdBoundaryUnknown == info->boundary) {
    sprintf(err, "%s: need to specify a boundary behavior", me);
    biffSet(NRRD, err); return 1;
  }

  dim = nin->dim;
  typeIn = nin->type;
  typeOut = nrrdTypeUnknown == info->type ? typeIn : info->type;

  if (_nrrdResampleCheckInfo(nin, info)) {
    sprintf(err, "%s: problem with arguments in nrrdResampleInfo", me);
    biffAdd(NRRD, err); return 1;
  }

  _nrrdResampleComputePermute(permute, ax, sz,
			      &topRax, &botRax, &passes,
			      nin, info);
  topLax = topRax ? 0 : 1;

  if (0 == passes) {
    /* actually, no resampling was desired.  Copy input to output */
    /* HEY! this could mean that fixed-point output types suffer
       wrap-around.  Should this be clamped? */
    if (nrrdConvert(nin, nout, typeOut)) {
      sprintf(err, "%s: couldn't copy input to output", me);
      biffAdd(NRRD, err); return 1;
    }
    return 0;
  }

  /* convert input nrrd to float if necessary */
  if (nrrdTypeFloat != typeIn) {
    floatNin = nrrdNew();
    if (nrrdConvert(floatNin, nin, nrrdTypeFloat)) {
      sprintf(err, "%s: couldn't create float copy of input", me);
      biffAdd(NRRD, err); return 1;
    }
    arr[0] = (float*)floatNin->data;
  }
  else {
    arr[0] = (float*)nin->data;
  }
  
  /* compute strideIn; this is actually the same for every pass
     because (strictly speaking) in every pass we are resampling
     the same axis, and axes with lower indices are constant length */
  strideIn = 1;
  for (d=0; d<=dim-1; d++) {
    if (!info->kernel[d]) {
      strideIn *= nin->size[d];
    }
    else {
      break;
    }
  }
  /*
  printf("%s: strideIn = "NRRD_BIG_INT_PRINTF"\n", me, strideIn);
  */

  /* go! */
  for (p=0; p<=passes-1; p++) {
    /*
    printf("%s: --- pass %d --- \n", me, p);
    */
    numOut = numLines = strideBIn = strideBOut = 1;
    for (d=0; d<=dim-1; d++) {
      if (d <= topRax)
	strideBIn *= sz[p][d];
      if (d <= botRax)
	strideBOut *= sz[p+1][d];
      if (d != topRax)
	numLines *= sz[p][d];
      numOut *= sz[p+1][d];
    }
    strideOut = strideBOut/sz[p+1][botRax];
    lengthIn = sz[p][topRax];
    lengthOut = sz[p+1][botRax];
    /* for the rest of the loop body, d is the original "dimension"
       for the axis being resampled */
    d = ax[p][topRax];
    /*
    printf("%s(%d): numOut = "NRRD_BIG_INT_PRINTF"\n", me, p, numOut);
    printf("%s(%d): numLines = "NRRD_BIG_INT_PRINTF"\n", me, p, numLines);
    printf("%s(%d): stride: In=%d, Out=%d\n", me, p, 
	   (int)strideIn, (int)strideOut);
    printf("%s(%d): strideB: In=%d, Out=%d\n", 
	   me, p, (int)strideBIn, (int)strideBOut);
    printf("%s(%d): lengthIn = %d\n", me, p, lengthIn);
    printf("%s(%d): lengthOut = %d\n", me, p, lengthOut);
    */

    /* we can free the input to the previous pass 
       (if its not the given data) */
    if (p > 0) {
      if (p == 1) {
	if (arr[0] != nin->data) {
	  floatNin = nrrdNuke(floatNin);

	  arr[0] = NULL;
	  /*
	  printf("%s: pass %d: freeing arr[0]\n", me, p);
	  */
	}
      }
      else {
	free(arr[p-1]);
	arr[p-1] = NULL;
	/*
	printf("%s: pass %d: freeing arr[%d]\n", me, p, p-1);
	*/
      }
    }

    /* allocate output volume */
    arr[p+1] = (float*)calloc(numOut, sizeof(float));
    if (!arr[p+1]) {
      sprintf(err, "%s: couldn't create array of "NRRD_BIG_INT_PRINTF" floats "
	      "for output of pass %d", me, numOut, p);
      biffAdd(NRRD, err); return 1;
    }
    /*
    printf("%s: allocated arr[%d]\n", me, p+1);
    */

    /* allocate contiguous input scanline buffer, we alloc one more
       than needed to provide a place for the pad value */
    in = (float *)calloc(lengthIn+1, sizeof(float));
    in[lengthIn] = info->padValue;

    dotLen = _nrrdResampleFillSmpIndex(&smp, &index, &smpRatio, nin, info, d);
    if (!dotLen) {
      sprintf(err, "%s: trouble creating smp and index vector arrays", me);
      biffAdd(NRRD, err); return 1;
    }
    smpRatios[d] = smpRatio;

    /* the real tofu of it: resample all the scanlines */
    _in = arr[p];
    _out = arr[p+1];
    memset(ci, 0, (NRRD_MAX_DIM+1)*sizeof(int));
    memset(co, 0, (NRRD_MAX_DIM+1)*sizeof(int));
    for (L=0; L<numLines; L++) {
      /* calculate the index to get to input and output scanlines,
	 according the coordinates of the start of the scanline */
      LI = ci[dim-1];
      LO = co[dim-1];
      for (e=dim-2; e>=0; e--) {
	LI = ci[e] + sz[p][e]*LI;
	LO = co[e] + sz[p+1][e]*LO;
      }
      _in = arr[p] + LI;
      _out = arr[p+1] + LO;
      
      /* read input scanline into contiguous array */
      for (i=0; i<lengthIn; i++) {
	in[i] = _in[i*strideIn];
      }

      /* do the weighting */
      tmpF = 0.0;
      for (i=0; i<lengthOut; i++) {
	tmpF = 0;
	for (s=0; s<dotLen; s++)
	  tmpF += in[index[s + dotLen*i]]*smp[s + dotLen*i];
	_out[i*strideOut] = tmpF;
      }
 
      /* update the coordinates for the scanline starts */
      e = topLax;
      ci[e]++; 
      co[permute[e]]++;
      while (L < numLines-1 && ci[e] == sz[p][e]) {
	ci[e] = co[permute[e]] = 0;
	e++;
	e += e == topRax;
	ci[e]++; 
	co[permute[e]]++;
      }
    }

    /* pass-specific clean up */
    free(smp);
    free(index);
    free(in);
  }

  /* clean up second-to-last array and scanline buffers */
  if (passes > 1) {
    free(arr[passes-1]);
    /*
    printf("%s: now freeing arr[%d]\n", me, passes-1);
    */
  }
  else if (arr[passes-1] != nin->data) {
    floatNin = nrrdNuke(floatNin);
  }
  arr[passes-1] = NULL;
  
  /* create output nrrd and set axis info */
  if (!nout->data) {
    if (nrrdAlloc(nout, numOut, typeOut, dim)) {
      sprintf(err, "%s: couldn't allocate final output nrrd", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  for (d=0; d<=dim-1; d++) {
    if (info->kernel[d]) {
      nout->size[d] = info->samples[d];
      nout->axisMin[d] = AIR_AFFINE(0, info->min[d], nin->size[d]-1,
				    nin->axisMin[d], nin->axisMax[d]);
      nout->axisMax[d] = AIR_AFFINE(0, info->max[d], nin->size[d]-1,
				    nin->axisMin[d], nin->axisMax[d]);
      nout->spacing[d] = nin->spacing[d]/smpRatios[d];
    }
    else {
      nout->size[d] = nin->size[d];
      nout->axisMin[d] = nin->axisMin[d];
      nout->axisMax[d] = nin->axisMax[d];
      nout->spacing[d] = nin->spacing[d];
    }
    strcpy(nout->label[d], nin->label[d]);
  }

  /* copy the resampling final result into the output nrrd, clamping
     as we go to insure that fixed point results don't have unexpected
     wrap-around.  Last value of numOut is still good. */
  for (I=0; I<=numOut-1; I++) {
    tmpF = nrrdFClamp[typeOut](arr[passes][I]);
    nrrdFInsert[typeOut](nout->data, I, tmpF);
  }

  /* final cleanup */
  free(arr[passes]);

  /* enough already */
  return 0;
}
