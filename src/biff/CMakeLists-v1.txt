# This variable will help provide a master list of all the sources.
# Add new source files here.
set(BIFF_SOURCES
  privateBiff.h
  biff.h
  biffmsg.c
  biffbiff.c
  )

target_sources(teem PRIVATE ${BIFF_SOURCES})
# This will group all the source files in the VS project by directory
source_group( BIFF FILES ${BIFF_SOURCES} )
