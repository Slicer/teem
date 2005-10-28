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

TARGET_LINK_LIBRARIES(limn ell nrrd biff hest air)
