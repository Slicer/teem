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
#include "privateGage.h"

void
gageShapeReset(gageShape *shape) {
  int i, ai;
  
  if (shape) {
    ELL_3V_SET(shape->size, -1, -1, -1);
    shape->center = nrrdCenterUnknown;
    ELL_3V_SET(shape->spacing, AIR_NAN, AIR_NAN, AIR_NAN);
    for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
      /* valgrind complained about AIR_NAN at -O2 */
      for (ai=0; ai<=2; ai++) {
	shape->fwScale[i][ai] = airNaN();
      }
    }
    ELL_3V_SET(shape->volHalfLen, AIR_NAN, AIR_NAN, AIR_NAN);
    ELL_3V_SET(shape->voxLen, AIR_NAN, AIR_NAN, AIR_NAN);
  }
  return;
}

gageShape *
gageShapeNew() {
  gageShape *shape;
  
  shape = (gageShape *)calloc(1, sizeof(gageShape));
  if (shape) {
    gageShapeReset(shape);
  }
  return shape;
}

gageShape *
gageShapeNix(gageShape *shape) {
  
  return airFree(shape);
}


int
gageShapeSet(gageShape *shape, Nrrd *nin, int baseDim) {
  char me[]="gageShapeSet", err[AIR_STRLEN_MED];
  int i, ai, ms, num[3];
  NrrdAxis *ax[3];
  double maxLen;

  if (!( shape && nin )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err);  if (shape) { gageShapeReset(shape); }
    return 1;
  }

  if (!(nin->dim == 3 + baseDim)) {
    sprintf(err, "%s: nrrd should be %d-D, not %d-D",
	    me, 3 + baseDim, nin->dim);
    biffAdd(GAGE, err); gageShapeReset(shape);
    return 1;
  }
  for (ai=0; ai<=2; ai++) {
    ax[ai] = &(nin->axis[baseDim+ai]);
  }

  /* equality of axis centers is checked by gageVolumeCheck(), but
     we do something more here for same reasons as above */
  if (ax[0]->center == ax[1]->center && ax[1]->center == ax[2]->center) {
    shape->center = (nrrdCenterUnknown == ax[0]->center
		     ? gageDefCenter
		     : ax[0]->center);
  } else {
    /* regular gage use wouldn't allow this ... */
    shape->center = gageDefCenter;
  }
  ms = (nrrdCenterCell == shape->center ? 1 : 2);
  if (!(ax[0]->size >= ms && ax[0]->size >= ms && ax[0]->size >= ms )) {
    sprintf(err, "%s: sizes (%d,%d,%d) must all be greater than %d "
	    "(minimum # of %s-centered samples)", me, 
	    ax[0]->size, ax[1]->size, ax[2]->size, ms,
	    airEnumStr(nrrdCenter, shape->center));
    biffAdd(GAGE, err); gageShapeReset(shape);
    return 1;
  }
  for (ai=0; ai<=2; ai++) {
    shape->size[ai] = ax[ai]->size;
  }
  /* regular gage usage would require all spacings to exist, we do this
     to allow gageShapeSet to be used on other contexts */
  for (ai=0; ai<=2; ai++) {
    shape->spacing[ai] = (AIR_EXISTS(ax[ai]->spacing) 
			  ? ax[ai]->spacing 
			  : nrrdDefSpacing);
  }
  for (i=0; i<GAGE_KERNEL_NUM; i++) {
    switch (i) {
    case gageKernel00:
    case gageKernel10:
    case gageKernel20:
      /* interpolation requires no re-weighting for non-unit spacing */
      for (ai=0; ai<=2; ai++) {
	shape->fwScale[i][ai] = 1.0;
      }
      break;
    case gageKernel11:
    case gageKernel21:
      for (ai=0; ai<=2; ai++) {
	shape->fwScale[i][ai] = 1.0/(shape->spacing[ai]);
      }
      break;
    case gageKernel22:
      for (ai=0; ai<=2; ai++) {
	shape->fwScale[i][ai] = 
	  1.0/((shape->spacing[ai])*(shape->spacing[ai]));
      }
      break;
    }
  }

  /* learn lengths for bounding nrrd in bi-unit cube */
  maxLen = 0.0;
  for (ai=0; ai<=2; ai++) {
    num[ai] = (nrrdCenterNode == shape->center
	       ? shape->size[ai]-1
	       : shape->size[ai]);
    shape->volHalfLen[ai] = num[ai]*shape->spacing[ai];
    maxLen = AIR_MAX(maxLen, shape->volHalfLen[ai]);
  }
  for (ai=0; ai<=2; ai++) {
    shape->volHalfLen[ai] /= maxLen;
    shape->voxLen[ai] = 2*shape->volHalfLen[ai]/num[ai];
  }

  return 0;
}

void
gageShapeUnitWtoI(gageShape *shape, double index[3], double world[3]) {
  int i;
  
  if (nrrdCenterNode == shape->center) {
    for (i=0; i<=2; i++) {
      index[i] = NRRD_NODE_IDX(-shape->volHalfLen[i], shape->volHalfLen[i],
			       shape->size[i], world[i]);
    }
  } else {
    for (i=0; i<=2; i++) {
      index[i] = NRRD_CELL_IDX(-shape->volHalfLen[i], shape->volHalfLen[i],
			       shape->size[i], world[i]);
    }
  }
}

void
gageShapeUnitItoW(gageShape *shape, double world[3], double index[3]) {
  int i;
  
  if (nrrdCenterNode == shape->center) {
    for (i=0; i<=2; i++) {
      world[i] = NRRD_NODE_POS(-shape->volHalfLen[i], shape->volHalfLen[i],
			       shape->size[i], index[i]);
    }
  } else {
    for (i=0; i<=2; i++) {
      world[i] = NRRD_CELL_POS(-shape->volHalfLen[i], shape->volHalfLen[i],
			       shape->size[i], index[i]);
    }
  }
}

int
gageShapeEqual(gageShape *shape1, char *_name1,
	       gageShape *shape2, char *_name2) {
  char me[]="_gageShapeEqual", err[AIR_STRLEN_MED],
    *name1, *name2, what[] = "???";

  name1 = _name1 ? _name1 : what;
  name2 = _name2 ? _name2 : what;
  if (!( shape1->size[0] == shape2->size[0] &&
	 shape1->size[1] == shape2->size[1] &&
	 shape1->size[2] == shape2->size[2] )) {
    sprintf(err, "%s: dimensions of %s (%d,%d,%d) != %s's (%d,%d,%d)", me,
	    name1, shape1->size[0], shape1->size[1], shape1->size[2],
	    name2, shape2->size[0], shape2->size[1], shape2->size[2]);
    biffAdd(GAGE, err); return 0;
  }
  if (!( shape1->spacing[0] == shape2->spacing[0] &&
	 shape1->spacing[1] == shape2->spacing[1] &&
	 shape1->spacing[2] == shape2->spacing[2] )) {
    sprintf(err, "%s: spacings of %s (%g,%g,%g) != %s's (%g,%g,%g)", me,
	    name1, shape1->spacing[0], shape1->spacing[1], shape1->spacing[2],
	    name2, shape2->spacing[0], shape2->spacing[1], shape2->spacing[2]);
    biffAdd(GAGE, err); return 0;
  }
  if (!( shape1->center == shape2->center )) {
    sprintf(err, "%s: centering of %s (%s) != %s's (%s)", me,
	    name1, airEnumStr(nrrdCenter, shape1->center),
	    name2, airEnumStr(nrrdCenter, shape2->center));
    biffAdd(GAGE, err); return 0;
  }

  return 1;
}
