# - Try to find LEVMAR
# Once done this will define
#  LEVMAR_FOUND - System has LEVMAR
#  LEVMAR_INCLUDE_DIRS - The LEVMAR include directories
#  LEVMAR_LIBRARIES - The libraries needed to use LEVMAR
#  LEVMAR_DEFINITIONS - Compiler switches required for using LEVMAR

find_package(PkgConfig)
pkg_check_modules(PC_LEVMAR QUIET levmar)
set(LEVMAR_DEFINITIONS ${PC_LEVMAR_CFLAGS_OTHER})

find_path(LEVMAR_INCLUDE_DIR lm.h
          HINTS ${PC_LEVMAR_INCLUDEDIR} ${PC_LEVMAR_INCLUDE_DIRS} )

find_library(LEVMAR_LIBRARY NAMES levmar liblevmar
             HINTS ${PC_LEVMAR_LIBDIR} ${PC_LEVMAR_LIBRARY_DIRS} )

set(LEVMAR_LIBRARIES ${LEVMAR_LIBRARY} )
set(LEVMAR_INCLUDE_DIRS ${LEVMAR_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LEVMAR_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LEVMAR  DEFAULT_MSG
                                  LEVMAR_LIBRARY LEVMAR_INCLUDE_DIR)

mark_as_advanced(LEVMAR_INCLUDE_DIR LEVMAR_LIBRARY )
