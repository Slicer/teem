# This variable will help provide a master list of all the sources.
# Add new source files here.
SET(HEST_SOURCES
  defaultsHest.c
  methodsHest.c
  parseHest.c
  usage.c
  )

ADD_TEEM_LIBRARY(hest ${HEST_SOURCES})
