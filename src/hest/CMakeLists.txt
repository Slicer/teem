ADD_LIBRARY(hest
  defaultsHest.c
  methodsHest.c
  parseHest.c
  usage.c
)

SET(DEPEND_LIBRARIES air)

INCLUDE_DIRECTORIES(${TEEM_BINARY_DIR}/include)

FOREACH(lib ${DEPEND_LIBRARIES})
INCLUDE_DIRECTORIES(${TEEM_SOURCE_DIR}/src/${lib}/../teem/)
ENDFOREACH(lib)

TARGET_LINK_LIBRARIES(hest ${DEPEND_LIBRARIES})

INSTALL_TARGETS(/lib hest)
INSTALL_FILES(/include/teem ".h" hest.h)
