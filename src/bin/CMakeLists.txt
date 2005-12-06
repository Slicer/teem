ADD_EXECUTABLE(airSanity airSanity.c)
TARGET_LINK_LIBRARIES(airSanity air)

ADD_EXECUTABLE(affine affine.c)
TARGET_LINK_LIBRARIES(affine air)

ADD_EXECUTABLE(cubic cubic.c)
TARGET_LINK_LIBRARIES(cubic ell)

ADD_EXECUTABLE(nrrdSanity nrrdSanity.c)
TARGET_LINK_LIBRARIES(nrrdSanity nrrd)

ADD_EXECUTABLE(undos undos.c)
TARGET_LINK_LIBRARIES(undos hest)

ADD_EXECUTABLE(idx2pos idx2pos.c)
TARGET_LINK_LIBRARIES(idx2pos nrrd)

ADD_EXECUTABLE(pos2idx pos2idx.c)
TARGET_LINK_LIBRARIES(pos2idx nrrd)

ADD_EXECUTABLE(overrgb overrgb.c)
TARGET_LINK_LIBRARIES(overrgb nrrd)

ADD_EXECUTABLE(qbert qbert.c)
TARGET_LINK_LIBRARIES(qbert gage)

ADD_EXECUTABLE(ilk ilk.c)
TARGET_LINK_LIBRARIES(ilk unrrdu moss)

ADD_EXECUTABLE(emap emap.c)
TARGET_LINK_LIBRARIES(emap limn)

ADD_EXECUTABLE(gkms gkms.c)
TARGET_LINK_LIBRARIES(gkms bane)

ADD_EXECUTABLE(ninspect ninspect.c)
TARGET_LINK_LIBRARIES(ninspect nrrd)


ADD_EXECUTABLE(unu unu.c)
TARGET_LINK_LIBRARIES(unu unrrdu) 

ADD_EXECUTABLE(miter miter.c)
TARGET_LINK_LIBRARIES(miter mite)


ADD_EXECUTABLE(ungantry ungantry.c)
TARGET_LINK_LIBRARIES(ungantry gage)

ADD_EXECUTABLE(talkweb talkweb.c)
TARGET_LINK_LIBRARIES(talkweb nrrd)

ADD_EXECUTABLE(tend tend.c)
TARGET_LINK_LIBRARIES(tend ten)

ADD_EXECUTABLE(mrender mrender.c)
TARGET_LINK_LIBRARIES(mrender hoover gage ten)

ADD_EXECUTABLE(vprobe vprobe.c)
TARGET_LINK_LIBRARIES(vprobe ten)

ADD_EXECUTABLE(pprobe pprobe.c)
TARGET_LINK_LIBRARIES(pprobe ten)

ADD_EXECUTABLE(spots spots.c)
TARGET_LINK_LIBRARIES(spots alan)

IF(BUILD_TESTING)
  ADD_TEST(nrrd.Sanity ${EXECUTABLE_OUTPUT_PATH}/nrrdSanity)
ENDIF(BUILD_TESTING)

