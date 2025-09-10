# CMake/CheckLibM.cmake: learn if need to link with -lm for math functions
# Copyright (C) 2025  University of Chicago
# See ../LICENSE.txt for licensing terms

### Rationale
# Different unices have different ways of tacitly linking (or not) with -lm.
# In the interests of pedantic explicitness, we figure out if linking with -lm
# is needed when linking with a library that calls math functions.  Yes,
# https://cmake.org/cmake/help/latest/module/CheckLibraryExists.html could
# probably do the job here, but once I (GLK) went down the rabbit hole of
# understanding what the problem was, I wanted to create a CMake module that
# replicated the minimal example I used. So, this test builds a shared library
# `libtiny` that calls some math functions, then an executable `maintiny` that
# calls into that library. When compiling `maintiny`, we try linking:
#   1. Without -lm
#   2. With -lm (if needed)
# Then we run `maintiny` to ensure the math is mathing.
#
# Defines:
#   LIBM_NEEDED -- TRUE if executables must link with -lm
#
# Usage example
#   include(CheckLibM)
#   if(LIBM_NEEDED)
#     target_link_libraries(Teem PRIVATE m)
#   endif()
#

### ------------------------------------------------------------------------
# Setup
if(DEFINED LIBM_NEEDED)
  # not our first rodeo
  return()
endif()

set(_lmn_desc "Does the platform require linking with -lm?")
message(STATUS "CheckLibM: ${_lmn_desc}")

set(_checklibm_dir "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/tmpCheckLibM")
file(MAKE_DIRECTORY "${_checklibm_dir}")

# write `tiny.c`: single source file for libtiny, using some functions that very
# likely do *not* exist outside the standard math library: tanf and log1pf.
# Will evaluate with val=1.5703125, but not at compile time or else optimizer
# will happily pre-compute the result, thus undermining this test.
# T=log1pf(tanf(1.5703125)) ~= 7.634267208876022, so RT=(int)T should be 7
# If 7 == RT, our exit status should be unix for "all good", i.e. 0 ==> return 7 != T
file(WRITE "${_checklibm_dir}/tiny.c" "
#include <stdio.h>
#include <math.h>
int tinyFunc(double val) {
  int ret = (6 != (int)(log1pf(tanf(val))));  // should be 7 not 6
  printf(\"tinyFunc: returning %d (%s)\n\", ret, ret ? \"bad\" : \"good\");
  return ret;
}
")

# write `maintiny.c`: single source for main() that calls into libtiny
file(WRITE "${_checklibm_dir}/maintiny.c" "
extern int tinyFunc(double val);
int main(void) {
  return tinyFunc(1.5703125f);
}
")

# big helper macro to build & run test project
macro(_checklibm_try_build_and_run _suffix _extra_libs _result_var)
  set(_proj_dir "${_checklibm_dir}/${_suffix}")
  file(MAKE_DIRECTORY "${_proj_dir}")

  # write a minimal CMake project
  file(WRITE "${_proj_dir}/CMakeLists.txt" "
cmake_minimum_required(VERSION 3.13)
project(CheckLibM_${_suffix} C)

add_library(tiny SHARED \"${_checklibm_dir}/tiny.c\")
add_executable(maintiny \"${_checklibm_dir}/maintiny.c\")
target_link_libraries(maintiny PRIVATE tiny ${_extra_libs})
  ")

  try_compile(_${_suffix}_ok
    "${_proj_dir}/build"
    "${_proj_dir}"
    CheckLibM_${_suffix}
    OUTPUT_VARIABLE _${_suffix}_out
  )

  if(_${_suffix}_ok)
    # Run the resulting binary to check runtime
    set(_bin "${_proj_dir}/build/maintiny${CMAKE_EXECUTABLE_SUFFIX}")
    if(EXISTS "${_bin}")
      execute_process(
        COMMAND "${_bin}"
        RESULT_VARIABLE _${_suffix}_runres
        OUTPUT_VARIABLE _${_suffix}_runout
        ERROR_VARIABLE  _${_suffix}_runerr
      )
      if(_${_suffix}_runres EQUAL 0)
        set(${_result_var} TRUE)
      else()
        message(FATAL_ERROR
          "CheckLibM: maintiny built in ${_suffix} mode but failed at runtime.\n"
          "Exit code: ${_${_suffix}_runres}\n"
          "Stdout: ${_${_suffix}_runout}\n"
          "Stderr: ${_${_suffix}_runerr}")
      endif()
    else()
      message(FATAL_ERROR "CheckLibM: maintiny executable missing in ${_suffix} test")
    endif()
  else()
    set(${_result_var} FALSE)
  endif()
endmacro()

### ------------------------------------------------------------------------
# Now run the test.
# First try compiling + running without `-lm` ...
_checklibm_try_build_and_run(no_libm "" _checklibm_no_libm_ok)

if(_checklibm_no_libm_ok)
  # ... and either it did work without `-lm`, or ...
  set(LIBM_NEEDED FALSE CACHE BOOL ${_lmn_desc})
  message(STATUS "CheckLibM: math works without -lm")
else()
  # ... it did not work without -lm.
  # Does it does work *with* -lm?
  _checklibm_try_build_and_run(with_libm "m" _checklibm_with_libm_ok)

  if(_checklibm_with_libm_ok)
    # Yes, it does work with -lm.
    set(LIBM_NEEDED TRUE CACHE BOOL "${_lmn_desc}")
    message(STATUS "CheckLibM: yes, math requires explicit -lm")
  else()
    # Yikes, it failed both without and with -lm. Bye.
    message(FATAL_ERROR
      "CheckLibM: math test failed even with -lm. Output:\n"
      "${_checklibm_no_libm_out}\n"
      "${_checklibm_with_libm_out}")
  endif()
endif()

### ------------------------------------------------------------------------
# Cleanup
file(REMOVE_RECURSE "${_checklibm_dir}")
