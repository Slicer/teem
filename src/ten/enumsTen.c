/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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
#include "tenPrivate.h"

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

/* -------------------------------------------------------------- */

char
_tenGageStr[][AIR_STRLEN_SMALL] = {
  "(unknown tenGage)",
  "tensor",
  "trace",
  "frob",
  "eigenvalues",
  "eigenvectors",
  "tensor gradient",
  "rr anisotropy",
  "rr gradient vector",
  "rr gradient magnitude",
  "normalized rr gradient",
  "multigrad",
  "fro(multigrad)",
  "multigrad eigenvalues",
  "multigrad eigenvectors",
  "anisotropies"
};

char
_tenGageDesc[][AIR_STRLEN_MED] = {
  "unknown tenGage query",
  "reconstructed tensor",
  "tensor trace",
  "frob(tensor)",
  "tensor eigenvalues",
  "tensor eigenvectors",
  "tensor gradient",
  "rr anisotropy",
  "rr gradient vector",
  "rr gradient magnitude",
  "normalized rr gradient",
  "multigrad",
  "frob(multigrad)",
  "multigrad eigenvalues",
  "multigrad eigenvectors",
  "anisotropies"
};

int
_tenGageVal[] = {
  tenGageUnknown,
  tenGageTensor,
  tenGageTrace,
  tenGageFrobTensor,
  tenGageEval,
  tenGageEvec,
  tenGageTensorGrad,
  tenGageRR,
  tenGageRRGradVec,
  tenGageRRGradMag,
  tenGageRRNormal,
  tenGageMultiGrad,
  tenGageFrobMG,
  tenGageMGEval,
  tenGageMGEvec,
  tenGageAniso
};

#define TG_T    tenGageTensor
#define TG_TR   tenGageTrace
#define TG_FT   tenGageFrobTensor
#define TG_AL   tenGageEval
#define TG_EC   tenGageEvec
#define TG_TG   tenGageTensorGrad
#define TG_RR   tenGageRR
#define TG_RGV  tenGageRRGradVec
#define TG_RGM  tenGageRRGradMag
#define TG_RGN  tenGageRRNormal
#define TG_MG   tenGageMultiGrad
#define TG_FMG  tenGageFrobMG
#define TG_MGA  tenGageMGEval
#define TG_MGE  tenGageMGEvec
#define TG_AN   tenGageAniso

char
_tenGageStrEqv[][AIR_STRLEN_SMALL] = {
  "t", "ten", "tensor",
  "tr", "trace",
  "frt", "fro", "frob", "frobt",
  "eval", "eigenvalues",
  "evec", "eigenvectors",
  "tg", "tgrad", "t grad", "tensor gradient",
  "rr",
  "rrv", "rrgrad", "rr grad", "rr gradient vector",
  "rrg", "rrgmag", "rr gmag", "rr gradient magnitude",
  "rrn", "rrnorm", "rr norm", "normalized rr gradient",
  "mg", "multigrad",
  "frmg", "frobmg", "frob mg",
  "mgeval", "mg eval", "multigrad eigenvalues",
  "mgevec", "mg evec", "multigrad eigenvectors",
  "an", "aniso", "anisotropies",
  ""
};

int
_tenGageValEqv[] = {
  TG_T, TG_T, TG_T,
  TG_TR, TG_TR,
  TG_FT, TG_FT, TG_FT, TG_FT,
  TG_AL, TG_AL,
  TG_EC, TG_EC,
  TG_TG, TG_TG, TG_TG, TG_TG,
  TG_RR,
  TG_RGV, TG_RGV, TG_RGV, TG_RGV,
  TG_RGM, TG_RGM, TG_RGM, TG_RGM,
  TG_RGN, TG_RGN, TG_RGN, TG_RGN,
  TG_MG, TG_MG,
  TG_FMG, TG_FMG, TG_FMG,
  TG_MGA, TG_MGA, TG_MGA,
  TG_MGE, TG_MGE, TG_MGE,
  TG_AN, TG_AN, TG_AN
};

airEnum
_tenGage = {
  "tenGage",
  TEN_GAGE_MAX+1,
  _tenGageStr, _tenGageVal,
  _tenGageDesc,
  _tenGageStrEqv, _tenGageValEqv,
  AIR_FALSE
};
airEnum *
tenGage = &_tenGage;

