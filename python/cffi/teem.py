#
# Teem: Tools to process and visualize scientific data and images
# Copyright (C) 2009--2023  University of Chicago
# Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
# Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah
#
# This library is free software; you can redistribute it and/or modify it under the terms
# of the GNU Lesser General Public License (LGPL) as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any later version.
# The terms of redistributing and/or modifying this software also include exceptions to
# the LGPL that facilitate static linking.
#
# This library is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along with
# this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
# Fifth Floor, Boston, MA 02110-1301 USA
"""
teem.py: A convenience wrapper around the _teem extension module, which in turn links into
the underlying libteem.{so,dylib} shared library. The main utility of teem.py is wrapping
calls into Teem functions that use biff, so that if the function has an error, the biff error
message is converted into a Python exception.  We also introduce the Tenum object for wrapping
an airEnum, and maybe eventually other ways of making pythonic interfaces to teem
functionality.  See teem/python/cffi/README.md.

teem/python/cffi/exult.py was likely used to both compile the _teem extension module (the
shared library), and to generate this wrapper, which is the result of simple text
transformations of the template wrapper in teem/python/cffi/lliibb.py
"""

import math as _math   # # likely used in _BIFF_DICT, below, for testing function return values

# halt if python2; thanks to https://stackoverflow.com/a/65407535/1465384
_x, *_y = 1, 2  # NOTE: A SyntaxError means you need Python3, not Python2
del _x, _y


def string(bstr):
    """Convenience utility for going from C char* bytes to Python string:
    string(B) is just _teem.ffi.string(B).decode('ascii')"""
    return _teem.ffi.string(bstr).decode('ascii')


class Tenum:
    """Helper/wrapper around (pointers to) airEnums (part of Teem's "air" library).
    This provides convenient ways to convert between integer enum values and real Python
    strings. The C airEnum underlying the Python Tenum foo is still available as foo().
    """

    def __init__(self, aenm, _name):
        """Constructor takes a Teem airEnum pointer (const airEnum *const)."""
        if not str(aenm).startswith("<cdata 'airEnum *' "):
            raise TypeError(f'passed argument {aenm} does not seem to be an airEnum pointer')
        self.aenm = aenm
        self.name = string(self.aenm.name)
        self._name = _name  # the variable name for the airEnum in libteem
        # following definition of airEnum struct in air.h
        self.vals = list(range(1, self.aenm.M + 1))
        if self.aenm.val:
            self.vals = [self.aenm.val[i] for i in self.vals]

    def __call__(self):
        """Returns (a pointer to) the underlying airEnum."""
        return self.aenm

    def __iter__(self):
        """Provides a way to iterate through the valid values of the enum"""
        return iter(self.vals)

    def valid(self, ios) -> bool:  # ios = int or string
        """Answers whether given int is a valid value of enum, or whether given string
        is a valid string in enum, depending on incoming type.
        (wraps airEnumValCheck() and airEnumVal())"""
        if isinstance(ios, int):
            return not _teem.lib.airEnumValCheck(self.aenm, ios)
        if isinstance(ios, str):
            return self.unknown() != self.val(ios)
        # else
        raise TypeError(f'Need an int or str argument (not {type(ios)})')

    def str(self, val: int, picky=False) -> str:
        """Converts from integer enum value val to string identifier
        (wraps airEnumStr())"""
        assert isinstance(val, int), f'Need an int argument (not {type(val)})'
        if picky and not self.valid(val):
            raise ValueError(f'{val} not a valid {self._name} ("{self.name}") enum value')
        # else
        return string(_teem.lib.airEnumStr(self.aenm, val))

    def desc(self, val: int) -> str:
        """Converts from integer value val to description string
        (wraps airEnumDesc())"""
        assert isinstance(val, int), f'Need an int argument (not {type(val)})'
        return string(_teem.lib.airEnumDesc(self.aenm, val))

    def val(self, sss: str, picky=False) -> int:
        """Converts from string sss to integer enum value
        (wraps airEnumVal())"""
        assert isinstance(sss, str), f'Need an string argument (not {type(sss)})'
        ret = _teem.lib.airEnumVal(self.aenm, sss.encode('ascii'))
        if picky and ret == self.unknown():
            raise ValueError(f'"{sss}" not parsable as {self._name} ("{self.name}") enum value')
        # else
        return ret

    def unknown(self) -> int:
        """Returns value representing unknown
        (wraps airEnumUnknown())"""
        return _teem.lib.airEnumUnknown(self.aenm)


def _equals_one(val):   # likely used in _BIFF_DICT, below
    """Returns True iff given val equals 1"""
    return val == 1


def _equals_null(val):   # likely used in _BIFF_DICT, below
    """Returns True iff given val equals NULL"""
    return val == NULL   # NULL is set at very end of this file


_BIFF_DICT = {  # contents here are filled in by teem/python/cffi/exult.py Tffi.wrap()
    'nrrdArrayCompare': (_equals_one, 0, b'nrrd', 'nrrd/accessors.c:517'),
    'nrrdApply1DLut': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:434'),
    'nrrdApplyMulti1DLut': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:465'),
    'nrrdApply1DRegMap': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:514'),
    'nrrdApplyMulti1DRegMap': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:545'),
    'nrrd1DIrregMapCheck': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:587'),
    'nrrd1DIrregAclCheck': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:684'),
    'nrrd1DIrregAclGenerate': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:816'),
    'nrrdApply1DIrregMap': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:880'),
    'nrrdApply1DSubstitution': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:1054'),
    'nrrdApply2DLut': (_equals_one, 0, b'nrrd', 'nrrd/apply2D.c:297'),
    'nrrdArithGamma': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:50'),
    'nrrdArithSRGBGamma': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:138'),
    'nrrdArithUnaryOp': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:344'),
    'nrrdArithBinaryOp': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:553'),
    'nrrdArithIterBinaryOpSelect': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:639'),
    'nrrdArithIterBinaryOp': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:726'),
    'nrrdArithTernaryOp': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:876'),
    'nrrdArithIterTernaryOpSelect': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:954'),
    'nrrdArithIterTernaryOp': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:1042'),
    'nrrdArithAffine': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:1065'),
    'nrrdArithIterAffine': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:1108'),
    'nrrdAxisInfoCompare': (_equals_one, 0, b'nrrd', 'nrrd/axis.c:929'),
    'nrrdOrientationReduce': (_equals_one, 0, b'nrrd', 'nrrd/axis.c:1221'),
    'nrrdMetaDataNormalize': (_equals_one, 0, b'nrrd', 'nrrd/axis.c:1266'),
    'nrrdCCFind': (_equals_one, 0, b'nrrd', 'nrrd/cc.c:285'),
    'nrrdCCAdjacency': (_equals_one, 0, b'nrrd', 'nrrd/cc.c:545'),
    'nrrdCCMerge': (_equals_one, 0, b'nrrd', 'nrrd/cc.c:645'),
    'nrrdCCRevalue': (_equals_one, 0, b'nrrd', 'nrrd/cc.c:795'),
    'nrrdCCSettle': (_equals_one, 0, b'nrrd', 'nrrd/cc.c:822'),
    'nrrdCCValid': ((lambda rv: 0 == rv), 0, b'nrrd', 'nrrd/ccmethods.c:26'),
    'nrrdCCSize': (_equals_one, 0, b'nrrd', 'nrrd/ccmethods.c:57'),
    'nrrdDeringVerboseSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:101'),
    'nrrdDeringLinearInterpSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:114'),
    'nrrdDeringVerticalSeamSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:127'),
    'nrrdDeringInputSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:140'),
    'nrrdDeringCenterSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:175'),
    'nrrdDeringClampPercSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:194'),
    'nrrdDeringClampHistoBinsSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:215'),
    'nrrdDeringRadiusScaleSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:234'),
    'nrrdDeringThetaNumSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:252'),
    'nrrdDeringRadialKernelSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:270'),
    'nrrdDeringThetaKernelSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:290'),
    'nrrdDeringExecute': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:750'),
    'nrrdCheapMedian': (_equals_one, 0, b'nrrd', 'nrrd/filt.c:407'),
    'nrrdDistanceL2': (_equals_one, 0, b'nrrd', 'nrrd/filt.c:814'),
    'nrrdDistanceL2Biased': (_equals_one, 0, b'nrrd', 'nrrd/filt.c:826'),
    'nrrdDistanceL2Signed': (_equals_one, 0, b'nrrd', 'nrrd/filt.c:838'),
    'nrrdHisto': (_equals_one, 0, b'nrrd', 'nrrd/histogram.c:40'),
    'nrrdHistoCheck': (_equals_one, 0, b'nrrd', 'nrrd/histogram.c:160'),
    'nrrdHistoDraw': (_equals_one, 0, b'nrrd', 'nrrd/histogram.c:189'),
    'nrrdHistoAxis': (_equals_one, 0, b'nrrd', 'nrrd/histogram.c:325'),
    'nrrdHistoJoint': (_equals_one, 0, b'nrrd', 'nrrd/histogram.c:439'),
    'nrrdHistoThresholdOtsu': (_equals_one, 0, b'nrrd', 'nrrd/histogram.c:649'),
    'nrrdKernelParse': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3032'),
    'nrrdKernelSpecParse': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3212'),
    'nrrdKernelSpecSprint': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3234'),
    'nrrdKernelSprint': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3289'),
    'nrrdKernelCompare': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3307'),
    'nrrdKernelSpecCompare': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3356'),
    'nrrdKernelCheck': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3429'),
    'nrrdConvert': (_equals_one, 0, b'nrrd', 'nrrd/map.c:234'),
    'nrrdClampConvert': (_equals_one, 0, b'nrrd', 'nrrd/map.c:254'),
    'nrrdCastClampRound': (_equals_one, 0, b'nrrd', 'nrrd/map.c:280'),
    'nrrdQuantize': (_equals_one, 0, b'nrrd', 'nrrd/map.c:302'),
    'nrrdUnquantize': (_equals_one, 0, b'nrrd', 'nrrd/map.c:474'),
    'nrrdHistoEq': (_equals_one, 0, b'nrrd', 'nrrd/map.c:611'),
    'nrrdProject': (_equals_one, 0, b'nrrd', 'nrrd/measure.c:1136'),
    'nrrdBoundarySpecCheck': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:93'),
    'nrrdBoundarySpecParse': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:117'),
    'nrrdBoundarySpecSprint': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:176'),
    'nrrdBoundarySpecCompare': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:198'),
    'nrrdBasicInfoCopy': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:538'),
    'nrrdWrap_nva': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:814'),
    'nrrdWrap_va': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:845'),
    'nrrdCopy': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:936'),
    'nrrdAlloc_nva': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:966'),
    'nrrdAlloc_va': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:1015'),
    'nrrdMaybeAlloc_nva': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:1136'),
    'nrrdMaybeAlloc_va': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:1153'),
    'nrrdCompare': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:1194'),
    'nrrdPPM': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:1380'),
    'nrrdPGM': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:1400'),
    'nrrdSpaceVectorParse': (_equals_one, 4, b'nrrd', 'nrrd/parseNrrd.c:521'),
    '_nrrdDataFNCheck': (_equals_one, 3, b'nrrd', 'nrrd/parseNrrd.c:1198'),
    'nrrdRangePercentileSet': (_equals_one, 0, b'nrrd', 'nrrd/range.c:109'),
    'nrrdRangePercentileFromStringSet': (_equals_one, 0, b'nrrd', 'nrrd/range.c:211'),
    'nrrdOneLine': (_equals_one, 0, b'nrrd', 'nrrd/read.c:72'),
    'nrrdLineSkip': (_equals_one, 0, b'nrrd', 'nrrd/read.c:236'),
    'nrrdByteSkip': (_equals_one, 0, b'nrrd', 'nrrd/read.c:332'),
    'nrrdRead': (_equals_one, 0, b'nrrd', 'nrrd/read.c:496'),
    'nrrdStringRead': (_equals_one, 0, b'nrrd', 'nrrd/read.c:516'),
    'nrrdLoad': ((lambda rv: 1 == rv or 2 == rv), 0, b'nrrd', 'nrrd/read.c:612'),
    'nrrdLoadMulti': (_equals_one, 0, b'nrrd', 'nrrd/read.c:666'),
    'nrrdInvertPerm': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:34'),
    'nrrdAxesInsert': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:86'),
    'nrrdAxesPermute': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:152'),
    'nrrdShuffle': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:306'),
    'nrrdAxesSwap': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:451'),
    'nrrdFlip': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:487'),
    'nrrdJoin': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:568'),
    'nrrdAxesSplit': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:815'),
    'nrrdAxesDelete': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:877'),
    'nrrdAxesMerge': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:929'),
    'nrrdReshape_nva': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:979'),
    'nrrdReshape_va': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:1047'),
    'nrrdBlock': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:1084'),
    'nrrdUnblock': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:1155'),
    'nrrdTile2D': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:1254'),
    'nrrdUntile2D': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:1368'),
    'nrrdResampleDefaultCenterSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:170'),
    'nrrdResampleNonExistentSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:191'),
    'nrrdResampleRangeSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:324'),
    'nrrdResampleOverrideCenterSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:343'),
    'nrrdResampleBoundarySet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:400'),
    'nrrdResamplePadValueSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:421'),
    'nrrdResampleBoundarySpecSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:438'),
    'nrrdResampleRenormalizeSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:459'),
    'nrrdResampleTypeOutSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:476'),
    'nrrdResampleRoundSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:501'),
    'nrrdResampleClampSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:518'),
    'nrrdResampleExecute': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:1453'),
    'nrrdFFTWWisdomRead': (_equals_one, 0, b'nrrd', 'nrrd/fftNrrd.c:34'),
    'nrrdFFT': (_equals_one, 0, b'nrrd', 'nrrd/fftNrrd.c:90'),
    'nrrdFFTWWisdomWrite': (_equals_one, 0, b'nrrd', 'nrrd/fftNrrd.c:287'),
    'nrrdSimpleResample': (_equals_one, 0, b'nrrd', 'nrrd/resampleNrrd.c:51'),
    'nrrdSpatialResample': (_equals_one, 0, b'nrrd', 'nrrd/resampleNrrd.c:521'),
    'nrrdSpaceSet': (_equals_one, 0, b'nrrd', 'nrrd/simple.c:83'),
    'nrrdSpaceDimensionSet': (_equals_one, 0, b'nrrd', 'nrrd/simple.c:120'),
    'nrrdSpaceOriginSet': (_equals_one, 0, b'nrrd', 'nrrd/simple.c:172'),
    'nrrdContentSet_va': (_equals_one, 0, b'nrrd', 'nrrd/simple.c:473'),
    '_nrrdCheck': (_equals_one, 3, b'nrrd', 'nrrd/simple.c:1077'),
    'nrrdCheck': (_equals_one, 0, b'nrrd', 'nrrd/simple.c:1114'),
    'nrrdSameSize': ((lambda rv: 0 == rv), 3, b'nrrd', 'nrrd/simple.c:1135'),
    'nrrdSanity': ((lambda rv: 0 == rv), 0, b'nrrd', 'nrrd/simple.c:1367'),
    'nrrdSlice': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:39'),
    'nrrdCrop': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:184'),
    'nrrdSliceSelect': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:366'),
    'nrrdSample_nva': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:578'),
    'nrrdSample_va': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:617'),
    'nrrdSimpleCrop': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:646'),
    'nrrdCropAuto': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:667'),
    'nrrdSplice': (_equals_one, 0, b'nrrd', 'nrrd/superset.c:32'),
    'nrrdInset': (_equals_one, 0, b'nrrd', 'nrrd/superset.c:157'),
    'nrrdPad_va': (_equals_one, 0, b'nrrd', 'nrrd/superset.c:281'),
    'nrrdPad_nva': (_equals_one, 0, b'nrrd', 'nrrd/superset.c:487'),
    'nrrdSimplePad_va': (_equals_one, 0, b'nrrd', 'nrrd/superset.c:515'),
    'nrrdSimplePad_nva': (_equals_one, 0, b'nrrd', 'nrrd/superset.c:553'),
    'nrrdIoStateSet': (_equals_one, 0, b'nrrd', 'nrrd/write.c:31'),
    'nrrdIoStateEncodingSet': (_equals_one, 0, b'nrrd', 'nrrd/write.c:104'),
    'nrrdIoStateFormatSet': (_equals_one, 0, b'nrrd', 'nrrd/write.c:124'),
    'nrrdWrite': (_equals_one, 0, b'nrrd', 'nrrd/write.c:944'),
    'nrrdStringWrite': (_equals_one, 0, b'nrrd', 'nrrd/write.c:960'),
    'nrrdSave': (_equals_one, 0, b'nrrd', 'nrrd/write.c:981'),
    'nrrdSaveMulti': (_equals_one, 0, b'nrrd', 'nrrd/write.c:1034'),
    'ell_Nm_check': (_equals_one, 0, b'ell', 'ell/genmat.c:25'),
    'ell_Nm_tran': (_equals_one, 0, b'ell', 'ell/genmat.c:59'),
    'ell_Nm_mul': (_equals_one, 0, b'ell', 'ell/genmat.c:104'),
    'ell_Nm_inv': (_equals_one, 0, b'ell', 'ell/genmat.c:338'),
    'ell_Nm_pseudo_inv': (_equals_one, 0, b'ell', 'ell/genmat.c:379'),
    'ell_Nm_wght_pseudo_inv': (_equals_one, 0, b'ell', 'ell/genmat.c:413'),
    'ell_q_avg4_d': (_equals_one, 0, b'ell', 'ell/quat.c:471'),
    'ell_q_avgN_d': (_equals_one, 0, b'ell', 'ell/quat.c:539'),
    'mossImageCheck': (_equals_one, 0, b'moss', 'moss/methodsMoss.c:74'),
    'mossImageAlloc': (_equals_one, 0, b'moss', 'moss/methodsMoss.c:95'),
    'mossSamplerImageSet': (_equals_one, 0, b'moss', 'moss/sampler.c:26'),
    'mossSamplerKernelSet': (_equals_one, 0, b'moss', 'moss/sampler.c:78'),
    'mossSamplerUpdate': (_equals_one, 0, b'moss', 'moss/sampler.c:100'),
    'mossSamplerSample': (_equals_one, 0, b'moss', 'moss/sampler.c:195'),
    'mossLinearTransform': (_equals_one, 0, b'moss', 'moss/xform.c:140'),
    'mossFourPointTransform': (_equals_one, 0, b'moss', 'moss/xform.c:219'),
    'alanUpdate': (_equals_one, 0, b'alan', 'alan/coreAlan.c:60'),
    'alanInit': (_equals_one, 0, b'alan', 'alan/coreAlan.c:99'),
    'alanRun': (_equals_one, 0, b'alan', 'alan/coreAlan.c:453'),
    'alanDimensionSet': (_equals_one, 0, b'alan', 'alan/methodsAlan.c:104'),
    'alan2DSizeSet': (_equals_one, 0, b'alan', 'alan/methodsAlan.c:119'),
    'alan3DSizeSet': (_equals_one, 0, b'alan', 'alan/methodsAlan.c:139'),
    'alanTensorSet': (_equals_one, 0, b'alan', 'alan/methodsAlan.c:161'),
    'alanParmSet': (_equals_one, 0, b'alan', 'alan/methodsAlan.c:208'),
    'gageContextCopy': (_equals_null, 0, b'gage', 'gage/ctx.c:88'),
    'gageKernelSet': (_equals_one, 0, b'gage', 'gage/ctx.c:199'),
    'gagePerVolumeAttach': (_equals_one, 0, b'gage', 'gage/ctx.c:398'),
    'gagePerVolumeDetach': (_equals_one, 0, b'gage', 'gage/ctx.c:457'),
    'gageDeconvolve': (_equals_one, 0, b'gage', 'gage/deconvolve.c:26'),
    'gageDeconvolveSeparable': (_equals_one, 0, b'gage', 'gage/deconvolve.c:208'),
    'gageKindCheck': (_equals_one, 0, b'gage', 'gage/kind.c:33'),
    'gageKindVolumeCheck': (_equals_one, 0, b'gage', 'gage/kind.c:218'),
    'gageVolumeCheck': (_equals_one, 0, b'gage', 'gage/pvl.c:36'),
    'gagePerVolumeNew': (_equals_null, 0, b'gage', 'gage/pvl.c:57'),
    'gageQueryReset': (_equals_one, 0, b'gage', 'gage/pvl.c:261'),
    'gageQuerySet': (_equals_one, 0, b'gage', 'gage/pvl.c:287'),
    'gageQueryAdd': (_equals_one, 0, b'gage', 'gage/pvl.c:343'),
    'gageQueryItemOn': (_equals_one, 0, b'gage', 'gage/pvl.c:361'),
    'gageShapeSet': (_equals_one, 0, b'gage', 'gage/shape.c:405'),
    'gageShapeEqual': ((lambda rv: 0 == rv), 0, b'gage', 'gage/shape.c:468'),
    'gageStructureTensor': (_equals_one, 0, b'gage', 'gage/st.c:83'),
    'gageStackPerVolumeNew': (_equals_one, 0, b'gage', 'gage/stack.c:98'),
    'gageStackPerVolumeAttach': (_equals_one, 0, b'gage', 'gage/stack.c:127'),
    'gageStackBlurParmCompare': (_equals_one, 0, b'gage', 'gage/stackBlur.c:125'),
    'gageStackBlurParmCopy': (_equals_one, 0, b'gage', 'gage/stackBlur.c:230'),
    'gageStackBlurParmSigmaSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:267'),
    'gageStackBlurParmScaleSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:361'),
    'gageStackBlurParmKernelSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:385'),
    'gageStackBlurParmRenormalizeSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:398'),
    'gageStackBlurParmBoundarySet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:410'),
    'gageStackBlurParmBoundarySpecSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:429'),
    'gageStackBlurParmOneDimSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:446'),
    'gageStackBlurParmNeedSpatialBlurSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:458'),
    'gageStackBlurParmVerboseSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:470'),
    'gageStackBlurParmDgGoodSigmaMaxSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:482'),
    'gageStackBlurParmCheck': (_equals_one, 0, b'gage', 'gage/stackBlur.c:498'),
    'gageStackBlurParmParse': (_equals_one, 0, b'gage', 'gage/stackBlur.c:545'),
    'gageStackBlurParmSprint': (_equals_one, 0, b'gage', 'gage/stackBlur.c:804'),
    'gageStackBlur': (_equals_one, 0, b'gage', 'gage/stackBlur.c:1386'),
    'gageStackBlurCheck': (_equals_one, 0, b'gage', 'gage/stackBlur.c:1489'),
    'gageStackBlurGet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:1597'),
    'gageStackBlurManage': (_equals_one, 0, b'gage', 'gage/stackBlur.c:1698'),
    'gageUpdate': (_equals_one, 0, b'gage', 'gage/update.c:313'),
    'gageOptimSigSet': (_equals_one, 0, b'gage', 'gage/optimsig.c:217'),
    'gageOptimSigContextNew': (_equals_null, 0, b'gage', 'gage/optimsig.c:311'),
    'gageOptimSigCalculate': (_equals_one, 0, b'gage', 'gage/optimsig.c:1090'),
    'gageOptimSigErrorPlot': (_equals_one, 0, b'gage', 'gage/optimsig.c:1162'),
    'gageOptimSigErrorPlotSliding': (_equals_one, 0, b'gage', 'gage/optimsig.c:1253'),
    'dyeConvert': (_equals_one, 0, b'dye', 'dye/convertDye.c:351'),
    'dyeColorParse': (_equals_one, 0, b'dye', 'dye/methodsDye.c:185'),
    'baneClipNew': (_equals_null, 0, b'bane', 'bane/clip.c:102'),
    'baneClipAnswer': (_equals_one, 0, b'bane', 'bane/clip.c:152'),
    'baneClipCopy': (_equals_null, 0, b'bane', 'bane/clip.c:167'),
    'baneFindInclusion': (_equals_one, 0, b'bane', 'bane/hvol.c:87'),
    'baneMakeHVol': (_equals_one, 0, b'bane', 'bane/hvol.c:248'),
    'baneGKMSHVol': (_equals_null, 0, b'bane', 'bane/hvol.c:447'),
    'baneIncNew': (_equals_null, 0, b'bane', 'bane/inc.c:251'),
    'baneIncAnswer': (_equals_one, 0, b'bane', 'bane/inc.c:360'),
    'baneIncCopy': (_equals_null, 0, b'bane', 'bane/inc.c:375'),
    'baneMeasrNew': (_equals_null, 0, b'bane', 'bane/measr.c:33'),
    'baneMeasrCopy': (_equals_null, 0, b'bane', 'bane/measr.c:149'),
    'baneRangeNew': (_equals_null, 0, b'bane', 'bane/rangeBane.c:89'),
    'baneRangeCopy': (_equals_null, 0, b'bane', 'bane/rangeBane.c:130'),
    'baneRangeAnswer': (_equals_one, 0, b'bane', 'bane/rangeBane.c:144'),
    'baneRawScatterplots': (_equals_one, 0, b'bane', 'bane/scat.c:26'),
    'baneOpacInfo': (_equals_one, 0, b'bane', 'bane/trnsf.c:29'),
    'bane1DOpacInfoFrom2D': (_equals_one, 0, b'bane', 'bane/trnsf.c:144'),
    'baneSigmaCalc': (_equals_one, 0, b'bane', 'bane/trnsf.c:222'),
    'banePosCalc': (_equals_one, 0, b'bane', 'bane/trnsf.c:253'),
    'baneOpacCalc': (_equals_one, 0, b'bane', 'bane/trnsf.c:403'),
    'baneInputCheck': (_equals_one, 0, b'bane', 'bane/valid.c:26'),
    'baneHVolCheck': (_equals_one, 0, b'bane', 'bane/valid.c:64'),
    'baneInfoCheck': (_equals_one, 0, b'bane', 'bane/valid.c:106'),
    'banePosCheck': (_equals_one, 0, b'bane', 'bane/valid.c:144'),
    'baneBcptsCheck': (_equals_one, 0, b'bane', 'bane/valid.c:179'),
    'limnCameraUpdate': (_equals_one, 0, b'limn', 'limn/cam.c:33'),
    'limnCameraAspectSet': (_equals_one, 0, b'limn', 'limn/cam.c:130'),
    'limnCameraPathMake': (_equals_one, 0, b'limn', 'limn/cam.c:189'),
    'limnEnvMapFill': (_equals_one, 0, b'limn', 'limn/envmap.c:25'),
    'limnEnvMapCheck': (_equals_one, 0, b'limn', 'limn/envmap.c:119'),
    'limnObjectWriteOFF': (_equals_one, 0, b'limn', 'limn/io.c:79'),
    'limnPolyDataWriteIV': (_equals_one, 0, b'limn', 'limn/io.c:138'),
    'limnObjectReadOFF': (_equals_one, 0, b'limn', 'limn/io.c:264'),
    'limnPolyDataWriteLMPD': (_equals_one, 0, b'limn', 'limn/io.c:455'),
    'limnPolyDataReadLMPD': (_equals_one, 0, b'limn', 'limn/io.c:582'),
    'limnPolyDataWriteVTK': (_equals_one, 0, b'limn', 'limn/io.c:965'),
    'limnPolyDataReadOFF': (_equals_one, 0, b'limn', 'limn/io.c:1055'),
    'limnPolyDataSave': (_equals_one, 0, b'limn', 'limn/io.c:1160'),
    'limnLightUpdate': (_equals_one, 0, b'limn', 'limn/light.c:67'),
    'limnPolyDataAlloc': (_equals_one, 0, b'limn', 'limn/polydata.c:149'),
    'limnPolyDataCopy': (_equals_one, 0, b'limn', 'limn/polydata.c:228'),
    'limnPolyDataCopyN': (_equals_one, 0, b'limn', 'limn/polydata.c:260'),
    'limnPolyDataPrimitiveVertexNumber': (_equals_one, 0, b'limn', 'limn/polydata.c:551'),
    'limnPolyDataPrimitiveArea': (_equals_one, 0, b'limn', 'limn/polydata.c:573'),
    'limnPolyDataRasterize': (_equals_one, 0, b'limn', 'limn/polydata.c:631'),
    'limnPolyDataSpiralTubeWrap': (_equals_one, 0, b'limn', 'limn/polyfilter.c:26'),
    'limnPolyDataSmoothHC': ((lambda rv: -1 == rv), 0, b'limn', 'limn/polyfilter.c:336'),
    'limnPolyDataVertexWindingFix': (_equals_one, 0, b'limn', 'limn/polymod.c:1230'),
    'limnPolyDataCCFind': (_equals_one, 0, b'limn', 'limn/polymod.c:1249'),
    'limnPolyDataPrimitiveSort': (_equals_one, 0, b'limn', 'limn/polymod.c:1380'),
    'limnPolyDataVertexWindingFlip': (_equals_one, 0, b'limn', 'limn/polymod.c:1463'),
    'limnPolyDataPrimitiveSelect': (_equals_one, 0, b'limn', 'limn/polymod.c:1492'),
    'limnPolyDataClipMulti': (_equals_one, 0, b'limn', 'limn/polymod.c:1707'),
    'limnPolyDataCompress': (_equals_null, 0, b'limn', 'limn/polymod.c:1994'),
    'limnPolyDataJoin': (_equals_null, 0, b'limn', 'limn/polymod.c:2084'),
    'limnPolyDataEdgeHalve': (_equals_one, 0, b'limn', 'limn/polymod.c:2152'),
    'limnPolyDataNeighborList': ((lambda rv: -1 == rv), 0, b'limn', 'limn/polymod.c:2329'),
    'limnPolyDataNeighborArray': ((lambda rv: -1 == rv), 0, b'limn', 'limn/polymod.c:2425'),
    'limnPolyDataNeighborArrayComp': ((lambda rv: -1 == rv), 0, b'limn', 'limn/polymod.c:2465'),
    'limnPolyDataCube': (_equals_one, 0, b'limn', 'limn/polyshapes.c:27'),
    'limnPolyDataCubeTriangles': (_equals_one, 0, b'limn', 'limn/polyshapes.c:137'),
    'limnPolyDataOctahedron': (_equals_one, 0, b'limn', 'limn/polyshapes.c:347'),
    'limnPolyDataCylinder': (_equals_one, 0, b'limn', 'limn/polyshapes.c:461'),
    'limnPolyDataCone': (_equals_one, 0, b'limn', 'limn/polyshapes.c:635'),
    'limnPolyDataSuperquadric': (_equals_one, 0, b'limn', 'limn/polyshapes.c:734'),
    'limnPolyDataSpiralBetterquadric': (_equals_one, 0, b'limn', 'limn/polyshapes.c:859'),
    'limnPolyDataSpiralSuperquadric': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1016'),
    'limnPolyDataPolarSphere': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1034'),
    'limnPolyDataSpiralSphere': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1046'),
    'limnPolyDataIcoSphere': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1097'),
    'limnPolyDataPlane': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1341'),
    'limnPolyDataSquare': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1396'),
    'limnPolyDataSuperquadric2D': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1439'),
    'limnQNDemo': (_equals_one, 0, b'limn', 'limn/qn.c:892'),
    'limnObjectRender': (_equals_one, 0, b'limn', 'limn/renderLimn.c:25'),
    'limnObjectPSDraw': (_equals_one, 0, b'limn', 'limn/renderLimn.c:184'),
    'limnObjectPSDrawConcave': (_equals_one, 0, b'limn', 'limn/renderLimn.c:314'),
    'limnSplineNrrdEvaluate': (_equals_one, 0, b'limn', 'limn/splineEval.c:323'),
    'limnSplineSample': (_equals_one, 0, b'limn', 'limn/splineEval.c:361'),
    'limnSplineTypeSpecNew': (_equals_null, 0, b'limn', 'limn/splineMethods.c:25'),
    'limnSplineNew': (_equals_null, 0, b'limn', 'limn/splineMethods.c:124'),
    'limnSplineNrrdCleverFix': (_equals_one, 0, b'limn', 'limn/splineMethods.c:249'),
    'limnSplineCleverNew': (_equals_null, 0, b'limn', 'limn/splineMethods.c:394'),
    'limnSplineUpdate': (_equals_one, 0, b'limn', 'limn/splineMethods.c:422'),
    'limnSplineTypeSpecParse': (_equals_null, 0, b'limn', 'limn/splineMisc.c:222'),
    'limnSplineParse': (_equals_null, 0, b'limn', 'limn/splineMisc.c:278'),
    'limnCBFCheck': (_equals_one, 0, b'limn', 'limn/splineFit.c:589'),
    'limnCBFitSingle': (_equals_one, 0, b'limn', 'limn/splineFit.c:860'),
    'limnCBFMulti': (_equals_one, 0, b'limn', 'limn/splineFit.c:951'),
    'limnCBFCorners': (_equals_one, 0, b'limn', 'limn/splineFit.c:1053'),
    'limnCBFit': (_equals_one, 0, b'limn', 'limn/splineFit.c:1123'),
    'limnObjectWorldHomog': (_equals_one, 0, b'limn', 'limn/transform.c:25'),
    'limnObjectFaceNormals': (_equals_one, 0, b'limn', 'limn/transform.c:47'),
    'limnObjectSpaceTransform': (_equals_one, 0, b'limn', 'limn/transform.c:210'),
    'limnObjectFaceReverse': (_equals_one, 0, b'limn', 'limn/transform.c:335'),
    'echoThreadStateInit': (_equals_one, 0, b'echo', 'echo/renderEcho.c:26'),
    'echoRTRenderCheck': (_equals_one, 0, b'echo', 'echo/renderEcho.c:134'),
    'echoRTRender': (_equals_one, 0, b'echo', 'echo/renderEcho.c:409'),
    'hooverContextCheck': (_equals_one, 0, b'hoover', 'hoover/methodsHoover.c:53'),
    'hooverRender': ((lambda rv: _teem.lib.hooverErrInit == rv), 0, b'hoover', 'hoover/rays.c:359'),
    'seekExtract': (_equals_one, 0, b'seek', 'seek/extract.c:936'),
    'seekDataSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:56'),
    'seekSamplesSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:116'),
    'seekTypeSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:149'),
    'seekLowerInsideSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:173'),
    'seekNormalsFindSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:193'),
    'seekStrengthUseSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:208'),
    'seekStrengthSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:223'),
    'seekItemScalarSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:285'),
    'seekItemStrengthSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:304'),
    'seekItemHessSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:323'),
    'seekItemGradientSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:343'),
    'seekItemNormalSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:364'),
    'seekItemEigensystemSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:385'),
    'seekIsovalueSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:414'),
    'seekEvalDiffThreshSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:440'),
    'seekVertexStrength': (_equals_one, 0, b'seek', 'seek/textract.c:1884'),
    'seekUpdate': (_equals_one, 0, b'seek', 'seek/updateSeek.c:672'),
    'tenAnisoPlot': (_equals_one, 0, b'ten', 'ten/aniso.c:1068'),
    'tenAnisoVolume': (_equals_one, 0, b'ten', 'ten/aniso.c:1127'),
    'tenAnisoHistogram': (_equals_one, 0, b'ten', 'ten/aniso.c:1199'),
    'tenEvecRGBParmCheck': (_equals_one, 0, b'ten', 'ten/aniso.c:1313'),
    'tenEMBimodal': (_equals_one, 0, b'ten', 'ten/bimod.c:412'),
    'tenBVecNonLinearFit': (_equals_one, 0, b'ten', 'ten/bvec.c:99'),
    'tenDWMRIKeyValueParse': (_equals_one, 0, b'ten', 'ten/chan.c:60'),
    'tenBMatrixCalc': (_equals_one, 0, b'ten', 'ten/chan.c:348'),
    'tenEMatrixCalc': (_equals_one, 0, b'ten', 'ten/chan.c:389'),
    'tenEstimateLinear3D': (_equals_one, 0, b'ten', 'ten/chan.c:582'),
    'tenEstimateLinear4D': (_equals_one, 0, b'ten', 'ten/chan.c:629'),
    'tenSimulate': (_equals_one, 0, b'ten', 'ten/chan.c:870'),
    'tenEpiRegister3D': (_equals_one, 0, b'ten', 'ten/epireg.c:1044'),
    'tenEpiRegister4D': (_equals_one, 0, b'ten', 'ten/epireg.c:1195'),
    'tenEstimateMethodSet': (_equals_one, 0, b'ten', 'ten/estimate.c:283'),
    'tenEstimateSigmaSet': (_equals_one, 0, b'ten', 'ten/estimate.c:305'),
    'tenEstimateValueMinSet': (_equals_one, 0, b'ten', 'ten/estimate.c:323'),
    'tenEstimateGradientsSet': (_equals_one, 0, b'ten', 'ten/estimate.c:341'),
    'tenEstimateBMatricesSet': (_equals_one, 0, b'ten', 'ten/estimate.c:368'),
    'tenEstimateSkipSet': (_equals_one, 0, b'ten', 'ten/estimate.c:395'),
    'tenEstimateSkipReset': (_equals_one, 0, b'ten', 'ten/estimate.c:413'),
    'tenEstimateThresholdFind': (_equals_one, 0, b'ten', 'ten/estimate.c:428'),
    'tenEstimateThresholdSet': (_equals_one, 0, b'ten', 'ten/estimate.c:496'),
    'tenEstimateUpdate': (_equals_one, 0, b'ten', 'ten/estimate.c:802'),
    'tenEstimate1TensorSimulateSingle_f': (_equals_one, 0, b'ten', 'ten/estimate.c:976'),
    'tenEstimate1TensorSimulateSingle_d': (_equals_one, 0, b'ten', 'ten/estimate.c:1004'),
    'tenEstimate1TensorSimulateVolume': (_equals_one, 0, b'ten', 'ten/estimate.c:1035'),
    'tenEstimate1TensorSingle_f': (_equals_one, 0, b'ten', 'ten/estimate.c:1740'),
    'tenEstimate1TensorSingle_d': (_equals_one, 0, b'ten', 'ten/estimate.c:1768'),
    'tenEstimate1TensorVolume4D': (_equals_one, 0, b'ten', 'ten/estimate.c:1805'),
    'tenFiberTraceSet': (_equals_one, 0, b'ten', 'ten/fiber.c:828'),
    'tenFiberTrace': (_equals_one, 0, b'ten', 'ten/fiber.c:848'),
    'tenFiberDirectionNumber': ((lambda rv: 0 == rv), 0, b'ten', 'ten/fiber.c:868'),
    'tenFiberSingleTrace': (_equals_one, 0, b'ten', 'ten/fiber.c:917'),
    'tenFiberMultiNew': (_equals_null, 0, b'ten', 'ten/fiber.c:960'),
    'tenFiberMultiTrace': (_equals_one, 0, b'ten', 'ten/fiber.c:1025'),
    'tenFiberMultiPolyData': (_equals_one, 0, b'ten', 'ten/fiber.c:1245'),
    'tenFiberMultiProbeVals': (_equals_one, 0, b'ten', 'ten/fiber.c:1256'),
    'tenFiberContextDwiNew': (_equals_null, 0, b'ten', 'ten/fiberMethods.c:210'),
    'tenFiberContextNew': (_equals_null, 0, b'ten', 'ten/fiberMethods.c:224'),
    'tenFiberTypeSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:248'),
    'tenFiberStopSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:378'),
    'tenFiberStopAnisoSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:554'),
    'tenFiberStopDoubleSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:566'),
    'tenFiberStopUIntSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:590'),
    'tenFiberAnisoSpeedSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:637'),
    'tenFiberAnisoSpeedReset': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:702'),
    'tenFiberKernelSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:717'),
    'tenFiberProbeItemSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:736'),
    'tenFiberIntgSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:748'),
    'tenFiberUpdate': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:791'),
    'tenGlyphParmCheck': (_equals_one, 0, b'ten', 'ten/glyph.c:72'),
    'tenGlyphGen': (_equals_one, 0, b'ten', 'ten/glyph.c:173'),
    'tenGradientCheck': (_equals_one, 0, b'ten', 'ten/grads.c:67'),
    'tenGradientRandom': (_equals_one, 0, b'ten', 'ten/grads.c:106'),
    'tenGradientJitter': (_equals_one, 0, b'ten', 'ten/grads.c:151'),
    'tenGradientBalance': (_equals_one, 0, b'ten', 'ten/grads.c:373'),
    'tenGradientDistribute': (_equals_one, 0, b'ten', 'ten/grads.c:458'),
    'tenGradientGenerate': (_equals_one, 0, b'ten', 'ten/grads.c:651'),
    'tenEvecRGB': (_equals_one, 0, b'ten', 'ten/miscTen.c:26'),
    'tenEvqVolume': (_equals_one, 0, b'ten', 'ten/miscTen.c:151'),
    'tenBMatrixCheck': (_equals_one, 0, b'ten', 'ten/miscTen.c:212'),
    '_tenFindValley': (_equals_one, 0, b'ten', 'ten/miscTen.c:256'),
    'tenSizeNormalize': (_equals_one, 0, b'ten', 'ten/mod.c:221'),
    'tenSizeScale': (_equals_one, 0, b'ten', 'ten/mod.c:237'),
    'tenAnisoScale': (_equals_one, 0, b'ten', 'ten/mod.c:255'),
    'tenEigenvalueClamp': (_equals_one, 0, b'ten', 'ten/mod.c:275'),
    'tenEigenvaluePower': (_equals_one, 0, b'ten', 'ten/mod.c:294'),
    'tenEigenvalueAdd': (_equals_one, 0, b'ten', 'ten/mod.c:312'),
    'tenEigenvalueMultiply': (_equals_one, 0, b'ten', 'ten/mod.c:330'),
    'tenLog': (_equals_one, 0, b'ten', 'ten/mod.c:348'),
    'tenExp': (_equals_one, 0, b'ten', 'ten/mod.c:365'),
    'tenInterpParmBufferAlloc': (_equals_one, 0, b'ten', 'ten/path.c:64'),
    'tenInterpParmCopy': (_equals_null, 0, b'ten', 'ten/path.c:123'),
    'tenInterpN_d': (_equals_one, 0, b'ten', 'ten/path.c:305'),
    'tenInterpTwoDiscrete_d': (_equals_one, 0, b'ten', 'ten/path.c:804'),
    'tenInterpMulti3D': (_equals_one, 0, b'ten', 'ten/path.c:954'),
    'tenDwiGageKindSet': (_equals_one, 0, b'ten', 'ten/tenDwiGage.c:1037'),
    'tenDwiGageKindCheck': (_equals_one, 0, b'ten', 'ten/tenDwiGage.c:1177'),
    'tenTensorCheck': (_equals_one, 4, b'ten', 'ten/tensor.c:54'),
    'tenMeasurementFrameReduce': (_equals_one, 0, b'ten', 'ten/tensor.c:87'),
    'tenExpand2D': (_equals_one, 0, b'ten', 'ten/tensor.c:157'),
    'tenExpand': (_equals_one, 0, b'ten', 'ten/tensor.c:231'),
    'tenShrink': (_equals_one, 0, b'ten', 'ten/tensor.c:287'),
    'tenMake': (_equals_one, 0, b'ten', 'ten/tensor.c:529'),
    'tenSlice': (_equals_one, 0, b'ten', 'ten/tensor.c:631'),
    'tenTripleCalc': (_equals_one, 0, b'ten', 'ten/triple.c:415'),
    'tenTripleConvert': (_equals_one, 0, b'ten', 'ten/triple.c:473'),
    'tenExperSpecGradSingleBValSet': (_equals_one, 0, b'ten', 'ten/experSpec.c:63'),
    'tenExperSpecGradBValSet': (_equals_one, 0, b'ten', 'ten/experSpec.c:104'),
    'tenExperSpecFromKeyValueSet': (_equals_one, 0, b'ten', 'ten/experSpec.c:173'),
    'tenDWMRIKeyValueFromExperSpecSet': (_equals_one, 0, b'ten', 'ten/experSpec.c:328'),
    'tenModelParse': (_equals_one, 0, b'ten', 'ten/tenModel.c:63'),
    'tenModelFromAxisLearn': (_equals_one, 0, b'ten', 'ten/tenModel.c:124'),
    'tenModelSimulate': (_equals_one, 0, b'ten', 'ten/tenModel.c:162'),
    'tenModelSqeFit': (_equals_one, 0, b'ten', 'ten/tenModel.c:410'),
    'tenModelConvert': (_equals_one, 0, b'ten', 'ten/tenModel.c:685'),
    'pullEnergyPlot': (_equals_one, 0, b'pull', 'pull/actionPull.c:232'),
    'pullBinProcess': (_equals_one, 0, b'pull', 'pull/actionPull.c:1106'),
    'pullGammaLearn': (_equals_one, 0, b'pull', 'pull/actionPull.c:1141'),
    'pullBinsPointAdd': (_equals_one, 0, b'pull', 'pull/binningPull.c:183'),
    'pullBinsPointMaybeAdd': (_equals_one, 0, b'pull', 'pull/binningPull.c:205'),
    'pullOutputGetFilter': (_equals_one, 0, b'pull', 'pull/contextPull.c:381'),
    'pullOutputGet': (_equals_one, 0, b'pull', 'pull/contextPull.c:577'),
    'pullPropGet': (_equals_one, 0, b'pull', 'pull/contextPull.c:590'),
    'pullPositionHistoryNrrdGet': (_equals_one, 0, b'pull', 'pull/contextPull.c:768'),
    'pullPositionHistoryPolydataGet': (_equals_one, 0, b'pull', 'pull/contextPull.c:840'),
    'pullIterParmSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:104'),
    'pullSysParmSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:193'),
    'pullFlagSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:272'),
    'pullVerboseSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:347'),
    'pullThreadNumSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:372'),
    'pullRngSeedSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:384'),
    'pullProgressBinModSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:396'),
    'pullCallbackSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:408'),
    'pullInterEnergySet': (_equals_one, 0, b'pull', 'pull/parmPull.c:433'),
    'pullLogAddSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:494'),
    'pullInitRandomSet': (_equals_one, 0, b'pull', 'pull/initPull.c:109'),
    'pullInitHaltonSet': (_equals_one, 0, b'pull', 'pull/initPull.c:127'),
    'pullInitPointPerVoxelSet': (_equals_one, 0, b'pull', 'pull/initPull.c:146'),
    'pullInitGivenPosSet': (_equals_one, 0, b'pull', 'pull/initPull.c:174'),
    'pullInitLiveThreshUseSet': (_equals_one, 0, b'pull', 'pull/initPull.c:188'),
    'pullInitUnequalShapesAllowSet': (_equals_one, 0, b'pull', 'pull/initPull.c:201'),
    'pullStart': (_equals_one, 0, b'pull', 'pull/corePull.c:113'),
    'pullFinish': (_equals_one, 0, b'pull', 'pull/corePull.c:168'),
    'pullRun': (_equals_one, 0, b'pull', 'pull/corePull.c:335'),
    'pullEnergySpecParse': (_equals_one, 0, b'pull', 'pull/energy.c:627'),
    'pullInfoSpecAdd': (_equals_one, 0, b'pull', 'pull/infoPull.c:132'),
    'pullInfoGet': (_equals_one, 0, b'pull', 'pull/infoPull.c:404'),
    'pullInfoSpecSprint': (_equals_one, 0, b'pull', 'pull/infoPull.c:449'),
    'pullPointNew': (_equals_null, 0, b'pull', 'pull/pointPull.c:33'),
    'pullProbe': (_equals_one, 0, b'pull', 'pull/pointPull.c:358'),
    'pullPointInitializePerVoxel': (_equals_one, 0, b'pull', 'pull/pointPull.c:637'),
    'pullPointInitializeRandomOrHalton': (_equals_one, 0, b'pull', 'pull/pointPull.c:822'),
    'pullPointInitializeGivenPos': (_equals_one, 0, b'pull', 'pull/pointPull.c:993'),
    'pullVolumeSingleAdd': (_equals_one, 0, b'pull', 'pull/volumePull.c:212'),
    'pullVolumeStackAdd': (_equals_one, 0, b'pull', 'pull/volumePull.c:238'),
    'pullVolumeLookup': (_equals_null, 0, b'pull', 'pull/volumePull.c:475'),
    'pullConstraintScaleRange': (_equals_one, 0, b'pull', 'pull/volumePull.c:494'),
    'pullCCFind': (_equals_one, 0, b'pull', 'pull/ccPull.c:30'),
    'pullCCMeasure': (_equals_one, 0, b'pull', 'pull/ccPull.c:114'),
    'pullCCSort': (_equals_one, 0, b'pull', 'pull/ccPull.c:209'),
    'pullTraceSet': (_equals_one, 0, b'pull', 'pull/trace.c:245'),
    'pullTraceMultiAdd': (_equals_one, 0, b'pull', 'pull/trace.c:674'),
    'pullTraceMultiPlotAdd': (_equals_one, 0, b'pull', 'pull/trace.c:704'),
    'pullTraceMultiWrite': (_equals_one, 0, b'pull', 'pull/trace.c:1014'),
    'pullTraceMultiRead': (_equals_one, 0, b'pull', 'pull/trace.c:1119'),
    'coilStart': (_equals_one, 0, b'coil', 'coil/coreCoil.c:287'),
    'coilIterate': (_equals_one, 0, b'coil', 'coil/coreCoil.c:362'),
    'coilFinish': (_equals_one, 0, b'coil', 'coil/coreCoil.c:407'),
    'coilVolumeCheck': (_equals_one, 0, b'coil', 'coil/methodsCoil.c:25'),
    'coilContextAllSet': (_equals_one, 0, b'coil', 'coil/methodsCoil.c:69'),
    'coilOutputGet': (_equals_one, 0, b'coil', 'coil/methodsCoil.c:200'),
    'pushOutputGet': (_equals_one, 0, b'push', 'push/action.c:71'),
    'pushBinProcess': (_equals_one, 0, b'push', 'push/action.c:161'),
    'pushBinPointAdd': (_equals_one, 0, b'push', 'push/binning.c:180'),
    'pushRebin': (_equals_one, 0, b'push', 'push/binning.c:197'),
    'pushStart': (_equals_one, 0, b'push', 'push/corePush.c:183'),
    'pushIterate': (_equals_one, 0, b'push', 'push/corePush.c:233'),
    'pushRun': (_equals_one, 0, b'push', 'push/corePush.c:306'),
    'pushFinish': (_equals_one, 0, b'push', 'push/corePush.c:396'),
    'pushEnergySpecParse': (_equals_one, 0, b'push', 'push/forces.c:304'),
    'miteSample': (_math.isnan, 0, b'mite', 'mite/ray.c:151'),
    'miteRenderBegin': (_equals_one, 0, b'mite', 'mite/renderMite.c:63'),
    'miteShadeSpecParse': (_equals_one, 0, b'mite', 'mite/shade.c:69'),
    'miteThreadNew': (_equals_null, 0, b'mite', 'mite/thread.c:26'),
    'miteThreadBegin': (_equals_one, 0, b'mite', 'mite/thread.c:92'),
    'miteVariableParse': (_equals_one, 0, b'mite', 'mite/txf.c:101'),
    'miteNtxfCheck': (_equals_one, 0, b'mite', 'mite/txf.c:232'),
    'meetAirEnumAllCheck': (_equals_one, 0, b'meet', 'meet/enumall.c:226'),
    'meetNrrdKernelAllCheck': (_equals_one, 0, b'meet', 'meet/meetNrrd.c:236'),
    'meetPullVolCopy': (_equals_null, 0, b'meet', 'meet/meetPull.c:44'),
    'meetPullVolParse': (_equals_one, 0, b'meet', 'meet/meetPull.c:100'),
    'meetPullVolLeechable': (_equals_one, 0, b'meet', 'meet/meetPull.c:314'),
    'meetPullVolStackBlurParmFinishMulti': (_equals_one, 0, b'meet', 'meet/meetPull.c:427'),
    'meetPullVolLoadMulti': (_equals_one, 0, b'meet', 'meet/meetPull.c:472'),
    'meetPullVolAddMulti': (_equals_one, 0, b'meet', 'meet/meetPull.c:552'),
    'meetPullInfoParse': (_equals_one, 0, b'meet', 'meet/meetPull.c:634'),
    'meetPullInfoAddMulti': (_equals_one, 0, b'meet', 'meet/meetPull.c:765'),
}


def _biffer(func, func_name: str, blob):
    """
    generates function wrappers that turn C biff errors into Python exceptions
    """
    (
        rvtf,  # C-function return value test function
        mubi,  # Maybe useBiff index (1-based) into function arguments
        bkey,  # bytes for biff key to retrieve biff error
        fnln,  # filename and line number of C function
    ) = blob

    def wrapper(*args):
        """
        function wrapper that turns C biff errors into Python exceptions
        """
        ret_val = func(*args)
        # we have to get biff error if rvtf(ret_val) indicates error, and,
        # either: this function definitely uses biff (0 == mubi)
        #     or: (this function maybe uses biff and) "useBiff" args[mubi-1] is True
        if rvtf(ret_val) and (0 == mubi or args[mubi - 1]):
            err = _teem.lib.biffGetDone(bkey)
            estr = string(err).rstrip()
            _teem.lib.free(err)
            raise RuntimeError(
                f'return value {ret_val} from C function "{func_name}" ({fnln}):\n{estr}'
            )
        return ret_val

    wrapper.__name__ = func_name
    wrapper.__doc__ = f"""
error-checking wrapper around C function "{func_name}" ({fnln}):
{func.__doc__}
"""
    return wrapper


def export_teem():
    """
    Exports things from _teem.lib, adding biff wrappers to functions where possible.
    """
    for sym_name in dir(_teem.lib):
        if 'free' == sym_name:
            # don't export C runtime's free(), though we use it above in the biff wrapper
            continue
        sym = getattr(_teem.lib, sym_name)
        # Create a python object in this module for the library symbol sym
        xprt = None
        # The exported symbol xprt will be ...
        if not sym_name in _BIFF_DICT:
            # ... either: not a function, or a function known to not use biff
            if str(sym).startswith("<cdata 'airEnum *' "):
                # _sym is name of an airEnum, wrap it as such
                xprt = Tenum(sym, sym_name)
            else:
                # straight copy of (reference to) sym
                xprt = sym
        else:
            # ... or: a Python wrapper around a function known to use biff.
            xprt = _biffer(sym, sym_name, _BIFF_DICT[sym_name])
        # can't do "if not xprt:" because, e.g. AIR_FALSE is 0 but needs to be exported
        if xprt is None:
            raise Exception(f"didn't handle symbol {sym_name}")
        globals()[sym_name] = xprt


if 'teem' == __name__:  # being imported
    try:
        import _teem
    except ModuleNotFoundError:
        print('\n*** teem.py: failed to "import _teem", the _teem extension ')
        print('*** module stored in a file named something like: ')
        print('*** _teem.cpython-platform.so.')
        print('*** Is there a build_teem.py script you can run to recompile it?\n')
        raise
    # The value of this ffi, as opposed to "from cffi import FFI; ffi = FFI()" is that it knows
    # about the various typedefs that were learned to build the CFFI wrapper, which may in turn
    # be useful for setting up calls into libteem
    ffi = _teem.ffi
    # enable access to original un-wrapped things, straight from cffi
    lib = _teem.lib
    # for slight convenience, e.g. when calling nrrdLoad with NULL (default) NrrdIoState
    NULL = _teem.ffi.NULL
    # now export/wrap everything
    export_teem()
