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


#include "../ten.h"

char *info = ("Compute the makings of a new tensor.dat file.");

int
main(int argc, char *argv[]) {
  char *me, *err;
  hestOpt *hopt=NULL;
  airArray *mop;

  int E, optimizeAngle;
  unsigned int ii, minNum, maxNum;
  double *log, minAngle, pot;
  char *outStr, logFilename[AIR_STRLEN_MED], gradFilename[AIR_STRLEN_MED],
    keyStr[AIR_STRLEN_MED], valStr[AIR_STRLEN_MED];
  tenGradientParm *tgparm;
  Nrrd *nlog, *ngrad;
  size_t size[2];
  
  mop = airMopNew();
  tgparm = tenGradientParmNew();
  airMopAdd(mop, tgparm, (airMopper)tenGradientParmNix, airMopAlways);
  tgparm->drag = 0.0;
  tgparm->charge = 1.0;
  tgparm->single = AIR_FALSE;
  tgparm->descent = AIR_TRUE;
  tgparm->snap = 0;
  tgparm->minMeanImprovement = 0.0;

  nlog = nrrdNew();
  airMopAdd(mop, nlog, (airMopper)nrrdNuke, airMopAlways);
  ngrad = nrrdNew();
  airMopAdd(mop, ngrad, (airMopper)nrrdNuke, airMopAlways);
  
  me = argv[0];
  hestOptAdd(&hopt, "min", "min #", airTypeUInt, 1, 1, &minNum, "6",
             "minimum number of gradients to be computed");
  hestOptAdd(&hopt, "max", "max #", airTypeUInt, 1, 1, &maxNum, "129",
             "maximum number of gradients to be computed");
  hestOptAdd(&hopt, "p", "exponent", airTypeUInt, 1, 1, &(tgparm->expo), "1",
             "the exponent p that defines the 1/r^p potential energy "
             "(Coulomb is 1)");
  hestOptAdd(&hopt, "dt", "dt", airTypeDouble, 1, 1, &(tgparm->dt), "1",
             "time increment in solver");
  hestOptAdd(&hopt, "maxiter", "# iters", airTypeInt, 1, 1,
             &(tgparm->maxIteration), "1000000",
             "max number of iterations for which to run the simulation");
  hestOptAdd(&hopt, "minvelo", "vel", airTypeDouble, 1, 1, 
             &(tgparm->minVelocity), "0.00000000001",
             "low threshold on mean velocity of repelling points, "
             "at which point repulsion phase of algorithm terminates.");
  hestOptAdd(&hopt, "oa", NULL, airTypeInt, 0, 0, &optimizeAngle, NULL,
             "optimize for the maximal minimal angle, instead of potential.");
  hestOptAdd(&hopt, "dp", "potential change", airTypeDouble, 1, 1, 
             &(tgparm->minPotentialChange), "0.00000000001",
             "low threshold on fractional change of potential at "
             "which point repulsion phase of algorithm terminates.");
  hestOptAdd(&hopt, "minmean", "len", airTypeDouble, 1, 1,
             &(tgparm->minMean), "0.001",
             "if length of mean gradient falls below this, finish "
             "the balancing phase");
  hestOptAdd(&hopt, "odir", "out", airTypeString, 1, 1, &outStr, ".",
             "output directory for all grad files and logs");
  hestParseOrDie(hopt, argc-1, argv+1, NULL,
                 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  /* see if we can open the log */
  sprintf(logFilename, "%s/000-%03u-log.nrrd", outStr, tgparm->expo);
  if (nrrdLoad(nlog, logFilename, NULL)) {
    /* no, we couldn't load it, and we don't care why */
    free(biffGetDone(NRRD));
    /* create a log nrrd of the correct size */
    size[0] = 6;
    size[1] = maxNum+1;
    if (nrrdMaybeAlloc_nva(nlog, nrrdTypeDouble, 2, size)) {
      airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble making log:\n%s\n", me, err);
      airMopError(mop); return 1;
    }
  } else {
    /* we could open the log, see if its the right size */
    if (!( nrrdTypeDouble == nlog->type
           && 2 == nlog->dim
           && 6 == nlog->axis[0].size
           && maxNum+1 == nlog->axis[1].size )) {
      fprintf(stderr, "%s: given log (%s %u-D %ux%ux?) doesn't match "
              "desired (%s 2-D 6x%u)\n", me,
              airEnumStr(nrrdType, nlog->type),
              nlog->dim, 
              AIR_CAST(unsigned int, nlog->axis[0].size),
              AIR_CAST(unsigned int, nlog->axis[1].size),
              airEnumStr(nrrdType, nrrdTypeDouble),
              maxNum+1);
      airMopError(mop); return 1;
    }
  }
  /* nlog is the right size */
  /* initialize log's first column and key/value pairs, and (re)save */
  log = AIR_CAST(double *, nlog->data);
  for (ii=minNum; ii<=maxNum; ii++) {
    log[0 + 6*ii] = ii;
  }
  E = 0;
  if (!E) strcpy(keyStr, "maxiter");
  if (!E) sprintf(valStr, "%d", tgparm->maxIteration);
  if (!E) E |= nrrdKeyValueAdd(nlog, keyStr, valStr);
  if (!E) strcpy(keyStr, "dt");
  if (!E) sprintf(valStr, "%g", tgparm->dt);
  if (!E) E |= nrrdKeyValueAdd(nlog, keyStr, valStr);
  if (!E) strcpy(keyStr, "dp");
  if (!E) sprintf(valStr, "%g", tgparm->minPotentialChange);
  if (!E) E |= nrrdKeyValueAdd(nlog, keyStr, valStr);
  if (!E) strcpy(keyStr, "minvelo");
  if (!E) sprintf(valStr, "%g", tgparm->minVelocity);
  if (!E) E |= nrrdKeyValueAdd(nlog, keyStr, valStr);
  if (!E) strcpy(keyStr, "minmean");
  if (!E) sprintf(valStr, "%g", tgparm->minMean);
  if (!E) E |= nrrdKeyValueAdd(nlog, keyStr, valStr);
  if (!E) E |= nrrdSave(logFilename, nlog, NULL);
  if (E) {
    airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing log:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  /* in master log (per gradient set):
     0: # grads
     1: last seed tried
     2: seed of best so far
     3: best phi
     4: best minAngle
     5: iters used
  */
  while (1) {
    for (ii=minNum; ii<=maxNum; ii++) {
      tgparm->seed = 1 + AIR_CAST(unsigned int, log[1 + 6*ii]);
      fprintf(stderr, "%s ================ %u %u\n", me, ii, tgparm->seed);
      if (tenGradientGenerate(ngrad, ii, tgparm)) {
        airMopAdd(mop, err=biffGetDone(TEN), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble making distribution:\n%s\n", me, err);
        airMopError(mop); return 1;
      }
      tenGradientMeasure(&minAngle, &pot, ngrad, tgparm);
      if (1 == tgparm->seed 
          || ((optimizeAngle && minAngle > log[4 + 6*ii])
              || pot < log[3 + 6*ii])) {
        /* this gradient set is best so far */
        log[2 + 6*ii] = tgparm->seed;
        log[3 + 6*ii] = pot;
        log[4 + 6*ii] = minAngle;
        log[5 + 6*ii] = tgparm->itersUsed;
        sprintf(gradFilename, "%s/%03u-%03u.nrrd", outStr, ii, tgparm->expo);
        if (nrrdSave(gradFilename, ngrad, NULL)) {
          airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
          fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
          airMopError(mop); return 1;
        }
      }
      log[1 + 6*ii] = tgparm->seed;
      if (nrrdSave(logFilename, nlog, NULL)) {
        airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble writing log:\n%s\n", me, err);
        airMopError(mop); return 1;
      }
    }
  }   

  /* to record in each gradient set:
     parms used: -dp, 
  */

  airMopOkay(mop);
  return 0;
}
