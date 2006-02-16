# This variable will help provide a master list of all the sources.
# Add new source files here.
SET(AIR_SOURCES
  754.c
  array.c
  dio.c
  endianAir.c
  enum.c
  miscAir.c
  mop.c
  parseAir.c
  randMT.c
  sane.c
  math.c
  string.c
  threadAir.c
  )

ADD_TEEM_LIBRARY(air ${AIR_SOURCES})
