CC ?= /usr/bin/cc
LD = /usr/bin/ld 
AR = /usr/bin/ar
RM = /usr/bin/rm -f
INSTALL ?= /usr/local/gnu/bin/install
CHMOD = /usr/bin/chmod

OTHER_CLEAN = so_locations

SHEXT = so
OPT_CFLAG = -O2
STATIC_CFLAG = -Bstatic
SHARED_CFLAG = -Bdynamic
SHARED_LDFLAG = -shared
ifeq ($(SUBARCH),n32)
  ARCH_CFLAG = -n32
  ARCH_LDFLAG = -n32
else
  ifeq ($(SUBARCH),64)
    ARCH_CFLAG = -64
    ARCH_LDFLAG = -64
  else 
    $(error irix6 mode $(SUBARCH) not recognized) 
  endif
endif
