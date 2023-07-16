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

#include "ten.h"
#include "privateTen.h"

#define INFO "Fiber tractography, from one or more seeds"
static const char *_tend_fiberInfoL
  = (INFO ".  A fairly complete command-line interface to the tenFiber API.");

static int
tend_fiberMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;
  char *outS;

  tenFiberContext *tfx;
  tenFiberSingle *tfbs;
  NrrdKernelSpec *ksp;
  double start[3], step, *_stop, *stop;
  const airEnum *ftypeEnum;
  char *ftypeS;
  int E, intg, useDwi, allPaths, verbose, worldSpace, worldSpaceOut, ftype, ftypeDef;
  Nrrd *nin, *nseed, *nmat, *_nmat;
  unsigned int si, stopLen, whichPath;
  double matx[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  tenFiberMulti *tfml;
  limnPolyData *fiberPld;

  hestOptAdd_1_Other(&hopt, "i", "nin", &nin, "-", "input volume", nrrdHestNrrd);
  hestOptAdd_Flag(&hopt, "dwi", &useDwi,
                  "input volume is a DWI volume, not a single tensor volume");
  hestOptAdd_3_Double(&hopt, "s", "seed point", start, "0 0 0",
                      "seed point for fiber; it will propogate in two opposite "
                      "directions starting from here");
  hestOptAdd_1_Other(&hopt, "ns", "seed nrrd", &nseed, "", "3-by-N nrrd of seedpoints",
                     nrrdHestNrrd);
  hestOptAdd_Flag(&hopt, "wsp", &worldSpace,
                  "define seedpoint and output path in worldspace.  Otherwise, "
                  "(without using this option) everything is in index space");
  hestOptAdd_1_String(&hopt, "t", "type", &ftypeS, "",
                      "fiber type; defaults to something");
  hestOptAdd_1_Enum(&hopt, "n", "intg", &intg, "rk4",
                    "integration method for fiber tracking", tenFiberIntg);
  hestOptAdd_1_Other(&hopt, "k", "kernel", &ksp, "tent",
                     "kernel for reconstructing tensor field", nrrdHestKernelSpec);
  hestOptAdd_1_UInt(&hopt, "wp", "which", &whichPath, "0",
                    "when doing multi-tensor tracking, index of path to follow "
                    "(made moot by \"-ap\")");
  hestOptAdd_Flag(&hopt, "ap", &allPaths,
                  "follow all paths from (all) seedpoint(s), output will be "
                  "polydata, rather than a single 3-by-N nrrd, even if only "
                  "a single path is generated");
  hestOptAdd_Flag(&hopt, "wspo", &worldSpaceOut,
                  "output should be in worldspace, even if input is not "
                  "(this feature is unstable and/or confusing)");
  hestOptAdd_1_Double(&hopt, "step", "step size", &step, "0.01",
                      "stepsize along fiber, in world space");
  hestOptAdd_Nv_Other(&hopt, "stop", "stop1", 1, -1, &_stop, NULL,
                      "the conditions that should signify the end of a fiber, or "
                      "when to discard a fiber that is done propagating. "
                      "Multiple stopping criteria are logically OR-ed and tested at "
                      "every point along the fiber.  Possibilities include:\n "
                      "\b\bo \"aniso:<type>,<thresh>\": require anisotropy to be "
                      "above the given threshold.  Which anisotropy type is given "
                      "as with \"tend anvol\" (see its usage info)\n "
                      "\b\bo \"len:<length>\": limits the length, in world space, "
                      "of each fiber half\n "
                      "\b\bo \"steps:<N>\": the number of steps in each fiber half "
                      "is capped at N\n "
                      "\b\bo \"conf:<thresh>\": requires the tensor confidence value "
                      "to be above the given thresh\n "
                      "\b\bo \"radius:<thresh>\": requires that the radius of "
                      "curvature of the fiber stay above given thr\n "
                      "\b\bo \"frac:<F>\": in multi-tensor tracking, the fraction "
                      "of the tracked component must stay above F\n "
                      "\b\bo \"minlen:<len>\": discard fiber if its final whole "
                      "length is below len (not really a termination criterion)\n "
                      "\b\bo \"minsteps:<N>\": discard fiber if its final number of "
                      "steps is below N (not really a termination criterion)",
                      &stopLen, tendFiberStopCB);
  hestOptAdd_1_Int(&hopt, "v", "verbose", &verbose, "0", "verbosity level");
  hestOptAdd_1_Other(&hopt, "nmat", "transform", &_nmat, "",
                     "a 4x4 homogenous transform matrix (as a nrrd, or just a text "
                     "file) given with this option will be applied to the output "
                     "tractography vertices just prior to output",
                     nrrdHestNrrd);
  hestOptAdd_1_String(&hopt, "o", "out", &outS, "-", "output fiber(s)");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_tend_fiberInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  tfbs = tenFiberSingleNew();
  airMopAdd(mop, tfbs, (airMopper)tenFiberSingleNix, airMopAlways);

  if (_nmat) {
    if (!(2 == _nmat->dim && 4 == _nmat->axis[0].size && 4 == _nmat->axis[1].size)) {
      fprintf(stderr,
              "%s: transform matrix must be 2-D 4-by-4 array "
              "not %u-D %u-by-?\n",
              me, AIR_UINT(_nmat->dim), AIR_UINT(_nmat->axis[0].size));
      airMopError(mop);
      return 1;
    }
    nmat = nrrdNew();
    airMopAdd(mop, nmat, (airMopper)nrrdNuke, airMopAlways);
    if (nrrdConvert(nmat, _nmat, nrrdTypeDouble)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: problem with transform matrix\n%s\n", me, err);
      airMopError(mop);
      return 1;
    }
    ELL_4M_COPY(matx, AIR_CAST(double *, nmat->data));
    fprintf(stderr, "%s: transforming output by:\n", me);
    ell_4m_print_d(stderr, matx);
  }
  if (useDwi) {
    tfx = tenFiberContextDwiNew(nin, 50, 1, 1, tenEstimate1MethodLLS,
                                tenEstimate2MethodPeled);
  } else {
    tfx = tenFiberContextNew(nin);
  }
  if (!tfx) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: failed to create the fiber context:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }
  airMopAdd(mop, tfx, (airMopper)tenFiberContextNix, airMopAlways);
  E = 0;
  for (si = 0, stop = _stop; si < stopLen; si++, stop += 3) {
    int istop; /* not from Apple */
    istop = AIR_INT(stop[0]);
    switch (istop) {
    case tenFiberStopAniso:
      if (!E) E |= tenFiberStopSet(tfx, istop, AIR_INT(stop[1]), stop[2]);
      break;
    case tenFiberStopNumSteps:
    case tenFiberStopMinNumSteps:
      if (!E) E |= tenFiberStopSet(tfx, istop, AIR_UINT(stop[1]));
      break;
    case tenFiberStopLength:
    case tenFiberStopConfidence:
    case tenFiberStopFraction:
    case tenFiberStopRadius:
    case tenFiberStopMinLength:
      if (!E) E |= tenFiberStopSet(tfx, istop, stop[1]);
      break;
    case tenFiberStopBounds:
      /* nothing to actually do */
      break;
    default:
      fprintf(stderr, "%s: stop method %d not supported\n", me, istop);
      airMopError(mop);
      return 1;
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
        fprintf(stderr, "%s: couldn't parse \"%s\" as a %s\n", me, ftypeS,
                ftypeEnum->name);
        airMopError(mop);
        return 1;
      }
    } else {
      ftype = ftypeDef;
      fprintf(stderr, "%s: (defaulting %s to %s)\n", me, ftypeEnum->name,
              airEnumStr(ftypeEnum, ftype));
    }
    E |= tenFiberTypeSet(tfx, ftype);
  }
  if (!E) E |= tenFiberKernelSet(tfx, ksp->kernel, ksp->parm);
  if (!E) E |= tenFiberIntgSet(tfx, intg);
  if (!E) E |= tenFiberParmSet(tfx, tenFiberParmStepSize, step);
  if (!E)
    E |= tenFiberParmSet(tfx, tenFiberParmUseIndexSpace,
                         worldSpace ? AIR_FALSE : AIR_TRUE);
  if (!E) E |= tenFiberUpdate(tfx);
  if (E) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  tenFiberVerboseSet(tfx, verbose);
  if (!allPaths) {
    if (tenFiberSingleTrace(tfx, tfbs, start, whichPath)) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble:\n%s\n", me, err);
      airMopError(mop);
      return 1;
    }
    if (tenFiberStopUnknown == tfx->whyNowhere) {
      fprintf(stderr, "%s: steps[back,forw] = %u,%u; seedIdx = %u\n", me,
              tfbs->stepNum[0], tfbs->stepNum[1], tfbs->seedIdx);
      fprintf(stderr, "%s: whyStop[back,forw] = %s,%s\n", me,
              airEnumStr(tenFiberStop, tfbs->whyStop[0]),
              airEnumStr(tenFiberStop, tfbs->whyStop[1]));
      if (worldSpaceOut && !worldSpace) {
        /* have to convert output to worldspace */
        fprintf(stderr,
                "%s: WARNING!!! output conversion "
                "to worldspace not done!!!\n",
                me);
      }
      if (_nmat) {
        fprintf(stderr,
                "%s: WARNING!!! output transform "
                "not done!!!\n",
                me);
      }
      if (nrrdSave(outS, tfbs->nvert, NULL)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
        airMopError(mop);
        return 1;
      }
    } else {
      fprintf(stderr, "%s: fiber failed to start: %s.\n", me,
              airEnumDesc(tenFiberStop, tfx->whyNowhere));
    }
  } else {
    if (!nseed) {
      fprintf(stderr, "%s: didn't get seed nrrd via \"-ns\"\n", me);
      airMopError(mop);
      return 1;
    }
    tfml = tenFiberMultiNew();
    airMopAdd(mop, tfml, (airMopper)tenFiberMultiNix, airMopAlways);
    /*
    fiberArr = airArrayNew(AIR_CAST(void **, &fiber), NULL,
                           sizeof(tenFiberSingle), 1024);
    airArrayStructCB(fiberArr,
                     AIR_CAST(void (*)(void *), tenFiberSingleInit),
                     AIR_CAST(void (*)(void *), tenFiberSingleDone));
    airMopAdd(mop, fiberArr, (airMopper)airArrayNuke, airMopAlways);
    */
    fiberPld = limnPolyDataNew();
    airMopAdd(mop, fiberPld, (airMopper)limnPolyDataNix, airMopAlways);
    if (tenFiberMultiTrace(tfx, tfml, nseed)
        || tenFiberMultiPolyData(tfx, fiberPld, tfml)) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble:\n%s\n", me, err);
      airMopError(mop);
      return 1;
    }
    if (worldSpaceOut && !worldSpace) {
      /* have to convert output to worldspace */
      unsigned int ii;
      double indx[4], world[3];
      for (ii = 0; ii < fiberPld->xyzwNum; ii++) {
        ELL_4V_COPY(indx, fiberPld->xyzw + 4 * ii);
        ELL_4V_HOMOG(indx, indx);
        gageShapeItoW(tfx->gtx->shape, world, indx);
        ELL_3V_COPY_TT(fiberPld->xyzw + 4 * ii, float, world);
        (fiberPld->xyzw + 4 * ii)[3] = 1.0;
      }
    }
    if (_nmat) {
      /* have to further transform output by given matrix */
      unsigned int ii;
      double xxx[4], yyy[4];
      for (ii = 0; ii < fiberPld->xyzwNum; ii++) {
        ELL_4V_COPY(xxx, fiberPld->xyzw + 4 * ii);
        ELL_4MV_MUL(yyy, matx, xxx);
        ELL_4V_HOMOG(yyy, yyy);
        ELL_4V_COPY_TT(fiberPld->xyzw + 4 * ii, float, yyy);
      }
    }
    if (limnPolyDataSave(outS, fiberPld)) {
      airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble saving:\n%s\n", me, err);
      airMopError(mop);
      return 1;
    }
  }

  airMopOkay(mop);
  return 0;
}
TEND_CMD(fiber, INFO);
