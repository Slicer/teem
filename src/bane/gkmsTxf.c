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

#include "bane.h"
#include "privateBane.h"

#define TXF_INFO "Create Levoy-style triangular 2D opacity functions"
static const char *_baneGkms_txfInfoL
  = (TXF_INFO ". The triangles are in the 2D space of data value and gradient "
              "magnitude.  They can be tilted sideways and clipped at the bottom. "
              "This doesn't strictly speaking belong in \"gkms\" but there's no "
              "other good place in Teem.");
static int /* Biff: 1 */
baneGkms_txfMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *perr;
  Nrrd *nout;
  airArray *mop;
  int pret, E, res[2], vi, gi, step;
  float min[2], max[2], top[2], v0, g0, *data, v, g, gwidth, width, mwidth, tvl, tvr, vl,
    vr, tmp, maxa;

  /* HEY many of these ints should be unsigned, but bane never got the signed->unsigned
   * treatment */
  hestOptAdd_2_Int(&opt, "r", "Vres Gres", res, "256 256",
                   "resolution of the transfer function in value and gradient "
                   "magnitude");
  hestOptAdd_2_Float(&opt, "min", "Vmin Gmin", min, "0.0 0.0",
                     "minimum value and grad mag in txf");
  hestOptAdd_2_Float(&opt, "max", "Vmax Gmax", max, NULL,
                     "maximum value and grad mag in txf");
  hestOptAdd_1_Float(&opt, "v", "base value", &v0, NULL,
                     "data value at which to position bottom of triangle");
  hestOptAdd_1_Float(&opt, "g", "gthresh", &g0, "0.0",
                     "lowest grad mag to receive opacity");
  hestOptAdd_1_Float(&opt, "gw", "gwidth", &gwidth, "0.0",
                     "range of grad mag values over which to apply threshold "
                     "at low gradient magnitudes");
  hestOptAdd_2_Float(&opt, "top", "Vtop Gtop", top, NULL,
                     "data value and grad mag at center of top of triangle");
  hestOptAdd_1_Float(&opt, "w", "value width", &width, NULL,
                     "range of values to be spanned at top of triangle");
  hestOptAdd_1_Float(&opt, "mw", "value width", &mwidth, "0",
                     "range of values to be spanned at BOTTOM of triangle");
  hestOptAdd_Flag(&opt, "step", &step,
                  "instead of assigning opacity inside a triangular region, "
                  "make it more like a step function, in which opacity never "
                  "decreases in increasing data value");
  hestOptAdd_1_Float(&opt, "a", "max opac", &maxa, "1.0", "highest opacity to assign");
  hestOptAdd_1_String(&opt, "o", "opacOut", &out, NULL,
                      "output opacity function filename");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_baneGkms_txfInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  E = 0;
  if (!E)
    E |= nrrdMaybeAlloc_va(nout, nrrdTypeFloat, 3, AIR_SIZE_T(1), AIR_SIZE_T(res[0]),
                           AIR_SIZE_T(res[1]));
  if (!E) E |= !(nout->axis[0].label = airStrdup("A"));
  if (!E) E |= !(nout->axis[1].label = airStrdup("gage(scalar:v)"));
  if (!E)
    nrrdAxisInfoSet_va(nout, nrrdAxisInfoMin, AIR_NAN, (double)min[0], (double)min[1]);
  if (!E)
    nrrdAxisInfoSet_va(nout, nrrdAxisInfoMax, AIR_NAN, (double)max[0], (double)max[1]);
  if (!E) E |= !(nout->axis[2].label = airStrdup("gage(scalar:gm)"));
  if (E) {
    biffMovef(BANE, NRRD, "%s: trouble creating opacity function nrrd", me);
    airMopError(mop);
    return 1;
  }
  data = (float *)nout->data;
  tvl = top[0] - width / 2;
  tvr = top[0] + width / 2;
  mwidth /= 2;
  for (gi = 0; gi < res[1]; gi++) {
    g = AIR_FLOAT(NRRD_CELL_POS(min[1], max[1], res[1], gi));
    for (vi = 0; vi < res[0]; vi++) {
      v = AIR_FLOAT(NRRD_CELL_POS(min[0], max[0], res[0], vi));
      vl = AIR_FLOAT(AIR_AFFINE(0, g, top[1], v0 - mwidth, tvl));
      vr = AIR_FLOAT(AIR_AFFINE(0, g, top[1], v0 + mwidth, tvr));
      if (g > top[1]) {
        data[vi + res[0] * gi] = 0;
        continue;
      }
      tmp = AIR_FLOAT((v - vl) / (0.00001 + vr - vl));
      tmp = 1 - AIR_ABS(2 * tmp - 1);
      if (step && v > (vr + vl) / 2) {
        tmp = 1;
      }
      tmp = AIR_MAX(0, tmp);
      data[vi + res[0] * gi] = tmp * maxa;
      tmp = AIR_FLOAT(AIR_AFFINE(g0 - gwidth / 2, g, g0 + gwidth / 2, 0.0, 1.0));
      tmp = AIR_CLAMP(0, tmp, 1);
      data[vi + res[0] * gi] *= tmp;
    }
  }
  if (nrrdSave(out, nout, NULL)) {
    biffMovef(BANE, NRRD, "%s: trouble saving opacity function", me);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
BANE_GKMS_CMD(txf, TXF_INFO);
