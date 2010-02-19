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

#ifndef TIJK_HAS_BEEN_INCLUDED
#define TIJK_HAS_BEEN_INCLUDED

#include <teem/ell.h>

#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(TEEM_STATIC)
#  if defined(TEEM_BUILD) || defined(tijk_EXPORTS) || defined(teem_EXPORTS)
#    define TIJK_EXPORT extern __declspec(dllexport)
#  else
#    define TIJK_EXPORT extern __declspec(dllimport)
#  endif
#else /* TEEM_STATIC || UNIX */
#  define TIJK_EXPORT extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tijk_sym_fun_t {
  /* Functions that are specific to completely symmetric tensors. */
  /* homogeneous scalar form */
  double (*s_form_d) (const double *A, const double *v);
  float  (*s_form_f) (const float  *A, const float  *v);
  /* mean value of scalar form */
  double (*mean_d) (const double *A);
  float  (*mean_f) (const float  *A);
  /* variance of scalar form */
  double (*var_d) (const double *A);
  float  (*var_f) (const float  *A);
  /* vector- and matrix-valued forms, proportional to gradient and
   * Hessian of scalar homogeneous forms */
  void (*v_form_d) (double *res, const double *A, const double *v);
  void (*v_form_f) (float  *res, const float  *A, const float  *v);
  /* returns a symmetric matrix (in non-redundant representation) */
  void (*m_form_d) (double *res, const double *A, const double *v);
  void (*m_form_f) (float  *res, const float  *A, const float  *v);
  /* gradient of the homogeneous form when restricted to the unit
   * hypersphere; assumes (but does not verify) that v is unit-length */
  void (*grad_d) (double *res, const double *A, const double *v);
  void (*grad_f) (float *res, const float *A, const float *v);
  /* Hessian of the homogeneous form when restricted to the unit
   * hypersphere; assumes (but does not verify) that v is unit-length */
  void (*hess_d) (double *res, const double *A, const double *v);
  void (*hess_f) (float *res, const float *A, const float *v);
  /* make a symmetric rank-1 tensor from the given scalar and vector */
  void (*make_rank1_d) (double *res, const double s, const double *v);
  void (*make_rank1_f) (float *res, const float s, const float *v);
  /* make a symmetric isotropic tensor from the given scalar */
  void (*make_iso_d) (double *res, const double s);
  void (*make_iso_f) (float *res, const float s);
} tijk_sym_fun;

typedef struct tijk_type_t {
  /* Holds information about (and functions needed for processing) a
   * specific type of tensor */
  const char *name; /* description of the tensor type */
  unsigned int order; /* number of tensor indices */
  unsigned int dim; /* dimension of each axis (only square tensors supported) */
  unsigned int num; /* unique number of components */
#define TIJK_TYPE_MAX_NUM 29
  const unsigned int *mult; /* multiplicity of each unique component;
			     * NULL indicates that tensor is unsymmetric */

  /* The following fields are used to map the elements of an
   * unsymmetric tensor to the unique elements of the symmetric one.
   * A value i in these arrays means:
   * i==0: element is unmapped (due to antisymmetry)
   * i>0 : element maps to index (i-1)
   * i<0 : element maps to index -(i+1), with negated sign
   * They are NULL if the tensor is unsymmetric or the order is so
   * high that the unsymmetric variant is not even implemented. */
  const int *unsym2uniq; /* unsymmetric to unique; length: pow(dim,order) */
  const int *uniq2unsym; /* unique to unsymmetric; length: sum(mult) */
  const unsigned int *uniq_idx; /* index into uniq2unsym for each
				 * unique component */
  
  /* tensor scalar product */
  double (*tsp_d) (const double *A, const double *B);
  float  (*tsp_f) (const float  *A, const float  *B);
  /* norm */
  double (*norm_d) (const double *A);
  float  (*norm_f) (const float  *A);
  /* transformation under change of basis;
   * applies the same matrix M to all tensor modes
   * assumes that res!=A */
  void (*trans_d) (double *res, const double *A, const double *M);
  void (*trans_f) (float  *res, const float  *A, const float  *M);
  /* converts to a different tensor type. Supported conversions are:
   * - exact same type (identity conversion)
   * - same order & dim, (partially) symmetric -> unsymmetric
   * - same dim & completely symmetric; lower -> higher order
   *   (preserves the homogeneous form)
   * res needs to have length res_type->num
   * Returns a non-zero value if requested conversion is not available.
   */
  int (*convert_d) (double *res, const struct tijk_type_t *res_type,
		    const double *A);
  int (*convert_f) (float  *res, const struct tijk_type_t *res_type,
		    const float  *A);
  /* approximates a tensor with one of the given target type.
   * Supported approximations are:
   * - same order & dim, unsymmetric -> (partially) symmetric
   *   (minimizes the norm of the residual)
   * - same dim & completely symmetric; higher -> lower order
   *   (preserves the frequencies that can be expressed in the low order)
   * res needs to have length res_type->num
   * Returns a non-zero value if requested approximation is not implemented.
   */
  int (*approx_d) (double *res, const struct tijk_type_t *res_type,
		   const double *A);
  int (*approx_f) (float *res, const struct tijk_type_t *res_type,
		   const float *A);

  /* convert/approximate from a different tensor type.
   * This should not be called in user code (instead, use convert/approx).
   * Needed if libraries other than ell want to define new tensor types.
   */
  int (*_convert_from_d) (double *res, const double *A,
			  const struct tijk_type_t *from_type);
  int (*_convert_from_f) (float *res, const float *A,
			  const struct tijk_type_t *from_type);
  int (*_approx_from_d) (double *res, const double *A,
			 const struct tijk_type_t *from_type);
  int (*_approx_from_f) (float *res, const float *A,
			 const struct tijk_type_t *from_type);
  /* sym holds additional functions which are only useful for
   * completely symmetric tensors. In other cases, sym==NULL */
  const tijk_sym_fun *sym;
} tijk_type;

/* 2dTijk.c */
TIJK_EXPORT const tijk_type *const tijk_2o2d_unsym;
TIJK_EXPORT const tijk_type *const tijk_2o2d_sym;
TIJK_EXPORT const tijk_type *const tijk_2o2d_asym;
TIJK_EXPORT const tijk_type *const tijk_4o2d_unsym;
TIJK_EXPORT const tijk_type *const tijk_4o2d_sym;

/* 3dTijk.c */
TIJK_EXPORT const tijk_type *const tijk_2o3d_unsym;
TIJK_EXPORT const tijk_type *const tijk_2o3d_sym;
TIJK_EXPORT const tijk_type *const tijk_2o3d_asym;
TIJK_EXPORT const tijk_type *const tijk_4o3d_sym;
TIJK_EXPORT const tijk_type *const tijk_6o3d_sym;

/* miscTijk.c */
TIJK_EXPORT void tijk_add_d(double *res, const double *A,
			    const double *B, const tijk_type *type);
TIJK_EXPORT void tijk_add_f(float *res, const float *A,
			    const float *B, const tijk_type *type);

TIJK_EXPORT void tijk_sub_d(double *res, const double *A,
				  const double *B, const tijk_type *type);
TIJK_EXPORT void tijk_sub_f(float *res, const float *A,
				  const float *B, const tijk_type *type);

TIJK_EXPORT void tijk_incr_d(double *res, const double *A,
				   const tijk_type *type);
TIJK_EXPORT void tijk_incr_f(float *res, const float *A,
				   const tijk_type *type);

TIJK_EXPORT void tijk_negate_d(double *res, const double *A,
				     const tijk_type *type);
TIJK_EXPORT void tijk_negate_f(float *res, const float *A,
				     const tijk_type *type);

TIJK_EXPORT void tijk_zero_d(double *res, const tijk_type *type);
TIJK_EXPORT void tijk_zero_f(float *res, const tijk_type *type);

TIJK_EXPORT void tijk_copy_d(double *res, const double *A,
			       const tijk_type *type);
TIJK_EXPORT void tijk_copy_f(float *res, const float *A,
			       const tijk_type *type);

/* approxTijk.c */
TIJK_EXPORT int tijk_init_rank1_2d_d(double *s, double *v, double *ten,
				     const tijk_type *type);
TIJK_EXPORT int tijk_init_rank1_2d_f(float *s, float *v, float *ten,
				     const tijk_type *type);

TIJK_EXPORT int tijk_init_rank1_3d_d(double *s, double *v, double *ten,
				     const tijk_type *type);
TIJK_EXPORT int tijk_init_rank1_3d_f(float *s, float *v, float *ten,
				     const tijk_type *type);

/* For ANSI C compatibility, these routines rely on
 * type->num<=TIJK_TYPE_MAX_NUM !*/
TIJK_EXPORT int tijk_refine_rank1_2d_d(double *s, double *v, double *ten,
				       const tijk_type *type);
TIJK_EXPORT int tijk_refine_rank1_2d_f(float *s, float *v, float *ten,
				       const tijk_type *type);
TIJK_EXPORT int tijk_refine_rank1_3d_d(double *s, double *v, double *ten,
				       const tijk_type *type);
TIJK_EXPORT int tijk_refine_rank1_3d_f(float *s, float *v, float *ten,
				       const tijk_type *type);

TIJK_EXPORT int tijk_approx_rankk_2d_d(double *ls, double *vs, double *res,
				       double *ten, const tijk_type *type,
				       unsigned int k, double rankthresh,
				       double *ratios);
TIJK_EXPORT int tijk_approx_rankk_2d_f(float *ls, float *vs, float *res,
				       float *ten, const tijk_type *type,
				       unsigned int k, float rankthresh,
				       float *ratios);
TIJK_EXPORT int tijk_approx_rankk_3d_d(double *ls, double *vs, double *res,
				       double *ten, const tijk_type *type,
				       unsigned int k, double rankthresh,
				       double *ratios);
TIJK_EXPORT int tijk_approx_rankk_3d_f(float *ls, float *vs, float *res,
				       float *ten, const tijk_type *type,
				       unsigned int k, float rankthresh,
				       float *ratios);

TIJK_EXPORT int tijk_refine_rankk_2d_d(double *ls, double *vs, double *tens,
				       double *res, double *resnorm,
				       const tijk_type *type, unsigned int k);
TIJK_EXPORT int tijk_refine_rankk_2d_f(float *ls, float *vs, float *tens,
				       float *res, float *resnorm,
				       const tijk_type *type, unsigned int k);
TIJK_EXPORT int tijk_refine_rankk_3d_d(double *ls, double *vs, double *tens,
				       double *res, double *resnorm,
				       const tijk_type *type, unsigned int k);
TIJK_EXPORT int tijk_refine_rankk_3d_f(float *ls, float *vs, float *tens,
				       float *res, float *resnorm,
				       const tijk_type *type, unsigned int k);

/* shTijk.c */
TIJK_EXPORT int tijk_eval_esh_basis_d(double *res, int order,
				     double theta, double phi);
TIJK_EXPORT int tijk_eval_esh_basis_f(float *res, int order,
				     float theta, float phi);

TIJK_EXPORT double tijk_eval_esh_d(double *coeffs, int order,
				       double theta, double phi);
TIJK_EXPORT float tijk_eval_esh_f(float *coeffs, int order,
				      float theta, float phi);

TIJK_EXPORT int tijk_3d_sym_to_esh_d(double *res, const double *ten,
					 const tijk_type *type);
TIJK_EXPORT int tijk_3d_sym_to_esh_f(float *res, const float *ten,
					 const tijk_type *type);

TIJK_EXPORT const tijk_type *tijk_esh_to_3d_sym_d(double *res,
						  const double *sh, int order);
TIJK_EXPORT const tijk_type *tijk_esh_to_3d_sym_f(float *res,
						  const float *sh, int order);
#ifdef __cplusplus
}
#endif

#endif /* ELL_HAS_BEEN_INCLUDED */
