_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    aniso.c bimod.c bvec.c chan.c defaultsTen.c enumsTen.c epireg.c estimate.c
    experSpec.c fiber.c fiberMethods.c glyph.c grads.c miscTen.c mod.c
    model1Cylinder.c model1Stick.c model1Tensor2.c model1Unit2D.c model1Vector2D.c
    model2Unit2D.c modelB0.c modelBall.c modelBall1Cylinder.c modelBall1Stick.c
    modelBall1StickEMD.c modelZero.c path.c qglox.c qseg.c tendAbout.c tendAnhist.c
    tendAnplot.c tendAnscale.c tendAnvol.c tendAvg.c tendBfit.c tendBmat.c
    tendEllipse.c tendEpireg.c tendEstim.c tendEval.c tendEvaladd.c tendEvalclamp.c
    tendEvalmult.c tendEvalpow.c tendEvec.c tendEvecrgb.c tendEvq.c tendExp.c
    tendExpand.c tendFiber.c tendFlotsam.c tendGlyph.c tendGrads.c tendHelix.c
    tendLmdemo.c tendLog.c tendMake.c tendMconv.c tendMfit.c tendMsim.c tendNorm.c
    tendPoint.c tendSatin.c tendShrink.c tendSim.c tendSlice.c tendSten.c
    tendTconv.c tendTriple.c tendUnmf.c tenDwiGage.c tenGage.c tenModel.c tensor.c
    triple.c
    # private header
    privateTen.h
  PUBLIC_HEADERS
    ten.h
    tenMacros.h
)
