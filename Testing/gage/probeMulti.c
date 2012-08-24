/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2012, 2011, 2010, 2009  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
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

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "teem/gage.h"

/* 
** Tests: 
** nrrdQuantize (to 8-bits), nrrdUnquantize
** nrrdArithBinaryOp (with nrrdBinaryOpSubtract)
*/

#define KERN_SIZE_MAX 10

static void
errPrefix(char *dst, 
          const char *me, int typi, unsigned int supi, unsigned int prbi,
          unsigned int probePass, double dxi, double dyi, double dzi,
          unsigned int xi, unsigned int yi, unsigned int zi) {
  sprintf(dst, "%s[%s][%u] #%u pp %u: (%g,%g,%g)->(%u,%u,%u): ", me,
          airEnumStr(nrrdType, typi), supi, prbi, probePass,
          dxi, dyi, dzi, xi, yi, zi);
  return;
}


int
main(int argc, const char **argv) {
  const char *me;
  airArray *mop, *submop;
  char *err;

  int typi;
  unsigned int supi, probePass;
  size_t sizes[3] = {42,61,50} /* one of these must be even */, ii, nn;
  Nrrd *norigScl, *nucharScl, *nunquant, *nqdiff,
    *nconvScl[NRRD_TYPE_MAX+1];
  unsigned char *ucharScl;
  gageContext *gctx[KERN_SIZE_MAX+1];
  gagePerVolume *gpvl[NRRD_TYPE_MAX+1][KERN_SIZE_MAX+1];
  const double *vansScl[NRRD_TYPE_MAX+1][KERN_SIZE_MAX+1],
    *gansScl[NRRD_TYPE_MAX+1][KERN_SIZE_MAX+1],
    *hansScl[NRRD_TYPE_MAX+1][KERN_SIZE_MAX+1];
  double *origScl, omin, omax,
    spcOrig[NRRD_SPACE_DIM_MAX] = {0.0, 0.0, 0.0},
    spcVec[3][NRRD_SPACE_DIM_MAX] = {
      {1.1, 0.0, 0.0},
      {0.0, 2.2, 0.0},
      {0.0, 0.0, 3.3}};
  
  
  mop = airMopNew();
  me = argv[0];

#define NRRD_NEW(name, mop)                                     \
  (name) = nrrdNew();                                           \
  airMopAdd((mop), (name), (airMopper)nrrdNuke, airMopAlways)

  /* --------------------------------------------------------------- */
  fprintf(stderr, "%s: creating initial volume ...\n", me);
  NRRD_NEW(norigScl, mop);
  if (nrrdMaybeAlloc_nva(norigScl, nrrdTypeDouble, 3, sizes)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble allocating:\n%s", me, err);
    airMopError(mop); return 1;
  }
  origScl = AIR_CAST(double *, norigScl->data);
  nn = nrrdElementNumber(norigScl);
  airSrandMT(42*42);
  for (ii=0; ii<nn/2; ii++) {
    airNormalRand(origScl + 2*ii + 0, origScl + 2*ii + 1);
  }
  /* learn real range */
  omin = omax = origScl[0];
  for (ii=1; ii<nn; ii++) {
    omin = AIR_MIN(omin, origScl[ii]);
    omax = AIR_MAX(omax, origScl[ii]);
  }
  ELL_3V_SET(spcOrig, 0.0, 0.0, 0.0);
  if (nrrdSpaceSet(norigScl, nrrdSpaceRightAnteriorSuperior)
      || nrrdSpaceOriginSet(norigScl, spcOrig)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble setting space:\n%s", me, err);
    airMopError(mop); return 1;
  }
  nrrdAxisInfoSet_nva(norigScl, nrrdAxisInfoSpaceDirection, spcVec);


  /* --------------------------------------------------------------- */
  fprintf(stderr, "%s: quantizing to 8-bits and checking ...\n", me);
  submop = airMopNew();
  NRRD_NEW(nucharScl, mop);
  NRRD_NEW(nunquant, submop);
  NRRD_NEW(nqdiff, submop);
  if (nrrdQuantize(nucharScl, norigScl, NULL, 8)
      || nrrdUnquantize(nunquant, nucharScl, nrrdTypeDouble)
      || nrrdArithBinaryOp(nqdiff, nrrdBinaryOpSubtract,
                           norigScl, nunquant)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble quantizing and back:\n%s", me, err);
    airMopError(submop); airMopError(mop); return 1;
  }
  if (!( nucharScl->oldMin == omin 
         && nucharScl->oldMax == omax )) {
    fprintf(stderr, "%s: quantization range [%g,%g] != real range [%g,%g]\n",
            me, nucharScl->oldMin, nucharScl->oldMax, omin, omax);
    airMopError(submop); airMopError(mop); return 1;
  }
  {
    double avgdiff, *qdiff, *unquant;
    avgdiff = 0.0;
    qdiff = AIR_CAST(double *, nqdiff->data);
    unquant = AIR_CAST(double *, nunquant->data);
    for (ii=0; ii<nn; ii++) {
      double dd;
      dd = qdiff[ii]*256/(omax - omin);
      /* empirically determined tolerance */
      if (AIR_ABS(dd) > 0.500000000000009) {
        unsigned int ui;
        ui = AIR_CAST(unsigned int, ii);
        fprintf(stderr, "%s: |orig[%u]=%g - unquant=%g| = %f > 0.5!\n", me,
                ui, origScl[ii], unquant[ii], AIR_ABS(dd));
        airMopError(submop); airMopError(mop); return 1;
      }
    }
  }
  airMopOkay(submop);
  ucharScl = AIR_CAST(unsigned char *, nucharScl->data);

  /* --------------------------------------------------------------- */
  fprintf(stderr, "%s: converting to all other types ...\n", me);
  for (typi=nrrdTypeUnknown+1; typi<nrrdTypeLast; typi++) {
    if (nrrdTypeBlock == typi) {
      nconvScl[typi] = NULL;
      continue;
    }
    NRRD_NEW(nconvScl[typi], mop);
    if (nrrdConvert(nconvScl[typi], nucharScl, typi)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble converting:\n%s", me, err);
      airMopError(mop); return 1;
    }
  }
  for (supi=1; supi<=KERN_SIZE_MAX; supi++) {
    int E;
    double kparm[1];
    gctx[supi] = gageContextNew();
    airMopAdd(mop, gctx[supi], (airMopper)gageContextNix, airMopAlways);
    gageParmSet(gctx[supi], gageParmRenormalize, AIR_FALSE);
    gageParmSet(gctx[supi], gageParmCheckIntegrals, AIR_TRUE);
    kparm[0] = supi;
    E = 0;
    if (!E) E |= gageKernelSet(gctx[supi], gageKernel00,
                               nrrdKernelBoxSupportDebug, kparm);
    for (typi=nrrdTypeUnknown+1; typi<nrrdTypeLast; typi++) {
      if (nrrdTypeBlock == typi) {
        gpvl[typi][supi] = NULL;
        continue;
      }
      if (!E) E |= !(gpvl[typi][supi] 
                     = gagePerVolumeNew(gctx[supi], nconvScl[typi],
                                        gageKindScl));
      if (!E) E |= gagePerVolumeAttach(gctx[supi], gpvl[typi][supi]);
      if (!E) E |= gageQueryItemOn(gctx[supi], gpvl[typi][supi],
                                   gageSclValue);
      if (E) {
        break;
      }
    }
    if (!E) E |= gageUpdate(gctx[supi]);
    if (E) {
      airMopAdd(mop, err = biffGetDone(GAGE), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble (supi=%u, %d/%s) set-up:\n%s\n", me,
              supi, typi, airEnumStr(nrrdType, typi), err);
      airMopError(mop); return 1;
    }
    if (gctx[supi]->radius != supi) {
      fprintf(stderr, "%s: supi %u != gageContext->radius %u\n",
              me, supi, gctx[supi]->radius);
      airMopError(mop); return 1;
    }
    for (typi=nrrdTypeUnknown+1; typi<nrrdTypeLast; typi++) {
      if (nrrdTypeBlock == typi) {
        vansScl[typi][supi] = NULL;
        gansScl[typi][supi] = NULL;
        hansScl[typi][supi] = NULL;
        continue;
      }
      vansScl[typi][supi] = gageAnswerPointer(gctx[supi], gpvl[typi][supi],
                                              gageSclValue);
      gansScl[typi][supi] = gageAnswerPointer(gctx[supi], gpvl[typi][supi],
                                              gageSclGradVec);
      hansScl[typi][supi] = gageAnswerPointer(gctx[supi], gpvl[typi][supi],
                                              gageSclHessian);
    }
  }

  /* --------------------------------------------------------------- */
#define PROBE_NUM 300
  for (probePass=0; probePass<=1; probePass++) {
    unsigned int prbi, subnum, lastjj=0, xi, yi, zi, sx, sy, sz;
    double thet, xu, yu, zu, dxi, dyi, dzi, dsx, dsy, dsz,
      elapsed[KERN_SIZE_MAX+1], time0;
    char errpre[AIR_STRLEN_LARGE];

    fprintf(stderr, "%s: probing (pass %u) ...\n", me, probePass);
    if (1 == probePass) {
      /* switch to cos^4 kernel, turn on gradient and hessian */
      for (supi=1; supi<=KERN_SIZE_MAX; supi++) {
        int E;
        double kparm[1];
        gageParmSet(gctx[supi], gageParmRenormalize, AIR_FALSE);
        gageParmSet(gctx[supi], gageParmCheckIntegrals, AIR_TRUE);
        kparm[0] = supi;
        E = 0;
        if (!E) E |= gageKernelSet(gctx[supi], gageKernel00,
                                   nrrdKernelCos4SupportDebug, kparm);
        if (!E) E |= gageKernelSet(gctx[supi], gageKernel11,
                                   nrrdKernelCos4SupportDebugD, kparm);
        if (!E) E |= gageKernelSet(gctx[supi], gageKernel22,
                                   nrrdKernelCos4SupportDebugDD, kparm);
        for (typi=nrrdTypeUnknown+1; typi<nrrdTypeLast; typi++) {
          if (nrrdTypeBlock == typi) {
            continue;
          }
          if (!E) E |= gageQueryItemOn(gctx[supi], gpvl[typi][supi],
                                       gageSclGradVec);
          if (!E) E |= gageQueryItemOn(gctx[supi], gpvl[typi][supi],
                                       gageSclHessian);
          if (E) {
            break;
          }
        }
        if (!E) E |= gageUpdate(gctx[supi]);
        if (E) {
          airMopAdd(mop, err = biffGetDone(GAGE), airFree, airMopAlways);
          fprintf(stderr, "%s: trouble (supi=%u, %d/%s) set-up:\n%s\n", me,
                  supi, typi, airEnumStr(nrrdType, typi), err);
          airMopError(mop); return 1;
        }
      }
    }
    for (supi=1; supi<=KERN_SIZE_MAX; supi++) {
      elapsed[supi] = 0.0;
    }
    /* these are harmlessly computed twice */
    dsx = AIR_CAST(double, sizes[0]);
    dsy = AIR_CAST(double, sizes[1]);
    dsz = AIR_CAST(double, sizes[2]);
    sx = AIR_CAST(unsigned int, sizes[0]);
    sy = AIR_CAST(unsigned int, sizes[1]);
    sz = AIR_CAST(unsigned int, sizes[2]);
    subnum = AIR_CAST(unsigned int, PROBE_NUM*0.9);
    for (prbi=0; prbi<PROBE_NUM; prbi++) {
      unsigned int jj;
      jj = airIndex(0, prbi, PROBE_NUM-1, subnum);
      thet = AIR_AFFINE(0, jj, subnum-1, 0.0, AIR_PI);
      xu = -cos(5*thet);
      yu = -cos(3*thet);
      zu = -cos(thet);
      dxi = AIR_AFFINE(-1.0, xu, 1.0, -0.5, dsx-0.5);
      dyi = AIR_AFFINE(-1.0, yu, 1.0, -0.5, dsy-0.5);
      dzi = AIR_AFFINE(-1.0, zu, 1.0, -0.5, dsz-0.5);
      if (prbi && lastjj == jj) {
        /* to occasionally test the logic in gage that seeks
           to re-use convolution weights when possible */
        dxi += airSgn(xu);
        dyi += airSgn(yu);
        dzi += airSgn(zu);
      }
      xi = airIndexClamp(-0.5, dxi, dsx-0.5, sx);
      yi = airIndexClamp(-0.5, dyi, dsy-0.5, sy);
      zi = airIndexClamp(-0.5, dzi, dsz-0.5, sz);
      lastjj = jj;
      for (supi=1; supi<=KERN_SIZE_MAX; supi++) {
        time0 = airTime();
        if (gageProbeSpace(gctx[supi], dxi, dyi, dzi,
                           AIR_TRUE /* indexSpace */,
                           AIR_TRUE /* clamp */)) {
          fprintf(stderr, "%s: probe (support %u) error (%d): %s\n", 
                  me, supi, gctx[supi]->errNum, gctx[supi]->errStr);
          airMopError(mop); return 1;
        }
        elapsed[supi] = airTime() - time0;
        for (typi=nrrdTypeUnknown+1; typi<nrrdTypeLast; typi++) {
          double convval, trueval, probeval;
          if (nrrdTypeBlock == typi 
              || (1 == probePass && nrrdTypeChar == typi)) {
            /* can't easily correct interpolation on signed char
               values to make it match interpolation on unsigned char
               values, prior to wrap-around */
            continue;
          }
          probeval = vansScl[typi][supi][0];
          if (0 == probePass) {
            convval = (nrrdDLookup[typi])(nconvScl[typi]->data,
                                          xi + sx*(yi + sy*zi));
            if (convval != probeval) {
#define ERR_PREFIX                                                      \
              errPrefix(errpre, me, typi, supi, prbi, probePass,        \
                        dxi, dyi, dzi, xi, yi, zi)
              ERR_PREFIX;
              fprintf(stderr, "%s: probed %g != conv %g\n", errpre,
                      probeval, convval);
              airMopError(mop); return 1;
            }
            trueval = AIR_CAST(double, ucharScl[xi + sx*(yi + sy*zi)]);
          } else {
            trueval = vansScl[nrrdTypeUChar][supi][0];
          }
          if (nrrdTypeChar == typi && trueval > 127) {
            /* recreate value wrapping of signed char */
            trueval -= 256;
          }
          if (trueval != probeval) {
            fprintf(stderr, "%s: probed %g != true %g\n", errpre,
                    probeval, trueval);
            airMopError(mop); return 1;
          }
          if (1 == probePass) {
            double diff3[3], diff9[9];
            ELL_3V_SUB(diff3, gansScl[nrrdTypeUChar][supi], gansScl[typi][supi]);
            if (ELL_3V_LEN(diff3) > 0.0) {
              ERR_PREFIX;
              fprintf(stderr, "%s: probed gradient error len %f\n",
                      errpre, ELL_3V_LEN(diff3));
              airMopError(mop); return 1;
            }
            ELL_9V_SUB(diff9, hansScl[nrrdTypeUChar][supi], hansScl[typi][supi]);
            if (ELL_9V_LEN(diff9) > 0.0) {
              ERR_PREFIX;
              fprintf(stderr, "%s: probed hessian error len %f\n",
                      errpre, ELL_9V_LEN(diff9));
              airMopError(mop); return 1;
            }
          }
        }
      }
    }
    for (supi=1; supi<=KERN_SIZE_MAX; supi++) {
      fprintf(stderr, "%s: elapsed[%u] = %g ms\n",
              me, supi, 1000*elapsed[supi]);
    }
  }
  
#undef NRRD_NEW;
#undef ERR_PREFIX;
  airMopOkay(mop);
  exit(0);
}
