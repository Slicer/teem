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


function(_td_dump_dir dir indent)
  if(NOT indent)
    set(indent "")
  endif()

  message("${indent}Directory: ${dir}")

  # Targets created by add_library/add_executable/add_custom_target in this dir
  get_property(_dir_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
  if(_dir_targets)
    foreach(_t IN LISTS _dir_targets)
      message("${indent}>>>>> target: ${_t}")
      # call user-provided per-target dumper
      dump_target(${_t})
    endforeach()
  endif()

  # Imported targets added in this directory (CMake >= 3.21)
  get_property(_dir_imported DIRECTORY "${dir}" PROPERTY IMPORTED_TARGETS)
  if(_dir_imported)
    foreach(_it IN LISTS _dir_imported)
      message("${indent}>>>>> imported: ${_it}")
      dump_target(${_it})
    endforeach()
  endif()

  # Recurse into subdirectories
  get_property(_subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
  foreach(_sd IN LISTS _subdirs)
    # _sd is a directory path (absolute or relative); calling recursively is fine.
    _td_dump_dir("${_sd}" "${indent}====")
  endforeach()
endfunction()

function(dump_targets_all)
  message("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv")
  message("=============== Begin Hierarchical Target Dump ==============")

  _td_dump_dir("${CMAKE_SOURCE_DIR}" "")

  message("================ End Hierarchical Target Dump ===============")
  message("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^")

endfunction()
