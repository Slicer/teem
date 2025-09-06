_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    accessors.c apply1D.c apply2D.c arith.c arraysNrrd.c axis.c bsplKernel.c cc.c
    ccmethods.c comment.c convertNrrd.c defaultsNrrd.c deringNrrd.c encoding.c
    encodingAscii.c encodingBzip2.c encodingGzip.c encodingHex.c encodingRaw.c
    encodingZRL.c endianNrrd.c enumsNrrd.c fftNrrd.c filt.c format.c formatEPS.c
    formatNRRD.c formatPNG.c formatPNM.c formatText.c formatVTK.c gzio.c hestNrrd.c
    histogram.c iter.c kernel.c keyvalue.c map.c measure.c methodsNrrd.c
    parseNrrd.c range.c read.c reorder.c resampleContext.c resampleNrrd.c simple.c
    subset.c superset.c tmfKernel.c winKernel.c write.c
    # private header
    privateNrrd.h
  PUBLIC_HEADERS
    nrrd.h
    nrrdDefines.h
    nrrdEnums.h
    nrrdMacros.h
)
