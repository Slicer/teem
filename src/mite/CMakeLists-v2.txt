_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    defaultsMite.c kindnot.c ray.c renderMite.c shade.c thread.c txf.c user.c
    # private header
    privateMite.h
  PUBLIC_HEADERS
    mite.h
)
