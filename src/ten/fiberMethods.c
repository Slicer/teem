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

/*
** the volume fits inside 
** [-vhlen[0],vhlen[0]] x [-vhlen[1],vhlen[1]] x [-vhlen[2],vhlen[2]]
*/
/*
void
_tenLearnLengths(double vhlen[3], int center, Nrrd *dtvol) {
  double maxLen;
  int numSamples[3], numElements[3];

  numSamples[0] = dtvol->axis[1].size;
  numSamples[1] = dtvol->axis[2].size;
  numSamples[2] = dtvol->axis[3].size;
  if (nrrdCenterCell == center) {
    numElements[0] = numSamples[0];
    numElements[1] = numSamples[1];
    numElements[2] = numSamples[2];
  } else {
    numElements[0] = numSamples[0]-1;
    numElements[1] = numSamples[1]-1;
    numElements[2] = numSamples[2]-1;
  }
  vhlen[0] = numElements[0]*dtvol->axis[1].spacing;
  vhlen[1] = numElements[1]*dtvol->axis[1].spacing;
  vhlen[2] = numElements[2]*dtvol->axis[1].spacing;
  maxLen = AIR_MAX(vhlen[0], vhlen[1]);
  maxLen = AIR_MAX(vhlen[2], maxLen);
  vhlen[0] /= maxLen;
  vhlen[1] /= maxLen;
  vhlen[2] /= maxLen;
}
*/

tenFiberContext *
tenFiberContextNew(Nrrd *dtvol) {
  char me[]="tenFiberContextNew", err[AIR_STRLEN_MED];
  tenFiberContext *tfx;
  gagePerVolume *pvl;
  NrrdKernel *kernel;
  double kparm[NRRD_KERNEL_PARMS_NUM];
  
  tfx = calloc(1, sizeof(tenFiberContext));
  if (tfx) {
    if (tenTensorCheck(dtvol, nrrdTypeUnknown, AIR_TRUE)) {
      sprintf(err, "%s: didn't get a tensor volume", me);
      biffAdd(TEN, err); return NULL;
    }
    tfx->dtvol = dtvol;
    tfx->ksp = nrrdKernelSpecNew();
    tfx->gtx = gageContextNew();
    if ( !(pvl = gagePerVolumeNew(dtvol, tenGageKind)) 
	 || (gagePerVolumeAttach(tfx->gtx, pvl)) ) {
      sprintf(err, "%s: gage trouble", me);
      biffMove(TEN, err, GAGE); return NULL;
    }
    if (nrrdKernelParse(&kernel, kparm, tenDefFiberKernel)) {
      sprintf(err, "%s: couldn't parse tenDefFiberKernel \"%s\"",
	      me,  tenDefFiberKernel);
      biffMove(TEN, err, NRRD); return NULL;
    }
    if (tenFiberKernelSet(tfx, kernel, kparm)) {
      sprintf(err, "%s: couldn't set default kernel", me);
      biffAdd(TEN, err); return NULL;
    }
    tfx->fiberType = tenFiberTypeUnknown;
    tfx->anisoType = tenDefFiberAnisoType;
    tfx->anisoThresh = tenDefFiberAnisoThresh;
    tfx->stepSize = tenDefFiberStepSize;
    tfx->maxHalfLen = tenDefFiberMaxHalfLen;
    tfx->stop = 0;

    tfx->query = 0;
    tfx->dten = gageAnswerPointer(tfx->gtx->pvl[0], tenGageTensor);
    tfx->evec = gageAnswerPointer(tfx->gtx->pvl[0], tenGageEvec);
    tfx->aniso = gageAnswerPointer(tfx->gtx->pvl[0], tenGageAniso);
  }
  return tfx;
}

int
tenFiberTypeSet(tenFiberContext *tfx, int type) {
  char me[]="tenFiberTypeSet", err[AIR_STRLEN_MED];
  gagePerVolume *pvl;
  int qse;

  if (!tfx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  pvl = tfx->gtx->pvl[0];
  qse = 0;
  switch(type) {
  case tenFiberTypeEvec1:
    tfx->query |= (1 << tenGageEvec);
    break;
  case tenFiberTypeTensorLine:
    tfx->query |= ((1 << tenGageTensor)
		   | (1 << tenGageEvec));
    break;
  case tenFiberTypePureLine:
    tfx->query |= (1 << tenGageTensor);
    break;
  case tenFiberTypeZhukov:
    sprintf(err, "%s: sorry, not Zhukov oriented tensors implemented", me);
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
** tenFiberStopSet(tfx, tenFiberStopLen, double maxHalfLen)
** tenFiberStopSet(tfx, tenFiberStopAniso, int anisoType, double anisoThresh)
*/
int
tenFiberStopSet(tenFiberContext *tfx, int stop, ...) {
  char me[]="tenFiberStopSet", err[AIR_STRLEN_MED];
  va_list ap;
  int ret=0;

  if (!tfx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  va_start(ap, stop);
  switch(stop) {
  case tenFiberStopLen:
    tfx->maxHalfLen = va_arg(ap, double);
    if (!( AIR_EXISTS(tfx->maxHalfLen) && tfx->maxHalfLen > 0.0 )) {
      sprintf(err, "%s: given maxHalfLen doesn't exist or isn't > 0.0", me);
      ret = 1; goto end;
    }
    /* no query modifications needed */
    break;
  case tenFiberStopBounds:
    /* nothing to set; always used as a stop criterion */
    break;
  case tenFiberStopAniso:
    tfx->anisoType = va_arg(ap, int);
    tfx->anisoThresh = va_arg(ap, double);
    if (!(AIR_IN_OP(tenAnisoUnknown, tfx->anisoType, tenAnisoLast))) {
      sprintf(err, "%s: given aniso type %d not valid", me, tfx->anisoType);
      ret = 1; goto end;
    }
    if (!(AIR_EXISTS(tfx->anisoThresh))) {
      sprintf(err, "%s: given aniso threshold doesn't exist", me);
      ret = 1;
      goto end;
    }
    tfx->query |= (1 << tenGageAniso);
    break;
  default:
    sprintf(err, "%s: stop criterion %d not recognized", me, stop);
    ret = 1; goto end;
  }
  tfx->stop |= (1 << stop);

 end:
  va_end(ap);
  return ret;
}

int
tenFiberKernelSet(tenFiberContext *tfx,
		  NrrdKernel *kern,
		  double parm[NRRD_KERNEL_PARMS_NUM]) {
  char me[]="tenFiberKernelSet", err[AIR_STRLEN_MED];

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

void
tenFiberParmSet(tenFiberContext *tfx, int parm, double val) {

  if (tfx) {
    switch(parm) {
    case tenFiberParmStepSize:
      tfx->stepSize = val;
      break;
    default:
      /* morons */
      break;
    }
  }
  return;
}

int
tenFiberUpdate(tenFiberContext *tfx) {
  char me[]="tenFiberUpdate", err[AIR_STRLEN_MED];

  if (!tfx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (tenFiberTypeUnknown == tfx->fiberType) {
    sprintf(err, "%s: fiber type not set", me);
    biffAdd(TEN, err); return 1;
  }
  if (0 == tfx->stop) {
    sprintf(err, "%s: no fiber stopping criteria set", me);
    biffAdd(TEN, err); return 1;
  }
  if (gageQuerySet(tfx->gtx->pvl[0], tfx->query)
      || gageUpdate(tfx->gtx)) {
    sprintf(err, "%s: trouble with gage", me);
    biffMove(TEN, err, GAGE); return 1;
  }
  return 0;
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
