_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    clip.c defaultsBane.c gkmsFlotsam.c gkmsHvol.c gkmsInfo.c gkmsMite.c gkmsOpac.c
    gkmsPvg.c gkmsScat.c gkmsTxf.c hvol.c inc.c measr.c methodsBane.c rangeBane.c
    scat.c trex.c trnsf.c valid.c
    # private header
    privateBane.h
  PUBLIC_HEADERS
    bane.h
)
