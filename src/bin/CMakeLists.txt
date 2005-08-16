INCLUDE_DIRECTORIES(
    ${TEEM_BINARY_DIR}/include/
)

ADD_EXECUTABLE(airSanity airSanity.c)
TARGET_LINK_LIBRARIES(airSanity air m)

ADD_EXECUTABLE(affine affine.c)
TARGET_LINK_LIBRARIES(affine air m)

ADD_EXECUTABLE(cubic cubic.c)
TARGET_LINK_LIBRARIES(cubic ell m)

ADD_EXECUTABLE(nrrdSanity nrrdSanity.c)
TARGET_LINK_LIBRARIES(nrrdSanity nrrd biff m)

ADD_EXECUTABLE(undos undos.c)
TARGET_LINK_LIBRARIES(undos hest air m)

ADD_EXECUTABLE(idx2pos idx2pos.c)
TARGET_LINK_LIBRARIES(idx2pos nrrd hest air m)

ADD_EXECUTABLE(pos2idx pos2idx.c)
TARGET_LINK_LIBRARIES(pos2idx nrrd hest air m)

ADD_EXECUTABLE(overrgb overrgb.c)
TARGET_LINK_LIBRARIES(overrgb nrrd hest biff air m)

ADD_EXECUTABLE(qbert qbert.c)
TARGET_LINK_LIBRARIES(qbert bane gage nrrd hest air m)

ADD_EXECUTABLE(ilk ilk.c)
TARGET_LINK_LIBRARIES(ilk moss unrrdu nrrd ell biff hest air m)

ADD_EXECUTABLE(emap emap.c)
TARGET_LINK_LIBRARIES(emap limn nrrd ell biff hest air m)

ADD_EXECUTABLE(gkms gkms.c)
TARGET_LINK_LIBRARIES(gkms bane nrrd biff air m)

ADD_EXECUTABLE(ninspect ninspect.c)
TARGET_LINK_LIBRARIES(ninspect nrrd biff hest air m)


ADD_EXECUTABLE(unu unu.c)
TARGET_LINK_LIBRARIES(unu unrrdu nrrd biff hest air m) 

ADD_EXECUTABLE(miter miter.c)
TARGET_LINK_LIBRARIES(miter mite hoover limn nrrd ell biff air m)


ADD_EXECUTABLE(ungantry ungantry.c)
TARGET_LINK_LIBRARIES(ungantry gage nrrd biff hest air m)

ADD_EXECUTABLE(talkweb talkweb.c)
TARGET_LINK_LIBRARIES(talkweb nrrd biff hest air m)

ADD_EXECUTABLE(tend tend.c)
TARGET_LINK_LIBRARIES(tend ten limn gage dye unrrdu nrrd ell biff air m)

ADD_EXECUTABLE(mrender mrender.c)
TARGET_LINK_LIBRARIES(mrender ten hoover limn gage nrrd ell biff hest air m)

ADD_EXECUTABLE(vprobe vprobe.c)
TARGET_LINK_LIBRARIES(vprobe ten limn gage nrrd ell biff hest air m)

ADD_EXECUTABLE(pprobe pprobe.c)
TARGET_LINK_LIBRARIES(pprobe ten limn gage nrrd ell biff hest air m)

ADD_EXECUTABLE(spots spots.c)
TARGET_LINK_LIBRARIES(spots alan nrrd ell biff hest air m)
