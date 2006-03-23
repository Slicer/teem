# This variable will help provide a master list of all the sources.
# Add new source files here.
SET(GAGE_SOURCES
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
  deconvolve.c
  )

ADD_TEEM_LIBRARY(gage ${GAGE_SOURCES})
