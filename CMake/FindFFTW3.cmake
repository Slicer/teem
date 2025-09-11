
find_path(FFTW3_INCLUDE_DIR fftw3.h
  /usr/local/include
  /usr/include
)

find_library(FFTW3_LIBRARY fftw3
  /usr/lib
  /usr/local/lib
)

set(FFTW3_FOUND FALSE)
if(FFTW3_INCLUDE_DIR AND FFTW3_LIBRARY)
    set(FFTW3_LIBRARIES ${FFTW3_LIBRARY} )
    set(FFTW3_FOUND TRUE)
endif()

mark_as_advanced(
  FFTW3_INCLUDE_DIR
  FFTW3_LIBRARIES
  FFTW3_FOUND
  )
