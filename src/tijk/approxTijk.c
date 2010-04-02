/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2010, 2009, 2008 Thomas Schultz
  Copyright (C) 2010, 2009, 2008 Gordon Kindlmann

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "tijk.h"
#include "privateTijk.h"

/* Functions for symmetric tensor approximation */

/* a coarse sampling of the unit semicircle */
const unsigned int _tijk_max_candidates_2d=8;

#define _CANDIDATES_2D(TYPE, SUF)					\
  static TYPE _candidates_2d_##SUF[16] = {				\
    1.0, 0.0,								\
    0.92387953251128674, 0.38268343236508978,				\
    0.70710678118654757, 0.70710678118654746,				\
    0.38268343236508984, 0.92387953251128674,				\
    0.0, 1.0,								\
    -0.38268343236508973, 0.92387953251128674,				\
    -0.70710678118654746, 0.70710678118654757,				\
    -0.92387953251128674, 0.38268343236508989				\
  };
_CANDIDATES_2D(double, d)
_CANDIDATES_2D(float, f)

/* a coarse sampling of the unit sphere */
const unsigned int _tijk_max_candidates_3d=30;

#define _CANDIDATES_3D(TYPE, SUF)					\
  static TYPE _candidates_3d_##SUF[90] = {				\
    -0.546405, 0.619202, 0.563943,					\
    -0.398931,-0.600006, 0.693432,					\
    0.587973, 0.521686, 0.618168,					\
    0.055894,-0.991971,-0.113444,					\
    -0.666933,-0.677984, 0.309094,					\
    0.163684, 0.533013, 0.830123,					\
    0.542826, 0.133898, 0.829102,					\
    -0.074751,-0.350412, 0.933608,					\
    0.845751, -0.478624,-0.235847,					\
    0.767148,-0.610673, 0.196372,					\
    -0.283810, 0.381633, 0.879663,					\
    0.537228,-0.616249, 0.575868,					\
    -0.711387, 0.197778, 0.674398,					\
    0.886511, 0.219025, 0.407586,					\
    0.296061, 0.842985, 0.449136,					\
    -0.937540,-0.340990, 0.068877,					\
    0.398833, 0.917023, 0.000835,					\
    0.097278,-0.711949, 0.695460,					\
    -0.311534, 0.908623,-0.278121,					\
    -0.432043,-0.089758, 0.897375,					\
    -0.949980, 0.030810, 0.310788,					\
    0.146722,-0.811981,-0.564942,					\
    -0.172201,-0.908573, 0.380580,					\
    0.507209,-0.848578,-0.150513,					\
    -0.730808,-0.654136,-0.194999,					\
    0.077744, 0.094961, 0.992441,					\
    0.383976,-0.293959, 0.875300,					\
    0.788208,-0.213656, 0.577130,					\
    -0.752333,-0.301447, 0.585769,					\
    -0.975732, 0.165497,-0.143382					\
  };

_CANDIDATES_3D(double, d)
_CANDIDATES_3D(float, f)

#define _TIJK_INIT_RANK1(TYPE, SUF, DIM)				\
  int									\
  tijk_init_rank1_##DIM##d_##SUF(TYPE *s, TYPE *v, TYPE *ten,		\
				 const tijk_type *type) {		\
    TYPE absmax=-1;							\
    unsigned int i;							\
    TYPE *candidate=_candidates_##DIM##d_##SUF;				\
    if (type->dim!=DIM || type->sym==NULL)				\
      return 1;								\
    for (i=0; i<_tijk_max_candidates_##DIM##d; i++) {			\
      TYPE val=(*type->sym->s_form_##SUF)(ten, candidate);		\
      TYPE absval=fabs(val);						\
      if (absval>absmax) {						\
	absmax=absval;							\
	*s=val;								\
	ELL_##DIM##V_COPY(v, candidate);				\
      }									\
      candidate+=DIM;							\
    }									\
    return 0;								\
  }

_TIJK_INIT_RANK1(double, d, 2)
_TIJK_INIT_RANK1(float, f, 2)
_TIJK_INIT_RANK1(double, d, 3)
_TIJK_INIT_RANK1(float, f, 3)

#define _TIJK_INIT_MAX(TYPE, SUF, DIM)					\
  int									\
  tijk_init_max_##DIM##d_##SUF(TYPE *s, TYPE *v, TYPE *ten,		\
			       const tijk_type *type) {			\
    TYPE max=0;								\
    unsigned int i;							\
    TYPE *candidate=_candidates_##DIM##d_##SUF;				\
    if (type->dim!=DIM || type->sym==NULL)				\
      return 1;								\
    *s=max=(*type->sym->s_form_##SUF)(ten, candidate);			\
    ELL_##DIM##V_COPY(v, candidate);					\
    for (i=1; i<_tijk_max_candidates_##DIM##d; i++) {			\
      TYPE val;								\
      candidate+=DIM;							\
      val=(*type->sym->s_form_##SUF)(ten, candidate);			\
      if (val>max) {							\
	max=val;							\
	*s=val;								\
	ELL_##DIM##V_COPY(v, candidate);				\
      }									\
    }									\
    return 0;								\
  }

_TIJK_INIT_MAX(double, d, 2)
_TIJK_INIT_MAX(float, f, 2)
_TIJK_INIT_MAX(double, d, 3)
_TIJK_INIT_MAX(float, f, 3)

#define _TIJK_REFINE_RANK1(TYPE, SUF, DIM)				\
  int									\
  tijk_refine_rank1_##DIM##d_##SUF(TYPE *s, TYPE *v, TYPE *ten,		\
				   const tijk_type *type) {		\
    TYPE isoten[TIJK_TYPE_MAX_NUM], tmpten[TIJK_TYPE_MAX_NUM];		\
    TYPE der[DIM], iso, anisonorm, anisonorminv, oldval=*s;		\
    char sign=(*s>0)?1:-1;						\
    if (type->dim!=DIM || type->sym==NULL)				\
      return 1;								\
    /* determine anisonorminv */					\
    iso=(*type->sym->mean_##SUF)(ten);					\
    (*type->sym->make_iso_##SUF)(isoten, iso);				\
    tijk_sub_##SUF(tmpten, ten, isoten, type);				\
    anisonorm=(*type->norm_##SUF)(tmpten);				\
    if (anisonorm<TIJK_EPS) {						\
      return 0; /* nothing to do */					\
    } else {								\
      anisonorminv=1.0/anisonorm;					\
    }									\
    /* set initial derivative */					\
    (*type->sym->grad_##SUF)(der, ten, v);				\
    while (1) { /* refine until convergence */				\
      TYPE beta=anisonorminv, gamma=0.9, sigma=0.5;			\
      TYPE alpha=sign*beta;						\
      unsigned int armijoct=0;						\
      TYPE testv[DIM], val;						\
      TYPE dist, derlen=ELL_##DIM##V_LEN(der);				\
      /* determine stepsize based on Armijo's rule */			\
      while (1) {							\
	++armijoct;							\
	if (armijoct>50) {						\
	  /* failed to find a valid stepsize */				\
	  return 2;							\
	}								\
	ELL_##DIM##V_SCALE_ADD2(testv,1.0,v,alpha,der);			\
	ELL_##DIM##V_NORM(testv,testv,dist);				\
	dist=1-ELL_##DIM##V_DOT(v,testv);				\
	val=(*type->sym->s_form_##SUF)(ten,testv);			\
	if (sign*val>=sign*oldval+sigma*derlen*dist) {			\
	  /* accept step */						\
	  ELL_##DIM##V_COPY(v,testv);					\
	  *s=val;							\
	  (*type->sym->grad_##SUF)(der, ten, v);			\
	  break;							\
	}								\
	alpha *= gamma; /* try again with decreased stepsize */		\
      }									\
      if (sign*(val-oldval)<=1e-4*anisonorm) {				\
	break; /* declare convergence */				\
      }									\
      oldval=val;							\
    }									\
    return 0;								\
  }

_TIJK_REFINE_RANK1(double, d, 2)
_TIJK_REFINE_RANK1(float, f, 2)
_TIJK_REFINE_RANK1(double, d, 3)
_TIJK_REFINE_RANK1(float, f, 3)

#define _TIJK_APPROX_RANKK(TYPE, SUF, DIM)				\
  int									\
  tijk_approx_rankk_##DIM##d_##SUF(TYPE *ls, TYPE *vs, TYPE *res,	\
				   TYPE *ten, const tijk_type *type,	\
				   unsigned int k, TYPE rankthresh,	\
				   TYPE *ratios)			\
  {									\
    TYPE *lstmp=NULL, *vstmp=NULL, *tens=NULL, *restmp=NULL;		\
    TYPE oldnorm, newnorm;						\
    unsigned int currank=1;						\
    if (type->dim!=DIM || type->sym==NULL)				\
      return 0;								\
    /* initializations */						\
    newnorm=oldnorm=(*type->norm_##SUF)(ten);				\
    if (oldnorm<TIJK_EPS || k==0) {					\
      if (res!=NULL) memcpy(res,ten,sizeof(TYPE)*type->num);		\
      return 0; /* nothing to do */					\
    }									\
    lstmp=AIR_CALLOC(k,TYPE);						\
    if (lstmp==NULL) goto cleanup_and_exit;				\
    vstmp=AIR_CALLOC(DIM*k,TYPE);					\
    if (vstmp==NULL) goto cleanup_and_exit;				\
    tens=AIR_CALLOC(k*type->num,TYPE);					\
    if (tens==NULL) goto cleanup_and_exit;				\
    restmp=AIR_CALLOC(type->num,TYPE);					\
    if (restmp==NULL) goto cleanup_and_exit;				\
    memcpy(restmp, ten, sizeof(TYPE)*type->num);			\
    for (currank=1; currank<=k; currank++) {				\
      int accept=1;							\
      tijk_refine_rankk_##DIM##d_##SUF(lstmp, vstmp, tens, restmp, &newnorm, \
				       type, currank);			\
      if (currank>1 && ratios!=NULL) {					\
	/* make sure the desired ratio is fulfilled */			\
	TYPE largest=fabs(lstmp[0]), smallest=largest;			\
	unsigned int i;							\
	for (i=1; i<currank; i++) {					\
	  if (fabs(lstmp[i])>largest) largest=fabs(lstmp[i]);		\
	  if (fabs(lstmp[i])<smallest) smallest=fabs(lstmp[i]);		\
	}								\
	if (largest/smallest>ratios[currank-2])				\
	  accept=0;							\
      }									\
      if (accept && newnorm<rankthresh*oldnorm) { /* copy over */	\
	memcpy(vs, vstmp, sizeof(TYPE)*DIM*currank);			\
	memcpy(ls, lstmp, sizeof(TYPE)*currank);			\
	if (res!=NULL)							\
	  memcpy(res, restmp, sizeof(TYPE)*type->num);			\
	oldnorm=newnorm;						\
      } else {								\
	break;								\
      }									\
    }									\
cleanup_and_exit:							\
 if (lstmp!=NULL) free(lstmp);						\
 if (vstmp!=NULL) free(vstmp);						\
 if (tens!=NULL) free(tens);						\
 if (restmp!=NULL) free(restmp);					\
 return currank-1;							\
  }

_TIJK_APPROX_RANKK(double, d, 2)
_TIJK_APPROX_RANKK(float, f, 2)
_TIJK_APPROX_RANKK(double, d, 3)
_TIJK_APPROX_RANKK(float, f, 3)

#define _TIJK_REFINE_RANKK(TYPE, SUF, DIM)				\
  int									\
  tijk_refine_rankk_##DIM##d_##SUF(TYPE *ls, TYPE *vs, TYPE *tens,	\
				   TYPE *res, TYPE *resnorm,		\
				   const tijk_type *type, unsigned int k) \
  {									\
    TYPE newnorm=(*resnorm);						\
    unsigned int i;							\
    if (type->dim!=DIM || type->sym==NULL)				\
      return 1;								\
    if (*resnorm<TIJK_EPS || k==0) {					\
      return 0; /* nothing to do */					\
    }									\
    do {								\
      *resnorm=newnorm;							\
      for (i=0; i<k; i++) {						\
	if (ls[i]!=0.0) { /* refine an existing term */			\
	  tijk_incr_##SUF(res, tens+i*type->num, type);			\
	  ls[i]=(*type->sym->s_form_##SUF)(res, vs+DIM*i);		\
	} else { /* add a new term */					\
	  tijk_init_rank1_##DIM##d_##SUF(ls+i, vs+DIM*i, res, type);	\
	}								\
	tijk_refine_rank1_##DIM##d_##SUF(ls+i, vs+DIM*i, res, type);	\
	(*type->sym->make_rank1_##SUF)(tens+i*type->num, ls[i], vs+DIM*i); \
	tijk_sub_##SUF(res, res, tens+i*type->num, type);		\
      }									\
      newnorm=(*type->norm_##SUF)(res);					\
    } while (newnorm<0.9999*(*resnorm));				\
    return 0;								\
  }

_TIJK_REFINE_RANKK(double, d, 2)
_TIJK_REFINE_RANKK(float, f, 2)
_TIJK_REFINE_RANKK(double, d, 3)
_TIJK_REFINE_RANKK(float, f, 3)

#define _TIJK_REFINE_POS_RANKK(TYPE, SUF, DIM)				\
  int									\
  tijk_refine_pos_rankk_##DIM##d_##SUF(TYPE *ls, TYPE *vs, TYPE *tens,	\
				       TYPE *res, TYPE *resnorm,	\
				       const tijk_type *type, unsigned int k) \
  {									\
    TYPE newnorm=(*resnorm);						\
    unsigned int i;							\
    if (type->dim!=DIM || type->sym==NULL)				\
      return 1;								\
    if (*resnorm<TIJK_EPS || k==0) {					\
      return 0; /* nothing to do */					\
    }									\
    do {								\
      *resnorm=newnorm;							\
      for (i=0; i<k; i++) {						\
	if (ls[i]!=0.0) { /* refine an existing term */			\
	  tijk_incr_##SUF(res, tens+i*type->num, type);			\
	  ls[i]=(*type->sym->s_form_##SUF)(res, vs+DIM*i);		\
	  if (ls[i]<0.0) { /* try a new one */				\
	    tijk_init_max_##DIM##d_##SUF(ls+i, vs+DIM*i, res, type);	\
	  }								\
	} else { /* add a new term */					\
	  tijk_init_max_##DIM##d_##SUF(ls+i, vs+DIM*i, res, type);	\
	}								\
	tijk_refine_rank1_##DIM##d_##SUF(ls+i, vs+DIM*i, res, type);	\
	if (ls[i]>0.0) {						\
	  (*type->sym->make_rank1_##SUF)(tens+i*type->num, ls[i], vs+DIM*i); \
	  tijk_sub_##SUF(res, res, tens+i*type->num, type);		\
	} else {							\
	  ls[i]=0.0;							\
	}								\
      }									\
      newnorm=(*type->norm_##SUF)(res);					\
    } while (newnorm<0.9999*(*resnorm));				\
    return 0;								\
  }

_TIJK_REFINE_POS_RANKK(double, d, 2)
_TIJK_REFINE_POS_RANKK(float, f, 2)
_TIJK_REFINE_POS_RANKK(double, d, 3)
_TIJK_REFINE_POS_RANKK(float, f, 3)
