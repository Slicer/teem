# This variable will help provide a master list of all the sources.
# Add new source files here.
set(GAGE_SOURCES
  ctx.c
  deconvolve.c
  defaultsGage.c
  filter.c
  gage.h
  kind.c
  miscGage.c
  print.c
  privateGage.h
  pvl.c
  scl.c
  sclanswer.c
  sclfilter.c
  sclprint.c
  shape.c
  st.c
  stack.c
  stackBlur.c
  update.c
  vecGage.c
  twovecGage.c
  vecprint.c
  optimsig.c
  )

target_sources(teem PRIVATE ${GAGE_SOURCES})
# This will group all the source files in the VS project by directory
source_group( gage FILES ${GAGE_SOURCES} )
