ADD_LIBRARY(dye
  convertDye.c
  methodsDye.c
)

TARGET_LINK_LIBRARIES(dye ell biff air)  
INSTALL_TARGETS(/lib dye)
