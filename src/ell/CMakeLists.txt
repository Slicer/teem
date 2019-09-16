# This variable will help provide a master list of all the sources.
# Add new source files here.
set(ELL_SOURCES
  cubicEll.c
  eigen.c
  ell.h
  ellMacros.h
  genmat.c
  mat.c
  miscEll.c
  quat.c
  vecEll.c
  )

target_sources(teem PRIVATE ${ELL_SOURCES})
# This will group all the source files in the VS project by directory
source_group( ell FILES ${ELL_SOURCES} )
