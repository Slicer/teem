# This variable will help provide a master list of all the sources.
# Add new source files here.
set(PULL_SOURCES
  actionPull.c
  binningPull.c
  constraints.c
  contextPull.c
  parmPull.c
  initPull.c
  corePull.c
  defaultsPull.c
  energy.c
  infoPull.c
  pointPull.c
  privatePull.h
  pull.h
  taskPull.c
  volumePull.c
  popcntl.c
  ccPull.c
  enumsPull.c
  trace.c
  )

target_sources(teem PRIVATE ${PULL_SOURCES})
# This will group all the source files in the VS project by directory
source_group( pull FILES ${PULL_SOURCES} )
