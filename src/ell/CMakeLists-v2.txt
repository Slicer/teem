_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    cubicEll.c eigen.c genmat.c mat.c miscEll.c quat.c vecEll.c
    # (no private header)
  PUBLIC_HEADERS
    ell.h
    ellMacros.h
)
