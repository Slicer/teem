_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    descend.c enumsSeek.c extract.c methodsSeek.c setSeek.c tables.c textract.c
    updateSeek.c
    # private header
    privateSeek.h
  PUBLIC_HEADERS
    seek.h
)
