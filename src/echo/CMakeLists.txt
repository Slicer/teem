ADD_LIBRARY(echo
  bounds.c
  color.c
  enumsEcho.c
  intx.c
  lightEcho.c
  list.c
  matter.c
  methodsEcho.c
  model.c
  objmethods.c
  renderEcho.c
  set.c
  sqd.c 
)

TARGET_LINK_LIBRARIES(echo limn ell nrrd biff air)
INSTALL_TARGETS(/lib echo)
