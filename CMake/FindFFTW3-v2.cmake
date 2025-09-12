# CMake/FindFFTW3.cmake: Slightly better way of looking for FFTW3 package
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

# TODO: handle the various variants for different precisions
# See FindFFTW3-multiprec.cmake for inspiration

# NOTE: If a working FFTW3Config.cmake is found, it will define
# FFTW3::fftw3 for us already, so we don’t have to do anything else.
# This fallback only runs when FFTW3_FOUND is still FALSE.
if (NOT FFTW3_FOUND)

  # Include the helper macro for standard handling of results
  include(FindPackageHandleStandardArgs)

  # Look for the FFTW3 header. Users can help by setting FFTW3_DIR or
  # by making sure the header is somewhere in the default search paths.
  find_path(FFTW3_INCLUDE_DIR
    NAMES fftw3.h
    HINTS ENV FFTW3_DIR  # WHAT MORE hints?
    PATH_SUFFIXES include
  )

  # Look for the FFTW3 library. Allow FFTW3_DIR to point to its prefix.
  find_library(FFTW3_LIBRARY
    NAMES fftw3
    HINTS ENV FFTW3_DIR  # WHAT MORE hints?
    PATH_SUFFIXES lib
  )

  # Standard handling macro: sets FFTW3_FOUND if successful
  find_package_handle_standard_args(FFTW3
    REQUIRED_VARS FFTW3_LIBRARY FFTW3_INCLUDE_DIR
  )

  # If we did find FFTW3, but no imported target exists yet,
  # create our own IMPORTED target so downstream code can use:
  #     target_link_libraries(myprog PRIVATE FFTW3::fftw3)
  if (FFTW3_FOUND AND NOT TARGET FFTW3::fftw3)
    add_library(FFTW3::fftw3 UNKNOWN IMPORTED)
    set_target_properties(FFTW3::fftw3 PROPERTIES
      IMPORTED_LOCATION             "${FFTW3_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${FFTW3_INCLUDE_DIR}"
      # HEY no INTERFACE_LINK_LIBRARIES ?
    )
  endif()
endif()

if (FFTW3_FOUND)
  message(STATUS "Using FFTW3: ${FFTW3_LIBRARY}")
else()
  message(STATUS "FFTW3 not found")
endif()
