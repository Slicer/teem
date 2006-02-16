# This variable will help provide a master list of all the sources.
# Add new source files here.
SET(ELL_SOURCES
  cubicEll.c
  eigen.c
  genmat.c
  mat.c
  miscEll.c
  quat.c
  vecEll.c
  )

ADD_TEEM_LIBRARY(ell ${ELL_SOURCES})
