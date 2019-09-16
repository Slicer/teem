# This variable will help provide a master list of all the sources.
# Add new source files here.
set(TIJK_SOURCES
  2dTijk.c
  3dTijk.c
  approxTijk.c
  enumsTijk.c
  fsTijk.c
  miscTijk.c
  nrrdTijk.c
  shTijk.c
  privateTijk.h
  convertQuietPush.h
  convertQuietPop.h
  shtables.h
  tijk.h
  )

target_sources(teem PRIVATE ${TIJK_SOURCES})
# This will group all the source files in the VS project by directory
source_group( tijk FILES ${TIJK_SOURCES} )
