/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

tenEMBimodalParm* 
tenEMBimodalParmNew() {
  tenEMBimodalParm *biparm;
  
  biparm = (tenEMBimodalParm*)calloc(1, sizeof(tenEMBimodalParm));
  if (biparm) {
    biparm->minProb = 0.0001;
    biparm->minDelta = 0.000001;
    biparm->minFraction = 0.05;  /* 5% */
    biparm->minConfidence = 0.5;
    biparm->maxIterations = 3000;

    biparm->histo = NULL;
    biparm->vmin = biparm->vmax = AIR_NAN;
    biparm->N = 0;
  }
  return biparm;
}

tenEMBimodalParm* 
tenEMBimodalParmNix(tenEMBimodalParm *biparm) {

  if (biparm) {
    AIR_FREE(biparm->histo);
  }
  return NULL;
}

int
_tenEMBimodalInit(tenEMBimodalParm *biparm, Nrrd *_nhisto) {
  char me[]="_tenEMBimodalInit", err[AIR_STRLEN_MED];
  int median;
  Nrrd *nhisto;
  double medianD;

  if (!( biparm->maxIterations > 5 )) {
    sprintf(err, "%s: biparm->maxIterations = %d too small", me, 
	    biparm->maxIterations);
    biffAdd(TEN, err); return 1;
  }
  
  nhisto = nrrdNew();
  if (nrrdConvert(nhisto, _nhisto, nrrdTypeDouble)) {
    sprintf(err, "%s: trouble converting histogram to double", me);
    biffMove(TEN, err, NRRD); nrrdNuke(nhisto); return 1;
  }
  biparm->histo = (double*)(nhisto->data);
  biparm->N = nhisto->axis[0].size;
  biparm->vmin = (AIR_EXISTS(nhisto->axis[0].min)
		  ? nhisto->axis[0].min
		  : -0.5);
  biparm->vmax = (AIR_EXISTS(nhisto->axis[0].max)
		  ? nhisto->axis[0].max
		  : biparm->N - 0.5);
  nrrdNix(nhisto);

  (nrrdMeasureLine[nrrdMeasureHistoMedian])
    (&medianD, nrrdTypeDouble,
     biparm->histo, nrrdTypeDouble, biparm->N,
     AIR_NAN, AIR_NAN);
  if (!AIR_EXISTS(medianD)) {
    sprintf(err, "%s: got empty histogram? (median calculation failed)", me);
    biffMove(TEN, err, NRRD); return 1;
  }

  median = medianD;
  fprintf(stderr, "!%s: median = %d\n", me, median);

  /* get mean and stdv of bins below median */
  (nrrdMeasureLine[nrrdMeasureHistoMean])
    (&(biparm->mean1), nrrdTypeDouble, 
     biparm->histo, nrrdTypeDouble, median,
     AIR_NAN, AIR_NAN);
  (nrrdMeasureLine[nrrdMeasureHistoSD])
    (&(biparm->stdv1), nrrdTypeDouble, 
     biparm->histo, nrrdTypeDouble, median,
     AIR_NAN, AIR_NAN);

  /* get mean (shift upwards by median) and stdv of bins above median */
  (nrrdMeasureLine[nrrdMeasureHistoMean])
    (&(biparm->mean2), nrrdTypeDouble, 
     biparm->histo + median, nrrdTypeDouble, biparm->N - median,
     AIR_NAN, AIR_NAN);
  (nrrdMeasureLine[nrrdMeasureHistoSD])
    (&(biparm->stdv2), nrrdTypeDouble, 
     biparm->histo + median, nrrdTypeDouble, biparm->N - median,
     AIR_NAN, AIR_NAN);

  biparm->mean2 += median;
  biparm->fraction1 = 0.5;

  fprintf(stderr, "!%s: m1, s1 = %g, %g; m2, s2 = %g, %g\n", me,
	  biparm->mean1, biparm->stdv1,
	  biparm->mean2, biparm->stdv2);
  
  return 0;
}

/*
** what is posterior probability that measured value x comes from
** material 1 (*pp1P) or 2 (*pp2P)
*/
void
_tenEMBimodalProb(double *pp1P, double *pp2P, 
		  double x, tenEMBimodalParm *biparm) {
  double g1, g2, f1;
  
  g1 = airGaussian(x, biparm->mean1, biparm->stdv1);
  g2 = airGaussian(x, biparm->mean2, biparm->stdv2);
  if (g1 < biparm->minProb && g2 < biparm->minProb) {
    *pp1P = *pp2P = 0;
  } else {
    f1 = biparm->fraction1;
    *pp1P = (f1*g1) / (f1*g1 + (1-f1)*g2);
    *pp2P = 1 - *pp1P;
  }
  return;
}

double
_tenEMBimodalNewFraction1(tenEMBimodalParm *biparm) {
  int i;
  double pp1, pp2, h, s1, s2;

  s1 = s2 = 0.0;
  for (i=0; i<biparm->N; i++) {
    _tenEMBimodalProb(&pp1, &pp2, i, biparm);
    h = biparm->histo[i];
    s1 += i*pp1*h;
    s2 += i*pp2*h;
  }
  return s1/(s1 + s2);
}

void
_tenEMBimodalNewMean(double *m1P, double *m2P,
		     tenEMBimodalParm *biparm) {
  int i;
  double pp1, pp2, h, sum1, isum1, sum2, isum2;
  
  sum1 = isum1 = sum2 = isum2 = 0.0;
  for (i=0; i<biparm->N; i++) {
    _tenEMBimodalProb(&pp1, &pp2, i, biparm);
    h = biparm->histo[i];
    isum1 += i*pp1*h;
    isum2 += i*pp2*h;
    sum1 += pp1*h;
    sum2 += pp2*h;
  }
  *m1P = isum1/sum1;
  *m2P = isum2/sum2;
}

void
_tenEMBimodalNewSigma(double *s1P, double *s2P,
		      double m1, double m2, 
		      tenEMBimodalParm *biparm) {
  int i;
  double pp1, pp2, h, sum1, isum1, sum2, isum2;
  
  sum1 = isum1 = sum2 = isum2 = 0.0;
  for (i=0; i<biparm->N; i++) {
    _tenEMBimodalProb(&pp1, &pp2, i, biparm);
    h = biparm->histo[i];
    isum1 += (i-m1)*(1-m1)*pp1*h;
    isum2 += (i-m2)*(i-m2)*pp2*h;
    sum1 += pp1*h;
    sum2 += pp2*h;
  }
  *s1P = sqrt(isum1/sum1);
  *s2P = sqrt(isum2/sum2);
}

int
_tenEMBimodalIterate(tenEMBimodalParm *biparm) {
  double om1, os1, om2, os2, of1, m1, s1, m2, s2, f1;

  /* copy old values */
  om1 = biparm->mean1;
  os1 = biparm->stdv1;
  of1 = biparm->fraction1;
  om2 = biparm->mean2;
  os2 = biparm->stdv2;

  /* find new values, and calculate delta */
  f1 = _tenEMBimodalNewFraction1(biparm);
  _tenEMBimodalNewMean(&m1, &m2, biparm);
  _tenEMBimodalNewSigma(&s1, &s2, m1, m2, biparm);
  biparm->delta = ((fabs(m1 - om1) + fabs(m2 - om2)
		    + fabs(s1 - os1) + fabs(s2 - os2))/biparm->N
		   + fabs(f1 - of1));
  fprintf(stderr, "%s: m1, s1 = %g, %g, m2, s2 = %g, %g, f1 = %g\n", 
	  "_tenEMBimodalIterate", m1, s1, m2, s2, f1);
  
  /* set new values */
  biparm->mean1 = m1;
  biparm->stdv1 = s1;
  biparm->fraction1 = f1;
  biparm->mean2 = m2;
  biparm->stdv2 = s2;
  
  return 0;
}

int
_tenEMBimodalConfThresh(tenEMBimodalParm *biparm) {
  char me[]="_tenEMBimodalConfThresh", err[AIR_STRLEN_MED];
  double m1, s1, m2, s2, f1, f2, A, B, C, D, t1, t2;

  biparm->confidence = ((biparm->mean2 - biparm->mean1)
			/ (biparm->stdv1 + biparm->stdv2));
  m1 = biparm->mean1;
  s1 = biparm->stdv1;
  f1 = biparm->fraction1;
  m2 = biparm->mean2;
  s2 = biparm->stdv2;
  f2 = 1 - f1;
  A = s1*s1 - s2*s2;
  B = 2*(m1*s2*s2 - m2*s1*s1);
  C = s1*s1*m2*m2 - s2*s2*m1*m1 + 4*s1*s1*s2*s2*log(s2*f1/(s1*f2));
  D = B*B - 4*A*C;
  if (D < 0) {
    sprintf("%s: threshold descriminant went negative (%g)", me, D);
    biffAdd(TEN, err); return 1;
  }
  t1 = (-B + sqrt(D))/(2*A);
  if (AIR_IN_OP(m1, t1, m2)) {
    biparm->threshold = t1;
  } else {
    t2 = (-B - sqrt(D))/(2*A);
    if (AIR_IN_OP(m1, t2, m2)) {
      biparm->threshold = t2;
    } else {
      sprintf("%s: neither computed threshold %g,%g inside open interval "
	      "between means (%g,%g)", me, t1, t2, m1, m2);
      biffAdd(TEN, err); return 1;
      return 1;
    }
  } 
  return 0;
}

int
_tenEMBimodalCheck(tenEMBimodalParm *biparm) {
  char me[]="_tenEMBimodalCheck", err[AIR_STRLEN_MED];

  if (!( biparm->confidence > biparm->minConfidence )) {
    sprintf(err, "%s: confidence %g went below threshold %g", me,
	    biparm->confidence, biparm->minConfidence);
    biffAdd(TEN, err); return 1;
  }
  if (!( biparm->stdv1 > 0 && biparm->stdv2 > 0 )) {
    sprintf(err, "%s: stdv of material 1 (%g) or 2 (%g) went negative", me,
	    biparm->stdv1, biparm->stdv2);
    biffAdd(TEN, err); return 1;
  }
  if (!( biparm->mean1 > biparm->vmin && biparm->mean1 < biparm->vmax
	 && biparm->mean2 > biparm->vmin && biparm->mean2 < biparm->vmax )) {
    sprintf(err, "%s: mean of material 1 (%g) or 2 (%g) went outside "
	    "given histogram range [%g .. %g]", me,
	    biparm->mean1, biparm->mean2,
	    biparm->vmin, biparm->vmax);
    biffAdd(TEN, err); return 1;
  }
  if (biparm->fraction1 < biparm->minFraction) {
    sprintf(err, "%s: material 1 fraction (%g) fell below threshold %g", me,
	    biparm->fraction1, biparm->minFraction);
    biffAdd(TEN, err); return 1;
  }
  if (1 - biparm->fraction1 < biparm->minFraction) {
    sprintf(err, "%s: material 2 fraction (%g) fell below threshold %g", me,
	    1 - biparm->fraction1, biparm->minFraction);
    biffAdd(TEN, err); return 1;
  }
  return 0;
}

int
tenEMBimodal(tenEMBimodalParm *biparm, Nrrd *_nhisto) {
  char me[]="tenEMBimodal", err[AIR_STRLEN_MED];
  int done, iter;
  
  if (!(biparm && _nhisto)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( 1 == _nhisto->dim )) {
    sprintf(err, "%s: histogram must be 1-D, not %d-D", me, _nhisto->dim);
    biffAdd(TEN, err); return 1;
  }

  if (_tenEMBimodalInit(biparm, _nhisto)) {
    sprintf(err, "%s: trouble initializing parameters", me);
    biffAdd(TEN, err); return 1;
  }
  done = AIR_FALSE;
  for (iter=0; iter<=biparm->maxIterations; iter++) {
    
    if (_tenEMBimodalIterate(biparm)    /* sets delta */
	|| _tenEMBimodalConfThresh(biparm)
	|| _tenEMBimodalCheck(biparm)) {
      sprintf(err, "%s: problem with fitting (iter=%d)", me, iter);
      biffAdd(TEN, err); return 1;
    }
    if (biparm->delta < biparm->minDelta) {
      done = AIR_TRUE;
      break;
    }
  }
  if (!done) {
    sprintf(err, "%s: didn't converge after %d iterations", me, 
	    biparm->maxIterations);
    biffAdd(TEN, err); return 1;
  }

  return 0;
}
