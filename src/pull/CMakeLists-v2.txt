_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    actionPull.c binningPull.c ccPull.c constraints.c contextPull.c corePull.c
    defaultsPull.c energy.c enumsPull.c infoPull.c initPull.c parmPull.c
    pointPull.c popcntl.c taskPull.c trace.c volumePull.c
    # private header
    privatePull.h
  PUBLIC_HEADERS
    pull.h
)
