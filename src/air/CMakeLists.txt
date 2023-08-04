# This variable will help provide a master list of all the sources.
# Add new source files here.
set(AIR_SOURCES
  754.c
  air.h
  array.c
  dio.c
  endianAir.c
  enum.c
  heap.c
  math.c
  miscAir.c
  mop.c
  parseAir.c
  privateAir.h
  randMT.c
  randJSF.c
  sane.c
  string.c
  threadAir.c
  )

target_sources(teem PRIVATE ${AIR_SOURCES})
# This will group all the source files in the VS project by directory
source_group( air FILES ${AIR_SOURCES} )
