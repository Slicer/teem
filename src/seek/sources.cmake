# This variable will help provide a master list of all the sources.
# Add new source files here.
SET(SEEK_SOURCES
  enumsSeek.c
  tables.c
  methodsSeek.c
  setSeek.c
  updateSeek.c
  extract.c
)

ADD_TEEM_LIBRARY(seek ${SEEK_SOURCES})
