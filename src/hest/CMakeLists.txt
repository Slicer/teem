# This variable will help provide a master list of all the sources.
# Add new source files here.
set(HEST_SOURCES
  defaultsHest.c
  hest.h
  methodsHest.c
  parseHest.c
  privateHest.h
  usage.c
  adders.c
  )

target_sources(teem PRIVATE ${HEST_SOURCES})
# This will group all the source files in the VS project by directory
source_group( hest FILES ${HEST_SOURCES} )
