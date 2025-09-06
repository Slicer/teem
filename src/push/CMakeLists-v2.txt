_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    action.c binning.c corePush.c defaultsPush.c forces.c methodsPush.c setup.c
    # private header
    privatePush.h
  PUBLIC_HEADERS
    push.h
)
