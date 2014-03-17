/*
  Teem: Tools to process and visualize scientific data and images             .
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
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

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "../ten.h"

char *info = ("Save a single ellipsoid or superquadric into an OFF file.");

int
soidDoit(limnObject *obj, int look,
         int gtype, float gamma, int res,
         float AB[2], float ten[7]) {
  int partIdx, axis;
  float cl, cp, qA, qB, eval[3], evec[9], matA[16], matB[16];

  if (AB) {
    qA = AB[0];
    qB = AB[1];
    axis = 2;
  } else {
    tenEigensolve_f(eval, evec, ten);
    ELL_SORT3(eval[0], eval[1], eval[2], cl);
    cl = (eval[0] - eval[1])/(eval[0] + eval[1] + eval[2]);
    cp = 2*(eval[1] - eval[2])/(eval[0] + eval[1] + eval[2]);
    if (cl > cp) {
      axis = 0;
      qA = pow(1-cp, gamma);
      qB = pow(1-cl, gamma);
    } else {
      axis = 2;
      qA = pow(1-cl, gamma);
      qB = pow(1-cp, gamma);
    }
    /*
    fprintf(stderr, "eval = %g %g %g -> cl=%g %s cp=%g -> axis = %d\n",
            eval[0], eval[1], eval[2], cl, cl > cp ? ">" : "<", cp, axis);
    */
  }

  if (tenGlyphTypeBox == gtype) {
    partIdx = limnObjectCubeAdd(obj, look);
  } else if (tenGlyphTypeSphere == gtype) {
    partIdx = limnObjectPolarSphereAdd(obj, look,
                                       0, 2*res, res);
  } else {
    partIdx = limnObjectPolarSuperquadAdd(obj, look,
                                          axis, qA, qB, 2*res, res);
  }
  ELL_4M_IDENTITY_SET(matA);
  ELL_4V_SET(matB + 0*4, eval[0],       0,       0, 0);
  ELL_4V_SET(matB + 1*4,       0, eval[1],       0, 0);
  ELL_4V_SET(matB + 2*4,       0,       0, eval[2], 0);
  ELL_4V_SET(matB + 3*4,       0,       0,       0, 1);
  ELL_4M_SCALE_SET(matB, eval[0], eval[1], eval[2]);
  ell_4m_post_mul_f(matA, matB);
  ELL_4V_SET(matB + 0*4, evec[0 + 0*3], evec[0 + 1*3], evec[0 + 2*3], 0);
  ELL_4V_SET(matB + 1*4, evec[1 + 0*3], evec[1 + 1*3], evec[1 + 2*3], 0);
  ELL_4V_SET(matB + 2*4, evec[2 + 0*3], evec[2 + 1*3], evec[2 + 2*3], 0);
  ELL_4V_SET(matB + 3*4,             0,             0,             0, 1);
  ell_4m_post_mul_f(matA, matB);
  limnObjectPartTransform(obj, partIdx, matA);

  return partIdx;
}

static void
scalingMatrix(double mat[9], double vec[3], double scl) {
  double dir[3], tmp[9], len;

  ELL_3V_NORM(dir, vec, len);
  ELL_3MV_OUTER(tmp, dir, dir);
  ELL_3M_SCALE(tmp, scl-1, tmp);
  ELL_3M_IDENTITY_SET(mat);
  ELL_3M_ADD2(mat, mat, tmp);
  return;
}

int
main(int argc, const char *argv[]) {
  const char *me;
  char *err, *outS;
  double eval[3], matA[9], matB[9], sval[3], uu[9], vv[9], escl[5],
    view[3];
  float matAf[9], matBf[16];
  float pp[3], qq[4], mR[9], len, gamma;
  float os, vs, rad, AB[2], ten[7];
  hestOpt *hopt=NULL;
  airArray *mop;
  limnObject *obj;
  limnLook *look; int lookRod, lookSoid;
  float kadsRod[3], kadsSoid[3];
  int gtype, partIdx=-1; /* sssh */
  int res;
  FILE *file;

  me = argv[0];
  hestOptAdd(&hopt, "sc", "evals", airTypeDouble, 3, 3, eval, "1 1 1",
             "original eigenvalues of tensor to be visualized");
  hestOptAdd(&hopt, "AB", "A, B exponents", airTypeFloat, 2, 2, AB, "nan nan",
             "Directly set the A, B parameters to the superquadric surface, "
             "over-riding the default behavior of determining them from the "
             "scalings \"-sc\" as superquadric tensor glyphs");
  hestOptAdd(&hopt, "os", "over-all scaling", airTypeFloat, 1, 1, &os, "1",
             "over-all scaling (multiplied by scalings)");
  hestOptAdd(&hopt, "vs", "view-dir scaling", airTypeFloat, 1, 1, &vs, "1",
             "scaling along view-direction (to show off bas-relief "
             "ambibuity of ellipsoids versus superquads)");
  hestOptAdd(&hopt, "es", "extra scaling", airTypeDouble, 5, 5, escl,
             "2 1 0 0 1", "extra scaling specified with five values "
             "0:tensor|1:geometry|2:none vx vy vz scaling");
  hestOptAdd(&hopt, "fr", "from (eye) point", airTypeDouble, 3, 3, &view,
             "4 4 4", "eye point, needed for non-unity \"-vs\"");
  hestOptAdd(&hopt, "gamma", "superquad sharpness", airTypeFloat, 1, 1,
             &gamma, "0",
             "how much to sharpen edges as a "
             "function of differences between eigenvalues");
  hestOptAdd(&hopt, "g", "glyph shape", airTypeEnum, 1, 1, &gtype, "sqd",
             "glyph to use; not all are implemented here",
             NULL, tenGlyphType);
  hestOptAdd(&hopt, "pp", "x y z", airTypeFloat, 3, 3, pp, "0 0 0",
             "transform: rotation identified by"
             "location in quaternion quotient space");
  hestOptAdd(&hopt, "r", "radius", airTypeFloat, 1, 1, &rad, "0.015",
             "black axis cylinder radius (or 0.0 to not drawn these)");
  hestOptAdd(&hopt, "res", "resolution", airTypeInt, 1, 1, &res, "25",
             "tesselation resolution for both glyph and axis cylinders");
  hestOptAdd(&hopt, "pg", "ka kd ks", airTypeFloat, 3, 3, kadsSoid,
             "0.2 0.8 0.0",
             "phong coefficients for glyph");
  hestOptAdd(&hopt, "pr", "ka kd ks", airTypeFloat, 3, 3, kadsRod, "1 0 0",
             "phong coefficients for black rods (if being drawn)");
  hestOptAdd(&hopt, "o", "output OFF", airTypeString, 1, 1, &outS, "out.off",
             "output file to save OFF into");
  hestParseOrDie(hopt, argc-1, argv+1, NULL,
                 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  obj = limnObjectNew(1000, AIR_TRUE);
  airMopAdd(mop, obj, (airMopper)limnObjectNix, airMopAlways);

  if (!( 0 == escl[0] || 1 == escl[0] || 2 == escl[0] )) {
    fprintf(stderr, "%s: escl[0] %g not 0, 1 or 2\n", me, escl[0]);
    airMopError(mop); return 1;
  }
  if (!(tenGlyphTypeBox == gtype ||
        tenGlyphTypeSphere == gtype ||
        tenGlyphTypeSuperquad == gtype)) {
    fprintf(stderr, "%s: got %s %s, but here only do %s, %s, or %s\n", me,
            tenGlyphType->name,
            airEnumStr(tenGlyphType, gtype),
            airEnumStr(tenGlyphType, tenGlyphTypeBox),
            airEnumStr(tenGlyphType, tenGlyphTypeSphere),
            airEnumStr(tenGlyphType, tenGlyphTypeSuperquad));
    airMopError(mop); return 1;
  }

  /* create limnLooks for glyph and for rods */
  lookSoid = limnObjectLookAdd(obj);
  look = obj->look + lookSoid;
  ELL_4V_SET(look->rgba, 1, 1, 1, 1);
  ELL_3V_COPY(look->kads, kadsSoid);

  look->spow = 0;
  lookRod = limnObjectLookAdd(obj);
  look = obj->look + lookRod;
  ELL_4V_SET(look->rgba, 0, 0, 0, 1);
  ELL_3V_COPY(look->kads, kadsRod);
  look->spow = 0;

  ELL_3M_IDENTITY_SET(matA); /* A = I */
  ELL_3V_SCALE(eval, os, eval);
  ELL_3M_SCALE_SET(matB, eval[0], eval[1], eval[2]); /* B = diag(eval) */
  ell_3m_post_mul_d(matA, matB); /* A = B*A = diag(eval) */

  if (0 == escl[0]) {
    scalingMatrix(matB, escl + 1, escl[4]);
    ell_3m_post_mul_d(matA, matB);
  }

  if (1 != vs) {
    if (!ELL_3V_LEN(view)) {
      fprintf(stderr, "%s: need non-zero view for vs %g != 1\n", me, vs);
      airMopError(mop); return 1;
    }
    scalingMatrix(matB, view, vs);
    /* the scaling along the view direction is a symmetric matrix,
       but applying that scaling to the symmetric input tensor
       is not necessarily symmetric */
    ell_3m_post_mul_d(matA, matB);  /* A = B*A */
  }
  /* so we do an SVD to get rotation U and the scalings sval[] */
  /* U * diag(sval) * V */
  ell_3m_svd_d(uu, sval, vv, matA, AIR_TRUE);

  /*
  fprintf(stderr, "%s: ____________________________________\n", me);
  fprintf(stderr, "%s: mat = \n", me);
  ell_3m_print_d(stderr, matA);
  fprintf(stderr, "%s: uu = \n", me);
  ell_3m_print_d(stderr, uu);
  ELL_3M_TRANSPOSE(matC, uu);
  ELL_3M_MUL(matB, uu, matC);
  fprintf(stderr, "%s: uu * uu^T = \n", me);
  ell_3m_print_d(stderr, matB);
  fprintf(stderr, "%s: sval = %g %g %g\n", me, sval[0], sval[1], sval[2]);
  fprintf(stderr, "%s: vv = \n", me);
  ell_3m_print_d(stderr, vv);
  ELL_3M_MUL(matB, vv, vv);
  fprintf(stderr, "%s: vv * vv^T = \n", me);
  ELL_3M_TRANSPOSE(matC, vv);
  ELL_3M_MUL(matB, vv, matC);
  ell_3m_print_d(stderr, matB);
  ELL_3M_IDENTITY_SET(matA);
  ell_3m_pre_mul_d(matA, uu);
  ELL_3M_SCALE_SET(matB, sval[0], sval[1], sval[2]);
  ell_3m_pre_mul_d(matA, matB);
  ell_3m_pre_mul_d(matA, vv);
  fprintf(stderr, "%s: uu * diag(sval) * vv = \n", me);
  ell_3m_print_d(stderr, matA);
  fprintf(stderr, "%s: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", me);
  */

  /* now create symmetric matrix out of U and sval */
  /* A = I */
  ELL_3M_IDENTITY_SET(matA);
  ell_3m_pre_mul_d(matA, uu);   /* A = A*U = I*U = U */
  ELL_3M_SCALE_SET(matB, sval[0], sval[1], sval[2]); /* B = diag(sval) */
  ell_3m_pre_mul_d(matA, matB); /* A = U*diag(sval) */
  ELL_3M_TRANSPOSE(matB, uu);
  ell_3m_pre_mul_d(matA, matB); /* A = U*diag(sval)*U^T */
  TEN_M2T(ten, matA);

  partIdx = soidDoit(obj, lookSoid,
                     gtype, gamma, res,
                     (AIR_EXISTS(AB[0]) && AIR_EXISTS(AB[1])) ? AB : NULL,
                     ten);

  if (1 == escl[0]) {
    scalingMatrix(matB, escl + 1, escl[4]);
    ELL_43M_INSET(matBf, matB);
    limnObjectPartTransform(obj, partIdx, matBf);
  }
  /* this is a rotate on the geomtry; nothing to do with the tensor */
  ELL_4V_SET(qq, 1, pp[0], pp[1], pp[2]);
  ELL_4V_NORM(qq, qq, len);
  ell_q_to_3m_f(mR, qq);
  ELL_43M_INSET(matBf, mR);
  limnObjectPartTransform(obj, partIdx, matBf);

  if (rad) {
    partIdx = limnObjectCylinderAdd(obj, lookRod, 0, res);
    ELL_4M_IDENTITY_SET(matAf);
    ELL_4M_SCALE_SET(matBf, (1-eval[0])/2, rad, rad);
    ell_4m_post_mul_f(matAf, matBf);
    ELL_4M_TRANSLATE_SET(matBf, (1+eval[0])/2, 0.0, 0.0);
    ell_4m_post_mul_f(matAf, matBf);
    limnObjectPartTransform(obj, partIdx, matAf);

    partIdx = limnObjectCylinderAdd(obj, lookRod, 0, res);
    ELL_4M_IDENTITY_SET(matAf);
    ELL_4M_SCALE_SET(matBf, (1-eval[0])/2, rad, rad);
    ell_4m_post_mul_f(matAf, matBf);
    ELL_4M_TRANSLATE_SET(matBf, -(1+eval[0])/2, 0.0, 0.0);
    ell_4m_post_mul_f(matAf, matBf);
    limnObjectPartTransform(obj, partIdx, matAf);

    partIdx = limnObjectCylinderAdd(obj, lookRod, 1, res);
    ELL_4M_IDENTITY_SET(matAf);
    ELL_4M_SCALE_SET(matBf, rad, (1-eval[1])/2, rad);
    ell_4m_post_mul_f(matAf, matBf);
    ELL_4M_TRANSLATE_SET(matBf, 0.0, (1+eval[1])/2, 0.0);
    ell_4m_post_mul_f(matAf, matBf);
    limnObjectPartTransform(obj, partIdx, matAf);

    partIdx = limnObjectCylinderAdd(obj, lookRod, 1, res);
    ELL_4M_IDENTITY_SET(matAf);
    ELL_4M_SCALE_SET(matBf, rad, (1-eval[1])/2, rad);
    ell_4m_post_mul_f(matAf, matBf);
    ELL_4M_TRANSLATE_SET(matBf, 0.0, -(1+eval[1])/2, 0.0);
    ell_4m_post_mul_f(matAf, matBf);
    limnObjectPartTransform(obj, partIdx, matAf);

    partIdx = limnObjectCylinderAdd(obj, lookRod, 2, res);
    ELL_4M_IDENTITY_SET(matAf);
    ELL_4M_SCALE_SET(matBf, rad, rad, (1-eval[2])/2);
    ell_4m_post_mul_f(matAf, matBf);
    ELL_4M_TRANSLATE_SET(matBf, 0.0, 0.0, (1+eval[2])/2);
    ell_4m_post_mul_f(matAf, matBf);
    limnObjectPartTransform(obj, partIdx, matAf);

    partIdx = limnObjectCylinderAdd(obj, lookRod, 2, res);
    ELL_4M_IDENTITY_SET(matAf);
    ELL_4M_SCALE_SET(matBf, rad, rad, (1-eval[2])/2);
    ell_4m_post_mul_f(matAf, matBf);
    ELL_4M_TRANSLATE_SET(matBf, 0.0, 0.0, -(1+eval[2])/2);
    ell_4m_post_mul_f(matAf, matBf);
    limnObjectPartTransform(obj, partIdx, matAf);
  }

  file = airFopen(outS, stdout, "w");
  airMopAdd(mop, file, (airMopper)airFclose, airMopAlways);

  if (limnObjectWriteOFF(file, obj)) {
    airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
