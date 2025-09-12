## FindLEVMAR.cmake: Careful+verbose way of looking for a levmar install
#    ("levmar" = https://users.ics.forth.gr/~lourakis/levmar/index.html)
# Copyright (C) 2025  University of Chicago
# See ../LICENSE.txt for licensing terms

# TODO:
# how is cmake detecting of this levmar also needs lapack and blas?
# how does find_package(LAPACK/BLAS QUIET) work? is that CMake built-in?
# check hints to find_path and find_library
# make sure we're getting the right version of levmar
# BUILD_SHARED_LIBS with liblevmar.a: [ 96%] Linking C shared library libteem.dylib
#          ld: warning: ignoring duplicate libraries: '-ldl', '-lm', '/Users/gk/src/levmar-2.6/liblevmar.a'

# This module defines:
#   LEVMAR_FOUND        - True if both header and library were found
#   LEVMAR_INCLUDE_DIR  - Directory containing levmar.h
#   LEVMAR_LIBRARY      - Full path to levmar library file
#   LEVMAR_INCLUDE_DIRS - Directories to add to include path (currently just levmar)
#   LEVMAR_LIBRARIES    - Libraries to link (levmar plus lapack/blas if found)
#
# Usage:
#   find_package(LEVMAR REQUIRED)
#   target_link_libraries(myexe PRIVATE ${LEVMAR_LIBRARIES})

# how we identify ourselves
set(_dep "LEVMAR")

# Option to enable extra debug output from this module
option(Teem_${_dep}_DEBUG "Print detailed debug about finding ${_dep}" OFF)

# Helper macros to print only when Teem_${_dep}_DEBUG is enabled
macro(_status msg)
  if(Teem_${_dep}_DEBUG)
    message(STATUS "[Find${_dep}] ${msg}")
  endif()
endmacro()
macro(_check_start msg)
  if(Teem_${_dep}_DEBUG)
    message(CHECK_START "[Find${_dep}] ${msg}")
  endif()
endmacro()
macro(_check_fail msg)
  if(Teem_${_dep}_DEBUG)
    message(CHECK_FAIL "${msg}")
  endif()
endmacro()
macro(_check_pass msg)
  if(Teem_${_dep}_DEBUG)
    message(CHECK_PASS "${msg}")
  endif()
endmacro()

# Try to find directory containing levmar.h
# https://cmake.org/cmake/help/latest/command/find_path.html
find_path(LEVMAR_INCLUDE_DIR
  NAMES levmar.h
  HINTS
    ${CMAKE_INSTALL_PREFIX}/include
    /usr/include
    /usr/local/include
  PATH_SUFFIXES
    include
    levmar
  DOC "Directory containing levmar.h"
)
_status("find_path result: LEVMAR_INCLUDE_DIR='${LEVMAR_INCLUDE_DIR}'")

# Try to find full path to liblevmar library file
find_library(LEVMAR_LIBRARY
  NAMES levmar
  HINTS
    ${CMAKE_INSTALL_PREFIX}/lib
    /usr/lib
    /usr/local/lib
  PATH_SUFFIXES
    lib
  DOC "Full path to the liblevmar.\{a\|so\|dylib\|lib\} file"
)
_status("find_library result: LEVMAR_LIBRARY='${LEVMAR_LIBRARY}'")

# If we found the library but levmar depends on LAPACK and BLAS, try to find them.
# Levmar's docs advise having LAPACK/BLAS available to use the full feature set.
# (This is optional — we don't fail if LAPACK isn't present.)
if(LEVMAR_LIBRARY)   # HEY is this conditional useful?
  find_package(LAPACK QUIET)
  if(LAPACK_FOUND)
    _status("LAPACK found: ${LAPACK_LIBRARIES}")
  else()
    _status("LAPACK *not* found")
  endif()

  find_package(BLAS QUIET)
  if(BLAS_FOUND)
    _status("BLAS found: ${BLAS_LIBRARIES}")
  else()
    _status("BLAS *not* found")
  endif()
endif()

# Include the helper macro for standard handling of results
include(FindPackageHandleStandardArgs)
_check_start("find_package_handle_standard_args(LEVMAR)")
# Standard handling macro: sets LEVMAR_FOUND if successful
find_package_handle_standard_args(LEVMAR
  REQUIRED_VARS LEVMAR_INCLUDE_DIR LEVMAR_LIBRARY
)

if(LEVMAR_FOUND)
  set(LEVMAR_INCLUDE_DIRS ${LEVMAR_INCLUDE_DIR})

  set(LEVMAR_LIBRARIES ${LEVMAR_LIBRARY})
  if(LAPACK_FOUND)
    list(APPEND LEVMAR_LIBRARIES ${LAPACK_LIBRARIES})
  endif()
  if(BLAS_FOUND)
    list(APPEND LEVMAR_LIBRARIES ${BLAS_LIBRARIES})
  endif()

  # If we did find LEVMAR, but no imported target exists yet,
  # create our own IMPORTED target so downstream code can use:
  #     target_link_libraries(myprog PRIVATE LEVMAR::LEVMAR)
  if(NOT TARGET LEVMAR::LEVMAR)
    add_library(LEVMAR::LEVMAR UNKNOWN IMPORTED)
    set_target_properties(LEVMAR::LEVMAR PROPERTIES
      IMPORTED_LOCATION             "${LEVMAR_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${LEVMAR_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES      "${LEVMAR_LIBRARIES}"
    )
  endif()
  _check_pass("Found: ${_dep}_INCLUDE_DIR=${${_dep}_INCLUDE_DIR} ${_dep}_LIBRARY=${${_dep}_LIBRARY}")
else()
  _check_fail("Not found")
endif()

# clean up
unset(_dep)
unset(_status)
unset(_check_start)
unset(_check_fail)
unset(_check_pass)
