# This variable will help provide a master list of all the sources.
# Add new source files here.
set(COIL_SOURCES
  coil.h
  coreCoil.c
  defaultsCoil.c
  enumsCoil.c
  methodsCoil.c
  realmethods.c
  scalarCoil.c
  tensorCoil.c
  )

target_sources(teem PRIVATE ${COIL_SOURCES})
# This will group all the source files in the VS project by directory
source_group( coil FILES ${COIL_SOURCES} )
