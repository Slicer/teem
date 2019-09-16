# This variable will help provide a master list of all the sources.
# Add new source files here.
set(MEET_SOURCES
  enumall.c
  meetNrrd.c
  meetGage.c
  meetPull.c
  meet.h
  )

target_sources(teem PRIVATE ${MEET_SOURCES})
# This will group all the source files in the VS project by directory
source_group( meet FILES ${MEET_SOURCES} )
