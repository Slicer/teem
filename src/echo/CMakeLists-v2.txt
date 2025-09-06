_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    bounds.c color.c enumsEcho.c intx.c lightEcho.c list.c matter.c methodsEcho.c
    model.c objmethods.c renderEcho.c set.c sqd.c
    # private header
    privateEcho.h
  PUBLIC_HEADERS
    echo.h
)
