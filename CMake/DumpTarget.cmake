## DumpTarget.cmake: utilities for printing target properties
# Copyright (C) 2025  University of Chicago
# See ../LICENSE.txt for licensing terms

# This is the result of GLK nudging ChatGPT to explain what exactly is the state built up
# inside CMake as a result of procesing a CMakeLists.txt; among all the new Teem v2 CMake
# files this one is most experimental and for learning-by-doing.  The intent, at least,
# is to make something useful for debugging.

# print_prop(pfx prop val) describes how property `prop` has value `val`,
# (including handling NOTFOUND), prefixing each line with `pfx`
function(print_prop pfx prop val)
  # Check for NOTFOUND marker (CMake sometimes returns "<something>-NOTFOUND")
  # (a value that CMake's if() considers false!)
  if(val MATCHES "NOTFOUND$")
    message("${pfx}(${prop} not set)")
  elseif(val STREQUAL "")
    message("${pfx}${prop} = \"\"")
  #elseif(val)     # not falsey
  #  message("${pfx}${prop} = ${val}")
  else()
    set(_genx "")
    if(val MATCHES "\\$<.*>")
      set(_genx " ⬅︎  generator expression")
    endif()
    message("${pfx}${prop} = ${val}${_genx}")
  endif()
endfunction()

# dump_target(tgt) prints a bunch of stuff about the target tgt
function(dump_target tgt)
  if(NOT TARGET "${tgt}")
    message(WARNING "Target ${tgt} does not exist")
    return()
  endif()

  message("")
  message("=====[ Target NAME ${tgt} ]=====")

  # --- Basic info ---
  set(_Basic_props
      TYPE
      IMPORTED
      #CXX_STANDARD
      #C_STANDARD
      POSITION_INDEPENDENT_CODE
      COMPILE_OPTIONS # set via target_compile_options
      LINK_FLAGS   # set via target_link_options
      #RUNTIME_OUTPUT_DIRECTORY
      #LIBRARY_OUTPUT_DIRECTORY
      #ARCHIVE_OUTPUT_DIRECTORY
      # EXPORT_SET  # not set until later, so not informative during configure time
  )
  foreach(_prop IN LISTS _Basic_props)
    get_target_property(_val "${tgt}" "${_prop}")
    print_prop("  " "${_prop}" "${_val}")
  endforeach()

  # lists of property lists
  set(_Compile_props
      INCLUDE_DIRECTORIES
      INTERFACE_INCLUDE_DIRECTORIES
      COMPILE_DEFINITIONS
      INTERFACE_COMPILE_DEFINITIONS
      SOURCES
  )
  set(_Link_props
      LINK_LIBRARIES
      INTERFACE_LINK_LIBRARIES
      OUTPUT_NAME
  )
  set(_Install_props
      INSTALL_RPATH
  )

  foreach(_Group IN ITEMS Compile Link Install)
    message("  • ${_Group} properties")
    foreach(_prop IN LISTS _${_Group}_props)
      get_target_property(_val "${tgt}" "${_prop}")
      if("${_prop}" STREQUAL "SOURCES")
        # special handling of SOURCES Compile property: normalize source paths
        set(_full_srcs "")
        foreach(s IN LISTS _val)
          get_filename_component(_abs "${s}" ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}")
          list(APPEND _full_srcs "${_abs}")
        endforeach()
        string(JOIN ";" _val "${_full_srcs}")
      endif()
      print_prop("      " "${_prop}" "${_val}")
    endforeach()
  endforeach()

  message("")
endfunction()
