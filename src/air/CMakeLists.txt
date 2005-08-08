ADD_LIBRARY(air
  754.c
  array.c
  dio.c
  endianAir.c
  enum.c
  miscAir.c
  mop.c 
  parseAir.c
  rand48.c
  sane.c
  string.c
  threadAir.c
)

INCLUDE_DIRECTORIES(${TEEM_SOURCE_DIR}/include/)

INSTALL_TARGETS(/lib air)
INSTALL_FILES(/include/teem/ ".h" air.h)
