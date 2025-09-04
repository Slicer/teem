# This variable will help provide a master list of all the sources.
# Add new source files here.
set(PUSH_SOURCES
  action.c
  binning.c
  corePush.c
  defaultsPush.c
  forces.c
  methodsPush.c
  privatePush.h
  push.h
  setup.c
  )

target_sources(teem PRIVATE ${PUSH_SOURCES})
# This will group all the source files in the VS project by directory
source_group( push FILES ${PUSH_SOURCES} )
