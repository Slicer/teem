# NOTE: this was written entirely by ChatFPT, and it has not been used or tested.
# It suggested these as precedent:
# https://github.com/acoustid/chromaprint/blob/master/cmake/modules/FindFFTW3.cmake
# https://github.com/egpbos/findFFTW

# - Find the FFTW3 library (Fastest Fourier Transform in the West).
#
# This module tries two strategies:
#   1. First, rely on CMake’s built-in package support (works only if FFTW3
#      was itself built and installed with CMake, which is often *not* the case).
#   2. If that fails, fall back to manual search: locate headers and libraries
#      by hand and create imported targets.
#
# Provides the following IMPORTED targets on success:
#
#   FFTW3::fftw3       - double precision base library
#   FFTW3::fftw3f      - single precision base library (optional)
#   FFTW3::fftw3l      - long double precision base library (optional)
#
#   FFTW3::fftw3_threads   - threads support for double precision
#   FFTW3::fftw3f_threads  - threads support for single precision
#   FFTW3::fftw3l_threads  - threads support for long double
#
# Variables defined:
#   FFTW3_FOUND, FFTW3_INCLUDE_DIR, FFTW3_LIBRARY, etc.
#
# Example usage:
#   find_package(FFTW3 REQUIRED)
#   target_link_libraries(myexe PRIVATE FFTW3::fftw3)
#   # optionally: FFTW3::fftw3f, FFTW3::fftw3_threads, etc.
#

# -------------------------------------------------------------------------
# STEP 1: Check if CMake’s own FFTW3 package support already worked.
# If so, we don’t need to do anything else.
# -------------------------------------------------------------------------
if (NOT FFTW3_FOUND)

  include(FindPackageHandleStandardArgs)

  # -----------------------------------------------------------------------
  # STEP 2: Custom fallback — locate FFTW headers and libraries manually.
  # -----------------------------------------------------------------------

  # Look for header
  find_path(FFTW3_INCLUDE_DIR
    NAMES fftw3.h
    HINTS ENV FFTW3_DIR ENV FFTW_DIR
    PATH_SUFFIXES include
  )

  # ---- Double precision (the "default" libfftw3) ----
  find_library(FFTW3_LIBRARY
    NAMES fftw3
    HINTS ENV FFTW3_DIR ENV FFTW_DIR
    PATH_SUFFIXES lib
  )

  # ---- Single precision (libfftw3f) ----
  find_library(FFTW3F_LIBRARY
    NAMES fftw3f
    HINTS ENV FFTW3_DIR ENV FFTW_DIR
    PATH_SUFFIXES lib
  )

  # ---- Long double precision (libfftw3l) ----
  find_library(FFTW3L_LIBRARY
    NAMES fftw3l
    HINTS ENV FFTW3_DIR ENV FFTW_DIR
    PATH_SUFFIXES lib
  )

  # ---- Threaded variants (may or may not exist) ----
  find_library(FFTW3_THREADS_LIBRARY
    NAMES fftw3_threads
    HINTS ENV FFTW3_DIR ENV FFTW_DIR
    PATH_SUFFIXES lib
  )
  find_library(FFTW3F_THREADS_LIBRARY
    NAMES fftw3f_threads
    HINTS ENV FFTW3_DIR ENV FFTW_DIR
    PATH_SUFFIXES lib
  )
  find_library(FFTW3L_THREADS_LIBRARY
    NAMES fftw3l_threads
    HINTS ENV FFTW3_DIR ENV FFTW_DIR
    PATH_SUFFIXES lib
  )

  # Check at least the base double precision lib
  find_package_handle_standard_args(FFTW3
    REQUIRED_VARS FFTW3_LIBRARY FFTW3_INCLUDE_DIR
  )

  # -----------------------------------------------------------------------
  # STEP 3: Define IMPORTED targets for each library found.
  # -----------------------------------------------------------------------
  if (FFTW3_FOUND)

    # --- Double precision ---
    if (NOT TARGET FFTW3::fftw3)
      add_library(FFTW3::fftw3 UNKNOWN IMPORTED)
      set_target_properties(FFTW3::fftw3 PROPERTIES
        IMPORTED_LOCATION "${FFTW3_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FFTW3_INCLUDE_DIR}"
      )
    endif()

    # --- Single precision ---
    if (FFTW3F_LIBRARY AND NOT TARGET FFTW3::fftw3f)
      add_library(FFTW3::fftw3f UNKNOWN IMPORTED)
      set_target_properties(FFTW3::fftw3f PROPERTIES
        IMPORTED_LOCATION "${FFTW3F_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FFTW3_INCLUDE_DIR}"
      )
    endif()

    # --- Long double precision ---
    if (FFTW3L_LIBRARY AND NOT TARGET FFTW3::fftw3l)
      add_library(FFTW3::fftw3l UNKNOWN IMPORTED)
      set_target_properties(FFTW3::fftw3l PROPERTIES
        IMPORTED_LOCATION "${FFTW3L_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FFTW3_INCLUDE_DIR}"
      )
    endif()

    # --- Threads (double) ---
    if (FFTW3_THREADS_LIBRARY AND NOT TARGET FFTW3::fftw3_threads)
      add_library(FFTW3::fftw3_threads UNKNOWN IMPORTED)
      set_target_properties(FFTW3::fftw3_threads PROPERTIES
        IMPORTED_LOCATION "${FFTW3_THREADS_LIBRARY}"
        INTERFACE_LINK_LIBRARIES FFTW3::fftw3
      )
    endif()

    # --- Threads (single) ---
    if (FFTW3F_THREADS_LIBRARY AND NOT TARGET FFTW3::fftw3f_threads)
      add_library(FFTW3::fftw3f_threads UNKNOWN IMPORTED)
      set_target_properties(FFTW3::fftw3f_threads PROPERTIES
        IMPORTED_LOCATION "${FFTW3F_THREADS_LIBRARY}"
        INTERFACE_LINK_LIBRARIES FFTW3::fftw3f
      )
    endif()

    # --- Threads (long double) ---
    if (FFTW3L_THREADS_LIBRARY AND NOT TARGET FFTW3::fftw3l_threads)
      add_library(FFTW3::fftw3l_threads UNKNOWN IMPORTED)
      set_target_properties(FFTW3::fftw3l_threads PROPERTIES
        IMPORTED_LOCATION "${FFTW3L_THREADS_LIBRARY}"
        INTERFACE_LINK_LIBRARIES FFTW3::fftw3l
      )
    endif()

  endif()

endif()
