#
# Teem: Tools to process and visualize scientific data and images
# Copyright (C) 2009--2022  University of Chicago
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
A python module to wrap the CFFI-generated _teem module (which links into the libteem
shared library). Main utility is converting calls into Teem functions that generated
a biff error message into a Python exception containing the error message.  Also
introduces the Tenum object for wrapping an airEnum, and eventually other ways of
making pythonic interfaces to Teem functionality.  See teem/python/cffi/README.md.
"""

# pylint can't see inside CFFI-generated modules, and
# long BIFFDICT lines don't need to be human-friendly
# pylint: disable=c-extension-no-member, line-too-long

import math as _math  # for isnan test that may appear in _BIFFDICT
import sys as _sys

# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x, *_y = 1, 2  # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y


class Tenum:
    """A helper/wrapper around airEnums (or pointers to them) in Teem, which provides
    convenient ways to convert between integer enum values and real Python strings.
    The C airEnum underlying the Python Tenum foo is still available as foo().
    """

    def __init__(self, aenm, _name, xmdl):
        """Constructor takes a Teem airEnum pointer (const airEnum *const)."""
        if not str(aenm).startswith("<cdata 'airEnum *' "):
            raise TypeError(f'passed argument {aenm} does not seem to be a Teem airEnum pointer')
        self.aenm = aenm
        # xmdl: what extension module is this enum coming from; it it is not _teem itself, then
        # we want to know the module so as to avoid errors (when calling .ffi.string) like:
        # "TypeError: initializer for ctype 'airEnum *' appears indeed to be 'airEnum *', but
        # the types are different (check that you are not e.g. mixing up different ffi instances)"
        # Unfortunately module xmdl also has to have (in its .lib) other Teem functions; this
        # is an ugly hack until GLK learns more about CFFI wrappers around multiple libraries.
        self.xmdl = xmdl
        self.name = self.xmdl.ffi.string(self.aenm.name).decode('ascii')
        self._name = _name  # the variable name for the airEnum in libteem
        # following definition of airEnum struct in air.h
        self.vals = list(range(1, self.aenm.M + 1))
        if self.aenm.val:
            self.vals = [self.aenm.val[i] for i in self.vals]

    def __call__(self):
        """Returns (a pointer to) the underlying Teem airEnum."""
        return self.aenm

    def __iter__(self):
        """Provides a way to iterate through the valid values of the enum"""
        return iter(self.vals)

    def valid(self, ios) -> bool:  # ios = int or string
        """Answers whether given int is a valid value of enum, or whether given string
        is a valid string in enum, depending on incoming type.
        (wraps airEnumValCheck() and airEnumVal())"""
        if isinstance(ios, int):
            return not self.xmdl.lib.airEnumValCheck(self.aenm, ios)
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
        return self.xmdl.ffi.string(self.xmdl.lib.airEnumStr(self.aenm, val)).decode('ascii')

    def desc(self, val: int) -> str:
        """Converts from integer value val to description string
        (wraps airEnumDesc())"""
        assert isinstance(val, int), f'Need an int argument (not {type(val)})'
        return self.xmdl.ffi.string(self.xmdl.lib.airEnumDesc(self.aenm, val)).decode('ascii')

    def val(self, sss: str, picky=False) -> int:
        """Converts from string sss to integer enum value
        (wraps airEnumVal())"""
        assert isinstance(sss, str), f'Need an string argument (not {type(sss)})'
        ret = self.xmdl.lib.airEnumVal(self.aenm, sss.encode('ascii'))
        if picky and ret == self.unknown():
            raise ValueError(f'"{sss}" not parsable as {self._name} ("{self.name}") enum value')
        # else
        return ret

    def unknown(self) -> int:
        """Returns value representing unknown
        (wraps airEnumUnknown())"""
        return self.xmdl.lib.airEnumUnknown(self.aenm)


# The following dictionary is for all of Teem, including functions from the
# "experimental" libraries; it is no problem if the libteem in use does not
# actually contain the experimental libs.
# (the following generated by teem/src/_util/buildBiffDict.py)


def _equals1(val):
    return val == 1


_BIFFDICT = {
    'nrrdArrayCompare': (_equals1, 0, b'nrrd', 'nrrd/accessors.c:518'),
    'nrrdApply1DLut': (_equals1, 0, b'nrrd', 'nrrd/apply1D.c:434'),
    'nrrdApplyMulti1DLut': (_equals1, 0, b'nrrd', 'nrrd/apply1D.c:465'),
    'nrrdApply1DRegMap': (_equals1, 0, b'nrrd', 'nrrd/apply1D.c:514'),
    'nrrdApplyMulti1DRegMap': (_equals1, 0, b'nrrd', 'nrrd/apply1D.c:545'),
    'nrrd1DIrregMapCheck': (_equals1, 0, b'nrrd', 'nrrd/apply1D.c:587'),
    'nrrd1DIrregAclCheck': (_equals1, 0, b'nrrd', 'nrrd/apply1D.c:684'),
    'nrrd1DIrregAclGenerate': (_equals1, 0, b'nrrd', 'nrrd/apply1D.c:816'),
    'nrrdApply1DIrregMap': (_equals1, 0, b'nrrd', 'nrrd/apply1D.c:881'),
    'nrrdApply1DSubstitution': (_equals1, 0, b'nrrd', 'nrrd/apply1D.c:1055'),
    'nrrdApply2DLut': (_equals1, 0, b'nrrd', 'nrrd/apply2D.c:298'),
    'nrrdArithGamma': (_equals1, 0, b'nrrd', 'nrrd/arith.c:51'),
    'nrrdArithSRGBGamma': (_equals1, 0, b'nrrd', 'nrrd/arith.c:139'),
    'nrrdArithUnaryOp': (_equals1, 0, b'nrrd', 'nrrd/arith.c:345'),
    'nrrdArithBinaryOp': (_equals1, 0, b'nrrd', 'nrrd/arith.c:554'),
    'nrrdArithIterBinaryOpSelect': (_equals1, 0, b'nrrd', 'nrrd/arith.c:640'),
    'nrrdArithIterBinaryOp': (_equals1, 0, b'nrrd', 'nrrd/arith.c:727'),
    'nrrdArithTernaryOp': (_equals1, 0, b'nrrd', 'nrrd/arith.c:877'),
    'nrrdArithIterTernaryOpSelect': (_equals1, 0, b'nrrd', 'nrrd/arith.c:955'),
    'nrrdArithIterTernaryOp': (_equals1, 0, b'nrrd', 'nrrd/arith.c:1043'),
    'nrrdArithAffine': (_equals1, 0, b'nrrd', 'nrrd/arith.c:1066'),
    'nrrdArithIterAffine': (_equals1, 0, b'nrrd', 'nrrd/arith.c:1109'),
    'nrrdAxisInfoCompare': (_equals1, 0, b'nrrd', 'nrrd/axis.c:930'),
    'nrrdOrientationReduce': (_equals1, 0, b'nrrd', 'nrrd/axis.c:1222'),
    'nrrdMetaDataNormalize': (_equals1, 0, b'nrrd', 'nrrd/axis.c:1267'),
    'nrrdCCFind': (_equals1, 0, b'nrrd', 'nrrd/cc.c:286'),
    'nrrdCCAdjacency': (_equals1, 0, b'nrrd', 'nrrd/cc.c:543'),
    'nrrdCCMerge': (_equals1, 0, b'nrrd', 'nrrd/cc.c:643'),
    'nrrdCCRevalue': (_equals1, 0, b'nrrd', 'nrrd/cc.c:793'),
    'nrrdCCSettle': (_equals1, 0, b'nrrd', 'nrrd/cc.c:820'),
    'nrrdCCValid': ((lambda rv: 0 == rv), 0, b'nrrd', 'nrrd/ccmethods.c:27'),
    'nrrdCCSize': (_equals1, 0, b'nrrd', 'nrrd/ccmethods.c:58'),
    'nrrdDeringVerboseSet': (_equals1, 0, b'nrrd', 'nrrd/deringNrrd.c:102'),
    'nrrdDeringLinearInterpSet': (_equals1, 0, b'nrrd', 'nrrd/deringNrrd.c:115'),
    'nrrdDeringVerticalSeamSet': (_equals1, 0, b'nrrd', 'nrrd/deringNrrd.c:128'),
    'nrrdDeringInputSet': (_equals1, 0, b'nrrd', 'nrrd/deringNrrd.c:141'),
    'nrrdDeringCenterSet': (_equals1, 0, b'nrrd', 'nrrd/deringNrrd.c:176'),
    'nrrdDeringClampPercSet': (_equals1, 0, b'nrrd', 'nrrd/deringNrrd.c:195'),
    'nrrdDeringClampHistoBinsSet': (_equals1, 0, b'nrrd', 'nrrd/deringNrrd.c:216'),
    'nrrdDeringRadiusScaleSet': (_equals1, 0, b'nrrd', 'nrrd/deringNrrd.c:235'),
    'nrrdDeringThetaNumSet': (_equals1, 0, b'nrrd', 'nrrd/deringNrrd.c:253'),
    'nrrdDeringRadialKernelSet': (_equals1, 0, b'nrrd', 'nrrd/deringNrrd.c:271'),
    'nrrdDeringThetaKernelSet': (_equals1, 0, b'nrrd', 'nrrd/deringNrrd.c:291'),
    'nrrdDeringExecute': (_equals1, 0, b'nrrd', 'nrrd/deringNrrd.c:752'),
    'nrrdCheapMedian': (_equals1, 0, b'nrrd', 'nrrd/filt.c:408'),
    'nrrdDistanceL2': (_equals1, 0, b'nrrd', 'nrrd/filt.c:815'),
    'nrrdDistanceL2Biased': (_equals1, 0, b'nrrd', 'nrrd/filt.c:827'),
    'nrrdDistanceL2Signed': (_equals1, 0, b'nrrd', 'nrrd/filt.c:839'),
    'nrrdHisto': (_equals1, 0, b'nrrd', 'nrrd/histogram.c:41'),
    'nrrdHistoCheck': (_equals1, 0, b'nrrd', 'nrrd/histogram.c:161'),
    'nrrdHistoDraw': (_equals1, 0, b'nrrd', 'nrrd/histogram.c:190'),
    'nrrdHistoAxis': (_equals1, 0, b'nrrd', 'nrrd/histogram.c:326'),
    'nrrdHistoJoint': (_equals1, 0, b'nrrd', 'nrrd/histogram.c:440'),
    'nrrdHistoThresholdOtsu': (_equals1, 0, b'nrrd', 'nrrd/histogram.c:651'),
    'nrrdKernelParse': (_equals1, 0, b'nrrd', 'nrrd/kernel.c:2972'),
    'nrrdKernelSpecParse': (_equals1, 0, b'nrrd', 'nrrd/kernel.c:3152'),
    'nrrdKernelSpecSprint': (_equals1, 0, b'nrrd', 'nrrd/kernel.c:3174'),
    'nrrdKernelSprint': (_equals1, 0, b'nrrd', 'nrrd/kernel.c:3229'),
    'nrrdKernelCompare': (_equals1, 0, b'nrrd', 'nrrd/kernel.c:3247'),
    'nrrdKernelSpecCompare': (_equals1, 0, b'nrrd', 'nrrd/kernel.c:3296'),
    'nrrdKernelCheck': (_equals1, 0, b'nrrd', 'nrrd/kernel.c:3365'),
    'nrrdConvert': (_equals1, 0, b'nrrd', 'nrrd/map.c:235'),
    'nrrdClampConvert': (_equals1, 0, b'nrrd', 'nrrd/map.c:255'),
    'nrrdCastClampRound': (_equals1, 0, b'nrrd', 'nrrd/map.c:281'),
    'nrrdQuantize': (_equals1, 0, b'nrrd', 'nrrd/map.c:303'),
    'nrrdUnquantize': (_equals1, 0, b'nrrd', 'nrrd/map.c:475'),
    'nrrdHistoEq': (_equals1, 0, b'nrrd', 'nrrd/map.c:612'),
    'nrrdProject': (_equals1, 0, b'nrrd', 'nrrd/measure.c:1131'),
    'nrrdBoundarySpecCheck': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:94'),
    'nrrdBoundarySpecParse': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:118'),
    'nrrdBoundarySpecSprint': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:177'),
    'nrrdBoundarySpecCompare': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:199'),
    'nrrdBasicInfoCopy': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:539'),
    'nrrdWrap_nva': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:815'),
    'nrrdWrap_va': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:846'),
    'nrrdCopy': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:937'),
    'nrrdAlloc_nva': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:967'),
    'nrrdAlloc_va': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:1016'),
    'nrrdMaybeAlloc_nva': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:1137'),
    'nrrdMaybeAlloc_va': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:1154'),
    'nrrdCompare': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:1195'),
    'nrrdPPM': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:1381'),
    'nrrdPGM': (_equals1, 0, b'nrrd', 'nrrd/methodsNrrd.c:1401'),
    'nrrdSpaceVectorParse': (_equals1, 4, b'nrrd', 'nrrd/parseNrrd.c:522'),
    '_nrrdDataFNCheck': (_equals1, 3, b'nrrd', 'nrrd/parseNrrd.c:1199'),
    'nrrdRangePercentileSet': (_equals1, 0, b'nrrd', 'nrrd/range.c:110'),
    'nrrdRangePercentileFromStringSet': (_equals1, 0, b'nrrd', 'nrrd/range.c:212'),
    'nrrdOneLine': (_equals1, 0, b'nrrd', 'nrrd/read.c:73'),
    'nrrdLineSkip': (_equals1, 0, b'nrrd', 'nrrd/read.c:237'),
    'nrrdByteSkip': (_equals1, 0, b'nrrd', 'nrrd/read.c:333'),
    'nrrdRead': (_equals1, 0, b'nrrd', 'nrrd/read.c:497'),
    'nrrdStringRead': (_equals1, 0, b'nrrd', 'nrrd/read.c:517'),
    'nrrdLoad': ((lambda rv: 1 == rv or 2 == rv), 0, b'nrrd', 'nrrd/read.c:613'),
    'nrrdLoadMulti': (_equals1, 0, b'nrrd', 'nrrd/read.c:667'),
    'nrrdInvertPerm': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:35'),
    'nrrdAxesInsert': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:87'),
    'nrrdAxesPermute': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:153'),
    'nrrdShuffle': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:307'),
    'nrrdAxesSwap': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:452'),
    'nrrdFlip': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:488'),
    'nrrdJoin': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:569'),
    'nrrdAxesSplit': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:816'),
    'nrrdAxesDelete': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:878'),
    'nrrdAxesMerge': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:930'),
    'nrrdReshape_nva': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:980'),
    'nrrdReshape_va': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:1048'),
    'nrrdBlock': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:1085'),
    'nrrdUnblock': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:1156'),
    'nrrdTile2D': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:1255'),
    'nrrdUntile2D': (_equals1, 0, b'nrrd', 'nrrd/reorder.c:1369'),
    'nrrdResampleDefaultCenterSet': (_equals1, 0, b'nrrd', 'nrrd/resampleContext.c:171'),
    'nrrdResampleNonExistentSet': (_equals1, 0, b'nrrd', 'nrrd/resampleContext.c:192'),
    'nrrdResampleRangeSet': (_equals1, 0, b'nrrd', 'nrrd/resampleContext.c:325'),
    'nrrdResampleOverrideCenterSet': (_equals1, 0, b'nrrd', 'nrrd/resampleContext.c:344'),
    'nrrdResampleBoundarySet': (_equals1, 0, b'nrrd', 'nrrd/resampleContext.c:401'),
    'nrrdResamplePadValueSet': (_equals1, 0, b'nrrd', 'nrrd/resampleContext.c:422'),
    'nrrdResampleBoundarySpecSet': (_equals1, 0, b'nrrd', 'nrrd/resampleContext.c:439'),
    'nrrdResampleRenormalizeSet': (_equals1, 0, b'nrrd', 'nrrd/resampleContext.c:460'),
    'nrrdResampleTypeOutSet': (_equals1, 0, b'nrrd', 'nrrd/resampleContext.c:477'),
    'nrrdResampleRoundSet': (_equals1, 0, b'nrrd', 'nrrd/resampleContext.c:502'),
    'nrrdResampleClampSet': (_equals1, 0, b'nrrd', 'nrrd/resampleContext.c:519'),
    'nrrdResampleExecute': (_equals1, 0, b'nrrd', 'nrrd/resampleContext.c:1457'),
    'nrrdFFTWWisdomRead': (_equals1, 0, b'nrrd', 'nrrd/fftNrrd.c:35'),
    'nrrdFFT': (_equals1, 0, b'nrrd', 'nrrd/fftNrrd.c:91'),
    'nrrdFFTWWisdomWrite': (_equals1, 0, b'nrrd', 'nrrd/fftNrrd.c:288'),
    'nrrdSimpleResample': (_equals1, 0, b'nrrd', 'nrrd/resampleNrrd.c:52'),
    'nrrdSpatialResample': (_equals1, 0, b'nrrd', 'nrrd/resampleNrrd.c:522'),
    'nrrdSpaceSet': (_equals1, 0, b'nrrd', 'nrrd/simple.c:84'),
    'nrrdSpaceDimensionSet': (_equals1, 0, b'nrrd', 'nrrd/simple.c:121'),
    'nrrdSpaceOriginSet': (_equals1, 0, b'nrrd', 'nrrd/simple.c:173'),
    'nrrdContentSet_va': (_equals1, 0, b'nrrd', 'nrrd/simple.c:474'),
    '_nrrdCheck': (_equals1, 3, b'nrrd', 'nrrd/simple.c:1078'),
    'nrrdCheck': (_equals1, 0, b'nrrd', 'nrrd/simple.c:1115'),
    'nrrdSameSize': ((lambda rv: 0 == rv), 3, b'nrrd', 'nrrd/simple.c:1136'),
    'nrrdSanity': ((lambda rv: 0 == rv), 0, b'nrrd', 'nrrd/simple.c:1368'),
    'nrrdSlice': (_equals1, 0, b'nrrd', 'nrrd/subset.c:40'),
    'nrrdCrop': (_equals1, 0, b'nrrd', 'nrrd/subset.c:185'),
    'nrrdSliceSelect': (_equals1, 0, b'nrrd', 'nrrd/subset.c:367'),
    'nrrdSample_nva': (_equals1, 0, b'nrrd', 'nrrd/subset.c:579'),
    'nrrdSample_va': (_equals1, 0, b'nrrd', 'nrrd/subset.c:618'),
    'nrrdSimpleCrop': (_equals1, 0, b'nrrd', 'nrrd/subset.c:647'),
    'nrrdCropAuto': (_equals1, 0, b'nrrd', 'nrrd/subset.c:668'),
    'nrrdSplice': (_equals1, 0, b'nrrd', 'nrrd/superset.c:33'),
    'nrrdInset': (_equals1, 0, b'nrrd', 'nrrd/superset.c:158'),
    'nrrdPad_va': (_equals1, 0, b'nrrd', 'nrrd/superset.c:282'),
    'nrrdPad_nva': (_equals1, 0, b'nrrd', 'nrrd/superset.c:488'),
    'nrrdSimplePad_va': (_equals1, 0, b'nrrd', 'nrrd/superset.c:516'),
    'nrrdSimplePad_nva': (_equals1, 0, b'nrrd', 'nrrd/superset.c:554'),
    'nrrdIoStateSet': (_equals1, 0, b'nrrd', 'nrrd/write.c:32'),
    'nrrdIoStateEncodingSet': (_equals1, 0, b'nrrd', 'nrrd/write.c:105'),
    'nrrdIoStateFormatSet': (_equals1, 0, b'nrrd', 'nrrd/write.c:125'),
    'nrrdWrite': (_equals1, 0, b'nrrd', 'nrrd/write.c:945'),
    'nrrdStringWrite': (_equals1, 0, b'nrrd', 'nrrd/write.c:961'),
    'nrrdSave': (_equals1, 0, b'nrrd', 'nrrd/write.c:982'),
    'nrrdSaveMulti': (_equals1, 0, b'nrrd', 'nrrd/write.c:1035'),
    'ell_Nm_check': (_equals1, 0, b'ell', 'ell/genmat.c:26'),
    'ell_Nm_tran': (_equals1, 0, b'ell', 'ell/genmat.c:60'),
    'ell_Nm_mul': (_equals1, 0, b'ell', 'ell/genmat.c:105'),
    'ell_Nm_inv': (_equals1, 0, b'ell', 'ell/genmat.c:339'),
    'ell_Nm_pseudo_inv': (_equals1, 0, b'ell', 'ell/genmat.c:380'),
    'ell_Nm_wght_pseudo_inv': (_equals1, 0, b'ell', 'ell/genmat.c:414'),
    'ell_q_avg4_d': (_equals1, 0, b'ell', 'ell/quat.c:472'),
    'ell_q_avgN_d': (_equals1, 0, b'ell', 'ell/quat.c:540'),
    'mossImageCheck': (_equals1, 0, b'moss', 'moss/methodsMoss.c:74'),
    'mossImageAlloc': (_equals1, 0, b'moss', 'moss/methodsMoss.c:95'),
    'mossSamplerImageSet': (_equals1, 0, b'moss', 'moss/sampler.c:27'),
    'mossSamplerKernelSet': (_equals1, 0, b'moss', 'moss/sampler.c:79'),
    'mossSamplerUpdate': (_equals1, 0, b'moss', 'moss/sampler.c:101'),
    'mossSamplerSample': (_equals1, 0, b'moss', 'moss/sampler.c:196'),
    'mossLinearTransform': (_equals1, 0, b'moss', 'moss/xform.c:141'),
    'mossFourPointTransform': (_equals1, 0, b'moss', 'moss/xform.c:220'),
    'alanUpdate': (_equals1, 0, b'alan', 'alan/coreAlan.c:61'),
    'alanInit': (_equals1, 0, b'alan', 'alan/coreAlan.c:104'),
    'alanRun': (_equals1, 0, b'alan', 'alan/coreAlan.c:457'),
    'alanDimensionSet': (_equals1, 0, b'alan', 'alan/methodsAlan.c:105'),
    'alan2DSizeSet': (_equals1, 0, b'alan', 'alan/methodsAlan.c:120'),
    'alan3DSizeSet': (_equals1, 0, b'alan', 'alan/methodsAlan.c:140'),
    'alanTensorSet': (_equals1, 0, b'alan', 'alan/methodsAlan.c:161'),
    'alanParmSet': (_equals1, 0, b'alan', 'alan/methodsAlan.c:208'),
    'gageContextCopy': ((lambda rv: _teem.ffi.NULL == rv), 0, b'gage', 'gage/ctx.c:89'),
    'gageKernelSet': (_equals1, 0, b'gage', 'gage/ctx.c:200'),
    'gagePerVolumeAttach': (_equals1, 0, b'gage', 'gage/ctx.c:399'),
    'gagePerVolumeDetach': (_equals1, 0, b'gage', 'gage/ctx.c:458'),
    'gageDeconvolve': (_equals1, 0, b'gage', 'gage/deconvolve.c:27'),
    'gageDeconvolveSeparable': (_equals1, 0, b'gage', 'gage/deconvolve.c:209'),
    'gageKindCheck': (_equals1, 0, b'gage', 'gage/kind.c:34'),
    'gageKindVolumeCheck': (_equals1, 0, b'gage', 'gage/kind.c:219'),
    'gageVolumeCheck': (_equals1, 0, b'gage', 'gage/pvl.c:37'),
    'gagePerVolumeNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'gage', 'gage/pvl.c:58'),
    'gageQueryReset': (_equals1, 0, b'gage', 'gage/pvl.c:262'),
    'gageQuerySet': (_equals1, 0, b'gage', 'gage/pvl.c:288'),
    'gageQueryAdd': (_equals1, 0, b'gage', 'gage/pvl.c:344'),
    'gageQueryItemOn': (_equals1, 0, b'gage', 'gage/pvl.c:362'),
    'gageShapeSet': (_equals1, 0, b'gage', 'gage/shape.c:406'),
    'gageShapeEqual': ((lambda rv: 0 == rv), 0, b'gage', 'gage/shape.c:469'),
    'gageStructureTensor': (_equals1, 0, b'gage', 'gage/st.c:84'),
    'gageStackPerVolumeNew': (_equals1, 0, b'gage', 'gage/stack.c:99'),
    'gageStackPerVolumeAttach': (_equals1, 0, b'gage', 'gage/stack.c:128'),
    'gageStackBlurParmCompare': (_equals1, 0, b'gage', 'gage/stackBlur.c:126'),
    'gageStackBlurParmCopy': (_equals1, 0, b'gage', 'gage/stackBlur.c:231'),
    'gageStackBlurParmSigmaSet': (_equals1, 0, b'gage', 'gage/stackBlur.c:268'),
    'gageStackBlurParmScaleSet': (_equals1, 0, b'gage', 'gage/stackBlur.c:362'),
    'gageStackBlurParmKernelSet': (_equals1, 0, b'gage', 'gage/stackBlur.c:386'),
    'gageStackBlurParmRenormalizeSet': (_equals1, 0, b'gage', 'gage/stackBlur.c:399'),
    'gageStackBlurParmBoundarySet': (_equals1, 0, b'gage', 'gage/stackBlur.c:411'),
    'gageStackBlurParmBoundarySpecSet': (_equals1, 0, b'gage', 'gage/stackBlur.c:430'),
    'gageStackBlurParmOneDimSet': (_equals1, 0, b'gage', 'gage/stackBlur.c:447'),
    'gageStackBlurParmNeedSpatialBlurSet': (_equals1, 0, b'gage', 'gage/stackBlur.c:459'),
    'gageStackBlurParmVerboseSet': (_equals1, 0, b'gage', 'gage/stackBlur.c:471'),
    'gageStackBlurParmDgGoodSigmaMaxSet': (_equals1, 0, b'gage', 'gage/stackBlur.c:483'),
    'gageStackBlurParmCheck': (_equals1, 0, b'gage', 'gage/stackBlur.c:499'),
    'gageStackBlurParmParse': (_equals1, 0, b'gage', 'gage/stackBlur.c:546'),
    'gageStackBlurParmSprint': (_equals1, 0, b'gage', 'gage/stackBlur.c:805'),
    'gageStackBlur': (_equals1, 0, b'gage', 'gage/stackBlur.c:1387'),
    'gageStackBlurCheck': (_equals1, 0, b'gage', 'gage/stackBlur.c:1490'),
    'gageStackBlurGet': (_equals1, 0, b'gage', 'gage/stackBlur.c:1598'),
    'gageStackBlurManage': (_equals1, 0, b'gage', 'gage/stackBlur.c:1699'),
    'gageUpdate': (_equals1, 0, b'gage', 'gage/update.c:314'),
    'gageOptimSigSet': (_equals1, 0, b'gage', 'gage/optimsig.c:218'),
    'gageOptimSigContextNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'gage', 'gage/optimsig.c:312'),
    'gageOptimSigCalculate': (_equals1, 0, b'gage', 'gage/optimsig.c:1093'),
    'gageOptimSigErrorPlot': (_equals1, 0, b'gage', 'gage/optimsig.c:1165'),
    'gageOptimSigErrorPlotSliding': (_equals1, 0, b'gage', 'gage/optimsig.c:1256'),
    'dyeConvert': (_equals1, 0, b'dye', 'dye/convertDye.c:352'),
    'dyeColorParse': (_equals1, 0, b'dye', 'dye/methodsDye.c:186'),
    'baneClipNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'bane', 'bane/clip.c:103'),
    'baneClipAnswer': (_equals1, 0, b'bane', 'bane/clip.c:153'),
    'baneClipCopy': ((lambda rv: _teem.ffi.NULL == rv), 0, b'bane', 'bane/clip.c:168'),
    'baneFindInclusion': (_equals1, 0, b'bane', 'bane/hvol.c:88'),
    'baneMakeHVol': (_equals1, 0, b'bane', 'bane/hvol.c:250'),
    'baneGKMSHVol': ((lambda rv: _teem.ffi.NULL == rv), 0, b'bane', 'bane/hvol.c:449'),
    'baneIncNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'bane', 'bane/inc.c:252'),
    'baneIncAnswer': (_equals1, 0, b'bane', 'bane/inc.c:361'),
    'baneIncCopy': ((lambda rv: _teem.ffi.NULL == rv), 0, b'bane', 'bane/inc.c:376'),
    'baneMeasrNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'bane', 'bane/measr.c:34'),
    'baneMeasrCopy': ((lambda rv: _teem.ffi.NULL == rv), 0, b'bane', 'bane/measr.c:150'),
    'baneRangeNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'bane', 'bane/rangeBane.c:90'),
    'baneRangeCopy': ((lambda rv: _teem.ffi.NULL == rv), 0, b'bane', 'bane/rangeBane.c:131'),
    'baneRangeAnswer': (_equals1, 0, b'bane', 'bane/rangeBane.c:145'),
    'baneRawScatterplots': (_equals1, 0, b'bane', 'bane/scat.c:27'),
    'baneOpacInfo': (_equals1, 0, b'bane', 'bane/trnsf.c:30'),
    'bane1DOpacInfoFrom2D': (_equals1, 0, b'bane', 'bane/trnsf.c:146'),
    'baneSigmaCalc': (_equals1, 0, b'bane', 'bane/trnsf.c:225'),
    'banePosCalc': (_equals1, 0, b'bane', 'bane/trnsf.c:256'),
    'baneOpacCalc': (_equals1, 0, b'bane', 'bane/trnsf.c:407'),
    'baneInputCheck': (_equals1, 0, b'bane', 'bane/valid.c:27'),
    'baneHVolCheck': (_equals1, 0, b'bane', 'bane/valid.c:65'),
    'baneInfoCheck': (_equals1, 0, b'bane', 'bane/valid.c:107'),
    'banePosCheck': (_equals1, 0, b'bane', 'bane/valid.c:145'),
    'baneBcptsCheck': (_equals1, 0, b'bane', 'bane/valid.c:180'),
    'limnCameraUpdate': (_equals1, 0, b'limn', 'limn/cam.c:34'),
    'limnCameraAspectSet': (_equals1, 0, b'limn', 'limn/cam.c:131'),
    'limnCameraPathMake': (_equals1, 0, b'limn', 'limn/cam.c:190'),
    'limnEnvMapFill': (_equals1, 0, b'limn', 'limn/envmap.c:26'),
    'limnEnvMapCheck': (_equals1, 0, b'limn', 'limn/envmap.c:120'),
    'limnObjectWriteOFF': (_equals1, 0, b'limn', 'limn/io.c:80'),
    'limnPolyDataWriteIV': (_equals1, 0, b'limn', 'limn/io.c:139'),
    'limnObjectReadOFF': (_equals1, 0, b'limn', 'limn/io.c:265'),
    'limnPolyDataWriteLMPD': (_equals1, 0, b'limn', 'limn/io.c:456'),
    'limnPolyDataReadLMPD': (_equals1, 0, b'limn', 'limn/io.c:583'),
    'limnPolyDataWriteVTK': (_equals1, 0, b'limn', 'limn/io.c:966'),
    'limnPolyDataReadOFF': (_equals1, 0, b'limn', 'limn/io.c:1056'),
    'limnPolyDataSave': (_equals1, 0, b'limn', 'limn/io.c:1161'),
    'limnLightUpdate': (_equals1, 0, b'limn', 'limn/light.c:68'),
    'limnPolyDataAlloc': (_equals1, 0, b'limn', 'limn/polydata.c:150'),
    'limnPolyDataCopy': (_equals1, 0, b'limn', 'limn/polydata.c:229'),
    'limnPolyDataCopyN': (_equals1, 0, b'limn', 'limn/polydata.c:261'),
    'limnPolyDataPrimitiveVertexNumber': (_equals1, 0, b'limn', 'limn/polydata.c:552'),
    'limnPolyDataPrimitiveArea': (_equals1, 0, b'limn', 'limn/polydata.c:574'),
    'limnPolyDataRasterize': (_equals1, 0, b'limn', 'limn/polydata.c:632'),
    'limnPolyDataSpiralTubeWrap': (_equals1, 0, b'limn', 'limn/polyfilter.c:27'),
    'limnPolyDataSmoothHC': ((lambda rv: -1 == rv), 0, b'limn', 'limn/polyfilter.c:337'),
    'limnPolyDataVertexWindingFix': (_equals1, 0, b'limn', 'limn/polymod.c:1232'),
    'limnPolyDataCCFind': (_equals1, 0, b'limn', 'limn/polymod.c:1251'),
    'limnPolyDataPrimitiveSort': (_equals1, 0, b'limn', 'limn/polymod.c:1382'),
    'limnPolyDataVertexWindingFlip': (_equals1, 0, b'limn', 'limn/polymod.c:1465'),
    'limnPolyDataPrimitiveSelect': (_equals1, 0, b'limn', 'limn/polymod.c:1494'),
    'limnPolyDataClipMulti': (_equals1, 0, b'limn', 'limn/polymod.c:1709'),
    'limnPolyDataCompress': ((lambda rv: _teem.ffi.NULL == rv), 0, b'limn', 'limn/polymod.c:1996'),
    'limnPolyDataJoin': ((lambda rv: _teem.ffi.NULL == rv), 0, b'limn', 'limn/polymod.c:2086'),
    'limnPolyDataEdgeHalve': (_equals1, 0, b'limn', 'limn/polymod.c:2154'),
    'limnPolyDataNeighborList': ((lambda rv: -1 == rv), 0, b'limn', 'limn/polymod.c:2331'),
    'limnPolyDataNeighborArray': ((lambda rv: -1 == rv), 0, b'limn', 'limn/polymod.c:2427'),
    'limnPolyDataNeighborArrayComp': ((lambda rv: -1 == rv), 0, b'limn', 'limn/polymod.c:2467'),
    'limnPolyDataCube': (_equals1, 0, b'limn', 'limn/polyshapes.c:28'),
    'limnPolyDataCubeTriangles': (_equals1, 0, b'limn', 'limn/polyshapes.c:138'),
    'limnPolyDataOctahedron': (_equals1, 0, b'limn', 'limn/polyshapes.c:348'),
    'limnPolyDataCylinder': (_equals1, 0, b'limn', 'limn/polyshapes.c:462'),
    'limnPolyDataCone': (_equals1, 0, b'limn', 'limn/polyshapes.c:636'),
    'limnPolyDataSuperquadric': (_equals1, 0, b'limn', 'limn/polyshapes.c:735'),
    'limnPolyDataSpiralBetterquadric': (_equals1, 0, b'limn', 'limn/polyshapes.c:860'),
    'limnPolyDataSpiralSuperquadric': (_equals1, 0, b'limn', 'limn/polyshapes.c:1017'),
    'limnPolyDataPolarSphere': (_equals1, 0, b'limn', 'limn/polyshapes.c:1035'),
    'limnPolyDataSpiralSphere': (_equals1, 0, b'limn', 'limn/polyshapes.c:1047'),
    'limnPolyDataIcoSphere': (_equals1, 0, b'limn', 'limn/polyshapes.c:1098'),
    'limnPolyDataPlane': (_equals1, 0, b'limn', 'limn/polyshapes.c:1342'),
    'limnPolyDataSquare': (_equals1, 0, b'limn', 'limn/polyshapes.c:1397'),
    'limnPolyDataSuperquadric2D': (_equals1, 0, b'limn', 'limn/polyshapes.c:1440'),
    'limnQNDemo': (_equals1, 0, b'limn', 'limn/qn.c:893'),
    'limnObjectRender': (_equals1, 0, b'limn', 'limn/renderLimn.c:26'),
    'limnObjectPSDraw': (_equals1, 0, b'limn', 'limn/renderLimn.c:185'),
    'limnObjectPSDrawConcave': (_equals1, 0, b'limn', 'limn/renderLimn.c:315'),
    'limnSplineNrrdEvaluate': (_equals1, 0, b'limn', 'limn/splineEval.c:324'),
    'limnSplineSample': (_equals1, 0, b'limn', 'limn/splineEval.c:362'),
    'limnSplineTypeSpecNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'limn', 'limn/splineMethods.c:26'),
    'limnSplineNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'limn', 'limn/splineMethods.c:125'),
    'limnSplineNrrdCleverFix': (_equals1, 0, b'limn', 'limn/splineMethods.c:250'),
    'limnSplineCleverNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'limn', 'limn/splineMethods.c:395'),
    'limnSplineUpdate': (_equals1, 0, b'limn', 'limn/splineMethods.c:423'),
    'limnSplineTypeSpecParse': ((lambda rv: _teem.ffi.NULL == rv), 0, b'limn', 'limn/splineMisc.c:223'),
    'limnSplineParse': ((lambda rv: _teem.ffi.NULL == rv), 0, b'limn', 'limn/splineMisc.c:279'),
    'limnCBFCheck': (_equals1, 0, b'limn', 'limn/splineFit.c:588'),
    'limnCBFitSingle': (_equals1, 0, b'limn', 'limn/splineFit.c:859'),
    'limnCBFMulti': (_equals1, 0, b'limn', 'limn/splineFit.c:950'),
    'limnCBFCorners': (_equals1, 0, b'limn', 'limn/splineFit.c:1052'),
    'limnCBFit': (_equals1, 0, b'limn', 'limn/splineFit.c:1122'),
    'limnObjectWorldHomog': (_equals1, 0, b'limn', 'limn/transform.c:26'),
    'limnObjectFaceNormals': (_equals1, 0, b'limn', 'limn/transform.c:48'),
    'limnObjectSpaceTransform': (_equals1, 0, b'limn', 'limn/transform.c:211'),
    'limnObjectFaceReverse': (_equals1, 0, b'limn', 'limn/transform.c:336'),
    'echoThreadStateInit': (_equals1, 0, b'echo', 'echo/renderEcho.c:27'),
    'echoRTRenderCheck': (_equals1, 0, b'echo', 'echo/renderEcho.c:137'),
    'echoRTRender': (_equals1, 0, b'echo', 'echo/renderEcho.c:412'),
    'hooverContextCheck': (_equals1, 0, b'hoover', 'hoover/methodsHoover.c:54'),
    'hooverRender': ((lambda rv: _teem.lib.hooverErrInit == rv), 0, b'hoover', 'hoover/rays.c:360'),
    'seekExtract': (_equals1, 0, b'seek', 'seek/extract.c:925'),
    'seekDataSet': (_equals1, 0, b'seek', 'seek/setSeek.c:57'),
    'seekSamplesSet': (_equals1, 0, b'seek', 'seek/setSeek.c:117'),
    'seekTypeSet': (_equals1, 0, b'seek', 'seek/setSeek.c:150'),
    'seekLowerInsideSet': (_equals1, 0, b'seek', 'seek/setSeek.c:174'),
    'seekNormalsFindSet': (_equals1, 0, b'seek', 'seek/setSeek.c:194'),
    'seekStrengthUseSet': (_equals1, 0, b'seek', 'seek/setSeek.c:209'),
    'seekStrengthSet': (_equals1, 0, b'seek', 'seek/setSeek.c:224'),
    'seekItemScalarSet': (_equals1, 0, b'seek', 'seek/setSeek.c:286'),
    'seekItemStrengthSet': (_equals1, 0, b'seek', 'seek/setSeek.c:305'),
    'seekItemHessSet': (_equals1, 0, b'seek', 'seek/setSeek.c:324'),
    'seekItemGradientSet': (_equals1, 0, b'seek', 'seek/setSeek.c:344'),
    'seekItemNormalSet': (_equals1, 0, b'seek', 'seek/setSeek.c:365'),
    'seekItemEigensystemSet': (_equals1, 0, b'seek', 'seek/setSeek.c:386'),
    'seekIsovalueSet': (_equals1, 0, b'seek', 'seek/setSeek.c:415'),
    'seekEvalDiffThreshSet': (_equals1, 0, b'seek', 'seek/setSeek.c:441'),
    'seekVertexStrength': (_equals1, 0, b'seek', 'seek/textract.c:1885'),
    'seekUpdate': (_equals1, 0, b'seek', 'seek/updateSeek.c:676'),
    'tenAnisoPlot': (_equals1, 0, b'ten', 'ten/aniso.c:1069'),
    'tenAnisoVolume': (_equals1, 0, b'ten', 'ten/aniso.c:1129'),
    'tenAnisoHistogram': (_equals1, 0, b'ten', 'ten/aniso.c:1201'),
    'tenEvecRGBParmCheck': (_equals1, 0, b'ten', 'ten/aniso.c:1316'),
    'tenEMBimodal': (_equals1, 0, b'ten', 'ten/bimod.c:413'),
    'tenBVecNonLinearFit': (_equals1, 0, b'ten', 'ten/bvec.c:100'),
    'tenDWMRIKeyValueParse': (_equals1, 0, b'ten', 'ten/chan.c:61'),
    'tenBMatrixCalc': (_equals1, 0, b'ten', 'ten/chan.c:349'),
    'tenEMatrixCalc': (_equals1, 0, b'ten', 'ten/chan.c:390'),
    'tenEstimateLinear3D': (_equals1, 0, b'ten', 'ten/chan.c:583'),
    'tenEstimateLinear4D': (_equals1, 0, b'ten', 'ten/chan.c:630'),
    'tenSimulate': (_equals1, 0, b'ten', 'ten/chan.c:871'),
    'tenEpiRegister3D': (_equals1, 0, b'ten', 'ten/epireg.c:1050'),
    'tenEpiRegister4D': (_equals1, 0, b'ten', 'ten/epireg.c:1201'),
    'tenEstimateMethodSet': (_equals1, 0, b'ten', 'ten/estimate.c:284'),
    'tenEstimateSigmaSet': (_equals1, 0, b'ten', 'ten/estimate.c:306'),
    'tenEstimateValueMinSet': (_equals1, 0, b'ten', 'ten/estimate.c:324'),
    'tenEstimateGradientsSet': (_equals1, 0, b'ten', 'ten/estimate.c:342'),
    'tenEstimateBMatricesSet': (_equals1, 0, b'ten', 'ten/estimate.c:369'),
    'tenEstimateSkipSet': (_equals1, 0, b'ten', 'ten/estimate.c:396'),
    'tenEstimateSkipReset': (_equals1, 0, b'ten', 'ten/estimate.c:414'),
    'tenEstimateThresholdFind': (_equals1, 0, b'ten', 'ten/estimate.c:429'),
    'tenEstimateThresholdSet': (_equals1, 0, b'ten', 'ten/estimate.c:497'),
    'tenEstimateUpdate': (_equals1, 0, b'ten', 'ten/estimate.c:805'),
    'tenEstimate1TensorSimulateSingle_f': (_equals1, 0, b'ten', 'ten/estimate.c:979'),
    'tenEstimate1TensorSimulateSingle_d': (_equals1, 0, b'ten', 'ten/estimate.c:1007'),
    'tenEstimate1TensorSimulateVolume': (_equals1, 0, b'ten', 'ten/estimate.c:1038'),
    'tenEstimate1TensorSingle_f': (_equals1, 0, b'ten', 'ten/estimate.c:1743'),
    'tenEstimate1TensorSingle_d': (_equals1, 0, b'ten', 'ten/estimate.c:1771'),
    'tenEstimate1TensorVolume4D': (_equals1, 0, b'ten', 'ten/estimate.c:1808'),
    'tenFiberTraceSet': (_equals1, 0, b'ten', 'ten/fiber.c:829'),
    'tenFiberTrace': (_equals1, 0, b'ten', 'ten/fiber.c:849'),
    'tenFiberDirectionNumber': ((lambda rv: 0 == rv), 0, b'ten', 'ten/fiber.c:869'),
    'tenFiberSingleTrace': (_equals1, 0, b'ten', 'ten/fiber.c:918'),
    'tenFiberMultiNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'ten', 'ten/fiber.c:961'),
    'tenFiberMultiTrace': (_equals1, 0, b'ten', 'ten/fiber.c:1026'),
    'tenFiberMultiPolyData': (_equals1, 0, b'ten', 'ten/fiber.c:1246'),
    'tenFiberMultiProbeVals': (_equals1, 0, b'ten', 'ten/fiber.c:1257'),
    'tenFiberContextDwiNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'ten', 'ten/fiberMethods.c:211'),
    'tenFiberContextNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'ten', 'ten/fiberMethods.c:225'),
    'tenFiberTypeSet': (_equals1, 0, b'ten', 'ten/fiberMethods.c:249'),
    'tenFiberStopSet': (_equals1, 0, b'ten', 'ten/fiberMethods.c:379'),
    'tenFiberStopAnisoSet': (_equals1, 0, b'ten', 'ten/fiberMethods.c:555'),
    'tenFiberStopDoubleSet': (_equals1, 0, b'ten', 'ten/fiberMethods.c:567'),
    'tenFiberStopUIntSet': (_equals1, 0, b'ten', 'ten/fiberMethods.c:591'),
    'tenFiberAnisoSpeedSet': (_equals1, 0, b'ten', 'ten/fiberMethods.c:638'),
    'tenFiberAnisoSpeedReset': (_equals1, 0, b'ten', 'ten/fiberMethods.c:703'),
    'tenFiberKernelSet': (_equals1, 0, b'ten', 'ten/fiberMethods.c:718'),
    'tenFiberProbeItemSet': (_equals1, 0, b'ten', 'ten/fiberMethods.c:737'),
    'tenFiberIntgSet': (_equals1, 0, b'ten', 'ten/fiberMethods.c:749'),
    'tenFiberUpdate': (_equals1, 0, b'ten', 'ten/fiberMethods.c:792'),
    'tenGlyphParmCheck': (_equals1, 0, b'ten', 'ten/glyph.c:73'),
    'tenGlyphGen': (_equals1, 0, b'ten', 'ten/glyph.c:174'),
    'tenGradientCheck': (_equals1, 0, b'ten', 'ten/grads.c:68'),
    'tenGradientRandom': (_equals1, 0, b'ten', 'ten/grads.c:107'),
    'tenGradientJitter': (_equals1, 0, b'ten', 'ten/grads.c:153'),
    'tenGradientBalance': (_equals1, 0, b'ten', 'ten/grads.c:375'),
    'tenGradientDistribute': (_equals1, 0, b'ten', 'ten/grads.c:460'),
    'tenGradientGenerate': (_equals1, 0, b'ten', 'ten/grads.c:653'),
    'tenEvecRGB': (_equals1, 0, b'ten', 'ten/miscTen.c:27'),
    'tenEvqVolume': (_equals1, 0, b'ten', 'ten/miscTen.c:152'),
    'tenBMatrixCheck': (_equals1, 0, b'ten', 'ten/miscTen.c:213'),
    '_tenFindValley': (_equals1, 0, b'ten', 'ten/miscTen.c:257'),
    'tenSizeNormalize': (_equals1, 0, b'ten', 'ten/mod.c:222'),
    'tenSizeScale': (_equals1, 0, b'ten', 'ten/mod.c:238'),
    'tenAnisoScale': (_equals1, 0, b'ten', 'ten/mod.c:256'),
    'tenEigenvalueClamp': (_equals1, 0, b'ten', 'ten/mod.c:276'),
    'tenEigenvaluePower': (_equals1, 0, b'ten', 'ten/mod.c:295'),
    'tenEigenvalueAdd': (_equals1, 0, b'ten', 'ten/mod.c:313'),
    'tenEigenvalueMultiply': (_equals1, 0, b'ten', 'ten/mod.c:331'),
    'tenLog': (_equals1, 0, b'ten', 'ten/mod.c:349'),
    'tenExp': (_equals1, 0, b'ten', 'ten/mod.c:366'),
    'tenInterpParmBufferAlloc': (_equals1, 0, b'ten', 'ten/path.c:65'),
    'tenInterpParmCopy': ((lambda rv: _teem.ffi.NULL == rv), 0, b'ten', 'ten/path.c:124'),
    'tenInterpN_d': (_equals1, 0, b'ten', 'ten/path.c:306'),
    'tenInterpTwoDiscrete_d': (_equals1, 0, b'ten', 'ten/path.c:807'),
    'tenInterpMulti3D': (_equals1, 0, b'ten', 'ten/path.c:958'),
    'tenDwiGageKindSet': (_equals1, 0, b'ten', 'ten/tenDwiGage.c:1038'),
    'tenDwiGageKindCheck': (_equals1, 0, b'ten', 'ten/tenDwiGage.c:1178'),
    'tenTensorCheck': (_equals1, 4, b'ten', 'ten/tensor.c:55'),
    'tenMeasurementFrameReduce': (_equals1, 0, b'ten', 'ten/tensor.c:88'),
    'tenExpand2D': (_equals1, 0, b'ten', 'ten/tensor.c:158'),
    'tenExpand': (_equals1, 0, b'ten', 'ten/tensor.c:232'),
    'tenShrink': (_equals1, 0, b'ten', 'ten/tensor.c:288'),
    'tenMake': (_equals1, 0, b'ten', 'ten/tensor.c:530'),
    'tenSlice': (_equals1, 0, b'ten', 'ten/tensor.c:632'),
    'tenTripleCalc': (_equals1, 0, b'ten', 'ten/triple.c:416'),
    'tenTripleConvert': (_equals1, 0, b'ten', 'ten/triple.c:474'),
    'tenExperSpecGradSingleBValSet': (_equals1, 0, b'ten', 'ten/experSpec.c:64'),
    'tenExperSpecGradBValSet': (_equals1, 0, b'ten', 'ten/experSpec.c:105'),
    'tenExperSpecFromKeyValueSet': (_equals1, 0, b'ten', 'ten/experSpec.c:174'),
    'tenDWMRIKeyValueFromExperSpecSet': (_equals1, 0, b'ten', 'ten/experSpec.c:329'),
    'tenModelParse': (_equals1, 0, b'ten', 'ten/tenModel.c:64'),
    'tenModelFromAxisLearn': (_equals1, 0, b'ten', 'ten/tenModel.c:125'),
    'tenModelSimulate': (_equals1, 0, b'ten', 'ten/tenModel.c:163'),
    'tenModelSqeFit': (_equals1, 0, b'ten', 'ten/tenModel.c:411'),
    'tenModelConvert': (_equals1, 0, b'ten', 'ten/tenModel.c:686'),
    'pullEnergyPlot': (_equals1, 0, b'pull', 'pull/actionPull.c:233'),
    'pullBinProcess': (_equals1, 0, b'pull', 'pull/actionPull.c:1107'),
    'pullGammaLearn': (_equals1, 0, b'pull', 'pull/actionPull.c:1142'),
    'pullBinsPointAdd': (_equals1, 0, b'pull', 'pull/binningPull.c:184'),
    'pullBinsPointMaybeAdd': (_equals1, 0, b'pull', 'pull/binningPull.c:206'),
    'pullOutputGetFilter': (_equals1, 0, b'pull', 'pull/contextPull.c:382'),
    'pullOutputGet': (_equals1, 0, b'pull', 'pull/contextPull.c:578'),
    'pullPropGet': (_equals1, 0, b'pull', 'pull/contextPull.c:591'),
    'pullPositionHistoryNrrdGet': (_equals1, 0, b'pull', 'pull/contextPull.c:769'),
    'pullPositionHistoryPolydataGet': (_equals1, 0, b'pull', 'pull/contextPull.c:841'),
    'pullIterParmSet': (_equals1, 0, b'pull', 'pull/parmPull.c:105'),
    'pullSysParmSet': (_equals1, 0, b'pull', 'pull/parmPull.c:194'),
    'pullFlagSet': (_equals1, 0, b'pull', 'pull/parmPull.c:273'),
    'pullVerboseSet': (_equals1, 0, b'pull', 'pull/parmPull.c:348'),
    'pullThreadNumSet': (_equals1, 0, b'pull', 'pull/parmPull.c:373'),
    'pullRngSeedSet': (_equals1, 0, b'pull', 'pull/parmPull.c:385'),
    'pullProgressBinModSet': (_equals1, 0, b'pull', 'pull/parmPull.c:397'),
    'pullCallbackSet': (_equals1, 0, b'pull', 'pull/parmPull.c:409'),
    'pullInterEnergySet': (_equals1, 0, b'pull', 'pull/parmPull.c:434'),
    'pullLogAddSet': (_equals1, 0, b'pull', 'pull/parmPull.c:495'),
    'pullInitRandomSet': (_equals1, 0, b'pull', 'pull/initPull.c:110'),
    'pullInitHaltonSet': (_equals1, 0, b'pull', 'pull/initPull.c:128'),
    'pullInitPointPerVoxelSet': (_equals1, 0, b'pull', 'pull/initPull.c:147'),
    'pullInitGivenPosSet': (_equals1, 0, b'pull', 'pull/initPull.c:175'),
    'pullInitLiveThreshUseSet': (_equals1, 0, b'pull', 'pull/initPull.c:189'),
    'pullInitUnequalShapesAllowSet': (_equals1, 0, b'pull', 'pull/initPull.c:202'),
    'pullStart': (_equals1, 0, b'pull', 'pull/corePull.c:114'),
    'pullFinish': (_equals1, 0, b'pull', 'pull/corePull.c:169'),
    'pullRun': (_equals1, 0, b'pull', 'pull/corePull.c:337'),
    'pullEnergySpecParse': (_equals1, 0, b'pull', 'pull/energy.c:628'),
    'pullInfoSpecAdd': (_equals1, 0, b'pull', 'pull/infoPull.c:133'),
    'pullInfoGet': (_equals1, 0, b'pull', 'pull/infoPull.c:405'),
    'pullInfoSpecSprint': (_equals1, 0, b'pull', 'pull/infoPull.c:450'),
    'pullPointNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'pull', 'pull/pointPull.c:34'),
    'pullProbe': (_equals1, 0, b'pull', 'pull/pointPull.c:359'),
    'pullPointInitializePerVoxel': (_equals1, 0, b'pull', 'pull/pointPull.c:638'),
    'pullPointInitializeRandomOrHalton': (_equals1, 0, b'pull', 'pull/pointPull.c:823'),
    'pullPointInitializeGivenPos': (_equals1, 0, b'pull', 'pull/pointPull.c:992'),
    'pullVolumeSingleAdd': (_equals1, 0, b'pull', 'pull/volumePull.c:213'),
    'pullVolumeStackAdd': (_equals1, 0, b'pull', 'pull/volumePull.c:239'),
    'pullVolumeLookup': ((lambda rv: _teem.ffi.NULL == rv), 0, b'pull', 'pull/volumePull.c:478'),
    'pullConstraintScaleRange': (_equals1, 0, b'pull', 'pull/volumePull.c:497'),
    'pullCCFind': (_equals1, 0, b'pull', 'pull/ccPull.c:31'),
    'pullCCMeasure': (_equals1, 0, b'pull', 'pull/ccPull.c:115'),
    'pullCCSort': (_equals1, 0, b'pull', 'pull/ccPull.c:210'),
    'pullTraceSet': (_equals1, 0, b'pull', 'pull/trace.c:246'),
    'pullTraceMultiAdd': (_equals1, 0, b'pull', 'pull/trace.c:675'),
    'pullTraceMultiPlotAdd': (_equals1, 0, b'pull', 'pull/trace.c:705'),
    'pullTraceMultiWrite': (_equals1, 0, b'pull', 'pull/trace.c:1015'),
    'pullTraceMultiRead': (_equals1, 0, b'pull', 'pull/trace.c:1120'),
    'coilStart': (_equals1, 0, b'coil', 'coil/coreCoil.c:288'),
    'coilIterate': (_equals1, 0, b'coil', 'coil/coreCoil.c:363'),
    'coilFinish': (_equals1, 0, b'coil', 'coil/coreCoil.c:408'),
    'coilVolumeCheck': (_equals1, 0, b'coil', 'coil/methodsCoil.c:26'),
    'coilContextAllSet': (_equals1, 0, b'coil', 'coil/methodsCoil.c:70'),
    'coilOutputGet': (_equals1, 0, b'coil', 'coil/methodsCoil.c:201'),
    'pushOutputGet': (_equals1, 0, b'push', 'push/action.c:72'),
    'pushBinProcess': (_equals1, 0, b'push', 'push/action.c:162'),
    'pushBinPointAdd': (_equals1, 0, b'push', 'push/binning.c:181'),
    'pushRebin': (_equals1, 0, b'push', 'push/binning.c:198'),
    'pushStart': (_equals1, 0, b'push', 'push/corePush.c:184'),
    'pushIterate': (_equals1, 0, b'push', 'push/corePush.c:234'),
    'pushRun': (_equals1, 0, b'push', 'push/corePush.c:307'),
    'pushFinish': (_equals1, 0, b'push', 'push/corePush.c:397'),
    'pushEnergySpecParse': (_equals1, 0, b'push', 'push/forces.c:305'),
    'miteSample': (_math.isnan, 0, b'mite', 'mite/ray.c:152'),
    'miteRenderBegin': (_equals1, 0, b'mite', 'mite/renderMite.c:64'),
    'miteShadeSpecParse': (_equals1, 0, b'mite', 'mite/shade.c:70'),
    'miteThreadNew': ((lambda rv: _teem.ffi.NULL == rv), 0, b'mite', 'mite/thread.c:27'),
    'miteThreadBegin': (_equals1, 0, b'mite', 'mite/thread.c:93'),
    'miteVariableParse': (_equals1, 0, b'mite', 'mite/txf.c:102'),
    'miteNtxfCheck': (_equals1, 0, b'mite', 'mite/txf.c:233'),
    'meetAirEnumAllCheck': (_equals1, 0, b'meet', 'meet/enumall.c:227'),
    'meetNrrdKernelAllCheck': (_equals1, 0, b'meet', 'meet/meetNrrd.c:231'),
    'meetPullVolCopy': ((lambda rv: _teem.ffi.NULL == rv), 0, b'meet', 'meet/meetPull.c:45'),
    'meetPullVolParse': (_equals1, 0, b'meet', 'meet/meetPull.c:101'),
    'meetPullVolLeechable': (_equals1, 0, b'meet', 'meet/meetPull.c:315'),
    'meetPullVolStackBlurParmFinishMulti': (_equals1, 0, b'meet', 'meet/meetPull.c:428'),
    'meetPullVolLoadMulti': (_equals1, 0, b'meet', 'meet/meetPull.c:473'),
    'meetPullVolAddMulti': (_equals1, 0, b'meet', 'meet/meetPull.c:553'),
    'meetPullInfoParse': (_equals1, 0, b'meet', 'meet/meetPull.c:635'),
    'meetPullInfoAddMulti': (_equals1, 0, b'meet', 'meet/meetPull.c:766'),
}

# generates a biff-checking wrapper around function func


def _biffer(func, func_name: str, rvtf, mubi: int, bkey, fnln: str):
    def wrapper(*args):
        # pass all args to underlying C function; get return value
        ret_val = func(*args)
        # we have to get biff error if rvtf(ret_val) == ret_valindicates error
        # and, either: this function definitely uses biff (0 == mubi)
        #          or: (this function maybe uses biff and) "useBiff" args[mubi-1] is True
        if rvtf(ret_val) and (0 == mubi or args[mubi - 1]):
            err = _teem.lib.biffGetDone(bkey)
            estr = _teem.ffi.string(err).decode('ascii').rstrip()
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
    """Exports things from _teem.lib, adding biff wrappers to functions where possible."""
    for sym_name in dir(_teem.lib):
        if 'free' == sym_name:
            # don't export C runtime's free(), though we use it above in the biff wrapper
            continue
        sym = getattr(_teem.lib, sym_name)
        # Create a python object in this module for the library symbol sym
        exp = 'unset'
        # The exported symbol _sym is ...
        if not sym_name in _BIFFDICT:
            # ... either a function known to not use biff, or, not a function
            if str(sym).startswith("<cdata 'airEnum *' "):
                # _sym is name of an airEnum, wrap it as such
                exp = Tenum(sym, sym_name, _teem)
            else:
                # straight copy of (reference to) sym
                exp = sym
        else:
            # ... or a Python wrapper around a function known to use biff.
            (rvtf, mubi, bkey, fnln) = _BIFFDICT[sym_name]
            exp = _biffer(sym, sym_name, rvtf, mubi, bkey, fnln)
        # can't do "if not exp:" because, e.g. AIR_FALSE is 0 but needs to be exported
        if 'unset' == exp:
            raise Exception(f"didn't handle symbol {sym_name}")
        globals()[sym_name] = exp


if 'teem' == __name__:  # being imported
    if _sys.platform == 'darwin':  # mac
        _SHEXT = 'dylib'
    else:
        _SHEXT = 'so'
    try:
        import _teem
    except ModuleNotFoundError:
        print('\n*** teem.py: failed to load shared library wrapper module "_teem", from')
        print('*** a shared object file named something like _teem.cpython-platform.so.')
        print('*** Make sure you first ran: "python3 build_teem.py" to build that.')
        raise
    # The value of this ffi, as opposed to "from cffi import FFI; ffi = FFI()" is that it knows
    # about the various typedefs that were learned to build the CFFI wrapper, which may in turn
    # be useful for setting up calls into libteem
    ffi = _teem.ffi
    # enable access to original un-wrapped things, straight from cffi
    lib = _teem.lib
    # for slight convenience, e.g. when calling nrrdLoad with NULL (default) NrrdIoState
    NULL = _teem.ffi.NULL
    export_teem()
