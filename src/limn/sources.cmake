# This variable will help provide a master list of all the sources.
# Add new source files here.
SET(LIMN_SOURCES
  defaultsLimn.c
  qn.c
  light.c
  envmap.c
  cam.c
  methodsLimn.c
  obj.c
  transform.c
  shapes.c
  renderLimn.c
  io.c
  hestLimn.c
  splineMisc.c
  splineMethods.c
  splineEval.c
  enumsLimn.c
  contour.c
  polydata.c
  )

ADD_TEEM_LIBRARY(limn ${LIMN_SOURCES})
