# This variable will help provide a master list of all the sources.
# Add new source files here.
set(HOOVER_SOURCES
  defaultsHoover.c
  hoover.h
  methodsHoover.c
  rays.c
  stub.c
  )

target_sources(teem PRIVATE ${HOOVER_SOURCES})
# This will group all the source files in the VS project by directory
source_group( hoover FILES ${HOOVER_SOURCES} )
