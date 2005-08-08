# This is the root Teem CMakeLists file.
CMAKE_MINIMUM_REQUIRED(VERSION 1.8)
PROJECT(TEEM)
#INCLUDE_REGULAR_EXPRESSION("^([Nn]rrd|itk|vtk|vnl|vcl|vxl|f2c|netlib|ce|itpack|DICOM|meta|png|dbh|tif|jpeg|zlib|gdcm).*$")
#SOURCE_GROUP("XML Files" REGULAR_EXPRESSION "[.]xml$")

# Don't build anything unless the version of CMake is high enough.
# The matching ELSE/ENDIF should be the last lines in the file.
IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.7)

# If CMake is the version we're expecting, don't show the
# CMAKE_BACKWARDS_COMPATIBILITY option.
IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" MATCHES "^1\\.8$")
  MARK_AS_ADVANCED(FORCE CMAKE_BACKWARDS_COMPATIBILITY)
ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" MATCHES "^1\\.8$")
  MARK_AS_ADVANCED(CLEAR CMAKE_BACKWARDS_COMPATIBILITY)
ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" MATCHES "^1\\.8$")

#-----------------------------------------------------------------------------
# TEEM version number.  An even minor number corresponds to releases.
SET(TEEM_VERSION_MAJOR "2")
SET(TEEM_VERSION_MINOR "1")
SET(TEEM_VERSION_PATCH "0")

# Version string should not include patch level.  The major.minor is
# enough to distinguish available features of the toolkit.
SET(TEEM_VERSION_STRING "${TEEM_VERSION_MAJOR}.${TEEM_VERSION_MINOR}")

#-----------------------------------------------------------------------------
# Test for some required system information.
INCLUDE (${CMAKE_ROOT}/Modules/CMakeBackwardCompatibilityC.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CMakeBackwardCompatibilityCXX.cmake)

#-----------------------------------------------------------------------------
# Output directories.
SET (LIBRARY_OUTPUT_PATH ${TEEM_BINARY_DIR}/lib CACHE INTERNAL "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${TEEM_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all executables.")
#SET (INCLUDE_OUTPUT_PATH ${TEEM_BINARY_DIR}/include CACHE INTERNAL "Single
#output directory for installing all include files.")
MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH INCLUDE_OUTPUT_PATH)
SET(TEEM_LIBRARY_PATH "${LIBRARY_OUTPUT_PATH}")
SET(TEEM_EXECUTABLE_PATH "${EXECUTABLE_OUTPUT_PATH}")
#SET(TEEM_INCLUDE_PATH "${INCLUDE_OUTPUT_PATH}")
SET(CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})


FILE(MAKE_DIRECTORY ${TEEM_BINARY_DIR}/include)
FILE(MAKE_DIRECTORY ${TEEM_BINARY_DIR}/include/teem)

#-----------------------------------------------------------------------------
# Find platform-specific differences in the handling of IEEE floating point 
# special values.
INCLUDE(${TEEM_SOURCE_DIR}/CMake/CheckBigBitfield.cmake)
CHECK_BIG_BITFIELD(BIGBITFIELD_VALUE ${TEEM_SOURCE_DIR}/CMake)
IF(BIGBITFIELD_VALUE)
   SET(BIGBITFIELD 1 CACHE INTERNAL "System handles bit-fields larger than 32 bits.")
ELSE(BIGBITFIELD_VALUE)
   SET(BIGBITFIELD 0 CACHE INTERNAL "System handles bit-fields larger than 32 bits.")
ENDIF(BIGBITFIELD_VALUE)

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
#Set Teem define for share library


#Options for thirh party stuff: zlib, png


#-----------------------------------------------------------------------------
#Include Teem Defines

#Turn on TEEM_BUILD so that the proper dll export def's are 
# used on windows builds.
ADD_DEFINITIONS(-DTEEM_BUILD=0)

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

# The BIGBITFIELD variable is configured by the root level CMakeLists.txt
IF(BIGBITFIELD)
  ADD_DEFINITIONS(-DTEEM_BIGBITFIELD=1)
ELSE(BIGBITFIELD)
  ADD_DEFINITIONS(-DTEEM_BIGBITFIELD=0)
ENDIF(BIGBITFIELD)

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


#-----------------------------------------------------------------------------
# Dispatch the build into the proper subdirectories.
SUBDIRS(src)

#---
# Include directory 
INCLUDE_DIRECTORIES(${TEEM_SOURCE_DIR}/include/)

#---
# Install directory
SET (CMAKE_INSTALL_PREFIX "${TEEM_BINARY_DIR}")

# If the version of CMake was too old, complain and build nothing.
# These should be the last lines in this file.
ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.7)
  MESSAGE(SEND_ERROR
          "This version of CMake is too old to build this version of Teem.  "
          "Please upgrade to CMake 1.8.")
ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.7)
