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

#include "gage.h"
#include "private.h"

gageKind
_gageKindScl = {
  "scalar",
  &_gageScl,
  0,
  1,
  GAGE_SCL_MAX,
  gageSclAnsLength,
  gageSclAnsOffset,
  GAGE_SCL_TOTAL_ANS_LENGTH,
  _gageSclNeedDeriv,
  _gageSclPrereq,
  _gageSclPrint_query,
  (void *(*)(void))_gageSclAnswerNew,
  (void *(*)(void*))_gageSclAnswerNix,
  _gageSclIv3Fill,
  _gageSclIv3Print,
  _gageSclFilter,
  _gageSclAnswer
};
gageKind *
gageKindScl = &_gageKindScl;

gageKind
_gageKindVec = {
  "vector",
  &_gageVec,
  1,
  3,
  GAGE_VEC_MAX,
  gageVecAnsLength,
  gageVecAnsOffset,
  GAGE_VEC_TOTAL_ANS_LENGTH,
  _gageVecNeedDeriv,
  _gageVecPrereq,
  _gageVecPrint_query,
  (void *(*)(void))_gageVecAnswerNew,
  (void *(*)(void*))_gageVecAnswerNix,
  _gageVecIv3Fill,
  _gageVecIv3Print,
  _gageVecFilter,
  _gageVecAnswer
};
gageKind *
gageKindVec = &_gageKindVec;

