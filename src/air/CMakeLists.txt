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
  string.c
  threadAir.c
)

INSTALL_TARGETS(/lib air)
INSTALL_FILES(/include/teem/ ".h" air.h)
