/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

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


#include "../push.h"

char *info = ("Test program for push library.");

int
main(int argc, char *argv[]) {
  char *me, *err;
  hestOpt *hopt=NULL;
  airArray *mop;
  
  char *outS;
  int numIters, numThread, numBatch, ptsPerBatch, snap;
  pushContext *pctx;
  Nrrd *nin, *nPosOut;
  double step, drag, minMeanVel;
  NrrdKernelSpec *kk;
  
  mop = airMopNew();
  me = argv[0];
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nin, "",
             "input volume to filter", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "iter", "# iters", airTypeInt, 1, 1, &numIters, "5",
             "number of iterations to do processing for");
  hestOptAdd(&hopt, "dt", "step", airTypeDouble, 1, 1, &step, "0.01",
             "time step in integration");
  hestOptAdd(&hopt, "drag", "drag", airTypeDouble, 1, 1, &drag, "0.01",
             "amount of drag");
  hestOptAdd(&hopt, "mmv", "mean vel", airTypeDouble, 1, 1, &minMeanVel,
             "0.1", "minimum mean velocity that signifies convergence");
  hestOptAdd(&hopt, "snap", "iters", airTypeInt, 1, 1, &snap, "0",
             "if non-zero, number of iterations between which a snapshot "
             "is saved");
  hestOptAdd(&hopt, "nt", "# threads", airTypeInt, 1, 1, &numThread, "5",
             "number of threads to run");
  hestOptAdd(&hopt, "nb", "# batches", airTypeInt, 1, 1, &numBatch, "5",
             "number of batches of points to use in simulation");
  hestOptAdd(&hopt, "ppb", "pts/batch", airTypeInt, 1, 1, &ptsPerBatch, "5",
             "number of points to put in each batch");
  hestOptAdd(&hopt, "k", "kernel", airTypeOther, 1, 1, &kk,
             "tent", "kernel for tensor field sampling",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "tmp.nrrd",
             "output file to save filtering result into");
  hestParseOrDie(hopt, argc-1, argv+1, NULL,
                 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  pctx = pushContextNew();
  airMopAdd(mop, pctx, (airMopper)pushContextNix, airMopAlways);
  nPosOut = nrrdNew();
  airMopAdd(mop, nPosOut, (airMopper)nrrdNuke, airMopAlways);
  
  pctx->nin = nin;
  pctx->numThread = numThread;
  pctx->drag = drag;
  pctx->step = step;
  pctx->minMeanVel = minMeanVel;
  pctx->snap = snap;
  pctx->numBatch = numBatch;
  pctx->pointsPerBatch = ptsPerBatch;
  pctx->numStage = 2;
  pctx->verbose = 0;
  pctx->kernel = kk->kernel;
  memcpy(pctx->kparm, kk->parm, NRRD_KERNEL_PARMS_NUM*sizeof(double));

  if (pushStart(pctx)
      || pushRun(pctx)
      || pushOutputGet(nPosOut, NULL, pctx)
      || pushFinish(pctx)) {
    airMopAdd(mop, err = biffGetDone(PUSH), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); 
    return 1;
  }
  fprintf(stderr, "%s: time to compute = %g secs\n", me, pctx->time);
  if (nrrdSave(outS, nPosOut, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: couldn't save output:\n%s\n", me, err);
    airMopError(mop); 
    return 1;
  }
  airMopOkay(mop);
  return 0;
}

