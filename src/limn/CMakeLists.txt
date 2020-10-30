# This variable will help provide a master list of all the sources.
# Add new source files here.
# NOTE that the lpu_*.c files are not included (no lpu from a CMake build)
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
  )

target_sources(teem PRIVATE ${LIMN_SOURCES})
# This will group all the source files in the VS project by directory
source_group( limn FILES ${LIMN_SOURCES} )
