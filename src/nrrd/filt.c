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
  NRRD_INDEX(nin->min, val, nin->max, bins, idx);
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
_nrrdMedian1D(Nrrd *nin, Nrrd *nout, int radius, 
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
    val = NRRD_AFFINE(0, idx, bins-1, nin->min, nin->max);
    nrrdDInsert[nout->type](nout->data, X, val);
    /* probably update histogram for next iteration */
    if (X < nin->num-radius-1) {
      hist[_index(nin, X+radius+1, bins)]++;
      hist[_index(nin, X-radius, bins)]--;
    }
  }
}

void
_nrrdMedian2D(Nrrd *nin, Nrrd *nout, int radius, 
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
      val = NRRD_AFFINE(0, idx, bins-1, nin->min, nin->max);
      nrrdDInsert[nout->type](nout->data, X + sx*Y, val);
      /* probably update histogram for next iteration */
      if (X < nin->num-radius-1) {
	for (J=-radius; J<=radius; J++) {
	  hist[_index(nin, X+radius+1 + sx*(J+Y), bins)]++;
	  hist[_index(nin, X-radius + sx*(J+Y), bins)]--;
	}
      }
    }
  }
}

void
_nrrdMedian3D(Nrrd *nin, Nrrd *nout, int radius, 
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
	val = NRRD_AFFINE(0, idx, bins-1, nin->min, nin->max);
	nrrdDInsert[nout->type](nout->data, X + sx*(Y + sy*Z), val);
	/* probably update histogram for next iteration */
	if (X < nin->num-radius-1) {
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
nrrdMedian(Nrrd *nin, Nrrd *nout, int radius, int bins) {
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
  if (!(NRRD_INSIDE(1, nin->dim, 3))) {
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
    if (nrrdRange(nin)) {
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
    _nrrdMedian1D(nin, nout, radius, bins, hist);
    break;
  case 2:
    _nrrdMedian2D(nin, nout, radius, bins, hist);
    break;
  case 3:
    _nrrdMedian3D(nin, nout, radius, bins, hist);
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

Nrrd *
nrrdNewMedian(Nrrd *nin, int radius, int bins) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdNewMedian";
  Nrrd *nout;

  if (!(nout = nrrdNew())) {
    sprintf(err, "%s: nrrdNew() failed", me);
    biffAdd(NRRD, err); return NULL;
  }
  if (nrrdMedian(nin, nout, radius, bins)) {
    sprintf(err, "%s: nrrdMedian() failed", me);
    nrrdNuke(nout);
    biffAdd(NRRD, err); return NULL;
  }
  return nout;
}

/*
void
_nrrdSpatialResampleF(float *in, NRRD_BIG_INT lengthIn,
		      float *out, NRRD_BIG_INT lengthOut,
		      float min, float max, int clamp,
		      nrrdResampleInfo *info) {

}
*/

/*
******** nrrdSpatialResample()
**
** general-purpose array-resampler
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
    *out,                     /* buffer for output of resampling; contiguous */
    *_out;                    /* output vector in context of volume;
				 never contiguous */
  Nrrd *floatNin = NULL;      /* if the input nrrd is not of type float,
				 then we make a copy here */
  int d, p, a,
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
    ax[NRRD_MAX_DIM][NRRD_MAX_DIM];  /* axis ordering on each pass */
  double spacing[NRRD_MAX_DIM];      /* sample spacing in output */
  /* all these variables have to do with the spacing of elements in
     memory for the current pass of resampling, and they (except
     strideIn) are re-set at the beginning of each pass */
  NRRD_BIG_INT 
    I, J,                     /* big ints */
    L,                        /* which line we're processing */
    strideIn,                 /* the stride between samples in the
				 "scanline" being resampled */
    strideOut,                /* stride between samples in vector
				 output of resampling */
    lengthIn, lengthOut,      /* lengths of input and output vectors */
    numLines,                 /* # of scanlines being resampled this pass */
    numOut,                   /* # of _samples_, total, in output volume;
				 this is for allocating the output */
    maxLenIn,                 /* longest input of resampling one axis */
    maxLenOut,                /* longest output of resampling one axis */
    sz[NRRD_MAX_DIM][NRRD_MAX_DIM];  /* how many samples along each
					axis, changing on each pass */

    
  if (!(nout && nin && info)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  dim = nin->dim;
  typeIn = nin->type;
  typeOut = nrrdTypeUnknown == info->type ? typeIn : info->type;

  /* compute the cyclic permutation among axes that each pass should
     effect (store this in "permute[]"), and determine the number of
     passes required ("passes").  Basically, we cyclically permute
     those axes being resampled, and never touch the position (in axis
     ordering) of axes along which we are not resampling.  This
     strategy is certainly not the most intelligent one possible, but
     it does mean that the axis along which we're currently
     resampling-- the one along which we'll have to look at multiple
     adjecent samples-- is that resampling axis which is currently
     most contiguous in memory.  It may make sense to precede (and
     follow) the resampling with an axis permutation which bubbles all
     the resampled axes to the front (most contiguous) end of the axis
     list, and then puts them back in place afterwards, depending on
     the cost of such axis permutation overhead.*/
  passes = a = 0;
  topRax = -1;
  maxLenIn = maxLenOut = 0;
  for (d=0; d<=dim-1; d++) {
    if (info->kernel[d]) {
      if (topRax < 0) {
	topRax = d;
      }
      botRax = d;
      do {
	a = AIR_MOD(a+1, dim);
      } while (nrrdKernelUnknown == info->kernel[a]);
      permute[d] = a;
      passes += 1;
      spacing[d] = nin->spacing[d]*nin->size[d]/info->samples[d];
      maxLenOut = AIR_MAX(maxLenOut, info->samples[d]);
    }
    else {
      permute[d] = d;
      a += a == d;
      maxLenOut = AIR_MAX(maxLenOut, nin->size[d]);
    }
    maxLenIn = AIR_MAX(maxLenIn, nin->size[d]);
  }
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
  printf("%s: topRax = %d; botRax = %d\n", me, topRax, botRax);
  printf("%s: passes = %d\n", me, passes);
  printf("%s: maxLen: In=%d, Out=%d\n", me, (int)maxLenIn, (int)maxLenOut);
  printf("%s: permute:\n", me);
  for (d=0; d<=dim-1; d++) {
    printf("   perm[%d] = %d\n", d, permute[d]);
  }
  in = calloc(maxLenIn, sizeof(float));
  out = calloc(maxLenOut, sizeof(float));
  if (!(in && out)) {
    sprintf(err, "%s: couldn't alloc scanline buffers", me);
    biffAdd(NRRD, err); return 1;
  }

  /* create array of how the axes will be arranged in each pass
     ("ax"), and create array of how big each axes is in each pass
     ("sz") */
  for (d=0; d<=dim-1; d++) {
    ax[0][d] = d;
    sz[0][d] = nin->size[d];
  }
  for (p=0; p<=passes-1; p++) {
    for (d=0; d<=dim-1; d++) {
      ax[p+1][d] = ax[p][permute[d]];
      if (topRax == permute[d]) {
	/* this is the axis which is getting resampled, 
	   so the number of samples is changing */
	sz[p+1][d] = info->samples[ax[p][topRax]];
      }
      else {
	/* this axis is just a shuffled version of the
	   previous axis; no resampling this pass */
	sz[p+1][d] = sz[p][permute[d]];
      }
    }
  }
  for (p=0; p<=passes; p++) {
    printf("%s: %d:", me, p);
    for (d=0; d<=dim-1; d++) {
      printf("\t%d("NRRD_BIG_INT_PRINTF")", ax[p][d], sz[p][d]); 
    }
    printf("\n");
  }
  
  /* convert input nrrd to float if necessary */
  if (nrrdTypeFloat != typeIn) {
    floatNin = nrrdNewConvert(nin, nrrdTypeFloat);
    if (!floatNin) {
      sprintf(err, "%s: couldn't create float copy of input", me);
      free(in); free(out);
      biffAdd(NRRD, err); return 1;
    }
    arr[0] = (float*)floatNin->data;
  }
  else {
    arr[0] = (float*)nin->data;
  }
  
  /* compute strideIn */
  strideIn = 1;
  for (d=0; d<=dim-1; d++) {
    if (nrrdKernelUnknown == info->kernel[d]) {
      strideIn *= nin->size[d];
    }
    else {
      break;
    }
  }
  printf("%s: strideIn = "NRRD_BIG_INT_PRINTF"\n", me, strideIn);

  /* go! */
  for (p=0; p<=passes-1; p++) {
    printf("%s: --- pass %d --- \n", me, p);
    /* allocate output volume */
    numOut = strideOut = numLines = 1;
    for (d=0; d<=dim-1; d++) {
      numOut *= sz[p+1][d];
      if (d > topRax) {
	numLines *= sz[p][d];
      }
      if (d < botRax) {
	strideOut *= sz[p+1][d];
      }
    }
    lengthIn = sz[p][topRax];
    lengthOut = sz[p+1][botRax];
    printf("%s(%d): numLines = "NRRD_BIG_INT_PRINTF"\n", me, p, numLines);
    printf("%s(%d): strideOut = "NRRD_BIG_INT_PRINTF"\n", me, p, strideOut);
    printf("%s(%d): numOut = "NRRD_BIG_INT_PRINTF"\n", me, p, numOut);
    printf("%s(%d): lengthIn = "NRRD_BIG_INT_PRINTF"\n", me, p, lengthIn);
    printf("%s(%d): lengthOut = "NRRD_BIG_INT_PRINTF"\n", me, p, lengthOut);
    /* we can free the input to the previous pass 
       (if its not the given data) */
    if (p > 0 && arr[p-1]) {
      if (p == 1 && arr[0] != nin->data) {
	floatNin = nrrdNuke(floatNin);
      }
      else {
	free(arr[p-1]);
      }
    }
    arr[p+1] = (float*)calloc(numOut, sizeof(float));
    if (!arr[p+1]) {
      sprintf(err, "%s: couldn't create array of "NRRD_BIG_INT_PRINTF" floats "
	      "for output of pass %d", me, numOut, p);
      if (arr[p] && (arr[p] != nin->data)) {
	/* we make some effort to clean up */
	floatNin = nrrdNuke(floatNin);
      }
      free(in); free(out);
      biffAdd(NRRD, err); return 1;
    }

    for (L=0; L<=numLines-1; L++) {
      _in = arr[p] + strideIn*lengthIn*L;
      for (I=0; I<=lengthIn-1; I++) {
	in[I] = _in[I*strideIn];
      }

      /* resample one line */

      _out = arr[p+1] + strideOut*lengthOut*L;
      for (I=0; I<=lengthOut-1; I++) {
	_out[I*strideOut] = out[I];
      }
    }
  }

  /* clean up second-to-last array and scanline buffers */
  if (passes > 1) {
    free(arr[passes-1]);
  }
  else if (arr[passes-1] != nin->data) {
    floatNin = nrrdNuke(floatNin);
  }
  free(in);
  free(out);
  
  /* create output nrrd and set axis info */
  if (nrrdAlloc(nout, numOut, typeOut, dim)) {
    sprintf(err, "%s: couldn't allocate final output nrrd", me);
    free(arr[passes]);
    biffAdd(NRRD, err); return 1;
  }
  for (d=0; d<=dim-1; d++) {
    if (nrrdKernelUnknown != info->kernel[d]) {
      nout->size[d] = info->samples[d];
      nout->axisMin[d] = AIR_AFFINE(0, info->min[d], nin->size[d]-1,
				    nin->axisMin[d], nin->axisMax[d]);
      nout->axisMax[d] = AIR_AFFINE(0, info->max[d], nin->size[d]-1,
				    nin->axisMin[d], nin->axisMax[d]);
    }
    else {
      nout->size[d] = nin->size[d];
      nout->axisMin[d] = nin->axisMin[d];
      nout->axisMax[d] = nin->axisMax[d];
    }
    strcpy(nout->label[d], nin->label[d]);
    nout->spacing[d] = spacing[d];
  }
  /* copy the resampling final result into the output nrrd, clamping
     as we go to insure that fixed point results don't have unexpected
     wrap-around.  Last value of numOut is still good. */
  for (I=0; I<=numOut-1; I++) {
    nrrdFInsert[typeOut](nout->data, I, nrrdFClamp[typeOut](arr[passes][I]));
  }

  /* final cleanup */
  free(arr[passes]);
  
  return 0;
}
