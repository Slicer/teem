ADD_LIBRARY(air
  754.c
  array.c
  dio.c
  endianAir.c
  enum.c
  miscAir.c
  mop.c 
  parseAir.c
  randMT.c
  sane.c
  math.c
  string.c
  threadAir.c
  )
IF(MATH_LIB)
  TARGET_LINK_LIBRARIES(air ${MATH_LIB})
ENDIF(MATH_LIB)

INSTALL_TARGETS(/lib air)
INSTALL_FILES(/include/teem/ ".h" air.h)
