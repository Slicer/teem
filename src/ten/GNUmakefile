#
# Teem: Tools to process and visualize scientific data and images
# Copyright (C) 2009--2025  University of Chicago
# Copyright (C) 2005--2008  Gordon Kindlmann
# Copyright (C) 1998--2004  University of Utah
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
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, see <https://www.gnu.org/licenses/>.
#

#### Library name
####
####
L := ten
####
####
####

# boilerplate: set top-level paths and targets if not set, include top-level GNUmakefile,
# and guard reading of per-library variables and (via template.mk) rules
TeemRoot ?= ../..
TeemSrc ?= ..
ifndef DefaultTargets
  DefaultTargets := yes
  bild  : $(L).bild
  test  : $(L).test
  clean : $(L).clean
endif # ifndef DefaultTargets
include $(TeemSrc)/GNUmakefile
ifndef $(L).Included
$(L).Included := yes

#### Describe library L here
####
####
$(L).Depends = echo limn gage unrrdu ell nrrd biff air
$(L).PublicHdr = ten.h tenMacros.h
$(L).PrivateHdr = privateTen.h
$(L).Obj = $(patsubst %.c,%.o, \
    aniso.c bimod.c bvec.c chan.c defaultsTen.c enumsTen.c epireg.c estimate.c \
    experSpec.c fiber.c fiberMethods.c glyph.c grads.c miscTen.c mod.c \
    model1Cylinder.c model1Stick.c model1Tensor2.c model1Unit2D.c model1Vector2D.c \
    model2Unit2D.c modelB0.c modelBall.c modelBall1Cylinder.c modelBall1Stick.c \
    modelBall1StickEMD.c modelZero.c path.c qglox.c qseg.c tendAbout.c tendAnhist.c \
    tendAnplot.c tendAnscale.c tendAnvol.c tendAvg.c tendBfit.c tendBmat.c \
    tendEllipse.c tendEpireg.c tendEstim.c tendEval.c tendEvaladd.c tendEvalclamp.c \
    tendEvalmult.c tendEvalpow.c tendEvec.c tendEvecrgb.c tendEvq.c tendExp.c \
    tendExpand.c tendFiber.c tendFlotsam.c tendGlyph.c tendGrads.c tendHelix.c \
    tendLmdemo.c tendLog.c tendMake.c tendMconv.c tendMfit.c tendMsim.c tendNorm.c \
    tendPoint.c tendSatin.c tendShrink.c tendSim.c tendSlice.c tendSten.c \
    tendTconv.c tendTriple.c tendUnmf.c tenDwiGage.c tenGage.c tenModel.c tensor.c \
    triple.c \
  )
# tendCalc.o nixed
$(L).Test = roistat tg tt tem rotedge tsoid odf-hist to tensorDotDat \
	igrt cntr geode taniso csim ttriple tqgl teigen
####
####
####

# boilerplate: enable "make test/T", template lib L rules, finish include guard
$(foreach T,$($(L).Test),$(eval test/$(T) : ../$(L)/test/$(T)))
include $(TeemSrc)/make/template.mk
endif # ifndef $(L).Included
