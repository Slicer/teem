ADD_LIBRARY(dye
  convertDye.c
  methodsDye.c
)

INCLUDE_DIRECTORIES(${TEEM_SOURCE_DIR}/include/ ${TEEM_BINARY_DIR}/include/)

TARGET_LINK_LIBRARIES(dye ell biff air)  
