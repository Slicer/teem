_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    ballStickElf.c ESHEstimElf.c glyphElf.c maximaElf.c
    # (no private header)
  PUBLIC_HEADERS
    elf.h
)
