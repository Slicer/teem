# This variable will help provide a master list of all the sources.
# Add new source files here.
set(MOSS_SOURCES
  defaultsMoss.c
  hestMoss.c
  methodsMoss.c
  moss.h
  privateMoss.h
  sampler.c
  xform.c
  )

target_sources(teem PRIVATE ${MOSS_SOURCES})
# This will group all the source files in the VS project by directory
source_group( moss FILES ${MOSS_SOURCES} )
