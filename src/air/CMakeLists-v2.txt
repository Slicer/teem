_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  # NOTE: teem/src/_util/gen_biffdata.py expects both "SOURCES" and "PUBLIC_HEADERS"
  # to be on lines by themselves (modulo whitespace) as THE way to list all .c files
  SOURCES
    # list here pasted in from:
    #   svn status *.c
    #   echo *.c | fold -s -w 80 | sed -e 's/^/    /' | sed -e 's/ *$//' | pbcopy
    # and copied this into GNUmakefile $(L).Obj but with " \" terminators
    754.c array.c endianAir.c enum.c heap.c math.c miscAir.c mop.c parseAir.c
    randJSF.c randMT.c sane.c string.c threadAir.c
    # private header
    privateAir.h
  PUBLIC_HEADERS
    air.h
)
