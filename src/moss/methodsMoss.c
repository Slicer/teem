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

const int mossPresent = 42;

/*
******** mossSamplerNew()
**
*/
mossSampler * /* Biff: nope */
mossSamplerNew(void) {
  mossSampler *smplr;
  int i;

  smplr = AIR_CALLOC(1, mossSampler);
  if (smplr) {
    smplr->verbose = 0;
    smplr->verbPixel[0] = smplr->verbPixel[1] = -1;
    smplr->image = NULL;
    smplr->boundary = nrrdBoundaryUnknown;
    smplr->kspec = nrrdKernelSpecNew();
    smplr->filterDiam = smplr->chanNum = 0;
    smplr->xIdx = smplr->yIdx = NULL;
    smplr->ivc = smplr->xFslw = smplr->yFslw = smplr->bg = NULL;
    for (i = 0; i <= MOSS_FLAG_MAX; i++) {
      smplr->flag[i] = AIR_FALSE;
    }
  }
  return smplr;
}

mossSampler * /* Biff: nope */
mossSamplerNix(mossSampler *smplr) {

  if (smplr) {
    /* do not own image */
    nrrdKernelSpecNix(smplr->kspec);
    airFree(smplr->xIdx);
    airFree(smplr->yIdx);
    airFree(smplr->ivc);
    airFree(smplr->xFslw);
    airFree(smplr->yFslw);
    airFree(smplr->bg);
    free(smplr);
  }
  return NULL;
}

/*
** mossImageCheck: ensures that given image works as an *input* image
*/
int /* Biff: 1 */
mossImageCheck(const Nrrd *image) {
  static const char me[] = "mossImageCheck";

  if (nrrdCheck(image)) {
    biffMovef(MOSS, NRRD, "%s: given nrrd invalid", me);
    return 1;
  }
  if (!((2 == image->dim || 3 == image->dim) && nrrdTypeBlock != image->type)) {
    biffAddf(MOSS, "%s: image has invalid dimension (%d) or type (%s)", me, image->dim,
             airEnumStr(nrrdType, image->type));
    return 1;
  }

  return 0;
}

/*
** mossImageAlloc: helper function to allocate *output* image within the given Nrrd
** container "image"
*/
int /* Biff: 1 */
mossImageAlloc(Nrrd *image, int type, unsigned int _sx, unsigned int _sy,
               unsigned int _chanNum) {
  static const char me[] = "mossImageAlloc";
  size_t sx, sy, chanNum;
  int ret;

  if (!(image                                              /* */
        && AIR_IN_OP(nrrdTypeUnknown, type, nrrdTypeBlock) /* */
        && _sx > 0 && _sy > 0 && _chanNum > 0)) {
    biffAddf(MOSS, "%s: got NULL pointer or bad args", me);
    return 1;
  }
  sx = AIR_SIZE_T(_sx);
  sy = AIR_SIZE_T(_sy);
  chanNum = AIR_SIZE_T(_chanNum);
  if (1 == chanNum) {
    ret = nrrdMaybeAlloc_va(image, type, 2, sx, sy);
  } else {
    ret = nrrdMaybeAlloc_va(image, type, 3, chanNum, sx, sy);
  }
  if (ret) {
    biffMovef(MOSS, NRRD, "%s: couldn't allocate image", me);
    return 1;
  }

  return 0;
}

int /* Biff: (private) nope */
_mossCenter(int center) {

  return (airEnumValCheck(nrrdCenter, center) ? mossDefCenter : center);
}
