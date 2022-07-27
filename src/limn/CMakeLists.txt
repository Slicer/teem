# This variable will help provide a master list of all the sources.
# Add new source files here.
# NOTE: used to be that lpu_*.c files were not listed here, since the CMake
# build didn't create the "lpu" utility.  However, with the July 2022 CFFI
# work, had to ensure that libteem was consistent with headers
set(LIMN_SOURCES
  cam.c
  defaultsLimn.c
  enumsLimn.c
  envmap.c
  hestLimn.c
  io.c
  light.c
  limn.h
  methodsLimn.c
  obj.c
  polydata.c
  polyfilter.c
  polymod.c
  polyshapes.c
  privateLimn.h
  qn.c
  renderLimn.c
  shapes.c
  splineEval.c
  splineMethods.c
  splineMisc.c
  splineFit.c
  transform.c
  lpuFlotsam.c
  lpu_about.c
  lpu_ccfind.c
  lpu_psel.c
  lpu_rast.c
  lpu_verts.c
  lpu_meas.c
  lpu_sort.c
  lpu_cbfit.c
  )

target_sources(teem PRIVATE ${LIMN_SOURCES})
# This will group all the source files in the VS project by directory
source_group( limn FILES ${LIMN_SOURCES} )
