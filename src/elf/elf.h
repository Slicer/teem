/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2011, 2010, 2009 Thomas Schultz

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

#ifndef ELF_HAS_BEEN_INCLUDED
#define ELF_HAS_BEEN_INCLUDED

#include <teem/tijk.h>
#include <teem/limn.h>

#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(TEEM_STATIC)
#  if defined(TEEM_BUILD) || defined(tijk_EXPORTS) || defined(teem_EXPORTS)
#    define ELF_EXPORT extern __declspec(dllexport)
#  else
#    define ELF_EXPORT extern __declspec(dllimport)
#  endif
#else /* TEEM_STATIC || UNIX */
#  define ELF_EXPORT extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* glyphElf.c */
ELF_EXPORT float elfGlyphHOME(limnPolyData *glyph, const char antipodal,
                              const float *ten, const tijk_type *type,
                              char *isdef, const char normalize);

ELF_EXPORT float elfGlyphPolar(limnPolyData *glyph, const char antipodal,
                               const float *ten, const tijk_type *type,
                               char *isdef, const char clamp,
                               const char normalize,
                               const unsigned char *posColor,
                               const unsigned char *negColor);

ELF_EXPORT int elfColorGlyphMaxima(limnPolyData *glyph, const char antipodal,
                                   const int *neighbors, unsigned int nbstride,
                                   const float *ten, const tijk_type *type,
                                   const char modulate, const float gamma);

/*
********** elfMaximaContext
**
** Allows us to precompute and store information needed to find all maxima
** of a given symmetric 3D tensor type. Should only be used through elfMaxima*
*/
typedef struct {
  unsigned int num;
  const tijk_type *type;
  tijk_refine_rank1_parm *parm;
  int refine;
  int *neighbors;
  unsigned int nbstride;
  float *vertices_f; /* we're only storing the non-redundant ones */
  double *vertices_d; /* only filled when needed */
} elfMaximaContext;

/* maximaElf.c */
ELF_EXPORT elfMaximaContext *elfMaximaContextNew(const tijk_type *type,
						 unsigned int level);
ELF_EXPORT elfMaximaContext *elfMaximaContextNix(elfMaximaContext *emc);
ELF_EXPORT void elfMaximaParmSet(elfMaximaContext *emc,
				 tijk_refine_rank1_parm *parm);
ELF_EXPORT void elfMaximaRefineSet(elfMaximaContext *emc, int refine);
ELF_EXPORT int elfMaximaFind_d(double **ls, double **vs, const double *ten,
			       elfMaximaContext *emc);
ELF_EXPORT int elfMaximaFind_f(float **ls, float **vs, const float *ten,
			       elfMaximaContext *emc);

#if 0 /* the following parts of the API are still unimplemented: */
/* ESHEstimElf.c */

/* elfESHEstimMatrix:
 *
 * Computes a matrix T that can be used to transform a measurement vector v
 * of ct values on the sphere, at positions given by thetaphi, into a vector
 * c of ESH coefficients of the desired order (c=Tv) such that the ESH
 * series approximates the given values in a least squares sense.
 *
 * T needs to be pre-allocated to length len(c)*ct
 * If H is non-NULL, it should be pre-allocated to length ct*ct
 *   In this case, the "hat" matrix will be written to H, which maps the
 *   measurement vector v to the corresponding model predictions
 * If lambda>0, Laplace-Beltrami regularization is employed
 * If w is non-NULL, it is assumed to be a weight vector with len(w)=ct
 *
 * Returns 0 on success, 1 if len(v)<len(c) (i.e., system is underdetermined)
 */
ELF_EXPORT int elfESHEstimMatrix_f(float *T, float *H, unsigned int order,
				   const float *thetaphi,
				   unsigned int ct, float lambda, float *w);

/* elfCart2Thetaphi:
 *
 * Helper function to transform ct 3D Cartesian coordinates into
 * theta (polar angle from positive z) and phi (azimuth from positive x)
 * Input vectors should be non-zero, but do not need to be normalized
 * thetaphi needs to be pre-allocated to length ct
 */
ELF_EXPORT void elfCart2Thetaphi_f(float *thetaphi, const float *dirs,
				   unsigned int ct);

/* elfKernelStick:
 *
 * Computes the deconvolution kernel corresponding to a single stick, for
 * desired ESH order, bd (b value times diffusivity) and b0 (non-DWI signal)
 * If delta!=0, deconvolves to delta peak, else to rank-1 peak
 *
 * returns 0 on success, 1 if order is not supported
 */
ELF_EXPORT int elfKernelStick_f(float *kernel, unsigned int order, float bd,
                                float b0, int delta);

/* BallStickElf.c */

/* elfDWIMeasurement:
 *
 * Collects the parameters and measurements of a DWI experiment
 */
typedef struct {
  float b0;           /* unweighted measurement */
  float b;            /* b value */
  float *dwis; 	      /* diffusion-weighted measurements */
  float *grads;       /* corresponding gradient vectors (Cartesian 3D) */
  unsigned int dwino; /* number of dwis */
} elfDWIMeasurement;

/* elfBallStickODF:
 *
 * Deconvolves DWI measurements to an ODF, using a kernel derived from
 * the ball-and-stick model
 *
 * Output:
 *  odf   - ESH coefficients of computed ODF
 *  fiso  - if non-NULL, estimated isotropic volume fraction
 *  d     - if non-NULL, estimated ADC
 * Input:
 *  dwi   - DWI measurement and parameters
 *  T     - matrix computed by elfESHEstimMatrix
 *  order - desired ESH order of odf
 *  delta - whether to use delta peak (set to 0 when using elfBallStickPredict)
 *
 * Returns 0 on success, 1 if order is not supported
 */
ELF_EXPORT int elfBallStickODF_f(float *odf, float *fiso, float *d,
                                 const elfDWIMeasurement *dwi,
				 const float *T, unsigned int order, int delta);

/* elfBallStickParms:
 *
 * Collects the parameters associated with a ball-and-multi-stick model
 * (up to three sticks), as well as debugging information
 */
typedef struct {
  float d; /* ADC */
  unsigned int fiberct;
  float fs[4]; /* fs[0]==fiso */
  float vs[9];
  /* remaining fields are for statistics/debugging */
  int stopreason;
  double sqrerr;
  double itr;
} elfBallStickParms;

/* elfBallStickPredict:
 *
 * Based on a rank-k decomposition of the given odf (of order "order"), sets
 * vs and fs[1..k] of parms. d (ADC) and fiso are used to set d and fs[0].
 * Returns 0 upon success, 1 upon error
 */
ELF_EXPORT int elfBallStickPredict_f(elfBallStickParms *parms, float *odf,
                                     unsigned int order, unsigned int k,
                                     float d, float fiso);

/* elfBallStickOptimize:
 *
 * Based on an initial guess of parms, use Levenberg-Marquardt optimization
 * to improve the fit w.r.t. the given DWI data.
 * Returns 0 upon success, 1 upon error
 */
ELF_EXPORT int elfBallStickOptimize_f(elfBallStickParms *parms,
                                      const elfDWIMeasurement *dwi);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ELF_HAS_BEEN_INCLUDED */
