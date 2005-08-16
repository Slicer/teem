ADD_LIBRARY(mite
  defaultsMite.c
  kindnot.c
  ray.c
  renderMite.c
  shade.c
  thread.c
  txf.c
  user.c
)

INCLUDE_DIRECTORIES(${TEEM_SOURCE_DIR}/include/ ${TEEM_BINARY_DIR}/include/)

TARGET_LINK_LIBRARIES(mite ten hoover limn gage ell nrrd biff air)
