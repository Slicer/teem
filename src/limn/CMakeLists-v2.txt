_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    # NOTE: used to be that lpu_*.c files were not listed here, since the CMake
    # build didn't create the "lpu" utility.  However, with July 2022 Python/CFFI
    # work, had to ensure that libteem was consistent with headers
    cam.c defaultsLimn.c enumsLimn.c envmap.c hestLimn.c io.c light.c lpu_about.c
    lpu_cbfit.c lpu_ccfind.c lpu_meas.c lpu_psel.c lpu_rast.c lpu_sort.c
    lpu_verts.c lpuFlotsam.c methodsLimn.c obj.c polydata.c polyfilter.c polymod.c
    polyshapes.c qn.c renderLimn.c shapes.c splineEval.c splineFit.c
    splineMethods.c splineMisc.c transform.c
    # private header
    privateLimn.h
  PUBLIC_HEADERS
    limn.h
)
