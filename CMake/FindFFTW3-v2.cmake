## FindFFTW3.cmake: Slightly better way of looking for FFTW3 package
#    ("fftw3" = http://www.fftw.org)
# Copyright (C) 2025  University of Chicago
# See ../LICENSE.txt for licensing terms

# Even though CMake already has a built-in FindFFTW3.cmake, it assumes that fftw3 was
# installed locally via CMake, but that's not what e.g. (mac) brew or (linux) apt
# actually does, so find_package(FFTW3) can succeed but is not able to define imported
# targets for use with target_link_libraries(FFTW3::fftw3) This is a long-standing issue:
# https://github.com/FFTW/fftw3/issues/130
#
# This module defines:
#   FFTW3_FOUND        - True if both header and library were found
#   FFTW3_INCLUDE_DIR  - Directory containing fftw3.h
#                        (legacy; for include_directories)
#   FFTW3_LIBRARY      - Full path to the fftw3 library file
#                        (legacy; for direct linking)
#   FFTW3_INCLUDE_DIRS - Directories to add to include path
#                        (legacy; currently just fftw3)
#   FFTW3_LIBRARIES    - Libraries to link
#                        (legacy; just fftw3, no transitive deps)
#   FFTW3_VERSION      - Optional version string for the library; not reliably
#                        parse-able from fftw3.h, so will be empty unless
#                        supplied by the user or cache.
#
# Imported target (modern CMake usage):
#   FFTW3::fftw3       - Imported target for modern target_link_libraries usage
#
# Usage:
#   find_package(FFTW3 REQUIRED)
#   target_link_libraries(myexe PRIVATE FFTW3::fftw3)
#   # or for legacy code:
#   target_include_directories(myexe PRIVATE ${FFTW3_INCLUDE_DIRS})
#   target_link_libraries(myexe PRIVATE ${FFTW3_LIBRARIES})
#
# For comparison look at:
# https://github.com/egpbos/findFFTW
# https://github.com/acoustid/chromaprint/blob/master/cmake/modules/FindFFTW3.cmake
# https://git.astron.nl/RD/EveryBeam/-/blob/v0.6.2/CMake/FindFFTW3.cmake

# TODO: handle the various variants for different precisions
# See FindFFTW3-multiprec.cmake for inspiration

# how we identify ourselves
set(_dep "FFTW3")

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

# NOTE: If a working FFTW3Config.cmake is found, it will define
# FFTW3::fftw3 for us already, so we don’t have to do anything else.
# This fallback only runs when FFTW3_FOUND is still FALSE.
if(NOT FFTW3_FOUND)

  # Find the directory containing fftw3.h. Users can help by setting FFTW3_DIR or
  # by making sure the header is somewhere in the default search paths.
  find_path(FFTW3_INCLUDE_DIR
    NAMES fftw3.h
    HINTS
      $ENV{FFTW3_DIR}
      ${CMAKE_INSTALL_PREFIX}
      /usr
      /usr/local
    PATH_SUFFIXES
      include
      include/fftw3
    DOC "Directory containing fftw3.h"
  )
  _status("find_path result: FFTW3_INCLUDE_DIR='${FFTW3_INCLUDE_DIR}'")

  # Look for the FFTW3 library. Can set FFTW3_DIR to containing directory.
  find_library(FFTW3_LIBRARY
    NAMES fftw3
    HINTS
      $ENV{FFTW3_DIR}
      ${CMAKE_INSTALL_PREFIX}
      /usr
      /usr/local
    PATH_SUFFIXES
      lib
      lib64
    DOC "Full path to the libfftw3.\{a\|so\|dylib\|lib\} file"
  )
  _status("find_library result: FFTW3_LIBRARY='${FFTW3_LIBRARY}'")

  # Optional user-provided version (since fftw3.h does not seem to declare its own
  # version in any way that can be learned at configure- or compile-time (the info is
  # available at run-time, but that's too annoying).
  if(NOT DEFINED FFTW3_VERSION)
    set(FFTW3_VERSION "" CACHE STRING "Optional: version of FFTW3 library")
  endif()

  # Include the helper macro for standard handling of results
  include(FindPackageHandleStandardArgs)
  # Sets FFTW3_FOUND if successful
  _check_start("find_package_handle_standard_args(FFTW3)")
  find_package_handle_standard_args(FFTW3
    REQUIRED_VARS FFTW3_LIBRARY FFTW3_INCLUDE_DIR
    VERSION_VAR FFTW3_VERSION
  )

  if (FFTW3_FOUND)
    # for use by old-style CMake code
    set(FFTW3_INCLUDE_DIRS ${FFTW3_INCLUDE_DIR})
    set(FFTW3_LIBRARIES ${FFTW3_LIBRARY})

    # If we did find FFTW3, but no imported target exists yet,
    # create our own IMPORTED target so downstream code can use:
    #     target_link_libraries(myprog PRIVATE FFTW3::fftw3)
    if (NOT TARGET FFTW3::fftw3)
      add_library(FFTW3::fftw3 UNKNOWN IMPORTED)
      set_target_properties(FFTW3::fftw3 PROPERTIES
        IMPORTED_LOCATION             "${FFTW3_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FFTW3_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES      "${FFTW3_LIBRARY}"  # maybe redundant?
      )
    endif()
    _check_pass("Found:\n      ${_dep}_INCLUDE_DIR=${${_dep}_INCLUDE_DIR}\n      ${_dep}_LIBRARY=${${_dep}_LIBRARY}")
  else()
    _check_fail("Not found")
  endif()
endif()

# clean up
unset(_dep)
unset(_status)
unset(_check_start)
unset(_check_fail)
unset(_check_pass)
