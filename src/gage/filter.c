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

#include "gage.h"
#include "private.h"

/*
** sets the filter sample location (fsl) array based
** on probe location (xf,yf,zf)
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
  gage_t *fslx, *fsly, *fslz;
  gage_t xf, yf, zf;

  fr = ctx->fr;
  fslx = ctx->fsl + 0*2*fr;
  fsly = ctx->fsl + 1*2*fr;
  fslz = ctx->fsl + 2*2*fr;
  xf = ctx->xf;
  yf = ctx->yf;
  zf = ctx->zf;
  switch (fr) {
  case 1:
    fslx[0] = xf; fslx[1] = xf-1;
    fsly[0] = yf; fsly[1] = yf-1;
    fslz[0] = zf; fslz[1] = zf-1;
    break;
  case 20:
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
  gage_t integral, sumX, sumY, sumZ, *fwX, *fwY, *fwZ;
  int i, fd;

  fd = ctx->fd;
  fwX = ctx->fw[wch] + 0*fd;
  fwY = ctx->fw[wch] + 1*fd;
  fwZ = ctx->fw[wch] + 2*fd;
  integral = ctx->k[wch]->integral(ctx->kparm[wch]);
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
}

/*
** renormalize weights of a derivative kernel with
** constraint: the sum of the weights must be zero, but
** sign of individual weights must be preserved
*/
void
_gageFwDerivRenormalize(gageContext *ctx, int wch) {
  char me[]="_gageFwDerivRenormalize";
  gage_t negX, negY, negZ, posX, posY, posZ, fixX, fixY, fixZ,
    *fwX, *fwY, *fwZ;
  int i, fd;

  fd = ctx->fd;
  fwX = ctx->fw[wch] + 0*fd;
  fwY = ctx->fw[wch] + 1*fd;
  fwZ = ctx->fw[wch] + 2*fd;
  negX = negY = negZ = 0;
  posX = posY = posZ = 0;
  for (i=0; i<fd; i++) {
    if (fwX[i] <= 0) { negX += fwX[i]; } else { posX += fwX[i]; }
    if (fwY[i] <= 0) { negY += fwY[i]; } else { posY += fwY[i]; }
    if (fwZ[i] <= 0) { negZ += fwZ[i]; } else { posZ += fwZ[i]; }
  }
  /* fix is the sqrt() of factor by which the positive values
     are too big.  negative values are scaled up by this;
     positive values are scaled down by this */
  fixX = sqrt(-posX/negX);
  fixY = sqrt(-posY/negY);
  fixZ = sqrt(-posZ/negZ);
  if (ctx->verbose > 1) {
    fprintf(stderr, "%s: fixX = % 10.4f, fixY = % 10.4f, fixX = % 10.4f\n",
	    me, (float)fixX, (float)fixY, (float)fixZ);
  }
  for (i=0; i<fd; i++) {
    if (fwX[i] <= 0) { fwX[i] *= fixX; } else { fwX[i] /= fixX; }
    if (fwY[i] <= 0) { fwY[i] *= fixY; } else { fwY[i] /= fixY; }
    if (fwZ[i] <= 0) { fwZ[i] *= fixZ; } else { fwZ[i] /= fixZ; }
  }
}

void
_gageFwSet(gageContext *ctx) {
  char me[]="_gageFwSet";
  int i, j, fd;
  gage_t *fwX, *fwY, *fwZ;

#if GT_FLOAT
#  define EVALN evalN_f
#else
#  define EVALN evalN_d
#endif

  for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
    if (!ctx->fw[i])
      continue;
    ctx->k[i]->EVALN(ctx->fw[i], ctx->fsl, 3*ctx->fd, ctx->kparm[i]);
  }

  if (ctx->verbose > 1) {
    fprintf(stderr, "%s: filter weights after kernel evaluation:\n", me);
    _gagePrint_fslw(ctx);
  }
  if (ctx->renormalize) {
    for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
      if (!ctx->fw[i])
	continue;
      switch (i) {
      case gageKernel00:
      case gageKernel10:
      case gageKernel20:
	_gageFwValueRenormalize(ctx, i);
	break;
      default:
	_gageFwDerivRenormalize(ctx, i);
	break;
      }
    }
    if (ctx->verbose > 1) {
      fprintf(stderr, "%s: filter weights after renormalization:\n", me);
      _gagePrint_fslw(ctx);
    }
  }

  /* fix weightings for non-unit-spacing samples */
  if (!( 1.0 == ctx->xs && 1.0 == ctx->ys && 1.0 == ctx->zs )) {
    for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
      if (gageKernel00 == i || gageKernel10 == i || gageKernel20 == i)
	continue;
      if (!ctx->fw[i])
	continue;
      fd = ctx->fd;
      fwX = ctx->fw[i] + 0*fd;
      fwY = ctx->fw[i] + 1*fd;
      fwZ = ctx->fw[i] + 2*fd;
      for (j=0; j<fd; j++) {
	fwX[j] *= ctx->fwScl[i][0];
	fwY[j] *= ctx->fwScl[i][1];
	fwZ[j] *= ctx->fwScl[i][2];
      }
    }
    if (ctx->verbose > 1) {
      fprintf(stderr, "%s: filter weights after non-unit fix:\n", me);
      _gagePrint_fslw(ctx);
    }
  }
}

/*
** _gageLocationSet
**
** updates probe location in general context, and things which
** depend on it:
** bidx, fsl, fw
**
** does NOT use biff, but returns 1 on error and 0 if all okay
** Currently only error is probing outside volume, which sets
** gageErrNum=0 and sprints message into gageErrStr.
*/
int
_gageLocationSet(gageContext *ctx, int *newBidxP,
		 gage_t x, gage_t y, gage_t z) {
  char me[]="_gageProbeLocationSet";
  int tx, ty, tz,     /* "top" x, y, z: highest valid float-point
			 position for position in unpadded volume */
    xi, yi, zi,       /* computed integral positions in unpadded
                         volume */
    dif,              /* difference between coordinates between 
			 "havePad"- and "needPad"-padded volumes */
    bidx;             /* base index in padded volume */   
  gage_t xf, yf, zf;
  
  tx = ctx->sx - 2*ctx->havePad - 1;
  ty = ctx->sy - 2*ctx->havePad - 1;
  tz = ctx->sz - 2*ctx->havePad - 1;
  if (!( AIR_INSIDE(0,x,tx) && AIR_INSIDE(0,y,ty) && AIR_INSIDE(0,z,tz) )) {
    sprintf(gageErrStr, "%s: position (%g,%g,%g) outside bounds "
	    "[0..%d,0..%d,0..%d]",
	    me, (float)x, (float)y, (float)z, tx, ty, tz);
    gageErrNum = 0;
    return 1;
  }
  /* else */

  xi = x; xi -= xi == tx; xf = x - xi;
  yi = y; yi -= yi == ty; yf = y - yi;
  zi = z; zi -= zi == tz; zf = z - zi;
  dif = ctx->havePad - ctx->needPad;
  bidx = xi + dif + ctx->sx*(yi + dif + ctx->sy*(zi + dif));
  if (ctx->verbose > 1) {
    fprintf(stderr, "%s: \n"
	    "        pos (% 15.7f,% 15.7f,% 15.7f) \n"
	    "        -> i(%5d,%5d,%5d) (unpadded) \n"
	    "        -> i(%5d,%5d,%5d) (padded) \n"
	    "         + f(% 15.7f,% 15.7f,% 15.7f) \n"
	    "        -> bidx = %d\n",
	    me,
	    (float)x, (float)y, (float)z,
	    xi, yi, zi,
	    xi + dif, yi + dif, zi + dif,
	    (float)xf, (float)yf, (float)zf,
	    bidx);
  }
  if (ctx->bidx != bidx) {
    *newBidxP = AIR_TRUE;
    ctx->bidx = bidx;
  } else {
    *newBidxP = AIR_FALSE;
  }
  
  if (!( ctx->xf == xf && ctx->yf == yf && ctx->zf == zf )) {
    ctx->xf = xf;
    ctx->yf = yf;
    ctx->zf = zf;
    /* this is essentially _gageLocationDependentSet() */
    _gageFslSet(ctx);
    _gageFwSet(ctx);
  }

  return 0;
}
