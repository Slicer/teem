#
# Teem: Tools to process and visualize scientific data and images
# Copyright (C) 2009--2022  University of Chicago
# Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
# Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# (LGPL) as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# The terms of redistributing and/or modifying this software also
# include exceptions to the LGPL that facilitate static linking.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# This is the root Teem CMakeLists file

# adapt to the available version of cmake
cmake_minimum_required(VERSION 3.13...3.15)

if(${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.15)
endif()

#-----------------------------------------------------------------------------
# Teem version number.  Must reflect the values in teem/src/air/air.h
# means the version number is 1.12.0
set(Teem_VERSION_MAJOR "1")
set(Teem_VERSION_MINOR "12")
set(Teem_VERSION_PATCH "0")

# Version string (no space in version numbers, so doesn't have to be quoted)
set(Teem_VERSION_STRING ${Teem_VERSION_MAJOR}.${Teem_VERSION_MINOR}.${Teem_VERSION_PATCH})

# DESCRIPTION was added to set a project description in CMake 3.9
project(Teem VERSION ${Teem_VERSION_STRING}
        DESCRIPTION "Teem libraries developed by Gordon Kindlmann et al."
        LANGUAGES C)

# specifying a search path for CMake modules to be loaded by the include() or find_package()
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

# Include directory
# path directory variable should always be quoted since path may contain spaces
include_directories("${Teem_BINARY_DIR}/include/" "${Teem_SOURCE_DIR}/include/")

# Turn on all of the warnings for debug builds
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
    # Exclude no-type-limits as it flags extra checks in macros vs unsigned
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-type-limits")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

# We need ansi c-flags, especially on HP
set(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_REQUIRED_FLAGS ${CMAKE_ANSI_CFLAGS})

#-----------------------------------------------------------------------------
# Test for some required system information.
# This checks size of various C type (int, long, void*, char, short, float, double),
# existence of some headers (limits.h, unistd.h, pthread.h, sys/types.h, sys/prctl.h),
# and then (why?!?) X11, gethostbyname, connect, remove, shmat, IceConnectionNumber?
# GLK wonders if this can't be pruned down to what is actually needed (certainly not X11)
include (CMakeBackwardCompatibilityC)

#-----------------------------------------------------------------------------
# set all output directories to be teem-build/bin unless defined
foreach(type LIBRARY RUNTIME ARCHIVE)
    if(DEFINED CMAKE_${type}_OUTPUT_DIRECTORY AND NOT EXISTS ${CMAKE_${type}_OUTPUT_DIRECTORY})
        # message(FATAL_ERROR "CMAKE_${type}_OUTPUT_DIRECTORY is set to a non-existing directory [${CMAKE_${type}_OUTPUT_DIRECTORY}]")
        message("CMAKE_${type}_OUTPUT_DIRECTORY is set to a non-existing directory [${CMAKE_${type}_OUTPUT_DIRECTORY}]")
        file(MAKE_DIRECTORY ${CMAKE_${type}_OUTPUT_DIRECTORY})
        message("${CMAKE_${type}_OUTPUT_DIRECTORY} has been created")
    endif()

    if(NOT DEFINED CMAKE_${type}_OUTPUT_DIRECTORY)
        set(CMAKE_${type}_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    endif()
endforeach()

# build shared libraries option
if(BUILD_SHARED_LIBS)
    set(__LIBRARY_OUTPUT_PATH__ "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
else()
    set(__LIBRARY_OUTPUT_PATH__ "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
endif()

set(__RUNTIME_OUTPUT_PATH_ "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# output directory for installing all include files
file(MAKE_DIRECTORY ${Teem_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${Teem_BINARY_DIR}/include/teem)

#-----------------------------------------------------------------------------
# Find platform-specific differences in the handling of IEEE floating point special values.
# This is needed by Teem
include(${Teem_SOURCE_DIR}/CMake/TestQnanhibit.cmake)
TEST_QNANHIBIT(QNANHIBIT_VALUE ${Teem_SOURCE_DIR}/CMake)
if(QNANHIBIT_VALUE)
    set(QNANHIBIT 1 CACHE INTERNAL "The 22nd bit of 32-bit floating-point quiet NaN.")
else()
    set(QNANHIBIT 0 CACHE INTERNAL "The 22nd bit of 32-bit floating-point quiet NaN.")
endif()
# Teem Defines
if(QNANHIBIT)
    add_definitions(-DTEEM_QNANHIBIT=1)
else()
    add_definitions(-DTEEM_QNANHIBIT=0)
endif()

#-----------------------------------------------------------------------------
# test if AIR_EXISTS macro fails
message(STATUS "Check if AIR_EXISTS works to detect IEEE754 special values")
# try_run(<runResultVar> <compileResultVar> <bindir> <srcfile>)
try_run(AIR_EXISTS_MACRO_FAILS AIR_EXISTS_TEST_COMPILES
        ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR}/CMake/TestAIR_EXISTS.c)
if (NOT AIR_EXISTS_TEST_COMPILES)
    message(FATAL_ERROR "Failed to compile TestAIR_EXISTS.c so could not test AIR_EXISTS macro")
endif()
if(AIR_EXISTS_MACRO_FAILS)
    file(APPEND ${CMAKE_BINARY_DIR}/CMakeError.log "No, AIR_EXISTS macro FAILS to detect IEEE754 special values")
    message(STATUS "No, AIR_EXISTS macro FAILS to detect IEEE754 special values")
else()
    file(APPEND ${CMAKE_BINARY_DIR}/CMakeError.log "Yes, AIR_EXISTS macro works to detect IEEE754 special values")
    message(STATUS "Yes, AIR_EXISTS macro works to detect IEEE754 special values")
endif()

#-----------------------------------------------------------------------------
# All the Teem build configuration options.
# Option 1 - BUILD_SHARED_LIBS is a global flag that causes add_library to create shared libraries if on.
option(BUILD_SHARED_LIBS "Build Teem with shared libraries." OFF)
# If present and true, this will cause all libraries to be built shared unless the library was explicitly added as a static library
set(Teem_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
if(NOT Teem_BUILD_SHARED_LIBS)
    add_definitions(-DTEEM_STATIC)
endif()
# Option 2 - if we are using ZLIB library
option(Teem_ZLIB "Build Teem with support for gzip compression." ON)
# Option 3 - if we are using PNG library
option(Teem_PNG "Build Teem with support for PNG images." ON)
# Can't build PNG without ZLIB, so force it on.
if(Teem_PNG AND NOT Teem_ZLIB)
    message(WARNING "Can't build PNG without ZLIB, so Teem_ZLIB option has been turned on.")
    set(Teem_ZLIB ON)
endif()
# Option 4 - if we are using bzip library
option(Teem_BZIP2 "Build Teem with support for bzip compression." OFF)
# Option 5 - if we are using threading libraries
option(Teem_PTHREAD "Build Teem with pthread library support." ON)
# Option 6 - Look for "levmar" library <http://www.ics.forth.gr/~lourakis/levmar/>
option(Teem_LEVMAR "Build Teem with levmar library support." OFF)
# Option 7 - Look for fftw <http://www.fftw.org/>
option(Teem_FFTW3 "Build Teem with fftw library support." OFF)
# Option 8
option(USE_DIRECTIO "Use DirectIO for Nrrd file IO.  Only valid on SGI systems." OFF)
# Option 9
option(Teem_USE_LIB_INSTALL_SUBDIR "Add a Teem-X.Y.Z directory layer to the installation tree for libraries and archives." OFF)
# Option 10 - Stand-alone programs to process hex encoding of data
option(BUILD_HEX "Build dehex and enhex" OFF)
# Option 11 - For testing
option(BUILD_TESTING "Enable this to perform testing of Teem" ON)

#-----------------------------------------------------------------------------
# Obtain these libraries if requested
# Option 2
if (Teem_ZLIB)
    # Find ZLIB
    find_package(ZLIB)
    if(ZLIB_FOUND)
        add_definitions(-DTEEM_ZLIB)
        set(Teem_ZLIB_LIB ${ZLIB_LIBRARIES})
        set(Teem_ZLIB_IPATH ${ZLIB_INCLUDE_DIR})
    else()
        # We need to set this as a cache variable, so that it will show up as being turned off in the cache.
        message("warning: Turning off Teem_ZLIB, because it wasn't found.")
        set(Teem_ZLIB OFF CACHE BOOL "Build Teem with support for gzip compression." FORCE)
    endif()
endif()
# Option 3
if (Teem_PNG)
    find_package(PNG)
    if(PNG_FOUND)
        add_definitions(-DTEEM_PNG ${PNG_DEFINITIONS})
        set(Teem_PNG_LIB ${PNG_LIBRARIES})
        set(Teem_PNG_IPATH ${PNG_INCLUDE_DIR})
    else()
        # We need to set this as a cache variable, so that it will show up as being turned off in the cache.
        message("warning: Turning off Teem_PNG, because it wasn't found.")
        set(Teem_PNG OFF CACHE BOOL "Build Teem with support for PNG images." FORCE)
    endif()
endif()
# Option 4
if(Teem_BZIP2)
    find_package(BZ2)
    if(BZ2_FOUND)
        add_definitions(-DTEEM_BZIP2)
        set(Teem_BZIP2_LIB ${BZ2_LIBRARIES})
        set(Teem_BZIP2_IPATH ${BZ2_INCLUDE_DIR})
    else()
        # We need to set this as a cache variable, so that it will show up as being turned off in the cache.
        message("warning: Turning off Teem_BZIP2, because it wasn't found.")
        set(Teem_BZIP2 OFF CACHE BOOL "Build Teem with support for bzip compression." FORCE)
    endif()
endif()
# Option 5
if(Teem_PTHREAD)
    include(FindThreads)
    if(CMAKE_USE_PTHREADS_INIT)
        add_definitions(-DTEEM_PTHREAD)
    else()
        # We need to set this as a cache variable, so that it will show up as being turned off in the cache.
        message("warning: Turning off Teem_PTHREAD, because it wasn't found.")
        set(Teem_PTHREAD OFF CACHE BOOL "Build Teem with pthread library support." FORCE)
        set(Teem_PTHREAD OFF)
    endif()
endif()
# Option 6
if(Teem_LEVMAR)
    find_package(LEVMAR)
    if(LEVMAR_FOUND)
        add_definitions(-DTEEM_LEVMAR)
        set(Teem_LEVMAR_LIB ${LEVMAR_LIBRARIES})
        set(Teem_LEVMAR_IPATH ${LEVMAR_INCLUDE_DIR})
    else()
        # We need to set this as a cache variable, so that it will show up as being turned off in the cache.
        message("warning: Turning off Teem_LEVMAR, because it wasn't found.")
        set(Teem_LEVMAR OFF CACHE BOOL "Build Teem with levmar library support." FORCE)
    endif()
endif()
# Option 7
if(Teem_FFTW3)
    find_package(FFTW3)
    if(FFTW3_FOUND)
        add_definitions(-DTEEM_FFTW3)
        set(Teem_FFTW3_LIB ${FFTW3_LIBRARIES})
        set(Teem_FFTW3_IPATH ${FFTW3_INCLUDE_DIR})
    else()
        # We need to set this as a cache variable, so that it will show up as being turned off in the cache.
        message("warning: Turning off Teem_FFTW3, because it wasn't found.")
        set(Teem_FFTW3 OFF CACHE BOOL "Build Teem with fftw library support." FORCE)
    endif()
endif()
# Option 8
# DirectIO is the fast way to do multi-gigabyte I/O and currently only available for SGI platforms.
if(USE_DIRECTIO)
    add_definitions(-DTEEM_DIO=1)
else()
    add_definitions(-DTEEM_DIO=0)
endif()
# Option 9
if(Teem_USE_LIB_INSTALL_SUBDIR)
    set(EXTRA_INSTALL_PATH /Teem-${Teem_VERSION_STRING})
else()
    set(EXTRA_INSTALL_PATH "")
endif()
# Option 10 - Stand-alone programs to process hex encoding of data
if(BUILD_HEX)
    add_subdirectory(src/hex)
endif()
# Option 11 - For testing
if(BUILD_TESTING)
    include(CTest)
    enable_testing()
    set(BUILDNAME "${BUILDNAME}" CACHE STRING "Name of build on the dashboard")
    mark_as_advanced(BUILDNAME)
    mark_as_advanced(TCL_TCLSH)
    add_subdirectory(Testing)
endif()

#-----------------------------------------------------------------------------
# For Visual Studio we don't care about warnings about deprecated sprintf and the like.
if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

set(BUILD_EXPERIMENTAL_LIBS OFF CACHE BOOL "Build Teem's experimental libraries")
set(BUILD_EXPERIMENTAL_APPS OFF CACHE BOOL "Build Teem's non-essential command-line tools")

# If its a Dashboard build; turn ON both BUILD_EXPERIMENTAL_APPS and BUILD_EXPERIMENTAL_LIBS
if(NOT "$ENV{DASHBOARD_TEST_FROM_CTEST}" STREQUAL "")
    # it is a dashboard build
    set(BUILD_EXPERIMENTAL_APPS ON)
    set(BUILD_EXPERIMENTAL_LIBS ON)
endif()

# set the Teem library as a list (TEEM_LIB_LIST)
set(Teem_LIBRARIES "air" "hest" "biff" "nrrd" "ell" "moss" "unrrdu" "gage" "dye" "limn" "echo" "hoover" "seek" "ten" "pull" "mite" "meet")
if(BUILD_EXPERIMENTAL_LIBS)
    # This list of the "experimental" libraries must be kept in sync with:
    # * definition of Teem_HEADER_FILES (below)
    # * teem/src/meet/meet.h
    # * teem/src/meet/enumsall.c
    # re-setting list so libraries appear in expected order
    list(APPEND Teem_LIBRARIES "alan" "tijk" "bane" "elf" "coil" "push")
    add_definitions(-DTEEM_BUILD_EXPERIMENTAL_LIBS)
endif()

if(BUILD_EXPERIMENTAL_APPS)
    add_definitions(-DTEEM_BUILD_EXPERIMENTAL_APPS)
endif()

# set the Teem headers as a list
set(Teem_HEADER_FILES
    "air/air.h"
    "hest/hest.h"
    "biff/biff.h"
    "nrrd/nrrd.h"   "nrrd/nrrdDefines.h"   "nrrd/nrrdMacros.h"  "nrrd/nrrdEnums.h"
    "ell/ell.h"     "ell/ellMacros.h"
    "moss/moss.h"
    "unrrdu/unrrdu.h"
    "gage/gage.h"
    "dye/dye.h"
    "limn/limn.h"
    "echo/echo.h"
    "hoover/hoover.h"
    "seek/seek.h"
    "ten/ten.h"     "ten/tenMacros.h"
    "pull/pull.h"
    "mite/mite.h"
    "meet/meet.h")

if(BUILD_EXPERIMENTAL_LIBS)
    list(APPEND Teem_HEADER_FILES
        "alan/alan.h"
        "tijk/tijk.h"
        "bane/bane.h"
        "elf/elf.h"
        "coil/coil.h"
        "push/push.h")
endif()

set(Teem_INSTALLED_HEADER_FILES)
foreach(header_file IN LISTS Teem_HEADER_FILES)
    # When debugging, uncomment this line
    # message(STATUS "Copy header file: ${header_file}")
    get_filename_component(file_no_path "${header_file}" NAME)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/${header_file}"
                    "${CMAKE_CURRENT_BINARY_DIR}/include/teem/${file_no_path}" COPYONLY IMMEDIATE)
    list(APPEND Teem_INSTALLED_HEADER_FILES "${CMAKE_CURRENT_BINARY_DIR}/include/teem/${file_no_path}")
endforeach()

#-----------------------------------------------------------------------------
# header that defines AIR_EXISTS_MACRO_FAILS
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMake/airExistsConf.h
                ${CMAKE_CURRENT_BINARY_DIR}/include/teem/airExistsConf.h)
list(APPEND Teem_INSTALLED_HEADER_FILES "${CMAKE_CURRENT_BINARY_DIR}/include/teem/airExistsConf.h")

#-----------------------------------------------------------------------------
# Build the Mega library
add_library(teem "")
foreach(dir IN LISTS Teem_LIBRARIES)
    add_subdirectory(src/${dir})
endforeach()

#-----------------------------------------------------------------------------
# Set up some library paths for installation.  Windows will ingore the
# RPATH stuff, and mac will ingore the INSTALL_RPATH.  Make sure for
# macs to set BUILD_WITH_INSTALL_RPATH OFF and set INSTALL_NAME_DIR.
# I belive INSTALL_NAME_DIR will be ignored on linux.
set_target_properties(teem PROPERTIES
                      BUILD_WITH_INSTALL_RPATH OFF
                      INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib
                      INSTALL_NAME_DIR ${CMAKE_INSTALL_PREFIX}/lib
                      SOVERSION 1
                      VERSION ${Teem_VERSION_STRING})

if(Teem_ZLIB_LIB)
    target_include_directories(teem PUBLIC ${Teem_ZLIB_IPATH})
    target_include_directories(teem PUBLIC ${Teem_ZLIB_DLLCONF_IPATH})
    target_link_libraries(teem ${Teem_ZLIB_LIB})
endif()
if(Teem_PNG_LIB)
    target_include_directories(teem PUBLIC ${Teem_PNG_IPATH})
    target_include_directories(teem PUBLIC ${Teem_PNG_DLLCONF_IPATH})
    target_link_libraries(teem ${Teem_PNG_LIB})
endif()
if(Teem_BZIP2_LIB)
    target_include_directories(teem PUBLIC ${Teem_BZIP2_IPATH})
    target_include_directories(teem PUBLIC ${Teem_BZIP2_DLLCONF_IPATH})
    target_link_libraries(teem ${Teem_BZIP2_LIB})
endif()
if(Teem_LEVMAR_LIB)
    target_include_directories(teem PUBLIC ${Teem_LEVMAR_IPATH})
    target_include_directories(teem PUBLIC ${Teem_LEVMAR_DLLCONF_IPATH})
    target_link_libraries(teem ${Teem_LEVMAR_LIB})
endif()
if(Teem_FFTW3_LIB)
    target_include_directories(teem PUBLIC ${Teem_FFTW3_IPATH})
    target_include_directories(teem PUBLIC ${Teem_FFTW3_DLLCONF_IPATH})
    target_link_libraries(teem ${Teem_FFTW3_LIB})
endif()
if(Teem_PTHREAD)
    target_link_libraries(teem ${CMAKE_THREAD_LIBS_INIT})
endif()
if(UNIX)
    target_link_libraries(teem -lm)
endif()

# install targets
install(TARGETS teem EXPORT teem
        RUNTIME DESTINATION bin COMPONENT RuntimeLibraries
        LIBRARY DESTINATION lib${EXTRA_INSTALL_PATH} COMPONENT RuntimeLibraries
        ARCHIVE DESTINATION lib${EXTRA_INSTALL_PATH} COMPONENT Development)

export(TARGETS teem FILE ${Teem_BINARY_DIR}/TeemLibraryDepends.cmake)

#-----------------------------------------------------------------------------
# Now compile the binaries
add_subdirectory(src/bin)

#-----------------------------------------------------------------------------
set(CFLAGS "${CMAKE_C_FLAGS}")
set(CC "${CMAKE_C_COMPILER}")

set(Teem_EXECUTABLE_DIRS ${__RUNTIME_OUTPUT_PATH_} CACHE INTERNAL "Single output directory for building all executables.")


# Teem_CV_ prefixed variables are only used inside TeemConfig.cmake.in for
# replacement during the following two CONFIGURE_FILE calls. One is for use
# from the build tree, one is for use from the install tree.


# For build tree usage

# In the build tree, TeemConfig.cmake is in Teem_BINARY_DIR. The root of the
# tree for finding include files relative to TeemConfig.cmake is "."
set(Teem_CV_CONFIG_TO_ROOT ".")
set(Teem_CV_BUILD_SETTINGS_FILE ${Teem_BINARY_DIR}/TeemBuildSettings.cmake)
set(Teem_CV_LIBRARY_DEPENDS_FILE ${Teem_BINARY_DIR}/TeemLibraryDepends.cmake)
set(Teem_CV_EXECUTABLE_DIRS ${Teem_EXECUTABLE_DIRS})
set(Teem_CV_LIBRARY_DIRS ${__LIBRARY_OUTPUT_PATH__})
set(Teem_CV_USE_FILE ${Teem_SOURCE_DIR}/CMake/TeemUse.cmake)
set(Teem_CV_INCLUDE_DIRS "${Teem_BINARY_DIR}/include")
set(Teem_CV_BUILT_LIBRARIES teem) # The libraries built by teem.  Currently we only build the mega library.

configure_file("${Teem_SOURCE_DIR}/CMake/TeemConfig.cmake.in"
                "${Teem_BINARY_DIR}/TeemConfig.cmake" @ONLY IMMEDIATE)


# For install tree usage

# In the install tree, TeemConfig.cmake is in lib/cmake/teem or lib/Teem-X.Y/cmake/teem based on the
# value of Teem_USE_LIB_INSTALL_SUBDIR. The root of the tree for finding
# include files relative to TeemConfig.cmake is therefore "../../../.." or "../../.."
if(Teem_USE_LIB_INSTALL_SUBDIR)
    set(Teem_CV_CONFIG_TO_ROOT "../../../..")
else()
    set(Teem_CV_CONFIG_TO_ROOT "../../..")
endif()

set(Teem_CV_LIBRARY_DEPENDS_FILE "\${Teem_ROOT_DIR}/lib${EXTRA_INSTALL_PATH}/cmake/teem/TeemLibraryDepends.cmake")
set(Teem_CV_EXECUTABLE_DIRS "\${Teem_ROOT_DIR}/bin")
set(Teem_CV_LIBRARY_DIRS "\${Teem_ROOT_DIR}/lib${EXTRA_INSTALL_PATH}")
set(Teem_CV_USE_FILE "\${Teem_ROOT_DIR}/lib${EXTRA_INSTALL_PATH}/TeemUse.cmake")
set(Teem_CV_INCLUDE_DIRS "\${Teem_ROOT_DIR}/include")
set(Teem_CV_BUILT_LIBRARIES teem) # single library containing all teem "libraries"

configure_file("${Teem_SOURCE_DIR}/CMake/TeemConfig.cmake.in"
                "${Teem_BINARY_DIR}/CMake/TeemConfig.cmake" @ONLY IMMEDIATE)

# install headers
install(FILES ${Teem_INSTALLED_HEADER_FILES}
        DESTINATION include/teem
        COMPONENT Development)

# make install and cpack
# ${Teem_BINARY_DIR} is the build directory like /Teem-build
set(Teem_CV_BUILD_SETTINGS_FILE "\${Teem_ROOT_DIR}/lib${EXTRA_INSTALL_PATH}/cmake/teem/TeemBuildSettings.cmake")
file(WRITE ${Teem_BINARY_DIR}/TeemBuildSettings.cmake)
install(EXPORT teem DESTINATION lib${EXTRA_INSTALL_PATH}/cmake/teem FILE TeemLibraryDepends.cmake)
install(FILES
        "${Teem_BINARY_DIR}/CMake/TeemConfig.cmake"
        "${Teem_SOURCE_DIR}/CMake/TeemUse.cmake"
        "${Teem_BINARY_DIR}/TeemBuildSettings.cmake"
        DESTINATION lib${EXTRA_INSTALL_PATH}/cmake/teem
        COMPONENT Development)
