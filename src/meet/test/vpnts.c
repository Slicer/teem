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

#include <teem/pull.h>
#include "../meet.h"

static const char *info =
  ("For the simple task of generating N locations inside a volume. As "
   "controlled by \"-m\", can be per voxel, uniform quasi-random, or "
   "or uniform random.");

int
main(int argc, const char **argv) {
  hestOpt *hopt=NULL;
  hestParm *hparm;
  airArray *mop;
  const char *me;

  char *err, *outS;
  Nrrd *nin, *npos, *nout;
  gageKind *kind;
  pullEnergySpec *enspR;
  meetPullInfo *minf[3];
  NrrdKernelSpec *k00, *k11, *k22;
  pullContext *pctx=NULL;
  int ret=0, verbose, method;
  unsigned int num, ss;
  double jitter;

  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);

  npos = nrrdNew();
  airMopAdd(mop, npos, (airMopper)nrrdNuke, airMopAlways);

  hparm->respFileEnable = AIR_TRUE;
  me = argv[0];

  /* these don't need to be visible on the command-line */
  enspR = pullEnergySpecNew();
  airMopAdd(mop, enspR, (airMopper)pullEnergySpecNix, airMopAlways);
  pullEnergySpecParse(enspR, "cotan");

  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nin, NULL,
             "input volume", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "k", "kind", airTypeOther, 1, 1, &kind, "scalar",
             "\"kind\" of volume (\"scalar\", \"vector\", "
             "\"tensor\", or \"dwi\")",
             NULL, NULL, meetHestGageKind);
  hestOptAdd(&hopt, "v", "verbosity", airTypeInt, 1, 1, &verbose, "0",
             "verbosity level");
  hestOptAdd(&hopt, "m", "method", airTypeEnum, 1, 1, &method, "ppv",
             "way of creating point locations. Can be:\n "
             "\b\bo \"ppv\": some points per-voxel (\"-n\" is how "
             "many per-voxel, can be N < -1 for every Nth voxel)\n "
             "\b\bo \"halton\": use halton sequence for quasi-random "
             "(\"-s\" gives initial value)\n "
             "\b\bo \"random\": use uniform random positions "
             "(\"-s\" gives RNG seed)",
             NULL, pullInitMethod);
  hestOptAdd(&hopt, "n", "#points or #ppv", airTypeUInt, 1, 1, &num, "1",
             "number of points to initialize with with random and halton, "
             "or, number points per voxel with ppv");
  hestOptAdd(&hopt, "s", "seed or start", airTypeUInt, 1, 1, &ss, "1",
             "random number seed for random and ppv, or (hack), start "
             "index for Halton-based sampling");
  hestOptAdd(&hopt, "jit", "jitter", airTypeDouble, 1, 1, &jitter, "1",
             "amount of jittering to do with ppv");
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "out.nrrd",
             "filename for saving positions");
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
                 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  /* other parms that we set just to keep pull happy */
  enspR = pullEnergySpecNew();
  airMopAdd(mop, enspR, (airMopper)pullEnergySpecNix, airMopAlways);
  k00 = nrrdKernelSpecNew();
  airMopAdd(mop, k00, (airMopper)nrrdKernelSpecNix, airMopAlways);
  k11 = nrrdKernelSpecNew();
  airMopAdd(mop, k11, (airMopper)nrrdKernelSpecNix, airMopAlways);
  k22 = nrrdKernelSpecNew();
  airMopAdd(mop, k22, (airMopper)nrrdKernelSpecNix, airMopAlways);
  if (pullEnergySpecParse(enspR, "cotan")) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble setting up faux energies:\n%s", me, err);
    airMopError(mop); return 1;
  }
  if (nrrdKernelSpecParse(k00, "box")
      || nrrdKernelSpecParse(k11, "zero")
      || nrrdKernelSpecParse(k22, "zero")) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble setting up faux kernels:\n%s", me, err);
    airMopError(mop); return 1;
  }
#define VNAME "bingo"

  pctx = pullContextNew();
  airMopAdd(mop, pctx, (airMopper)pullContextNix, airMopAlways);
  minf[0] = meetPullInfoNew();
  airMopAdd(mop, minf[0], (airMopper)*meetPullInfoNix, airMopAlways);
  minf[1] = meetPullInfoNew();
  airMopAdd(mop, minf[1], (airMopper)*meetPullInfoNix, airMopAlways);
  minf[2] = meetPullInfoNew();
  airMopAdd(mop, minf[2], (airMopper)*meetPullInfoNix, airMopAlways);
  int E = pullVerboseSet(pctx, verbose);
  if (pullInitMethodRandom == method) {
    if (!E) E |= pullRngSeedSet(pctx, ss);
    if (!E) E |= pullInitRandomSet(pctx, num);
  } else if (pullInitMethodHalton == method) {
    if (!E) E |= pullInitHaltonSet(pctx, num, ss);
  } else if (pullInitMethodPointPerVoxel == method) {
    if (!E) E |= pullRngSeedSet(pctx, ss);
    if (!E) E |= pullInitPointPerVoxelSet(pctx, num, 1, 0, 0, jitter);
  } else if (pullInitMethodGivenPos == method) {
    fprintf(stderr, "%s: this utility is for making point positions; "
            "init method %s not available", me,
            airEnumStr(pullInitMethod, method));
    airMopError(mop); return 1;
  } else {
    fprintf(stderr, "%s: unsupported %s %s\n", me, pullInitMethod->name,
            airEnumStr(pullInitMethod, method));
    airMopError(mop); return 1;
  }
  if (E
      || pullFlagSet(pctx, pullFlagNixAtVolumeEdgeSpaceInitRorH, AIR_TRUE)
      || pullInterEnergySet(pctx, pullInterTypeJustR,
                            enspR, NULL, NULL)
      || pullVolumeSingleAdd(pctx, kind, VNAME, nin,
                             k00, k11, k22)) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble starting system:\n%s", me, err);
    airMopError(mop); return 1;
  }
  /* figure out how big a voxel is, so that the bins set up don't
     get overflowed.  The fact that there is binning happening
     at all is a sign that we shouldn't have to rely on so much
     pull infrastructure just to figure out this sampling ... */
  const double *spc = pctx->vol[0]->gctx->shape->spacing;
  double vlen = (spc[0] + spc[1] + spc[2])/3;
  /* "<info>[-c]:<volname>:<item>[:<zero>:<scale>]" */
  if (meetPullInfoParse(minf[0], "h:" VNAME ":val:0:1")
      || meetPullInfoParse(minf[1], "hgvec:" VNAME ":gvec")
      /* can you see the hack on the next line */
      || meetPullInfoParse(minf[2], "sthr:" VNAME ":val:-88888888:1")
      || meetPullInfoAddMulti(pctx, minf, 3)) {
    airMopAdd(mop, err = biffGetDone(MEET), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble setting up faux info:\n%s", me, err);
    airMopError(mop); return 1;
  }
  if (pullSysParmSet(pctx, pullSysParmRadiusSpace, vlen)
      || pullStart(pctx)
      || pullOutputGet(npos, NULL, NULL, NULL, 0.0, pctx)) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble starting or getting output:\n%s", me, err);
    airMopError(mop); return 1;
  }
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  size_t cmin[2] = {0,0};
  size_t cmax[2] = {2,npos->axis[1].size-1};
  if (nrrdCrop(nout, npos, cmin, cmax)
      || nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble saving output:\n%s", me, err);
    airMopError(mop); return 1;
  }

  pullFinish(pctx);
  airMopOkay(mop);
  return ret;
}
