/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

#include "ten.h"
#include "privateTen.h"

/* -------------------------------------------------------------- */

char
_tenAnisoStr[TEN_ANISO_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown aniso)",
  "Cl1",
  "Cp1",
  "Ca1",
  "Cs1",
  "Ct1",
  "Cl2",
  "Cp2",
  "Ca2",
  "Cs2",
  "Ct2",
  "RA",
  "FA",
  "VF",
  "Q",
  "R",
  "S",
  "Th",
  "Cz",
  "Det",
  "Tr"
};

airEnum
_tenAniso = {
  "anisotropy metric",
  TEN_ANISO_MAX,
  _tenAnisoStr,  NULL,
  NULL, NULL,
  AIR_FALSE
};
airEnum *
tenAniso = &_tenAniso;

/* --------------------------------------------------------------------- */

char
_tenGageStr[][AIR_STRLEN_SMALL] = {
  "(unknown tenGage)",

  "tensor",
  "trace",
  "B",
  "det",
  "S",
  "Q",
  "FA",
  "R",

  "evals",
  "eval0",
  "eval1",
  "eval2",
  "evecs",
  "evec0",
  "evec1",
  "evec2",

  "tensor grad",

  "trace grad vec",
  "trace grad mag",
  "trace normal",

  "B grad vec",
  "B grad mag",
  "B normal",

  "det grad vec",
  "det grad mag",
  "det normal",

  "S grad vec",
  "S grad mag",
  "S normal",

  "Q grad vec",
  "Q grad mag",
  "Q normal",

  "FA grad vec",
  "FA grad mag",
  "FA normal",

  "R grad vec",
  "R grad mag",
  "R normal",

  "anisotropies"
};

char
_tenGageDesc[][AIR_STRLEN_MED] = {
  "(unknown tenGage item)",
  "tensor",
  "trace",
  "B",
  "determinant",
  "S",
  "Q",
  "FA",
  "R",
  "3 eigenvalues",
  "eigenvalue 0",
  "eigenvalue 1",
  "eigenvalue 2",
  "3 eigenvectors",
  "eigenvector 0",
  "eigenvector 1",
  "eigenvector 2",
  "tensor gradients",
  "trace grad vec",
  "trace grad mag",
  "trace normal",
  "B grad vec",
  "B grad mag",
  "B normal",
  "determinant grad vec",
  "determinant grad mag",
  "determinant normal",
  "S grad vec",
  "S grad mag",
  "S normal",
  "Q grad vec",
  "Q grad mag",
  "Q normal",
  "FA grad vec",
  "FA grad mag",
  "FA normal",
  "R grad vec",
  "R grad mag",
  "R normal",
  "anisotropies"
};

int
_tenGageVal[] = {
  tenGageUnknown,
  tenGageTensor,        /*  0: "t", the reconstructed tensor: GT[7] */
  tenGageTrace,         /*  1: "tr", trace of tensor: GT[1] */
  tenGageB,             /*  2: "b": GT[1] */
  tenGageDet,           /*  3: "det", determinant of tensor: GT[1] */
  tenGageS,             /*  4: "s", square of frobenius norm: GT[1] */
  tenGageQ,             /*  5: "q", (S - B)/9: GT[1] */
  tenGageFA,            /*  6: "fa", fractional anisotropy: GT[1] */
  tenGageR,             /*  7: "r", 9*A*B - 2*A^3 - 27*C: GT[1] */
  tenGageEval,          /*  8: "eval", all eigenvalues of tensor : GT[3] */
  tenGageEval0,         /*  9: "eval0", major eigenvalue of tensor : GT[1] */
  tenGageEval1,         /* 10: "eval1", medium eigenvalue of tensor : GT[1] */
  tenGageEval2,         /* 11: "eval2", minor eigenvalue of tensor : GT[1] */
  tenGageEvec,          /* 12: "evec", major eigenvectors of tensor: GT[9] */
  tenGageEvec0,         /* 13: "evec0", major eigenvectors of tensor: GT[3] */
  tenGageEvec1,         /* 14: "evec1", medium eigenvectors of tensor: GT[3] */
  tenGageEvec2,         /* 15: "evec2", minor eigenvectors of tensor: GT[3] */
  tenGageTensorGrad,    /* 16: "tg, all tensor component gradients: GT[21] */
  tenGageTraceGradVec,  /* 17: "trgv": gradient (vector) of trace: GT[3] */
  tenGageTraceGradMag,  /* 18: "trgm": gradient magnitude of trace: GT[1] */
  tenGageTraceNormal,   /* 19: "trn": normal of trace: GT[3] */
  tenGageBGradVec,      /* 20: "bgv", gradient (vector) of B: GT[3] */
  tenGageBGradMag,      /* 21: "bgm", gradient magnitude of B: GT[1] */
  tenGageBNormal,       /* 22: "bn", normal of B: GT[3] */
  tenGageDetGradVec,    /* 23: "detgv", gradient (vector) of Det: GT[3] */
  tenGageDetGradMag,    /* 24: "detgm", gradient magnitude of Det: GT[1] */
  tenGageDetNormal,     /* 25: "detn", normal of Det: GT[3] */
  tenGageSGradVec,      /* 26: "sgv", gradient (vector) of S: GT[3] */
  tenGageSGradMag,      /* 27: "sgm", gradient magnitude of S: GT[1] */
  tenGageSNormal,       /* 28: "sn", normal of S: GT[3] */
  tenGageQGradVec,      /* 29: "qgv", gradient vector of Q: GT[3] */
  tenGageQGradMag,      /* 30: "qgm", gradient magnitude of Q: GT[1] */
  tenGageQNormal,       /* 31: "qn", normalized gradient of Q: GT[3] */
  tenGageFAGradVec,     /* 32: "fagv", gradient vector of FA: GT[3] */
  tenGageFAGradMag,     /* 33: "fagm", gradient magnitude of FA: GT[1] */
  tenGageFANormal,      /* 34: "fan", normalized gradient of FA: GT[3] */
  tenGageRGradVec,      /* 35: "rgv", gradient vector of Q: GT[3] */
  tenGageRGradMag,      /* 36: "rgm", gradient magnitude of Q: GT[1] */
  tenGageRNormal,       /* 37: "rn", normalized gradient of Q: GT[3] */
  tenGageAniso          /* 38: "an", all anisotropies: GT[TEN_ANISO_MAX+1] */
};

char
_tenGageStrEqv[][AIR_STRLEN_SMALL] = {
  "t", "tensor",
  "tr", "trace",
  "b",
  "det",
  "s",
  "q",
  "fa",
  "r",
  "eval",
  "eval0",
  "eval1",
  "eval2",
  "evec",
  "evec0",
  "evec1",
  "evec2",
  "tg", "tensor grad",
  "trgv", "tracegv", "trace grad vec",
  "trgm", "tracegm", "trace grad mag",
  "trn", "tracen", "trace normal",
  "bgv", "b grad vec",
  "bgm", "b grad mag",
  "bn", "b normal",
  "detgv", "det grad vec",
  "detgm", "det grad mag",
  "detn", "det normal",
  "sgv", "s grad vec",
  "sgm", "s grad mag",
  "sn", "s normal",
  "qgv", "q grad vec",
  "qgm", "q grad mag",
  "qn", "q normal",
  "fagv", "fa grad vec",
  "fagm", "fa grad mag",
  "fan", "fa normal",
  "rgv", "r grad vec",
  "rgm", "r grad mag",
  "rn", "r normal",
  "an", "aniso", "anisotropies",
  ""
};

int
_tenGageValEqv[] = {
  tenGageTensor, tenGageTensor,
  tenGageTrace, tenGageTrace,
  tenGageB,
  tenGageDet,
  tenGageS,
  tenGageQ,
  tenGageFA,
  tenGageR,
  tenGageEval,
  tenGageEval0,
  tenGageEval1,
  tenGageEval2,
  tenGageEvec,
  tenGageEvec0,
  tenGageEvec1,
  tenGageEvec2,
  tenGageTensorGrad, tenGageTensorGrad,
  tenGageTraceGradVec, tenGageTraceGradVec, tenGageTraceGradVec,
  tenGageTraceGradMag, tenGageTraceGradMag, tenGageTraceGradMag,
  tenGageTraceNormal, tenGageTraceNormal, tenGageTraceNormal,
  tenGageBGradVec, tenGageBGradVec,
  tenGageBGradMag, tenGageBGradMag,
  tenGageBNormal, tenGageBNormal,
  tenGageDetGradVec, tenGageDetGradVec,
  tenGageDetGradMag, tenGageDetGradMag,
  tenGageDetNormal, tenGageDetNormal,
  tenGageSGradVec, tenGageSGradVec,
  tenGageSGradMag, tenGageSGradMag,
  tenGageSNormal, tenGageSNormal,
  tenGageQGradVec, tenGageQGradVec,
  tenGageQGradMag, tenGageQGradMag,
  tenGageQNormal, tenGageQNormal,
  tenGageFAGradVec, tenGageFAGradVec,
  tenGageFAGradMag, tenGageFAGradMag,
  tenGageFANormal, tenGageFANormal,
  tenGageRGradVec, tenGageRGradVec,
  tenGageRGradMag, tenGageRGradMag,
  tenGageRNormal, tenGageRNormal,
  tenGageAniso, tenGageAniso, tenGageAniso
};

airEnum
_tenGage = {
  "tenGage",
  TEN_GAGE_ITEM_MAX+1,
  _tenGageStr, _tenGageVal,
  _tenGageDesc,
  _tenGageStrEqv, _tenGageValEqv,
  AIR_FALSE
};
airEnum *
tenGage = &_tenGage;

/* --------------------------------------------------------------------- */

char
_tenFiberTypeStr[][AIR_STRLEN_SMALL] = {
  "(unknown tenFiberType)",
  "evec1",
  "tensorline",
  "pureline",
  "zhukov"
};

char
_tenFiberTypeDesc[][AIR_STRLEN_MED] = {
  "unknown tenFiber type",
  "simply follow principal eigenvector",
  "Weinstein-Kindlmann tensorlines",
  "based on tensor multiplication only",
  "Zhukov's oriented tensors"
};

char
_tenFiberTypeStrEqv[][AIR_STRLEN_SMALL] = {
  "ev1", "evec1",
  "tline", "tensorline",
  "pline", "pureline",
  "z", "zhukov",
  ""
};

int
_tenFiberTypeValEqv[] = {
  tenFiberTypeEvec1, tenFiberTypeEvec1,
  tenFiberTypeTensorLine, tenFiberTypeTensorLine,
  tenFiberTypePureLine, tenFiberTypePureLine,
  tenFiberTypeZhukov, tenFiberTypeZhukov
};

airEnum
_tenFiberType = {
  "tenFiberType",
  TEN_FIBER_TYPE_MAX,
  _tenFiberTypeStr, NULL,
  _tenFiberTypeDesc,
  _tenFiberTypeStrEqv, _tenFiberTypeValEqv,
  AIR_FALSE
};
airEnum *
tenFiberType = &_tenFiberType;

/* ----------------------------------------------------------------------- */

char
_tenFiberStopStr[][AIR_STRLEN_SMALL] = {
  "(unknown tenFiberStop)",
  "aniso",
  "length",
  "steps",
  "confidence",
  "bounds"
};

char
_tenFiberStopStrEqv[][AIR_STRLEN_SMALL] = {
  "aniso",
  "length", "len",
  "steps",
  "confidence", "conf",
  "bounds",
  ""
};

int
_tenFiberStopValEqv[] = {
  tenFiberStopAniso,
  tenFiberStopLength, tenFiberStopLength,
  tenFiberStopNumSteps,
  tenFiberStopConfidence, tenFiberStopConfidence,
  tenFiberStopBounds
};

char
_tenFiberStopDesc[][AIR_STRLEN_MED] = {
  "unknown tenFiber stop",
  "anisotropy went below threshold",
  "fiber length exceeded normalcy bounds",
  "number of steps along fiber too many",
  "tensor \"confidence\" value too low",
  "fiber went outside bounding box"
};

airEnum
_tenFiberStop = {
  "fiber stopping criteria",
  TEN_FIBER_STOP_MAX,
  _tenFiberStopStr, NULL,
  _tenFiberStopDesc,
  _tenFiberStopStrEqv, _tenFiberStopValEqv, 
  AIR_FALSE
};
airEnum *
tenFiberStop = &_tenFiberStop;

/* ----------------------------------------------------------------------- */

char
_tenGlyphTypeStr[][AIR_STRLEN_SMALL] = {
  "(unknown tenGlyphType)",
  "box",
  "sphere",
  "cylinder",
  "superquad"
};

#define BOX tenGlyphTypeBox
#define SPH tenGlyphTypeSphere
#define CYL tenGlyphTypeCylinder
#define SQD tenGlyphTypeSuperquad

char
_tenGlyphTypeStrEqv[][AIR_STRLEN_SMALL] = {
  "b", "box",
  "s", "sph", "sphere",
  "c", "cyl", "cylind", "cylinder",
  "q", "superq", "sqd", "superquad", "superquadric",
  ""
};

int
_tenGlyphTypeValEqv[] = {
  BOX, BOX,
  SPH, SPH, SPH,
  CYL, CYL, CYL, CYL,
  SQD, SQD, SQD, SQD, SQD
};

char
_tenGlyphTypeDesc[][AIR_STRLEN_MED] = {
  "unknown tenGlyph type",
  "box/cube (rectangular prisms)",
  "sphere (ellipsoids)",
  "cylinders aligned along major eigenvector",
  "superquadric (superellipsoids)"
};

airEnum
_tenGlyphType = {
  "tenGlyphType",
  TEN_GLYPH_TYPE_MAX,
  _tenGlyphTypeStr, NULL,
  _tenGlyphTypeDesc,
  _tenGlyphTypeStrEqv, _tenGlyphTypeValEqv, 
  AIR_FALSE
};
airEnum *
tenGlyphType = &_tenGlyphType;
