# This variable will help provide a master list of all the sources.
# Add new source files here.
set(MITE_SOURCES
  defaultsMite.c
  kindnot.c
  mite.h
  privateMite.h
  ray.c
  renderMite.c
  shade.c
  thread.c
  txf.c
  user.c
  )

target_sources(teem PRIVATE ${MITE_SOURCES})
# This will group all the source files in the VS project by directory
source_group( mite FILES ${MITE_SOURCES} )
