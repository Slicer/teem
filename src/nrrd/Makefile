LIB = nrrd
HEADERS = nrrd.h nrrdDefines.h nrrdMacros.h nrrdAccessors.h
LIBOBJS = types.o arrays.o accessors.o io.o methods.o \
	subset.o measr.o histogram.o filt.o convert.o endian.o
#TEST_BINS = test/subtest test/iotest test/typestest
TEST_BINS = test/rsmp
BINS = 
BINLIBS = -lnrrd -lbiff -lair

include ../make/Common.mk
