ADD_LIBRARY(gage
  ctx.c
  defaultsGage.c
  filter.c
  kind.c
  miscGage.c
  print.c
  pvl.c
  scl.c
  sclanswer.c
  sclfilter.c
  sclprint.c
  shape.c
  st.c
  update.c
  vecGage.c
  vecprint.c
)

INCLUDE_DIRECTORIES(${TEEM_SOURCE_DIR}/include/ ${TEEM_BINARY_DIR}/include/)

TARGET_LINK_LIBRARIES(gage ell nrrd biff air)
