# This variable will help provide a master list of all the sources.
# Add new source files here.
set(DYE_SOURCES
  convertDye.c
  dye.h
  methodsDye.c
  )

target_sources(teem PRIVATE ${DYE_SOURCES})
# This will group all the source files in the VS project by directory
source_group( dye FILES ${DYE_SOURCES} )
