/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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


#include "limn.h"

char
_limnSplineTypeStr[LIMN_SPLINE_TYPE_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_spline_type)",
  "linear",
  "timewarp",
  "hermite",
  "cubic-bezier",
  "BC"
};

char
_limnSplineTypeDesc[LIMN_SPLINE_TYPE_MAX+1][AIR_STRLEN_MED] = {
  "unknown spline type",
  "simple linear interpolation between control points",
  "pseudo-Hermite spline for warping time to uniform (integral) "
    "control point locations",
  "Hermite cubic interpolating spline",
  "cubic Bezier spline",
  "Mitchell-Netravalli BC-family of cubic splines"
};

char
_limnSplineTypeStrEqv[][AIR_STRLEN_SMALL] = {
  "linear",
  "timewarp", "time-warp", "warp",
  "hermite",
  "cubicbezier", "cubic-bezier", "bezier", "bez",
  "BC", "BC-spline",
  ""
};

int
_limnSplineTypeValEqv[] = {
  limnSplineTypeLinear,
  limnSplineTypeTimeWarp, limnSplineTypeTimeWarp, limnSplineTypeTimeWarp,
  limnSplineTypeHermite,
  limnSplineTypeCubicBezier, limnSplineTypeCubicBezier, 
      limnSplineTypeCubicBezier, limnSplineTypeCubicBezier,
  limnSplineTypeBC, limnSplineTypeBC
};

airEnum
_limnSplineType = {
  "spline-type",
  LIMN_SPLINE_TYPE_MAX,
  _limnSplineTypeStr,  NULL,
  _limnSplineTypeDesc,
  _limnSplineTypeStrEqv, _limnSplineTypeValEqv,
  AIR_FALSE
};
airEnum *
limnSplineType = &_limnSplineType;

char
_limnSplineInfoStr[LIMN_SPLINE_INFO_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_spline_info)",
  "scalar",
  "2vector",
  "3vector",
  "4vector",
  "quaternion"
};

char
_limnSplineInfoDesc[LIMN_SPLINE_INFO_MAX+1][AIR_STRLEN_MED] = {
  "unknown spline info",
  "scalar",
  "2-vector",
  "3-vector",
  "4-vector, interpolated in R^4",
  "quaternion, interpolated in S^3"
};

char
_limnSplineInfoStrEqv[][AIR_STRLEN_SMALL] = {
  "scalar", "scale", "s", "t",
  "2-vector", "2vector", "2vec", "2v", "v2", "vec2", "vector2", "vector-2",
  "3-vector", "3vector", "3vec", "3v", "v3", "vec3", "vector3", "vector-3",
  "4-vector", "4vector", "4vec", "4v", "v4", "vec4", "vector4", "vector-4",
  "quaternion", "q",
  ""
};

#define SISS limnSplineInfoScalar
#define SI2V limnSplineInfo2Vector
#define SI3V limnSplineInfo3Vector
#define SI4V limnSplineInfo4Vector
#define SIQQ limnSplineInfoQuaternion

int
_limnSplineInfoValEqv[] = {
  SISS, SISS, SISS, SISS,
  SI2V, SI2V, SI2V, SI2V, SI2V, SI2V, SI2V, SI2V,
  SI3V, SI3V, SI3V, SI3V, SI3V, SI3V, SI3V, SI3V,
  SI4V, SI4V, SI4V, SI4V, SI4V, SI4V, SI4V, SI4V,
  SIQQ, SIQQ
};

airEnum
_limnSplineInfo = {
  "spline-info",
  LIMN_SPLINE_TYPE_MAX,
  _limnSplineInfoStr,  NULL,
  _limnSplineInfoDesc,
  _limnSplineInfoStrEqv, _limnSplineInfoValEqv,
  AIR_FALSE
};
airEnum *
limnSplineInfo = &_limnSplineInfo;

/*
******** limnSplineInfoSize[]
**
** gives the number of scalars per "value" for each splineInfo 
*/
int
limnSplineInfoSize[LIMN_SPLINE_INFO_MAX+1] = {
  -1, /* limnSplineInfoUnknown */
  1,  /* limnSplineInfoScalar */
  2,  /* limnSplineInfo2Vector */
  3,  /* limnSplineInfo3Vector */
  4,  /* limnSplineInfo4Vector */
  4   /* limnSplineInfoQuaternion */
};

/*
******** limnSplineTypeHasImplicitTangents[]
**
** this is non-zero when the spline path is determined solely the
** main control point values, without needing additional control 
** points (as in cubic Bezier) or tangent information (as in Hermite)
*/
int
limnSplineTypeHasImplicitTangents[LIMN_SPLINE_TYPE_MAX+1] = {
  AIR_FALSE, /* limnSplineTypeUnknown */
  AIR_TRUE,  /* limnSplineTypeLinear */
  AIR_FALSE, /* limnSplineTypeTimeWarp */
  AIR_FALSE, /* limnSplineTypeHermite */
  AIR_FALSE, /* limnSplineTypeCubicBezier */
  AIR_TRUE   /* limnSplineTypeBC */
};

int
limnSplineNumPoints(limnSpline *spline) {
  int ret;

  ret = -1;
  if (spline) {
    ret = spline->ncpt->axis[2].size;
  }
  return ret;
}

double 
limnSplineMinT(limnSpline *spline) {
  double ret;

  ret = AIR_NAN;
  if (spline) {
    ret = spline->time ? spline->time[0] : 0;
  }
  return ret;
}

double 
limnSplineMaxT(limnSpline *spline) {
  double ret;
  int N;

  ret = AIR_NAN;
  if (spline) {
    N = spline->ncpt->axis[2].size;
    if (spline->time) {
      ret = spline->time[N-1];
    } else {
      ret = spline->loop ? N : N-1;
    }
  }
  return ret;
}

void
limnSplineBCSet(limnSpline *spline, double B, double C) {

  if (spline) {
    spline->B = B;
    spline->C = C;
  }
  return;
}

/*
** the spline command-line specification is of the form 
** <nrrdFileName>:<splineType>[:B,C]
**
** for the time being, there is no way to explicitly give
** the spline info; it is determined from the nrrd dimensions.
*/
int
_limnHestSplineParse(void *ptr, char *_str, char err[AIR_STRLEN_HUGE]) {
  char me[] = "_limnHestSplineParse", *nerr, *str, *col, *typeS,
    *fnameS=NULL, *paramS=NULL;
  limnSpline **splineP;
  int type, info;
  Nrrd *ninA, *ninB;
  airArray *mop;
  double B=0, C=0;
  
  if (!(ptr && _str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  splineP = (limnSpline **)ptr;
  if (!airStrlen(_str)) {
    /* got an empty string, which for now we take as an okay way
       to NOT ask for a spline */
    *splineP = NULL;
    return 0;
  }

  mop = airMopNew();
  airMopAdd(mop, str=airStrdup(_str), airFree, airMopAlways);
  
  /* find seperation between nrrd filename and "type[:B,C] */
  col = strchr(str, ':');
  if (!col) {
    sprintf(err, "%s: saw no colon seperator in \"%s\"", me, _str);
    airMopError(mop); return 1;
  }
  fnameS = str;
  *col = 0;
  typeS = col+1;

  airMopAdd(mop, ninA = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  if (nrrdLoad(ninA, fnameS, NULL)) {
    airMopAdd(mop, nerr = biffGetDone(NRRD), airFree, airMopOnError);
    sprintf(err, "%s: couldn't read control point nrrd:\n", me);
    strncat(err, nerr, AIR_STRLEN_HUGE-1-strlen(err));
    airMopError(mop); return 1;
  }
  if (1 == ninA->dim) {
    info = limnSplineInfoScalar;
  } else {
    for (info = limnSplineInfoUnknown+1;
	 info < limnSplineInfoLast;
	 info++) {
      if (ninA->axis[0].size == limnSplineInfoSize[info]) {
	break;
      }
    }
    if (limnSplineInfoLast == info) {
      sprintf(err, "%s: nin->axis[0].size %d doesn't match that "
	      "of any known limnSplineInfo", me, ninA->axis[0].size);
      airMopError(mop); return 1;
    }
    /* else we've set info okay */
  }

  col = strchr(typeS, ':');
  if (col) {
    *col = 0;
    paramS = col+1;
  }
  if (limnSplineTypeUnknown == (type = airEnumVal(limnSplineType, typeS))) {
    sprintf(err, "%s: couldn't parse \"%s\" as spline type", me, typeS);
    airMopError(mop); return 1;
  }
  if (limnSplineTypeTimeWarp == type 
      && limnSplineInfoScalar != info) {
    sprintf(err, "%s: can only time-warp %s info, not %s", me,
	    airEnumStr(limnSplineInfo, limnSplineInfoScalar),
	    airEnumStr(limnSplineInfo, info));
    airMopError(mop); return 1;
  }

  if (!( (limnSplineTypeBC == type) == !!paramS )) {
    sprintf(err, "%s: spline type %s %s, but %s a parameter string %s%s%s", me,
	    (limnSplineTypeBC == type) ? "is" : "is not",
	    airEnumStr(limnSplineType, limnSplineTypeBC),
	    !!paramS ? "got" : "did not get",
	    !!paramS ? "\"" : "",
	    !!paramS ? paramS : "",
	    !!paramS ? "\"" : "");
    airMopError(mop); return 1;
  }
  if (limnSplineTypeBC == type) {
    if (2 != sscanf(paramS, "%lg,%lg", &B, &C)) {
      sprintf(err, "%s: couldn't parse \"B,C\" parameters from \"%s\"", me,
	      paramS);
      airMopError(mop); return 1;
    }
  }

  airMopAdd(mop, ninB = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  if (limnSplineNrrdCleverFix(ninB, ninA, type, info)) {
    airMopAdd(mop, nerr = biffGetDone(LIMN), airFree, airMopOnError);
    sprintf(err, "%s: couldn't reshape given nrrd:\n", me);
    strncat(err, nerr, AIR_STRLEN_HUGE-1-strlen(err));
    airMopError(mop); return 1;
  }
  if (!( *splineP = limnSplineNew(ninB, type, info) )) {
    airMopAdd(mop, nerr = biffGetDone(LIMN), airFree, airMopOnError);
    sprintf(err, "%s: couldn't create spline:\n", me);
    strncat(err, nerr, AIR_STRLEN_HUGE-1-strlen(err));
    airMopError(mop); return 1;
  }
  if (limnSplineTypeBC == type) {
    limnSplineBCSet(*splineP, B, C);
  }  
  airMopOkay(mop);
  return 0;
}


hestCB
_limnHestSpline = {
  sizeof(limnSpline *),
  "spline specification",
  _limnHestSplineParse,
  (airMopper)limnSplineNix
}; 

hestCB *
limnHestSpline = &_limnHestSpline;

