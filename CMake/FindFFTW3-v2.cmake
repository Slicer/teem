## FindFFTW3.cmake: Slightly better way of looking for FFTW3 package
# Copyright (C) 2025  University of Chicago
# See ../LICENSE.txt for licensing terms

# Even though CMake already has a built-in FindFFTW3.cmake, it assumes that fftw3 was
# installed locally via CMake, but that's not what e.g. (mac) brew or (linux) apt
# actually does, so find_package(FFTW3) can succeed but is not able to define imported
# targets for use with target_link_libraries(FFTW3::fftw3) This is a long-standing issue:
# https://github.com/FFTW/fftw3/issues/130
# and there must be various solutions out there, since ChatGPT basically wrote the code
# below (and is characterically unable to provide a citation for similarly simple but
# working code online)
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

# NOTE: If a working FFTW3Config.cmake is found, it will define
# FFTW3::fftw3 for us already, so we don’t have to do anything else.
# This fallback only runs when FFTW3_FOUND is still FALSE.
if(NOT FFTW3_FOUND)

  # Look for the FFTW3 header. Users can help by setting FFTW3_DIR or
  # by making sure the header is somewhere in the default search paths.
  find_path(FFTW3_INCLUDE_DIR
    NAMES fftw3.h
    HINTS ENV FFTW3_DIR  # WHAT MORE hints?
    PATH_SUFFIXES include
  )
  _status("find_path result: FFTW3_INCLUDE_DIR='${FFTW3_INCLUDE_DIR}'")

  # Look for the FFTW3 library. Allow FFTW3_DIR to point to its prefix.
  find_library(FFTW3_LIBRARY
    NAMES fftw3
    HINTS ENV FFTW3_DIR  # WHAT MORE hints?
    PATH_SUFFIXES lib
  )
  _status("find_library result: FFTW3_LIBRARY='${FFTW3_LIBRARY}'")

  # Include the helper macro for standard handling of results
  include(FindPackageHandleStandardArgs)
  # Sets FFTW3_FOUND if successful
  _check_start("find_package_handle_standard_args(FFTW3)")
  find_package_handle_standard_args(FFTW3
    REQUIRED_VARS FFTW3_LIBRARY FFTW3_INCLUDE_DIR
  )

  if (FFTW3_FOUND)
    # If we did find FFTW3, but no imported target exists yet,
    # create our own IMPORTED target so downstream code can use:
    #     target_link_libraries(myprog PRIVATE FFTW3::fftw3)
    if (NOT TARGET FFTW3::fftw3)
      add_library(FFTW3::fftw3 UNKNOWN IMPORTED)
      set_target_properties(FFTW3::fftw3 PROPERTIES
        IMPORTED_LOCATION             "${FFTW3_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FFTW3_INCLUDE_DIR}"
        # HEY no INTERFACE_LINK_LIBRARIES ?
      )
    endif()
    _check_pass("Found: ${_dep}_INCLUDE_DIR=${${_dep}_INCLUDE_DIR} ${_dep}_LIBRARY=${${_dep}_LIBRARY}")
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
