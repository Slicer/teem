# This variable will help provide a master list of all the sources.
# Add new source files here.
set(ECHO_SOURCES
  bounds.c
  color.c
  echo.h
  enumsEcho.c
  intx.c
  lightEcho.c
  list.c
  matter.c
  methodsEcho.c
  model.c
  objmethods.c
  privateEcho.h
  renderEcho.c
  set.c
  sqd.c
  )

target_sources(teem PRIVATE ${ECHO_SOURCES})
# This will group all the source files in the VS project by directory
source_group( echo FILES ${ECHO_SOURCES} )
