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

/*
******** nrrdHistoAxis
**
** replace scanlines along one scanline with a histogram of the scanline
**
** this looks at nin->min and nin->max to see if they are both non-NaN.
** If so, it uses these as the range of the histogram, otherwise it
** finds the min and max present in the volume
**
** By its very nature, and by the simplicity of this implemention,
** this can be a slow process due to terrible memory locality.  User
** may want to permute axes before and after this, but that can be
** slow too...  
*/
int
nrrdHistoAxis(Nrrd *nout, Nrrd *nin, int ax, int bins, int type) {
  char err[NRRD_STRLEN_MED], me[] = "nrrdHistoAxis";
  int hidx, d, map[NRRD_DIM_MAX];
  unsigned int szIn[NRRD_DIM_MAX], szOut[NRRD_DIM_MAX],
    coordIn[NRRD_DIM_MAX], coordOut[NRRD_DIM_MAX];
  nrrdBigInt I, hI;
  float val;
  double count;

  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(bins > 0)) {
    sprintf(err, "%s: bins value (%d) invalid", me, bins);
    biffAdd(NRRD, err); return 1;
  }
  if (!( AIR_BETWEEN(nrrdTypeUnknown, type, nrrdTypeLast-1) )) {
    sprintf(err, "%s: invalid nrrd type %d", me, type);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_INSIDE(0, ax, nin->dim-1)) {
    sprintf(err, "%s: axis %d is not in range [0,%d]", me, ax, nin->dim-1);
    biffAdd(NRRD, err); return 1;
  }
  if (!( AIR_EXISTS(nin->min) && AIR_EXISTS(nin->max) )) {
    if (nrrdMinMaxFind(&nin->min, &nin->max, nin)) {
      sprintf(err, "%s: couldn't find value range", me);
      biffAdd(NRRD, err); return 1;
    }
    if (!(AIR_EXISTS(nin->min) && AIR_EXISTS(nin->max))) {
      sprintf(err, "%s: didn't see real numbers in nrrd (no min or max)", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  if (nrrdMaybeAlloc(nout, (nin->num/nin->axis[ax].size)*bins, 
		     type, nin->dim)) {
    sprintf(err, "%s: failed to alloc output nrrd", me);
    biffAdd(NRRD, err); return 1;
  }
  
  /* copy axis information */
  for (d=0; d<=nin->dim-1; d++) {
    map[d] = d != ax ? d : -1;
  }
  nrrdAxesCopy(nout, nin, map, NRRD_AXESINFO_NONE);
  nout->axis[ax].size = bins;
  nout->axis[ax].spacing = AIR_NAN; /* min and max convey the information */
  nout->axis[ax].min = nin->min;
  nout->axis[ax].max = nin->max;
  nout->axis[ax].center = nrrdCenterCell;
  if (nin->axis[ax].label) {
    nout->axis[ax].label = calloc(strlen("histax()")
				  + strlen(nin->axis[ax].label)
				  + 1, sizeof(char));
    if (nout->axis[ax].label) {
      sprintf(nout->axis[ax].label, "histax(%s)", nin->axis[ax].label);
    }
    else {
      sprintf(err, "%s: couldn't allocate output label", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  else {
    nout->axis[ax].label = NULL;
  }


  /* the skinny: we traverse the input samples in linear order, and
     increment the bin in the histogram for the scanline we're in.
     This is not terribly clever, and the memory locality is a
     disaster */
  nrrdAxesGet(nin, nrrdAxesInfoSize, szIn);
  nrrdAxesGet(nout, nrrdAxesInfoSize, szOut);
  memset(coordIn, 0, NRRD_DIM_MAX*sizeof(unsigned int));
  for (I=0; I<=nin->num-1; I++) {
    /* get input nrrd value and compute its histogram index */
    val = nrrdFLookup[nin->type](nin->data, I);
    AIR_INDEX(nin->min, val, nin->max, bins, hidx);
    if (AIR_INSIDE(0, hidx, bins-1)) {
      memcpy(coordOut, coordIn, nin->dim*sizeof(int));
      coordOut[ax] = hidx;
      NRRD_COORD_INDEX(hI, coordOut, szOut, nout->dim, d);
      count = nrrdDLookup[nout->type](nout->data, hI);
      count = nrrdDClamp[nout->type](count + 1);
      nrrdDInsert[nout->type](nout->data, hI, count);
    }
    NRRD_COORD_INCR(coordIn, szIn, nin->dim, d);
  }

  if (nin->content) {
    nout->content = calloc(strlen("histax(,,)")
			   + strlen(nin->content)
			   + 11
			   + 11
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->axis[ax].label, "histax(%s,%d,%d)", 
	      nin->axis[ax].label, ax, bins);
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  return(0);
}

/*
******** nrrdHisto()
**
** makes a 1D histogram of a given size and type
**
** this looks at nin->min and nin->max to see if they are both non-NaN.
** If so, it uses these as the range of the histogram, otherwise it
** finds the min and max present in the volume.  If nin->min and nin->max
** are being used as the histogram range, then values which fall outside
** this are ignored (they don't contribute to the histogram).
*/
int
nrrdHisto(Nrrd *nout, Nrrd *nin, int bins, int type) {
  char err[NRRD_STRLEN_MED], me[] = "nrrdHisto", cmt[NRRD_STRLEN_MED];
  int idx, foundRange;
  nrrdBigInt I;
  double min, max, val, count;

  if (!(nin && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(bins > 0)) {
    sprintf(err, "%s: bins value (%d) invalid", me, bins);
    biffAdd(NRRD, err); return 1;
  }
  if (!( AIR_BETWEEN(nrrdTypeUnknown, type, nrrdTypeLast-1) )) {
    sprintf(err, "%s: invalid nrrd type %d", me, type);
    biffAdd(NRRD, err); return 1;
  }

  if (nrrdMaybeAlloc_va(nout, type, 1, bins)) {
    sprintf(err, "%s: failed to alloc histo array (len %d)", me, bins);
    biffAdd(NRRD, err); return 1;
  }
  /* nout->axis[0].size set */
  nout->axis[0].spacing = AIR_NAN;
  if (!( AIR_EXISTS(nin->min) && AIR_EXISTS(nin->max) )) {
    foundRange = AIR_TRUE;
    if (nrrdMinMaxFind(&min, &max, nin)) {
      sprintf(err, "%s: couldn't determine value range", me);
      biffAdd(NRRD, err); return 1;
    }
    if (!(AIR_EXISTS(min) && AIR_EXISTS(max))) {
      sprintf(err, "%s: didn't see real numbers in nrrd (no min or max)", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  else {
    foundRange = AIR_FALSE;
    min = nin->min;
    max = nin->max;
  }
  if (min == max) {
    /* need this to insure that index generation isn't confused */
    max++;
    sprintf(cmt, "%s: artificially increasing max from %g to %g", 
	    me, min, max);
    nrrdCommentAdd(nout, cmt, AIR_FALSE);
  }
  nout->axis[0].min = min;
  nout->axis[0].max = max;
  nout->axis[0].center = nrrdCenterCell;
  /* nout->axis[0].label set below */
  
  /* make histogram */
  for (I=0; I<=nin->num-1; I++) {
    val = nrrdDLookup[nin->type](nin->data, I);
    if (AIR_EXISTS(val)) {
      if (val < min || val > max) {
	if (!foundRange) {
	  /* value is outside range; ignore it */
	  continue;
	}
	else {
	  /* not cool */
	  fprintf(stderr, "%s: PANIC: found value %g outside range [%g,%g]\n",
		  me, val, min, max);
	  exit(1);
	}
      }
      AIR_INDEX(min, val, max, bins, idx);
      /* count is a double in order to simplify clamping the
	 hit values to the representable range for nout->type */
      /*
      printf("!%s: %d: index(%g, %g, %g, %d) = %d\n", 
	     me, (int)I, min, val, max, bins, idx);
      */
      count = nrrdDLookup[nout->type](nout->data, idx);
      count = nrrdDClamp[nout->type](count + 1);
      nrrdDInsert[nout->type](nout->data, idx, count);
    }
  }

  if (nin->content) {
    nout->content = calloc(strlen("histo(,)")
			   + strlen(nin->content)
			   + 11
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "histo(%s,%d)", nin->content, bins);
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
    nout->axis[0].label = calloc(strlen("histo()")
				 + strlen(nin->content)
				 + 1, sizeof(char));
    if (nout->axis[0].label) {
      sprintf(nout->axis[0].label, "histo(%s)", nin->content);
    }
    else {
      sprintf(err, "%s: couldn't allocate output label", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  else {
    nout->content = airFree(nout->content);
    nout->axis[0].label = airFree(nout->axis[0].label);
  }
  
  return 0;
}

int 
nrrdHistoMulti(Nrrd *nout, Nrrd **nin, 
	       int num, int *bin, int type,
	       double *min, double *max, int *clamp) {
  char me[]="nrrdHistoMulti", err[NRRD_STRLEN_MED];
  int d, coord[NRRD_DIM_MAX], skip, hadContent, totalContentStrlen, len;
  double val, count;
  nrrdBigInt Iin, Iout, size;

  /* error checking */
  if (!(num >= 1)) {
    sprintf("%s: need num >= 1 (not %d)", me, num);
    biffAdd(NRRD, err); return 1;
  }
  if (num > NRRD_DIM_MAX) {
    sprintf("%s: can only deal with up to %d nrrds (not %d)", me,
	    NRRD_DIM_MAX, num);
    biffAdd(NRRD, err); return 1;
  }
  if (!( AIR_BETWEEN(nrrdTypeUnknown, type, nrrdTypeLast-1) )) {
    sprintf(err, "%s: invalid nrrd type %d", me, type);
    biffAdd(NRRD, err); return 1;
  }
  if (!(nout && nin && bin && min && max && clamp)) {
    sprintf("%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  for (d=0; d<=num-1; d++) {
    if (!nin[d]) {
      sprintf("%s: input nrrd[%d] NULL", me, d);
      biffAdd(NRRD, err); return 1;
    }
    if (!(bin[d] >= 1)) {
      sprintf("%s: need bin[%d] >= 1 (not %d)", me, d, bin[d]);
      biffAdd(NRRD, err); return 1;
    }
    if (!(AIR_EXISTS(min[d]) && AIR_EXISTS(max[d]))) {
      sprintf("%s: must have non-NaN values for all min and max", me);
      biffAdd(NRRD, err); return 1;
    }
    if (d && !nrrdSameSize(nin[0], nin[d], AIR_TRUE)) {
      sprintf("%s: nin[%d] size mismatch with nin[0]", me, d);
      biffAdd(NRRD, err); return 1;
    }
  }

  /* allocate output nrrd */
  size = 1;
  for (d=0; d<=num-1; d++) {
    size *= bin[d];
  }
  if (nrrdMaybeAlloc(nout, size, type, num)) {
    sprintf("%s: couldn't allocate multi-dimensional histogram", me);
    biffAdd(NRRD, err); return 1;
  }
  hadContent = 0;
  totalContentStrlen = 0;
  for (d=0; d<=num-1; d++) {
    nout->axis[d].size = bin[d];
    nout->axis[d].spacing = AIR_NAN;
    nout->axis[d].min = min[d];
    nout->axis[d].max = max[d];
    if (nin[d]->content) {
      hadContent = 1;
      totalContentStrlen += strlen(nin[d]->content);
      nout->axis[d].label = calloc(strlen("histo(,)")
				   + strlen(nin[d]->content)
				   + 11
				   + 1, sizeof(char));
      if (nout->axis[d].label) {
	sprintf(nout->axis[d].label, "histo(%s,%d)", nin[d]->content, bin[d]);
      }
      else {
	sprintf(err, "%s: couldn't allocate output label #%d", me, d);
	biffAdd(NRRD, err); return 1;
      }
    }
    else {
      nout->axis[d].label = airFree(nout->axis[d].label);
      totalContentStrlen += 2;
    }
  }

  /* the skinny */
  for (Iin=0; Iin<=nin[0]->num-1; Iin++) {
    skip = 0;
    for (d=0; d<=num-1; d++) {
      val = nrrdDLookup[nin[d]->type](nin[d]->data, Iin);
      if (!AIR_INSIDE(min[d], val, max[d])) {
	if (clamp[d]) {
	  val = AIR_CLAMP(min[d], val, max[d]);
	}
	else {
	  skip = 1;
	  break;
	}
      }
      AIR_INDEX(min[d], val, max[d], bin[d], coord[d]);
    }
    if (skip)
      continue;
    NRRD_COORD_INDEX(Iout, coord, bin, num, d);
    count = nrrdDLookup[nout->type](nout->data, Iout);
    count = nrrdDClamp[nout->type](count + 1);
    nrrdDInsert[nout->type](nout->data, Iout, count);
  }

  if (hadContent) {
    nout->content = calloc(strlen("histomulti()")
			   + num*strlen(",")
			   + totalContentStrlen
			   + 1, sizeof(char));
    if (nout->content) {
      sprintf(nout->content, "histomulti(");
      for (d=0; d<=num-1; d++) {
	len = strlen(nout->content);
	strcpy(nout->content + len,
	       nin[d]->content ? nin[d]->content : "?");
	len = strlen(nout->content);
	nout->content[len] = d < num-1 ? ',' : ')';
      }
      nout->content[len+1] = '\0';
    }
    else {
      sprintf(err, "%s: couldn't allocate output content", me);
      biffAdd(NRRD, err); return 1;
    }
  }

  return 0;
}

int
nrrdHistoDraw(Nrrd *nout, Nrrd *nin, int sy) {
  char err[NRRD_STRLEN_MED], me[] = "nrrdHistoDraw", cmt[NRRD_STRLEN_MED];
  int k, sx, x, y, maxhitidx,
    numticks, *Y, *logY, tick, *ticks;
  double hits, maxhits;
  unsigned char *pgmData;

  if (!(nin && nout && sy > 0)) {
    sprintf(err, "%s: invalid args", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(1 == nin->dim && nrrdTypeBlock != nin->type)) {
    sprintf(err, "%s: given nrrd can\'t be a histogram (dim %d, type %s)", 
	    me, nin->dim, nrrdEnumValToStr(nrrdEnumType, nin->type));
    biffAdd(NRRD, err); return 1;
  }
  sx = nin->axis[0].size;
  if (nrrdPGM(nout, sx, sy)) {
    sprintf(err, "%s: failed to allocate histogram image", me);
    biffAdd(NRRD, err); return 1;
  }
  nout->axis[0].spacing = nout->axis[1].spacing = AIR_NAN;
  nout->axis[0].min = nin->axis[0].min;
  nout->axis[0].max = nin->axis[0].max;
  nout->axis[1].min = nout->axis[1].max = AIR_NAN;
  nout->axis[0].center = nout->axis[1].center = nrrdCenterCell;
  nout->axis[0].label = airStrdup(nin->axis[0].label);
  nout->axis[1].label = airFree(nout->axis[1].label);
  pgmData = nout->data;
  maxhits = maxhitidx = 0;
  for (x=0; x<=sx-1; x++) {
    hits = nrrdDLookup[nin->type](nin->data, x);
    if (maxhits < hits) {
      maxhits = hits;
      maxhitidx = x;
    }
  }
  numticks = log10(maxhits + 1);
  ticks = (int*)calloc(numticks, sizeof(int));
  Y = (int*)calloc(sx, sizeof(int));
  logY = (int*)calloc(sx, sizeof(int));
  if (!(ticks && Y && logY)) {
    sprintf(err, "%s: failed to allocate temp arrays", me);
    biffAdd(NRRD, err); return 1;
  }
  for (k=0; k<=numticks-1; k++) {
    AIR_INDEX(0, log10(pow(10,k+1) + 1), log10(maxhits+1), sy, ticks[k]);
  }
  for (x=0; x<=sx-1; x++) {
    hits = nrrdDLookup[nin->type](nin->data, x);
    AIR_INDEX(0, hits, maxhits, sy, Y[x]);
    AIR_INDEX(0, log10(hits+1), log10(maxhits+1), sy, logY[x]);
    /* printf("%d -> %d,%d", x, Y[x], logY[x]); */
  }
  for (y=0; y<=sy-1; y++) {
    tick = 0;
    for (k=0; k<=numticks-1; k++)
      tick |= ticks[k] == y;
    for (x=0; x<=sx-1; x++) {
      pgmData[x + sx*(sy-1-y)] = 
	(y >= logY[x]       /* above log curve                       */
	 ? (!tick ? 0       /*                    not on tick mark   */
	    : 255)          /*                    on tick mark       */
	 : (y >= Y[x]       /* below log curve, above normal curve   */
	    ? (!tick ? 128  /*                    not on tick mark   */
	       : 0)         /*                    on tick mark       */
	    :255            /* below log curve, below normal curve */
	    )
	 );
    }
  }
  sprintf(cmt, "min value: %g\n", nout->axis[0].min);
  nrrdCommentAdd(nout, cmt, AIR_FALSE);
  sprintf(cmt, "max value: %g\n", nout->axis[0].max);
  nrrdCommentAdd(nout, cmt, AIR_FALSE);
  sprintf(cmt, "max hits: %lg, around value %lg\n", maxhits,
	  nrrdAxisPosition(nout, 0, maxhitidx));
  nrrdCommentAdd(nout, cmt, AIR_FALSE);

  /* bye */
  Y = airFree(Y);
  logY = airFree(logY);
  ticks = airFree(ticks);
  return 0;
}

