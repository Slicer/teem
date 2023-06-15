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

# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x, *_y = 1, 2  # NOTE: A SyntaxError here means you need python3, not python2
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
    'nrrdArrayCompare': (_equals_one, 0, b'nrrd', 'nrrd/accessors.c:519'),
    'nrrdApply1DLut': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:435'),
    'nrrdApplyMulti1DLut': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:466'),
    'nrrdApply1DRegMap': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:515'),
    'nrrdApplyMulti1DRegMap': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:546'),
    'nrrd1DIrregMapCheck': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:588'),
    'nrrd1DIrregAclCheck': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:685'),
    'nrrd1DIrregAclGenerate': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:817'),
    'nrrdApply1DIrregMap': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:882'),
    'nrrdApply1DSubstitution': (_equals_one, 0, b'nrrd', 'nrrd/apply1D.c:1056'),
    'nrrdApply2DLut': (_equals_one, 0, b'nrrd', 'nrrd/apply2D.c:299'),
    'nrrdArithGamma': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:52'),
    'nrrdArithSRGBGamma': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:140'),
    'nrrdArithUnaryOp': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:346'),
    'nrrdArithBinaryOp': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:555'),
    'nrrdArithIterBinaryOpSelect': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:641'),
    'nrrdArithIterBinaryOp': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:728'),
    'nrrdArithTernaryOp': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:878'),
    'nrrdArithIterTernaryOpSelect': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:956'),
    'nrrdArithIterTernaryOp': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:1044'),
    'nrrdArithAffine': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:1067'),
    'nrrdArithIterAffine': (_equals_one, 0, b'nrrd', 'nrrd/arith.c:1110'),
    'nrrdAxisInfoCompare': (_equals_one, 0, b'nrrd', 'nrrd/axis.c:931'),
    'nrrdOrientationReduce': (_equals_one, 0, b'nrrd', 'nrrd/axis.c:1223'),
    'nrrdMetaDataNormalize': (_equals_one, 0, b'nrrd', 'nrrd/axis.c:1268'),
    'nrrdCCFind': (_equals_one, 0, b'nrrd', 'nrrd/cc.c:287'),
    'nrrdCCAdjacency': (_equals_one, 0, b'nrrd', 'nrrd/cc.c:544'),
    'nrrdCCMerge': (_equals_one, 0, b'nrrd', 'nrrd/cc.c:644'),
    'nrrdCCRevalue': (_equals_one, 0, b'nrrd', 'nrrd/cc.c:794'),
    'nrrdCCSettle': (_equals_one, 0, b'nrrd', 'nrrd/cc.c:821'),
    'nrrdCCValid': ((lambda rv: 0 == rv), 0, b'nrrd', 'nrrd/ccmethods.c:28'),
    'nrrdCCSize': (_equals_one, 0, b'nrrd', 'nrrd/ccmethods.c:59'),
    'nrrdDeringVerboseSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:103'),
    'nrrdDeringLinearInterpSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:116'),
    'nrrdDeringVerticalSeamSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:129'),
    'nrrdDeringInputSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:142'),
    'nrrdDeringCenterSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:177'),
    'nrrdDeringClampPercSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:196'),
    'nrrdDeringClampHistoBinsSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:217'),
    'nrrdDeringRadiusScaleSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:236'),
    'nrrdDeringThetaNumSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:254'),
    'nrrdDeringRadialKernelSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:272'),
    'nrrdDeringThetaKernelSet': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:292'),
    'nrrdDeringExecute': (_equals_one, 0, b'nrrd', 'nrrd/deringNrrd.c:753'),
    'nrrdCheapMedian': (_equals_one, 0, b'nrrd', 'nrrd/filt.c:409'),
    'nrrdDistanceL2': (_equals_one, 0, b'nrrd', 'nrrd/filt.c:816'),
    'nrrdDistanceL2Biased': (_equals_one, 0, b'nrrd', 'nrrd/filt.c:828'),
    'nrrdDistanceL2Signed': (_equals_one, 0, b'nrrd', 'nrrd/filt.c:840'),
    'nrrdHisto': (_equals_one, 0, b'nrrd', 'nrrd/histogram.c:42'),
    'nrrdHistoCheck': (_equals_one, 0, b'nrrd', 'nrrd/histogram.c:162'),
    'nrrdHistoDraw': (_equals_one, 0, b'nrrd', 'nrrd/histogram.c:191'),
    'nrrdHistoAxis': (_equals_one, 0, b'nrrd', 'nrrd/histogram.c:327'),
    'nrrdHistoJoint': (_equals_one, 0, b'nrrd', 'nrrd/histogram.c:441'),
    'nrrdHistoThresholdOtsu': (_equals_one, 0, b'nrrd', 'nrrd/histogram.c:652'),
    'nrrdKernelParse': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3036'),
    'nrrdKernelSpecParse': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3216'),
    'nrrdKernelSpecSprint': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3238'),
    'nrrdKernelSprint': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3293'),
    'nrrdKernelCompare': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3311'),
    'nrrdKernelSpecCompare': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3360'),
    'nrrdKernelCheck': (_equals_one, 0, b'nrrd', 'nrrd/kernel.c:3429'),
    'nrrdConvert': (_equals_one, 0, b'nrrd', 'nrrd/map.c:236'),
    'nrrdClampConvert': (_equals_one, 0, b'nrrd', 'nrrd/map.c:256'),
    'nrrdCastClampRound': (_equals_one, 0, b'nrrd', 'nrrd/map.c:282'),
    'nrrdQuantize': (_equals_one, 0, b'nrrd', 'nrrd/map.c:304'),
    'nrrdUnquantize': (_equals_one, 0, b'nrrd', 'nrrd/map.c:476'),
    'nrrdHistoEq': (_equals_one, 0, b'nrrd', 'nrrd/map.c:613'),
    'nrrdProject': (_equals_one, 0, b'nrrd', 'nrrd/measure.c:1138'),
    'nrrdBoundarySpecCheck': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:95'),
    'nrrdBoundarySpecParse': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:119'),
    'nrrdBoundarySpecSprint': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:178'),
    'nrrdBoundarySpecCompare': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:200'),
    'nrrdBasicInfoCopy': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:540'),
    'nrrdWrap_nva': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:816'),
    'nrrdWrap_va': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:847'),
    'nrrdCopy': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:938'),
    'nrrdAlloc_nva': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:968'),
    'nrrdAlloc_va': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:1017'),
    'nrrdMaybeAlloc_nva': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:1138'),
    'nrrdMaybeAlloc_va': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:1155'),
    'nrrdCompare': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:1196'),
    'nrrdPPM': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:1382'),
    'nrrdPGM': (_equals_one, 0, b'nrrd', 'nrrd/methodsNrrd.c:1402'),
    'nrrdSpaceVectorParse': (_equals_one, 4, b'nrrd', 'nrrd/parseNrrd.c:523'),
    '_nrrdDataFNCheck': (_equals_one, 3, b'nrrd', 'nrrd/parseNrrd.c:1200'),
    'nrrdRangePercentileSet': (_equals_one, 0, b'nrrd', 'nrrd/range.c:111'),
    'nrrdRangePercentileFromStringSet': (_equals_one, 0, b'nrrd', 'nrrd/range.c:213'),
    'nrrdOneLine': (_equals_one, 0, b'nrrd', 'nrrd/read.c:74'),
    'nrrdLineSkip': (_equals_one, 0, b'nrrd', 'nrrd/read.c:238'),
    'nrrdByteSkip': (_equals_one, 0, b'nrrd', 'nrrd/read.c:334'),
    'nrrdRead': (_equals_one, 0, b'nrrd', 'nrrd/read.c:498'),
    'nrrdStringRead': (_equals_one, 0, b'nrrd', 'nrrd/read.c:518'),
    'nrrdLoad': ((lambda rv: 1 == rv or 2 == rv), 0, b'nrrd', 'nrrd/read.c:614'),
    'nrrdLoadMulti': (_equals_one, 0, b'nrrd', 'nrrd/read.c:668'),
    'nrrdInvertPerm': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:36'),
    'nrrdAxesInsert': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:88'),
    'nrrdAxesPermute': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:154'),
    'nrrdShuffle': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:308'),
    'nrrdAxesSwap': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:453'),
    'nrrdFlip': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:489'),
    'nrrdJoin': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:570'),
    'nrrdAxesSplit': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:817'),
    'nrrdAxesDelete': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:879'),
    'nrrdAxesMerge': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:931'),
    'nrrdReshape_nva': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:981'),
    'nrrdReshape_va': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:1049'),
    'nrrdBlock': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:1086'),
    'nrrdUnblock': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:1157'),
    'nrrdTile2D': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:1256'),
    'nrrdUntile2D': (_equals_one, 0, b'nrrd', 'nrrd/reorder.c:1370'),
    'nrrdResampleDefaultCenterSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:172'),
    'nrrdResampleNonExistentSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:193'),
    'nrrdResampleRangeSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:326'),
    'nrrdResampleOverrideCenterSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:345'),
    'nrrdResampleBoundarySet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:402'),
    'nrrdResamplePadValueSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:423'),
    'nrrdResampleBoundarySpecSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:440'),
    'nrrdResampleRenormalizeSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:461'),
    'nrrdResampleTypeOutSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:478'),
    'nrrdResampleRoundSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:503'),
    'nrrdResampleClampSet': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:520'),
    'nrrdResampleExecute': (_equals_one, 0, b'nrrd', 'nrrd/resampleContext.c:1458'),
    'nrrdFFTWWisdomRead': (_equals_one, 0, b'nrrd', 'nrrd/fftNrrd.c:36'),
    'nrrdFFT': (_equals_one, 0, b'nrrd', 'nrrd/fftNrrd.c:92'),
    'nrrdFFTWWisdomWrite': (_equals_one, 0, b'nrrd', 'nrrd/fftNrrd.c:289'),
    'nrrdSimpleResample': (_equals_one, 0, b'nrrd', 'nrrd/resampleNrrd.c:53'),
    'nrrdSpatialResample': (_equals_one, 0, b'nrrd', 'nrrd/resampleNrrd.c:523'),
    'nrrdSpaceSet': (_equals_one, 0, b'nrrd', 'nrrd/simple.c:85'),
    'nrrdSpaceDimensionSet': (_equals_one, 0, b'nrrd', 'nrrd/simple.c:122'),
    'nrrdSpaceOriginSet': (_equals_one, 0, b'nrrd', 'nrrd/simple.c:174'),
    'nrrdContentSet_va': (_equals_one, 0, b'nrrd', 'nrrd/simple.c:475'),
    '_nrrdCheck': (_equals_one, 3, b'nrrd', 'nrrd/simple.c:1079'),
    'nrrdCheck': (_equals_one, 0, b'nrrd', 'nrrd/simple.c:1116'),
    'nrrdSameSize': ((lambda rv: 0 == rv), 3, b'nrrd', 'nrrd/simple.c:1137'),
    'nrrdSanity': ((lambda rv: 0 == rv), 0, b'nrrd', 'nrrd/simple.c:1369'),
    'nrrdSlice': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:41'),
    'nrrdCrop': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:186'),
    'nrrdSliceSelect': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:368'),
    'nrrdSample_nva': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:580'),
    'nrrdSample_va': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:619'),
    'nrrdSimpleCrop': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:648'),
    'nrrdCropAuto': (_equals_one, 0, b'nrrd', 'nrrd/subset.c:669'),
    'nrrdSplice': (_equals_one, 0, b'nrrd', 'nrrd/superset.c:34'),
    'nrrdInset': (_equals_one, 0, b'nrrd', 'nrrd/superset.c:159'),
    'nrrdPad_va': (_equals_one, 0, b'nrrd', 'nrrd/superset.c:283'),
    'nrrdPad_nva': (_equals_one, 0, b'nrrd', 'nrrd/superset.c:489'),
    'nrrdSimplePad_va': (_equals_one, 0, b'nrrd', 'nrrd/superset.c:517'),
    'nrrdSimplePad_nva': (_equals_one, 0, b'nrrd', 'nrrd/superset.c:555'),
    'nrrdIoStateSet': (_equals_one, 0, b'nrrd', 'nrrd/write.c:33'),
    'nrrdIoStateEncodingSet': (_equals_one, 0, b'nrrd', 'nrrd/write.c:106'),
    'nrrdIoStateFormatSet': (_equals_one, 0, b'nrrd', 'nrrd/write.c:126'),
    'nrrdWrite': (_equals_one, 0, b'nrrd', 'nrrd/write.c:946'),
    'nrrdStringWrite': (_equals_one, 0, b'nrrd', 'nrrd/write.c:962'),
    'nrrdSave': (_equals_one, 0, b'nrrd', 'nrrd/write.c:983'),
    'nrrdSaveMulti': (_equals_one, 0, b'nrrd', 'nrrd/write.c:1036'),
    'ell_Nm_check': (_equals_one, 0, b'ell', 'ell/genmat.c:27'),
    'ell_Nm_tran': (_equals_one, 0, b'ell', 'ell/genmat.c:61'),
    'ell_Nm_mul': (_equals_one, 0, b'ell', 'ell/genmat.c:106'),
    'ell_Nm_inv': (_equals_one, 0, b'ell', 'ell/genmat.c:340'),
    'ell_Nm_pseudo_inv': (_equals_one, 0, b'ell', 'ell/genmat.c:381'),
    'ell_Nm_wght_pseudo_inv': (_equals_one, 0, b'ell', 'ell/genmat.c:415'),
    'ell_q_avg4_d': (_equals_one, 0, b'ell', 'ell/quat.c:473'),
    'ell_q_avgN_d': (_equals_one, 0, b'ell', 'ell/quat.c:541'),
    'mossImageCheck': (_equals_one, 0, b'moss', 'moss/methodsMoss.c:75'),
    'mossImageAlloc': (_equals_one, 0, b'moss', 'moss/methodsMoss.c:96'),
    'mossSamplerImageSet': (_equals_one, 0, b'moss', 'moss/sampler.c:28'),
    'mossSamplerKernelSet': (_equals_one, 0, b'moss', 'moss/sampler.c:80'),
    'mossSamplerUpdate': (_equals_one, 0, b'moss', 'moss/sampler.c:102'),
    'mossSamplerSample': (_equals_one, 0, b'moss', 'moss/sampler.c:197'),
    'mossLinearTransform': (_equals_one, 0, b'moss', 'moss/xform.c:142'),
    'mossFourPointTransform': (_equals_one, 0, b'moss', 'moss/xform.c:221'),
    'alanUpdate': (_equals_one, 0, b'alan', 'alan/coreAlan.c:62'),
    'alanInit': (_equals_one, 0, b'alan', 'alan/coreAlan.c:105'),
    'alanRun': (_equals_one, 0, b'alan', 'alan/coreAlan.c:458'),
    'alanDimensionSet': (_equals_one, 0, b'alan', 'alan/methodsAlan.c:106'),
    'alan2DSizeSet': (_equals_one, 0, b'alan', 'alan/methodsAlan.c:121'),
    'alan3DSizeSet': (_equals_one, 0, b'alan', 'alan/methodsAlan.c:141'),
    'alanTensorSet': (_equals_one, 0, b'alan', 'alan/methodsAlan.c:162'),
    'alanParmSet': (_equals_one, 0, b'alan', 'alan/methodsAlan.c:209'),
    'gageContextCopy': (_equals_null, 0, b'gage', 'gage/ctx.c:90'),
    'gageKernelSet': (_equals_one, 0, b'gage', 'gage/ctx.c:201'),
    'gagePerVolumeAttach': (_equals_one, 0, b'gage', 'gage/ctx.c:400'),
    'gagePerVolumeDetach': (_equals_one, 0, b'gage', 'gage/ctx.c:459'),
    'gageDeconvolve': (_equals_one, 0, b'gage', 'gage/deconvolve.c:28'),
    'gageDeconvolveSeparable': (_equals_one, 0, b'gage', 'gage/deconvolve.c:210'),
    'gageKindCheck': (_equals_one, 0, b'gage', 'gage/kind.c:35'),
    'gageKindVolumeCheck': (_equals_one, 0, b'gage', 'gage/kind.c:220'),
    'gageVolumeCheck': (_equals_one, 0, b'gage', 'gage/pvl.c:38'),
    'gagePerVolumeNew': (_equals_null, 0, b'gage', 'gage/pvl.c:59'),
    'gageQueryReset': (_equals_one, 0, b'gage', 'gage/pvl.c:263'),
    'gageQuerySet': (_equals_one, 0, b'gage', 'gage/pvl.c:289'),
    'gageQueryAdd': (_equals_one, 0, b'gage', 'gage/pvl.c:345'),
    'gageQueryItemOn': (_equals_one, 0, b'gage', 'gage/pvl.c:363'),
    'gageShapeSet': (_equals_one, 0, b'gage', 'gage/shape.c:407'),
    'gageShapeEqual': ((lambda rv: 0 == rv), 0, b'gage', 'gage/shape.c:470'),
    'gageStructureTensor': (_equals_one, 0, b'gage', 'gage/st.c:85'),
    'gageStackPerVolumeNew': (_equals_one, 0, b'gage', 'gage/stack.c:100'),
    'gageStackPerVolumeAttach': (_equals_one, 0, b'gage', 'gage/stack.c:129'),
    'gageStackBlurParmCompare': (_equals_one, 0, b'gage', 'gage/stackBlur.c:127'),
    'gageStackBlurParmCopy': (_equals_one, 0, b'gage', 'gage/stackBlur.c:232'),
    'gageStackBlurParmSigmaSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:269'),
    'gageStackBlurParmScaleSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:363'),
    'gageStackBlurParmKernelSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:387'),
    'gageStackBlurParmRenormalizeSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:400'),
    'gageStackBlurParmBoundarySet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:412'),
    'gageStackBlurParmBoundarySpecSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:431'),
    'gageStackBlurParmOneDimSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:448'),
    'gageStackBlurParmNeedSpatialBlurSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:460'),
    'gageStackBlurParmVerboseSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:472'),
    'gageStackBlurParmDgGoodSigmaMaxSet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:484'),
    'gageStackBlurParmCheck': (_equals_one, 0, b'gage', 'gage/stackBlur.c:500'),
    'gageStackBlurParmParse': (_equals_one, 0, b'gage', 'gage/stackBlur.c:547'),
    'gageStackBlurParmSprint': (_equals_one, 0, b'gage', 'gage/stackBlur.c:806'),
    'gageStackBlur': (_equals_one, 0, b'gage', 'gage/stackBlur.c:1388'),
    'gageStackBlurCheck': (_equals_one, 0, b'gage', 'gage/stackBlur.c:1491'),
    'gageStackBlurGet': (_equals_one, 0, b'gage', 'gage/stackBlur.c:1599'),
    'gageStackBlurManage': (_equals_one, 0, b'gage', 'gage/stackBlur.c:1700'),
    'gageUpdate': (_equals_one, 0, b'gage', 'gage/update.c:315'),
    'gageOptimSigSet': (_equals_one, 0, b'gage', 'gage/optimsig.c:219'),
    'gageOptimSigContextNew': (_equals_null, 0, b'gage', 'gage/optimsig.c:313'),
    'gageOptimSigCalculate': (_equals_one, 0, b'gage', 'gage/optimsig.c:1094'),
    'gageOptimSigErrorPlot': (_equals_one, 0, b'gage', 'gage/optimsig.c:1166'),
    'gageOptimSigErrorPlotSliding': (_equals_one, 0, b'gage', 'gage/optimsig.c:1257'),
    'dyeConvert': (_equals_one, 0, b'dye', 'dye/convertDye.c:353'),
    'dyeColorParse': (_equals_one, 0, b'dye', 'dye/methodsDye.c:187'),
    'baneClipNew': (_equals_null, 0, b'bane', 'bane/clip.c:104'),
    'baneClipAnswer': (_equals_one, 0, b'bane', 'bane/clip.c:154'),
    'baneClipCopy': (_equals_null, 0, b'bane', 'bane/clip.c:169'),
    'baneFindInclusion': (_equals_one, 0, b'bane', 'bane/hvol.c:89'),
    'baneMakeHVol': (_equals_one, 0, b'bane', 'bane/hvol.c:251'),
    'baneGKMSHVol': (_equals_null, 0, b'bane', 'bane/hvol.c:450'),
    'baneIncNew': (_equals_null, 0, b'bane', 'bane/inc.c:253'),
    'baneIncAnswer': (_equals_one, 0, b'bane', 'bane/inc.c:362'),
    'baneIncCopy': (_equals_null, 0, b'bane', 'bane/inc.c:377'),
    'baneMeasrNew': (_equals_null, 0, b'bane', 'bane/measr.c:35'),
    'baneMeasrCopy': (_equals_null, 0, b'bane', 'bane/measr.c:151'),
    'baneRangeNew': (_equals_null, 0, b'bane', 'bane/rangeBane.c:91'),
    'baneRangeCopy': (_equals_null, 0, b'bane', 'bane/rangeBane.c:132'),
    'baneRangeAnswer': (_equals_one, 0, b'bane', 'bane/rangeBane.c:146'),
    'baneRawScatterplots': (_equals_one, 0, b'bane', 'bane/scat.c:28'),
    'baneOpacInfo': (_equals_one, 0, b'bane', 'bane/trnsf.c:31'),
    'bane1DOpacInfoFrom2D': (_equals_one, 0, b'bane', 'bane/trnsf.c:147'),
    'baneSigmaCalc': (_equals_one, 0, b'bane', 'bane/trnsf.c:226'),
    'banePosCalc': (_equals_one, 0, b'bane', 'bane/trnsf.c:257'),
    'baneOpacCalc': (_equals_one, 0, b'bane', 'bane/trnsf.c:408'),
    'baneInputCheck': (_equals_one, 0, b'bane', 'bane/valid.c:28'),
    'baneHVolCheck': (_equals_one, 0, b'bane', 'bane/valid.c:66'),
    'baneInfoCheck': (_equals_one, 0, b'bane', 'bane/valid.c:108'),
    'banePosCheck': (_equals_one, 0, b'bane', 'bane/valid.c:146'),
    'baneBcptsCheck': (_equals_one, 0, b'bane', 'bane/valid.c:181'),
    'limnCameraUpdate': (_equals_one, 0, b'limn', 'limn/cam.c:35'),
    'limnCameraAspectSet': (_equals_one, 0, b'limn', 'limn/cam.c:132'),
    'limnCameraPathMake': (_equals_one, 0, b'limn', 'limn/cam.c:191'),
    'limnEnvMapFill': (_equals_one, 0, b'limn', 'limn/envmap.c:27'),
    'limnEnvMapCheck': (_equals_one, 0, b'limn', 'limn/envmap.c:121'),
    'limnObjectWriteOFF': (_equals_one, 0, b'limn', 'limn/io.c:81'),
    'limnPolyDataWriteIV': (_equals_one, 0, b'limn', 'limn/io.c:140'),
    'limnObjectReadOFF': (_equals_one, 0, b'limn', 'limn/io.c:266'),
    'limnPolyDataWriteLMPD': (_equals_one, 0, b'limn', 'limn/io.c:457'),
    'limnPolyDataReadLMPD': (_equals_one, 0, b'limn', 'limn/io.c:584'),
    'limnPolyDataWriteVTK': (_equals_one, 0, b'limn', 'limn/io.c:967'),
    'limnPolyDataReadOFF': (_equals_one, 0, b'limn', 'limn/io.c:1057'),
    'limnPolyDataSave': (_equals_one, 0, b'limn', 'limn/io.c:1162'),
    'limnLightUpdate': (_equals_one, 0, b'limn', 'limn/light.c:69'),
    'limnPolyDataAlloc': (_equals_one, 0, b'limn', 'limn/polydata.c:151'),
    'limnPolyDataCopy': (_equals_one, 0, b'limn', 'limn/polydata.c:230'),
    'limnPolyDataCopyN': (_equals_one, 0, b'limn', 'limn/polydata.c:262'),
    'limnPolyDataPrimitiveVertexNumber': (_equals_one, 0, b'limn', 'limn/polydata.c:553'),
    'limnPolyDataPrimitiveArea': (_equals_one, 0, b'limn', 'limn/polydata.c:575'),
    'limnPolyDataRasterize': (_equals_one, 0, b'limn', 'limn/polydata.c:633'),
    'limnPolyDataSpiralTubeWrap': (_equals_one, 0, b'limn', 'limn/polyfilter.c:28'),
    'limnPolyDataSmoothHC': ((lambda rv: -1 == rv), 0, b'limn', 'limn/polyfilter.c:338'),
    'limnPolyDataVertexWindingFix': (_equals_one, 0, b'limn', 'limn/polymod.c:1233'),
    'limnPolyDataCCFind': (_equals_one, 0, b'limn', 'limn/polymod.c:1252'),
    'limnPolyDataPrimitiveSort': (_equals_one, 0, b'limn', 'limn/polymod.c:1383'),
    'limnPolyDataVertexWindingFlip': (_equals_one, 0, b'limn', 'limn/polymod.c:1466'),
    'limnPolyDataPrimitiveSelect': (_equals_one, 0, b'limn', 'limn/polymod.c:1495'),
    'limnPolyDataClipMulti': (_equals_one, 0, b'limn', 'limn/polymod.c:1710'),
    'limnPolyDataCompress': (_equals_null, 0, b'limn', 'limn/polymod.c:1997'),
    'limnPolyDataJoin': (_equals_null, 0, b'limn', 'limn/polymod.c:2087'),
    'limnPolyDataEdgeHalve': (_equals_one, 0, b'limn', 'limn/polymod.c:2155'),
    'limnPolyDataNeighborList': ((lambda rv: -1 == rv), 0, b'limn', 'limn/polymod.c:2332'),
    'limnPolyDataNeighborArray': ((lambda rv: -1 == rv), 0, b'limn', 'limn/polymod.c:2428'),
    'limnPolyDataNeighborArrayComp': ((lambda rv: -1 == rv), 0, b'limn', 'limn/polymod.c:2468'),
    'limnPolyDataCube': (_equals_one, 0, b'limn', 'limn/polyshapes.c:29'),
    'limnPolyDataCubeTriangles': (_equals_one, 0, b'limn', 'limn/polyshapes.c:139'),
    'limnPolyDataOctahedron': (_equals_one, 0, b'limn', 'limn/polyshapes.c:349'),
    'limnPolyDataCylinder': (_equals_one, 0, b'limn', 'limn/polyshapes.c:463'),
    'limnPolyDataCone': (_equals_one, 0, b'limn', 'limn/polyshapes.c:637'),
    'limnPolyDataSuperquadric': (_equals_one, 0, b'limn', 'limn/polyshapes.c:736'),
    'limnPolyDataSpiralBetterquadric': (_equals_one, 0, b'limn', 'limn/polyshapes.c:861'),
    'limnPolyDataSpiralSuperquadric': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1018'),
    'limnPolyDataPolarSphere': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1036'),
    'limnPolyDataSpiralSphere': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1048'),
    'limnPolyDataIcoSphere': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1099'),
    'limnPolyDataPlane': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1343'),
    'limnPolyDataSquare': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1398'),
    'limnPolyDataSuperquadric2D': (_equals_one, 0, b'limn', 'limn/polyshapes.c:1441'),
    'limnQNDemo': (_equals_one, 0, b'limn', 'limn/qn.c:894'),
    'limnObjectRender': (_equals_one, 0, b'limn', 'limn/renderLimn.c:27'),
    'limnObjectPSDraw': (_equals_one, 0, b'limn', 'limn/renderLimn.c:186'),
    'limnObjectPSDrawConcave': (_equals_one, 0, b'limn', 'limn/renderLimn.c:316'),
    'limnSplineNrrdEvaluate': (_equals_one, 0, b'limn', 'limn/splineEval.c:325'),
    'limnSplineSample': (_equals_one, 0, b'limn', 'limn/splineEval.c:363'),
    'limnSplineTypeSpecNew': (_equals_null, 0, b'limn', 'limn/splineMethods.c:27'),
    'limnSplineNew': (_equals_null, 0, b'limn', 'limn/splineMethods.c:126'),
    'limnSplineNrrdCleverFix': (_equals_one, 0, b'limn', 'limn/splineMethods.c:251'),
    'limnSplineCleverNew': (_equals_null, 0, b'limn', 'limn/splineMethods.c:396'),
    'limnSplineUpdate': (_equals_one, 0, b'limn', 'limn/splineMethods.c:424'),
    'limnSplineTypeSpecParse': (_equals_null, 0, b'limn', 'limn/splineMisc.c:224'),
    'limnSplineParse': (_equals_null, 0, b'limn', 'limn/splineMisc.c:280'),
    'limnCBFCheck': (_equals_one, 0, b'limn', 'limn/splineFit.c:589'),
    'limnCBFitSingle': (_equals_one, 0, b'limn', 'limn/splineFit.c:860'),
    'limnCBFMulti': (_equals_one, 0, b'limn', 'limn/splineFit.c:951'),
    'limnCBFCorners': (_equals_one, 0, b'limn', 'limn/splineFit.c:1053'),
    'limnCBFit': (_equals_one, 0, b'limn', 'limn/splineFit.c:1123'),
    'limnObjectWorldHomog': (_equals_one, 0, b'limn', 'limn/transform.c:27'),
    'limnObjectFaceNormals': (_equals_one, 0, b'limn', 'limn/transform.c:49'),
    'limnObjectSpaceTransform': (_equals_one, 0, b'limn', 'limn/transform.c:212'),
    'limnObjectFaceReverse': (_equals_one, 0, b'limn', 'limn/transform.c:337'),
    'echoThreadStateInit': (_equals_one, 0, b'echo', 'echo/renderEcho.c:28'),
    'echoRTRenderCheck': (_equals_one, 0, b'echo', 'echo/renderEcho.c:138'),
    'echoRTRender': (_equals_one, 0, b'echo', 'echo/renderEcho.c:413'),
    'hooverContextCheck': (_equals_one, 0, b'hoover', 'hoover/methodsHoover.c:55'),
    'hooverRender': ((lambda rv: _teem.lib.hooverErrInit == rv), 0, b'hoover', 'hoover/rays.c:361'),
    'seekExtract': (_equals_one, 0, b'seek', 'seek/extract.c:926'),
    'seekDataSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:58'),
    'seekSamplesSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:118'),
    'seekTypeSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:151'),
    'seekLowerInsideSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:175'),
    'seekNormalsFindSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:195'),
    'seekStrengthUseSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:210'),
    'seekStrengthSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:225'),
    'seekItemScalarSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:287'),
    'seekItemStrengthSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:306'),
    'seekItemHessSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:325'),
    'seekItemGradientSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:345'),
    'seekItemNormalSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:366'),
    'seekItemEigensystemSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:387'),
    'seekIsovalueSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:416'),
    'seekEvalDiffThreshSet': (_equals_one, 0, b'seek', 'seek/setSeek.c:442'),
    'seekVertexStrength': (_equals_one, 0, b'seek', 'seek/textract.c:1886'),
    'seekUpdate': (_equals_one, 0, b'seek', 'seek/updateSeek.c:677'),
    'tenAnisoPlot': (_equals_one, 0, b'ten', 'ten/aniso.c:1070'),
    'tenAnisoVolume': (_equals_one, 0, b'ten', 'ten/aniso.c:1130'),
    'tenAnisoHistogram': (_equals_one, 0, b'ten', 'ten/aniso.c:1202'),
    'tenEvecRGBParmCheck': (_equals_one, 0, b'ten', 'ten/aniso.c:1317'),
    'tenEMBimodal': (_equals_one, 0, b'ten', 'ten/bimod.c:414'),
    'tenBVecNonLinearFit': (_equals_one, 0, b'ten', 'ten/bvec.c:101'),
    'tenDWMRIKeyValueParse': (_equals_one, 0, b'ten', 'ten/chan.c:62'),
    'tenBMatrixCalc': (_equals_one, 0, b'ten', 'ten/chan.c:350'),
    'tenEMatrixCalc': (_equals_one, 0, b'ten', 'ten/chan.c:391'),
    'tenEstimateLinear3D': (_equals_one, 0, b'ten', 'ten/chan.c:584'),
    'tenEstimateLinear4D': (_equals_one, 0, b'ten', 'ten/chan.c:631'),
    'tenSimulate': (_equals_one, 0, b'ten', 'ten/chan.c:872'),
    'tenEpiRegister3D': (_equals_one, 0, b'ten', 'ten/epireg.c:1051'),
    'tenEpiRegister4D': (_equals_one, 0, b'ten', 'ten/epireg.c:1202'),
    'tenEstimateMethodSet': (_equals_one, 0, b'ten', 'ten/estimate.c:285'),
    'tenEstimateSigmaSet': (_equals_one, 0, b'ten', 'ten/estimate.c:307'),
    'tenEstimateValueMinSet': (_equals_one, 0, b'ten', 'ten/estimate.c:325'),
    'tenEstimateGradientsSet': (_equals_one, 0, b'ten', 'ten/estimate.c:343'),
    'tenEstimateBMatricesSet': (_equals_one, 0, b'ten', 'ten/estimate.c:370'),
    'tenEstimateSkipSet': (_equals_one, 0, b'ten', 'ten/estimate.c:397'),
    'tenEstimateSkipReset': (_equals_one, 0, b'ten', 'ten/estimate.c:415'),
    'tenEstimateThresholdFind': (_equals_one, 0, b'ten', 'ten/estimate.c:430'),
    'tenEstimateThresholdSet': (_equals_one, 0, b'ten', 'ten/estimate.c:498'),
    'tenEstimateUpdate': (_equals_one, 0, b'ten', 'ten/estimate.c:806'),
    'tenEstimate1TensorSimulateSingle_f': (_equals_one, 0, b'ten', 'ten/estimate.c:980'),
    'tenEstimate1TensorSimulateSingle_d': (_equals_one, 0, b'ten', 'ten/estimate.c:1008'),
    'tenEstimate1TensorSimulateVolume': (_equals_one, 0, b'ten', 'ten/estimate.c:1039'),
    'tenEstimate1TensorSingle_f': (_equals_one, 0, b'ten', 'ten/estimate.c:1744'),
    'tenEstimate1TensorSingle_d': (_equals_one, 0, b'ten', 'ten/estimate.c:1772'),
    'tenEstimate1TensorVolume4D': (_equals_one, 0, b'ten', 'ten/estimate.c:1809'),
    'tenFiberTraceSet': (_equals_one, 0, b'ten', 'ten/fiber.c:830'),
    'tenFiberTrace': (_equals_one, 0, b'ten', 'ten/fiber.c:850'),
    'tenFiberDirectionNumber': ((lambda rv: 0 == rv), 0, b'ten', 'ten/fiber.c:870'),
    'tenFiberSingleTrace': (_equals_one, 0, b'ten', 'ten/fiber.c:919'),
    'tenFiberMultiNew': (_equals_null, 0, b'ten', 'ten/fiber.c:962'),
    'tenFiberMultiTrace': (_equals_one, 0, b'ten', 'ten/fiber.c:1027'),
    'tenFiberMultiPolyData': (_equals_one, 0, b'ten', 'ten/fiber.c:1247'),
    'tenFiberMultiProbeVals': (_equals_one, 0, b'ten', 'ten/fiber.c:1258'),
    'tenFiberContextDwiNew': (_equals_null, 0, b'ten', 'ten/fiberMethods.c:212'),
    'tenFiberContextNew': (_equals_null, 0, b'ten', 'ten/fiberMethods.c:226'),
    'tenFiberTypeSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:250'),
    'tenFiberStopSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:380'),
    'tenFiberStopAnisoSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:556'),
    'tenFiberStopDoubleSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:568'),
    'tenFiberStopUIntSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:592'),
    'tenFiberAnisoSpeedSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:639'),
    'tenFiberAnisoSpeedReset': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:704'),
    'tenFiberKernelSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:719'),
    'tenFiberProbeItemSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:738'),
    'tenFiberIntgSet': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:750'),
    'tenFiberUpdate': (_equals_one, 0, b'ten', 'ten/fiberMethods.c:793'),
    'tenGlyphParmCheck': (_equals_one, 0, b'ten', 'ten/glyph.c:74'),
    'tenGlyphGen': (_equals_one, 0, b'ten', 'ten/glyph.c:175'),
    'tenGradientCheck': (_equals_one, 0, b'ten', 'ten/grads.c:69'),
    'tenGradientRandom': (_equals_one, 0, b'ten', 'ten/grads.c:108'),
    'tenGradientJitter': (_equals_one, 0, b'ten', 'ten/grads.c:154'),
    'tenGradientBalance': (_equals_one, 0, b'ten', 'ten/grads.c:376'),
    'tenGradientDistribute': (_equals_one, 0, b'ten', 'ten/grads.c:461'),
    'tenGradientGenerate': (_equals_one, 0, b'ten', 'ten/grads.c:654'),
    'tenEvecRGB': (_equals_one, 0, b'ten', 'ten/miscTen.c:28'),
    'tenEvqVolume': (_equals_one, 0, b'ten', 'ten/miscTen.c:153'),
    'tenBMatrixCheck': (_equals_one, 0, b'ten', 'ten/miscTen.c:214'),
    '_tenFindValley': (_equals_one, 0, b'ten', 'ten/miscTen.c:258'),
    'tenSizeNormalize': (_equals_one, 0, b'ten', 'ten/mod.c:223'),
    'tenSizeScale': (_equals_one, 0, b'ten', 'ten/mod.c:239'),
    'tenAnisoScale': (_equals_one, 0, b'ten', 'ten/mod.c:257'),
    'tenEigenvalueClamp': (_equals_one, 0, b'ten', 'ten/mod.c:277'),
    'tenEigenvaluePower': (_equals_one, 0, b'ten', 'ten/mod.c:296'),
    'tenEigenvalueAdd': (_equals_one, 0, b'ten', 'ten/mod.c:314'),
    'tenEigenvalueMultiply': (_equals_one, 0, b'ten', 'ten/mod.c:332'),
    'tenLog': (_equals_one, 0, b'ten', 'ten/mod.c:350'),
    'tenExp': (_equals_one, 0, b'ten', 'ten/mod.c:367'),
    'tenInterpParmBufferAlloc': (_equals_one, 0, b'ten', 'ten/path.c:66'),
    'tenInterpParmCopy': (_equals_null, 0, b'ten', 'ten/path.c:125'),
    'tenInterpN_d': (_equals_one, 0, b'ten', 'ten/path.c:307'),
    'tenInterpTwoDiscrete_d': (_equals_one, 0, b'ten', 'ten/path.c:808'),
    'tenInterpMulti3D': (_equals_one, 0, b'ten', 'ten/path.c:959'),
    'tenDwiGageKindSet': (_equals_one, 0, b'ten', 'ten/tenDwiGage.c:1039'),
    'tenDwiGageKindCheck': (_equals_one, 0, b'ten', 'ten/tenDwiGage.c:1179'),
    'tenTensorCheck': (_equals_one, 4, b'ten', 'ten/tensor.c:56'),
    'tenMeasurementFrameReduce': (_equals_one, 0, b'ten', 'ten/tensor.c:89'),
    'tenExpand2D': (_equals_one, 0, b'ten', 'ten/tensor.c:159'),
    'tenExpand': (_equals_one, 0, b'ten', 'ten/tensor.c:233'),
    'tenShrink': (_equals_one, 0, b'ten', 'ten/tensor.c:289'),
    'tenMake': (_equals_one, 0, b'ten', 'ten/tensor.c:531'),
    'tenSlice': (_equals_one, 0, b'ten', 'ten/tensor.c:633'),
    'tenTripleCalc': (_equals_one, 0, b'ten', 'ten/triple.c:417'),
    'tenTripleConvert': (_equals_one, 0, b'ten', 'ten/triple.c:475'),
    'tenExperSpecGradSingleBValSet': (_equals_one, 0, b'ten', 'ten/experSpec.c:65'),
    'tenExperSpecGradBValSet': (_equals_one, 0, b'ten', 'ten/experSpec.c:106'),
    'tenExperSpecFromKeyValueSet': (_equals_one, 0, b'ten', 'ten/experSpec.c:175'),
    'tenDWMRIKeyValueFromExperSpecSet': (_equals_one, 0, b'ten', 'ten/experSpec.c:330'),
    'tenModelParse': (_equals_one, 0, b'ten', 'ten/tenModel.c:65'),
    'tenModelFromAxisLearn': (_equals_one, 0, b'ten', 'ten/tenModel.c:126'),
    'tenModelSimulate': (_equals_one, 0, b'ten', 'ten/tenModel.c:164'),
    'tenModelSqeFit': (_equals_one, 0, b'ten', 'ten/tenModel.c:412'),
    'tenModelConvert': (_equals_one, 0, b'ten', 'ten/tenModel.c:687'),
    'pullEnergyPlot': (_equals_one, 0, b'pull', 'pull/actionPull.c:234'),
    'pullBinProcess': (_equals_one, 0, b'pull', 'pull/actionPull.c:1108'),
    'pullGammaLearn': (_equals_one, 0, b'pull', 'pull/actionPull.c:1143'),
    'pullBinsPointAdd': (_equals_one, 0, b'pull', 'pull/binningPull.c:185'),
    'pullBinsPointMaybeAdd': (_equals_one, 0, b'pull', 'pull/binningPull.c:207'),
    'pullOutputGetFilter': (_equals_one, 0, b'pull', 'pull/contextPull.c:383'),
    'pullOutputGet': (_equals_one, 0, b'pull', 'pull/contextPull.c:579'),
    'pullPropGet': (_equals_one, 0, b'pull', 'pull/contextPull.c:592'),
    'pullPositionHistoryNrrdGet': (_equals_one, 0, b'pull', 'pull/contextPull.c:770'),
    'pullPositionHistoryPolydataGet': (_equals_one, 0, b'pull', 'pull/contextPull.c:842'),
    'pullIterParmSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:106'),
    'pullSysParmSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:195'),
    'pullFlagSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:274'),
    'pullVerboseSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:349'),
    'pullThreadNumSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:374'),
    'pullRngSeedSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:386'),
    'pullProgressBinModSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:398'),
    'pullCallbackSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:410'),
    'pullInterEnergySet': (_equals_one, 0, b'pull', 'pull/parmPull.c:435'),
    'pullLogAddSet': (_equals_one, 0, b'pull', 'pull/parmPull.c:496'),
    'pullInitRandomSet': (_equals_one, 0, b'pull', 'pull/initPull.c:111'),
    'pullInitHaltonSet': (_equals_one, 0, b'pull', 'pull/initPull.c:129'),
    'pullInitPointPerVoxelSet': (_equals_one, 0, b'pull', 'pull/initPull.c:148'),
    'pullInitGivenPosSet': (_equals_one, 0, b'pull', 'pull/initPull.c:176'),
    'pullInitLiveThreshUseSet': (_equals_one, 0, b'pull', 'pull/initPull.c:190'),
    'pullInitUnequalShapesAllowSet': (_equals_one, 0, b'pull', 'pull/initPull.c:203'),
    'pullStart': (_equals_one, 0, b'pull', 'pull/corePull.c:115'),
    'pullFinish': (_equals_one, 0, b'pull', 'pull/corePull.c:170'),
    'pullRun': (_equals_one, 0, b'pull', 'pull/corePull.c:338'),
    'pullEnergySpecParse': (_equals_one, 0, b'pull', 'pull/energy.c:629'),
    'pullInfoSpecAdd': (_equals_one, 0, b'pull', 'pull/infoPull.c:134'),
    'pullInfoGet': (_equals_one, 0, b'pull', 'pull/infoPull.c:406'),
    'pullInfoSpecSprint': (_equals_one, 0, b'pull', 'pull/infoPull.c:451'),
    'pullPointNew': (_equals_null, 0, b'pull', 'pull/pointPull.c:35'),
    'pullProbe': (_equals_one, 0, b'pull', 'pull/pointPull.c:360'),
    'pullPointInitializePerVoxel': (_equals_one, 0, b'pull', 'pull/pointPull.c:639'),
    'pullPointInitializeRandomOrHalton': (_equals_one, 0, b'pull', 'pull/pointPull.c:824'),
    'pullPointInitializeGivenPos': (_equals_one, 0, b'pull', 'pull/pointPull.c:993'),
    'pullVolumeSingleAdd': (_equals_one, 0, b'pull', 'pull/volumePull.c:214'),
    'pullVolumeStackAdd': (_equals_one, 0, b'pull', 'pull/volumePull.c:240'),
    'pullVolumeLookup': (_equals_null, 0, b'pull', 'pull/volumePull.c:477'),
    'pullConstraintScaleRange': (_equals_one, 0, b'pull', 'pull/volumePull.c:496'),
    'pullCCFind': (_equals_one, 0, b'pull', 'pull/ccPull.c:32'),
    'pullCCMeasure': (_equals_one, 0, b'pull', 'pull/ccPull.c:116'),
    'pullCCSort': (_equals_one, 0, b'pull', 'pull/ccPull.c:211'),
    'pullTraceSet': (_equals_one, 0, b'pull', 'pull/trace.c:247'),
    'pullTraceMultiAdd': (_equals_one, 0, b'pull', 'pull/trace.c:676'),
    'pullTraceMultiPlotAdd': (_equals_one, 0, b'pull', 'pull/trace.c:706'),
    'pullTraceMultiWrite': (_equals_one, 0, b'pull', 'pull/trace.c:1016'),
    'pullTraceMultiRead': (_equals_one, 0, b'pull', 'pull/trace.c:1121'),
    'coilStart': (_equals_one, 0, b'coil', 'coil/coreCoil.c:289'),
    'coilIterate': (_equals_one, 0, b'coil', 'coil/coreCoil.c:364'),
    'coilFinish': (_equals_one, 0, b'coil', 'coil/coreCoil.c:409'),
    'coilVolumeCheck': (_equals_one, 0, b'coil', 'coil/methodsCoil.c:27'),
    'coilContextAllSet': (_equals_one, 0, b'coil', 'coil/methodsCoil.c:71'),
    'coilOutputGet': (_equals_one, 0, b'coil', 'coil/methodsCoil.c:202'),
    'pushOutputGet': (_equals_one, 0, b'push', 'push/action.c:73'),
    'pushBinProcess': (_equals_one, 0, b'push', 'push/action.c:163'),
    'pushBinPointAdd': (_equals_one, 0, b'push', 'push/binning.c:182'),
    'pushRebin': (_equals_one, 0, b'push', 'push/binning.c:199'),
    'pushStart': (_equals_one, 0, b'push', 'push/corePush.c:185'),
    'pushIterate': (_equals_one, 0, b'push', 'push/corePush.c:235'),
    'pushRun': (_equals_one, 0, b'push', 'push/corePush.c:308'),
    'pushFinish': (_equals_one, 0, b'push', 'push/corePush.c:398'),
    'pushEnergySpecParse': (_equals_one, 0, b'push', 'push/forces.c:306'),
    'miteSample': (_math.isnan, 0, b'mite', 'mite/ray.c:153'),
    'miteRenderBegin': (_equals_one, 0, b'mite', 'mite/renderMite.c:65'),
    'miteShadeSpecParse': (_equals_one, 0, b'mite', 'mite/shade.c:71'),
    'miteThreadNew': (_equals_null, 0, b'mite', 'mite/thread.c:28'),
    'miteThreadBegin': (_equals_one, 0, b'mite', 'mite/thread.c:94'),
    'miteVariableParse': (_equals_one, 0, b'mite', 'mite/txf.c:103'),
    'miteNtxfCheck': (_equals_one, 0, b'mite', 'mite/txf.c:234'),
    'meetAirEnumAllCheck': (_equals_one, 0, b'meet', 'meet/enumall.c:228'),
    'meetNrrdKernelAllCheck': (_equals_one, 0, b'meet', 'meet/meetNrrd.c:234'),
    'meetPullVolCopy': (_equals_null, 0, b'meet', 'meet/meetPull.c:46'),
    'meetPullVolParse': (_equals_one, 0, b'meet', 'meet/meetPull.c:102'),
    'meetPullVolLeechable': (_equals_one, 0, b'meet', 'meet/meetPull.c:316'),
    'meetPullVolStackBlurParmFinishMulti': (_equals_one, 0, b'meet', 'meet/meetPull.c:429'),
    'meetPullVolLoadMulti': (_equals_one, 0, b'meet', 'meet/meetPull.c:474'),
    'meetPullVolAddMulti': (_equals_one, 0, b'meet', 'meet/meetPull.c:554'),
    'meetPullInfoParse': (_equals_one, 0, b'meet', 'meet/meetPull.c:636'),
    'meetPullInfoAddMulti': (_equals_one, 0, b'meet', 'meet/meetPull.c:767'),
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
