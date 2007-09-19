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

#include "ten.h"
#include "privateTen.h"

#define INFO "Extract a single fiber tract, given a start point"
char *_tend_fiberInfoL =
  (INFO
   ".  This is really only useful for debugging purposes; there is no way "
   "that a command-line tractography tool is going to be very interesting "
   "for real applications. The output fiber is in the form "
   "of a 3xN array of doubles, with N points along fiber.");

int
tend_fiberMain(int argc, char **argv, char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;
  char *outS;

  tenFiberContext *tfx;
  tenFiberSingle *tfbs, *fiber;
  NrrdKernelSpec *ksp;
  double start[3], step, *_stop, *stop;
  airEnum *ftypeEnum;
  char *ftypeS;
  int E, intg, useDwi, allPaths, verbose, worldSpace, worldSpaceOut,
    ftype, ftypeDef;
  Nrrd *nin, *nseed;
  unsigned int si, stopLen, whichPath;
  airArray *fiberArr;
  limnPolyData *fiberPld;

  hestOptAdd(&hopt, "wsp", NULL, airTypeInt, 0, 0, &worldSpace, NULL,
             "define seedpoint and output path in worldspace.  Otherwise, "
             "(without using this option) everything is in index space");
  hestOptAdd(&hopt, "wspo", NULL, airTypeInt, 0, 0, &worldSpaceOut, NULL,
             "output should be in worldspace, even if input is not "
             "(this is confusing)");
  hestOptAdd(&hopt, "s", "seed point", airTypeDouble, 3, 3, start, "0 0 0",
             "seed point for fiber; it will propogate in two opposite "
             "directions starting from here");
  hestOptAdd(&hopt, "ns", "seed nrrd", airTypeOther, 1, 1, &nseed, "",
             "3-by-N nrrd of seedpoints", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "step", "step size", airTypeDouble, 1, 1, &step, "0.01",
             "stepsize along fiber, in world space");
  hestOptAdd(&hopt, "stop", "stop1", airTypeOther, 1, -1, &_stop, NULL,
             "the conditions that should signify the end of a fiber. "
             "Multiple stopping criteria are logically OR-ed and tested at "
             "every point along the fiber.  Possibilities include:\n "
             "\b\bo \"aniso:<type>,<thresh>\": require anisotropy to be "
             "above the given threshold.  Which anisotropy type is given "
             "as with \"tend anvol\" (see its usage info)\n "
             "\b\bo \"len:<length>\": limits the length, in world space, "
             "of each fiber half\n "
             "\b\bo \"steps:<N>\": limits the number of points in each "
             "fiber half\n "
             "\b\bo \"conf:<thresh>\": requires the tensor confidence value "
             "to be above the given threshold. ",
             &stopLen, NULL, tendFiberStopCB);
  hestOptAdd(&hopt, "t", "type", airTypeString, 1, 1, &ftypeS, "", 
             "fiber type; defaults to something");
  hestOptAdd(&hopt, "n", "intg", airTypeEnum, 1, 1, &intg, "rk4",
             "integration method for fiber tracking",
             NULL, tenFiberIntg);
  hestOptAdd(&hopt, "k", "kernel", airTypeOther, 1, 1, &ksp, "tent",
             "kernel for reconstructing tensor field",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nin, "-",
             "input volume", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "dwi", NULL, airTypeInt, 0, 0, &useDwi, NULL,
             "input volume is a DWI volume, not a single tensor volume");
  hestOptAdd(&hopt, "wp", "which", airTypeUInt, 1, 1, &whichPath, "0",
             "when doing multi-tensor tracking, index of path to follow "
             "(made moot by \"-ap\")");
  hestOptAdd(&hopt, "ap", "allpaths", airTypeInt, 0, 0, &allPaths, NULL,
             "follow all paths from seedpoint(s), output will be "
             "polydata, rather than a single 3-by-N nrrd");
  hestOptAdd(&hopt, "v", "verbose", airTypeInt, 1, 1, &verbose, "0",
             "verbosity level");
  hestOptAdd(&hopt, "o", "out", airTypeString, 1, 1, &outS, "-",
             "output fiber(s)");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_fiberInfoL);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  tfbs = tenFiberSingleNew();
  airMopAdd(mop, tfbs, (airMopper)tenFiberSingleNix, airMopAlways);

  if (useDwi) {
    tfx = tenFiberContextDwiNew(nin, 50, 1, 1,
                                tenEstimate1MethodLLS,
                                tenEstimate2MethodPeled);
  } else {
    tfx = tenFiberContextNew(nin);
  }
  if (!tfx) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: failed to create the fiber context:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, tfx, (airMopper)tenFiberContextNix, airMopAlways);
  E = 0;
  for (si=0, stop=_stop; si<stopLen; si++, stop+=3) {
    switch(AIR_CAST(int, stop[0])) {
    case tenFiberStopAniso:
      if (!E) E |= tenFiberStopSet(tfx, tenFiberStopAniso,
                                   (int)stop[1], stop[2]);
      break;
    case tenFiberStopNumSteps:
      if (!E) E |= tenFiberStopSet(tfx, tenFiberStopNumSteps, (int)stop[1]);
      break;
    case tenFiberStopLength:
    case tenFiberStopConfidence:
    case tenFiberStopFraction:
    case tenFiberStopRadius:
      if (!E) E |= tenFiberStopSet(tfx, AIR_CAST(int, stop[0]), stop[1]);
      break;
    case tenFiberStopBounds:
      /* nothing to actually do */
      break;
    default:
      fprintf(stderr, "%s: stop method %d not supported\n", me,
              AIR_CAST(int, stop[0]));
      airMopError(mop); return 1;
      break;
    }
  }
  if (!E) {
    if (useDwi) {
      ftypeEnum = tenDwiFiberType;
      ftypeDef = tenDwiFiberType2Evec0;
    } else {
      ftypeEnum = tenFiberType;
      ftypeDef = tenFiberTypeEvec0;
    }
    if (airStrlen(ftypeS)) {
      ftype = airEnumVal(ftypeEnum, ftypeS);
      if (airEnumUnknown(ftypeEnum) == ftype) {
        fprintf(stderr, "%s: couldn't parse \"%s\" as a %s\n", me,
                ftypeS, ftypeEnum->name);
        airMopError(mop); return 1;
      }
    } else {
      ftype = ftypeDef;
      fprintf(stderr, "%s: (defaulting %s to %s)\n", me,
              ftypeEnum->name, airEnumStr(ftypeEnum, ftype));
    }
    E |= tenFiberTypeSet(tfx, ftype);
  }
  if (!E) E |= tenFiberKernelSet(tfx, ksp->kernel, ksp->parm);
  if (!E) E |= tenFiberIntgSet(tfx, intg);
  if (!E) E |= tenFiberParmSet(tfx, tenFiberParmStepSize, step);
  if (!E) E |= tenFiberParmSet(tfx, tenFiberParmUseIndexSpace,
                               worldSpace ? AIR_FALSE: AIR_TRUE);
  if (!E) E |= tenFiberUpdate(tfx);
  if (E) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  tfx->verbose = verbose;
  if (!allPaths) {
    if (tenFiberSingleTrace(tfx, tfbs, start, whichPath)) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble:\n%s\n", me, err);
      airMopError(mop); return 1;
    }
    if (tenFiberStopUnknown == tfx->whyNowhere) {
      fprintf(stderr, "%s: steps[back,forw] = %u,%u; seedIdx = %u\n", me,
              tfbs->stepNum[0], tfbs->stepNum[1], tfbs->seedIdx);
      fprintf(stderr, "%s: whyStop[back,forw] = %s,%s\n", me,
              airEnumStr(tenFiberStop, tfbs->whyStop[0]),
              airEnumStr(tenFiberStop, tfbs->whyStop[1]));
      if (worldSpaceOut && !worldSpace) {
        /* have to convert output to worldspace */
        fprintf(stderr, "%s: WARNING!!! output conversion "
                "to worldspace not done!!!\n", me);
      }
      if (nrrdSave(outS, tfbs->nvert, NULL)) {
        airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
        airMopError(mop); return 1;
      }
    } else {
      fprintf(stderr, "%s: fiber failed to start: %s.\n",
              me, airEnumDesc(tenFiberStop, tfx->whyNowhere));
    }
  } else {
    if (!nseed) {
      fprintf(stderr, "%s: didn't get seed nrrd via \"-ns\"\n", me);
      airMopError(mop); return 1;
    }
    fiberArr = airArrayNew(AIR_CAST(void **, &fiber), NULL,
                           sizeof(tenFiberSingle), 1024 /* incr */);
    airArrayStructCB(fiberArr,
                     AIR_CAST(void (*)(void *), tenFiberSingleInit),
                     AIR_CAST(void (*)(void *), tenFiberSingleDone));
    airMopAdd(mop, fiberArr, (airMopper)airArrayNuke, airMopAlways);
    fiberPld = limnPolyDataNew();
    airMopAdd(mop, fiberPld, (airMopper)limnPolyDataNix, airMopAlways);
    if (tenFiberMultiTrace(tfx, fiberArr, nseed)
        || tenFiberMultiPolyData(tfx, fiberPld, fiberArr)) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble:\n%s\n", me, err);
      airMopError(mop); return 1;
    }
    if (worldSpaceOut && !worldSpace) {
      /* have to convert output to worldspace */
      unsigned int ii;
      double index[4], world[3];
      for (ii=0; ii<fiberPld->xyzwNum; ii++) {
        ELL_4V_COPY(index, fiberPld->xyzw + 4*ii);
        ELL_4V_HOMOG(index, index);
        gageShapeItoW(tfx->gtx->shape, world, index);
        ELL_4V_COPY(fiberPld->xyzw + 4*ii, world);
        (fiberPld->xyzw + 4*ii)[3] = 1.0;
      }
    }
    if (limnPolyDataSave(outS, fiberPld)) {
      airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble saving:\n%s\n", me, err);
      airMopError(mop); return 1;
    }
  }

  airMopOkay(mop);
  return 0;
}
/* TEND_CMD(fiber, INFO); */
unrrduCmd tend_fiberCmd = { "fiber", INFO, tend_fiberMain };
