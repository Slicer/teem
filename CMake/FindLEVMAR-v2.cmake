# (from cif) suggested usage
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
#find_package(LEVMAR REQUIRED)
#
#if(LEVMAR_FOUND)
#  include_directories(${LEVMAR_INCLUDE_DIRS})
#  target_link_libraries(your_target ${LEVMAR_LIBRARIES})
#endif()

find_path(LEVMAR_INCLUDE_DIR levmar.h
  PATH_SUFFIXES levmar
)

find_library(LEVMAR_LIBRARY NAMES levmar
  PATH_SUFFIXES lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LEVMAR DEFAULT_MSG LEVMAR_LIBRARY LEVMAR_INCLUDE_DIR)

if(LEVMAR_FOUND)
  set(LEVMAR_LIBRARIES ${LEVMAR_LIBRARY})
  set(LEVMAR_INCLUDE_DIRS ${LEVMAR_INCLUDE_DIR})
endif()
