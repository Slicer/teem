ADD_LIBRARY(limn
 cam.c
 contour.c
 defaultsLimn.c
 enumsLimn.c
 env.c
 hestLimn.c
 io.c
 light.c
 methodsLimn.c
 obj.c
 qn.c
 renderLimn.c
 shapes.c
 splineEval.c
 splineMethods.c
 splineMisc.c
 transform.c
)

INCLUDE_DIRECTORIES(${TEEM_SOURCE_DIR}/include/ ${TEEM_BINARY_DIR}/include/)

TARGET_LINK_LIBRARIES(limn ell nrrd biff hest air)
