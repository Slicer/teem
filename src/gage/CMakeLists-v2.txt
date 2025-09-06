_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    # NOT including multiGage.c for now, and scl3pfilterbody.c is just a macro
    ctx.c deconvolve.c defaultsGage.c filter.c kind.c miscGage.c
    optimsig.c print.c pvl.c scl.c sclanswer.c sclfilter.c
    sclprint.c shape.c st.c stack.c stackBlur.c twovecGage.c update.c vecGage.c
    vecprint.c
    # private header
    privateGage.h
  PUBLIC_HEADERS
    gage.h
)
