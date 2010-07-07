# This variable will help provide a master list of all the sources.
# Add new source files here.
SET(TIJK_SOURCES
  2dTijk.c
  3dTijk.c
  approxTijk.c
  miscTijk.c
  shTijk.c
  fsTijk.c
  privateTijk.h
  tijk.h
  )

ADD_TEEM_LIBRARY(tijk ${TIJK_SOURCES})
