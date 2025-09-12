## FindLEVMAR.cmake: Careful+verbose way of looking for a levmar install
#    ("levmar" = https://users.ics.forth.gr/~lourakis/levmar/index.html)
# Copyright (C) 2025  University of Chicago
# See ../LICENSE.txt for licensing terms

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
option(Teem_DEBUG_${_dep} "Print detailed debug about finding ${_dep}" OFF)

# Helper macros to print only when Teem_DEBUG_${_dep} is enabled
macro(_status msg)
  if(Teem_DEBUG_${_dep})
    message(STATUS "[Find${_dep}] ${msg}")
  endif()
endmacro()
macro(_check_start msg)
  if(Teem_DEBUG_${_dep})
    message(CHECK_START "[Find${_dep}] ${msg}")
  endif()
endmacro()
macro(_check_fail msg)
  if(Teem_DEBUG_${_dep})
    message(CHECK_FAIL "${msg}")
  endif()
endmacro()
macro(_check_pass msg)
  if(Teem_DEBUG_${_dep})
    message(CHECK_PASS "${msg}")
  endif()
endmacro()

# Try to find directory containing levmar.h
# https://cmake.org/cmake/help/latest/command/find_path.html
find_path(LEVMAR_INCLUDE_DIR
  NAMES levmar.h
  HINTS
    $ENV{LEVMAR_DIR}
    ${CMAKE_INSTALL_PREFIX}
    /usr
    /usr/local
  PATH_SUFFIXES
    include
    include/levmar
  DOC "Directory containing levmar.h"
)
_status("find_path result: LEVMAR_INCLUDE_DIR='${LEVMAR_INCLUDE_DIR}'")

# Learn the levmar version number from levmar.h. Versions 2.4 and prior didn't have a
# levmar.h; they had lm.h, so failing to find levmar.h is one brute-force way to enforce
# that we're looking at a version >= 2.5.  Version 2.5 has:
#     #define LM_VERSION "2.5 (December 2009)"
# and Version 2.6 has:
#     #define LM_VERSION "2.6 (November 2011)"
# To learn the version from levmar.h:
if(LEVMAR_INCLUDE_DIR AND EXISTS "${LEVMAR_INCLUDE_DIR}/levmar.h")
  file(READ "${LEVMAR_INCLUDE_DIR}/levmar.h" _levmar_header)

  # Match e.g.: #define LM_VERSION "2.6 (November 2011)"
  # https://cmake.org/cmake/help/latest/command/string.html
  # sets CMAKE_MATCH_<N> for the N'th parethesized subexpression
  string(REGEX MATCH "#define[ \t]+LM_VERSION[ \t]+\"([0-9]+\\.[0-9]+)" _match "${_levmar_header}")

  if(CMAKE_MATCH_1)
    set(LEVMAR_VERSION "${CMAKE_MATCH_1}")
    _status("From levmar.h parsed version: ${LEVMAR_VERSION}")
  endif()
endif()

# Try to find full path to liblevmar library file
find_library(LEVMAR_LIBRARY
  NAMES levmar
  HINTS
    $ENV{LEVMAR_DIR}
    ${CMAKE_INSTALL_PREFIX}
    /usr
    /usr/local
  PATH_SUFFIXES
    lib
    lib64
  DOC "Full path to the liblevmar.\{a\|so\|dylib\|lib\} file"
)
_status("find_library result: LEVMAR_LIBRARY='${LEVMAR_LIBRARY}'")

# If we found the library but levmar *may* depend in turn on LAPACK and BLAS (more levmar
# functions are available that way), so we try to find them using their built-in CMake
# modules.  It's not that CMake can detect now whether the levmar found indeed depends on
# LAPACK and BLAS; we just try to find them if we did find levmar.  And we don't if
# LAPACK or BLAS are not found, but if they are required for this levmar, then we may
# have linker error later.
if(LEVMAR_LIBRARY)
  _status("Looking for LAPACK & BLAS (may trigger Threads find)")
  # if we didn't find LEVMAR, no point in looking for LAPACK, BLAS
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
  # report to caller of find_package(LEVMAR) what version they're getting
  VERSION_VAR LEVMAR_VERSION
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
  _check_pass("Found:\n      ${_dep}_INCLUDE_DIR=${${_dep}_INCLUDE_DIR}\n      ${_dep}_LIBRARY=${${_dep}_LIBRARY}")
else()
  _check_fail("Not found")
endif()

# clean up
unset(_dep)
unset(_status)
unset(_check_start)
unset(_check_fail)
unset(_check_pass)
