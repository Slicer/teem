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


#include "../limn.h"

char *info = ("Save a single ellipsoid or superquadric into an OFF file.");

void
washQtoM3(float m[9], float q[4]) {
  float p[4], w, x, y, z, len;

  ELL_4V_COPY(p, q);
  len = ELL_4V_LEN(p);
  ELL_4V_SCALE(p, 1.0/len, p);
  w = p[0];
  x = p[1];
  y = p[2];
  z = p[3];
  /* mathematica work implies that we should be 
     setting ROW vectors here */
  ELL_3V_SET(m+0, 
	     1 - 2*(y*y + z*z),
	     2*(x*y - w*z),
	     2*(x*z + w*y));
  ELL_3V_SET(m+3,
	     2*(x*y + w*z),
	     1 - 2*(x*x + z*z),
	     2*(y*z - w*x));
  ELL_3V_SET(m+6,
	     2*(x*z - w*y),
	     2*(y*z + w*x),
	     1 - 2*(x*x + y*y));
}

int
main(int argc, char *argv[]) {
  char *me, *err, *outS;
  float p[3], q[4], mR[9], eval[3], len, sh, cl, cp, qA, qB;
  float matA[16], matB[16], os, rad, edgeWidth[5], AB[2];
  hestOpt *hopt=NULL;
  airArray *mop;
  limnObject *obj;
  limnLook *look; int lookRod, lookSoid;
  limnPart *part; int partIdx=-1; /* sssh */
  int res, axis, sphere;
  FILE *file;

  mop = airMopNew();
  
  edgeWidth[0] = 0;
  edgeWidth[1] = 0;
  me = argv[0];
  hestOptAdd(&hopt, "sc", "scalings", airTypeFloat, 3, 3, eval, "1 1 1",
	     "axis-aligned scaling to do on ellipsoid");
  hestOptAdd(&hopt, "AB", "A, B exponents", airTypeFloat, 2, 2, AB, "nan nan",
	     "Directly set the A, B parameters to the superquadric surface, "
	     "over-riding the default behavior of determining them from the "
	     "scalings \"-sc\" as superquadric tensor glyphs");
  hestOptAdd(&hopt, "os", "over-all scaling", airTypeFloat, 1, 1, &os, "1",
	     "over-all scaling (multiplied by scalings)");
  hestOptAdd(&hopt, "sh", "superquad sharpness", airTypeFloat, 1, 1, &sh, "0",
	     "how much to sharpen edges as a "
	     "function of differences between eigenvalues");
  hestOptAdd(&hopt, "sphere", NULL, airTypeInt, 0, 0, &sphere, NULL,
	     "use a sphere instead of a superquadric");
  hestOptAdd(&hopt, "p", "x y z", airTypeFloat, 3, 3, p, "0 0 0",
	     "location in quaternion quotient space");
  hestOptAdd(&hopt, "r", "radius", airTypeFloat, 1, 1, &rad, "0.015",
	     "black axis cylinder radius (or 0.0 to not drawn these)");
  hestOptAdd(&hopt, "res", "resolution", airTypeInt, 1, 1, &res, "25",
	     "tesselation resolution for both glyph and axis cylinders");
  hestOptAdd(&hopt, "o", "output OFF", airTypeString, 1, 1, &outS, "out.off",
	     "output file to save OFF into");
  hestParseOrDie(hopt, argc-1, argv+1, NULL,
		 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);
  
  obj = limnObjectNew(10, AIR_TRUE);
  airMopAdd(mop, obj, (airMopper)limnObjectNix, airMopAlways);

  /* create limnLooks for ellipsoid and for rods */
  lookSoid = limnObjectLookAdd(obj);
  look = obj->look + lookSoid;
  ELL_4V_SET(look->rgba, 1, 1, 1, 1);
  ELL_3V_SET(look->kads, 0.2, 0.8, 0);
  look->spow = 0;
  lookRod = limnObjectLookAdd(obj);
  look = obj->look + lookRod;
  ELL_4V_SET(look->rgba, 0, 0, 0, 1);
  ELL_3V_SET(look->kads, 1, 0, 0);
  look->spow = 0;

  q[0] = 1.0;
  q[1] = p[0];
  q[2] = p[1];
  q[3] = p[2];
  len = ELL_4V_LEN(q);
  ELL_4V_SCALE(q, 1.0/len, q);
  washQtoM3(mR, q);

  if (AIR_EXISTS(AB[0]) && AIR_EXISTS(AB[1])) {
    qA = AB[0];
    qB = AB[1];
    axis = 2;
  } else {
    ELL_3V_SCALE(eval, os, eval);
    ELL_SORT3(eval[0], eval[1], eval[2], cl);
    cl = (eval[0] - eval[1])/(eval[0] + eval[1] + eval[2]);
    cp = 2*(eval[1] - eval[2])/(eval[0] + eval[1] + eval[2]);
    if (cl > cp) {
      axis = ELL_MAX3_IDX(eval[0], eval[1], eval[2]);
      qA = pow(1-cp, sh);
      qB = pow(1-cl, sh);
    } else {
      axis = ELL_MIN3_IDX(eval[0], eval[1], eval[2]);
      qA = pow(1-cl, sh);
      qB = pow(1-cp, sh);
    }
    /*
    fprintf(stderr, "eval = %g %g %g -> cl=%g %s cp=%g -> axis = %d\n",
	    eval[0], eval[1], eval[2], cl, cl > cp ? ">" : "<", cp, axis);
    */
  }
  if (sphere) {
    partIdx = limnObjectPolarSphereAdd(obj, lookSoid, 
				       0, 2*res, res);
  } else {
    partIdx = limnObjectPolarSuperquadAdd(obj, lookSoid, 
					  axis, qA, qB, 2*res, res);
  }
  part = obj->part + partIdx;
  ELL_4M_IDENTITY_SET(matA);
  ELL_4M_SCALE_SET(matB, eval[0], eval[1], eval[2]);
  ell_4m_post_mul_f(matA, matB);
  ELL_43M_INSET(matB, mR);
  ell_4m_post_mul_f(matA, matB);
  limnObjectPartTransform(obj, partIdx, matA);

  if (rad) {
    partIdx = limnObjectCylinderAdd(obj, lookRod, 0, res);
    ELL_4M_IDENTITY_SET(matA);
    ELL_4M_SCALE_SET(matB, (1-eval[0])/2, rad, rad);
    ell_4m_post_mul_f(matA, matB);
    ELL_4M_TRANSLATE_SET(matB, (1+eval[0])/2, 0.0, 0.0); 
    ell_4m_post_mul_f(matA, matB);
    limnObjectPartTransform(obj, partIdx, matA);
    
    partIdx = limnObjectCylinderAdd(obj, lookRod, 0, res);
    ELL_4M_IDENTITY_SET(matA);
    ELL_4M_SCALE_SET(matB, (1-eval[0])/2, rad, rad);
    ell_4m_post_mul_f(matA, matB);
    ELL_4M_TRANSLATE_SET(matB, -(1+eval[0])/2, 0.0, 0.0); 
    ell_4m_post_mul_f(matA, matB);
    limnObjectPartTransform(obj, partIdx, matA);
    
    partIdx = limnObjectCylinderAdd(obj, lookRod, 1, res);
    ELL_4M_IDENTITY_SET(matA);
    ELL_4M_SCALE_SET(matB, rad, (1-eval[1])/2, rad);
    ell_4m_post_mul_f(matA, matB);
    ELL_4M_TRANSLATE_SET(matB, 0.0, (1+eval[1])/2, 0.0); 
    ell_4m_post_mul_f(matA, matB);
    limnObjectPartTransform(obj, partIdx, matA);
    
    partIdx = limnObjectCylinderAdd(obj, lookRod, 1, res);
    ELL_4M_IDENTITY_SET(matA);
    ELL_4M_SCALE_SET(matB, rad, (1-eval[1])/2, rad);
    ell_4m_post_mul_f(matA, matB);
    ELL_4M_TRANSLATE_SET(matB, 0.0, -(1+eval[1])/2, 0.0); 
    ell_4m_post_mul_f(matA, matB);
    limnObjectPartTransform(obj, partIdx, matA);
    
    partIdx = limnObjectCylinderAdd(obj, lookRod, 2, res);
    ELL_4M_IDENTITY_SET(matA);
    ELL_4M_SCALE_SET(matB, rad, rad, (1-eval[2])/2);
    ell_4m_post_mul_f(matA, matB);
    ELL_4M_TRANSLATE_SET(matB, 0.0, 0.0, (1+eval[2])/2); 
    ell_4m_post_mul_f(matA, matB);
    limnObjectPartTransform(obj, partIdx, matA);
    
    partIdx = limnObjectCylinderAdd(obj, lookRod, 2, res);
    ELL_4M_IDENTITY_SET(matA);
    ELL_4M_SCALE_SET(matB, rad, rad, (1-eval[2])/2);
    ell_4m_post_mul_f(matA, matB);
    ELL_4M_TRANSLATE_SET(matB, 0.0, 0.0, -(1+eval[2])/2); 
    ell_4m_post_mul_f(matA, matB);
    limnObjectPartTransform(obj, partIdx, matA);
  }

  file = airFopen(outS, stdout, "w");
  airMopAdd(mop, file, (airMopper)airFclose, airMopAlways);

  if (limnObjectOFFWrite(file, obj)) {
    airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  
  airMopOkay(mop);
  return 0;
}

