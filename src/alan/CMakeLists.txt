ADD_LIBRARY(alan
coreAlan.c
enumsAlan.c
methodsAlan.c
)


INCLUDE_DIRECTORIES(${TEEM_SOURCE_DIR}/include/ ${TEEM_BINARY_DIR}/include/)

TARGET_LINK_LIBRARIES(alan air biff ell nrrd)
INSTALL_TARGETS(/lib alan)
INSTALL_FILES(/include/teem ".h" alan.h)
