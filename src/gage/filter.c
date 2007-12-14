/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

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

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "gage.h"
#include "privateGage.h"

/*
** sets the filter sample location (fsl) array based
** on probe location (xf,yf,zf) stored in ctx->point
**
** One possible rare surpise: if a filter is not continuous with 0
** at the end of its support, and if the sample location is at the
** highest possible point (xi == N-2, xf = 1.0), then the filter
** weights may not be the desired ones.  Forward differencing (via
** nrrdKernelForwDiff) is a good example of this.
*/
void
_gageFslSet(gageContext *ctx) {
  int fr, i;
  double *fslx, *fsly, *fslz;
  double xf, yf, zf;

  fr = ctx->radius;
  fslx = ctx->fsl + 0*2*fr;
  fsly = ctx->fsl + 1*2*fr;
  fslz = ctx->fsl + 2*2*fr;
  xf = ctx->point.xf;
  yf = ctx->point.yf;
  zf = ctx->point.zf;
  switch (fr) {
  case 1:
    fslx[0] = xf; fslx[1] = xf-1;
    fsly[0] = yf; fsly[1] = yf-1;
    fslz[0] = zf; fslz[1] = zf-1;
    break;
  case 2:
    fslx[0] = xf+1; fslx[1] = xf; fslx[2] = xf-1; fslx[3] = xf-2;
    fsly[0] = yf+1; fsly[1] = yf; fsly[2] = yf-1; fsly[3] = yf-2;
    fslz[0] = zf+1; fslz[1] = zf; fslz[2] = zf-1; fslz[3] = zf-2;
    break;
  default:
    /* filter radius bigger than 2 */
    for (i=-fr+1; i<=fr; i++) {
      fslx[i+fr-1] = xf-i;
      fsly[i+fr-1] = yf-i;
      fslz[i+fr-1] = zf-i;
    }
    break;
  }
  return;
}

/*
** renormalize weights of a reconstruction kernel with
** constraint: the sum of the weights must equal the continuous
** integral of the kernel
*/
void
_gageFwValueRenormalize(gageContext *ctx, int wch) {
  double integral, sumX, sumY, sumZ, *fwX, *fwY, *fwZ;
  int i, fd;

  fd = 2*ctx->radius;
  fwX = ctx->fw + 0 + fd*(0 + 3*wch);
  fwY = ctx->fw + 0 + fd*(1 + 3*wch);
  fwZ = ctx->fw + 0 + fd*(2 + 3*wch);
  integral = ctx->ksp[wch]->kernel->integral(ctx->ksp[wch]->parm);
  sumX = sumY = sumZ = 0;
  for (i=0; i<fd; i++) {
    sumX += fwX[i];
    sumY += fwY[i];
    sumZ += fwZ[i];
  }
  for (i=0; i<fd; i++) {
    fwX[i] *= integral/sumX;
    fwY[i] *= integral/sumY;
    fwZ[i] *= integral/sumZ;
  }
  return;
}

/*
** renormalize weights of a derivative kernel with
** constraint: the sum of the weights must be zero, but
** sign of individual weights must be preserved
*/
void
_gageFwDerivRenormalize(gageContext *ctx, int wch) {
  char me[]="_gageFwDerivRenormalize";
  double negX, negY, negZ, posX, posY, posZ, fixX, fixY, fixZ,
    *fwX, *fwY, *fwZ;
  int i, fd;

  fd = 2*ctx->radius;
  fwX = ctx->fw + 0 + fd*(0 + 3*wch);
  fwY = ctx->fw + 0 + fd*(1 + 3*wch);
  fwZ = ctx->fw + 0 + fd*(2 + 3*wch);
  negX = negY = negZ = 0;
  posX = posY = posZ = 0;
  for (i=0; i<fd; i++) {
    if (fwX[i] <= 0) { negX += -fwX[i]; } else { posX += fwX[i]; }
    if (fwY[i] <= 0) { negY += -fwY[i]; } else { posY += fwY[i]; }
    if (fwZ[i] <= 0) { negZ += -fwZ[i]; } else { posZ += fwZ[i]; }
  }
  /* fix is the sqrt() of factor by which the positive values
     are too big.  negative values are scaled up by fix;
     positive values are scaled down by fix */
  fixX = sqrt(posX/negX);
  fixY = sqrt(posY/negY);
  fixZ = sqrt(posZ/negZ);
  if (ctx->verbose > 1) {
    fprintf(stderr, "%s: fixX = % 10.4f, fixY = % 10.4f, fixX = % 10.4f\n",
            me, (float)fixX, (float)fixY, (float)fixZ);
  }
  for (i=0; i<fd; i++) {
    if (fwX[i] <= 0) { fwX[i] *= fixX; } else { fwX[i] /= fixX; }
    if (fwY[i] <= 0) { fwY[i] *= fixY; } else { fwY[i] /= fixY; }
    if (fwZ[i] <= 0) { fwZ[i] *= fixZ; } else { fwZ[i] /= fixZ; }
  }
  return;
}

void
_gageFwSet(gageContext *ctx) {
  char me[]="_gageFwSet";
  int kidx, j, fd;
  double *fwX, *fwY, *fwZ;
  
  fd = 2*ctx->radius;
  for (kidx=gageKernelUnknown+1; kidx<gageKernelLast; kidx++) {
    if (!ctx->needK[kidx]) {
      continue;
    }
    /* we evaluate weights for all three axes with one call */
    ctx->ksp[kidx]->kernel->evalN_d(ctx->fw + fd*3*kidx, ctx->fsl,
                                    fd*3, ctx->ksp[kidx]->parm);
  }
  
  if (ctx->verbose > 1) {
    fprintf(stderr, "%s: filter weights after kernel evaluation:\n", me);
    _gagePrint_fslw(stderr, ctx);
  }
  if (ctx->parm.renormalize) {
    for (kidx=gageKernelUnknown+1; kidx<gageKernelLast; kidx++) {
      if (!ctx->needK[kidx])
        continue;
      switch (kidx) {
      case gageKernel00:
      case gageKernel10:
      case gageKernel20:
        _gageFwValueRenormalize(ctx, kidx);
        break;
      default:
        _gageFwDerivRenormalize(ctx, kidx);
        break;
      }
    }
    if (ctx->verbose > 1) {
      fprintf(stderr, "%s: filter weights after renormalization:\n", me);
      _gagePrint_fslw(stderr, ctx);
    }
  }
  
  /* fix weightings for non-unit-spacing samples */
  if (!( 1.0 == ctx->shape->spacing[0] &&
         1.0 == ctx->shape->spacing[1] &&
         1.0 == ctx->shape->spacing[2] )) {
    for (kidx=gageKernelUnknown+1; kidx<gageKernelLast; kidx++) {
      if (!ctx->needK[kidx]) {
        continue;
      }
      if (gageKernel00 == kidx
          || gageKernel10 == kidx
          || gageKernel20 == kidx) {
        continue;
      }
      fwX = ctx->fw + 0 + fd*(0 + 3*kidx);
      fwY = ctx->fw + 0 + fd*(1 + 3*kidx);
      fwZ = ctx->fw + 0 + fd*(2 + 3*kidx);
      for (j=0; j<fd; j++) {
        fwX[j] *= ctx->shape->fwScale[kidx][0];
        fwY[j] *= ctx->shape->fwScale[kidx][1];
        fwZ[j] *= ctx->shape->fwScale[kidx][2];
      }
    }
    if (ctx->verbose > 1) {
      fprintf(stderr, "%s: filter weights after non-unit fix:\n", me);
      _gagePrint_fslw(stderr, ctx);
    }
  }

  return;
}

/*
** _gageLocationSet
**
** updates probe location in general context, and things which
** depend on it:
** fsl, fw
**
** (_xi,_yi,_zi) is *index* space position in the volume
** _si is the index-space position in the stack, the value is ignored
** if there is no stack behavior 
**
** does NOT use biff, but returns 1 on error and 0 if all okay
** Currently only error is probing outside volume, which sets
** ctx->errNum=0 and sprints message into ctx->errStr.
*/
int
_gageLocationSet(gageContext *ctx, double _xi, double _yi, double _zi,
                 double stackIdx) {
  char me[]="_gageProbeLocationSet";
  unsigned int top[3];  /* "top" x, y, z: highest valid index in volume */
  int xi, yi, zi;       /* computed integral positions in volume */
  double xf, yf, zf, min, max[3];

  top[0] = ctx->shape->size[0] - 1;
  top[1] = ctx->shape->size[1] - 1;
  top[2] = ctx->shape->size[2] - 1;
  if (nrrdCenterNode == ctx->shape->center) {
    min = 0;
    max[0] = top[0];
    max[1] = top[1];
    max[2] = top[2];
  } else {
    min = -0.5;
    max[0] = top[0] + 0.5;
    max[1] = top[1] + 0.5;
    max[2] = top[2] + 0.5;
  }
  if (!( AIR_IN_CL(min, _xi, max[0]) && 
         AIR_IN_CL(min, _yi, max[1]) && 
         AIR_IN_CL(min, _zi, max[2]) )) {
    sprintf(ctx->errStr, "%s: position (%g,%g,%g) outside (%s-centered) "
            "bounds [%g,%g]x[%g,%g]x[%g,%g]",
            me, _xi, _yi, _zi,
            airEnumStr(nrrdCenter, ctx->shape->center),
            min, max[0], min, max[1], min, max[2]);
    ctx->errNum = 0;
    return 1;
  }
  if (ctx->parm.stackUse) {
    if (!( AIR_IN_CL(0, stackIdx, ctx->pvlNum-2) )) {
      sprintf(ctx->errStr, "%s: stack position %g outside (%s-centered) "
              "bounds [0,%u]", me,
              stackIdx, airEnumStr(nrrdCenter, nrrdCenterNode), ctx->pvlNum-2);
      ctx->errNum = 0;
      return 1;
    }
  }
  xi = AIR_CAST(unsigned int, _xi+1) - 1; xf = _xi - xi;
  yi = AIR_CAST(unsigned int, _yi+1) - 1; yf = _yi - yi;
  zi = AIR_CAST(unsigned int, _zi+1) - 1; zf = _zi - zi;

  ctx->point.xi = xi;
  ctx->point.yi = yi;
  ctx->point.zi = zi;
  if (ctx->verbose > 1) {
    fprintf(stderr, "%s: \n"
            "        pos (% 15.7f,% 15.7f,% 15.7f) \n"
            "        -> i(%5d,%5d,%5d) \n"
            "         + f(% 15.7f,% 15.7f,% 15.7f) \n",
            me, _xi, _yi, _zi, xi, yi, zi, xf, yf, zf);
  }
  
  if (ctx->parm.stackUse
      || !( ctx->point.xf == xf &&
            ctx->point.yf == yf &&
            ctx->point.zf == zf )) {
    ctx->point.xf = xf;
    ctx->point.yf = yf;
    ctx->point.zf = zf;
    /* these may take some time (especially if using renormalization),
       hence the conditional above */
    _gageFslSet(ctx);
    _gageFwSet(ctx);
  }

  if (ctx->parm.stackUse) {
    int stackBase;
    double stackFrac, sum;
    unsigned int ii;
    NrrdKernelSpec *sksp;

    /* node-centered sampling of stack indices from 0 to ctx->pvlNum-2 */
    stackBase = AIR_CAST(int, stackIdx);
    stackFrac = stackIdx - stackBase;
    for (ii=0; ii<ctx->pvlNum-1; ii++) {
      ctx->stackFslw[ii] = stackIdx - ii;
    }  
    sksp = ctx->ksp[gageKernelStack];
    sksp->kernel->evalN_d(ctx->stackFslw, ctx->stackFslw,
                          ctx->pvlNum-1, sksp->parm);
  
    /* HEY: we really are quite far from implementing arbitrary
       nrrdBoundary behaviors here!!!! */
    /* HEY: surprise: gageParmStackRenormalize does NOT control this,
       since that refers to *derivative* normalization */
    /* renormalize weights */
    sum = 0;
    for (ii=0; ii<ctx->pvlNum-1; ii++) {
      sum += ctx->stackFslw[ii];
    }
    for (ii=0; ii<ctx->pvlNum-1; ii++) {
      ctx->stackFslw[ii] /= sum;
    }

    /* fix derivative kernel weights for stack */
    if (ctx->parm.stackRenormalize) {
      unsigned int kidx, fd, j;
      double scl, *fwX, *fwY, *fwZ;
      
      scl= AIR_AFFINE(0, stackIdx, ctx->pvlNum-2,
                      ctx->stackRange[0], ctx->stackRange[1]);
      fd = 2*ctx->radius;
      kidx = gageKernel11;
      fwX = ctx->fw + 0 + fd*(0 + 3*kidx);
      fwY = ctx->fw + 0 + fd*(1 + 3*kidx);
      fwZ = ctx->fw + 0 + fd*(2 + 3*kidx);
      for (j=0; j<fd; j++) {
        fwX[j] *= scl;
        fwY[j] *= scl;
        fwZ[j] *= scl;
      }
      kidx = gageKernel22;
      fwX = ctx->fw + 0 + fd*(0 + 3*kidx);
      fwY = ctx->fw + 0 + fd*(1 + 3*kidx);
      fwZ = ctx->fw + 0 + fd*(2 + 3*kidx);
      for (j=0; j<fd; j++) {
        fwX[j] *= scl*scl;
        fwY[j] *= scl*scl;
        fwZ[j] *= scl*scl;
      }
    }
  }
  
  return 0;
}
