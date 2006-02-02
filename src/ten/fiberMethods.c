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

tenFiberContext *
tenFiberContextNew(const Nrrd *dtvol) {
  char me[]="tenFiberContextNew", err[BIFF_STRLEN];
  tenFiberContext *tfx;

  if (tenTensorCheck(dtvol, nrrdTypeUnknown, AIR_TRUE, AIR_TRUE)) {
    sprintf(err, "%s: didn't get a tensor volume", me);
    biffAdd(TEN, err); return NULL;
  }
  if (!( tfx = (tenFiberContext *)calloc(1, sizeof(tenFiberContext)) )) {
    /* that is not good */
    return NULL;
  }
  if ( !(tfx->gtx = gageContextNew())
       || !(tfx->pvl = gagePerVolumeNew(tfx->gtx, dtvol, tenGageKind)) 
       || (gagePerVolumeAttach(tfx->gtx, tfx->pvl)) ) {
    sprintf(err, "%s: gage trouble", me);
    biffMove(TEN, err, GAGE); free(tfx); return NULL;
  }
  tfx->dtvol = dtvol;
  tfx->ksp = nrrdKernelSpecNew();
  if (nrrdKernelSpecParse(tfx->ksp, tenDefFiberKernel)) {
    sprintf(err, "%s: couldn't parse tenDefFiberKernel \"%s\"",
            me,  tenDefFiberKernel);
    biffMove(TEN, err, NRRD); return NULL;
  }
  if (tenFiberKernelSet(tfx, tfx->ksp->kernel, tfx->ksp->parm)) {
    sprintf(err, "%s: couldn't set default kernel", me);
    biffAdd(TEN, err); return NULL;
  }
  /* looks to GK like GK says that we must set fiber type and 
     some stop criterion */
  tfx->fiberType = tenFiberTypeUnknown;
  tfx->intg = tenDefFiberIntg;
  tfx->anisoStopType = tenDefFiberAnisoStopType;
  tfx->anisoSpeedType = tenAnisoUnknown;
  tfx->stop = 0;
  tfx->anisoThresh = tenDefFiberAnisoThresh;
  /* so I'm not using the normal default mechanism, shoot me */
  tfx->anisoSpeedFunc[0] = 0;
  tfx->anisoSpeedFunc[1] = 0;
  tfx->anisoSpeedFunc[2] = 0;
  tfx->maxNumSteps = tenDefFiberMaxNumSteps;
  tfx->useIndexSpace = tenDefFiberUseIndexSpace;
  tfx->stepSize = tenDefFiberStepSize;
  tfx->maxHalfLen = tenDefFiberMaxHalfLen;
  tfx->confThresh = 0.5; /* why do I even bother setting these- they'll
                            only get read if the right tenFiberStopSet has
                            been called, in which case they'll be set... */
  tfx->minRadius = 1;    /* above lament applies here as well */
  tfx->wPunct = tenDefFiberWPunct;

  GAGE_QUERY_RESET(tfx->query);
  tfx->dten = gageAnswerPointer(tfx->gtx, tfx->pvl, tenGageTensor);
  tfx->eval = gageAnswerPointer(tfx->gtx, tfx->pvl, tenGageEval0);
  tfx->evec = gageAnswerPointer(tfx->gtx, tfx->pvl, tenGageEvec0);
  tfx->anisoStop = NULL;
  tfx->anisoSpeed = NULL;
  tfx->radius = AIR_NAN;
  /* no more; set below 
  tfx->aniso = gageAnswerPointer(tfx->gtx, tfx->pvl, tenGageAniso); */
  return tfx;
}

int
tenFiberTypeSet(tenFiberContext *tfx, int type) {
  char me[]="tenFiberTypeSet", err[BIFF_STRLEN];
  int qse;

  if (!tfx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  qse = 0;
  switch(type) {
  case tenFiberTypeEvec1:
    GAGE_QUERY_ITEM_ON(tfx->query, tenGageEvec0);
    break;
  case tenFiberTypeTensorLine:
    GAGE_QUERY_ITEM_ON(tfx->query, tenGageTensor);
    GAGE_QUERY_ITEM_ON(tfx->query, tenGageEval0);
    GAGE_QUERY_ITEM_ON(tfx->query, tenGageEval1);
    GAGE_QUERY_ITEM_ON(tfx->query, tenGageEval2);
    GAGE_QUERY_ITEM_ON(tfx->query, tenGageEvec0);
    GAGE_QUERY_ITEM_ON(tfx->query, tenGageEvec1);
    GAGE_QUERY_ITEM_ON(tfx->query, tenGageEvec2);
    break;
  case tenFiberTypePureLine:
    GAGE_QUERY_ITEM_ON(tfx->query, tenGageTensor);
    break;
  case tenFiberTypeZhukov:
    sprintf(err, "%s: sorry, Zhukov oriented tensors not implemented", me);
    biffAdd(TEN, err); return 1;
    break;
  default:
    sprintf(err, "%s: fiber type %d not recognized", me, type);
    biffAdd(TEN, err); return 1;
    break;
  }
  if (qse) {
    sprintf(err, "%s: problem setting query", me);
    biffMove(TEN, err, GAGE); return 1;
  }
  tfx->fiberType = type;
  return 0;
}

/*
******** tenFiberStopSet
**
** how to set stop criteria and their parameters.  a little tricky because
** of the use of varargs
**
** valid calls:
** tenFiberStopSet(tfx, tenFiberStopLength, double maxHalfLen)
** tenFiberStopSet(tfx, tenFiberStopAniso, int anisoType, double anisoThresh)
** tenFiberStopSet(tfx, tenFiberStopNumSteps, int numSteps)
** tenFiberStopSet(tfx, tenFiberStopConfidence, double conf)
** tenFiberStopSet(tfx, tenFiberStopRadius, double radius)
** tenFiberStopSet(tfx, tenFiberStopBounds)
** tenFiberStopSet(tfx, tenFiberStopStub)
*/
int
tenFiberStopSet(tenFiberContext *tfx, int stop, ...) {
  char me[]="tenFiberStopSet", err[BIFF_STRLEN];
  va_list ap;
  int ret=0;
  int anisoGage;

  if (!tfx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  va_start(ap, stop);
  switch(stop) {
  case tenFiberStopAniso:
    tfx->anisoStopType = va_arg(ap, int);
    tfx->anisoThresh = va_arg(ap, double);
    if (!(AIR_IN_OP(tenAnisoUnknown, tfx->anisoStopType, tenAnisoLast))) {
      sprintf(err, "%s: given aniso stop type %d not valid", me,
              tfx->anisoStopType);
      biffAdd(TEN, err); ret = 1; goto end;
    }
    if (!(AIR_EXISTS(tfx->anisoThresh))) {
      sprintf(err, "%s: given aniso threshold doesn't exist", me);
      biffAdd(TEN, err); ret = 1; goto end;
    }
    switch(tfx->anisoStopType) {
    case tenAniso_FA:
      anisoGage = tenGageFA;
      break;
    case tenAniso_Cl1:
      anisoGage = tenGageCl1;
      break;
    case tenAniso_Cp1:
      anisoGage = tenGageCp1;
      break;
    case tenAniso_Ca1:
      anisoGage = tenGageCa1;
      break;
    case tenAniso_Cl2:
      anisoGage = tenGageCl2;
      break;
    case tenAniso_Cp2:
      anisoGage = tenGageCp2;
      break;
    case tenAniso_Ca2:
      anisoGage = tenGageCa2;
      break;
    default:
      sprintf(err, "%s: sorry, currently don't have fast %s computation "
              "via gage", me, airEnumStr(tenAniso, tfx->anisoStopType));
      biffAdd(TEN, err); ret = 1; goto end;
      break;
    }
    /* NOTE: we are no longer computing ALL anisotropy measures ...
       GAGE_QUERY_ITEM_ON(tfx->query, tenGageAniso); 
    */
    GAGE_QUERY_ITEM_ON(tfx->query, anisoGage);
    tfx->anisoStop = gageAnswerPointer(tfx->gtx, tfx->pvl, anisoGage);
    break;
  case tenFiberStopLength:
    tfx->maxHalfLen = va_arg(ap, double);
    if (!( AIR_EXISTS(tfx->maxHalfLen) && tfx->maxHalfLen > 0.0 )) {
      sprintf(err, "%s: given maxHalfLen doesn't exist or isn't > 0.0", me);
      biffAdd(TEN, err); ret = 1; goto end;
    }
    /* no query modifications needed */
    break;
  case tenFiberStopNumSteps:
    tfx->maxNumSteps = va_arg(ap, int);
    if (!( tfx->maxNumSteps > 0 )) {
      sprintf(err, "%s: given maxNumSteps isn't > 0.0", me);
      biffAdd(TEN, err); ret = 1; goto end;
    }
    /* no query modifications needed */
    break;
  case tenFiberStopConfidence:
    tfx->confThresh = va_arg(ap, double);
    if (!( AIR_EXISTS(tfx->confThresh) )) {
      sprintf(err, "%s: given confThresh doesn't exist", me);
      biffAdd(TEN, err); ret = 1; goto end;
    }
    GAGE_QUERY_ITEM_ON(tfx->query, tenGageTensor);
    break;
  case tenFiberStopRadius:
    tfx->minRadius = va_arg(ap, double);
    if (!( AIR_EXISTS(tfx->minRadius) )) {
      sprintf(err, "%s: given minimum radius doesn't exist", me);
      biffAdd(TEN, err); ret = 1; goto end;
    }
    /* no query modifications needed */
    break;
  case tenFiberStopBounds:
    /* nothing to set; always used as a stop criterion */
    break;
  case tenFiberStopStub:
    /* no var-args to grab */
    /* no query modifications needed */
    break;
  default:
    sprintf(err, "%s: stop criterion %d not recognized", me, stop);
    biffAdd(TEN, err); ret = 1; goto end;
  }
  tfx->stop = tfx->stop | (1 << stop);

 end:
  va_end(ap);
  return ret;
}

void
tenFiberStopOn(tenFiberContext *tfx, int stop) {

  if (tfx && !airEnumValCheck(tenFiberStop, stop)) {
    tfx->stop = tfx->stop | (1 << stop);
  }
  return;
}

void
tenFiberStopOff(tenFiberContext *tfx, int stop) {

  if (tfx && !airEnumValCheck(tenFiberStop, stop)) {
    tfx->stop = tfx->stop & ~(1 << stop);
  }
  return;
}

void
tenFiberStopReset(tenFiberContext *tfx) {

  if (tfx) {
    tfx->stop = 0;
  }
  return;
}

int
tenFiberAnisoSpeedSet(tenFiberContext *tfx, int aniso,
                      double lerp, double thresh, double soft) {
  char me[]="tenFiberAnisoSpeedSet", err[BIFF_STRLEN];
  int anisoGage;

  if (!tfx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (airEnumValCheck(tenAniso, aniso)) {
    sprintf(err, "%s: aniso %d not valid", me, aniso);
    biffAdd(TEN, err); return 1;
  }
  switch(aniso) {
  case tenAniso_FA:
    anisoGage = tenGageFA;
    break;
  case tenAniso_Cl1:
    anisoGage = tenGageCl1;
    break;
  case tenAniso_Cp1:
    anisoGage = tenGageCp1;
    break;
  case tenAniso_Ca1:
    anisoGage = tenGageCa1;
    break;
  case tenAniso_Cl2:
    anisoGage = tenGageCl2;
    break;
  case tenAniso_Cp2:
    anisoGage = tenGageCp2;
    break;
  case tenAniso_Ca2:
    anisoGage = tenGageCa2;
    break;
  default:
    sprintf(err, "%s: sorry, currently don't have fast %s computation "
            "via gage", me, airEnumStr(tenAniso, tfx->anisoStopType));
    biffAdd(TEN, err); return 1;
    break;
  }
  tfx->anisoSpeedType = aniso;
  GAGE_QUERY_ITEM_ON(tfx->query, anisoGage);
  tfx->anisoSpeed = gageAnswerPointer(tfx->gtx, tfx->pvl, anisoGage);
  tfx->anisoSpeedFunc[0] = lerp;
  tfx->anisoSpeedFunc[1] = thresh;
  tfx->anisoSpeedFunc[2] = soft;

  return 0;
}

int
tenFiberAnisoSpeedReset(tenFiberContext *tfx) {
  char me[]="tenFiberAnisoSpeedReset", err[BIFF_STRLEN];

  if (!tfx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  tfx->anisoSpeedType = tenAnisoUnknown;
  /* HEY: GAGE_QUERY_ITEM_OFF something? */
  tfx->anisoSpeed = NULL;
  return 0;
}

int
tenFiberKernelSet(tenFiberContext *tfx,
                  const NrrdKernel *kern,
                  const double parm[NRRD_KERNEL_PARMS_NUM]) {
  char me[]="tenFiberKernelSet", err[BIFF_STRLEN];

  if (!(tfx && kern)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  nrrdKernelSpecSet(tfx->ksp, kern, parm);
  if (gageKernelSet(tfx->gtx, gageKernel00,
                    tfx->ksp->kernel, tfx->ksp->parm)) {
    sprintf(err, "%s: problem setting kernel", me);
    biffMove(TEN, err, GAGE); return 1;
  }
  
  return 0;
}

int
tenFiberIntgSet(tenFiberContext *tfx, int intg) {
  char me[]="tenFiberIntTypeSet", err[BIFF_STRLEN];

  if (!(tfx)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( AIR_IN_OP(tenFiberIntgUnknown, intg, tenFiberIntgLast) )) {
    sprintf(err, "%s: got invalid integration type %d", me, intg);
    biffAdd(TEN, err); return 1;
  }
  tfx->intg = intg;
  
  return 0;
}

int
tenFiberParmSet(tenFiberContext *tfx, int parm, double val) {
  char me[]="tenFiberParmSet";

  if (tfx) {
    switch(parm) {
    case tenFiberParmStepSize:
      tfx->stepSize = val;
      break;
    case tenFiberParmUseIndexSpace:
      tfx->useIndexSpace = !!val;
      break;
    case tenFiberParmWPunct:
      tfx->wPunct = val;
      break;
    default:
      fprintf(stderr, "%s: WARNING!!! tenFiberParm %d not handled\n",
              me, parm);
      break;
    }
  }
  return 0;
}

int
tenFiberUpdate(tenFiberContext *tfx) {
  char me[]="tenFiberUpdate", err[BIFF_STRLEN];

  if (!tfx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (tenFiberTypeUnknown == tfx->fiberType) {
    sprintf(err, "%s: fiber type not set", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( AIR_IN_OP(tenFiberTypeUnknown, tfx->fiberType, tenFiberTypeLast) )) {
    sprintf(err, "%s: tfx->fiberType set to bogus value (%d)", me,
            tfx->fiberType);
    biffAdd(TEN, err); return 1;
  }
  if (tenFiberIntgUnknown == tfx->intg) {
    sprintf(err, "%s: integration type not set", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( AIR_IN_OP(tenFiberIntgUnknown, tfx->intg, tenFiberIntgLast) )) {
    sprintf(err, "%s: tfx->intg set to bogus value (%d)", me, tfx->intg);
    biffAdd(TEN, err); return 1;
  }
  if (0 == tfx->stop) {
    sprintf(err, "%s: no fiber stopping criteria set", me);
    biffAdd(TEN, err); return 1;
  }
  if (gageQuerySet(tfx->gtx, tfx->pvl, tfx->query)
      || gageUpdate(tfx->gtx)) {
    sprintf(err, "%s: trouble with gage", me);
    biffMove(TEN, err, GAGE); return 1;
  }
  return 0;
}

/*
** exact same precautions about utility of this as with gageContextCopy!!!
** So: only after tenFiberUpdate, and don't touch anything, and don't
** call anything except tenFiberTrace and tenFiberContextNix
*/
tenFiberContext *
tenFiberContextCopy(tenFiberContext *oldTfx) {
  tenFiberContext *tfx;

  tfx = (tenFiberContext *)calloc(1, sizeof(tenFiberContext));
  memcpy(tfx, oldTfx, sizeof(tenFiberContext));
  tfx->ksp = nrrdKernelSpecCopy(oldTfx->ksp);
  tfx->gtx = gageContextCopy(oldTfx->gtx);
  tfx->pvl = tfx->gtx->pvl[0];  /* HEY! gage API sucks */
  tfx->dten = gageAnswerPointer(tfx->gtx, tfx->pvl, tenGageTensor);
  tfx->eval = gageAnswerPointer(tfx->gtx, tfx->pvl, tenGageEval0);
  tfx->evec = gageAnswerPointer(tfx->gtx, tfx->pvl, tenGageEvec0);
  tfx->anisoStop = gageAnswerPointer(tfx->gtx, tfx->pvl, tfx->anisoStopType);
  tfx->anisoSpeed = (tfx->anisoSpeedType
                     ? gageAnswerPointer(tfx->gtx, tfx->pvl,
                                         tfx->anisoSpeedType)
                     : NULL);
  return tfx;
}

tenFiberContext *
tenFiberContextNix(tenFiberContext *tfx) {
  
  if (tfx) {
    tfx->ksp = nrrdKernelSpecNix(tfx->ksp);
    tfx->gtx = gageContextNix(tfx->gtx);
    free(tfx);
  }
  return NULL;
}
