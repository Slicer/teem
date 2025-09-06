_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    biffbiff.c biffmsg.c
    # private header
    privateBiff.h
  PUBLIC_HEADERS
    biff.h
)
