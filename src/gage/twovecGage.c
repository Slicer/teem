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

#include "gage.h"
#include "privateGage.h"

/* clang-format off */
static const char *
_gage2VecStr[] = {
  "(unknown gage2Vec)",
  "vector",
  "vector0",
  "vector1",
  "length",
};

static const char *
_gage2VecDesc[] = {
  "unknown gage2Vec query",
  "component-wise-interpolated vector",
  "vector[0]",
  "vector[1]",
  "length of vector",
};

static const int
_gage2VecVal[] = {
  gage2VecUnknown,
  gage2VecVector,
  gage2VecVector0,
  gage2VecVector1,
  gage2VecLength,
};

#define GV_V   gage2VecVector
#define GV_V0  gage2VecVector0
#define GV_V1  gage2VecVector1
#define GV_L   gage2VecLength

static const char *
_gage2VecStrEqv[] = {
  "v", "vector", "vec",
  "v0", "vector0", "vec0",
  "v1", "vector1", "vec1",
  "l", "length", "len",
  ""
};

static const int
_gage2VecValEqv[] = {
  GV_V, GV_V, GV_V,
  GV_V0, GV_V0, GV_V0,
  GV_V1, GV_V1, GV_V1,
  GV_L, GV_L, GV_L,
};

static const airEnum
_gage2Vec = {
  "gage2Vec",
  GAGE_2VEC_ITEM_MAX,
  _gage2VecStr, _gage2VecVal,
  _gage2VecDesc,
  _gage2VecStrEqv, _gage2VecValEqv,
  AIR_FALSE
};
const airEnum *const
gage2Vec = &_gage2Vec;

static gageItemEntry
_gage2VecTable[GAGE_2VEC_ITEM_MAX+1] = {
  /* enum value         len, deriv, prereqs,                                                  parent item, parent index, needData */
  {gage2VecUnknown,        0,  0,   {0},                                                                0,      0,       AIR_FALSE},
  {gage2VecVector,         2,  0,   {0},                                                                0,      0,       AIR_FALSE},
  {gage2VecVector0,        1,  0,   {gage2VecVector},                                      gage2VecVector,      0,       AIR_FALSE},
  {gage2VecVector1,        1,  0,   {gage2VecVector},                                      gage2VecVector,      1,       AIR_FALSE},
  {gage2VecLength,         1,  0,   {gage2VecVector},                                                   0,      0,       AIR_FALSE},
};

static void
_gage2VecFilter(gageContext *ctx, gagePerVolume *pvl) {
  static const char me[] = "_gage2VecFilter";
  double *fw00, *fw11, *fw22, *vec;
  int fd;
  gageScl3PFilter_t *filter[5] = {NULL, gageScl3PFilter2, gageScl3PFilter4,
                                  gageScl3PFilter6, gageScl3PFilter8};
  unsigned int valIdx;

  fd = 2*ctx->radius;
  vec  = pvl->directAnswer[gage2VecVector];
  if (!ctx->parm.k3pack) {
    fprintf(stderr, "!%s: sorry, 6pack filtering not implemented\n", me);
    return;
  }
  fw00 = ctx->fw + fd*3*gageKernel00;
  fw11 = ctx->fw + fd*3*gageKernel11;
  fw22 = ctx->fw + fd*3*gageKernel22;
  /* perform the filtering */
  if (fd <= 8) {
    for (valIdx=0; valIdx<2; valIdx++) {
      filter[ctx->radius](ctx->shape,
                          pvl->iv3 + valIdx*fd*fd*fd,
                          pvl->iv2 + valIdx*fd*fd,
                          pvl->iv1 + valIdx*fd,
                          fw00, fw11, fw22,
                          vec + valIdx, NULL, NULL, /* jac + valIdx*2, hes + valIdx*4, */
                          pvl->needD);
    }
  } else {
    for (valIdx=0; valIdx<2; valIdx++) {
      gageScl3PFilterN(ctx->shape, fd,
                       pvl->iv3 + valIdx*fd*fd*fd,
                       pvl->iv2 + valIdx*fd*fd,
                       pvl->iv1 + valIdx*fd,
                       fw00, fw11, fw22,
                       vec + valIdx, NULL, NULL, /* jac + valIdx*2, hes + valIdx*4, */
                       pvl->needD);
    }
  }

  return;
}

static void
_gage2VecAnswer(gageContext *ctx, gagePerVolume *pvl) {
  /* static const char me[] = "_gage2VecAnswer"; */
  double *vecAns;
  /* int asw; */

  AIR_UNUSED(ctx);
  vecAns          = pvl->directAnswer[gage2VecVector];

  if (GAGE_QUERY_ITEM_TEST(pvl->query, gage2VecVector)) {
    /* done if doV */
    /*
    if (ctx->verbose) {
      fprintf(stderr, "vec = %f %f", vecAns[0], vecAns[1]);
    }
    */
  }
  /* done if doV
  if (GAGE_QUERY_ITEM_TEST(pvl->query, gage2VecVector{0,1})) {
  }
  */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, gage2VecLength)) {
    pvl->directAnswer[gage2VecLength][0] = ELL_2V_LEN(vecAns);
  }
  return;
}

static void
_gage2VecIv3Print (FILE *file, gageContext *ctx, gagePerVolume *pvl) {

  AIR_UNUSED(ctx);
  AIR_UNUSED(pvl);
  fprintf(file, "_gage2VecIv3Print() not implemented\n");
}

static gageKind
_gageKind2Vec = {
  AIR_FALSE, /* statically allocated */
  "2vector",
  &_gage2Vec,
  1, /* baseDim */
  2, /* valLen */
  GAGE_2VEC_ITEM_MAX,
  _gage2VecTable,
  _gage2VecIv3Print,
  _gage2VecFilter,
  _gage2VecAnswer,
  NULL, NULL, NULL, NULL,
  NULL
};
gageKind *const
gageKind2Vec = &_gageKind2Vec;
/* clang-format on */
