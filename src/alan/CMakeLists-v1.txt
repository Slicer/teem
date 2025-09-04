# This variable will help provide a master list of all the sources.
# Add new source files here.
set(ALAN_SOURCES
  alan.h
  coreAlan.c
  enumsAlan.c
  methodsAlan.c
  )

target_sources(teem PRIVATE ${ALAN_SOURCES})
# This will group all the source files in the VS project by directory
source_group( alan FILES ${ALAN_SOURCES} )
