CC ?= /usr/bin/gcc
LD = /usr/bin/ld 
AR = /usr/bin/ar
RM = /bin/rm -f
INSTALL ?= /usr/bin/install
CHMOD = /bin/chmod

SHEXT = so
OPT_CFLAG = -O2
STATIC_CFLAG = -Wl,-Bstatic
SHARED_CFLAG = -Wl,-Bdynamic
SHARED_LDFLAG = -shared
ARCH_CFLAG = 
ARCH_LDFLAG = 
