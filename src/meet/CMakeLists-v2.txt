_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    enumall.c meetGage.c meetNrrd.c meetPull.c
    # (no private header)
  PUBLIC_HEADERS
    meet.h
)
