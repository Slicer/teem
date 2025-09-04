# This variable will help provide a master list of all the sources.
# Add new source files here.
set(BANE_SOURCES
  bane.h
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
  privateBane.h
  rangeBane.c
  scat.c
  trex.c
  trnsf.c
  valid.c
  )

target_sources(teem PRIVATE ${BANE_SOURCES})
# This will group all the source files in the VS project by directory
source_group( bane FILES ${BANE_SOURCES} )
