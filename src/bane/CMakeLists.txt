ADD_LIBRARY(bane
  clip.c
  defaultsBane.c
  gkmsFlotsam.c
  gkmsHvol.c
  gkmsInfo.c
  gkmsMite.c
  gkmsOpac.c
  gkmsPvg.c
  gkmsScat.c
  gkmsTxf.c
  hvol.c
  inc.c
  measr.c
  methodsBane.c
  rangeBane.c
  scat.c
  trex.c
  trnsf.c
  valid.c
)

SET(DEPEND_LIBRARIES gage unrrdu nrrd biff air)

FOREACH(lib ${DEPEND_LIBRARIES})
INCLUDE_DIRECTORIES(${TEEM_SOURCE_DIR}/src/${lib})
ENDFOREACH(lib)


TARGET_LINK_LIBRARIES(bane ${DEPEND_LIBRARIES})
INSTALL_TARGETS(/lib bane)
INSTALL_FILES(/include/teem ".h" bane.h)
