_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    2dTijk.c 3dTijk.c approxTijk.c enumsTijk.c fsTijk.c miscTijk.c nrrdTijk.c
    shTijk.c
    # private headers
    convertQuietPop.h convertQuietPush.h privateTijk.h shtables.h
  PUBLIC_HEADERS
    tijk.h
)
