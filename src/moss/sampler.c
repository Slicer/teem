/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "moss.h"
#include "privateMoss.h"

int /* Biff: 1 */
mossSamplerImageSet(mossSampler *smplr, const Nrrd *image, int boundary,
                    const double *bg) {
  static const char me[] = "mossSamplerImageSet";

  if (!(smplr && image)) {
    biffAddf(MOSS, "%s: got NULL pointer", me);
    return 1;
  }
  if (mossImageCheck(image)) {
    biffAddf(MOSS, "%s: ", me);
    return 1;
  }
  if (airEnumValCheck(nrrdBoundary, boundary)) {
    biffAddf(MOSS, "%s: %d not a valid %s", me, boundary, nrrdBoundary->name);
    return 1;
  }
  smplr->bg = (double *)airFree(smplr->bg);
  /* after error chacking, allocation of smplr->bg array handled here, since it is
     directly associated with image, and it's too annoying to maintain separate info
     about how long bg is allocated for, separately from chanNum */
  if (nrrdBoundaryPad != boundary) {
    if (bg) {
      biffAddf(MOSS,
               "%s: want %s %s (which does not need a background color), but given "
               "non-NULL background color bg",
               me, nrrdBoundary->name, airEnumStr(nrrdBoundary, boundary));
      return 1;
    }
  } else { /* nrrdBoundaryPad == boundary */
    unsigned int ci, chanNum;
    if (!bg) {
      biffAddf(MOSS,
               "%s: want %s %s (which needs background color), but given NULL "
               "background color bg",
               me, nrrdBoundary->name, airEnumStr(nrrdBoundary, boundary));
      return 1;
    }
    /* because bg is allocate-able directly from image, we handle that here,
       instead of in mossSamplerUpdate */
    chanNum = MOSS_CHAN_NUM(image);
    smplr->bg = AIR_CALLOC(chanNum, double);
    for (ci = 0; ci < chanNum; ci++) {
      smplr->bg[ci] = bg[ci];
    }
  }
  smplr->image = image;
  smplr->boundary = boundary;
  smplr->flag[mossFlagImage] = AIR_TRUE;
  return 0;
}

int /* Biff: 1 */
mossSamplerKernelSet(mossSampler *smplr, const NrrdKernelSpec *kspec) {
  static const char me[] = "mossSamplerKernelSet";

  if (!(smplr && kspec)) {
    biffAddf(MOSS, "%s: got NULL pointer", me);
    return 1;
  }
  nrrdKernelSpecSet(smplr->kspec, kspec->kernel, kspec->parm);
  smplr->flag[mossFlagKernel] = AIR_TRUE;
  return 0;
}

static int
flagUp(const mossSampler *smplr) {
  int fi, up = 0;
  for (fi = mossFlagUnknown + 1; fi < mossFlagLast; fi++) {
    up |= smplr->flag[fi];
  }
  return up;
}

int /* Biff: 1 */
mossSamplerUpdate(mossSampler *smplr) {
  static const char me[] = "mossSamplerUpdate";

  if (!(smplr)) {
    biffAddf(MOSS, "%s: got NULL pointer", me);
    return 1;
  }

  if (smplr->flag[mossFlagImage]) {
    unsigned int chn = MOSS_CHAN_NUM(smplr->image);
    if (smplr->verbose) {
      printf("%s: see mossFlagImage UP\n", me);
    }
    if (chn != smplr->chanNum) {
      if (smplr->verbose) {
        printf("%s: new %u chanNum != old %u --> raising mossFlagChanNum UP\n", me, chn,
               smplr->chanNum);
      }
      smplr->chanNum = chn;
      smplr->flag[mossFlagChanNum] = AIR_TRUE;
    }
    smplr->flag[mossFlagImage] = AIR_FALSE;
    if (smplr->verbose) {
      printf("%s: pulling mossFlagImage down\n", me);
    }
  }

  if (smplr->flag[mossFlagKernel]) {
    /* note that filterDiam will always be EVEN */
    unsigned int fdiam
      = 2 * AIR_ROUNDUP_UI(smplr->kspec->kernel->support(smplr->kspec->parm));
    if (smplr->verbose) {
      printf("%s: see mossFlagKernel UP\n", me);
    }
    if (fdiam != smplr->filterDiam) {
      if (smplr->verbose) {
        printf("%s: old filter diam %u != new %u --> raising mossFlagFilterDiam\n", me,
               smplr->filterDiam, fdiam);
      }
      smplr->filterDiam = fdiam;
      smplr->flag[mossFlagFilterDiam] = AIR_TRUE;
    }
    if (smplr->verbose) {
      printf("%s: pulling mossFlagKernel down\n", me);
    }
    smplr->flag[mossFlagKernel] = AIR_FALSE;
  }

  if (smplr->flag[mossFlagFilterDiam]) {
    if (smplr->verbose) {
      printf("%s: see mossFlagFilterDiam UP --> realloc {x,y}{Idx,Fslw}\n", me);
    }
    airFree(smplr->xIdx);
    airFree(smplr->yIdx);
    airFree(smplr->xFslw);
    airFree(smplr->yFslw);
    smplr->xIdx = AIR_CALLOC(smplr->filterDiam, int);
    smplr->yIdx = AIR_CALLOC(smplr->filterDiam, int);
    smplr->xFslw = AIR_CALLOC(smplr->filterDiam, double);
    smplr->yFslw = AIR_CALLOC(smplr->filterDiam, double);
  }
  if (smplr->flag[mossFlagFilterDiam] || smplr->flag[mossFlagChanNum]) {
    unsigned int fdiam = smplr->filterDiam, nchan = smplr->chanNum;
    if (smplr->verbose) {
      printf("%s: see either mossFlag{FilterDiam,ChanNum} UP --> realloc ivc\n", me);
    }
    airFree(smplr->ivc);
    smplr->ivc = AIR_CALLOC(fdiam * fdiam * nchan, double);
  }
  if (smplr->flag[mossFlagFilterDiam]) {
    if (smplr->verbose) {
      printf("%s: setting mossFlagFilterDiam DOWN\n", me);
    }
    smplr->flag[mossFlagFilterDiam] = AIR_FALSE;
  }
  if (smplr->flag[mossFlagChanNum]) {
    if (smplr->verbose) {
      printf("%s: setting mossFlagChanNum DOWN\n", me);
    }
    smplr->flag[mossFlagChanNum] = AIR_FALSE;
  }

  if (flagUp(smplr)) {
    biffAddf(MOSS, "%s: flag handling error", me);
    return 1;
  }
  return 0;
}

/*
** NOTE: this currently ONLY works with (xPos,yPos) in *index* space
**
** returns non-zero in case of error, but does NOT use biff (too heavy weight)
*/
int /* Biff: 1 */
mossSamplerSample(double *val, mossSampler *smplr, double xPos, double yPos) {
  static const char me[] = "mossSamplerSample";
  double xf, yf;
  int ii, jj, sx, sy, xi, yi, fdiam, frad;
  unsigned int ci, nchan;
  double (*lup)(const void *v, size_t I);
  const void *data;

  if (!(val && smplr && smplr->ivc)) {
    return 1;
  }

  /* set {x,y}Idx, set {x,y}Fslw to sample locations */
  sx = MOSS_SX(smplr->image);
  sy = MOSS_SY(smplr->image);
  xi = (int)floor(xPos);
  yi = (int)floor(yPos);
  xf = xPos - xi;
  yf = yPos - yi;
  fdiam = smplr->filterDiam; /* always EVEN */
  frad = fdiam / 2;
  if (smplr->verbose) {
    printf("%s: fdiam = %d; frad = %d\n", me, fdiam, frad);
    printf("%s: {x,y}Pos = %g %g --> %d %d  +  %g %g\n", me, xPos, yPos, xi, yi, xf, yf);
  }
  for (ii = 1 - frad; ii <= frad; ii++) {
    int ai = ii - (1 - frad);
    smplr->xIdx[ai] = xi + ii;
    smplr->yIdx[ai] = yi + ii;
    smplr->xFslw[ai] = xf - ii;
    smplr->yFslw[ai] = yf - ii;
    if (smplr->verbose) {
      printf("  orig --> {x,y}Idx[%d->%d]: %d %d ; {x,y}Fsl %g %g\n", ii, ai,
             smplr->xIdx[ai], smplr->yIdx[ai], smplr->xFslw[ai], smplr->yFslw[ai]);
    }
  }
  switch (smplr->boundary) {
  case nrrdBoundaryBleed:
    for (ii = 0; ii < fdiam; ii++) {
      smplr->xIdx[ii] = AIR_CLAMP(0, smplr->xIdx[ii], sx - 1);
      smplr->yIdx[ii] = AIR_CLAMP(0, smplr->yIdx[ii], sy - 1);
    }
    break;
  case nrrdBoundaryWrap:
    for (ii = 0; ii < fdiam; ii++) {
      smplr->xIdx[ii] = AIR_MOD(smplr->xIdx[ii], sx);
      smplr->yIdx[ii] = AIR_MOD(smplr->yIdx[ii], sy);
    }
    break;
  case nrrdBoundaryMirror:
    for (ii = 0; ii < fdiam; ii++) {
      smplr->xIdx[ii] = airIndexMirror32(smplr->xIdx[ii], AIR_UINT(sx));
      smplr->yIdx[ii] = airIndexMirror32(smplr->yIdx[ii], AIR_UINT(sy));
    }
    break;
  case nrrdBoundaryPad:
    /* this is handled later */
    break;
  default:
    biffAddf(MOSS, "%s: sorry, %s boundary not implemented", me,
             airEnumStr(nrrdBoundary, smplr->boundary));
    return 1;
  }
  if (smplr->verbose) {
    for (ii = 0; ii < fdiam; ii++) {
      printf(" bound --> {x,y}Idx[%u]: %d %d\n", ii, smplr->xIdx[ii], smplr->yIdx[ii]);
    }
  }

  /* copy values to ivc, set {x,y}Fslw to filter sample weights */
  nchan = smplr->chanNum;
  data = smplr->image->data;
  lup = nrrdDLookup[smplr->image->type];
  if (nrrdBoundaryPad == smplr->boundary) {
    for (jj = 0; jj < fdiam; jj++) {
      yi = smplr->yIdx[jj];
      for (ii = 0; ii < fdiam; ii++) {
        xi = smplr->xIdx[ii];
        if (AIR_IN_CL(0, xi, sx - 1) && AIR_IN_CL(0, yi, sy - 1)) {
          for (ci = 0; ci < nchan; ci++) {
            smplr->ivc[ii + fdiam * (jj + fdiam * ci)]
              = lup(data, ci + nchan * (xi + sx * yi));
          }
        } else {
          for (ci = 0; ci < nchan; ci++) {
            smplr->ivc[ii + fdiam * (jj + fdiam * ci)] = smplr->bg[ci];
          }
        }
        if (smplr->verbose) {
          for (ci = 0; ci < nchan; ci++) {
            printf("  ivc[ii=%d, jj=%d, ci=%u] = %g\n", ii, jj, ci,
                   smplr->ivc[ii + fdiam * (jj + fdiam * ci)]);
          }
        }
      }
    }
  } else {
    for (jj = 0; jj < fdiam; jj++) {
      yi = smplr->yIdx[jj];
      for (ii = 0; ii < fdiam; ii++) {
        xi = smplr->xIdx[ii];
        for (ci = 0; ci < nchan; ci++) {
          smplr->ivc[ii + fdiam * (jj + fdiam * ci)] = lup(data,
                                                           ci + nchan * (xi + sx * yi));
        }
      }
    }
  }
  smplr->kspec->kernel->evalN_d(smplr->xFslw, smplr->xFslw, fdiam, smplr->kspec->parm);
  smplr->kspec->kernel->evalN_d(smplr->yFslw, smplr->yFslw, fdiam, smplr->kspec->parm);
  if (smplr->verbose) {
    for (ii = 0; ii < fdiam; ii++) {
      printf("   [%d] --> {x,y}Fsw %g %g\n", ii, smplr->xFslw[ii], smplr->yFslw[ii]);
    }
  }

  /* do convolution */
  memset(val, 0, nchan * sizeof(double));
  ii = 0;
  for (ci = 0; ci < nchan; ci++) {
    double tmp = 0;
    for (yi = 0; yi < fdiam; yi++) {
      for (xi = 0; xi < fdiam; xi++) {
        tmp += (smplr->yFslw[yi] * smplr->xFslw[xi] * smplr->ivc[ii++]);
      }
    }
    val[ci] += tmp;
  }

  return 0;
}
