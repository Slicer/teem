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
tenEstimationMatrix(Nrrd *nemat, Nrrd *ngrad) {
  char me[]="tenEstimationMatrix", err[AIR_STRLEN_MED];
  Nrrd *nrow;
  int NN, nn;
  double *row, G[3], len;
  
  if (!(nemat && ngrad)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!(2 == ngrad->dim && 3 == ngrad->axis[0].size &&
	(nrrdTypeFloat == ngrad->type || nrrdTypeDouble == ngrad->type))) {
    sprintf(err, "%s: given nrrd isn't 3-by-N array of floats or doubles", me);
    biffAdd(TEN, err); return 1;
  }
  if (!(ngrad->axis[1].size >= 6)) {
    sprintf(err, "%s: can't estimate tensors w/ %d gradients, need at least 6",
	    me, ngrad->axis[1].size);
    biffAdd(TEN, err); return 1;
  }
  if (nrrdMaybeAlloc(nrow = nrrdNew(), nrrdTypeDouble, 2,
		     6, ngrad->axis[1].size)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, NRRD); return 1;
  }

  NN = ngrad->axis[1].size;
  row = (double*)(nrow->data);
  for (nn=0; nn<NN; nn++) {
    ELL_3V_SET(G,
	       nrrdDLookup[ngrad->type](ngrad->data, 0 + 3*nn),
	       nrrdDLookup[ngrad->type](ngrad->data, 1 + 3*nn),
	       nrrdDLookup[ngrad->type](ngrad->data, 2 + 3*nn));
    ELL_3V_NORM(G, G, len);
    if (!len) {
      sprintf(err, "%s: gradient[%d] has zero length", me, nn);
      biffAdd(TEN, err); return 1;
    }
    ELL_6V_SET(row,
	       G[0]*G[0], 2*G[0]*G[1], 2*G[0]*G[2],
	       G[1]*G[1], 2*G[1]*G[2],
	       G[2]*G[2]);
    row += 6;
  }
  if (ellNmPseudoInverse(nemat, nrow)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, ELL); nrrdNuke(nrow); return 1;
  }
  nrrdNuke(nrow);
  return 0;
}

/*
******** tenCalcOneTensor1
**
** make one diffusion tensor from the measured at one voxel, based
** on the gradient directions used by Andy Alexander
*/
void
tenCalcOneTensor1(float tens[7], float chan[7], 
		  float thresh, float slope, float b) {
  double c[7], sum, d1, d2, d3, d4, d5, d6;
  
  c[0] = AIR_MAX(chan[0], 1);
  c[1] = AIR_MAX(chan[1], 1);
  c[2] = AIR_MAX(chan[2], 1);
  c[3] = AIR_MAX(chan[3], 1);
  c[4] = AIR_MAX(chan[4], 1);
  c[5] = AIR_MAX(chan[5], 1);
  c[6] = AIR_MAX(chan[6], 1);
  sum = c[1] + c[2] + c[3] + c[4] + c[5] + c[6];
  tens[0] = (1 + airErf(slope*(sum - thresh)))/2.0;
  d1 = (log(c[0]) - log(c[1]))/b;
  d2 = (log(c[0]) - log(c[2]))/b;
  d3 = (log(c[0]) - log(c[3]))/b;
  d4 = (log(c[0]) - log(c[4]))/b;
  d5 = (log(c[0]) - log(c[5]))/b;
  d6 = (log(c[0]) - log(c[6]))/b;
  tens[1] =  d1 + d2 - d3 - d4 + d5 + d6;    /* Dxx */
  tens[2] =  d5 - d6;                        /* Dxy */
  tens[3] =  d1 - d2;                        /* Dxz */
  tens[4] = -d1 - d2 + d3 + d4 + d5 + d6;    /* Dyy */
  tens[5] =  d3 - d4;                        /* Dyz */
  tens[6] =  d1 + d2 + d3 + d4 - d5 - d6;    /* Dzz */
  return;
}

/*
******** tenCalcOneTensor2
**
** using gradient directions used by EK
*/
void
tenCalcOneTensor2(float tens[7], float chan[7], 
		  float thresh, float slope, float b) {
  double c[7], sum, d1, d2, d3, d4, d5, d6;
  
  c[0] = AIR_MAX(chan[0], 1);
  c[1] = AIR_MAX(chan[1], 1);
  c[2] = AIR_MAX(chan[2], 1);
  c[3] = AIR_MAX(chan[3], 1);
  c[4] = AIR_MAX(chan[4], 1);
  c[5] = AIR_MAX(chan[5], 1);
  c[6] = AIR_MAX(chan[6], 1);
  sum = c[1] + c[2] + c[3] + c[4] + c[5] + c[6];
  tens[0] = (1 + airErf(slope*(sum - thresh)))/2.0;
  d1 = (log(c[0]) - log(c[1]))/b;
  d2 = (log(c[0]) - log(c[2]))/b;
  d3 = (log(c[0]) - log(c[3]))/b;
  d4 = (log(c[0]) - log(c[4]))/b;
  d5 = (log(c[0]) - log(c[5]))/b;
  d6 = (log(c[0]) - log(c[6]))/b;
  tens[1] =  d1;                 /* Dxx */
  tens[2] =  d6 - (d1 + d2)/2;   /* Dxy */
  tens[3] =  d5 - (d1 + d3)/2;   /* Dxz */
  tens[4] =  d2;                 /* Dyy */
  tens[5] =  d4 - (d2 + d3)/2;   /* Dyz */
  tens[6] =  d3;                 /* Dzz */
  return;
}

/*
******** tenCalcTensor
**
** Calculate a volume of tensors from measured data
*/
int
tenCalcTensor(Nrrd *nout, Nrrd *nin, int version,
	      float thresh, float slope, float b) {
  char me[] = "tenCalcTensor", err[128], cmt[128];
  float *out, tens[6], chan[7];
  int sx, sy, sz;
  size_t I;
  void (*calcten)(float tens[7], float chan[7], 
		  float thresh, float slope, float b);
  
  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( 1 == version || 2 == version )) {
    sprintf(err, "%s: version should be 1 or 2, not %d", me, version);
    biffAdd(TEN, err); return 1;
  }
  switch (version) {
  case 1:
    calcten = tenCalcOneTensor1;
    break;
  case 2:
    calcten = tenCalcOneTensor2;
    break;
  default:
    sprintf(err, "%s: PANIC, version = %d not handled", me, version);
    biffAdd(TEN, err); return 1;
    break;
  }
  if (tenTensorCheck(nin, nrrdTypeUnknown, AIR_TRUE)) {
    sprintf(err, "%s: wasn't given valid tensor nrrd", me);
    biffAdd(TEN, err); return 1;
  }
  sx = nin->axis[1].size;
  sy = nin->axis[2].size;
  sz = nin->axis[3].size;
  if (nrrdMaybeAlloc(nout, nrrdTypeFloat, 4, 7, sx, sy, sz)) {
    sprintf(err, "%s: couldn't alloc output", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  nout->axis[0].label = airStrdup("c,Dxx,Dxy,Dxz,Dyy,Dyz,Dzz");
  nout->axis[1].label = airStrdup("x");
  nout->axis[2].label = airStrdup("y");
  nout->axis[3].label = airStrdup("z");
  nout->axis[0].spacing = AIR_NAN;
  if (AIR_EXISTS(nin->axis[1].spacing) && 
      AIR_EXISTS(nin->axis[2].spacing) &&
      AIR_EXISTS(nin->axis[3].spacing)) {
    nout->axis[1].spacing = nin->axis[1].spacing;
    nout->axis[2].spacing = nin->axis[2].spacing;
    nout->axis[3].spacing = nin->axis[3].spacing;
  }
  else {
    nout->axis[1].spacing = 1.0;
    nout->axis[2].spacing = 1.0;
    nout->axis[3].spacing = 1.0;
  }    
  sprintf(cmt, "%s: using thresh = %g, slope = %g, b = %g\n", 
	  me, thresh, slope, b);
  nrrdCommentAdd(nout, cmt);
  out = nout->data;
  for (I=0; I<=sx*sy*sz-1; I++) {
    if (tenVerbose && !(I % (sx*sy)))
      fprintf(stderr, "%s: z = %d of %d\n", me, ((int)I)/(sx*sy), sz-1);
    chan[0] = nrrdFLookup[nin->type](nin->data, 0 + 7*I);
    chan[1] = nrrdFLookup[nin->type](nin->data, 1 + 7*I);
    chan[2] = nrrdFLookup[nin->type](nin->data, 2 + 7*I);
    chan[3] = nrrdFLookup[nin->type](nin->data, 3 + 7*I);
    chan[4] = nrrdFLookup[nin->type](nin->data, 4 + 7*I);
    chan[5] = nrrdFLookup[nin->type](nin->data, 5 + 7*I);
    chan[6] = nrrdFLookup[nin->type](nin->data, 6 + 7*I);
    calcten(tens, chan, thresh, slope, b);
    out[0 + 7*I] = tens[0];
    out[1 + 7*I] = tens[1];
    out[2 + 7*I] = tens[2];
    out[3 + 7*I] = tens[3];
    out[4 + 7*I] = tens[4];
    out[5 + 7*I] = tens[5];
    out[6 + 7*I] = tens[6];
  }
  return 0;
}

#define _TEN_MAX_DWI_NUM 128

/*
******** tenEstimateOne
**
** estimate one tensor
**
** output:
** ten[0]..ten[6] will be the confidence value followed by the tensor
**
** input:
** dwi[0] is the B0 image, dwi[1]..dwi[NN] are the DW image values
** emat is the (NN-1)-by-6 estimation matrix
*/
void
tenEstimateOne(float *ten, float *dwi, float *emat, int NN,
	       float thresh, float soft, float b) {
  double v0, v[_TEN_MAX_DWI_NUM], mean;
  int i, j;

  v0 = log(AIR_MAX(dwi[0], 1));
  mean = 0;
  for (i=1; i<=NN; i++) {
    mean += (v[i-1] = AIR_MAX(dwi[i], 1));
    v[i-1] = (v0 - log(v[i-1]))/b;
  }
  mean /= NN;
  ten[0] = AIR_AFFINE(-1, airErf((mean - thresh)/(soft + 0.000001)), 1, 0, 1);
  for (j=1; j<=6; j++) {
    ten[j] = 0;
    for (i=0; i<NN; i++) {
      ten[j] += emat[i + (NN-1)*(j-1)]*v[i];
    }
  }
  return;
}

/*
******** tenEstimate
**
** given a stack of DWI volumes (ndwi) and the estimation matrix for computing
** the diffusion tensor elements (nemat), compute and store diffusion tensors
** into nten.
**
** The mean of the diffusion-weighted images is thresholded at "thresh" with
** softness parameter "soft".
*/
int
tenEstimate(Nrrd *nten, Nrrd *_ndwi, Nrrd *_nemat,
	    float thresh, float soft, float b) {
  char me[]="tenEstimate", err[AIR_STRLEN_MED];
  Nrrd *nemat, *ndwi;
  airArray *mop;
  int NN, II, sx, sy, sz;
  float *emat, *ten, *dwi;

  if (!(nten && _ndwi && _nemat)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( 4 == _ndwi->dim && 7 <= _ndwi->axis[0].size )) {
    sprintf(err, "%s: dwi should be 4-D array with axis 0 size >= 7", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( 2 == _nemat->dim && 6 == _nemat->axis[1].size )) {
    sprintf(err, "%s: emat should be a N-by-6 matrix (2-D array)", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( _ndwi->axis[0].size == 1 + _nemat->axis[0].size )) {
    sprintf(err, "%s: # dwi (%d) != 1 + # columns in estimation matrix (1+%d)",
	    me, _ndwi->axis[0].size, _nemat->axis[0].size);
    biffAdd(TEN, err); return 1;
  }

  mop = airMopNew();
  nemat = nrrdNew();
  airMopAdd(mop, nemat, (airMopper)nrrdNuke, airMopAlways);
  ndwi = nrrdNew();
  airMopAdd(mop, ndwi, (airMopper)nrrdNuke, airMopAlways);

  NN = _ndwi->axis[0].size;
  sx = _ndwi->axis[1].size;
  sy = _ndwi->axis[2].size;
  sz = _ndwi->axis[3].size;
  if (nrrdConvert(nemat, _nemat, nrrdTypeFloat)
      || nrrdConvert(ndwi, _ndwi, nrrdTypeFloat)
      || nrrdMaybeAlloc(nten, nrrdTypeFloat, 4, 7, sx, sy, sz)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  emat = (float*)(nemat->data);
  ten = (float*)(nten->data);
  dwi = (float*)(ndwi->data);
  for (II=0; II<sx*sy*sz; II++) {
    tenEstimateOne(ten, dwi, emat, NN, thresh, soft, b);
    ten += 7;
    dwi += NN;
  }

  airMopOkay(mop);
  return 0;
}
