ADD_LIBRARY(coil
  coreCoil.c
  defaultsCoil.c
  enumsCoil.c
  methodsCoil.c
  realmethods.c
  scalarCoil.c
  tensorCoil.c
)

INCLUDE_DIRECTORIES(${TEEM_SOURCE_DIR}/include/ ${TEEM_BINARY_DIR}/include/)

TARGET_LINK_LIBRARIES(coil ten gage ell nrrd biff air)
