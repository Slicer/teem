# This variable will help provide a master list of all the sources.
# Add new source files here.
set(SEEK_SOURCES
  descend.c
  enumsSeek.c
  extract.c
  methodsSeek.c
  privateSeek.h
  seek.h
  setSeek.c
  tables.c
  textract.c
  updateSeek.c
  )

target_sources(teem PRIVATE ${SEEK_SOURCES})
# This will group all the source files in the VS project by directory
source_group( seek FILES ${SEEK_SOURCES} )
