/*
  Teem: Tools to process and visualize scientific data and images              
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

#include "gage.h"
#include "privateGage.h"

#define N -1

/*
** NOTE: This table was computed by Raul San Jose Estepar
**
** Basic indexing idea: [sigma max][total # samples][which sample]
**
** "sigma max" can't be 0; smallest value is 1
** ==> index with (sigma max)-1
** biggest value is GAGE_OPTIMSIG_SIGMA_MAX, 
** ==> biggest index is GAGE_OPTIMSIG_SIGMA_MAX-1
** ==> allocate for GAGE_OPTIMSIG_SIGMA_MAX
**
** "total # samples" can't be 0, or 1, smallest value is 2
** ==> index with (total # samples)-2
** biggest value is GAGE_OPTIMSIG_SAMPLES_MAXNUM
** ==> biggest index is GAGE_OPTIMSIG_SAMPLES_MAXNUM-2
** ==> allocate for GAGE_OPTIMSIG_SAMPLES_MAXNUM-1
**
** "which sample" ranges from 0 to GAGE_OPTIMSIG_SAMPLES_MAXNUM-1
** ==> allocate for GAGE_OPTIMSIG_SAMPLES_MAXNUM
*/
static double
_optimSigTable[GAGE_OPTIMSIG_SIGMA_MAX][GAGE_OPTIMSIG_SAMPLES_MAXNUM-1][GAGE_OPTIMSIG_SAMPLES_MAXNUM] = {
  {
    {0,1,N,N,N,N,N,N,N,N},
    {0,0.520755,1,N,N,N,N,N,N,N},
    {0,0.283027,0.669151,1,N,N,N,N,N,N},
    {0,0.209929,0.552348,0.770834,1,N,N,N,N,N},
    {0,0.173387,0.378436,0.604894,0.793886,1,N,N,N,N},
    {0,0.134881,0.288941,0.515696,0.682551,0.836998,1,N,N,N},
    {0,0.128017,0.253151,0.462236,0.588126,0.717821,0.852488,1,N,N},
    {0,0.112715,0.223891,0.334963,0.522566,0.636144,0.751803,0.86886,1,N},
    {0,0.111111,0.222222,0.333333,0.444444,0.555556,0.666667,0.777778,0.888889,1}
  }, {
    {0,2,N,N,N,N,N,N,N,N},
    {0,0.894447,2,N,N,N,N,N,N,N},
    {0,0.791103,1.32514,2,N,N,N,N,N,N},
    {0,0.408347,0.834154,1.36224,2,N,N,N,N,N},
    {0,0.282745,0.668827,0.999315,1.4862,2,N,N,N,N},
    {0,0.217996,0.601946,0.864022,1.17092,1.5677,2,N,N,N},
    {0,0.210786,0.556168,0.776414,1.00855,1.31339,1.6362,2,N,N},
    {0,0.188748,0.428301,0.644747,0.847655,1.07408,1.36986,1.6701,2,N},
    {0,0.166782,0.359528,0.579335,0.766882,0.950667,1.1702,1.43221,1.69825,2}
  }, {
    {0,3,N,N,N,N,N,N,N,N},
    {0,1.14084,3,N,N,N,N,N,N,N},
    {0,0.841905,1.73238,3,N,N,N,N,N,N},
    {0,0.790237,1.31016,1.96863,3,N,N,N,N,N},
    {0,0.409074,0.835194,1.36517,2.00441,3,N,N,N,N},
    {0,0.301577,0.689588,1.04009,1.56327,2.14135,3,N,N,N},
    {0,0.239395,0.628878,0.919926,1.29603,1.72362,2.25393,3,N,N},
    {0,0.214494,0.585704,0.830516,1.10355,1.48037,1.86144,2.34487,3,N},
    {0,0.20797,0.553449,0.771092,1.00198,1.30069,1.61891,1.97351,2.41963,3}
  }, {
    {0,4,N,N,N,N,N,N,N,N},
    {0,1.29373,4,N,N,N,N,N,N,N},
    {0,0.891281,1.98772,4,N,N,N,N,N,N},
    {0,0.812102,1.51247,2.4052,4,N,N,N,N,N},
    {0,0.788091,1.24542,1.83483,2.67689,4,N,N,N,N},
    {0,0.78746,1.12466,1.60209,2.13469,2.91117,4,N,N,N},
    {0,0.290894,0.677842,1.01677,1.52179,2.06317,2.85211,4,N,N},
    {0,0.237903,0.627206,0.917396,1.28992,1.71623,2.24197,2.97986,4,N},
    {0,0.215115,0.592669,0.841147,1.12279,1.50482,1.90353,2.41258,3.08597,4}
  }, {
    {0,5,N,N,N,N,N,N,N,N},
    {0,1.41067,5,N,N,N,N,N,N,N},
    {0,0.953278,2.20477,5,N,N,N,N,N,N},
    {0,0.834267,1.67815,2.83204,5,N,N,N,N,N},
    {0,0.792187,1.3403,2.03171,3.15005,5,N,N,N,N},
    {0,0.78757,1.18893,1.72149,2.4075,3.44719,5,N,N,N},
    {0,0.78743,1.13311,1.61308,2.1125,2.79478,3.69104,5,N,N},
    {0,0.274745,0.660696,0.982844,1.44933,1.94005,2.62578,3.60429,5,N},
    {0,0.227728,0.61747,0.896691,1.24262,1.65644,2.14203,2.81396,3.7414,5}
  }, {
    {0,6,N,N,N,N,N,N,N,N},
    {0,1.51118,6,N,N,N,N,N,N,N},
    {0,1.01124,2.41248,6,N,N,N,N,N,N},
    {0,0.851069,1.79261,3.20679,6,N,N,N,N,N},
    {0,0.802132,1.43603,2.23513,3.61894,6,N,N,N,N},
    {0,0.788066,1.24208,1.8278,2.65974,3.96683,6,N,N,N},
    {0,0.787497,1.1568,1.65453,2.23342,3.06432,4.2812,6,N,N},
    {0,0.787485,1.13379,1.56653,2.00911,2.68008,3.49909,4.5694,6,N},
    {0,0.787667,1.18479,1.67819,2.09585,2.4242,3.00662,3.77465,4.93869,6}
  }, {
    {0,7,N,N,N,N,N,N,N,N},
    {0,1.60329,7,N,N,N,N,N,N,N},
    {0,1.06085,2.61341,7,N,N,N,N,N,N},
    {0,0.866894,1.88075,3.54483,7,N,N,N,N,N},
    {0,0.814332,1.52904,2.44358,4.08604,7,N,N,N,N},
    {0,0.789343,1.29133,1.92917,2.90412,4.47442,7,N,N,N},
    {0,0.787596,1.18935,1.71404,2.37715,3.36803,4.86346,7,N,N},
    {0,0.787514,1.15695,1.60371,2.10829,2.84863,3.89034,5.1996,7,N},
    {0,0.787457,1.07104,1.46988,1.87322,2.40183,3.10841,4.0735,5.36295,7}
  }, {
    {0,8,N,N,N,N,N,N,N,N},
    {0,1.69025,8,N,N,N,N,N,N,N},
    {0,1.10353,2.80924,8,N,N,N,N,N,N},
    {0,0.883383,1.95573,3.85935,8,N,N,N,N,N},
    {0,0.825068,1.6096,2.64212,4.54026,8,N,N,N,N},
    {0,0.791992,1.33713,2.025,3.13391,4.96489,8,N,N,N},
    {0,0.788026,1.22801,1.79392,2.56332,3.74548,5.44656,8,N,N},
    {0,0.787564,1.17443,1.65664,2.22185,3.06335,4.19682,5.81895,8,N},
    {0,0.787537,1.16581,1.62938,2.18573,2.92214,3.78781,4.79439,6.1351,8}
  }, {
    {0,9,N,N,N,N,N,N,N,N},
    {0,1.77412,9,N,N,N,N,N,N,N},
    {0,1.14066,2.99946,9,N,N,N,N,N,N},
    {0,0.900744,2.02332,4.15788,9,N,N,N,N,N},
    {0,0.833793,1.67451,2.82137,4.97343,9,N,N,N,N},
    {0,0.796065,1.38336,2.12246,3.36198,5.45218,9,N,N,N},
    {0,0.787977,1.24206,1.82732,2.65703,3.96454,5.93871,9,N,N},
    {0,0.787529,1.20274,1.72546,2.40277,3.37085,4.7923,6.61365,9,N},
    {0,0.787591,1.1866,1.70605,2.32059,3.16159,4.1253,5.40548,7.03411,9}
  }, {
    {0,10,N,N,N,N,N,N,N,N},
    {0,1.85608,10,N,N,N,N,N,N,N},
    {0,1.17364,3.18453,10,N,N,N,N,N,N},
    {0,0.918224,2.08614,4.44436,10,N,N,N,N,N},
    {0,0.841096,1.7269,2.98229,5.38617,10,N,N,N,N},
    {0,0.801356,1.42973,2.22153,3.58821,5.93456,10,N,N,N},
    {0,0.788839,1.27109,1.88632,2.79603,4.23227,6.52024,10,N,N},
    {0,0.787688,1.19698,1.72921,2.4077,3.46282,4.8803,7.09829,10,N},
    {0,0.787439,1.19722,1.6892,2.2477,3.17214,4.26716,5.71583,7.47542,10}
  }, {
    {0,11,N,N,N,N,N,N,N,N},
    {0,1.93545,11,N,N,N,N,N,N,N},
    {0,1.20318,3.36452,11,N,N,N,N,N,N},
    {0,0.935901,2.14566,4.72059,11,N,N,N,N,N},
    {0,0.847605,1.7706,3.12873,5.78181,11,N,N,N,N},
    {0,0.80719,1.47539,2.32141,3.81262,6.41322,11,N,N,N},
    {0,0.789345,1.2892,1.92443,2.89215,4.44879,6.94919,11,N,N},
    {0,0.788229,1.22938,1.7927,2.52919,3.60546,5.39521,7.70833,11,N},
    {0,0.787337,1.18853,1.72515,2.4498,3.5312,4.66609,6.13707,8.41855,11}
  }, {
    {0,12,N,N,N,N,N,N,N,N},
    {0,2.01416,12,N,N,N,N,N,N,N},
    {0,1.23012,3.53954,12,N,N,N,N,N,N},
    {0,0.952603,2.20271,4.98931,12,N,N,N,N,N},
    {0,0.853664,1.80833,3.26401,6.16338,12,N,N,N,N},
    {0,0.81297,1.51917,2.42056,4.0342,6.88767,12,N,N,N},
    {0,0.790331,1.31188,1.97217,3.0083,4.69776,7.45512,12,N,N},
    {0,0.787965,1.2179,1.78305,2.55933,3.72792,5.48793,8.0347,12,N},
    {0,0.787576,1.1817,1.69768,2.32112,3.26031,4.51281,6.36702,8.6217,12}
  }, {
    {0,13,N,N,N,N,N,N,N,N},
    {0,2.09072,13,N,N,N,N,N,N,N},
    {0,1.25473,3.71005,13,N,N,N,N,N,N},
    {0,0.968504,2.25759,5.25073,13,N,N,N,N,N},
    {0,0.859459,1.84179,3.39052,6.5332,13,N,N,N,N},
    {0,0.818391,1.55938,2.51581,4.24908,7.35454,13,N,N,N},
    {0,0.791833,1.33589,2.02218,3.1265,4.94677,7.95647,13,N,N},
    {0,0.787653,1.23894,1.81694,2.61786,3.85211,5.84892,8.72848,13,N},
    {0,0.787967,1.22789,1.79172,2.54216,3.58073,5.09756,6.99612,9.75473,13}
  }, {
    {0,14,N,N,N,N,N,N,N,N},
    {0,2.16714,14,N,N,N,N,N,N,N},
    {0,1.27754,3.87624,14,N,N,N,N,N,N},
    {0,0.983617,2.31064,5.50629,14,N,N,N,N,N},
    {0,0.865179,1.87209,3.50991,6.89294,14,N,N,N,N},
    {0,0.823231,1.59581,2.60645,4.45698,7.81366,14,N,N,N},
    {0,0.793843,1.35989,2.0725,3.24427,5.19604,8.45755,14,N,N},
    {0,0.788189,1.24679,1.83587,2.67506,3.99763,5.99675,9.18925,14,N},
    {0,0.787518,1.19268,1.72283,2.40965,3.41008,4.95037,6.95596,10.0416,14}
  }, {
    {0,15,N,N,N,N,N,N,N,N},
    {0,2.24195,15,N,N,N,N,N,N,N},
    {0,1.29877,4.03819,15,N,N,N,N,N,N},
    {0,0.997855,2.36236,5.75601,15,N,N,N,N,N},
    {0,0.870817,1.89996,3.62341,7.24393,15,N,N,N,N},
    {0,0.827579,1.62851,2.69216,4.65806,8.26562,15,N,N,N},
    {0,0.795992,1.38308,2.12163,3.35882,5.44078,8.96653,15,N,N},
    {0,0.788222,1.25363,1.8518,2.71754,4.08443,6.2259,9.61325,15,N},
    {0,0.787589,1.22373,1.78686,2.54249,3.60068,5.06811,7.15615,10.7298,15}
  }, {
    {0,16,N,N,N,N,N,N,N,N},
    {0,2.31533,16,N,N,N,N,N,N,N},
    {0,1.31863,4.19611,16,N,N,N,N,N,N},
    {0,1.0114,2.41248,6.00077,16,N,N,N,N,N},
    {0,0.876466,1.9259,3.73192,7.58716,16,N,N,N,N},
    {0,0.831402,1.65685,2.77054,4.84786,8.70531,16,N,N,N},
    {0,0.798602,1.4072,2.17278,3.47535,5.68589,9.46791,16,N,N},
    {0,0.788959,1.27518,1.89518,2.81881,4.27432,6.59075,10.3003,16,N},
    {0,0.787508,1.19052,1.72604,2.41099,3.44569,5.04957,7.34631,10.822,16}
  }, {
    {0,17,N,N,N,N,N,N,N,N},
    {0,2.38842,17,N,N,N,N,N,N,N},
    {0,1.33745,4.35066,17,N,N,N,N,N,N},
    {0,1.02407,2.46138,6.24124,17,N,N,N,N,N},
    {0,0.882058,1.9503,3.83612,7.92352,17,N,N,N,N},
    {0,0.834906,1.68239,2.84459,5.03152,9.13817,17,N,N,N},
    {0,0.801152,1.42771,2.21701,3.57698,5.90701,9.93583,17,N,N},
    {0,0.788986,1.28833,1.92263,2.88691,4.4271,6.84082,10.8081,17,N},
    {0,0.787599,1.19465,1.7481,2.48295,3.58968,5.21382,7.71567,11.437,17}
  }, {
    {0,18,N,N,N,N,N,N,N,N},
    {0,2.46122,18,N,N,N,N,N,N,N},
    {0,1.35525,4.50157,18,N,N,N,N,N,N},
    {0,1.03593,2.50922,6.47782,18,N,N,N,N,N},
    {0,0.887668,1.97341,3.93658,8.25361,18,N,N,N,N},
    {0,0.838035,1.70525,2.91389,5.20786,9.56279,18,N,N,N},
    {0,0.804084,1.45016,2.26575,3.6869,6.1415,10.4245,18,N,N},
    {0,0.789732,1.29808,1.94283,2.93386,4.53324,7.08002,11.2605,18,N},
    {0,0.788503,1.24114,1.82049,2.6194,3.79914,5.47903,8.01455,12.2307,18}
  }, {
    {0,19,N,N,N,N,N,N,N,N},
    {0,2.53263,19,N,N,N,N,N,N,N},
    {0,1.37223,4.64947,19,N,N,N,N,N,N},
    {0,1.04728,2.55589,6.71041,19,N,N,N,N,N},
    {0,0.893288,1.99543,4.03373,8.57804,19,N,N,N,N},
    {0,0.840954,1.72595,2.97921,5.378,9.98027,19,N,N,N},
    {0,0.806932,1.47242,2.31466,3.79638,6.37439,10.9104,19,N,N},
    {0,0.790022,1.30679,1.96143,2.98198,4.63905,7.32692,11.7341,19,N},
    {0,0.788279,1.2464,1.81929,2.61519,3.84443,5.72051,8.45263,12.787,19}
  }, {
    {0,20,N,N,N,N,N,N,N,N},
    {0,2.60266,20,N,N,N,N,N,N,N},
    {0,1.38836,4.79358,20,N,N,N,N,N,N},
    {0,1.05804,2.60142,6.93936,20,N,N,N,N,N},
    {0,0.898865,2.01655,4.12794,8.8972,20,N,N,N,N},
    {0,0.843712,1.74482,3.04097,5.54248,10.391,20,N,N,N},
    {0,0.809398,1.49207,2.35873,3.89597,6.59121,11.3758,20,N,N},
    {0,0.790554,1.31755,1.98405,3.03747,4.75634,7.5704,12.2317,20,N},
    {0,0.787816,1.23873,1.82796,2.67325,3.93618,5.86681,8.77522,13.3479,20}
  }
};

int
gageOptimSigSet(double *scale, unsigned int num, unsigned int sigmaMax) {
  char me[]="gageOptimSigSet", err[BIFF_STRLEN];
  unsigned int si;
  
  if (!scale) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!AIR_IN_CL(2, num, GAGE_OPTIMSIG_SAMPLES_MAXNUM)) {
    sprintf(err, "%s: requested # sigma samples %u not in known range [2,%u]",
            me, num, GAGE_OPTIMSIG_SAMPLES_MAXNUM);
    biffAdd(GAGE, err); return 1;
  }
  if (!AIR_IN_CL(1, sigmaMax, GAGE_OPTIMSIG_SIGMA_MAX)) {
    sprintf(err, "%s: requested sigma max %u not in known range [1,%u]",
            me, sigmaMax, GAGE_OPTIMSIG_SIGMA_MAX);
    biffAdd(GAGE, err); return 1;
  }

  for (si=0; si<num; si++) {
    scale[si] = _optimSigTable[sigmaMax-1][num-2][si];
  }
  return 0;
}

gageOptimSigParm *
gageOptimSigParmNew(unsigned int sampleNumMax) {
  gageOptimSigParm *parm;
  
  parm = AIR_CAST(gageOptimSigParm *, calloc(1, sizeof(gageOptimSigParm)));
  if (parm) {
    unsigned int si;
    parm->dim = 0;
    parm->sampleNumMax = sampleNumMax;
    parm->sigmatru = NULL;
    parm->truth = NULL;
    parm->ntruth = nrrdNew();
    parm->nerr = nrrdNew();
    parm->ntruline = nrrdNew();
    parm->ninterp = nrrdNew();
    parm->ndiff = nrrdNew();
    parm->scalePos = AIR_CAST(double *, calloc(sampleNumMax, sizeof(double)));
    parm->step = AIR_CAST(double *, calloc(sampleNumMax, sizeof(double)));
    parm->nsampvol = AIR_CAST(Nrrd **, calloc(sampleNumMax, sizeof(Nrrd *)));
    for (si=0; si<sampleNumMax; si++) {
      parm->nsampvol[si] = nrrdNew();
    }
    parm->pvl = NULL;
    parm->pvlSS = AIR_CAST(gagePerVolume **,
                           calloc(sampleNumMax, sizeof(gagePerVolume *)));
    parm->gctx = gageContextNew();
  }
  return parm;
}

gageOptimSigParm *
gageOptimSigParmNix(gageOptimSigParm *parm) {

  if (parm) {
    unsigned int si;
    airFree(parm->sigmatru);
    nrrdNuke(parm->ntruth);
    nrrdNuke(parm->nerr);
    nrrdNix(parm->ntruline);
    nrrdNuke(parm->ninterp);
    nrrdNuke(parm->ndiff);
    airFree(parm->scalePos);
    airFree(parm->step);
    for (si=0; si<parm->sampleNumMax; si++) {
      nrrdNuke(parm->nsampvol[si]);
    }
    airFree(parm->nsampvol);
    airFree(parm->pvlSS);
    gageContextNix(parm->gctx);
    airFree(parm);
  }
  return NULL;
}

static void
_volTrueBlur(Nrrd *nvol, double sigma, gageOptimSigParm *parm) {
  double *vol, xv, yv, zv;
  unsigned int xi, yi, zi;
  NrrdKernel *dg;
  double kparm[NRRD_KERNEL_PARMS_NUM], xrad, yrad, zrad;
  
  vol = AIR_CAST(double *, nvol->data);
  xrad = (nvol->axis[0].size + 1)/2 - 1;
  yrad = (nvol->axis[1].size + 1)/2 - 1;
  zrad = (nvol->axis[2].size + 1)/2 - 1;
  dg = nrrdKernelDiscreteGaussian;
  kparm[0] = sigma;
  kparm[1] = parm->cutoff;
  for (zi=0; zi<parm->sz; zi++) {
    zv = (parm->dim >= 2
          ? dg->eval1_d(AIR_CAST(double, zi) - zrad, kparm)
          : 1);
    for (yi=0; yi<parm->sy; yi++) {
      yv = (parm->dim >= 3
            ? dg->eval1_d(AIR_CAST(double, yi) - yrad, kparm)
            : 1);
      for (xi=0; xi<parm->sx; xi++) {
        xv = dg->eval1_d(AIR_CAST(double, xi) - xrad, kparm);
        vol[xi + parm->sx*(yi + parm->sy*zi)] = xv*yv*zv;
      }
    }
  }
  return;
}

int
gageOptimSigTruthSet(gageOptimSigParm *parm,
                     unsigned int dim,
                     double sigmaMax, double cutoff,
                     unsigned int measrSampleNum) {
  char me[]="gageOptimSigTruthSet", err[BIFF_STRLEN],
    doneStr[AIR_STRLEN_SMALL];
  double kparm[NRRD_KERNEL_PARMS_NUM], tauMax;
  unsigned int support, ii;

  if (!parm) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!AIR_IN_CL(1, dim, 3)) {
    sprintf(err, "%s: dim %u not 1, 2, or 3", me, dim);
    biffAdd(GAGE, err); return 1;
  }
  if (!(sigmaMax > 0 && cutoff > 0)) {
    sprintf(err, "%s: sigmaMax %g, cutoff %g not both > 0", me, 
            sigmaMax, cutoff);
    biffAdd(GAGE, err); return 1;
  }
  if (!(measrSampleNum >= 3)) {
    sprintf(err, "%s: measrSampleNum %u not >= 3", me, measrSampleNum);
    biffAdd(GAGE, err); return 1;
  }

  parm->dim = dim;
  kparm[0] = parm->sigmaMax = sigmaMax;
  kparm[1] = parm->cutoff = cutoff;
  parm->measrSampleNum = measrSampleNum;
  support = AIR_ROUNDUP(nrrdKernelDiscreteGaussian->support(kparm));
  /* may later allow different shaped volumes */
  parm->sx = parm->sy = parm->sz = 2*support - 1;
  printf("!%s: support = %u, vol size = %u\n", me, support, parm->sx);
  airFree(parm->sigmatru);
  parm->sigmatru = AIR_CAST(double *, calloc(measrSampleNum, sizeof(double)));
  if (!parm->sigmatru) {
    sprintf(err, "%s: couldn't alloc sigmatru buffer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (nrrdMaybeAlloc_va(parm->ntruth, nrrdTypeDouble, 4,
                        AIR_CAST(size_t, parm->sx),
                        AIR_CAST(size_t, parm->sy),
                        AIR_CAST(size_t, parm->sz),
                        AIR_CAST(size_t, measrSampleNum))
      || nrrdMaybeAlloc_va(parm->nerr, nrrdTypeDouble, 1,
                           AIR_CAST(size_t, measrSampleNum))
      /* ntruline->data will be re-set willy-nilly */
      || nrrdWrap_va(parm->ntruline, parm->ntruth->data,
                     parm->ntruth->type, 3, 
                     AIR_CAST(size_t, parm->sx),
                     AIR_CAST(size_t, parm->sy),
                     AIR_CAST(size_t, parm->sz))
      || nrrdMaybeAlloc_va(parm->ninterp, nrrdTypeDouble, 3,
                           AIR_CAST(size_t, parm->sx),
                           AIR_CAST(size_t, parm->sy),
                           AIR_CAST(size_t, parm->sz))
      || nrrdMaybeAlloc_va(parm->ndiff, nrrdTypeDouble, 3,
                           AIR_CAST(size_t, parm->sx),
                           AIR_CAST(size_t, parm->sy),
                           AIR_CAST(size_t, parm->sz))) {
    sprintf(err, "%s: couldn't allocate truth", me);
    biffMove(GAGE, err, NRRD); return 1;
  }
  parm->truth = AIR_CAST(double *, parm->ntruth->data);
  nrrdAxisInfoSet_va(parm->ntruth, nrrdAxisInfoSpacing,
                     1.0, 1.0, 1.0, AIR_NAN);
  nrrdAxisInfoSet_va(parm->ntruline, nrrdAxisInfoSpacing,
                     1.0, 1.0, 1.0);
  nrrdAxisInfoSet_va(parm->ninterp, nrrdAxisInfoSpacing,
                     1.0, 1.0, 1.0);
  nrrdAxisInfoSet_va(parm->ndiff, nrrdAxisInfoSpacing,
                     1.0, 1.0, 1.0);
  for (ii=0; ii<parm->sampleNumMax; ii++) {
    if (nrrdMaybeAlloc_va(parm->nsampvol[ii], nrrdTypeDouble, 3,
                          AIR_CAST(size_t, parm->sx),
                          AIR_CAST(size_t, parm->sy),
                          AIR_CAST(size_t, parm->sz))) {
      sprintf(err, "%s: couldn't allocate vol[%u]", me, ii);
      biffMove(GAGE, err, NRRD); return 1;
    }
    nrrdAxisInfoSet_va(parm->nsampvol[ii], nrrdAxisInfoSpacing,
                       1.0, 1.0, 1.0);
  }
  printf("%s: computing reference blurrings ...       ", me);
  tauMax =  gageTauOfSig(parm->sigmaMax);
  for (ii=0; ii<parm->measrSampleNum; ii++) {
    double sigma, tau;
    if (!(ii % 10)) {
      printf("%s", airDoneStr(0, ii, parm->measrSampleNum, doneStr));
      fflush(stdout);
    }
    parm->ntruline->data = parm->truth + ii*parm->sx*parm->sy*parm->sz;
    tau = AIR_AFFINE(0, ii, parm->measrSampleNum-1, 0.0, tauMax);
    sigma = parm->sigmatru[ii] = gageSigOfTau(tau);
    _volTrueBlur(parm->ntruline, sigma, parm);
  }
  printf("%s\n", airDoneStr(0, ii, parm->measrSampleNum, doneStr));
  return 0;
}

static void
_volInterp(Nrrd *ninterp, double scale, gageOptimSigParm *parm) {
  double *interp, scaleIdx;
  const double *answer;
  unsigned int xi, yi, zi;
  int outside;

  scaleIdx = _gageStackWtoI(parm->gctx, scale, &outside);
  answer = gageAnswerPointer(parm->gctx, parm->pvl, gageSclValue);
  interp = AIR_CAST(double *, ninterp->data);
  for (zi=0; zi<parm->sz; zi++) {
    for (yi=0; yi<parm->sy; yi++) {
      for (xi=0; xi<parm->sx; xi++) {
        gageStackProbe(parm->gctx, xi, yi, zi, scaleIdx);
        interp[xi + parm->sx*(yi + parm->sy*zi)] = answer[0];
      }
    }
  }
  return;
}

static double
_errSingle(gageOptimSigParm *parm, unsigned int sigmaIdx) {
  double *interp, *truline, *diff, ret;
  size_t ii, nn;

  _volInterp(parm->ninterp, parm->sigmatru[sigmaIdx], parm);
  interp = AIR_CAST(double *, parm->ninterp->data);
  nn = parm->sx*parm->sy*parm->sz;
  truline = parm->truth + sigmaIdx*nn;
  diff = AIR_CAST(double *, parm->ndiff->data);
  for (ii=0; ii<nn; ii++) {
    diff[ii] = truline[ii] - interp[ii];
  }
  if (0) {
    char fname[AIR_STRLEN_SMALL];
    sprintf(fname, "interp-%03u.nrrd", sigmaIdx);
    nrrdSave(fname, parm->ninterp, NULL);
  }
  nrrdMeasureLine[parm->volMeasr](&ret, nrrdTypeDouble,
                                  diff, nrrdTypeDouble,
                                  nn, AIR_NAN, AIR_NAN);
  return ret;
}

static double
_errTotal(gageOptimSigParm *parm) {
  unsigned int ii;
  double *err, ret;

  for (ii=0; ii<parm->sampleNum; ii++) {
    parm->gctx->stackPos[ii] = parm->scalePos[ii];
  }
  err = AIR_CAST(double *, parm->nerr->data);
  for (ii=0; ii<parm->measrSampleNum; ii++) {
    err[ii] = _errSingle(parm, ii);
  }
  nrrdMeasureLine[parm->lineMeasr](&ret, nrrdTypeDouble,
                                   err, nrrdTypeDouble,
                                   parm->measrSampleNum,
                                   AIR_NAN, AIR_NAN);
  if (0) {
    static unsigned int call;
    char fname[AIR_STRLEN_SMALL];
    sprintf(fname, "err-%04u.nrrd", call);
    nrrdSave(fname, parm->nerr, NULL);
    call++;
  }
  return ret;
}

static int
_gageSetup(gageOptimSigParm *parm) {
  char me[]="_gageSetup", err[BIFF_STRLEN];
  double kparm[NRRD_KERNEL_PARMS_NUM], time0;
  int E;

  time0 = airTime();
  printf("%s: ... hi!\n", me);
  if (parm->gctx) {
    gageContextNix(parm->gctx);
  }
  printf("%s: ... 0 %g\n", me, airTime() - time0);
  parm->gctx = gageContextNew();
  gageParmSet(parm->gctx, gageParmVerbose, 0);
  gageParmSet(parm->gctx, gageParmRenormalize, AIR_FALSE);
  gageParmSet(parm->gctx, gageParmCheckIntegrals, AIR_FALSE);
  gageParmSet(parm->gctx, gageParmOrientationFromSpacing, AIR_TRUE);
  gageParmSet(parm->gctx, gageParmStackUse, AIR_TRUE);
  printf("%s: ... 1 %g\n", me, airTime() - time0);
  E = 0;
  if (!E) E |= !(parm->pvl = gagePerVolumeNew(parm->gctx, parm->nsampvol[0],
                                              gageKindScl));
  printf("%s: ... 2 %g\n", me, airTime() - time0);
  if (!E) E |= gageStackPerVolumeNew(parm->gctx, parm->pvlSS,
                                     AIR_CAST(const Nrrd**, parm->nsampvol),
                                     parm->sampleNum, gageKindScl);
  printf("%s: ... 3 %g\n", me, airTime() - time0);
  if (!E) E |= gageStackPerVolumeAttach(parm->gctx, parm->pvl, parm->pvlSS,
                                        parm->scalePos, parm->sampleNum);
  kparm[0] = 1;
  printf("%s: ... 4 %g\n", me, airTime() - time0);
  if (!E) E |= gageKernelSet(parm->gctx, gageKernel00,
                             nrrdKernelTent, kparm);
  if (!E) E |= gageKernelSet(parm->gctx, gageKernelStack,
                             nrrdKernelHermiteFlag, kparm);
  printf("%s: ... 5 %g\n", me, airTime() - time0);
  if (!E) E |= gageQueryItemOn(parm->gctx, parm->pvl, gageSclValue);
  if (!E) E |= gageUpdate(parm->gctx);
  printf("%s: ... 6 %g\n", me, airTime() - time0);
  if (E) {
    sprintf(err, "%s: problem setting up gage", me);
    biffAdd(GAGE, err); return 1;
  }
  printf("%s: ... 7 %g\n", me, airTime() - time0);
  return 0;
}

static void
_scalePosSet(gageOptimSigParm *parm, unsigned int ii, double sigma) {

  parm->scalePos[ii] = sigma;
  _volTrueBlur(parm->nsampvol[ii], parm->scalePos[ii], parm);
  gagePointReset(&(parm->gctx->point));
}

static char *
_timefmt(char tstr[AIR_STRLEN_MED], double deltim) {
  
  if (deltim < 60) {
    sprintf(tstr, "%g secs", deltim);
    return tstr;
  }
  deltim /= 60;
  if (deltim < 60) {
    sprintf(tstr, "%g mins", deltim);
    return tstr;
  }
  deltim /= 60;
  if (deltim < 24) {
    sprintf(tstr, "%g hours", deltim);
    return tstr;
  }
  deltim /= 24;
  if (deltim < 7) {
    sprintf(tstr, "%g days", deltim);
    return tstr;
  }
  deltim /= 7;
  sprintf(tstr, "%g weeks", deltim);
  return tstr;
}

static int
_optsigrun(gageOptimSigParm *parm) {
  char me[]="_optsigrun", err[BIFF_STRLEN], tstr[AIR_STRLEN_MED];
  unsigned int iter, pnt;
  double lastErr, newErr, sigeps, oppor, lastPos, backoff, decavg, time0;
  int badStep;

  time0 = airTime();
  lastErr = _errTotal(parm);
  printf("%s: (%s for initial error measr)\n", me,
         _timefmt(tstr, airTime() - time0));
  newErr = AIR_NAN;
  decavg = parm->sampleNum; /* hack */
  /* meaningful discrete difference for looking at error gradient is
     bounded by the resolution of the sampling we're doing along scale */
  sigeps = parm->sigmatru[1]/10;
  oppor = 1.3333;
  backoff = 0.25;
  for (pnt=1; pnt<parm->sampleNum-1; pnt++) {
    parm->step[pnt] = 10;
  }
  for (iter=0; iter<parm->maxIter; iter++) {
    double limit, err1, grad, delta;
    unsigned int tryi;
    int zerodelta;
    pnt = 1 + (iter % (parm->sampleNum-2));
    lastPos = parm->scalePos[pnt];
    printf("%s: ***** iter %u; [[ err %g ]] %s\n", 
           me, iter, lastErr, _timefmt(tstr, airTime() - time0));
    limit = AIR_MIN((parm->scalePos[pnt] - parm->scalePos[pnt-1])/3,
                    (parm->scalePos[pnt+1] - parm->scalePos[pnt])/3);
    printf(". pnt %u: pos %g, step %g\n", pnt, lastPos, parm->step[pnt]);
    printf(". limit = min((%g-%g)/3,(%g-%g)/3) = %g\n", 
           parm->scalePos[pnt], parm->scalePos[pnt-1],
           parm->scalePos[pnt+1], parm->scalePos[pnt], limit);
    _scalePosSet(parm, pnt, lastPos + sigeps);
    err1 = _errTotal(parm);
    _scalePosSet(parm, pnt, lastPos);
    grad = (err1 - lastErr)/sigeps;
    printf(". grad = %g\n", grad);
    delta = -grad*parm->step[pnt];
    if (!AIR_EXISTS(delta)) {
      sprintf(err, "%s: got non-exist delta %g on iter %u (pnt %u) err %g",
              me, delta, iter, pnt, lastErr);
      biffAdd(GAGE, err); return 1;
    }
    if (AIR_ABS(delta) > limit) {
      parm->step[pnt] *= limit/AIR_ABS(delta);
      printf(". step *= %g/%g -> %g\n",
             limit, AIR_ABS(delta), parm->step[pnt]);
      delta = -grad*parm->step[pnt];
    }
    printf(". delta = %g\n", delta);
    tryi = 0;
    badStep = AIR_FALSE;
    do {
      if (tryi == parm->maxIter) {
        sprintf(err, "%s: confusion (tryi %u) on iter %u (pnt %u) err %g",
                me, tryi, iter, pnt, lastErr);
        biffAdd(GAGE, err); return 1;
      }
      if (!delta) {
        printf("... try %u: delta = 0; nothing to do\n", tryi);
        newErr = lastErr;
        zerodelta = AIR_TRUE;
      } else {
        zerodelta = AIR_FALSE;
        _scalePosSet(parm, pnt, lastPos + delta);
        newErr = _errTotal(parm);
        badStep = newErr > lastErr;
        printf("... try %u: pos[%u] %g + %g = %g;\n    %s: err %g %s %g\n",
               tryi, pnt, lastPos, delta,
               parm->scalePos[pnt],
               badStep ? "*BAD*" : "good",
               newErr, newErr > lastErr ? ">" : "<=", lastErr);
        if (badStep) {
          parm->step[pnt] *= backoff;
          if (parm->step[pnt] < sigeps/10) {
            /* step got so small its stupid to be moving this point */
            printf("... !! step %g < %g pointlessly small, moving on\n", 
                   parm->step[pnt], sigeps/10);
            _scalePosSet(parm, pnt, lastPos);
            newErr = lastErr;
            badStep = AIR_FALSE;
          } else {
            delta = -grad*parm->step[pnt];
          }
        }
      }
      tryi++;
    } while (badStep);
    if (!zerodelta) {
      /* don't update decavg if we moved on because slope was EXACTLY zero */
      decavg = AIR_AFFINE(0, 1, parm->sampleNum,
                          decavg, (lastErr - newErr)/lastErr);
      parm->step[pnt] *= oppor;
    }
    if (decavg <= parm->convEps) {
      printf("%s: converged (%g <= %g) after %u iters\n", me,
             decavg, parm->convEps, iter);
      break;
    } else {
      printf("%s: _____ iter %u done; decavg = %g > %g\n", me,
             iter, decavg, parm->convEps);
    }
    lastErr = newErr;
  }
  if (iter == parm->maxIter) {
    sprintf(err, "%s: failed to converge (%g > %g) after %u iters\n", me,
            decavg, parm->convEps, iter);
    biffAdd(GAGE, err); return 1;
  }
  parm->finalErr = lastErr;
  return 0;
}

int
gageOptimSigCalculate(gageOptimSigParm *parm,
                      double *scalePos, unsigned int num,
                      int volMeasr, int lineMeasr,
                      double convEps, unsigned int maxIter) {
  char me[]="gageOptimSigCalculate", err[BIFF_STRLEN];
  unsigned int ii;
  double tauMax;

  if (!( parm && scalePos && num )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!( AIR_IN_CL(1, parm->dim, 3)
         && parm->ntruth->data )) {
    sprintf(err, "%s: incomplete parm setup?", me);
    biffAdd(GAGE, err); return 1;
  }
  if (num > parm->sampleNumMax) {
    sprintf(err, "%s: parm setup for max %u samples, not %u", me, 
            parm->sampleNumMax, num);
    biffAdd(GAGE, err); return 1;
  }
  /* copy remaining input parms */
  parm->sampleNum = num;
  parm->volMeasr = volMeasr;
  parm->lineMeasr = lineMeasr;
  parm->maxIter = maxIter;
  parm->convEps = convEps;

  /* initialize the scalePos[] array to uniform samples in tau */
  printf("%s: initializing samples ... ", me); fflush(stdout);
  tauMax = gageTauOfSig(parm->sigmaMax);
  for (ii=0; ii<parm->sampleNum; ii++) {
    double tau;
    tau = AIR_AFFINE(0, ii, parm->sampleNum-1, 0, tauMax);
    _scalePosSet(parm, ii, gageSigOfTau(tau));
  }
  printf("done.\n");

  /* set up gage */
  printf("%s: setting up gage ... \n", me);
  if (_gageSetup(parm)) {
    sprintf(err, "%s: problem setting up gage", me);
    biffAdd(GAGE, err); return 1;
  }
  printf("%s: gage setup done.\n", me);

  /* run the optimization */
  if (num > 2) {
    if (_optsigrun(parm)) {
      sprintf(err, "%s: trouble", me);
      biffAdd(GAGE, err); return 1;
    }
  } else {
    printf("%s: num == 2, no optimization, finding error ... ", me);
    fflush(stdout);
    parm->finalErr = _errTotal(parm);
    printf("done.\n");
  }
  
  /* save output */
  for (ii=0; ii<num; ii++) {
    scalePos[ii] = parm->scalePos[ii];
  }

  return 0;
}
