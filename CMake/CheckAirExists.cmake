# CMake/CheckAirExists.cmake: see if AIR_EXISTS() macro works like isfinite()
# Copyright (C) 2025  University of Chicago
# See ../LICENSE.txt for licensing terms

### Rationale
# Modern C has isfinite() to detect if a given floating point value is an IEEE754 special
# values (NaNs and infinities).  Teem development, however, started before isfinite() was
# widely available, so it uses the AIR_EXISTS() macro in air.h.  Being a macro, there are
# expressions that could do the right thing with some compiler options, but fail with
# different options (like -ffast-math). So we jump through some hoops to rigorously check
# if AIR_EXISTS() is really working, based on the little CMake/TestAIR_EXISTS.c test
# program. For this to be a useful test that informs how the rest of Teem code will work,
# TestAIR_EXISTS.c needs to be compiled the same as Teem itself will be compiled later.
# Sadly, it was not until Teem V2 that this consistency was actually enforced (!)  To
# further help debugging, we also print out exactly how TestAIR_EXISTS.c is compiled, and
# we use try_compile instead of try_run so the resulting executable file stays available
# for later inspection.
#
# Defines:
#   AIR_EXISTS_MACRO_FAILS -- TRUE if AIR_EXISTS() fails with current compiler settings


# "taex" = Test Air_EXists
set(_taex_src "${CMAKE_CURRENT_LIST_DIR}/TestAIR_EXISTS.c")

# Build flags for the probe (same as Teem itself)
string(TOUPPER "${CMAKE_BUILD_TYPE}" _taex_BTUC)
set(_taex_flags "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${_taex_BTUC}}")
set(_taex_me "AIR_EXISTS (compiled \"${_taex_flags}\")")

# Where we want a durable copy of the compiled probe
if(WIN32)
  set(_taex_copy "${CMAKE_BINARY_DIR}/TestAIR_EXISTS.exe")
else()
  set(_taex_copy "${CMAKE_BINARY_DIR}/TestAIR_EXISTS")
endif()

# Remove any stale copy from previous runs
if(EXISTS "${_taex_copy}")
  file(REMOVE "${_taex_copy}")
endif()

# Compile the probe
message(CHECK_START "Testing whether macro ${_taex_me} detects IEEE754 special values")
try_compile(
  _taex_compiles         # boolean result: TRUE if compile succeeded
  SOURCES "${_taex_src}" # one or more source files for the test
  CMAKE_FLAGS
    "-DCMAKE_VERBOSE_MAKEFILE=ON"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    "-DCMAKE_C_FLAGS_${_taex_BTUC}=${_taex_flags}"
    "-DCMAKE_DEPENDS_USE_COMPILER=FALSE"  # no -MD/-MT/-MF dependency cruft
  COPY_FILE "${_taex_copy}"     # copy the built executable to a stable path
  OUTPUT_VARIABLE _taex_compile_out
  COPY_FILE_ERROR _taex_copy_error
)
# Always show the captured compile output, as progress indication
string(REPLACE "\n" "\n     " _taex_compile_out_indented "${_taex_compile_out}")
message(STATUS "Compile output:\n     ${_taex_compile_out_indented}")

if(NOT _taex_compiles)
  message(CHECK_FAIL "compile failed")
  message(FATAL_ERROR "Could not compile ${_taex_src}")
elseif(_taex_copy_error)
  message(CHECK_FAIL "copy failed")
  message(FATAL_ERROR "Test program compiled but could not copy: ${_taex_copy_error}")
elseif(NOT EXISTS "${_taex_copy}")
  message(CHECK_FAIL "missing executable")
  message(FATAL_ERROR "Test program compiled but file not found at ${_taex_copy}")
else()
  message(CHECK_PASS "compiled successfully")
endif()

# Run the test program
message(CHECK_START "Running ${_taex_me} test program")
# https://cmake.org/cmake/help/latest/command/execute_process.html
execute_process(
  COMMAND "${_taex_copy}"
  RESULT_VARIABLE _taex_run_status
  OUTPUT_VARIABLE _taex_run_out
  ERROR_VARIABLE _taex_run_err
)

# Act on the results; set AIR_EXISTS_MACRO_FAILS
if(_taex_run_status EQUAL 0)
  message(CHECK_PASS "Yes, it detects IEEE754 special values")
  set(AIR_EXISTS_MACRO_FAILS 0 CACHE INTERNAL "AIR_EXISTS macro works correctly")
else()
    # Always show stdout/stderr from the probe as indented sub-log
  if(_taex_run_out)
    string(REPLACE "\n" "\n     " _taex_run_out_indented "${_taex_run_out}")
    message(STATUS "Probe stdout:\n     ${_taex_run_out_indented}")
  endif()
  if(_taex_run_err)
    string(REPLACE "\n" "\n     " _taex_run_err_indented "${_taex_run_err}")
    message(STATUS "Probe stderr:\n     ${_taex_run_err_indented}")
  endif()
  message(CHECK_FAIL "NO, it FAILS to detect IEEE754 special values")
  set(AIR_EXISTS_MACRO_FAILS 1 CACHE INTERNAL "AIR_EXISTS macro fails")
endif()

# (rejected but interesting debugging strategies:
#  message(STATUS "Pausing CMake so you can inspect temporary files...")
#  execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 30)
#  or
#  execute_process(COMMAND /bin/sh -c "read -p 'Press ENTER to continue...'")
# )
