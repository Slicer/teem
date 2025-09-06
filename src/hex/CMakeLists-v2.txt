# helper function HEY copied from ../bin/CMakeLists.txt
function(teem_add_executable name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE Teem)

  install(TARGETS ${name}
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    COMPONENT Runtime
  )
endfunction()

teem_add_executable(dehex dehex.c)
teem_add_executable(enhex enhex.c)
