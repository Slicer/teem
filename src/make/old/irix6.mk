CC = /usr/bin/cc
LD = /usr/bin/ld 
AR = /usr/bin/ar
RM = /usr/bin/rm -f
INSTALL = /usr/local/gnu/bin/install

SHEXT = so
OPT_CFLAG = -O2
# actually, PIC does matter, in the sense that it doesn't seem
# that you can link a bunch of non-pic .o's into a shared library
# However, it also doesn't seem that it is even possible to build a
# completely static non-pic executable on IRIX, so we pretend that
# PIC doesn't matter, and let the default actions of $(CC) produce
# the PIC which it really wants to make.  Of course, we can still
# enforce -Bstatic when making an executable, which controls which
# libraries we link against, but this doesn't actually produce an
# entirely self-contained static binary
#PIC_MATTERS = true
#PIC_CFLAG = -KPIC
#NONPIC_CFLAG = -non_shared
PIC_MATTERS = false
PIC_CFLAG = 
NONPIC_CFLAG = 
STATIC_CFLAG = -Bstatic
SHARED_CFLAG = -Bdynamic
SHARED_LDFLAG = -shared
ifeq ($(SUBARCH),n32)
  ARCH_CFLAG = -n32
else
  ifeq ($(SUBARCH),64)
    ARCH_CFLAG = -64
  else 
    $(error irix6 mode $(SUBARCH) not recognized) 
  endif
endif
