# This variable will help provide a master list of all the sources.
# Add new source files here.
set(ELF_SOURCES
  ballStickElf.c
  ESHEstimElf.c
  glyphElf.c
  maximaElf.c
  elf.h
  )

target_sources(teem PRIVATE ${ELF_SOURCES})
# This will group all the source files in the VS project by directory
source_group( elf FILES ${ELF_SOURCES} )
