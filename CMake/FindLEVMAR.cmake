
find_path(LEVMAR_INCLUDE_DIR levmar.h
  /usr/local/include/levmar
  /usr/local/include
  /usr/include
)

find_library(LEVMAR_LIBRARY levmar
  /usr/lib
  /usr/local/lib
)

set(LEVMAR_FOUND FALSE)
if(LEVMAR_INCLUDE_DIR AND LEVMAR_LIBRARY)
    set(LEVMAR_INCLUDE_DIRS ${LEVMAR_INCLUDE_DIR} )
    set(LEVMAR_LIBRARIES ${LEVMAR_LIBRARY} )
    set(LEVMAR_FOUND TRUE)
endif()

mark_as_advanced(
  LEVMAR_INCLUDE_DIR
  LEVMAR_LIBRARIES
  LEVMAR_FOUND
  )
