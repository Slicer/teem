ADD_LIBRARY(moss
  defaultsMoss.c
  hestMoss.c
  methodsMoss.c
  sampler.c
  xform.c
)

INCLUDE_DIRECTORIES(${TEEM_SOURCE_DIR}/include/ ${TEEM_BINARY_DIR}/include/)

TARGET_LINK_LIBRARIES(moss ell nrrd biff hest air)
