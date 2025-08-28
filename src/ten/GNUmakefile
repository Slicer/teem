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
$(L).Obj = tensor.o chan.o aniso.o glyph.o enumsTen.o grads.o miscTen.o \
	mod.o estimate.o tenGage.o tenDwiGage.o qseg.o path.o qglox.o \
	fiberMethods.o fiber.o epireg.o defaultsTen.o bimod.o bvec.o \
	triple.o experSpec.o tenModel.o modelBall.o model1Stick.o \
	model1Vector2D.o model1Unit2D.o model2Unit2D.o \
	modelBall1Stick.o modelBall1StickEMD.o modelBall1Cylinder.o \
	model1Cylinder.o model1Tensor2.o modelZero.o modelB0.o \
	tendAbout.o \
	tendFlotsam.o tendGrads.o tendAnplot.o tendAnvol.o tendEval.o \
	tendEvec.o tendSten.o tendExpand.o tendEvq.o tendPoint.o \
	tendTriple.o tendTconv.o tendAvg.o \
	tendAnhist.o tendMake.o tendSatin.o tendShrink.o tendGlyph.o \
	tendFiber.o tendEpireg.o tendBmat.o tendEstim.o tendSim.o \
	tendMsim.o tendMfit.o tendMconv.o \
	tendSlice.o tendEllipse.o tendEvecrgb.o tendNorm.o tendAnscale.o \
	tendEvalpow.o tendEvalclamp.o tendEvaladd.o tendEvalmult.o \
	tendHelix.o tendBfit.o \
	tendUnmf.o tendLog.o tendExp.o
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
