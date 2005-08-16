ADD_LIBRARY(hoover
  defaultsHoover.c
  methodsHoover.c
  rays.c
  stub.c
)

INCLUDE_DIRECTORIES(${TEEM_SOURCE_DIR}/include/ ${TEEM_BINARY_DIR}/include/)

TARGET_LINK_LIBRARIES(hoover limn ell nrrd biff air)
