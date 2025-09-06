_Teem_add_library(${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    flotsam.c
    1op.c 2op.c 3op.c aabplot.c about.c acrop.c affine.c axdelete.c axinfo.c
    axinsert.c axmerge.c axsplit.c basinfo.c block.c built.c ccadj.c ccfind.c
    ccmerge.c ccsettle.c cksum.c cmedian.c convert.c crop.c data.c dering.c
    dhisto.c dice.c diff.c dist.c dnorm.c env.c fft.c flip.c gamma.c grid.c hack.c
    head.c heq.c histax.c histo.c i2w.c ilk.c imap.c inset.c jhisto.c join.c lut.c
    lut2.c make.c minmax.c mlut.c mrmap.c ninspect.c pad.c permute.c project.c
    quantize.c resample.c reshape.c rmap.c save.c shuffle.c slice.c splice.c
    sselect.c subst.c swap.c tile.c unblock.c uncmt.c undos.c unorient.c
    unquantize.c untile.c vidicon.c w2i.c
    # private header
    privateUnrrdu.h
  PUBLIC_HEADERS
    unrrdu.h
)
