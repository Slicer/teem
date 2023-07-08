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

#include "meet.h"

const int meetPresent = 42;

const char *const meetBiffKey = "meet";

typedef union {
  const airEnum ***enm;
  void **v;
} foobarUnion;

/* clang-format off */  /* for the command below */
/*
******** meetAirEnumAll
**
** ALLOCATES and returns a NULL-terminated array of pointers to all the
** airEnums in Teem
**
** It might be better if this array could be created at compile-time, but
** efforts at doing this resulted in lots of "initializer is not const" errors.
**
** NOTE: the order here reflects the library ordering of the LIBS variable in
** teem/src/GNUMakefile, which is the canonical dependency ordering of the
** libraries.  Can manually check that this really does list all the airEnums
** with: (TEEM_LIB_LIST)

grep "airEnum *" {air,hest,biff,nrrd,ell,moss,unrrdu,alan,tijk,gage,dye,bane,limn,echo,hoover,seek,ten,elf,pull,coil,push,mite}/?*.h | grep EXPORT | more

** (with the ? in "}/?*.h" to stop compiler warnings about / * inside comment)
** We could grep specifically for "const airEnum *const", but its also good to
** use this occasion to also make sure that all public airEnums are indeed
** const airEnum *const.
*/
/* clang-format on */
const airEnum ** /* Biff: nope */
meetAirEnumAll() {
  airArray *arr;
  const airEnum **enm;
  unsigned int ii;
  foobarUnion fbu;

  arr = airArrayNew((fbu.enm = &enm, fbu.v), NULL, sizeof(airEnum *), 2);

#define ADD(E)                                                                          \
  ii = airArrayLenIncr(arr, 1);                                                         \
  enm[ii] = (E)

  /* air */
  ADD(airEndian);
  ADD(airBool);
  ADD(airFPClass_ae);

  /* hest: no airEnums */

  /* biff: no airEnums */

  /* nrrd */
  ADD(nrrdFormatType);
  ADD(nrrdType);
  ADD(nrrdEncodingType);
  ADD(nrrdCenter);
  ADD(nrrdKind);
  ADD(nrrdField);
  ADD(nrrdSpace);
  ADD(nrrdSpacingStatus);
  ADD(nrrdFormatPNGsRGBIntent);
  ADD(nrrdOrientationHave);
  ADD(nrrdBoundary);
  ADD(nrrdMeasure);
  ADD(nrrdUnaryOp);
  ADD(nrrdBinaryOp);
  ADD(nrrdTernaryOp);
  ADD(nrrdFFTWPlanRigor);
  ADD(nrrdResampleNonExistent);
  ADD(nrrdMetaDataCanonicalVersion);

  /* ell */
  ADD(ell_quadratic_root);
  ADD(ell_cubic_root);

  /* unrrdu: no airEnums */

#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  /* alan */
  ADD(alanStop);
#endif

  /* moss: no airEnums */

#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  /* tijk */
  ADD(tijk_class);
#endif

  /* gage */
  ADD(gageErr);
  ADD(gageKernel);
  ADD(gageItemPackPart);
  ADD(gageScl);
  ADD(gageVec);
  ADD(gage2Vec);
  ADD(gageSigmaSampling);

  /* dye */
  ADD(dyeSpace);

#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  /* bane */
  ADD(baneGkmsMeasr);
#endif

  /* limn */
  ADD(limnSpace);
  ADD(limnPolyDataInfo);
  ADD(limnCameraPathTrack);
  ADD(limnPrimitive);
  ADD(limnSplineType);
  ADD(limnSplineInfo);

  /* echo */
  ADD(echoJitter);
  ADD(echoType);
  ADD(echoMatter);

  /* hoover */
  ADD(hooverErr);

  /* seek */
  ADD(seekType);

  /* ten */
  ADD(tenAniso);
  ADD(tenInterpType);
  ADD(tenGage);
  ADD(tenFiberType);
  ADD(tenDwiFiberType);
  ADD(tenFiberStop);
  ADD(tenFiberIntg);
  ADD(tenGlyphType);
  ADD(tenEstimate1Method);
  ADD(tenEstimate2Method);
  ADD(tenTripleType);
  ADD(tenDwiGage);

#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  /* elf: no airEnums */
#endif

  /* pull */
  ADD(pullInterType);
  ADD(pullEnergyType);
  ADD(pullInfo);
  ADD(pullSource);
  ADD(pullProp);
  ADD(pullProcessMode);
  ADD(pullTraceStop);
  ADD(pullInitMethod);
  ADD(pullCount);
  ADD(pullConstraintFail);

#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  /* coil */
  ADD(coilMethodType);
  ADD(coilKindType);

  /* push */
  ADD(pushEnergyType);
#endif

  /* mite */
  ADD(miteVal);
  ADD(miteStageOp);

  /* meet: no new airEnums of its own */

  /* NULL-terminate the list */
  ADD(NULL);
#undef ADD

  /* nix, not nuke the airArray */
  airArrayNix(arr);
  return enm;
}

void
meetAirEnumAllPrint(FILE *file) {
  const airEnum **enm, *ee;
  unsigned int ei;

  if (!file) {
    return;
  }
  enm = meetAirEnumAll();
  ei = 0;
  while ((ee = enm[ei])) {
    airEnumPrint(file, ee);
    fprintf(file, "\n");
    ei++;
  }
  free(AIR_VOIDP(enm));
  return;
}

int /* Biff: 1 */
meetAirEnumAllCheck(void) {
  static const char me[] = "meetAirEnumAllCheck";
  const airEnum **enm, *ee;
  char err[AIR_STRLEN_LARGE + 1];
  unsigned int ei;
  airArray *mop;

  mop = airMopNew();
  enm = meetAirEnumAll();
  airMopAdd(mop, (void *)enm, airFree, airMopAlways);
  ei = 0;
  while ((ee = enm[ei])) {
    /* fprintf(stderr, "!%s: %u %s\n", me, ei, ee->name); */
    if (airEnumCheck(err, ee)) {
      biffAddf(MEET, "%s: problem with enum \"%s\" (%u)", me, ee->name, ei);
      biffAddf(MEET, "%s", err); /* kind of a hack */
      airMopError(mop);
      return 1;
    }
    ei++;
  }
  airMopOkay(mop);
  return 0;
}

const char *const meetTeemLibs[] = {
  /* TEEM_LIB_LIST */
  "air",    /* */
  "hest",   /* */
  "biff",   /* */
  "nrrd",   /* */
  "ell",    /* */
  "moss",   /* */
  "unrrdu", /* */
#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  "alan", /* */
  "tijk", /* */
#endif
  "gage", /* */
  "dye",  /* */
#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  "bane", /* */
#endif
  "limn",   /* */
  "echo",   /* */
  "hoover", /* */
  "seek",   /* */
  "ten",    /* */
#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  "elf", /* */
#endif
  "pull", /* */
#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
  "coil", /* */
  "push", /* */
#endif
  "mite", /* */
  "meet", /* */
  NULL};
