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

#define HVOL_INFO "Make histogram volume"
static const char *_baneGkms_hvolInfoL
  = (HVOL_INFO ".  The histogram volume is a three-dimensional histogram recording "
               "the relationship between data value, gradient magnitude, and the "
               "second directional derivative along the gradient direction.  Creating "
               "it is the first step in semi-automatic transfer function generation.  ");

static int /* Biff: 1 */
baneGkms_hvolMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *perr;
  Nrrd *nin, *nout;
  airArray *mop;
  int pret, dim[3], lapl, slow, gz = AIR_FALSE;
  double inc[3 * (1 + BANE_PARM_NUM)];
  baneHVolParm *hvp;
  NrrdIoState *nio;
  NrrdKernelSpec *ksp00, *ksp11, *ksp22;

  hestOptAdd_3_Other(&opt, "s", "incV incG incH", inc, "f:1.0 p:0.005 p:0.015",
                     "Strategies for determining how much of the range "
                     "of a quantity should be included and quantized in its axis "
                     "of the histogram volume.  Possibilities include:\n "
                     "\b\bo \"f:<F>\": included range is some fraction of the "
                     "total range, as scaled by F\n "
                     "\b\bo \"p:<P>\": exclude the extremal P percent of "
                     "the values\n "
                     "\b\bo \"s:<S>\": included range is S times the standard "
                     "deviation of the values\n "
                     "\b\bo \"a:<min>,<max>\": range is from <min> to <max>",
                     baneGkmsHestIncStrategy);
  hestOptAdd_3_Int(&opt, "d", "dimV dimG dimH", dim, "256 256 256",
                   "Dimensions of histogram volume; number of samples along "
                   "each axis");
  hestOptAdd_1_Other(&opt, "k00", "kernel", &ksp00, "tent",
                     "value reconstruction kernel", nrrdHestKernelSpec);
  hestOptAdd_1_Other(&opt, "k11", "kernel", &ksp11, "cubicd:1,0",
                     "first derivative kernel", nrrdHestKernelSpec);
  hestOptAdd_1_Other(&opt, "k22", "kernel", &ksp22, "cubicdd:1,0",
                     "second derivative kernel", nrrdHestKernelSpec);
  hestOptAdd_Flag(&opt, "l", &lapl,
                  "Use Laplacian instead of Hessian to approximate second "
                  "directional derivative.  No faster, less accurate.");
  hestOptAdd_Flag(&opt, "slow", &slow,
                  "Instead of allocating a floating point VGH volume and measuring "
                  "V,G,H once, measure V,G,H multiple times on separate passes "
                  "(slower, but needs less memory)");
  if (nrrdEncodingGzip->available()) {
    hestOptAdd_Flag(&opt, "gz", &gz,
                    "Use gzip compression for output histo-volume; "
                    "much less disk space, slightly slower to read/write");
  }
  hestOptAdd_1_Other(&opt, "i", "volumeIn", &nin, NULL,
                     "input scalar volume for which a transfer function is needed",
                     nrrdHestNrrd);
  hestOptAdd_1_String(&opt, "o", "hvolOut", &out, NULL,
                      "output histogram volume, used by \"gkms scat\" and "
                      "\"gkms info\"");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_baneGkms_hvolInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  nio = nrrdIoStateNew();
  airMopAdd(mop, nio, (airMopper)nrrdIoStateNix, airMopAlways);
  hvp = baneHVolParmNew();
  airMopAdd(mop, hvp, (airMopper)baneHVolParmNix, airMopAlways);
  baneHVolParmGKMSInit(hvp);
  hvp->makeMeasrVol = !slow;

  fprintf(stderr, "!%s: need to be using baneHVolParmAxisSet\n", me);
  /*
  hvp->axis[0].res = dim[perm[0]];
  hvp->axis[1].res = dim[perm[1]];
  hvp->axis[2].res = dim[perm[2]];
  hvp->axis[1].measr = lapl ? baneMeasrLapl : baneMeasrHess;
  for (i=0; i<=2; i++) {
    hvp->ax[i].inc = baneIncArray[(int)inc[(1+BANE_INC_PARM_NUM)*perm[i]]];
    for (j=0; j<BANE_INC_PARM_NUM; j++) {
      hvp->ax[i].incParm[j] = inc[1 + j + (1+BANE_INC_PARM_NUM)*perm[i]];
    }
  }
  */
  hvp->k3pack = AIR_TRUE;
  nrrdKernelParmSet(&hvp->k[gageKernel00], hvp->kparm[gageKernel00], ksp00);
  nrrdKernelParmSet(&hvp->k[gageKernel11], hvp->kparm[gageKernel11], ksp11);
  nrrdKernelParmSet(&hvp->k[gageKernel22], hvp->kparm[gageKernel22], ksp22);
  if (baneMakeHVol(nout, nin, hvp)) {
    biffAddf(BANE, "%s: trouble making histogram volume", me);
    airMopError(mop);
    return 1;
  }

  nio->encoding = gz ? nrrdEncodingGzip : nrrdEncodingRaw;
  if (nrrdSave(out, nout, nio)) {
    biffMovef(BANE, NRRD, "%s: error saving histogram volume", me);
    airMopError(mop);
    return 1;
  }
  airMopOkay(mop);
  return 0;
}
BANE_GKMS_CMD(hvol, HVOL_INFO);
