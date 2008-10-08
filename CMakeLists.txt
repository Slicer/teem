# This is the root Teem CMakeLists file
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# As of CMake 2.6 Policies were introduced in order to provide a mechanism for
# adding backwards compatibility one feature at a time.

# Run cmake --help-policy CMP<num> to see documentation.
IF(COMMAND cmake_policy)
  # Library paths (/path/to/libmy.so not translated to -L/path/to -lmy)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

PROJECT(TEEM)

#-----------------------------------------------------------------------------
# TEEM version number.  Must reflect the values in teem/src/air/air.h
SET(TEEM_VERSION_MAJOR "1")
SET(TEEM_VERSION_MINOR "10")
SET(TEEM_VERSION_PATCH "0")

# Version string should not include patch level.  The major.minor is
# enough to distinguish available features of the toolkit.
SET(TEEM_VERSION_STRING "${TEEM_VERSION_MAJOR}.${TEEM_VERSION_MINOR}")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

# We need ansi c-flags, especially on HP
SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
SET(CMAKE_REQUIRED_FLAGS ${CMAKE_ANSI_CFLAGS})

#-----------------------------------------------------------------------------
# Test for some required system information.
INCLUDE (CMakeBackwardCompatibilityC)

#-----------------------------------------------------------------------------
# Output directories.
SET (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all executables.")
#output directory for installing all include files.")
MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)
SET(TEEM_LIBRARY_PATH "${LIBRARY_OUTPUT_PATH}")
SET(TEEM_EXECUTABLE_PATH "${EXECUTABLE_OUTPUT_PATH}")
SET(C_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})


FILE(MAKE_DIRECTORY ${TEEM_BINARY_DIR}/include)
FILE(MAKE_DIRECTORY ${TEEM_BINARY_DIR}/include/teem)

#-----------------------------------------------------------------------------
# Find platform-specific differences in the handling of IEEE floating point 
# special values.

INCLUDE(${TEEM_SOURCE_DIR}/CMake/TestQnanhibit.cmake)
TEST_QNANHIBIT(QNANHIBIT_VALUE ${TEEM_SOURCE_DIR}/CMake)
IF(QNANHIBIT_VALUE)
  SET(QNANHIBIT 1 CACHE INTERNAL "The 22nd bit of 32-bit floating-point quiet NaN.")
ELSE(QNANHIBIT_VALUE)
  SET(QNANHIBIT 0 CACHE INTERNAL "The 22nd bit of 32-bit floating-point quiet NaN.")
ENDIF(QNANHIBIT_VALUE)


#-----------------------------------------------------------------------------
# TEEM build configuration options.
OPTION(BUILD_SHARED_LIBS "Build Teem with shared libraries." OFF)
SET(TEEM_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
IF(NOT TEEM_BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DTEEM_STATIC)
ENDIF(NOT TEEM_BUILD_SHARED_LIBS)

OPTION(TEEM_ZLIB "Build Teem with support for gzip compression." ON)
OPTION(TEEM_PNG "Build Teem with support for PNG images." ON)
OPTION(TEEM_VTK_MANGLE "Build Teem using the mangled libraries from VTK for ZLIB and PNG." OFF)
SET(TEEM_VTK_ZLIB_MANGLE_IPATH "" CACHE PATH "Location of vtk_zlib_mangle.h")
OPTION(TEEM_VTK_TOOLKITS_IPATH "Location of VTK Build directory for ZLIB and PNG includes." OFF)

# Can't build PNG without ZLIB, so force it on.
IF(TEEM_PNG AND NOT TEEM_ZLIB)
  SET(TEEM_ZLIB ON)
ENDIF(TEEM_PNG AND NOT TEEM_ZLIB)

IF(TEEM_VTK_MANGLE)
  ADD_DEFINITIONS(-DTEEM_VTK_MANGLE=1)
  INCLUDE_DIRECTORIES( ${TEEM_VTK_TOOLKITS_IPATH} )
  INCLUDE_DIRECTORIES( ${TEEM_VTK_ZLIB_MANGLE_IPATH} )
ENDIF(TEEM_VTK_MANGLE)


SET(TEEM_ZLIB_LIB "")
SET(TEEM_PNG_LIB "")
IF(TEEM_ZLIB)
  # Find ZLIB
  FIND_PACKAGE(ZLIB)
  IF(ZLIB_FOUND)
    ADD_DEFINITIONS(-DTEEM_ZLIB=1)
    SET(TEEM_ZLIB_LIB ${ZLIB_LIBRARIES})
    SET(TEEM_ZLIB_IPATH ${ZLIB_INCLUDE_DIR})
  ELSE(ZLIB_FOUND)
    # We need to set this as a cache variable, so that it will show up as
    # being turned off in the cache.
    MESSAGE(SEND_ERROR "Turning off TEEM_ZLIB, because it wasn't found.")
    SET(TEEM_ZLIB OFF CACHE BOOL "Build Teem with support for gzip compression." FORCE)
  ENDIF(ZLIB_FOUND)

  IF(TEEM_PNG)
    FIND_PACKAGE(PNG)
    IF(PNG_FOUND)
      ADD_DEFINITIONS(-DTEEM_PNG=1 ${PNG_DEFINITIONS})
      SET(TEEM_PNG_LIB ${PNG_LIBRARIES})
      SET(TEEM_PNG_IPATH ${PNG_INCLUDE_DIR})
    ELSE(PNG_FOUND)
      # We need to set this as a cache variable, so that it will show up as
      # being turned off in the cache.
      MESSAGE(SEND_ERROR "Turning off TEEM_PNG, because it wasn't found.")
      SET(TEEM_PNG OFF CACHE BOOL "Build Teem with support for PNG images." FORCE)
    ENDIF(PNG_FOUND)
  ENDIF(TEEM_PNG)
ENDIF(TEEM_ZLIB)

# Try and locate BZIP2 stuff
OPTION(TEEM_BZIP2 "Build Teem with support for bzip compression." ON)
SET(TEEM_BZIP2_LIB "")

IF(TEEM_BZIP2)
  FIND_PACKAGE(BZ2)

  IF(BZ2_FOUND)
    ADD_DEFINITIONS(-DTEEM_BZIP2=1)
    SET(TEEM_BZIP2_LIB ${BZ2_LIBRARIES})
    SET(TEEM_BZIP2_IPATH ${BZ2_INCLUDE_DIR})
  ELSE(BZ2_FOUND)
    # We need to set this as a cache variable, so that it will show up as
    # being turned off in the cache.
    MESSAGE(SEND_ERROR "Turning off TEEM_BZIP2, because it wasn't found.")
    SET(TEEM_BZIP2 OFF CACHE BOOL "Build Teem with support for bzip compression." FORCE)
  ENDIF(BZ2_FOUND)
ENDIF(TEEM_BZIP2)

# Look for threading libraries
OPTION(TEEM_PTHREAD "Build Teem with pthread library support." ON)
IF(TEEM_PTHREAD)
  INCLUDE(FindThreads)
  IF(CMAKE_USE_PTHREADS_INIT)
    ADD_DEFINITIONS(-DTEEM_PTHREAD)
  ELSE(CMAKE_USE_PTHREADS_INIT)
    # We need to set this as a cache variable, so that it will show up as
    # being turned off in the cache.
    MESSAGE(SEND_ERROR "Turning off TEEM_PTHREAD, because it wasn't found.")
    SET(TEEM_PTHREAD OFF CACHE BOOL "Build Teem with pthread library support." FORCE)
    SET(TEEM_PTHREAD OFF)
  ENDIF(CMAKE_USE_PTHREADS_INIT)
ENDIF(TEEM_PTHREAD)

#-----------------------------------------------------------------------------

IF(${CMAKE_GENERATOR} MATCHES "Visual Studio")
  # For Visual Studio we don't care about warnings about deprecated sprintf and
  # the like.
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
ENDIF(${CMAKE_GENERATOR} MATCHES "Visual Studio")

#Include Teem Defines
# Set compiler flags for endian-ness.
IF(CMAKE_WORDS_BIGENDIAN)
  ADD_DEFINITIONS(-DTEEM_ENDIAN=4321)
ELSE(CMAKE_WORDS_BIGENDIAN)
  ADD_DEFINITIONS(-DTEEM_ENDIAN=1234)
ENDIF(CMAKE_WORDS_BIGENDIAN)

# Set compiler flags for 32 or 64 bit architecture (based on the size
# of a void pointer).
IF(CMAKE_SIZEOF_VOID_P MATCHES  8)
  ADD_DEFINITIONS(-DTEEM_32BIT=0)
ELSE(CMAKE_SIZEOF_VOID_P MATCHES 8)
  ADD_DEFINITIONS(-DTEEM_32BIT=1)
ENDIF(CMAKE_SIZEOF_VOID_P MATCHES 8)

#The QNANHIBIT variable is configured by the root level CMakeLists.txt
IF(QNANHIBIT)
  ADD_DEFINITIONS(-DTEEM_QNANHIBIT=1)
ELSE(QNANHIBIT)
  ADD_DEFINITIONS(-DTEEM_QNANHIBIT=0)
ENDIF(QNANHIBIT)

#DirectIO is the fast way to do multi-gigabyte I/O and currently only available
#for SGI platforms.  Use of DirectIO is enabled manually for now.
#OPTION(USE_DIRECTIO "Use DirectIO for Nrrd file IO.  Only valid on SGI systems." 0)
#MARK_AS_ADVANCED(USE_DIRECTIO)
#IF(USE_DIRECTIO)
#  ADD_DEFINITIONS(-DTEEM_DIO=1)
#ELSE(USE_DIRECTIO)
ADD_DEFINITIONS(-DTEEM_DIO=0)
#ENDIF(USE_DIRECTIO)

SET(BUILD_EXPERIMENTAL OFF CACHE BOOL "Build experimental components of teem")

SET(TEEM_LIBRARIES air hest biff nrrd ell unrrdu moss gage bane
 limn seek hoover echo ten dye mite)
IF(BUILD_EXPERIMENTAL)
  SET(TEEM_LIBRARIES ${TEEM_LIBRARIES} coil alan pull)
ENDIF(BUILD_EXPERIMENTAL)

SET(TEEM_HEADER_FILES
  air/air.h
  hest/hest.h
  biff/biff.h
  nrrd/nrrd.h
  nrrd/nrrdDefines.h
  nrrd/nrrdMacros.h
  nrrd/nrrdEnums.h
  ell/ell.h
  ell/ellMacros.h
  unrrdu/unrrdu.h
  moss/moss.h
  gage/gage.h
  bane/bane.h
  limn/limn.h
  seek/seek.h
  hoover/hoover.h
  echo/echo.h
  ten/ten.h
  ten/tenMacros.h
  dye/dye.h
  mite/mite.h
  )
IF(BUILD_EXPERIMENTAL)
  SET(TEEM_HEADER_FILES ${TEEM_HEADER_FILES}
    alan/alan.h
    coil/coil.h
    pull/pull.h
    )
ENDIF(BUILD_EXPERIMENTAL)
SET(TEEM_INSTALLED_HEADER_FILES)
FOREACH(header_file ${TEEM_HEADER_FILES})
  # When debugging, uncomment this line
  #MESSAGE(STATUS "Copy header file: ${header_file}")
  GET_FILENAME_COMPONENT(file_no_path "${header_file}" NAME)
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/src/${header_file}"
    "${CMAKE_CURRENT_BINARY_DIR}/include/teem/${file_no_path}" COPYONLY IMMEDIATE)
  SET(TEEM_INSTALLED_HEADER_FILES
    ${TEEM_INSTALLED_HEADER_FILES}
    "${CMAKE_CURRENT_BINARY_DIR}/include/teem/${file_no_path}")
ENDFOREACH(header_file)

#---
# Include directory 
INCLUDE_DIRECTORIES(
  "${TEEM_BINARY_DIR}/include/"
  "${TEEM_SOURCE_DIR}/include/"
  )


## Add external library path includes
IF(TEEM_ZLIB)
  INCLUDE_DIRECTORIES(${TEEM_ZLIB_IPATH})
  INCLUDE_DIRECTORIES(${TEEM_ZLIB_DLLCONF_IPATH})
  IF(TEEM_PNG)
    INCLUDE_DIRECTORIES(${TEEM_PNG_IPATH})
    INCLUDE_DIRECTORIES(${TEEM_PNG_DLLCONF_IPATH})
  ENDIF(TEEM_PNG)
ENDIF(TEEM_ZLIB)

IF(TEEM_BZIP2)
  INCLUDE_DIRECTORIES(${TEEM_BZIP2_IPATH})
ENDIF(TEEM_BZIP2)

# All the source files
SET(TEEM_SOURCES)

#-----------------------------------------------------------------------------
# Macro for adding the current set of source files to the master list
MACRO(ADD_TEEM_LIBRARY dir)
  SET(srcs)
  FOREACH(src ${ARGN})
    SET(srcs ${srcs} "src/${dir}/${src}")
  ENDFOREACH(src ${ARGN})
  SET(TEEM_SOURCES ${TEEM_SOURCES} ${srcs})
ENDMACRO(ADD_TEEM_LIBRARY name)


#-----------------------------------------------------------------------------
# Dispatch the build into the proper subdirectories.
FOREACH(dir ${TEEM_LIBRARIES})
  INCLUDE(src/${dir}/sources.cmake)
ENDFOREACH(dir ${TEEM_LIBRARIES})

#-----------------------------------------------------------------------------
# Build the Mega library
#
# This needs to occur after the parsing of the subdirectories.

# Create the library target
ADD_LIBRARY(teem ${TEEM_SOURCES})

# Set up some library paths for installation.  Windows will ingore the
# RPATH stuff, and mac will ingore the INSTALL_RPATH.  Make sure for
# macs to set BUILD_WITH_INSTALL_RPATH OFF and set INSTALL_NAME_DIR.
# I belive INSTALL_NAME_DIR will be ignored on linux.
SET_TARGET_PROPERTIES(teem PROPERTIES
  BUILD_WITH_INSTALL_RPATH OFF
  INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib
  INSTALL_NAME_DIR ${CMAKE_INSTALL_PREFIX}/lib
  )

IF(TEEM_BZIP2_LIB)
  TARGET_LINK_LIBRARIES(teem ${TEEM_BZIP2_LIB})
ENDIF(TEEM_BZIP2_LIB)
IF(TEEM_ZLIB_LIB)
  TARGET_LINK_LIBRARIES(teem ${TEEM_ZLIB_LIB})
  IF(TEEM_PNG_LIB)
    TARGET_LINK_LIBRARIES(teem ${TEEM_PNG_LIB})
  ENDIF(TEEM_PNG_LIB)
ENDIF(TEEM_ZLIB_LIB)

IF(TEEM_PTHREAD)
  TARGET_LINK_LIBRARIES(teem ${CMAKE_THREAD_LIBS_INIT})
ENDIF(TEEM_PTHREAD)

IF(UNIX)
  TARGET_LINK_LIBRARIES(teem -lm)
ENDIF(UNIX)

INSTALL(TARGETS teem
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  )

# Now compile the binaries
ADD_SUBDIRECTORY(src/bin)

# I found some hex programs
OPTION(BUILD_HEX "Build hehex and enhex" OFF)
IF(BUILD_HEX)
  ADD_SUBDIRECTORY(src/hex)
ENDIF(BUILD_HEX)

#-----------------------------------------------------------------------------
# For testing
OPTION(BUILD_TESTING "Enable this to perform testing of TEEM" OFF)
IF(BUILD_TESTING)
  SET(DART_ROOT "" CACHE INTERNAL "" FORCE)
  INCLUDE(CTest)
  ENABLE_TESTING()
  MARK_AS_ADVANCED(TCL_TCLSH)
ENDIF(BUILD_TESTING)

#-----------------------------------------------------------------------------
# Help outside projects build TEEM projects.
INCLUDE(CMakeExportBuildSettings)
EXPORT_LIBRARY_DEPENDENCIES(${TEEM_BINARY_DIR}/TEEMLibraryDepends.cmake)
CMAKE_EXPORT_BUILD_SETTINGS(${TEEM_BINARY_DIR}/TEEMBuildSettings.cmake)

SET(CFLAGS "${CMAKE_C_FLAGS}")
SET(CC "${CMAKE_C_COMPILER}")

# For build tree usage
SET(TEEM_LIBRARY_DEPENDS_FILE ${TEEM_BINARY_DIR}/TEEMLibraryDepends.cmake)
SET(TEEM_EXECUTABLE_DIRS ${EXECUTABLE_OUTPUT_PATH})
SET(TEEM_LIBRARY_DIRS ${LIBRARY_OUTPUT_PATH})
SET(TEEM_USE_FILE ${TEEM_SOURCE_DIR}/CMake/TEEMUse.cmake)
SET(TEEM_INCLUDE_DIRS "${TEEM_BINARY_DIR}/include")
SET(TEEM_BUILD_SETTINGS_FILE ${TEEM_BINARY_DIR}/TEEMBuildSettings.cmake)
SET(TEEM_TOOLS "affine;idx2pos;nrrdSanity;spots;unu;airSanity;ilk;overrgb;talkweb;vprobe;cubic;miter;pos2idx;tend;emap;mrender;pprobe;undos;gkms;ninspect;qbert;ungantry;deconv")


SET(prefix "${TEEM_BINARY_DIR}")
SET(exec_prefix "${prefix}")
SET(exec_prefix_set "no")
SET(bindir "${prefix}/bin")
SET(includedir "${prefix}")
SET(libdir "${prefix}/bin")
SET(srcdir "${TEEM_SOURCE_DIR}")

CONFIGURE_FILE("${TEEM_SOURCE_DIR}/CMake/TEEMConfig.cmake.in"
  "${TEEM_BINARY_DIR}/TEEMConfig.cmake" @ONLY IMMEDIATE)

# For installed tree usage
SET(TEEM_LIBRARY_DEPENDS_FILE ${CMAKE_INSTALL_PREFIX}/lib/TEEM-${TEEM_VERSION_STRING}/TEEMLibraryDepends.cmake)
SET(TEEM_EXECUTABLE_DIRS "${CMAKE_INSTALL_PREFIX}/bin")
SET(TEEM_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/lib")
SET(TEEM_USE_FILE "${CMAKE_INSTALL_PREFIX}/lib/TEEM-${TEEM_VERSION_STRING}/TEEMUse.cmake")
SET(TEEM_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
SET(TEEM_BUILD_SETTINGS_FILE ${CMAKE_INSTALL_PREFIX}/lib/TEEM-${TEEM_VERSION_STRING}/TEEMBuildSettings.cmake)
SET(TEEM_BUILT_LIBRARIES teem) # The libraries built by teem.  Currently we only build the mega library.
SET(prefix "${CMAKE_INSTALL_PREFIX}")
SET(exec_prefix "${prefix}")
SET(exec_prefix_set "no")
SET(bindir "${prefix}/bin")
SET(includedir "${prefix}")
SET(libdir "${prefix}/lib")
SET(srcdir ".")

CONFIGURE_FILE("${TEEM_SOURCE_DIR}/CMake/TEEMConfig.cmake.in"
  "${TEEM_BINARY_DIR}/CMake/TEEMConfig.cmake" @ONLY IMMEDIATE)

INSTALL(FILES ${TEEM_INSTALLED_HEADER_FILES}
  DESTINATION include/teem
  )
INSTALL(FILES
  "${TEEM_SOURCE_DIR}/CMake/TEEMUse.cmake"
  "${TEEM_BINARY_DIR}/TEEMBuildSettings.cmake"
  "${TEEM_BINARY_DIR}/TEEMLibraryDepends.cmake"
  "${TEEM_BINARY_DIR}/CMake/TEEMConfig.cmake"
  DESTINATION lib/TEEM-${TEEM_VERSION_STRING}
  )

