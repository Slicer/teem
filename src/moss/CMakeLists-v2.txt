_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    defaultsMoss.c hestMoss.c methodsMoss.c sampler.c xform.c
    # private header
    privateMoss.h
  PUBLIC_HEADERS
    moss.h
)
