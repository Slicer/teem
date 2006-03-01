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

gageItemEntry
_tenGageTable[TEN_GAGE_ITEM_MAX+1] = {
  /* enum value              len,deriv,  prereqs,                                                                    parent item, parent index, needData*/
  {tenGageTensor,              7,  0,  {-1, -1, -1, -1, -1, -1},                                                              -1,        -1,    0},
  {tenGageConfidence,          1,  0,  {tenGageTensor, -1, -1, -1, -1, -1},                                        tenGageTensor,         0,    0},

  {tenGageTrace,               1,  0,  {tenGageTensor, -1, -1, -1, -1, -1},                                                   -1,        -1,    0},
  {tenGageB,                   1,  0,  {tenGageTensor, -1, -1, -1, -1, -1},                                                   -1,        -1,    0},
  {tenGageDet,                 1,  0,  {tenGageTensor, -1, -1, -1, -1, -1},                                                   -1,        -1,    0},
  {tenGageS,                   1,  0,  {tenGageTensor, -1, -1, -1, -1, -1},                                                   -1,        -1,    0},
  {tenGageQ,                   1,  0,  {tenGageS, tenGageB, -1, -1, -1, -1},                                                  -1,        -1,    0},
  {tenGageFA,                  1,  0,  {tenGageQ, tenGageS, -1, -1, -1, -1},                                                  -1,        -1,    0},
  {tenGageR,                   1,  0,  {tenGageTrace, tenGageB, tenGageDet, tenGageS, -1, -1},                                -1,        -1,    0},
  {tenGageMode,                1,  0,  {tenGageR, tenGageQ, -1, -1, -1, -1},                                                  -1,        -1,    0},
  {tenGageTheta,               1,  0,  {tenGageMode, -1, -1, -1, -1, -1},                                                     -1,        -1,    0},
  {tenGageModeWarp,            1,  0,  {tenGageMode, -1, -1, -1, -1, -1},                                                     -1,        -1,    0},
  {tenGageOmega,               1,  0,  {tenGageFA, tenGageMode, -1, -1, -1, -1},                                              -1,        -1,    0},

  {tenGageEval,                3,  0,  {tenGageTensor, -1, -1, -1, -1, -1},                                                   -1,        -1,    0},
  {tenGageEval0,               1,  0,  {tenGageEval, -1, -1, -1, -1, -1},                                            tenGageEval,         0,    0},
  {tenGageEval1,               1,  0,  {tenGageEval, -1, -1, -1, -1, -1},                                            tenGageEval,         1,    0},
  {tenGageEval2,               1,  0,  {tenGageEval, -1, -1, -1, -1, -1},                                            tenGageEval,         2,    0},
  {tenGageEvec,                9,  0,  {tenGageTensor, -1, -1, -1, -1, -1},                                                   -1,        -1,    0},
  {tenGageEvec0,               3,  0,  {tenGageEvec, -1, -1, -1, -1, -1},                                            tenGageEvec,         0,    0},
  {tenGageEvec1,               3,  0,  {tenGageEvec, -1, -1, -1, -1, -1},                                            tenGageEvec,         3,    0},
  {tenGageEvec2,               3,  0,  {tenGageEvec, -1, -1, -1, -1, -1},                                            tenGageEvec,         6,    0},

  {tenGageTensorGrad,         21,  1,  {-1, -1, -1, -1, -1, -1},                                                              -1,        -1,    0},
  {tenGageTensorGradMag,       3,  1,  {tenGageTensorGrad, -1, -1, -1, -1, -1},                                               -1,        -1,    0},
  {tenGageTensorGradMagMag,    1,  1,  {tenGageTensorGradMag, -1, -1, -1, -1, -1},                                            -1,        -1,    0},

  {tenGageTraceGradVec,        3,  1,  {tenGageTensor, tenGageTensorGrad, -1, -1, -1, -1},                                    -1,        -1,    0},
  {tenGageTraceGradMag,        1,  1,  {tenGageTraceGradVec, -1, -1, -1, -1, -1},                                             -1,        -1,    0},
  {tenGageTraceNormal,         3,  1,  {tenGageTraceGradVec, tenGageTraceGradMag, -1, -1, -1, -1},                            -1,        -1,    0},

  {tenGageBGradVec,            3,  1,  {tenGageTensor, tenGageTensorGrad, -1, -1, -1, -1},                                    -1,        -1,    0},
  {tenGageBGradMag,            1,  1,  {tenGageBGradVec, -1, -1, -1, -1, -1},                                                 -1,        -1,    0},
  {tenGageBNormal,             3,  1,  {tenGageBGradVec, tenGageBGradMag, -1, -1, -1, -1},                                    -1,        -1,    0},

  {tenGageDetGradVec,          3,  1,  {tenGageTensor, tenGageTensorGrad, -1, -1, -1, -1},                                    -1,        -1,    0},
  {tenGageDetGradMag,          1,  1,  {tenGageDetGradVec, -1, -1, -1, -1, -1},                                               -1,        -1,    0},
  {tenGageDetNormal,           3,  1,  {tenGageDetGradVec, tenGageDetGradMag, -1, -1, -1, -1},                                -1,        -1,    0},

  {tenGageSGradVec,            3,  1,  {tenGageTensor, tenGageTensorGrad, -1, -1, -1, -1},                                    -1,        -1,    0},
  {tenGageSGradMag,            1,  1,  {tenGageSGradVec, -1, -1, -1, -1, -1},                                                 -1,        -1,    0},
  {tenGageSNormal,             3,  1,  {tenGageSGradVec, tenGageSGradMag, -1, -1, -1, -1},                                    -1,        -1,    0},

  {tenGageQGradVec,            3,  1,  {tenGageSGradVec, tenGageBGradVec, -1, -1, -1, -1},                                    -1,        -1,    0},
  {tenGageQGradMag,            1,  1,  {tenGageQGradVec, -1, -1, -1, -1, -1},                                                 -1,        -1,    0},
  {tenGageQNormal,             3,  1,  {tenGageQGradVec, tenGageQGradMag, -1, -1, -1, -1},                                    -1,        -1,    0},

  {tenGageFAGradVec,           3,  1,  {tenGageQGradVec, tenGageSGradVec, tenGageFA, -1, -1, -1},                             -1,        -1,    0},
  {tenGageFAGradMag,           1,  1,  {tenGageFAGradVec, -1, -1, -1, -1, -1},                                                -1,        -1,    0},
  {tenGageFANormal,            3,  1,  {tenGageFAGradVec, tenGageFAGradMag, -1, -1, -1, -1},                                  -1,        -1,    0},

  {tenGageRGradVec,            3,  1,  {tenGageR, tenGageTraceGradVec, tenGageBGradVec, tenGageDetGradVec, tenGageSGradVec, -1}, -1,     -1,    0},
  {tenGageRGradMag,            1,  1,  {tenGageRGradVec, -1, -1, -1, -1, -1},                                                 -1,        -1,    0},
  {tenGageRNormal,             3,  1,  {tenGageRGradVec, tenGageRGradMag, -1, -1, -1, -1},                                    -1,        -1,    0},

  {tenGageModeGradVec,         3,  1,  {tenGageRGradVec, tenGageQGradVec, tenGageMode, -1, -1, -1},                           -1,        -1,    0},
  {tenGageModeGradMag,         1,  1,  {tenGageModeGradVec, -1, -1, -1, -1, -1},                                              -1,        -1,    0},
  {tenGageModeNormal,          3,  1,  {tenGageModeGradVec, tenGageModeGradMag, -1, -1, -1, -1},                              -1,        -1,    0},
  
  {tenGageThetaGradVec,        3,  1,  {tenGageRGradVec, tenGageQGradVec, tenGageTheta, -1, -1, -1},                          -1,        -1,    0},
  {tenGageThetaGradMag,        1,  1,  {tenGageThetaGradVec, -1, -1, -1, -1, -1},                                             -1,        -1,    0},
  {tenGageThetaNormal,         3,  1,  {tenGageThetaGradVec, tenGageThetaGradMag, -1, -1, -1, -1},                            -1,        -1,    0},
  
  {tenGageOmegaGradVec,        3,  1,  {tenGageFA, tenGageMode, tenGageFAGradVec, tenGageModeGradVec, -1, -1},                -1,        -1,    0},
  {tenGageOmegaGradMag,        1,  1,  {tenGageOmegaGradVec, -1, -1, -1, -1, -1},                                             -1,        -1,    0},
  {tenGageOmegaNormal,         3,  1,  {tenGageOmegaGradVec, tenGageOmegaGradMag, -1, -1, -1, -1},                            -1,        -1,    0},
  
  {tenGageInvarGrads,          9,  1,  {tenGageTensor, tenGageTensorGrad, -1, -1, -1, -1},                                    -1,        -1,    0},
  {tenGageInvarGradMags,       3,  1,  {tenGageInvarGrads, -1, -1, -1, -1, -1},                                               -1,        -1,    0},
  {tenGageRotTans,             9,  1,  {tenGageTensor, tenGageTensorGrad, tenGageEval, tenGageEvec, -1, -1},                  -1,        -1,    0},
  {tenGageRotTanMags,          3,  1,  {tenGageRotTans, -1, -1, -1, -1, -1},                                                  -1,        -1,    0},

  {tenGageEvalGrads,           9,  1,  {tenGageTensorGrad, tenGageEval, tenGageEvec, -1, -1},                                 -1,        -1,    0},

  {tenGageCl1,                 1,  0,  {tenGageTensor, tenGageEval0, tenGageEval1, tenGageEval2, -1, -1},                     -1,        -1,    0},
  {tenGageCp1,                 1,  0,  {tenGageTensor, tenGageEval0, tenGageEval1, tenGageEval2, -1, -1},                     -1,        -1,    0},
  {tenGageCa1,                 1,  0,  {tenGageTensor, tenGageEval0, tenGageEval1, tenGageEval2, -1, -1},                     -1,        -1,    0},
  {tenGageCl2,                 1,  0,  {tenGageTensor, tenGageEval0, tenGageEval1, tenGageEval2, -1, -1},                     -1,        -1,    0},
  {tenGageCp2,                 1,  0,  {tenGageTensor, tenGageEval0, tenGageEval1, tenGageEval2, -1, -1},                     -1,        -1,    0},
  {tenGageCa2,                 1,  0,  {tenGageTensor, tenGageEval0, tenGageEval1, tenGageEval2, -1, -1},                     -1,        -1,    0},

  {tenGageHessian,            63,  2,  {-1, -1, -1, -1, -1, -1},                                                              -1,        -1,    0},
  {tenGageTraceHessian,        9,  2,  {tenGageHessian, -1, -1, -1, -1, -1},                                                  -1,        -1,    0},
  {tenGageBHessian,            9,  2,  {tenGageTensor, tenGageTensorGrad, tenGageHessian, -1, -1, -1},                        -1,        -1,    0},
  {tenGageDetHessian,          9,  2,  {tenGageTensor, tenGageTensorGrad, tenGageHessian, -1, -1, -1},                        -1,        -1,    0},
  {tenGageSHessian,            9,  2,  {tenGageTensor, tenGageTensorGrad, tenGageHessian, -1, -1, -1},                        -1,        -1,    0},
  {tenGageQHessian,            9,  2,  {tenGageBHessian, tenGageSHessian, -1, -1, -1, -1},                                    -1,        -1,    0},

  {tenGageFAHessian,           9,  2,  {tenGageSHessian, tenGageQHessian, tenGageSGradVec, tenGageQGradVec, tenGageFA, -1},   -1,        -1,    0},
  {tenGageFAHessianEval,       3,  2,  {tenGageFAHessian, -1, -1, -1, -1, -1},                                                -1,        -1,    0},
  {tenGageFAHessianEval0,      1,  2,  {tenGageFAHessianEval, -1, -1, -1, -1, -1},                          tenGageFAHessianEval,         0,    0},
  {tenGageFAHessianEval1,      1,  2,  {tenGageFAHessianEval, -1, -1, -1, -1, -1},                          tenGageFAHessianEval,         1,    0},
  {tenGageFAHessianEval2,      1,  2,  {tenGageFAHessianEval, -1, -1, -1, -1, -1},                          tenGageFAHessianEval,         2,    0},
  {tenGageFAHessianEvec,       9,  2,  {tenGageFAHessian, -1, -1, -1, -1, -1},                                                -1,        -1,    0},
  {tenGageFAHessianEvec0,      3,  2,  {tenGageFAHessianEvec, -1, -1, -1, -1, -1},                          tenGageFAHessianEvec,         0,    0},
  {tenGageFAHessianEvec1,      3,  2,  {tenGageFAHessianEvec, -1, -1, -1, -1, -1},                          tenGageFAHessianEvec,         3,    0},
  {tenGageFAHessianEvec2,      3,  2,  {tenGageFAHessianEvec, -1, -1, -1, -1, -1},                          tenGageFAHessianEvec,         6,    0},

  {tenGageRHessian,            9,  2,  {tenGageR, tenGageRGradVec, tenGageTraceHessian,
                                        tenGageBHessian, tenGageDetHessian, tenGageSHessian},                                 -1,        -1,    0},

  {tenGageModeHessian,         9,  2,  {tenGageR, tenGageQ, tenGageRGradVec, tenGageQGradVec,
                                        tenGageRHessian, tenGageQHessian},                                                    -1,        -1,    0},
  {tenGageModeHessianEval,     3,  2,  {tenGageModeHessian, -1, -1, -1, -1, -1},                                              -1,        -1,    0},
  {tenGageModeHessianEval0,    1,  2,  {tenGageModeHessianEval, -1, -1, -1, -1, -1},                      tenGageModeHessianEval,         0,    0},
  {tenGageModeHessianEval1,    1,  2,  {tenGageModeHessianEval, -1, -1, -1, -1, -1},                      tenGageModeHessianEval,         1,    0},
  {tenGageModeHessianEval2,    1,  2,  {tenGageModeHessianEval, -1, -1, -1, -1, -1},                      tenGageModeHessianEval,         2,    0},
  {tenGageModeHessianEvec,     9,  2,  {tenGageModeHessian, -1, -1, -1, -1, -1},                                              -1,        -1,    0},
  {tenGageModeHessianEvec0,    3,  2,  {tenGageModeHessianEvec, -1, -1, -1, -1, -1},                      tenGageModeHessianEvec,         0,    0},
  {tenGageModeHessianEvec1,    3,  2,  {tenGageModeHessianEvec, -1, -1, -1, -1, -1},                      tenGageModeHessianEvec,         3,    0},
  {tenGageModeHessianEvec2,    3,  2,  {tenGageModeHessianEvec, -1, -1, -1, -1, -1},                      tenGageModeHessianEvec,         6,    0},

  {tenGageOmegaHessian,        9,  2,  {tenGageFA, tenGageMode, tenGageFAGradVec, tenGageModeGradVec,
                                        tenGageFAHessian, tenGageModeHessian},                                                  -1,        -1,    0},
  {tenGageOmegaHessianEval,    3,  2,  {tenGageOmegaHessian, -1, -1, -1, -1, -1},                                               -1,        -1,    0},
  {tenGageOmegaHessianEval0,   1,  2,  {tenGageOmegaHessianEval, -1, -1, -1, -1, -1},                      tenGageOmegaHessianEval,         0,    0},
  {tenGageOmegaHessianEval1,   1,  2,  {tenGageOmegaHessianEval, -1, -1, -1, -1, -1},                      tenGageOmegaHessianEval,         1,    0},
  {tenGageOmegaHessianEval2,   1,  2,  {tenGageOmegaHessianEval, -1, -1, -1, -1, -1},                      tenGageOmegaHessianEval,         2,    0},
  {tenGageOmegaHessianEvec,    9,  2,  {tenGageOmegaHessian, -1, -1, -1, -1, -1},                                               -1,        -1,    0},
  {tenGageOmegaHessianEvec0,   3,  2,  {tenGageOmegaHessianEvec, -1, -1, -1, -1, -1},                      tenGageOmegaHessianEvec,         0,    0},
  {tenGageOmegaHessianEvec1,   3,  2,  {tenGageOmegaHessianEvec, -1, -1, -1, -1, -1},                      tenGageOmegaHessianEvec,         3,    0},
  {tenGageOmegaHessianEvec2,   3,  2,  {tenGageOmegaHessianEvec, -1, -1, -1, -1, -1},                      tenGageOmegaHessianEvec,         6,    0},

  {tenGageAniso, TEN_ANISO_MAX+1,  0,  {tenGageEval0, tenGageEval1, tenGageEval2, -1, -1, -1},                                  -1,        -1,    0}
};

void
_tenGageIv3Print(FILE *file, gageContext *ctx, gagePerVolume *pvl) {
  double *iv3;
  int i, fd;

  fd = 2*ctx->radius;
  iv3 = pvl->iv3 + fd*fd*fd;
  fprintf(file, "iv3[]'s *Dxx* component:\n");
  switch(fd) {
  case 2:
    fprintf(file, "% 10.4f   % 10.4f\n", (float)iv3[6], (float)iv3[7]);
    fprintf(file, "   % 10.4f   % 10.4f\n\n", (float)iv3[4], (float)iv3[5]);
    fprintf(file, "% 10.4f   % 10.4f\n", (float)iv3[2], (float)iv3[3]);
    fprintf(file, "   % 10.4f   % 10.4f\n", (float)iv3[0], (float)iv3[1]);
    break;
  case 4:
    for (i=3; i>=0; i--) {
      fprintf(file, "% 10.4f   % 10.4f   % 10.4f   % 10.4f\n", 
              (float)iv3[12+16*i], (float)iv3[13+16*i], 
              (float)iv3[14+16*i], (float)iv3[15+16*i]);
      fprintf(file, "   % 10.4f  %c% 10.4f   % 10.4f%c   % 10.4f\n", 
              (float)iv3[ 8+16*i], (i==1||i==2)?'\\':' ',
              (float)iv3[ 9+16*i], (float)iv3[10+16*i], (i==1||i==2)?'\\':' ',
              (float)iv3[11+16*i]);
      fprintf(file, "      % 10.4f  %c% 10.4f   % 10.4f%c   % 10.4f\n", 
              (float)iv3[ 4+16*i], (i==1||i==2)?'\\':' ',
              (float)iv3[ 5+16*i], (float)iv3[ 6+16*i], (i==1||i==2)?'\\':' ',
              (float)iv3[ 7+16*i]);
      fprintf(file, "         % 10.4f   % 10.4f   % 10.4f   % 10.4f\n", 
              (float)iv3[ 0+16*i], (float)iv3[ 1+16*i],
              (float)iv3[ 2+16*i], (float)iv3[ 3+16*i]);
      if (i) fprintf(file, "\n");
    }
    break;
  default:
    for (i=0; i<fd*fd*fd; i++) {
      fprintf(file, "  iv3[% 3d,% 3d,% 3d] = % 10.4f\n",
              i%fd, (i/fd)%fd, i/(fd*fd), (float)iv3[i]);
    }
    break;
  }
  return;
}

void
_tenGageFilter(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_tenGageFilter";
  double *fw00, *fw11, *fw22, *tensor, *tgrad, *thess;
  int fd;

  fd = 2*ctx->radius;
  tensor = pvl->directAnswer[tenGageTensor];
  tgrad = pvl->directAnswer[tenGageTensorGrad];
  thess = pvl->directAnswer[tenGageHessian];
  if (!ctx->parm.k3pack) {
    fprintf(stderr, "!%s: sorry, 6pack filtering not implemented\n", me);
    return;
  }
  fw00 = ctx->fw + fd*3*gageKernel00;
  fw11 = ctx->fw + fd*3*gageKernel11;
  fw22 = ctx->fw + fd*3*gageKernel22;
  /* perform the filtering */
  switch (fd) {
  case 2:
#define DOIT_2(J) \
      gageScl3PFilter2(pvl->iv3 + J*8, pvl->iv2 + J*4, pvl->iv1 + J*2, \
                       fw00, fw11, fw22, \
                       tensor + J, tgrad + J*3, thess + J*9, \
                       pvl->needD[0], pvl->needD[1], pvl->needD[2])
    /* HEY: want trilinear interpolation of confidence */
    /* old idea: do average of confidence at 8 corners of containing voxel
    tensor[0] = (pvl->iv3[0] + pvl->iv3[1] + pvl->iv3[2] + pvl->iv3[3]
                 + pvl->iv3[4] + pvl->iv3[5] + pvl->iv3[6] + pvl->iv3[7])/8;
    */
    /* new idea (circa Sat Apr  2 06:59:02 EST 2005):
       do the same filtering- its just too weird for confidence to not 
       be C0 when the filtering result is */
    DOIT_2(0); 
    DOIT_2(1); DOIT_2(2); DOIT_2(3);
    DOIT_2(4); DOIT_2(5); DOIT_2(6); 
    break;
  case 4:
#define DOIT_4(J) \
      gageScl3PFilter4(pvl->iv3 + J*64, pvl->iv2 + J*16, pvl->iv1 + J*4, \
                       fw00, fw11, fw22, \
                       tensor + J, tgrad + J*3, thess + J*9, \
                       pvl->needD[0], pvl->needD[1], pvl->needD[2])
    /* HEY: want trilinear interpolation of confidence */
    /* old: SEE NOTE ABOVE
    tensor[0] = (pvl->iv3[21] + pvl->iv3[22]
                 + pvl->iv3[25] + pvl->iv3[26]
                 + pvl->iv3[37] + pvl->iv3[38] 
                 + pvl->iv3[41] + pvl->iv3[42])/8;
    */
    DOIT_4(0); 
    DOIT_4(1); DOIT_4(2); DOIT_4(3);
    DOIT_4(4); DOIT_4(5); DOIT_4(6); 
    break;
  default:
#define DOIT_N(J)\
      gageScl3PFilterN(fd, \
                       pvl->iv3 + J*fd*fd*fd, \
                       pvl->iv2 + J*fd*fd, pvl->iv1 + J*fd, \
                       fw00, fw11, fw22, \
                       tensor + J, tgrad + J*3, thess + J*9, \
                       pvl->needD[0], pvl->needD[1], pvl->needD[2])
    /* HEY: this sucks: want trilinear interpolation of confidence */
    DOIT_N(0);
    DOIT_N(1); DOIT_N(2); DOIT_N(3);
    DOIT_N(4); DOIT_N(5); DOIT_N(6); 
    break;
  }

  return;
}

void
_tenGageAnswer(gageContext *ctx, gagePerVolume *pvl) {
  /* char me[]="_tenGageAnswer"; */
  double *tenAns, *evalAns, *evecAns, *vecTmp=NULL, *matTmp=NULL,
    *gradDtA=NULL, *gradDtB=NULL, *gradDtC=NULL,
    *gradDtD=NULL, *gradDtE=NULL, *gradDtF=NULL,
    *hessDtA=NULL, *hessDtB=NULL, *hessDtC=NULL,
    *hessDtD=NULL, *hessDtE=NULL, *hessDtF=NULL,
    *gradCbS=NULL, *gradCbB=NULL, *gradCbQ=NULL, *gradCbR=NULL,
    *hessCbS=NULL, *hessCbB=NULL, *hessCbQ=NULL, *hessCbR=NULL,
    gradDdXYZ[21]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  double tmp0, tmp1, tmp2, magTmp=0,
    dtA=0, dtB=0, dtC=0, dtD=0, dtE=0, dtF=0,
    cbQ=0, cbR=0, cbA=0, cbB=0, cbC=0, cbS=0,
    gradCbA[3]={0,0,0}, gradCbC[3]={0,0,0};
  double hessCbA[9]={0,0,0,0,0,0,0,0,0},
    hessCbC[9]={0,0,0,0,0,0,0,0,0};
  int ci;
  float evalAnsF[3], aniso[TEN_ANISO_MAX+1];

  tenAns = pvl->directAnswer[tenGageTensor];
  evalAns = pvl->directAnswer[tenGageEval];
  evecAns = pvl->directAnswer[tenGageEvec];
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTensor)) {
    /* done if doV */
    tenAns[0] = AIR_CLAMP(0, tenAns[0], 1);
    dtA = tenAns[1];
    dtB = tenAns[2];
    dtC = tenAns[3];
    dtD = tenAns[4];
    dtE = tenAns[5];
    dtF = tenAns[6];
    if (ctx->verbose) {
      fprintf(stderr, "tensor = (%g) %g %g %g   %g %g   %g\n", tenAns[0],
              dtA, dtB, dtC, dtD, dtE, dtF);
    }
  }
  /* done if doV 
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageConfidence)) {
  }
  */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTrace)) {
    cbA = -(pvl->directAnswer[tenGageTrace][0] = dtA + dtD + dtF);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageB)) {
    cbB = pvl->directAnswer[tenGageB][0] = 
      dtA*dtD + dtA*dtF + dtD*dtF - dtB*dtB - dtC*dtC - dtE*dtE;
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageDet)) {
    cbC = -(pvl->directAnswer[tenGageDet][0] = 
            2*dtB*dtC*dtE + dtA*dtD*dtF 
            - dtC*dtC*dtD - dtA*dtE*dtE - dtB*dtB*dtF);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageS)) {
    cbS = (pvl->directAnswer[tenGageS][0] = 
           dtA*dtA + dtD*dtD + dtF*dtF
           + 2*dtB*dtB + 2*dtC*dtC + 2*dtE*dtE);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageQ)) {
    cbQ = pvl->directAnswer[tenGageQ][0] = (cbS - cbB)/9;
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageFA)) {
    tmp0 = (cbS
            ? cbQ/cbS
            : 0);
    tmp0 = AIR_MAX(0, tmp0);
    pvl->directAnswer[tenGageFA][0] = 3*sqrt(tmp0);
    /*
    if (!AIR_EXISTS(pvl->directAnswer[tenGageFA][0])) { 
      fprintf(stderr, "!%s: cbS = %g, cbQ = %g, cbQ/(epsilon + cbS) = %g\n"
              "tmp0 = max(0, cbQ/(epsilon + cbS)) = %g\n"
              "sqrt(tmp0) = %g --> %g\n", me,
              cbS, cbQ, cbQ/(epsilon + cbS),
              tmp0, sqrt(tmp0), pvl->directAnswer[tenGageFA][0]);
    }
    */
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageR)) {
    cbR = pvl->directAnswer[tenGageR][0] =
      (5*cbA*cbB - 27*cbC - 2*cbA*cbS)/54;
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageMode)) {
    double cbQQQ;
    cbQQQ = 2*AIR_MAX(0, cbQ*cbQ*cbQ);
    tmp0 = cbQQQ ? cbR/(sqrt(cbQQQ)) : 0;
    pvl->directAnswer[tenGageMode][0] = AIR_CLAMP(-1, tmp0, 1);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageOmega)) {
    pvl->directAnswer[tenGageOmega][0] = 
      pvl->directAnswer[tenGageFA][0]*(1+pvl->directAnswer[tenGageMode][0])/2;
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTheta)) {
    pvl->directAnswer[tenGageTheta][0] = 
      acos(-pvl->directAnswer[tenGageMode][0])/AIR_PI;
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageModeWarp)) {
    pvl->directAnswer[tenGageModeWarp][0] = 
      cos((1-pvl->directAnswer[tenGageMode][0])*AIR_PI/2);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageEvec)) {
    /* we do the longer process to get eigenvectors, and in the process
       we always find the eigenvalues, whether or not they were asked for */
    tenEigensolve_d(evalAns, evecAns, tenAns);
  } else if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageEval)) {
    /* else eigenvectors are NOT needed, but eigenvalues ARE needed */
    tenEigensolve_d(evalAns, NULL, tenAns);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTensorGrad)) {
    /* done if doD1 */
    /* still have to set up pointer variables that item answers
       below will rely on as short-cuts */
    vecTmp = pvl->directAnswer[tenGageTensorGrad];
    gradDtA = vecTmp + 1*3;
    gradDtB = vecTmp + 2*3;
    gradDtC = vecTmp + 3*3;
    gradDtD = vecTmp + 4*3;
    gradDtE = vecTmp + 5*3;
    gradDtF = vecTmp + 6*3;
    TEN_T_SET(gradDdXYZ + 0*7, tenAns[0],
              gradDtA[0], gradDtB[0], gradDtC[0],
              gradDtD[0], gradDtE[0],
              gradDtF[0]);
    TEN_T_SET(gradDdXYZ + 1*7, tenAns[0],
              gradDtA[1], gradDtB[1], gradDtC[1],
              gradDtD[1], gradDtE[1],
              gradDtF[1]);
    TEN_T_SET(gradDdXYZ + 2*7, tenAns[0],
              gradDtA[2], gradDtB[2], gradDtC[2],
              gradDtD[2], gradDtE[2],
              gradDtF[2]);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTensorGradMag)) {
    vecTmp = pvl->directAnswer[tenGageTensorGradMag];
    vecTmp[0] = sqrt(TEN_T_DOT(gradDdXYZ + 0*7, gradDdXYZ + 0*7));
    vecTmp[1] = sqrt(TEN_T_DOT(gradDdXYZ + 1*7, gradDdXYZ + 1*7));
    vecTmp[2] = sqrt(TEN_T_DOT(gradDdXYZ + 2*7, gradDdXYZ + 2*7));
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTensorGradMag)) {
    pvl->directAnswer[tenGageTensorGradMagMag][0] = ELL_3V_LEN(vecTmp);
  }

  /* --- Trace --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTraceGradVec)) {
    vecTmp = pvl->directAnswer[tenGageTraceGradVec];
    ELL_3V_ADD3(vecTmp, gradDtA, gradDtD, gradDtF);
    ELL_3V_SCALE(gradCbA, -1, vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTraceGradMag)) {
    magTmp = pvl->directAnswer[tenGageTraceGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTraceNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageTraceNormal],
                 magTmp ? 1/magTmp : 0, vecTmp);
  }
  /* --- B --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageBGradVec)) {
    gradCbB = vecTmp = pvl->directAnswer[tenGageBGradVec];
    ELL_3V_SCALE_ADD6(vecTmp, 
                      dtD + dtF, gradDtA,
                      dtA + dtF, gradDtD,
                      dtA + dtD, gradDtF,
                      -2*dtB, gradDtB,
                      -2*dtC, gradDtC,
                      -2*dtE, gradDtE);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageBGradMag)) {
    magTmp = pvl->directAnswer[tenGageBGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageBNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageBNormal],
                 magTmp ? 1/magTmp : 0, vecTmp);
  }
  /* --- Det --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageDetGradVec)) {
    vecTmp = pvl->directAnswer[tenGageDetGradVec];
    ELL_3V_SCALE_ADD6(vecTmp,
                      dtD*dtF - dtE*dtE, gradDtA,
                      2*(dtC*dtE - dtB*dtF), gradDtB,
                      2*(dtB*dtE - dtC*dtD), gradDtC,
                      dtA*dtF - dtC*dtC, gradDtD,
                      2*(dtB*dtC - dtA*dtE), gradDtE,
                      dtA*dtD - dtB*dtB, gradDtF);
    ELL_3V_SCALE(gradCbC, -1, vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageDetGradMag)) {
    magTmp = pvl->directAnswer[tenGageDetGradMag][0] =
      AIR_CAST(float, ELL_3V_LEN(vecTmp));
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageDetNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageDetNormal],
                 magTmp ? 1/magTmp : 0, vecTmp);
  }
  /* --- S --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageSGradVec)) {
    gradCbS = vecTmp = pvl->directAnswer[tenGageSGradVec];
    ELL_3V_SCALE_ADD6(vecTmp,
                      2*dtA, gradDtA,
                      2*dtD, gradDtD,
                      2*dtF, gradDtF,
                      4*dtB, gradDtB,
                      4*dtC, gradDtC,
                      4*dtE, gradDtE);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageSGradMag)) {
    magTmp = pvl->directAnswer[tenGageSGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageSNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageSNormal],
                 magTmp ? 1/magTmp : 0, vecTmp);
  }
  /* --- Q --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageQGradVec)) {
    gradCbQ = vecTmp = pvl->directAnswer[tenGageQGradVec];
    ELL_3V_SCALE_ADD2(vecTmp,
                      1.0/9, gradCbS, 
                      -1.0/9, gradCbB);

  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageQGradMag)) {
    magTmp = pvl->directAnswer[tenGageQGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageQNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageQNormal],
                 magTmp ? 1/magTmp : 0, vecTmp);
  }
  /* --- FA --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageFAGradVec)) {
    vecTmp = pvl->directAnswer[tenGageFAGradVec];
    tmp2 = AIR_MAX(0, pvl->directAnswer[tenGageFA][0]);
    tmp0 = cbQ ? tmp2/(2*cbQ) : 0;
    tmp1 = cbS ? -tmp2/(2*cbS) : 0;
    ELL_3V_SCALE_ADD2(vecTmp,
                      tmp0, gradCbQ, 
                      tmp1, gradCbS);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageFAGradMag)) {
    magTmp = pvl->directAnswer[tenGageFAGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageFANormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageFANormal],
                 magTmp ? 1/magTmp : 0, vecTmp);
  }
  /* --- R --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageRGradVec)) {
    gradCbR = vecTmp = pvl->directAnswer[tenGageRGradVec];
    ELL_3V_SCALE_ADD4(vecTmp,
                      (5*cbB - 2*cbS)/54, gradCbA,
                      5*cbA/54, gradCbB,
                      -0.5, gradCbC,
                      -cbA/27, gradCbS);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageRGradMag)) {
    magTmp = pvl->directAnswer[tenGageRGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageRNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageRNormal],
                 magTmp ? 1/magTmp : 0, vecTmp);
  }
  /* --- Mode --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageModeGradVec)) {
    vecTmp = pvl->directAnswer[tenGageModeGradVec];
    tmp1 = AIR_MAX(0, cbQ*cbQ*cbQ);
    tmp1 = tmp1 ? sqrt(1/tmp1) : 0;
    tmp0 = cbQ ? -tmp1*3*cbR/(2*cbQ) : 0;
    ELL_3V_SCALE_ADD2(vecTmp,
                      tmp0, gradCbQ,
                      tmp1, gradCbR);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageModeGradMag)) {
    magTmp = pvl->directAnswer[tenGageModeGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageModeNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageModeNormal],
                 magTmp ? 1/magTmp : 0, vecTmp);
  }
  /* --- Theta --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageThetaGradVec)) {
    vecTmp = pvl->directAnswer[tenGageThetaGradVec];
    tmp1 = AIR_MAX(0, cbQ*cbQ*cbQ);
    tmp0 = tmp1 ? cbR*cbR/tmp1 : 0;
    tmp1 = sqrt(tmp1)*sqrt(1.0 - tmp0);
    tmp1 = tmp1 ? 1/(AIR_PI*tmp1) : 0.0;
    tmp0 = cbQ ? -tmp1*3*cbR/(2*cbQ) : 0.0;
    ELL_3V_SCALE_ADD2(vecTmp,
                      tmp0, gradCbQ,
                      tmp1, gradCbR);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageThetaGradMag)) {
    magTmp = pvl->directAnswer[tenGageThetaGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageThetaNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageThetaNormal],
                 magTmp ? 1/magTmp : 0, vecTmp);
  }
  /* --- Omega --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageOmegaGradVec)) {
    double fa, mode, *faGrad, *modeGrad;
    vecTmp = pvl->directAnswer[tenGageOmegaGradVec];
    fa = pvl->directAnswer[tenGageFA][0];
    mode = pvl->directAnswer[tenGageMode][0];
    faGrad = pvl->directAnswer[tenGageFAGradVec];
    modeGrad = pvl->directAnswer[tenGageModeGradVec];
    ELL_3V_SCALE_ADD2(vecTmp,
                      (1+mode)/2, faGrad,
                      fa/2, modeGrad);
    ELL_3V_SCALE(vecTmp, (1+mode)/2, faGrad);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageOmegaGradMag)) {
    magTmp = pvl->directAnswer[tenGageOmegaGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageOmegaNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageOmegaNormal],
                 magTmp ? 1/magTmp : 0, vecTmp);
  }
  /* --- Invariant gradients + rotation tangents --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageInvarGrads)) {
    double mu1Grad[7], mu2Grad[7], mu2Norm,
      skwGrad[7], skwNorm, copyT[7];
    
    TEN_T_COPY(copyT, tenAns);
    tenInvariantGradients_d(mu1Grad, 
                            mu2Grad, &mu2Norm,
                            skwGrad, &skwNorm,
                            copyT);
    ELL_3V_SET(pvl->directAnswer[tenGageInvarGrads] + 0*3,
               TEN_T_DOT(mu1Grad, gradDdXYZ + 0*7),
               TEN_T_DOT(mu1Grad, gradDdXYZ + 1*7),
               TEN_T_DOT(mu1Grad, gradDdXYZ + 2*7));
    ELL_3V_SET(pvl->directAnswer[tenGageInvarGrads] + 1*3,
               TEN_T_DOT(mu2Grad, gradDdXYZ + 0*7),
               TEN_T_DOT(mu2Grad, gradDdXYZ + 1*7),
               TEN_T_DOT(mu2Grad, gradDdXYZ + 2*7));
    ELL_3V_SET(pvl->directAnswer[tenGageInvarGrads] + 2*3,
               TEN_T_DOT(skwGrad, gradDdXYZ + 0*7),
               TEN_T_DOT(skwGrad, gradDdXYZ + 1*7),
               TEN_T_DOT(skwGrad, gradDdXYZ + 2*7));
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageInvarGradMags)) {
    ELL_3V_SET(pvl->directAnswer[tenGageInvarGradMags],
               ELL_3V_LEN(pvl->directAnswer[tenGageInvarGrads] + 0*3),
               ELL_3V_LEN(pvl->directAnswer[tenGageInvarGrads] + 1*3),
               ELL_3V_LEN(pvl->directAnswer[tenGageInvarGrads] + 2*3));
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageEvalGrads)) {
    double matOut[9], tenOut[9];
    int evi;
    for (evi=0; evi<=2; evi++) {
      ELL_3MV_OUTER(matOut, evecAns + evi*3, evecAns + evi*3);
      TEN_M2T(tenOut, matOut);
      ELL_3V_SET(pvl->directAnswer[tenGageEvalGrads] + evi*3,
                 TEN_T_DOT(tenOut, gradDdXYZ + 0*7),
                 TEN_T_DOT(tenOut, gradDdXYZ + 1*7),
                 TEN_T_DOT(tenOut, gradDdXYZ + 2*7));
    }
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageRotTans)) {
    double phi1[7], phi2[7], phi3[7], evec[9];

    ELL_9V_COPY(evec, evecAns);
    tenRotationTangents_d(phi1, phi2, phi3, evec);
    ELL_3V_SET(pvl->directAnswer[tenGageRotTans] + 0*3,
               TEN_T_DOT(phi1, gradDdXYZ + 0*7),
               TEN_T_DOT(phi1, gradDdXYZ + 1*7),
               TEN_T_DOT(phi1, gradDdXYZ + 2*7));
    ELL_3V_SET(pvl->directAnswer[tenGageRotTans] + 1*3,
               TEN_T_DOT(phi2, gradDdXYZ + 0*7),
               TEN_T_DOT(phi2, gradDdXYZ + 1*7),
               TEN_T_DOT(phi2, gradDdXYZ + 2*7));
    ELL_3V_SET(pvl->directAnswer[tenGageRotTans] + 2*3,
               TEN_T_DOT(phi3, gradDdXYZ + 0*7),
               TEN_T_DOT(phi3, gradDdXYZ + 1*7),
               TEN_T_DOT(phi3, gradDdXYZ + 2*7));
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageRotTanMags)) {
    ELL_3V_SET(pvl->directAnswer[tenGageRotTanMags],
               ELL_3V_LEN(pvl->directAnswer[tenGageRotTans] + 0*3),
               ELL_3V_LEN(pvl->directAnswer[tenGageRotTans] + 1*3),
               ELL_3V_LEN(pvl->directAnswer[tenGageRotTans] + 2*3));
  }
  /* --- C{l,p,a}1 --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageCl1)) {
    tmp0 = tenAnisoEval_d(evalAns, tenAniso_Cl1);
    pvl->directAnswer[tenGageCl1][0] = AIR_CLAMP(0, tmp0, 1);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageCp1)) {
    tmp0 = tenAnisoEval_d(evalAns, tenAniso_Cp1);
    pvl->directAnswer[tenGageCp1][0] = AIR_CLAMP(0, tmp0, 1);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageCa1)) {
    tmp0 = tenAnisoEval_d(evalAns, tenAniso_Ca1);
    pvl->directAnswer[tenGageCa1][0] = AIR_CLAMP(0, tmp0, 1);
  }
  /* --- C{l,p,a}2 --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageCl2)) {
    tmp0 = tenAnisoEval_d(evalAns, tenAniso_Cl2);
    pvl->directAnswer[tenGageCl2][0] = AIR_CLAMP(0, tmp0, 1);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageCp2)) {
    tmp0 = tenAnisoEval_d(evalAns, tenAniso_Cp2);
    pvl->directAnswer[tenGageCp2][0] = AIR_CLAMP(0, tmp0, 1);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageCa2)) {
    tmp0 = tenAnisoEval_d(evalAns, tenAniso_Ca2);
    pvl->directAnswer[tenGageCa2][0] = AIR_CLAMP(0, tmp0, 1);
  }
  /* --- Hessian madness (the derivative, not the soldier) --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageHessian)) {
    /* done if doD2; still have to set up pointers */
    matTmp = pvl->directAnswer[tenGageHessian];
    hessDtA = matTmp + 1*9;
    hessDtB = matTmp + 2*9;
    hessDtC = matTmp + 3*9;
    hessDtD = matTmp + 4*9;
    hessDtE = matTmp + 5*9;
    hessDtF = matTmp + 6*9;
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTraceHessian)) {
    ELL_3M_SCALE_ADD3(pvl->directAnswer[tenGageTraceHessian],
                      1.0, hessDtA,
                      1.0, hessDtD,
                      1.0, hessDtF);
    ELL_3M_SCALE(hessCbA, -1, pvl->directAnswer[tenGageTraceHessian]);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageBHessian)) {
    hessCbB = matTmp = pvl->directAnswer[tenGageBHessian];
    ELL_3M_ZERO_SET(matTmp);
    ELL_3M_SCALE_INCR(matTmp, dtB, hessDtB);
    ELL_3M_SCALE_INCR(matTmp, dtC, hessDtC);
    ELL_3M_SCALE_INCR(matTmp, dtE, hessDtE);
    ELL_3MV_OUTER_INCR(matTmp, gradDtB, gradDtB);
    ELL_3MV_OUTER_INCR(matTmp, gradDtC, gradDtC);
    ELL_3MV_OUTER_INCR(matTmp, gradDtE, gradDtE);
    ELL_3M_SCALE(matTmp, -2, matTmp);
    ELL_3MV_OUTER_INCR(matTmp, gradDtD, gradDtA);
    ELL_3MV_OUTER_INCR(matTmp, gradDtF, gradDtA);
    ELL_3MV_OUTER_INCR(matTmp, gradDtA, gradDtD);
    ELL_3MV_OUTER_INCR(matTmp, gradDtF, gradDtD);
    ELL_3MV_OUTER_INCR(matTmp, gradDtA, gradDtF);
    ELL_3MV_OUTER_INCR(matTmp, gradDtD, gradDtF);
    ELL_3M_SCALE_INCR(matTmp, dtD + dtF, hessDtA);
    ELL_3M_SCALE_INCR(matTmp, dtA + dtF, hessDtD);
    ELL_3M_SCALE_INCR(matTmp, dtA + dtD, hessDtF);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageDetHessian)) {
    double tmp[3];
    matTmp = pvl->directAnswer[tenGageDetHessian];
    ELL_3M_ZERO_SET(matTmp);
    ELL_3V_SCALE_ADD3(tmp, dtD, gradDtF,
                      dtF, gradDtD,
                      -2*dtE, gradDtE);
    ELL_3MV_OUTER_INCR(matTmp, tmp, gradDtA);
    ELL_3M_SCALE_INCR(matTmp, dtD*dtF - dtE*dtE, hessDtA);
    ELL_3V_SCALE_ADD4(tmp, 2*dtC, gradDtE,
                      2*dtE, gradDtC,
                      -2*dtB, gradDtF,
                      -2*dtF, gradDtB);
    ELL_3MV_OUTER_INCR(matTmp, tmp, gradDtB);
    ELL_3M_SCALE_INCR(matTmp, 2*(dtC*dtE - dtB*dtF), hessDtB);
    ELL_3V_SCALE_ADD4(tmp, 2*dtB, gradDtE,
                      2*dtE, gradDtB,
                      -2*dtC, gradDtD,
                      -2*dtD, gradDtC);
    ELL_3MV_OUTER_INCR(matTmp, tmp, gradDtC);
    ELL_3M_SCALE_INCR(matTmp, 2*(dtB*dtE - dtC*dtD), hessDtC);
    ELL_3V_SCALE_ADD3(tmp, dtA, gradDtF,
                      dtF, gradDtA,
                      -2*dtC, gradDtC);
    ELL_3MV_OUTER_INCR(matTmp, tmp, gradDtD);
    ELL_3M_SCALE_INCR(matTmp, dtA*dtF - dtC*dtC, hessDtD);
    ELL_3V_SCALE_ADD4(tmp, 2*dtB, gradDtC,
                      2*dtC, gradDtB,
                      -2*dtA, gradDtE,
                      -2*dtE, gradDtA);
    ELL_3MV_OUTER_INCR(matTmp, tmp, gradDtE);
    ELL_3M_SCALE_INCR(matTmp, 2*(dtB*dtC - dtA*dtE), hessDtE);
    ELL_3V_SCALE_ADD3(tmp, dtA, gradDtD,
                      dtD, gradDtA,
                      -2*dtB, gradDtB);
    ELL_3MV_OUTER_INCR(matTmp, tmp, gradDtF);
    ELL_3M_SCALE_INCR(matTmp, dtA*dtD - dtB*dtB, hessDtF);
    ELL_3M_SCALE(hessCbC, -1, pvl->directAnswer[tenGageDetHessian]);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageSHessian)) {
    hessCbS = matTmp = pvl->directAnswer[tenGageSHessian];
    ELL_3M_ZERO_SET(matTmp);
    ELL_3M_SCALE_INCR(matTmp,      dtB, hessDtB);
    ELL_3MV_OUTER_INCR(matTmp, gradDtB, gradDtB);
    ELL_3M_SCALE_INCR(matTmp,      dtC, hessDtC);
    ELL_3MV_OUTER_INCR(matTmp, gradDtC, gradDtC);
    ELL_3M_SCALE_INCR(matTmp,      dtE, hessDtE);
    ELL_3MV_OUTER_INCR(matTmp, gradDtE, gradDtE);
    ELL_3M_SCALE(matTmp, 2, matTmp);
    ELL_3M_SCALE_INCR(matTmp,      dtA, hessDtA);
    ELL_3MV_OUTER_INCR(matTmp, gradDtA, gradDtA);
    ELL_3M_SCALE_INCR(matTmp,      dtD, hessDtD);
    ELL_3MV_OUTER_INCR(matTmp, gradDtD, gradDtD);
    ELL_3M_SCALE_INCR(matTmp,      dtF, hessDtF);
    ELL_3MV_OUTER_INCR(matTmp, gradDtF, gradDtF);
    ELL_3M_SCALE(matTmp, 2, matTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageQHessian)) {
    hessCbQ = pvl->directAnswer[tenGageQHessian];
    ELL_3M_SCALE_ADD2(hessCbQ,
                      1.0/9, hessCbS, 
                      -1.0/9, hessCbB);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageFAHessian)) {
    double tmpQ, rQ, orQ, oQ, tmpS, rS, orS, oS;
    tmpQ = AIR_MAX(0, cbQ);
    tmpS = AIR_MAX(0, cbS);
    oQ = tmpQ ? 1/tmpQ : 0;
    oS = tmpS ? 1/tmpS : 0;
    rQ = sqrt(tmpQ);
    rS = sqrt(tmpS);
    orQ = rQ ? 1/rQ : 0;
    orS = rS ? 1/rS : 0;
    matTmp = pvl->directAnswer[tenGageFAHessian];
    ELL_3M_ZERO_SET(matTmp);
    ELL_3M_SCALE_INCR(matTmp, orS*orQ, hessCbQ);
    ELL_3M_SCALE_INCR(matTmp, -rQ*orS*oS, hessCbS);
    ELL_3MV_SCALE_OUTER_INCR(matTmp, -orS*orQ*oQ/2, gradCbQ, gradCbQ);
    ELL_3MV_SCALE_OUTER_INCR(matTmp, 3*rQ*orS*oS*oS/2, gradCbS, gradCbS);
    ELL_3MV_SCALE_OUTER_INCR(matTmp, -orS*oS*orQ/2, gradCbS, gradCbQ);
    ELL_3MV_SCALE_OUTER_INCR(matTmp, -orQ*orS*oS/2, gradCbQ, gradCbS);
    ELL_3M_SCALE(matTmp, 3.0/2, matTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageFAHessianEvec)) {
    /* HEY: cut-and-paste from tenGageEvec, with minimal changes */
    double fakeTen[7];
    TEN_M2T(fakeTen, pvl->directAnswer[tenGageFAHessian]);
    tenEigensolve_d(pvl->directAnswer[tenGageFAHessianEval],
                    pvl->directAnswer[tenGageFAHessianEvec], fakeTen);
  } else if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageFAHessianEval)) {
    double fakeTen[7];
    TEN_M2T(fakeTen, pvl->directAnswer[tenGageFAHessian]);
    /* else eigenvectors are NOT needed, but eigenvalues ARE needed */
    tenEigensolve_d(pvl->directAnswer[tenGageFAHessianEval], NULL, fakeTen);
  }

  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageRHessian)) {
    hessCbR = matTmp = pvl->directAnswer[tenGageRHessian];
    ELL_3M_ZERO_SET(matTmp);
    ELL_3M_SCALE_INCR(matTmp, 5*cbB - 2*cbS, hessCbA);
    ELL_3MV_SCALE_OUTER_INCR(matTmp, 5, gradCbB, gradCbA);
    ELL_3MV_SCALE_OUTER_INCR(matTmp, -2, gradCbS, gradCbA);
    ELL_3M_SCALE_INCR(matTmp, 5*cbA, hessCbB);
    ELL_3MV_SCALE_OUTER_INCR(matTmp, 5, gradCbA, gradCbB);
    ELL_3M_SCALE_INCR(matTmp, -27, hessCbC);
    ELL_3M_SCALE_INCR(matTmp, -2*cbA, hessCbS);
    ELL_3MV_SCALE_OUTER_INCR(matTmp, -2, gradCbA, gradCbS);
    ELL_3M_SCALE(matTmp, 1.0/54, matTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageModeHessian)) {
    double tmpQ, oQ, rQ;
    tmpQ = AIR_MAX(0, cbQ);
    rQ = sqrt(tmpQ);
    oQ = tmpQ ? 1/tmpQ : 0;
    matTmp = pvl->directAnswer[tenGageModeHessian];
    ELL_3M_ZERO_SET(matTmp);
    ELL_3M_SCALE_INCR(matTmp, -(3.0/2)*cbR, hessCbQ);
    ELL_3MV_SCALE_OUTER_INCR(matTmp, (15.0/4)*cbR*oQ, gradCbQ, gradCbQ);
    ELL_3MV_SCALE_OUTER_INCR(matTmp, -(3.0/2), gradCbR, gradCbQ);
    ELL_3M_SCALE_INCR(matTmp, cbQ, hessCbR);
    ELL_3MV_SCALE_OUTER_INCR(matTmp, -(3.0/2), gradCbQ, gradCbR);
    tmp0 = (tmpQ && rQ) ? 1/(tmpQ*tmpQ*rQ) : 0.0;
    ELL_3M_SCALE(matTmp, tmp0, matTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageModeHessianEvec)) {
    /* HEY: cut-and-paste from tenGageFAHessianEvec */
    double fakeTen[7];
    TEN_M2T(fakeTen, pvl->directAnswer[tenGageModeHessian]);
    tenEigensolve_d(pvl->directAnswer[tenGageModeHessianEval],
                    pvl->directAnswer[tenGageModeHessianEvec], fakeTen);
  } else if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageModeHessianEval)) {
    double fakeTen[7];
    TEN_M2T(fakeTen, pvl->directAnswer[tenGageModeHessian]);
    /* else eigenvectors are NOT needed, but eigenvalues ARE needed */
    tenEigensolve_d(pvl->directAnswer[tenGageModeHessianEval], NULL, fakeTen);
  }

  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageOmegaHessian)) {
    double fa, mode, *modeGrad, *faGrad, *modeHess, *faHess;
    fa = pvl->directAnswer[tenGageFA][0];
    mode = pvl->directAnswer[tenGageMode][0];
    faGrad = pvl->directAnswer[tenGageFAGradVec];
    modeGrad = pvl->directAnswer[tenGageModeGradVec];
    faHess = pvl->directAnswer[tenGageFAHessian];
    modeHess = pvl->directAnswer[tenGageModeHessian];
    matTmp = pvl->directAnswer[tenGageOmegaHessian];
    ELL_3M_ZERO_SET(matTmp);
    ELL_3M_SCALE_INCR(matTmp, (1+mode)/2, faHess);
    ELL_3M_SCALE_INCR(matTmp, fa/2, modeHess);
    ELL_3MV_OUTER_INCR(matTmp, modeGrad, faGrad);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageOmegaHessianEvec)) {
    /* HEY: cut-and-paste from tenGageFAHessianEvec */
    double fakeTen[7];
    TEN_M2T(fakeTen, pvl->directAnswer[tenGageOmegaHessian]);
    tenEigensolve_d(pvl->directAnswer[tenGageOmegaHessianEval],
                    pvl->directAnswer[tenGageOmegaHessianEvec], fakeTen);
  } else if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageOmegaHessianEval)) {
    double fakeTen[7];
    TEN_M2T(fakeTen, pvl->directAnswer[tenGageOmegaHessian]);
    /* else eigenvectors are NOT needed, but eigenvalues ARE needed */
    tenEigensolve_d(pvl->directAnswer[tenGageOmegaHessianEval], NULL, fakeTen);
  }

  /* --- Aniso --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageAniso)) {
    ELL_3V_COPY(evalAnsF, evalAns);
    tenAnisoCalc_f(aniso, evalAnsF);
    for (ci=0; ci<=TEN_ANISO_MAX; ci++) {
      pvl->directAnswer[tenGageAniso][ci] = aniso[ci];
    }
  }
  return;
}

gageKind
_tenGageKind = {
  "tensor",
  &_tenGage,
  1,
  7,
  TEN_GAGE_ITEM_MAX,
  _tenGageTable,
  _tenGageIv3Print,
  _tenGageFilter,
  _tenGageAnswer,
  NULL, NULL, NULL,
  NULL
};
gageKind *
tenGageKind = &_tenGageKind;
