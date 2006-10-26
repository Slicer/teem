/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
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


#include "../push.h"

char *info = ("Test program for push library.");

int
main(int argc, char *argv[]) {
  char *me, *err;
  hestOpt *hopt=NULL;
  airArray *mop;
  
  char *outS[2];
  char *gravStr, *gravGradStr;
  pushContext *pctx;
  Nrrd *_nin, *nin, *nPosIn, *nPosOut, *nTenOut;
  NrrdKernelSpec *ksp00, *ksp11;
  pushEnergySpec *ensp;
  
  me = argv[0];

  mop = airMopNew();
  pctx = pushContextNew();
  airMopAdd(mop, pctx, (airMopper)pushContextNix, airMopAlways);

  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &_nin, NULL,
             "input volume to filter", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "np", "# points", airTypeUInt, 1, 1,
             &(pctx->pointNum), "1000",
             "number of points to use in simulation");
  hestOptAdd(&hopt, "pi", "npos", airTypeOther, 1, 1, &nPosIn, "",
             "positions to start at (overrides \"-np\")",
             NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "step", "step", airTypeDouble, 1, 1,
             &(pctx->step), "1",
             "stepsize for gradient descent");
  hestOptAdd(&hopt, "scl", "scale", airTypeDouble, 1, 1,
             &(pctx->scale), "1500",
             "scaling from tensor size to glyph size");
  hestOptAdd(&hopt, "wall", "wall", airTypeDouble, 1, 1,
             &(pctx->wall), "0.0",
             "spring constant of containing walls");
  hestOptAdd(&hopt, "cnts", "scale", airTypeDouble, 1, 1, 
             &(pctx->cntScl), "0.0",
             "scaling of containment force");
  hestOptAdd(&hopt, "detr", NULL, airTypeBool, 0, 0, 
             &(pctx->detReject), NULL,
             "do determinant-based rejection of initial sample locations");
  hestOptAdd(&hopt, "rng", "seed", airTypeUInt, 1, 1, 
             &(pctx->seedRNG), "42",
             "seed value for RNG which determines initial point locations");
  hestOptAdd(&hopt, "nt", "# threads", airTypeUInt, 1, 1,
             &(pctx->threadNum), "1",
             "number of threads to run");
  hestOptAdd(&hopt, "nskip", "# iters", airTypeUInt, 1, 1,
             &(pctx->iterNeighbor), "0",
             "only do full neighbor traversal on iterations "
             "that are multiples of this");
  hestOptAdd(&hopt, "pskip", "# iters", airTypeUInt, 1, 1,
              &(pctx->iterProbe), "0",
             "only do field probing on iterations that are multiples of this");
  hestOptAdd(&hopt, "maxi", "# iters", airTypeUInt, 1, 1,
             &(pctx->maxIter), "0",
             "if non-zero, max # iterations to run");
  hestOptAdd(&hopt, "snap", "iters", airTypeUInt, 1, 1, &(pctx->snap), "0",
             "if non-zero, # iterations between which a snapshot "
             "is saved");

  hestOptAdd(&hopt, "grv", "item", airTypeString, 1, 1, &gravStr, "none",
             "item to act as gravity");
  hestOptAdd(&hopt, "grvgv", "item", airTypeString, 1, 1, &gravGradStr, "none",
             "item to act as gravity gradient");
  hestOptAdd(&hopt, "grvs", "scale", airTypeDouble, 1, 1, &(pctx->gravScl),
             "nan", "magnitude and scaling of gravity vector");
  hestOptAdd(&hopt, "grvz", "scale", airTypeDouble, 1, 1, &(pctx->gravZero),
             "nan", "height (WRT gravity) of zero potential energy");

  hestOptAdd(&hopt, "energy", "spec", airTypeOther, 1, 1, &ensp, "cotan",
             "specification of energy function to use",
             NULL, NULL, pushHestEnergySpec);

  hestOptAdd(&hopt, "nobin", NULL, airTypeBool, 0, 0,
             &(pctx->binSingle), NULL,
             "turn off spatial binning (which prevents multi-threading "
             "from being useful), for debugging or speed-up measurement");

  hestOptAdd(&hopt, "k00", "kernel", airTypeOther, 1, 1, &ksp00,
             "tent", "kernel for tensor field sampling",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k11", "kernel", airTypeOther, 1, 1, &ksp11,
             "fordif", "kernel for finding containment gradient from mask",
             NULL, NULL, nrrdHestKernelSpec);

  hestOptAdd(&hopt, "o", "nout", airTypeString, 2, 2, outS, "p.nrrd t.nrrd",
             "output files to save position and tensor info into");

  hestParseOrDie(hopt, argc-1, argv+1, NULL,
                 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nPosOut = nrrdNew();
  airMopAdd(mop, nPosOut, (airMopper)nrrdNuke, airMopAlways);
  nTenOut = nrrdNew();
  airMopAdd(mop, nTenOut, (airMopper)nrrdNuke, airMopAlways);
  
  if (3 == _nin->spaceDim && AIR_EXISTS(_nin->measurementFrame[0][0])) {
    nin = nrrdNew();
    airMopAdd(mop, nin, (airMopper)nrrdNuke, airMopAlways);
    if (tenMeasurementFrameReduce(nin, _nin)) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble undoing measurement frame:\n%s", me, err);
      airMopError(mop);
      exit(1);
    }
  } else {
    nin = _nin;
  }

  pctx->nin = nin;
  pctx->npos = nPosIn;
  pctx->verbose = 0;
  pctx->binIncr = 84;  /* random small-ish value */
  pushEnergySpecSet(pctx->ensp, ensp->energy, ensp->parm);
  nrrdKernelSpecSet(pctx->ksp00, ksp00->kernel, ksp00->parm);
  nrrdKernelSpecSet(pctx->ksp11, ksp11->kernel, ksp11->parm);
  if (strcmp("none", gravStr)) {
    pctx->gravItem = airEnumVal(tenGage, gravStr);
    if (tenGageUnknown == pctx->gravItem) {
      fprintf(stderr, "%s: couldn't parse \"%s\" as a %s (gravity)\n", me,
              gravStr, tenGage->name);
      airMopError(mop);
      return 1;
    }
    pctx->gravGradItem = airEnumVal(tenGage, gravGradStr);
    if (tenGageUnknown == pctx->gravGradItem) {
      fprintf(stderr, "%s: couldn't parse \"%s\" as a %s (gravity grad)\n",
              me, gravGradStr, tenGage->name);
      airMopError(mop);
      return 1;
    }
  } else {
    pctx->gravItem = tenGageUnknown;
    pctx->gravGradItem = tenGageUnknown;
    pctx->gravZero = AIR_NAN;
    pctx->gravScl = AIR_NAN;
  }

  if (pushStart(pctx)
      || pushRun(pctx)
      || pushOutputGet(nPosOut, nTenOut, NULL, pctx)
      || pushFinish(pctx)) {
    airMopAdd(mop, err = biffGetDone(PUSH), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); 
    return 1;
  }
  fprintf(stderr, "%s: time for %d iterations= %g secs\n",
          me, pctx->iter, pctx->timeRun);
  if (nrrdSave(outS[0], nPosOut, NULL)
      || nrrdSave(outS[1], nTenOut, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: couldn't save output:\n%s\n", me, err);
    airMopError(mop); 
    return 1;
  }
  airMopOkay(mop);
  return 0;
}

