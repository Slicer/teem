_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    coreCoil.c defaultsCoil.c enumsCoil.c methodsCoil.c realmethods.c scalarCoil.c
    tensorCoil.c
    # (no private header)
  PUBLIC_HEADERS
    coil.h
)
