ADD_LIBRARY(ell
  cubicEll.c
  eigen.c
  genmat.c
  mat.c
  miscEll.c
  quat.c
  vecEll.c
)


INCLUDE_DIRECTORIES(${TEEM_SOURCE_DIR}/include/ ${TEEM_BINARY_DIR}/include/)

TARGET_LINK_LIBRARIES(ell nrrd biff air)
